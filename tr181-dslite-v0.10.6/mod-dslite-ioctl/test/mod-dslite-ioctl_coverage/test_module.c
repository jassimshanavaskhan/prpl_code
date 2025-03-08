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
#include <stdio.h>
#include <setjmp.h>
#include <stdarg.h>
#include <cmocka.h>

#include <amxc/amxc.h>
#include <amxc/amxc_macros.h>
#include <amxp/amxp.h>

#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_transaction.h>

#include <amxo/amxo.h>
#include <amxm/amxm.h>

#include <linux/sockios.h>

#include "test_module.h"
#include "dslite_ioctl.h"
#include "../common/mock.h"

#define MOD_NAME "target_module"
#define MOD_PATH "../modules_under_test/" MOD_NAME ".so"
#define DSLITE_NETDEV "dslite0"

static amxd_dm_t dm;
static amxo_parser_t parser;

static void handle_events(void) {
    while(amxp_signal_read() == 0) {
    }
}

static void set_argument(amxc_var_t* args, const char* key, const char* value) {
    amxc_var_t* tmp = amxc_var_take_key(args, key);
    if(tmp != NULL) {
        amxc_var_delete(&tmp);
    }
    amxc_var_add_key(cstring_t, args, key, value);

}

int test_setup(UNUSED void** state) {
    amxd_object_t* root_obj = NULL;

    assert_int_equal(amxd_dm_init(&dm), amxd_status_ok);
    assert_int_equal(amxo_parser_init(&parser), 0);

    root_obj = amxd_dm_get_root(&dm);
    assert_non_null(root_obj);

    handle_events();

    return 0;
}

int test_teardown(UNUSED void** state) {
    clear_netdevs();

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);

    return 0;
}

void test_can_load_module(UNUSED void** state) {
    amxm_shared_object_t* so = NULL;
    assert_int_equal(amxm_so_open(&so, MOD_NAME, MOD_PATH), 0);
}

void test_has_functions(UNUSED void** state) {
    assert_true(amxm_has_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel"));
    assert_true(amxm_has_function(MOD_NAME, MOD_DSLITE_CTRL, "delete_tunnel"));
}

void test_add_tunnel(UNUSED void** state) {
    amxc_var_t args;
    amxc_var_t ret;

    amxc_var_init(&args);
    amxc_var_init(&ret);

    // Function fails because no parameters are given
    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel", &args, &ret), -1);

    amxc_var_set_type(&args, AMXC_VAR_ID_HTABLE);
    set_argument(&args, "tunnel_if", DSLITE_NETDEV);
    set_argument(&args, "netdev", "eth0");
    set_argument(&args, "local_endpoint_addr", "2001:db8:3333:4444:5555:6666:7777:8888");
    set_argument(&args, "remote_endpoint_addr", "2001:db8:3333:4444:CCCC:DDDD:EEEE:FFFF");

    // Function fails because tunneled interface netdev doesn't exist
    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel", &args, &ret), -1);
    assert_false(is_if_up());

    init_netdevs();
    add_netdev("eth0");

    set_argument(&args, "local_endpoint_addr", "faulty.local.addr");

    // Function fails because local endpoint address is incorrect
    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel", &args, &ret), -1);
    assert_false(is_if_up());

    set_argument(&args, "local_endpoint_addr", "2001:db8:3333:4444:5555:6666:7777:8888");
    set_argument(&args, "remote_endpoint_addr", "faulty.remote.addr");

    // Function fails because remote endpoint address is incorrect
    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel", &args, &ret), -1);
    assert_false(is_if_up());

    set_argument(&args, "remote_endpoint_addr", "2001:db8:3333:4444:CCCC:DDDD:EEEE:FFFF");

    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel", &args, &ret), 0);
    assert_true(is_if_up());

    amxc_var_clean(&args);
    amxc_var_clean(&ret);

}

void test_modify_tunnel(UNUSED void** state) {
    amxc_var_t args;
    amxc_var_t ret;

    amxc_var_init(&args);
    amxc_var_init(&ret);

    init_netdevs();
    add_netdev("eth0");

    amxc_var_set_type(&args, AMXC_VAR_ID_HTABLE);
    set_argument(&args, "tunnel_if", DSLITE_NETDEV);
    set_argument(&args, "netdev", "eth0");
    set_argument(&args, "local_endpoint_addr", "2001:db8:3333:4444:5555:6666:7777:8888");
    set_argument(&args, "remote_endpoint_addr", "2001:db8:3333:4444:CCCC:DDDD:EEEE:FFFF");

    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel", &args, &ret), 0);
    assert_true(is_if_up());

    set_argument(&args, "local_endpoint_addr", "2001:db8:3333:4444:5555:6666:7777:8888");
    set_argument(&args, "remote_endpoint_addr", "faulty.remote.addr");

    // Function fails because remote endpoint address is incorrect
    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel", &args, &ret), -1);

    set_argument(&args, "remote_endpoint_addr", "2001:db8:3333:4444:CCCC:DDDD:EEEE:9999");

    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel", &args, &ret), 0);
    assert_true(is_if_up());

    amxc_var_clean(&args);
    amxc_var_clean(&ret);
}

void test_delete_tunnel(UNUSED void** state) {
    amxc_var_t args;
    amxc_var_t ret;

    amxc_var_init(&args);
    amxc_var_init(&ret);

    init_netdevs();
    add_netdev("eth0");

    amxc_var_set_type(&args, AMXC_VAR_ID_HTABLE);
    set_argument(&args, "tunnel_if", DSLITE_NETDEV);
    set_argument(&args, "netdev", "eth0");
    set_argument(&args, "local_endpoint_addr", "2001:db8:3333:4444:5555:6666:7777:8888");
    set_argument(&args, "remote_endpoint_addr", "2001:db8:3333:4444:CCCC:DDDD:EEEE:FFFF");

    // Function fails because tunnel interface to delete doesn't exist
    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "delete_tunnel", &args, &ret), -1);

    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "add_or_modify_tunnel", &args, &ret), 0);

    assert_int_equal(amxm_execute_function(MOD_NAME, MOD_DSLITE_CTRL, "delete_tunnel", &args, &ret), 0);
    assert_false(is_if_up());

    amxc_var_clean(&args);
    amxc_var_clean(&ret);
}

void test_can_close_module(UNUSED void** state) {
    assert_int_equal(amxm_close_all(), 0);
}
