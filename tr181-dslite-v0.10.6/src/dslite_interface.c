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

#include <debug/sahtrace.h>
#include <debug/sahtrace_macros.h>

#include "dslite.h"
#include "dslite_interface.h"
#include "dslite_utils.h"
#include "dslite_controller.h"
#include "dslite_cares.h"
#include "dslite_constants.h"
#include "dslite_netmodel.h"
#include "firewall.h"

#define ME "interface"

void dslite_intf_priv_t_ctor(amxd_object_t* if_obj) {
    dslite_intf_priv_t* intf_priv = NULL;

    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");

    intf_priv = (dslite_intf_priv_t*) calloc(1, sizeof(dslite_intf_priv_t));
    when_null_trace(intf_priv, exit, ERROR, "Failed to allocate memory for interface private data");

    intf_priv->if_obj = if_obj;
    intf_priv->nm_dhcp_query = NULL;
    intf_priv->nm_addr_query = NULL;
    if_obj->priv = intf_priv;

exit:
    return;
}

void cleanup_private_data(amxd_object_t* if_obj) {
    dslite_intf_priv_t* intf_priv = NULL;

    when_null_trace(if_obj, exit, INFO, "Interface object is NULL");
    intf_priv = (dslite_intf_priv_t*) if_obj->priv;
    when_null_trace(intf_priv, exit, INFO, "Interface private data isn't allocated");

    netmodel_closeQuery(intf_priv->nm_dhcp_query);
    netmodel_closeQuery(intf_priv->nm_addr_query);

    delete_ares_timers(intf_priv->dns_query);
    dslite_close_dns_query(intf_priv->dns_query);
    amxp_timer_delete(&(intf_priv->dns_timer));

    free(intf_priv->netdev);
    free(intf_priv);
    intf_priv = NULL;

exit:
    return;
}

static int set_static_remote_addr(amxd_object_t* if_obj) {
    dslite_intf_priv_t* intf_priv = NULL;
    char* remote_endp_addr = NULL;
    int rv = -1;

    when_null_trace(if_obj, exit, ERROR, "Interface object is empty");

    intf_priv = (dslite_intf_priv_t*) if_obj->priv;
    when_null_trace(intf_priv, exit, ERROR, "Private interface data is empty");

    remote_endp_addr = amxd_object_get_value(cstring_t, if_obj, "EndpointAddress", NULL);
    when_str_empty_trace(remote_endp_addr, exit, ERROR, "Remote endpoint address was not specified");

    strncpy(intf_priv->remote_addr, remote_endp_addr, INET6_ADDRSTRLEN - 1);
    rv = 0;

exit:
    free(remote_endp_addr);
    return rv;
}

// Updates the remote endpoint address in the interface private data
tunnel_param_status_t get_remote_endp_addr(amxd_object_t* if_obj) {
    tunnel_param_status_t rv = TUNNEL_PARAM_ERROR;
    char* endp_name = NULL;
    dslite_intf_priv_t* intf_priv = NULL;
    endp_assignment_method_t endp_assign_method = INVALID;

    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL, can't get remote endpoint address");

    endp_assign_method = get_endp_assignment_method(if_obj);

    switch(endp_assign_method) {
    case STATIC_IPV6:
        intf_priv = if_obj->priv;
        if(dslite_is_waiting_for_getaddrinfo(intf_priv->dns_query)) {
            dslite_close_dns_query(intf_priv->dns_query);
        }
        if(set_static_remote_addr(if_obj) == 0) {
            rv = TUNNEL_PARAM_READY;
        }
        break;
    case STATIC_FQDN:
        endp_name = amxd_object_get_value(cstring_t, if_obj, "EndpointName", NULL);
        if(resolve_endpoint_name(endp_name, if_obj) == 0) {
            rv = TUNNEL_PARAM_WAITING;
        }
        break;
    case DHCPV6:
        if(nm_open_dhcpv6_query(if_obj) == 0) {
            rv = TUNNEL_PARAM_WAITING;
        }
        break;
    default:
        break;
    }
exit:
    free(endp_name);
    return rv;
}

static int update_dslite_tunnel(amxd_object_t* if_obj) {
    int rv = -1;
    amxc_var_t ctrlr_args;
    amxc_var_t ctrlr_ret;
    amxd_object_t* dslite_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.");

    amxc_var_init(&ctrlr_args);
    amxc_var_init(&ctrlr_ret);

    when_null_trace(dslite_obj, exit, ERROR, "Failed to retrieve DSLite object");
    rv = dslite_build_ctrlr_args_from_object(if_obj, &ctrlr_args);

    when_failed_trace(rv, exit, ERROR, "Failed to build controller args from interface object");
    rv = cfgctrlr_execute_function(dslite_obj, "add_or_modify_tunnel", &ctrlr_args, &ctrlr_ret);
exit:
    amxc_var_clean(&ctrlr_args);
    amxc_var_clean(&ctrlr_ret);
    return rv;
}

static void delete_dslite_tunnel(amxd_object_t* if_obj) {
    int rv = -1;
    amxc_var_t ctrlr_args;
    amxc_var_t ctrlr_ret;
    amxd_object_t* dslite_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.");

    amxc_var_init(&ctrlr_args);
    amxc_var_init(&ctrlr_ret);

    when_null_trace(dslite_obj, exit, ERROR, "Failed to retrieve DSLite object");

    rv = dslite_build_ctrlr_args_from_object(if_obj, &ctrlr_args);
    when_failed_trace(rv, exit, ERROR, "Failed to build controller args from interface object");

    rv = cfgctrlr_execute_function(dslite_obj, "delete_tunnel", &ctrlr_args, &ctrlr_ret);
    when_failed_trace(rv, exit, ERROR, "Failed to execute delete_tunnel function");

exit:
    amxc_var_clean(&ctrlr_args);
    amxc_var_clean(&ctrlr_ret);
    return;
}

static void update_interface_status(amxd_object_t* if_obj, if_status_t status) {
    amxd_trans_t* trans = NULL;
    char* origin = NULL;
    dslite_intf_priv_t* intf_priv = NULL;

    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL, can't update status");

    intf_priv = (dslite_intf_priv_t*) if_obj->priv;
    when_null_trace(intf_priv, exit, ERROR, "Interface object private data is NULL, can't update status");


    trans = dslite_trans_create(if_obj);
    origin = amxd_object_get_value(cstring_t, if_obj, "EndpointAssignmentPrecedence", NULL);

    switch(status) {
    case if_status_disabled:
    //fall through
    case if_status_error:
        amxd_trans_set_value(cstring_t, trans, "EndpointAddressInUse", "");
        amxd_trans_set_value(cstring_t, trans, "LocalAddress", "");
        break;
    case if_status_enabled:
        amxd_trans_set_value(cstring_t, trans, "EndpointAddressInUse", intf_priv->remote_addr);
        amxd_trans_set_value(cstring_t, trans, "LocalAddress", intf_priv->local_addr);
        break;
    }
    amxd_trans_set_value(cstring_t, trans, "Status", status_to_str(status));
    amxd_trans_set_value(cstring_t, trans, "Origin", origin);

    if(dslite_trans_apply(trans) != amxd_status_ok) {
        SAH_TRACEZ_ERROR(ME, "Failed to update interface status");
    }

exit:
    free(origin);
}

void setup_dslite_interface(amxd_object_t* if_obj) {
    int rv = -1;
    char* iface = amxd_object_get_value(cstring_t, if_obj, "WANInterface", NULL);

    when_str_empty_trace(iface, exit, ERROR, "WAN Interface is empty, can't setup DSLite interface");

    rv = update_dslite_tunnel(if_obj);
    when_failed_trace(rv, exit, ERROR, "Failed to setup dslite interface");
    open_firewall(iface);
    rv = toggle_dslite_ip_interfaces(if_obj, true);

    if(rv < 0) {
        SAH_TRACEZ_ERROR(ME, "Failed to toggle IP interfaces");
        close_firewall();
        delete_dslite_tunnel(if_obj);
    }

exit:
    update_interface_status(if_obj, rv == 0 ? if_status_enabled : if_status_error);
    free(iface);
}

void disable_interface(amxd_object_t* if_obj, if_status_t new_status) {
    char* old_status = NULL;

    when_true_trace(new_status == if_status_enabled, exit, WARNING, "Called disabled_interface() with new status: Enabled, doing nothing");
    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");

    old_status = amxd_object_get_value(cstring_t, if_obj, "Status", NULL);
    when_null_trace(old_status, exit, ERROR, "Could not retrieve status of DSLite interfacesetting");

    if(strcmp(old_status, "Enabled") == 0) {
        delete_dslite_tunnel(if_obj);
        close_firewall();
        toggle_dslite_ip_interfaces(if_obj, false);
    }
    update_interface_status(if_obj, new_status);

exit:
    free(old_status);
}

tunnel_param_status_t get_tunnel_parameters(amxd_object_t* if_obj) {
    tunnel_param_status_t rv = TUNNEL_PARAM_ERROR;
    when_failed_trace(nm_open_local_addr_query(if_obj), exit, ERROR, "Failed to get local endpoint address");
    rv = get_remote_endp_addr(if_obj);
    when_false_trace(rv >= 0, exit, ERROR, "Failed to get remote endpoint address");

exit:
    return rv;
}

void enable_interface(amxd_object_t* if_obj) {
    tunnel_param_status_t rv = TUNNEL_PARAM_ERROR;

    dslite_intf_priv_t_ctor(if_obj);
    rv = get_tunnel_parameters(if_obj);
    if(rv == TUNNEL_PARAM_READY) {
        setup_dslite_interface(if_obj);
    } else if(rv == TUNNEL_PARAM_ERROR) {
        SAH_TRACEZ_ERROR(ME, "Failed to collect tunnel parameters");
        disable_interface(if_obj, if_status_error);
    }
}


