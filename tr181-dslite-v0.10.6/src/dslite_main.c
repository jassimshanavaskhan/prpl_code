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
#include "dslite.h"
#include "dslite_controller.h"

#include <netmodel/client.h>

#include <debug/sahtrace.h>
#include <debug/sahtrace_macros.h>

#define ME "main"
#define ODL_INCLUDE "?include '${odl.directory}/${name}.odl':'${odl.dm-defaults}';"

static dslite_app_t dslite;

amxd_dm_t* dslite_get_dm(void) {
    return dslite.dm;
}

amxo_parser_t* dslite_get_parser(void) {
    return dslite.parser;
}

static int dslite_init_bus_ctx(void) {
    int status = -1;
    dslite.ctx = amxb_be_who_has("DSLite");
    if(dslite.ctx != NULL) {
        status = 0;
    }
    return status;
}

static int dslite_init(amxd_dm_t* dm, amxo_parser_t* parser) {
    int rv = -1;
    SAH_TRACEZ_INFO(ME, "DSLite init");

    dslite.dm = dm;
    dslite.parser = parser;

    netmodel_initialize();

    rv = dslite_init_bus_ctx();
    when_failed_trace(rv, exit, ERROR, "Unable to initialise bus context, retval = %d", rv);

    rv = dslite_open_cfgctrlrs();
    when_failed_trace(rv, exit, ERROR, "Failed to load all supported config controllers");
    rv = dslite_open_fwctrlrs();
    when_failed_trace(rv, exit, ERROR, "Failed to load all supported firewall controllers");

    // load the defaults after the configuration module is loaded so the
    // default/saved configuration can be applied at start-up.
    rv = amxo_parser_parse_string(parser, ODL_INCLUDE, amxd_dm_get_root(dm));
    when_failed_trace(rv, exit, ERROR, "Failed to load default configs");

exit:
    return rv;
}

static int dslite_cleanup(void) {
    SAH_TRACEZ_INFO(ME, "DSLite cleanup");

    amxm_close_all();
    netmodel_cleanup();

    dslite.dm = NULL;
    dslite.parser = NULL;

    return 0;
}

int _dslite_main(int reason,
                 amxd_dm_t* dm,
                 amxo_parser_t* parser) {
    int rv = -1;

    switch(reason) {
    case 0:     // START
        rv = dslite_init(dm, parser);
        break;
    case 1:     // STOP
        rv = dslite_cleanup();
        break;
    }

    return rv;
}
