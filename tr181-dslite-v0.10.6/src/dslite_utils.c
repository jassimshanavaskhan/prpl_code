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

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <amxc/amxc_macros.h>

#include <debug/sahtrace.h>
#include <debug/sahtrace_macros.h>

#include "dslite.h"
#include "dslite_utils.h"
#include "dslite_interface.h"
#include "dslite_constants.h"

#define ME "utils"

typedef enum {TUNNEL, TUNNELED} if_type_t;

amxd_status_t dslite_trans_apply(amxd_trans_t* trans) {
    amxd_status_t status = amxd_status_unknown_error;
    when_null(trans, exit);

    status = amxd_trans_apply(trans, dslite_get_dm());
    if(status != amxd_status_ok) {
        SAH_TRACEZ_ERROR(ME, "Failed to apply transaction, return %d", status);
    }
    amxd_trans_delete(&trans);

exit:
    return status;
}

amxd_trans_t* dslite_trans_create(const amxd_object_t* const object) {
    amxd_trans_t* trans = NULL;

    when_failed(amxd_trans_new(&trans), exit);
    amxd_trans_set_attr(trans, amxd_tattr_change_ro, true);
    amxd_trans_select_object(trans, object);

exit:
    return trans;
}

static void dns_timer_cb(UNUSED amxp_timer_t* timer, void* priv) {
    amxd_object_t* if_obj = (amxd_object_t*) priv;

    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");
    char* endp_name = amxd_object_get_value(cstring_t, if_obj, "EndpointName", NULL);

    SAH_TRACEZ_INFO(ME, "In DNS timer callback, trying to resolve endpoint name %s again", endp_name);
    resolve_endpoint_name(endp_name, if_obj);
    free(endp_name);
exit:
    return;
}

static void remote_endpoint_addr_query_cb(int result, const char* ip_address, void* priv) {
    dslite_intf_priv_t* intf_priv = NULL;
    amxd_object_t* if_obj;

    intf_priv = (dslite_intf_priv_t*) priv;
    when_null_trace(intf_priv, exit, ERROR, "Interface private data is NULL");
    if_obj = intf_priv->if_obj;
    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");

    if(result == 0) {
        SAH_TRACEZ_INFO(ME, "DNS query succeeded, got IP address %s", ip_address);

        strncpy(intf_priv->remote_addr, ip_address, INET6_ADDRSTRLEN - 1);
        setup_dslite_interface(if_obj);
        amxp_timer_delete(&(intf_priv->dns_timer));
        intf_priv->dns_timer = NULL;
    } else {
        SAH_TRACEZ_WARNING(ME, "DNS query failed");
        disable_interface(if_obj, if_status_error);
        SAH_TRACEZ_INFO(ME, "Starting DNS timer");
        amxp_timer_start(intf_priv->dns_timer, 10000);
    }
exit:
    return;

}

int resolve_endpoint_name(const char* hostname, amxd_object_t* if_obj) {
    int rv = -1;
    dslite_intf_priv_t* intf_priv = NULL;

    when_str_empty_trace(hostname, exit, ERROR, "Hostname is empty");
    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");
    intf_priv = (dslite_intf_priv_t*) if_obj->priv;
    when_null_trace(intf_priv, exit, ERROR, "Private interface data is NULL");

    if(intf_priv->dns_timer == NULL) {
        amxp_timer_new(&(intf_priv->dns_timer), dns_timer_cb, if_obj);
    } else if(amxp_timer_get_state(intf_priv->dns_timer) != amxp_timer_off) {
        amxp_timer_stop(intf_priv->dns_timer);
    }

    if(STRING_EMPTY(intf_priv->local_addr)) {
        SAH_TRACEZ_INFO(ME, "Starting DNS timer because WAN interface doesn't have an IP address yet");
        amxp_timer_start(intf_priv->dns_timer, 5000);
        goto exit;
    }

    dslite_close_dns_query(intf_priv->dns_query);
    intf_priv->dns_query = dslite_open_dns_query(hostname, remote_endpoint_addr_query_cb, intf_priv);

    when_null(intf_priv->dns_query, exit);

    rv = 0;
exit:
    return rv;
}

endp_assignment_method_t get_endp_assignment_method(amxd_object_t* if_obj) {
    endp_assignment_method_t assign_method = INVALID;
    char* endp_assign_prec = amxd_object_get_value(cstring_t, if_obj, "EndpointAssignmentPrecedence", NULL);
    char* endp_addr_type_prec = NULL;

    if(strcmp(endp_assign_prec, "Static") == 0) {
        endp_addr_type_prec = amxd_object_get_value(cstring_t, if_obj, "EndpointAddressTypePrecedence", NULL);
        if(strcmp(endp_addr_type_prec, "IPv6Address") == 0) {
            assign_method = STATIC_IPV6;
        } else if(strcmp(endp_addr_type_prec, "FQDN") == 0) {
            assign_method = STATIC_FQDN;
        }
    } else if(strcmp(endp_assign_prec, "DHCPv6") == 0) {
        assign_method = DHCPV6;
    }

    free(endp_assign_prec);
    free(endp_addr_type_prec);
    return assign_method;
}

const char* status_to_str(if_status_t status) {
    const char* status_str = NULL;

    if(status == if_status_disabled) {
        status_str = "Disabled";
    } else if(status == if_status_enabled) {
        status_str = "Enabled";
    } else {
        status_str = "Error";
    }
    return status_str;
}

if_status_t str_to_status(const char* str) {
    if_status_t status = if_status_error;

    if(strcmp(str, "Enabled") == 0) {
        status = if_status_enabled;
    } else if(strcmp(str, "Disabled") == 0) {
        status = if_status_disabled;
    }
    return status;
}

static int toggle_ip_interface(amxd_object_t* if_obj, bool enable, if_type_t if_type) {
    int rv = -1;
    char* ip_if = NULL;
    const char* if_type_str = if_type == TUNNELED ? "TunneledInterface" : "TunnelInterface";
    amxc_var_t values;
    amxc_var_t ret;

    amxc_var_init(&values);
    amxc_var_init(&ret);

    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");

    ip_if = amxd_object_get_value(cstring_t, if_obj, if_type_str, NULL);
    when_str_empty_trace(ip_if, exit, ERROR, "Failed to get %s", if_type_str);
    SAH_TRACEZ_INFO(ME, "%s is %s", if_type_str, ip_if);


    amxc_var_set_type(&values, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(bool, &values, "Enable", enable);

    if(if_type == TUNNELED) {
        amxc_string_t lower_layers;
        amxc_var_t* tmp = NULL;
        when_failed_trace(amxc_string_init(&lower_layers, 0), exit, ERROR, "Failed to allocate memory for string");

        if(enable) {
            char* if_obj_path = amxd_object_get_path(if_obj, AMXD_OBJECT_INDEXED | AMXD_OBJECT_TERMINATE);

            if(if_obj_path == NULL) {
                SAH_TRACEZ_ERROR(ME, "Failed to get lower layer for tunneled interface %s", ip_if);
                amxc_string_clean(&lower_layers);
                goto exit;
            }
            amxc_string_setf(&lower_layers, "Device.%s", if_obj_path);

            free(if_obj_path);
        } else {
            amxc_string_set(&lower_layers, "");
        }

        tmp = amxc_var_add_new_key(&values, "LowerLayers");
        amxc_var_push(cstring_t, tmp, amxc_string_take_buffer(&lower_layers));
        amxc_string_clean(&lower_layers);
    }

    rv = amxb_set(amxb_be_who_has("IP"), ip_if, &values, &ret, 3);
    when_failed_trace(rv, exit, ERROR, "Failed to enable IP interface %s", ip_if);

exit:
    free(ip_if);
    amxc_var_clean(&values);
    amxc_var_clean(&ret);
    return rv;
}

int toggle_dslite_ip_interfaces(amxd_object_t* if_obj, bool enable) {
    int rv = -1;

    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");

    rv = toggle_ip_interface(if_obj, enable, TUNNELED);
    when_failed_trace(rv, exit, ERROR, "Failed to %s tunneled interface", enable ? "Enable" : "Disable");
    rv = toggle_ip_interface(if_obj, enable, TUNNEL);
    when_failed_trace(rv, exit, ERROR, "Failed to %s tunnel interface", enable ? "Enable" : "Disable");

exit:
    return rv;
}

bool is_dslite_enabled(void) {
    bool enabled = false;
    amxd_object_t* dslite_obj = amxd_dm_findf(dslite_get_dm(), "DSLite");

    when_null_trace(dslite_obj, exit, ERROR, "Couldn't find object DSLite");

    enabled = amxd_object_get_value(bool, dslite_obj, "Enable", NULL);

exit:
    return enabled;
}

void clear_remote_endpoint_properties(dslite_intf_priv_t* intf_priv) {
    when_null_trace(intf_priv, exit, WARNING, "Can't clear remote endpoint properties, private data is NULL");

    memset(intf_priv->remote_addr, 0x0, INET6_ADDRSTRLEN);
    netmodel_closeQuery(intf_priv->nm_dhcp_query);
    intf_priv->nm_dhcp_query = NULL;
    delete_ares_timers(intf_priv->dns_query);
    dslite_close_dns_query(intf_priv->dns_query);
    intf_priv->dns_query = NULL;
    amxp_timer_delete(&(intf_priv->dns_timer));
    intf_priv->dns_timer = NULL;

exit:
    return;
}
