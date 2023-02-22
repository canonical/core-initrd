/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <errno.h>
#include <poll.h>
#include <sys/inotify.h>

#include "sd-network.h"

#include "alloc-util.h"
#include "env-file.h"
#include "fd-util.h"
#include "fs-util.h"
#include "inotify-util.h"
#include "macro.h"
#include "parse-util.h"
#include "stdio-util.h"
#include "string-util.h"
#include "strv.h"
#include "util.h"

static int network_get_string(const char *field, char **ret) {
        _cleanup_free_ char *s = NULL;
        int r;

        assert_return(ret, -EINVAL);

        r = parse_env_file(NULL, "/run/systemd/netif/state", field, &s);
        if (r == -ENOENT)
                return -ENODATA;
        if (r < 0)
                return r;
        if (isempty(s))
                return -ENODATA;

        *ret = TAKE_PTR(s);

        return 0;
}

int sd_network_get_operational_state(char **state) {
        return network_get_string("OPER_STATE", state);
}

int sd_network_get_carrier_state(char **state) {
        return network_get_string("CARRIER_STATE", state);
}

int sd_network_get_address_state(char **state) {
        return network_get_string("ADDRESS_STATE", state);
}

int sd_network_get_ipv4_address_state(char **state) {
        return network_get_string("IPV4_ADDRESS_STATE", state);
}

int sd_network_get_ipv6_address_state(char **state) {
        return network_get_string("IPV6_ADDRESS_STATE", state);
}

int sd_network_get_online_state(char **state) {
        return network_get_string("ONLINE_STATE", state);
}

static int network_get_strv(const char *key, char ***ret) {
        _cleanup_strv_free_ char **a = NULL;
        _cleanup_free_ char *s = NULL;
        int r;

        assert_return(ret, -EINVAL);

        r = parse_env_file(NULL, "/run/systemd/netif/state", key, &s);
        if (r == -ENOENT)
                return -ENODATA;
        if (r < 0)
                return r;
        if (isempty(s)) {
                *ret = NULL;
                return 0;
        }

        a = strv_split(s, " ");
        if (!a)
                return -ENOMEM;

        strv_uniq(a);
        r = (int) strv_length(a);

        *ret = TAKE_PTR(a);

        return r;
}

int sd_network_get_dns(char ***ret) {
        return network_get_strv("DNS", ret);
}

int sd_network_get_ntp(char ***ret) {
        return network_get_strv("NTP", ret);
}

int sd_network_get_search_domains(char ***ret) {
        return network_get_strv("DOMAINS", ret);
}

int sd_network_get_route_domains(char ***ret) {
        return network_get_strv("ROUTE_DOMAINS", ret);
}

static int network_link_get_string(int ifindex, const char *field, char **ret) {
        char path[STRLEN("/run/systemd/netif/links/") + DECIMAL_STR_MAX(ifindex)];
        _cleanup_free_ char *s = NULL;
        int r;

        assert_return(ifindex > 0, -EINVAL);
        assert_return(ret, -EINVAL);

        xsprintf(path, "/run/systemd/netif/links/%i", ifindex);

        r = parse_env_file(NULL, path, field, &s);
        if (r == -ENOENT)
                return -ENODATA;
        if (r < 0)
                return r;
        if (isempty(s))
                return -ENODATA;

        *ret = TAKE_PTR(s);

        return 0;
}

static int network_link_get_strv(int ifindex, const char *key, char ***ret) {
        char path[STRLEN("/run/systemd/netif/links/") + DECIMAL_STR_MAX(ifindex)];
        _cleanup_strv_free_ char **a = NULL;
        _cleanup_free_ char *s = NULL;
        int r;

        assert_return(ifindex > 0, -EINVAL);
        assert_return(ret, -EINVAL);

        xsprintf(path, "/run/systemd/netif/links/%i", ifindex);
        r = parse_env_file(NULL, path, key, &s);
        if (r == -ENOENT)
                return -ENODATA;
        if (r < 0)
                return r;
        if (isempty(s)) {
                *ret = NULL;
                return 0;
        }

        a = strv_split(s, " ");
        if (!a)
                return -ENOMEM;

        strv_uniq(a);
        r = (int) strv_length(a);

        *ret = TAKE_PTR(a);

        return r;
}

int sd_network_link_get_setup_state(int ifindex, char **state) {
        return network_link_get_string(ifindex, "ADMIN_STATE", state);
}

int sd_network_link_get_network_file(int ifindex, char **filename) {
        return network_link_get_string(ifindex, "NETWORK_FILE", filename);
}

int sd_network_link_get_operational_state(int ifindex, char **state) {
        return network_link_get_string(ifindex, "OPER_STATE", state);
}

int sd_network_link_get_required_family_for_online(int ifindex, char **state) {
        _cleanup_free_ char *s = NULL;
        int r;

        assert_return(state, -EINVAL);

        r = network_link_get_string(ifindex, "REQUIRED_FAMILY_FOR_ONLINE", &s);
        if (r < 0) {
                if (r != -ENODATA)
                        return r;

                s = strdup("any");
                if (!s)
                        return -ENOMEM;
        }

        *state = TAKE_PTR(s);
        return 0;
}

int sd_network_link_get_carrier_state(int ifindex, char **state) {
        return network_link_get_string(ifindex, "CARRIER_STATE", state);
}

int sd_network_link_get_address_state(int ifindex, char **state) {
        return network_link_get_string(ifindex, "ADDRESS_STATE", state);
}

int sd_network_link_get_ipv4_address_state(int ifindex, char **state) {
        return network_link_get_string(ifindex, "IPV4_ADDRESS_STATE", state);
}

int sd_network_link_get_ipv6_address_state(int ifindex, char **state) {
        return network_link_get_string(ifindex, "IPV6_ADDRESS_STATE", state);
}

int sd_network_link_get_online_state(int ifindex, char **state) {
        return network_link_get_string(ifindex, "ONLINE_STATE", state);
}

int sd_network_link_get_dhcp6_client_iaid_string(int ifindex, char **iaid) {
        return network_link_get_string(ifindex, "DHCP6_CLIENT_IAID", iaid);
}

int sd_network_link_get_dhcp6_client_duid_string(int ifindex, char **duid) {
        return network_link_get_string(ifindex, "DHCP6_CLIENT_DUID", duid);
}

int sd_network_link_get_required_for_online(int ifindex) {
        _cleanup_free_ char *s = NULL;
        int r;

        r = network_link_get_string(ifindex, "REQUIRED_FOR_ONLINE", &s);
        if (r < 0) {
                /* Handle -ENODATA as RequiredForOnline=yes, for compatibility */
                if (r == -ENODATA)
                        return true;
                return r;
        }

        return parse_boolean(s);
}

int sd_network_link_get_required_operstate_for_online(int ifindex, char **state) {
        _cleanup_free_ char *s = NULL;
        int r;

        assert_return(state, -EINVAL);

        r = network_link_get_string(ifindex, "REQUIRED_OPER_STATE_FOR_ONLINE", &s);
        if (r < 0) {
                if (r != -ENODATA)
                        return r;

                /* For compatibility, assuming degraded. */
                s = strdup("degraded");
                if (!s)
                        return -ENOMEM;
        }

        *state = TAKE_PTR(s);
        return 0;
}

int sd_network_link_get_activation_policy(int ifindex, char **policy) {
        _cleanup_free_ char *s = NULL;
        int r;

        assert_return(policy, -EINVAL);

        r = network_link_get_string(ifindex, "ACTIVATION_POLICY", &s);
        if (r < 0) {
                if (r != -ENODATA)
                        return r;

                /* For compatibility, assuming up. */
                s = strdup("up");
                if (!s)
                        return -ENOMEM;
        }

        *policy = TAKE_PTR(s);
        return 0;
}

int sd_network_link_get_llmnr(int ifindex, char **llmnr) {
        return network_link_get_string(ifindex, "LLMNR", llmnr);
}

int sd_network_link_get_mdns(int ifindex, char **mdns) {
        return network_link_get_string(ifindex, "MDNS", mdns);
}

int sd_network_link_get_dns_over_tls(int ifindex, char **dns_over_tls) {
        return network_link_get_string(ifindex, "DNS_OVER_TLS", dns_over_tls);
}

int sd_network_link_get_dnssec(int ifindex, char **dnssec) {
        return network_link_get_string(ifindex, "DNSSEC", dnssec);
}

int sd_network_link_get_dnssec_negative_trust_anchors(int ifindex, char ***nta) {
        return network_link_get_strv(ifindex, "DNSSEC_NTA", nta);
}

int sd_network_link_get_dns(int ifindex, char ***ret) {
        return network_link_get_strv(ifindex, "DNS", ret);
}

int sd_network_link_get_ntp(int ifindex, char ***ret) {
        return network_link_get_strv(ifindex, "NTP", ret);
}

int sd_network_link_get_sip(int ifindex, char ***ret) {
        return network_link_get_strv(ifindex, "SIP", ret);
}

int sd_network_link_get_search_domains(int ifindex, char ***ret) {
        return network_link_get_strv(ifindex, "DOMAINS", ret);
}

int sd_network_link_get_route_domains(int ifindex, char ***ret) {
        return network_link_get_strv(ifindex, "ROUTE_DOMAINS", ret);
}

int sd_network_link_get_dns_default_route(int ifindex) {
        char path[STRLEN("/run/systemd/netif/links/") + DECIMAL_STR_MAX(ifindex)];
        _cleanup_free_ char *s = NULL;
        int r;

        assert_return(ifindex > 0, -EINVAL);

        xsprintf(path, "/run/systemd/netif/links/%i", ifindex);

        r = parse_env_file(NULL, path, "DNS_DEFAULT_ROUTE", &s);
        if (r == -ENOENT)
                return -ENODATA;
        if (r < 0)
                return r;
        if (isempty(s))
                return -ENODATA;
        return parse_boolean(s);
}

static int network_link_get_ifindexes(int ifindex, const char *key, int **ret) {
        char path[STRLEN("/run/systemd/netif/links/") + DECIMAL_STR_MAX(ifindex)];
        _cleanup_free_ int *ifis = NULL;
        _cleanup_free_ char *s = NULL;
        size_t c = 0;
        int r;

        assert_return(ifindex > 0, -EINVAL);
        assert_return(ret, -EINVAL);

        xsprintf(path, "/run/systemd/netif/links/%i", ifindex);
        r = parse_env_file(NULL, path, key, &s);
        if (r == -ENOENT)
                return -ENODATA;
        if (r < 0)
                return r;

        for (const char *x = s;;) {
                _cleanup_free_ char *word = NULL;

                r = extract_first_word(&x, &word, NULL, 0);
                if (r < 0)
                        return r;
                if (r == 0)
                        break;

                if (!GREEDY_REALLOC(ifis, c + 2))
                        return -ENOMEM;

                r = ifis[c++] = parse_ifindex(word);
                if (r < 0)
                        return r;
        }

        if (ifis)
                ifis[c] = 0; /* Let's add a 0 ifindex to the end, to be nice */

        *ret = TAKE_PTR(ifis);

        return c;
}

int sd_network_link_get_carrier_bound_to(int ifindex, int **ret) {
        return network_link_get_ifindexes(ifindex, "CARRIER_BOUND_TO", ret);
}

int sd_network_link_get_carrier_bound_by(int ifindex, int **ret) {
        return network_link_get_ifindexes(ifindex, "CARRIER_BOUND_BY", ret);
}

int sd_network_link_get_stat(int ifindex, struct stat *ret) {
        char path[STRLEN("/run/systemd/netif/links/") + DECIMAL_STR_MAX(ifindex)];
        struct stat st;

        assert_return(ifindex > 0, -EINVAL);

        xsprintf(path, "/run/systemd/netif/links/%i", ifindex);

        if (stat(path, &st) < 0)
                return -errno;

        if (ret)
                *ret = st;

        return 0;
}

static int MONITOR_TO_FD(sd_network_monitor *m) {
        return (int) (unsigned long) m - 1;
}

static sd_network_monitor* FD_TO_MONITOR(int fd) {
        return (sd_network_monitor*) (unsigned long) (fd + 1);
}

static int monitor_add_inotify_watch(int fd) {
        int wd;

        wd = inotify_add_watch(fd, "/run/systemd/netif/links/", IN_MOVED_TO|IN_DELETE);
        if (wd >= 0)
                return wd;
        else if (errno != ENOENT)
                return -errno;

        wd = inotify_add_watch(fd, "/run/systemd/netif/", IN_CREATE|IN_ISDIR);
        if (wd >= 0)
                return wd;
        else if (errno != ENOENT)
                return -errno;

        wd = inotify_add_watch(fd, "/run/systemd/", IN_CREATE|IN_ISDIR);
        if (wd < 0)
                return -errno;

        return wd;
}

int sd_network_monitor_new(sd_network_monitor **m, const char *category) {
        _cleanup_close_ int fd = -1;
        int k;
        bool good = false;

        assert_return(m, -EINVAL);

        fd = inotify_init1(IN_NONBLOCK|IN_CLOEXEC);
        if (fd < 0)
                return -errno;

        if (!category || streq(category, "links")) {
                k = monitor_add_inotify_watch(fd);
                if (k < 0)
                        return k;

                good = true;
        }

        if (!good)
                return -EINVAL;

        *m = FD_TO_MONITOR(TAKE_FD(fd));
        return 0;
}

sd_network_monitor* sd_network_monitor_unref(sd_network_monitor *m) {
        if (m)
                (void) close_nointr(MONITOR_TO_FD(m));

        return NULL;
}

int sd_network_monitor_flush(sd_network_monitor *m) {
        union inotify_event_buffer buffer;
        ssize_t l;
        int fd;

        assert_return(m, -EINVAL);

        fd = MONITOR_TO_FD(m);

        l = read(fd, &buffer, sizeof(buffer));
        if (l < 0) {
                if (ERRNO_IS_TRANSIENT(errno))
                        return 0;

                return -errno;
        }

        FOREACH_INOTIFY_EVENT(e, buffer, l) {
                if (e->mask & IN_ISDIR) {
                        int wd;

                        wd = monitor_add_inotify_watch(fd);
                        if (wd < 0)
                                return wd;

                        if (wd != e->wd) {
                                if (inotify_rm_watch(fd, e->wd) < 0)
                                        return -errno;
                        }
                }
        }

        return 0;
}

int sd_network_monitor_get_fd(sd_network_monitor *m) {

        assert_return(m, -EINVAL);

        return MONITOR_TO_FD(m);
}

int sd_network_monitor_get_events(sd_network_monitor *m) {

        assert_return(m, -EINVAL);

        /* For now we will only return POLLIN here, since we don't
         * need anything else ever for inotify.  However, let's have
         * this API to keep our options open should we later on need
         * it. */
        return POLLIN;
}

int sd_network_monitor_get_timeout(sd_network_monitor *m, uint64_t *timeout_usec) {

        assert_return(m, -EINVAL);
        assert_return(timeout_usec, -EINVAL);

        /* For now we will only return UINT64_MAX, since we don't
         * need any timeout. However, let's have this API to keep our
         * options open should we later on need it. */
        *timeout_usec = UINT64_MAX;
        return 0;
}
