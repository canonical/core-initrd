/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

typedef struct Address Address;
typedef struct Link Link;

int ipv4acd_configure(Address *address);
int ipv4acd_update_mac(Link *link);
int ipv4acd_start(Link *link);
int ipv4acd_stop(Link *link);
int ipv4acd_set_ifname(Link *link);
