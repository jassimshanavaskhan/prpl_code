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
#include <arpa/inet.h>
#include <string.h>

#include "ares.h"

void ares_timeout(ares_channel channel, struct timeval* maxtv, struct timeval* tv) {
    tv->tv_sec = channel->timeout / 1000;
    tv->tv_usec = (channel->timeout % 1000) * 1000;
}

void ares_destroy(ares_channel channel) {
    free(channel->node);
    channel->node = NULL;
    free(channel);
    channel = NULL;
}

void ares_process_fd(ares_channel channel, ares_socket_t read_fd, ares_socket_t write_fd) {
    struct ares_addrinfo* result = NULL;
    struct ares_addrinfo_node* node = NULL;
    struct sockaddr_in6* addr = NULL;
    int rv = ARES_ENOMEM;

    if(strcmp(channel->node, "aftr.be.softathome.com") == 0) {
        result = calloc(1, sizeof(struct ares_addrinfo));
        node = calloc(1, sizeof(struct ares_addrinfo_node));
        addr = calloc(1, sizeof(struct sockaddr_in6));

        node->ai_family = AF_INET6;
        inet_pton(AF_INET6, "2a02:1802:94:b::47", &(addr->sin6_addr));
        node->ai_addr = (struct sockaddr*) addr;
        result->nodes = node;
        rv = ARES_SUCCESS;
    }

    channel->addr_info_callback(channel->addr_info_callback_data, rv, 0, result);

}

static void ares_freenodes(struct ares_addrinfo_node* head) {
    struct ares_addrinfo_node* current;
    while(head) {
        current = head;
        head = head->ai_next;
        free(current->ai_addr);
        current->ai_addr = NULL;
        free(current);
        current = NULL;
    }
}

static void ares_freecnames(struct ares_addrinfo_cname* head) {
    struct ares_addrinfo_cname* current;
    while(head) {
        current = head;
        head = head->next;
        free(current->alias);
        current->alias = NULL;
        free(current->name);
        current->name = NULL;
        free(current);
        current = NULL;
    }
}

void ares_freeaddrinfo(struct ares_addrinfo* ai) {
    if(ai != NULL) {
        ares_freenodes(ai->nodes);
        ares_freecnames(ai->cnames);
        free(ai);
        ai = NULL;
    }
}

int ares_init_options(ares_channel* channel, struct ares_options* options, int optmask) {
    *channel = calloc(1, sizeof(struct ares_channeldata));
    (*channel)->timeout = options->timeout;
    (*channel)->tries = options->tries;
    (*channel)->sock_state_cb = options->sock_state_cb;
    (*channel)->sock_state_cb_data = options->sock_state_cb_data;
    return 0;
}

void ares_getaddrinfo(ares_channel channel,
                      const char* node,
                      const char* service,
                      const struct ares_addrinfo_hints* hints,
                      ares_addrinfo_callback callback,
                      void* arg) {
    int fd = 5;
    channel->addr_info_callback = callback;
    channel->addr_info_callback_data = arg;
    channel->node = calloc(1, 128);
    strcpy(channel->node, node);
    channel->sock_state_cb(channel->sock_state_cb_data, fd, 1, 0);
    channel->sock_state_cb(channel->sock_state_cb_data, fd, 0, 0);
}

const char* ares_strerror(int error) {
    return "c-ares error occured";
}