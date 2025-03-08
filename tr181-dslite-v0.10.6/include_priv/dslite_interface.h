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
#if !defined(__DSLITE_INTERFACE_H__)
#define __DSLITE_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <arpa/inet.h>

#include <amxc/amxc.h>
#include <amxp/amxp.h>
#include <amxd/amxd_dm.h>
#include <netmodel/client.h>

#include "dslite_cares.h"

typedef enum {INVALID, STATIC_IPV6, STATIC_FQDN, DHCPV6} endp_assignment_method_t;
typedef enum {if_status_error, if_status_disabled, if_status_enabled} if_status_t;
typedef enum {TUNNEL_PARAM_ERROR = -1, TUNNEL_PARAM_READY = 0, TUNNEL_PARAM_WAITING} tunnel_param_status_t;

typedef struct dslite_intf_priv {
    char local_addr[INET6_ADDRSTRLEN];
    char remote_addr[INET6_ADDRSTRLEN];
    char* netdev;
    amxd_object_t* if_obj;
    netmodel_query_t* nm_dhcp_query;
    netmodel_query_t* nm_addr_query;
    dns_query_t* dns_query;
    amxp_timer_t* dns_timer;
} dslite_intf_priv_t;

void dslite_intf_priv_t_ctor(amxd_object_t* if_obj);
void cleanup_private_data(amxd_object_t* if_obj);

tunnel_param_status_t get_remote_endp_addr(amxd_object_t* if_obj);
tunnel_param_status_t get_tunnel_parameters(amxd_object_t* if_obj);

void setup_dslite_interface(amxd_object_t* if_obj);

void enable_interface(amxd_object_t* if_obj);
void disable_interface(amxd_object_t* if_obj, if_status_t new_status);


#ifdef __cplusplus
}
#endif

#endif // __DSLITE_INTERFACE_H__