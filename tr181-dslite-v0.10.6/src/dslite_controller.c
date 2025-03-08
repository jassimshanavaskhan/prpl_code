/****************************************************************************
**
** SPDX-License-Identifier: BSD-2-Clause-Patent
**
** SPDX-FileCopyrightText: Copyright (c) 2022 SoftAtHome
**
** Redistribution and use in source and binary forms, with or
** without modification, are permitted provided that the following
** conditions are met:
**
** 1. Redistributions of source code must retain the above copyright
** notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above
** copyright notice, this list of conditions and the following
** disclaimer in the documentation and/or other materials provided
** with the distribution.
**
** Subject to the terms and conditions of this license, each
** copyright holder and contributor hereby grants to those receiving
** rights under this license a perpetual, worldwide, non-exclusive,
** no-charge, royalty-free, irrevocable (except for failure to
** satisfy the conditions of this license) patent license to make,
** have made, use, offer to sell, sell, import, and otherwise
** transfer this software, where such license applies only to those
** patent claims, already acquired or hereafter acquired, licensable
** by such copyright holder or contributor that are necessarily
** infringed by:
**
** (a) their Contribution(s) (the licensed copyrights of copyright
** holders and non-copyrightable additions of contributors, in
** source or binary form) alone; or
**
** (b) combination of their Contribution(s) with the work of
** authorship to which such Contribution(s) was added by such
** copyright holder or contributor, if, at the time the Contribution
** is added, such addition causes such combination to be necessarily
** infringed. The patent license shall not apply to any other
** combinations which include the Contribution.
**
** Except as expressly stated above, no rights or licenses from any
** copyright holder or contributor is granted under this license,
** whether expressly, by implication, estoppel or otherwise.
**
** DISCLAIMER
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
** CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
** INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
** USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
** AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
** ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#include <amxc/amxc_macros.h>

#include <debug/sahtrace.h>
#include <debug/sahtrace_macros.h>

#include "dslite_controller.h"
#include "dslite_interface.h"
#include "dslite.h"
#include "dslite_constants.h"

#define ME "controller"


static int dslite_open_controllers(const char* mod_dir, const amxc_var_t* controllers) {
    int rv = -1;
    amxc_var_t lcontrollers;
    amxc_string_t mod_path;
    amxm_shared_object_t* so = NULL;

    amxc_var_init(&lcontrollers);
    amxc_string_init(&mod_path, 0);

    when_null_trace(controllers, exit, ERROR, "List of supported controllers is empty");
    when_null_trace(mod_dir, exit, ERROR, "Module directory was not specified");

    amxc_var_convert(&lcontrollers, controllers, AMXC_VAR_ID_LIST);
    amxc_var_for_each(controller, &lcontrollers) {
        const char* name = GET_CHAR(controller, NULL);
        amxc_string_setf(&mod_path, "%s/%s.so", mod_dir, name);
        rv = amxm_so_open(&so, name, amxc_string_get(&mod_path, 0));
        SAH_TRACEZ_INFO(ME, "Loading controller '%s' %s", name, rv ? "failed" : "successful");
        when_failed(rv, exit);
    }
    rv = 0;

exit:
    amxc_string_clean(&mod_path);
    amxc_var_clean(&lcontrollers);
    return rv;
}


static int load_controllers(const char* const ctrlrs, const char* const mod_dir) {
    int rv = -1;

    amxd_object_t* dslite_obj = amxd_dm_findf(dslite_get_dm(), "DSLite");
    const amxc_var_t* controllers = amxd_object_get_param_value(dslite_obj, ctrlrs);

    when_null_trace(dslite_obj, exit, ERROR, "Failed to retrieve DSLite object");
    when_null_trace(controllers, exit, ERROR, "Failed to get list of supported controllers");

    rv = dslite_open_controllers(mod_dir, controllers);

exit:
    return rv;
}

int dslite_open_cfgctrlrs(void) {
    int rv = -1;
    amxo_parser_t* parser = dslite_get_parser();
    const char* mod_dir = NULL;
    when_null_trace(parser, exit, ERROR, "Failed to get DSLite data model parser");

    mod_dir = GETP_CHAR(&parser->config, "mod-dir");
    when_null_trace(mod_dir, exit, ERROR, "Failed to get controller module directory");

    rv = load_controllers("SupportedControllers", mod_dir);

exit:
    return rv;
}

int dslite_open_fwctrlrs(void) {
    int rv = -1;
    amxo_parser_t* parser = dslite_get_parser();
    const char* mod_dir = NULL;
    when_null_trace(parser, exit, ERROR, "Failed to get DSLite data model parser");

    mod_dir = GETP_CHAR(&parser->config, "sys-mod-dir");
    when_null_trace(mod_dir, exit, ERROR, "Failed to get controller module directory");

    rv = load_controllers("SupportedFWControllers", mod_dir);

exit:
    return rv;
}

/**
 * @brief Executes a function on the DSLite configuration controller
 * @param obj The object from which to get the module name to use, if NULL mod-dslite-ioctl is used
 * @param func_name The name of the function to execute
 * @param args variant structure of the arguments to the function. All functions expect the same fields in the variant!
 * @param ret variant structure of the return values from the function
 * @return 0 if the function executed without errors, otherwise it return -1
 */
int cfgctrlr_execute_function(amxd_object_t* obj,
                              const char* const func_name,
                              amxc_var_t* args,
                              amxc_var_t* ret) {
    int status = -1;
    const amxc_var_t* dslite_ctrl = amxd_object_get_param_value(obj, "Controller");
    const char* dslite_ctrlr_name = NULL;

    // find controller for the configuration
    if(obj == NULL) {
        // If no object was given use the default Controller
        dslite_ctrl = amxd_object_get_param_value(obj, "DefaultController");
    }
    dslite_ctrlr_name = GET_CHAR(dslite_ctrl, NULL);
    when_str_empty_trace(dslite_ctrlr_name, exit, ERROR, "Could not find the controller for this configuration");

    // Call controller-specific implementation
    if(amxm_execute_function(dslite_ctrlr_name, "dslite-ctrl", func_name, args, ret) != 0) {
        SAH_TRACEZ_ERROR(ME, "Could not execute '%s' on the '%s' controller", func_name, dslite_ctrlr_name);
        goto exit;
    } else {
        status = 0;
    }

exit:
    return status;
}

int dslite_build_ctrlr_args_from_object(amxd_object_t* if_obj,
                                        amxc_var_t* ctrlr_args) {
    int rv = -1;
    dslite_intf_priv_t* intf_priv = NULL;

    when_null_trace(if_obj, exit, ERROR, "No interface object provided");
    intf_priv = (dslite_intf_priv_t*) if_obj->priv;
    when_null_trace(intf_priv, exit, ERROR, "No interface private data provided");

    when_null_trace(intf_priv->netdev, exit, ERROR, "Tunneled interface netdev name is NULL");

    amxc_var_set_type(ctrlr_args, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(cstring_t, ctrlr_args, "netdev", intf_priv->netdev);
    amxc_var_add_key(cstring_t, ctrlr_args, "tunnel_if", DSLITE_NETDEV);
    amxc_var_add_key(cstring_t, ctrlr_args, "remote_endpoint_addr", intf_priv->remote_addr);
    amxc_var_add_key(cstring_t, ctrlr_args, "local_endpoint_addr", intf_priv->local_addr);

    rv = 0;

exit:
    return rv;
}