/****************************************************************************
**
** SPDX-License-Identifier: BSD-2-Clause-Patent
**
** SPDX-FileCopyrightText: Copyright (c) 2023 SoftAtHome
**
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
** this list of conditions and the following disclaimer in the documentation
** and/or other materials provided with the distribution.
**
** Subject to the terms and conditions of this license, each copyright holder
** and contributor hereby grants to those receiving rights under this license
** a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable
** (except for failure to satisfy the conditions of this license) patent license
** to make, have made, use, offer to sell, sell, import, and otherwise transfer
** this software, where such license applies only to those patent claims, already
** acquired or hereafter acquired, licensable by such copyright holder or contributor
** that are necessarily infringed by:
**
** (a) their Contribution(s) (the licensed copyrights of copyright holders and
** non-copyrightable additions of contributors, in source or binary form) alone;
** or
**
** (b) combination of their Contribution(s) with the work of authorship to which
** such Contribution(s) was added by such copyright holder or contributor, if,
** at the time the Contribution is added, such addition causes such combination
** to be necessarily infringed. The patent license shall not apply to any other
** combinations which include the Contribution.
**
** Except as expressly stated above, no rights or licenses from any copyright
** holder or contributor is granted under this license, whether expressly, by
** implication, estoppel or otherwise.
**
** DISCLAIMER
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
** USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/
#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>
#include <cmocka.h>

#include <amxc/amxc.h>
#include <amxp/amxp.h>

#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_transaction.h>

#include <amxo/amxo.h>

#include "test_events.h"
#include "dslite.h"
#include "dm_validate.h"
#include "dm_dslite.h"
#include "dslite_interface.h"
#include "dslite_utils.h"
#include "dummy_backend.h"

static const char* odl_defs = "../common/tr181-dslite_test.odl";
static const char* odl_config = "../common/mock.odl";

static amxd_dm_t dm;
static amxo_parser_t parser;
static amxb_bus_ctx_t* bus_ctx = NULL;

static void handle_events(void) {
    while(amxp_signal_read() == 0) {
    }
}

// read sig alarm is used to wait for a timer to run out so the callback
// function for this timer will be executed before continuing with the
// next test. In these tests it is used to wait on the delayed uci commit
static void read_sig_alarm(void) {
    sigset_t mask;
    int sfd;
    struct signalfd_siginfo fdsi;
    ssize_t s;

    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);

    sigprocmask(SIG_BLOCK, &mask, NULL);

    sfd = signalfd(-1, &mask, 0);
    s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    assert_int_equal(s, sizeof(struct signalfd_siginfo));
    if(fdsi.ssi_signo == SIGALRM) {
        amxp_timers_calculate();
        amxp_timers_check();
        handle_events();
    }
}

static void check_field_equals(amxd_object_t* if_obj, const char* field, const char* expected_value) {
    char* value = amxd_object_get_value(cstring_t, if_obj, field, NULL);
    assert_string_equal(value, expected_value);
    free(value);
    value = NULL;
}

static void check_status_equals(amxd_object_t* if_obj, const char* expected_status) {
    check_field_equals(if_obj, "Status", expected_status);
}

static void check_origin_equals(amxd_object_t* if_obj, const char* expected_origin) {
    check_field_equals(if_obj, "Origin", expected_origin);
}

static void check_addr_in_use_equals(amxd_object_t* if_obj, const char* expected_addr_in_use) {
    check_field_equals(if_obj, "EndpointAddressInUse", expected_addr_in_use);
}

static void resolve_functions(void) {
    assert_int_equal(amxo_resolver_ftab_add(&parser, "toggle_interface",
                                            AMXO_FUNC(_toggle_interface)), 0);
    assert_int_equal(amxo_resolver_ftab_add(&parser, "endpoint_address_changed",
                                            AMXO_FUNC(_endpoint_address_changed)), 0);
    assert_int_equal(amxo_resolver_ftab_add(&parser, "endpoint_address_type_precedence_changed",
                                            AMXO_FUNC(_endpoint_address_type_precedence_changed)), 0);
    assert_int_equal(amxo_resolver_ftab_add(&parser, "endpoint_name_changed",
                                            AMXO_FUNC(_endpoint_name_changed)), 0);
    assert_int_equal(amxo_resolver_ftab_add(&parser, "toggle_dslite",
                                            AMXO_FUNC(_toggle_dslite)), 0);
    assert_int_equal(amxo_resolver_ftab_add(&parser, "wan_interface_changed",
                                            AMXO_FUNC(_wan_interface_changed)), 0);

}

int test_setup(UNUSED void** state) {
    amxd_object_t* root_obj = NULL;

    assert_int_equal(amxd_dm_init(&dm), amxd_status_ok);
    assert_int_equal(amxo_parser_init(&parser), 0);

    test_register_dummy_be();

    root_obj = amxd_dm_get_root(&dm);
    assert_non_null(root_obj);

    // Create dummy/fake bus connections
    assert_int_equal(amxb_connect(&bus_ctx, "dummy:/tmp/dummy.sock"), 0);
    amxo_connection_add(&parser, amxb_get_fd(bus_ctx), connection_read, "dummy:/tmp/dummy.sock", AMXO_BUS, bus_ctx);
    // Register data model
    amxb_register(bus_ctx, &dm);

    resolve_functions();

    assert_int_equal(amxo_parser_parse_file(&parser, odl_config, root_obj), 0);
    assert_int_equal(amxo_parser_parse_file(&parser, odl_defs, root_obj), 0);

    assert_int_equal(_dslite_main(AMXO_START, &dm, &parser), 0);

    handle_events();

    return 0;
}

int test_teardown(UNUSED void** state) {

    assert_int_equal(_dslite_main(AMXO_STOP, &dm, &parser), 0);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);

    return 0;
}

void test_static_ipv6_interface(UNUSED void** state) {
    amxd_trans_t* trans = NULL;
    amxd_object_t* if_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.InterfaceSetting.1.");
    amxd_object_t* dslite_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "EndpointAssignmentPrecedence", "Static");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(bool, trans, "Enable", 1);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Disabled");
    check_addr_in_use_equals(if_obj, "");

    trans = dslite_trans_create(dslite_obj);
    amxd_trans_set_value(bool, trans, "Enable", 1);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(bool, trans, "Enable", 1);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Error");
    check_origin_equals(if_obj, "Static");
    check_addr_in_use_equals(if_obj, "");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(bool, trans, "Enable", 0);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Disabled");
    check_origin_equals(if_obj, "Static");
    check_addr_in_use_equals(if_obj, "");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "EndpointAddress", "2a02:1802:94:b::47");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(bool, trans, "Enable", 1);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Enabled");
    check_origin_equals(if_obj, "Static");
    check_addr_in_use_equals(if_obj, "2a02:1802:94:b::47");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "WANInterface", "incorrect.wan.interface");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Error");
    check_origin_equals(if_obj, "Static");
    check_addr_in_use_equals(if_obj, "");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "WANInterface", "Device.IP.Interface.2.");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Enabled");
    check_origin_equals(if_obj, "Static");
    check_addr_in_use_equals(if_obj, "2a02:1802:94:b::47");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "EndpointAddress", "2a02:1802:94:b::48");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Enabled");
    check_origin_equals(if_obj, "Static");
    check_addr_in_use_equals(if_obj, "2a02:1802:94:b::48");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(bool, trans, "Enable", 0);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Disabled");
    check_origin_equals(if_obj, "Static");
    check_addr_in_use_equals(if_obj, "");
}

void test_static_fqdn_interface(UNUSED void** state) {
    amxd_trans_t* trans = NULL;
    amxd_object_t* if_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.InterfaceSetting.1.");
    amxd_object_t* dslite_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "EndpointAssignmentPrecedence", "Static");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "EndpointAddressTypePrecedence", "FQDN");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(dslite_obj);
    amxd_trans_set_value(bool, trans, "Enable", 1);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(bool, trans, "Enable", 1);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "EndpointName", "aftr.be.softathome.com");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "EndpointAddressTypePrecedence", "FQDN");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Enabled");
    check_origin_equals(if_obj, "Static");
    check_addr_in_use_equals(if_obj, "2a02:1802:94:b::47");

    read_sig_alarm();

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "EndpointName", "incorrect.aftr.name");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    read_sig_alarm();

    check_status_equals(if_obj, "Error");
    check_origin_equals(if_obj, "Static");
    check_addr_in_use_equals(if_obj, "");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(bool, trans, "Enable", 0);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();
}

void test_dhcpv6_interface(UNUSED void** state) {
    amxd_trans_t* trans = NULL;
    amxd_object_t* if_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.InterfaceSetting.1.");
    amxd_object_t* dslite_obj = amxd_dm_findf(dslite_get_dm(), "DSLite.");

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(cstring_t, trans, "EndpointAssignmentPrecedence", "DHCPv6");
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(dslite_obj);
    amxd_trans_set_value(bool, trans, "Enable", 1);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    trans = dslite_trans_create(if_obj);
    amxd_trans_set_value(bool, trans, "Enable", 1);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Enabled");
    check_origin_equals(if_obj, "DHCPv6");
    check_addr_in_use_equals(if_obj, "2a02:1802:94:b::47");
    read_sig_alarm();

    trans = dslite_trans_create(dslite_obj);
    amxd_trans_set_value(bool, trans, "Enable", 0);
    assert_int_equal(dslite_trans_apply(trans), amxd_status_ok);
    handle_events();

    check_status_equals(if_obj, "Disabled");
    check_origin_equals(if_obj, "DHCPv6");
    check_addr_in_use_equals(if_obj, "");

}
