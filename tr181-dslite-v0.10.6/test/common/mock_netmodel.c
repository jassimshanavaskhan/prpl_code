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
#include <string.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <amxc/amxc_macros.h>

#include "mock_netmodel.h"

netmodel_query_t* __wrap_netmodel_openQuery_getDHCPOption(const char* intf,
                                                          const char* subscriber,
                                                          const char* type,
                                                          uint16_t tag,
                                                          const char* traverse,
                                                          amxp_slot_fn_t handler,
                                                          void* userdata) {
    amxc_var_t data;
    netmodel_query_t* q = NULL;
    amxc_var_init(&data);

    assert_non_null(traverse);
    assert_non_null(handler);
    assert_string_equal(subscriber, "dslite");
    assert_int_equal(tag, 64);
    when_false(strncmp(intf, "Device.IP.Interface.", 20) == 0, exit);

    q = malloc(sizeof(netmodel_query_t*));

    amxc_var_set(cstring_t, &data, "aftr.be.softathome.com");
    handler(NULL, &data, userdata);

exit:
    amxc_var_clean(&data);
    return q;
}

netmodel_query_t* __wrap_netmodel_openQuery_getAddrs(const char* intf,
                                                     const char* subscriber,
                                                     UNUSED const char* flag,
                                                     const char* traverse,
                                                     amxp_slot_fn_t handler,
                                                     void* userdata) {
    amxc_var_t addr_list;
    amxc_var_t* addr = NULL;
    netmodel_query_t* q = NULL;
    amxc_var_init(&addr_list);
    amxc_var_set_type(&addr_list, AMXC_VAR_ID_LIST);

    assert_non_null(traverse);
    assert_non_null(handler);
    assert_string_equal(subscriber, "dslite");
    when_false(strncmp(intf, "Device.IP.Interface.", 13) == 0, exit);

    q = malloc(sizeof(netmodel_query_t*));

    addr = amxc_var_add(amxc_htable_t, &addr_list, NULL);
    amxc_var_add_key(cstring_t, addr, "Address", "2001:db8:3333:4444:5555:6666:7777:8888");
    amxc_var_add_key(cstring_t, addr, "NetDevName", "eth0");

    handler(NULL, &addr_list, userdata);
    amxc_var_delete(&addr);

exit:
    amxc_var_clean(&addr_list);
    return q;
}

bool __wrap_netmodel_initialize(void) {
    return true;
}

void __wrap_netmodel_closeQuery(netmodel_query_t* query) {
    free(query);
}