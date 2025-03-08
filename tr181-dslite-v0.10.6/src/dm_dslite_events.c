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
#include <debug/sahtrace.h>
#include <debug/sahtrace_macros.h>

#include "dslite.h"
#include "dm_dslite.h"
#include "dslite_interface.h"
#include "dslite_utils.h"
#include "dslite_netmodel.h"

#define ME "dslite-event"

void _interface_added(const char* const sig_name,
                      const amxc_var_t* const data,
                      UNUSED void* const priv
                      ) {
    amxd_object_t* obj = amxd_dm_signal_get_object(dslite_get_dm(), data);
    amxd_object_t* if_obj = NULL;
    endp_assignment_method_t endp_assign_method = STATIC_FQDN;
    amxd_param_t* endp_name_param = NULL;

    when_null_trace(obj, exit, ERROR, "Data model object from signal %s is NULL", sig_name);
    if_obj = amxd_object_get_instance(obj, NULL, GET_UINT32(data, "index"));
    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");

    endp_assign_method = get_endp_assignment_method(if_obj);
    endp_name_param = amxd_object_get_param_def(if_obj, "EndpointName");

    when_null_trace(endp_name_param, exit, ERROR, "Couldn't retrieve EndpointName parameter definition");

    if(endp_assign_method == DHCPV6) {
        amxd_param_set_attr(endp_name_param, amxd_pattr_read_only, true);
        amxd_param_set_attr(endp_name_param, amxd_pattr_persistent, false);
    } else {
        amxd_param_set_attr(endp_name_param, amxd_pattr_read_only, false);
        amxd_param_set_attr(endp_name_param, amxd_pattr_persistent, true);
    }
exit:
    return;
}

void _toggle_dslite(UNUSED const char* const sig_name,
                    const amxc_var_t* const data,
                    UNUSED void* const priv
                    ) {
    amxd_object_t* dslite_obj = amxd_dm_signal_get_object(dslite_get_dm(), data);
    bool enable = GETP_BOOL(data, "parameters.Enable.to");

    amxd_object_for_each(instance, it, amxd_object_findf(dslite_obj, ".InterfaceSetting.")) {
        amxd_object_t* if_obj = amxc_container_of(it, amxd_object_t, it);
        if(amxd_object_get_value(bool, if_obj, "Enable", NULL)) {
            if(enable == true) {
                enable_interface(if_obj);
            } else {
                disable_interface(if_obj, if_status_disabled);
                cleanup_private_data(if_obj);
            }
        }
    }
}

void _toggle_interface(UNUSED const char* const sig_name,
                       const amxc_var_t* const data,
                       UNUSED void* const priv
                       ) {
    amxd_object_t* if_obj = amxd_dm_signal_get_object(dslite_get_dm(), data);
    bool enable = GETP_BOOL(data, "parameters.Enable.to");

    if(is_dslite_enabled()) {
        if(enable) {
            enable_interface(if_obj);
        } else {
            disable_interface(if_obj, if_status_disabled);
            cleanup_private_data(if_obj);
        }
    }
    return;
}

void _endpoint_address_changed(UNUSED const char* const sig_name,
                               UNUSED const amxc_var_t* const data,
                               UNUSED void* const priv) {

    amxd_object_t* if_obj = amxd_dm_signal_get_object(dslite_get_dm(), data);
    bool if_enabled = amxd_object_get_value(bool, if_obj, "Enable", NULL);

    when_false_trace(is_dslite_enabled(), exit, INFO, "Endpoint address changed but DSLite is disabled");
    when_false_trace(if_enabled, exit, INFO, "Endpoint address changed but interface is disabled");

    if(get_endp_assignment_method(if_obj) == STATIC_IPV6) {
        if(get_remote_endp_addr(if_obj) == TUNNEL_PARAM_READY) {
            setup_dslite_interface(if_obj);
        } else {
            SAH_TRACEZ_ERROR(ME, "Failed to get remote endpoint address");
            disable_interface(if_obj, if_status_error);
        }
    }
exit:
    return;
}

void _endpoint_assignment_precedence_changed(UNUSED const char* const sig_name,
                                             const amxc_var_t* const data,
                                             UNUSED void* const priv) {

    tunnel_param_status_t rv = TUNNEL_PARAM_ERROR;
    amxd_object_t* if_obj = amxd_dm_signal_get_object(dslite_get_dm(), data);
    bool if_enabled = amxd_object_get_value(bool, if_obj, "Enable", NULL);
    endp_assignment_method_t endp_assign_method = get_endp_assignment_method(if_obj);
    amxd_param_t* endp_name_param = amxd_object_get_param_def(if_obj, "EndpointName");

    when_null_trace(endp_name_param, exit, ERROR, "Couldn't retrieve EndpointName parameter definition");

    if(endp_assign_method == DHCPV6) {
        amxd_param_set_attr(endp_name_param, amxd_pattr_read_only, true);
        amxd_param_set_attr(endp_name_param, amxd_pattr_persistent, false);
    } else {
        amxd_param_set_attr(endp_name_param, amxd_pattr_read_only, false);
        amxd_param_set_attr(endp_name_param, amxd_pattr_persistent, true);
    }

    when_false_trace(is_dslite_enabled(), exit, INFO, "Endpoint assignment precedence changed but DSLite is disabled");
    when_false_trace(if_enabled, exit, INFO, "Endpoint assignment precedence changed but interface is disabled");
    when_null_trace(endp_name_param, exit, ERROR, "Could not retrieve EndpointName parameter");

    clear_remote_endpoint_properties(if_obj->priv);
    rv = get_remote_endp_addr(if_obj);
    if(rv == TUNNEL_PARAM_READY) {
        setup_dslite_interface(if_obj);
    } else if(rv == TUNNEL_PARAM_ERROR) {
        SAH_TRACEZ_ERROR(ME, "Failed to get remote endpoint address");
        disable_interface(if_obj, if_status_error);
    }
exit:
    return;
}

void _endpoint_name_changed(UNUSED const char* const sig_name,
                            const amxc_var_t* const data,
                            UNUSED void* const priv) {
    amxd_object_t* if_obj = amxd_dm_signal_get_object(dslite_get_dm(), data);
    bool if_enabled = amxd_object_get_value(bool, if_obj, "Enable", NULL);
    endp_assignment_method_t endp_assign_method = get_endp_assignment_method(if_obj);
    const char* endp_name = GETP_CHAR(data, "parameters.EndpointName.to");

    when_false_trace(is_dslite_enabled(), exit, INFO, "Endpoint name changed but DSLite is disabled");
    when_false_trace(if_enabled, exit, INFO, "Endpoint name changed but interface is disabled");
    when_str_empty_trace(endp_name, exit, INFO, "Endpoint name is empty");

    if((endp_assign_method == STATIC_FQDN) || (endp_assign_method == DHCPV6)) {
        int rv = -1;
        rv = resolve_endpoint_name(endp_name, if_obj);
        if(rv != 0) {
            disable_interface(if_obj, if_status_error);
        }
    }
exit:
    return;
}

void _endpoint_address_type_precedence_changed(UNUSED const char* const sig_name,
                                               const amxc_var_t* const data,
                                               UNUSED void* const priv) {
    tunnel_param_status_t rv = TUNNEL_PARAM_ERROR;
    amxd_object_t* if_obj = amxd_dm_signal_get_object(dslite_get_dm(), data);
    bool if_enabled = amxd_object_get_value(bool, if_obj, "Enable", NULL);
    endp_assignment_method_t endp_assign_method = get_endp_assignment_method(if_obj);

    when_false_trace(is_dslite_enabled(), exit, INFO, "Endpoint address type precedence changed but DSLite is disabled");
    when_false_trace(if_enabled, exit, INFO, "Endpoint address type precedence changed but interface is disabled");

    if(endp_assign_method == DHCPV6) {
        SAH_TRACEZ_INFO(ME, "Endpoint address type precedence changed but endpoint assignment precedence is DHCPv6");
        goto exit;
    }

    clear_remote_endpoint_properties(if_obj->priv);

    rv = get_remote_endp_addr(if_obj);
    if(rv == TUNNEL_PARAM_READY) {
        setup_dslite_interface(if_obj);
    } else if(rv == TUNNEL_PARAM_ERROR) {
        SAH_TRACEZ_ERROR(ME, "Failed to get remote endpoint address");
        disable_interface(if_obj, if_status_error);
    }

exit:
    return;
}

void _wan_interface_changed(UNUSED const char* const sig_name,
                            const amxc_var_t* const data,
                            UNUSED void* const priv) {

    amxd_object_t* if_obj = amxd_dm_signal_get_object(dslite_get_dm(), data);
    bool if_enabled = amxd_object_get_value(bool, if_obj, "Enable", NULL);

    when_false_trace(is_dslite_enabled(), exit, INFO, "WAN interface changed but DSLite is disabled");
    when_false_trace(if_enabled, exit, INFO, "WAN interface changed but dslite interface is disabled");
    if(nm_open_local_addr_query(if_obj) != 0) {
        SAH_TRACEZ_ERROR(ME, "Failed to get local endpoint address");
        disable_interface(if_obj, if_status_error);
    } else {
        setup_dslite_interface(if_obj);
    }
exit:
    return;
}
