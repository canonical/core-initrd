/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <linux/netlink.h>

#include "sd-netlink.h"

#include "list.h"
#include "netlink-types.h"
#include "prioq.h"
#include "time-util.h"

#define NETLINK_DEFAULT_TIMEOUT_USEC ((usec_t) (25 * USEC_PER_SEC))

#define NETLINK_RQUEUE_MAX 64*1024

#define NETLINK_CONTAINER_DEPTH 32

struct reply_callback {
        sd_netlink_message_handler_t callback;
        usec_t timeout;
        uint32_t serial;
        unsigned prioq_idx;
};

struct match_callback {
        sd_netlink_message_handler_t callback;
        uint32_t *groups;
        size_t n_groups;
        uint16_t type;
        uint8_t cmd; /* used by genl */

        LIST_FIELDS(struct match_callback, match_callbacks);
};

typedef enum NetlinkSlotType {
        NETLINK_REPLY_CALLBACK,
        NETLINK_MATCH_CALLBACK,
        _NETLINK_SLOT_INVALID = -EINVAL,
} NetlinkSlotType;

struct sd_netlink_slot {
        unsigned n_ref;
        NetlinkSlotType type:8;
        bool floating;
        sd_netlink *netlink;
        void *userdata;
        sd_netlink_destroy_t destroy_callback;

        char *description;

        LIST_FIELDS(sd_netlink_slot, slots);

        union {
                struct reply_callback reply_callback;
                struct match_callback match_callback;
        };
};

struct sd_netlink {
        unsigned n_ref;

        int fd;

        union {
                struct sockaddr sa;
                struct sockaddr_nl nl;
        } sockaddr;

        int protocol;

        Hashmap *broadcast_group_refs;
        bool broadcast_group_dont_leave:1; /* until we can rely on 4.2 */

        sd_netlink_message **rqueue;
        unsigned rqueue_size;

        sd_netlink_message **rqueue_partial;
        unsigned rqueue_partial_size;

        struct nlmsghdr *rbuffer;

        bool processing:1;

        uint32_t serial;

        struct Prioq *reply_callbacks_prioq;
        Hashmap *reply_callbacks;

        LIST_HEAD(struct match_callback, match_callbacks);

        LIST_HEAD(sd_netlink_slot, slots);

        pid_t original_pid;

        sd_event_source *io_event_source;
        sd_event_source *time_event_source;
        sd_event_source *exit_event_source;
        sd_event *event;

        Hashmap *genl_family_by_name;
        Hashmap *genl_family_by_id;
};

struct netlink_attribute {
        size_t offset; /* offset from hdr to attribute */
        bool nested:1;
        bool net_byteorder:1;
};

struct netlink_container {
        const struct NLTypeSystem *type_system; /* the type system of the container */
        size_t offset; /* offset from hdr to the start of the container */
        struct netlink_attribute *attributes;
        uint16_t max_attribute; /* the maximum attribute in container */
};

struct sd_netlink_message {
        unsigned n_ref;

        int protocol;

        struct nlmsghdr *hdr;
        struct netlink_container containers[NETLINK_CONTAINER_DEPTH];
        unsigned n_containers; /* number of containers */
        uint32_t multicast_group;
        bool sealed:1;

        sd_netlink_message *next; /* next in a chain of multi-part messages */
};

int message_new_empty(sd_netlink *nl, sd_netlink_message **ret);
int message_new_full(
                sd_netlink *nl,
                uint16_t nlmsg_type,
                const NLTypeSystem *type_system,
                size_t header_size,
                sd_netlink_message **ret);
int message_new(sd_netlink *nl, sd_netlink_message **ret, uint16_t type);
int message_new_synthetic_error(sd_netlink *nl, int error, uint32_t serial, sd_netlink_message **ret);
uint32_t message_get_serial(sd_netlink_message *m);
void message_seal(sd_netlink_message *m);

int netlink_open_family(sd_netlink **ret, int family);
bool netlink_pid_changed(sd_netlink *nl);
int netlink_rqueue_make_room(sd_netlink *nl);
int netlink_rqueue_partial_make_room(sd_netlink *nl);

int socket_open(int family);
int socket_bind(sd_netlink *nl);
int socket_broadcast_group_ref(sd_netlink *nl, unsigned group);
int socket_broadcast_group_unref(sd_netlink *nl, unsigned group);
int socket_write_message(sd_netlink *nl, sd_netlink_message *m);
int socket_writev_message(sd_netlink *nl, sd_netlink_message **m, size_t msgcount);
int socket_read_message(sd_netlink *nl);

int netlink_add_match_internal(
                sd_netlink *nl,
                sd_netlink_slot **ret_slot,
                const uint32_t *groups,
                size_t n_groups,
                uint16_t type,
                uint8_t cmd,
                sd_netlink_message_handler_t callback,
                sd_netlink_destroy_t destroy_callback,
                void *userdata,
                const char *description);

/* Make sure callbacks don't destroy the netlink connection */
#define NETLINK_DONT_DESTROY(nl) \
        _cleanup_(sd_netlink_unrefp) _unused_ sd_netlink *_dont_destroy_##nl = sd_netlink_ref(nl)
