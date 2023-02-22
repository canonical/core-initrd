/* SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright © 2019 VMware, Inc.
 */

#pragma once

#include <inttypes.h>

#include "sd-netlink.h"

#include "conf-parser.h"
#include "hashmap.h"
#include "in-addr-util.h"
#include "networkd-util.h"

typedef struct Link Link;
typedef struct Manager Manager;
typedef struct Network Network;

typedef struct NextHop {
        Network *network;
        Manager *manager;
        Link *link;
        ConfigSection *section;
        NetworkConfigSource source;
        NetworkConfigState state;

        uint8_t protocol;

        uint32_t id;
        bool blackhole;
        int family;
        union in_addr_union gw;
        uint8_t flags;
        int onlink; /* Only used in conf parser and nexthop_section_verify(). */
        Hashmap *group;
} NextHop;

NextHop *nexthop_free(NextHop *nexthop);

void network_drop_invalid_nexthops(Network *network);

int link_drop_managed_nexthops(Link *link);
int link_drop_foreign_nexthops(Link *link);
void link_foreignize_nexthops(Link *link);

int link_request_static_nexthops(Link *link, bool only_ipv4);

int manager_get_nexthop_by_id(Manager *manager, uint32_t id, NextHop **ret);
int manager_rtnl_process_nexthop(sd_netlink *rtnl, sd_netlink_message *message, Manager *m);

DEFINE_NETWORK_CONFIG_STATE_FUNCTIONS(NextHop, nexthop);

CONFIG_PARSER_PROTOTYPE(config_parse_nexthop_id);
CONFIG_PARSER_PROTOTYPE(config_parse_nexthop_gateway);
CONFIG_PARSER_PROTOTYPE(config_parse_nexthop_family);
CONFIG_PARSER_PROTOTYPE(config_parse_nexthop_onlink);
CONFIG_PARSER_PROTOTYPE(config_parse_nexthop_blackhole);
CONFIG_PARSER_PROTOTYPE(config_parse_nexthop_group);
