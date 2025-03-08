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
#include <stdio.h>
#include <linux/ip6_tunnel.h>
#include <linux/if_tunnel.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include "cmocka.h"
#include <amxc/amxc.h>
#include <amxc/amxc_htable.h>
#include <amxc/amxc_macros.h>
#include <amxm/amxm.h>

#include "mock.h"

unsigned long request_to_fail = DO_NOT_FAIL;
static int netdev_idx = 1;
static bool if_up = false;

amxc_var_t netdevs;

void init_netdevs() {
    clear_netdevs();
    amxc_var_init(&netdevs);
    amxc_var_set_type(&netdevs, AMXC_VAR_ID_HTABLE);
}

void add_netdev(const char* netdev) {
    amxc_var_add_key(uint32_t, &netdevs, netdev, netdev_idx);
    netdev_idx++;
}

int remove_netdev(const char* netdev) {
    int rv = -1;
    amxc_var_t* tmp = amxc_var_take_key(&netdevs, netdev);

    if(tmp != NULL) {
        amxc_var_delete(&tmp);
        rv = 0;
        if(netdev_idx > 0) {
            netdev_idx--;
        }
    }
    return rv;
}

void clear_netdevs() {
    amxc_var_clean(&netdevs);
}

int get_netdev_idx(const char* netdev) {
    int idx = -1;

    if(netdev == NULL) {
        return idx;
    }
    idx = GETP_UINT32(&netdevs, netdev);

    return (idx == 0 ? -1 : idx);
}

static void set_if_up(bool enable) {
    if_up = enable;
}

bool is_if_up(void) {
    return if_up;
}

int socket(UNUSED int domain, UNUSED int type, UNUSED int protocol) {
    return 1;
}

int ioctl(int d, unsigned long request, char* argp) {
    (void) d;

    struct ifreq* ifr = (struct ifreq*) argp;
    struct ip6_tnl_parm* p;

    switch(request) {
    case 0x89F1 /*SIOCADDTUNNEL*/:
        p = (struct ip6_tnl_parm*) (ifr->ifr_ifru.ifru_data);
        add_netdev(p->name);
        break;
    case 0x89F2 /*SIOCDELTUNNEL*/:
        set_if_up(false);
        return remove_netdev(ifr->ifr_name);
    case 0x89F3 /*SIOCCHGTUNNEL*/:
        assert_int_not_equal(get_netdev_idx(ifr->ifr_name), -1);
        break;
    case 0x8933 /*SIOCGIFINDEX*/:
        return get_netdev_idx(ifr->ifr_name);
        break;
    case 0x8914 /*SIOCSIFFLAGS*/:
        set_if_up(true);
        break;
    }

    return 0;
}