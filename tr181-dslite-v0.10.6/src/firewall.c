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
#include <stdlib.h>

#include <amxc/amxc.h>
#include <amxp/amxp.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxm/amxm.h>

#include <debug/sahtrace.h>
#include <debug/sahtrace_macros.h>

#include "firewall.h"
#include "dslite.h"
#include "dslite_constants.h"

#define ME "fwl"
#define FIREWALL_MODULE "fw"
#define FW_ALIAS "dslite"


int open_firewall(const char* iface) {
    int retval = -1;
    amxc_var_t args;
    amxc_var_t ret;
    amxd_status_t status = amxd_status_unknown_error;
    amxd_object_t* dslite_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.");
    char* controller = NULL;

    amxc_var_init(&args);
    amxc_var_init(&ret);

    when_str_empty_trace(iface, exit, ERROR, "No interface specified on which to open firewall rule");
    when_null_trace(dslite_obj, exit, ERROR, "Failed to retrieve dslite object");

    amxc_var_set_type(&args, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(cstring_t, &args, "id", FW_ALIAS);
    amxc_var_add_key(cstring_t, &args, "interface", iface);
    amxc_var_add_key(cstring_t, &args, "ipversion", "6");
    amxc_var_add_key(cstring_t, &args, "protocol", IPIP_PROTO);
    amxc_var_add_key(bool, &args, "enable", true);

    controller = amxd_object_get_value(cstring_t, dslite_obj, "FWController", NULL);

    when_str_empty_trace(controller, exit, ERROR, "Failed to retrieve firewall controller");

    status = amxm_execute_function(controller,
                                   FIREWALL_MODULE,
                                   "set_service",
                                   &args,
                                   &ret);

    when_failed_trace(status, exit, ERROR, "Failed to execute dslite set_service function");
    retval = 0;

exit:
    free(controller);
    amxc_var_clean(&args);
    amxc_var_clean(&ret);
    return retval;
}


int close_firewall(void) {
    int retval = -1;
    amxc_var_t args;
    amxc_var_t ret;
    amxd_status_t status = amxd_status_unknown_error;
    amxd_object_t* dslite_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.");
    char* controller = NULL;

    amxc_var_init(&args);
    amxc_var_init(&ret);

    when_null_trace(dslite_obj, exit, ERROR, "Failed to retrieve dslite object");

    amxc_var_set_type(&args, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(cstring_t, &args, "id", FW_ALIAS);

    controller = amxd_object_get_value(cstring_t, dslite_obj, "FWController", NULL);
    when_str_empty_trace(controller, exit, ERROR, "Failed to retrieve firewall controller");

    status = amxm_execute_function(controller,
                                   FIREWALL_MODULE,
                                   "delete_service",
                                   &args,
                                   &ret);

    when_failed_trace(status, exit, ERROR, "Failed to execute dslite delete_service function");
    retval = 0;
    SAH_TRACEZ_INFO(ME, "removed firewall service[%s]", FW_ALIAS);

exit:
    free(controller);
    amxc_var_clean(&args);
    amxc_var_clean(&ret);

    return retval;
}