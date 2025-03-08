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

#ifndef __ARES_H__
#define __ARES_H__

#include <sys/time.h>

#define ARES_SOCKET_BAD -1

#define ARES_SUCCESS            0
#define ARES_ENOMEM             15
#define ARES_EDESTRUCTION       16

#define ARES_AI_NOSORT                  (1 << 7)

/* Option mask values */
#define ARES_OPT_TIMEOUTMS       (1 << 1)
#define ARES_OPT_TRIES          (1 << 2)
#define ARES_OPT_SOCK_STATE_CB  (1 << 9)

typedef int ares_socket_t;

/*
 * Similar to addrinfo, but with extra ttl and missing canonname.
 */
struct ares_addrinfo_node {
    int ai_ttl;
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    int ai_addrlen;
    struct sockaddr* ai_addr;
    struct ares_addrinfo_node* ai_next;
};

/*
 * alias - label of the resource record.
 * name - value (canonical name) of the resource record.
 * See RFC2181 10.1.1. CNAME terminology.
 */
struct ares_addrinfo_cname {
    int ttl;
    char* alias;
    char* name;
    struct ares_addrinfo_cname* next;
};

struct ares_addrinfo {
    struct ares_addrinfo_cname* cnames;
    struct ares_addrinfo_node* nodes;
};

struct ares_addrinfo_hints {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
};

typedef void (* ares_sock_state_cb)(void* data,
                                    ares_socket_t socket_fd,
                                    int readable,
                                    int writable);

typedef void (* ares_addrinfo_callback)(void* arg,
                                        int status,
                                        int timeouts,
                                        struct ares_addrinfo* res);

struct ares_channeldata {
    /* Configuration data */
    int flags;
    int timeout; /* in milliseconds */
    int tries;
    char* node;
    ares_addrinfo_callback addr_info_callback;
    void* addr_info_callback_data;
    ares_sock_state_cb sock_state_cb;
    void* sock_state_cb_data;

};

typedef struct ares_channeldata* ares_channel;

struct ares_options {
    int flags;
    int timeout; /* in seconds or milliseconds, depending on options */
    int tries;
    ares_sock_state_cb sock_state_cb;
    void* sock_state_cb_data;
};


void ares_timeout(ares_channel channel, struct timeval* maxtv, struct timeval* tv);
void ares_destroy(ares_channel channel);
void ares_process_fd(ares_channel channel, ares_socket_t read_fd, ares_socket_t write_fd);
void ares_freeaddrinfo(struct ares_addrinfo* ai);
int ares_init_options(ares_channel* channel, struct ares_options* options, int optmask);
void ares_getaddrinfo(ares_channel channel,
                      const char* node,
                      const char* service,
                      const struct ares_addrinfo_hints* hints,
                      ares_addrinfo_callback callback,
                      void* arg);
const char* ares_strerror(int error);


#endif // __ARES_H__