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

#include <amxc/amxc_macros.h>
#include <debug/sahtrace.h>
#include <debug/sahtrace_macros.h>

#include "dslite.h"
#include "dslite_cares.h"

#define ME "dslite-ares"

#define ARES_TIMEOUT_MS 5000
#define ARES_TRIES 1

static amxc_var_t* list_get(amxc_var_t* list, int value) {
    amxc_var_t* found_var = NULL;
    amxc_var_for_each(var, list) {
        int val = GET_INT32(var, NULL);
        if(val == value) {
            found_var = var;
            break;
        }
    }
    return found_var;
}

static void dslite_ares_destroy_channel_cb(UNUSED amxp_timer_t* timer, void* priv) {
    dns_query_t* dns_query = (dns_query_t*) priv;
    when_null_trace(dns_query, exit, ERROR, "DNS query is NULL, cannot destroy ares channel");
    when_null_trace(dns_query->ares_channel, exit, ERROR, "Ares channel is NULL, cannot be destroyed");

    SAH_TRACEZ_INFO(ME, "Destroy timer expired");

    ares_destroy(dns_query->ares_channel);
    dns_query->ares_channel = NULL;
    amxp_timer_delete(&(dns_query->timer_destroy_ares_channel));


exit:
    return;
}

static void dslite_ares_timeout_cb(UNUSED amxp_timer_t* timer, void* priv) {
    dns_query_t* dns_query = (dns_query_t*) priv;

    SAH_TRACEZ_INFO(ME, "DNS timer expired");

    when_null_trace(dns_query, exit, INFO, "DNS Query is NULL");
    when_null_trace(dns_query->ares_channel, exit, INFO, "Ares Channel is NULL");
    amxp_timer_stop(dns_query->timer_dns);
    ares_process_fd(dns_query->ares_channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);

exit:
    return;
}

static void dslite_ares_start_timer(dns_query_t* dns_query) {
    int wait_time_ms;
    when_null_trace(dns_query, exit, ERROR, "DNS query is NULL so timer cannot be started");

    if(dns_query->timer_dns == NULL) {
        amxp_timer_new(&dns_query->timer_dns, dslite_ares_timeout_cb, dns_query);
    }
    when_null_trace(dns_query->timer_dns, exit, ERROR, "DNS query timer is NULL");

    wait_time_ms = (ARES_TIMEOUT_MS * ARES_TRIES) + 500;
    SAH_TRACEZ_INFO(ME, "Start timer[%d ms]", wait_time_ms);

    amxp_timer_start(dns_query->timer_dns, wait_time_ms);

exit:
    return;
}

static void dslite_ares_addrinfo_cb(void* arg, int status, UNUSED int timeouts, struct ares_addrinfo* result) {

    dns_query_t* dns_query = (dns_query_t*) arg;
    char ip_address[INET6_ADDRSTRLEN] = {'\0'};
    int rv = -1;

    when_null_trace(dns_query, exit, ERROR, "DNS query is NULL");
    when_null_trace(dns_query->cb_fn, exit, ERROR, "DNS query callback function is NULL");
    amxp_timer_stop(dns_query->timer_dns);

    if(status != ARES_SUCCESS) {
        SAH_TRACEZ_WARNING(ME, "DNS lookup failed: %s", ares_strerror(status));
    } else {
        for(struct ares_addrinfo_node* ai = result->nodes; ai != NULL; ai = ai->ai_next) {
            int family = ai->ai_family == AF_INET ? 4 : 6;

            if(family == 6) {
                struct sockaddr_in6* sa = (struct sockaddr_in6*) ai->ai_addr;
                inet_ntop(AF_INET6, &sa->sin6_addr, ip_address, INET6_ADDRSTRLEN);
                rv = 0;
                break;
            }
        }
    }
    dns_query->cb_fn(rv, ip_address, dns_query->cb_arg);

exit:
    ares_freeaddrinfo(result);
    return;
}

static void dslite_ares_eventloop_cb(int fd, void* data) {
    dns_query_t* dns_query = (dns_query_t*) data;
    when_null_trace(dns_query, exit, ERROR, "DNS query is NULL");

    SAH_TRACEZ_INFO(ME, "Process %d", fd);

    ares_process_fd(dns_query->ares_channel, fd, ARES_SOCKET_BAD);
exit:
    return;
}

static void dslite_ares_sock_cb(void* data, ares_socket_t socket_fd, int readable, int writable) {
    dns_query_t* dns_query = (dns_query_t*) data;
    when_null_trace(dns_query, exit, ERROR, "DNS query is NULL");

    SAH_TRACEZ_INFO(ME, "fd %d R %d W %d", socket_fd, readable, writable);

    if(readable + writable > 0) {
        if(dns_query->ares_fds == NULL) {
            amxc_var_new(&dns_query->ares_fds);
            amxc_var_set_type(dns_query->ares_fds, AMXC_VAR_ID_LIST);
        }
        if(list_get(dns_query->ares_fds, socket_fd) == NULL) {
            amxc_var_add(int32_t, dns_query->ares_fds, socket_fd);
            SAH_TRACEZ_INFO(ME, "Adding fd %d to event loop", socket_fd);
            amxo_connection_add(dslite_get_parser(), socket_fd, dslite_ares_eventloop_cb, NULL, AMXO_CUSTOM, dns_query);

            dslite_ares_start_timer(dns_query);
        } else {
            SAH_TRACEZ_INFO(ME, "Ares fd list already contains %d", socket_fd);
        }
    } else {
        amxc_var_t* found_var = list_get(dns_query->ares_fds, socket_fd);

        if(found_var != NULL) {
            amxo_connection_remove(dslite_get_parser(), socket_fd);
            amxc_var_delete(&found_var);
        } else {
            SAH_TRACEZ_INFO(ME, "%d not in fd list", socket_fd);
        }

        if(amxc_var_get_first(dns_query->ares_fds) == NULL) {
            amxp_timer_start(dns_query->timer_destroy_ares_channel, 100);
            SAH_TRACEZ_INFO(ME, "Destroy timer started");
        }
    }
exit:
    return;
}

static int init_ares_channel(dns_query_t* dns_query) {
    struct ares_options ares_options;
    int optmask = 0;
    int rv = ARES_ENOMEM;

    when_null_trace(dns_query, exit, ERROR, "DNS query is NULL so can't initialize ares channel");

    ares_options.sock_state_cb = dslite_ares_sock_cb;
    ares_options.sock_state_cb_data = dns_query;
    ares_options.tries = ARES_TRIES;
    ares_options.timeout = ARES_TIMEOUT_MS; // 5 s
    optmask = ARES_OPT_SOCK_STATE_CB | ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES;

    rv = ares_init_options(&dns_query->ares_channel,
                           &ares_options,
                           optmask);

exit:
    return rv;
}

static dns_query_t* dns_query_ctor(dns_query_cb callback_fn, void* priv) {
    dns_query_t* dns_query = NULL;

    dns_query = calloc(1, sizeof(dns_query_t));
    when_null_trace(dns_query, exit, ERROR, "Failed to allocate memory for DNS query");

    if(dns_query->timer_destroy_ares_channel == NULL) {
        amxp_timer_new(&dns_query->timer_destroy_ares_channel, dslite_ares_destroy_channel_cb, dns_query);
    }

    if(init_ares_channel(dns_query) != ARES_SUCCESS) {
        if(dns_query->timer_destroy_ares_channel != NULL) {
            amxp_timer_delete(&dns_query->timer_destroy_ares_channel);
        }
        free(dns_query);
        dns_query = NULL;
        goto exit;
    }

    dns_query->cb_fn = callback_fn;
    dns_query->cb_arg = priv;

exit:
    return dns_query;
}

dns_query_t* dslite_open_dns_query(const char* hostname, dns_query_cb callback_fn, void* priv) {
    struct ares_addrinfo_hints hints;
    dns_query_t* dns_query = NULL;

    SAH_TRACEZ_INFO(ME, "Opening DNS query");

    when_str_empty_trace(hostname, exit, ERROR, "Hostname is empty");

    dns_query = dns_query_ctor(callback_fn, priv);
    when_null_trace(dns_query, exit, ERROR, "failed to construct dns query");

    memset(&hints, 0, sizeof(struct ares_addrinfo_hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = ARES_AI_NOSORT;

    ares_getaddrinfo(dns_query->ares_channel, hostname, NULL, &hints, dslite_ares_addrinfo_cb, dns_query);

exit:
    return dns_query;
}

void dslite_close_dns_query(dns_query_t* dns_query) {
    when_null(dns_query, exit);

    if(dns_query->ares_fds != NULL) {
        amxc_var_for_each(var, dns_query->ares_fds) {
            int fd = GET_INT32(var, NULL);
            SAH_TRACEZ_INFO(ME, "Removing fd %d from event loop", fd);
            amxo_connection_remove(dslite_get_parser(), fd);
            amxc_var_delete(&var);
        }
        amxc_var_delete(&dns_query->ares_fds);
    }
    if((dns_query->ares_channel != NULL) && (amxp_timer_get_state(dns_query->timer_destroy_ares_channel) == amxp_timer_off)) {
        ares_destroy(dns_query->ares_channel);
        dns_query->ares_channel = NULL;
    }

    dns_query->cb_fn = NULL;
    dns_query->cb_arg = NULL;
    free(dns_query);

exit:
    return;
}

bool dslite_is_waiting_for_getaddrinfo(dns_query_t* dns_query) {
    bool rv = false;
    when_null(dns_query, exit);
    if(dns_query->ares_channel != NULL) {
        rv = true;
    }
exit:
    return rv;
}

void delete_ares_timers(dns_query_t* dns_query) {
    when_null(dns_query, exit);
    amxp_timer_delete(&(dns_query->timer_dns));
    amxp_timer_delete(&(dns_query->timer_destroy_ares_channel));
exit:
    return;
}
