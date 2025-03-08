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
#include <errno.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/if.h>
#include <linux/ip6_tunnel.h>
#include <linux/if_tunnel.h>

#include <debug/sahtrace.h>
#include <debug/sahtrace_macros.h>

#include "dslite_ioctl.h"

static amxm_module_t* mod = NULL;

static int get_tunnel_params(amxc_var_t* args, struct ip6_tnl_parm* p) {
    int rv = -1;
    const char* local_endpoint_addr = GETP_CHAR(args, "local_endpoint_addr");
    const char* remote_endpoint_addr = GETP_CHAR(args, "remote_endpoint_addr");
    const char* tunnel_if = GETP_CHAR(args, "tunnel_if");

    when_str_empty_trace(tunnel_if, error, ERROR, "Could not retrieve tunnel interface name");
    when_str_empty_trace(remote_endpoint_addr, error, ERROR, "Could not retrieve remote endpoint address");
    when_str_empty_trace(local_endpoint_addr, error, ERROR, "Could not retrieve local endpoint address");

    memset(p, 0, sizeof(struct ip6_tnl_parm));

    /* ipv4 in ipv6 is ipip */
    p->proto = IPPROTO_IPIP;

    /* default values */
    p->encap_limit = IPV6_DEFAULT_TNL_ENCAP_LIMIT;
    p->hop_limit = 64;
    p->flags = IP6_TNL_F_USE_ORIG_TCLASS | IP6_TNL_F_IGN_ENCAP_LIMIT;

    strncpy(p->name, tunnel_if, IFNAMSIZ - 1);

    /* setup local and remote address */
    if(inet_pton(AF_INET6, local_endpoint_addr, &p->laddr) == 0) {
        SAH_TRACE_ERROR("Unable to parse local address %s", local_endpoint_addr);
        goto error;
    }

    if(inet_pton(AF_INET6, remote_endpoint_addr, &p->raddr) == 0) {
        SAH_TRACE_ERROR("Unable to parse remote address %s", remote_endpoint_addr);
        goto error;
    }

    rv = 0;

error:
    return rv;
}

static int set_if_up(const char* if_name) {
    int fd;
    struct ifreq ifr;
    int rv = -1;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(fd < 0) {
        SAH_TRACE_ERROR("Unable to bring %s up, failed to open socket", if_name);
        goto exit;
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);

    ifr.ifr_flags |= IFF_UP;
    when_failed_trace(ioctl(fd, SIOCSIFFLAGS, &ifr), exit_close_fd, ERROR, "Failed to set interface %s up", if_name);
    rv = 0;
    SAH_TRACE_INFO("Set interface %s up", if_name);

exit_close_fd:
    close(fd);
exit:
    return rv;
}

static int tunnel_operate(amxc_var_t* args, unsigned long cmd) {
    int rv = -1;
    struct ip6_tnl_parm p;
    struct ifreq ifr;
    int fd = -1;
    const char* tunnel_if = GETP_CHAR(args, "tunnel_if");
    const char* netdev = GETP_CHAR(args, "netdev");

    when_str_empty_trace(tunnel_if, error, ERROR, "Could not retrieve tunnel interface name");
    when_str_empty_trace(netdev, error, ERROR, "Could not retrieve netdev name");

    if(cmd != SIOCDELTUNNEL) {
        if(get_tunnel_params(args, &p) < 0) {
            SAH_TRACE_ERROR("Failed to initialize tunnel params for %s", tunnel_if);
            goto error;
        }
    }
    memset(&ifr, 0, sizeof(struct ifreq));

    fd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
    if(fd < 0) {
        goto error;
    }

    /* get netdev index of tunneled interface */
    strncpy(ifr.ifr_name, netdev, IFNAMSIZ - 1);
    if(ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        SAH_TRACE_ERROR("Failed to get index of netdev %s", netdev);
        goto error_close_fd;
    }

    p.link = ifr.ifr_ifindex;
    memset(&ifr, 0, sizeof(struct ifreq));

    if(cmd == SIOCADDTUNNEL) {
        strncpy(ifr.ifr_name, "ip6tnl0", IFNAMSIZ - 1);
    } else {
        strncpy(ifr.ifr_name, tunnel_if, IFNAMSIZ - 1);
    }

    strncpy(p.name, tunnel_if, IFNAMSIZ - 1);
    ifr.ifr_ifru.ifru_data = &p;

    SAH_TRACE_INFO("operating tunnel %s", tunnel_if);
    rv = ioctl(fd, cmd, &ifr);
    if(rv != 0) {
        SAH_TRACE_ERROR("unable to operate tunnel %s with error %s", tunnel_if, strerror(errno));
        goto error_close_fd;
    } else {
        if((cmd == SIOCADDTUNNEL) || (cmd == SIOCCHGTUNNEL)) {
            rv = set_if_up(tunnel_if);
        }
    }

error_close_fd:
    close(fd);

error:
    return rv;

}

// Returns -1 on error, 0 if no tunnel exists and 1 if a tunnel already exists
static int tunnel_exists(amxc_var_t* args) {
    int rv = -1;
    struct ifreq ifr;
    int fd = -1;
    const char* tunnel_if = GETP_CHAR(args, "tunnel_if");
    when_null_trace(tunnel_if, error, ERROR, "Could not retrieve tunnel interface name");

    memset(&ifr, 0, sizeof(struct ifreq));

    fd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
    if(fd < 0) {
        SAH_TRACE_ERROR("unable to check tunnel %s", tunnel_if);
        goto error;
    }

    /* get netdev index */
    strncpy(ifr.ifr_name, tunnel_if, IFNAMSIZ - 1);
    if(ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        rv = 0;
        goto error_close_fd;
    }

    rv = 1;

error_close_fd:
    close(fd);
error:
    return rv;
}

static int add_or_modify_tunnel(UNUSED const char* function_name,
                                amxc_var_t* args,
                                UNUSED amxc_var_t* ret) {
    int rv = -1;
    int tunnel_exist = tunnel_exists(args);
    if(tunnel_exist == 1) {
        when_failed_trace(tunnel_operate(args, SIOCCHGTUNNEL), exit, ERROR, "Could not modify IPv6 tunnel");
    } else if(tunnel_exist == 0) {
        when_failed_trace(tunnel_operate(args, SIOCADDTUNNEL), exit, ERROR, "Could not add IPv6 tunnel");
    } else {
        SAH_TRACE_ERROR("Could not check if IPv6 tunnel already exists");
        goto exit;
    }

    rv = 0;
exit:
    return rv;
}

static int delete_tunnel(UNUSED const char* function_name,
                         amxc_var_t* args,
                         UNUSED amxc_var_t* ret) {
    int rv = -1;
    when_failed_trace(tunnel_operate(args, SIOCDELTUNNEL), exit, ERROR, "Could not delete IPv6 tunnel");
    rv = 0;

exit:
    return rv;
}

static AMXM_CONSTRUCTOR dslite_ioctl_start(void) {
    amxm_shared_object_t* so = amxm_so_get_current();

    amxm_module_register(&mod, so, MOD_DSLITE_CTRL);
    amxm_module_add_function(mod, "add_or_modify_tunnel", add_or_modify_tunnel);
    amxm_module_add_function(mod, "delete_tunnel", delete_tunnel);

    return 0;
}

static AMXM_DESTRUCTOR dslite_ioctl_stop(void) {
    return 0;
}

