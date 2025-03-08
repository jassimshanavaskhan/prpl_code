/****************************************************************************
**
** SPDX-License-Identifier: BSD-2-Clause-Patent
**
** SPDX-FileCopyrightText: Copyright (c) 2023 SoftAtHome
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

#include <netmodel/client.h>

#include "dslite_utils.h"
#include "dslite_interface.h"
#include "dslite_constants.h"
#include "dslite_netmodel.h"

#define ME "dslite-netmodel"

static void nm_dhcp_cb(UNUSED const char* sig_name, const amxc_var_t* result, void* userdata) {
    dslite_intf_priv_t* intf_priv = NULL;
    amxd_object_t* if_obj = NULL;
    const char* endp_name = GET_CHAR(result, NULL);
    char* curr_endp_name = NULL;
    amxd_trans_t* trans = NULL;

    when_str_empty_trace(endp_name, exit, WARNING, "No endpoint name provided by DHCPv6 server");
    when_null_trace(userdata, exit, ERROR, "No userdata provided");
    intf_priv = (dslite_intf_priv_t*) userdata;
    if_obj = intf_priv->if_obj;

    curr_endp_name = amxd_object_get_value(cstring_t, if_obj, "EndpointName", NULL);

    SAH_TRACEZ_INFO(ME, "Endpoint name '%s' found, current endpoint name is %s", endp_name, curr_endp_name);

    if(strcmp(endp_name, curr_endp_name) == 0) {
        if(resolve_endpoint_name(endp_name, if_obj) != 0) {
            disable_interface(if_obj, if_status_error);
        }
    } else {
        trans = dslite_trans_create(if_obj);
        amxd_trans_set_value(cstring_t, trans, "EndpointName", endp_name);

        if(dslite_trans_apply(trans) != amxd_status_ok) {
            SAH_TRACEZ_ERROR(ME, "Failed to update EndpointName");
        }
    }
    free(curr_endp_name);

exit:
    return;
}

static void nm_local_addr_cb(UNUSED const char* sig_name, const amxc_var_t* result, void* userdata) {

    dslite_intf_priv_t* intf_priv = NULL;
    const char* addr = GETP_CHAR(result, "0.Address");
    const char* netdev = GETP_CHAR(result, "0.NetDevName");
    amxd_object_t* if_obj = NULL;
    char* if_status = NULL;

    when_null_trace(userdata, exit, ERROR, "No userdata provided");
    intf_priv = (dslite_intf_priv_t*) userdata;

    if(STRING_EMPTY(addr)) {
        SAH_TRACEZ_WARNING(ME, "No IP address found for wan interface");
        disable_interface(intf_priv->if_obj, if_status_error);
        goto exit;
    }
    if(STRING_EMPTY(netdev)) {
        SAH_TRACEZ_WARNING(ME, "No NetDev found for wan interface");
        disable_interface(intf_priv->if_obj, if_status_error);
        goto exit;
    }

    strncpy(intf_priv->local_addr, addr, INET6_ADDRSTRLEN - 1);
    free(intf_priv->netdev);
    intf_priv->netdev = strdup(netdev);

    if_obj = intf_priv->if_obj;
    if_status = amxd_object_get_value(cstring_t, if_obj, "Status", NULL);

    if(str_to_status(if_status) == if_status_disabled) {
        SAH_TRACEZ_INFO(ME, "Local address found but interface is disabled");
        goto exit;
    }

    if(!STRING_EMPTY(intf_priv->remote_addr)) {
        setup_dslite_interface(if_obj);
    }

exit:
    free(if_status);
    return;
}

int nm_open_dhcpv6_query(amxd_object_t* if_obj) {
    int rv = -1;
    dslite_intf_priv_t* intf_priv = NULL;
    char* wan_iface = NULL;

    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");
    intf_priv = (dslite_intf_priv_t*) if_obj->priv;
    when_null_trace(intf_priv, exit, ERROR, "Private interface data is NULL");

    wan_iface = amxd_object_get_value(cstring_t, if_obj, "WANInterface", NULL);
    when_str_empty_trace(wan_iface, exit, ERROR, "Couldn't retrieve WAN interface");

    netmodel_closeQuery(intf_priv->nm_dhcp_query);
    intf_priv->nm_dhcp_query = netmodel_openQuery_getDHCPOption(wan_iface, "dslite", "req6", OPTION_AFTR_NAME, "down", nm_dhcp_cb, intf_priv);

    when_null_trace(intf_priv->nm_dhcp_query, exit, ERROR, "Failed to open DHCP option %d query on '%s'",
                    OPTION_AFTR_NAME, wan_iface);
    rv = 0;

exit:
    free(wan_iface);
    return rv;
}

int nm_open_local_addr_query(amxd_object_t* if_obj) {
    int rv = -1;
    dslite_intf_priv_t* intf_priv = NULL;
    char* wan_iface = NULL;

    when_null_trace(if_obj, exit, ERROR, "Interface object is NULL");
    intf_priv = (dslite_intf_priv_t*) if_obj->priv;
    when_null_trace(intf_priv, exit, ERROR, "Interface private data is NULL");

    wan_iface = amxd_object_get_value(cstring_t, if_obj, "WANInterface", NULL);
    when_str_empty_trace(wan_iface, exit, ERROR, "Couldn't retrieve WAN interface");

    netmodel_closeQuery(intf_priv->nm_addr_query);
    intf_priv->nm_addr_query = netmodel_openQuery_getAddrs(wan_iface, "dslite", "ipv6 && global", "down", nm_local_addr_cb, intf_priv);

    when_null_trace(intf_priv->nm_addr_query, exit, ERROR, "Failed to open addr query on %s", wan_iface);
    rv = 0;

exit:
    free(wan_iface);
    return rv;
}
