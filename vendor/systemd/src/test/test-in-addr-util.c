/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <fnmatch.h>
#include <netinet/in.h>

#include "in-addr-util.h"
#include "strv.h"
#include "tests.h"

static void test_in_addr_prefix_from_string_one(
                const char *p,
                int family,
                int ret,
                const union in_addr_union *u,
                unsigned char prefixlen,
                int ret_refuse,
                unsigned char prefixlen_refuse,
                int ret_legacy,
                unsigned char prefixlen_legacy) {

        union in_addr_union q;
        unsigned char l;
        int f, r;

        r = in_addr_prefix_from_string(p, family, &q, &l);
        assert_se(r == ret);

        if (r < 0)
                return;

        assert_se(in_addr_equal(family, &q, u));
        assert_se(l == prefixlen);

        r = in_addr_prefix_from_string_auto(p, &f, &q, &l);
        assert_se(r >= 0);

        assert_se(f == family);
        assert_se(in_addr_equal(family, &q, u));
        assert_se(l == prefixlen);

        r = in_addr_prefix_from_string_auto_internal(p, PREFIXLEN_REFUSE, &f, &q, &l);
        assert_se(r == ret_refuse);

        if (r >= 0) {
                assert_se(f == family);
                assert_se(in_addr_equal(family, &q, u));
                assert_se(l == prefixlen_refuse);
        }

        r = in_addr_prefix_from_string_auto_internal(p, PREFIXLEN_LEGACY, &f, &q, &l);
        assert_se(r == ret_legacy);

        if (r >= 0) {
                assert_se(f == family);
                assert_se(in_addr_equal(family, &q, u));
                assert_se(l == prefixlen_legacy);
        }
}

TEST(in_addr_prefix_from_string) {
        test_in_addr_prefix_from_string_one("", AF_INET, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string_one("/", AF_INET, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string_one("/8", AF_INET, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string_one("1.2.3.4", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 32, -ENOANO, 0, 0, 8);
        test_in_addr_prefix_from_string_one("1.2.3.4/0", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 0, 0, 0, 0, 0);
        test_in_addr_prefix_from_string_one("1.2.3.4/1", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 1, 0, 1, 0, 1);
        test_in_addr_prefix_from_string_one("1.2.3.4/2", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 2, 0, 2, 0, 2);
        test_in_addr_prefix_from_string_one("1.2.3.4/32", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 32, 0, 32, 0, 32);
        test_in_addr_prefix_from_string_one("1.2.3.4/33", AF_INET, -ERANGE, NULL, 0, -ERANGE, 0, -ERANGE, 0);
        test_in_addr_prefix_from_string_one("1.2.3.4/-1", AF_INET, -ERANGE, NULL, 0, -ERANGE, 0, -ERANGE, 0);
        test_in_addr_prefix_from_string_one("::1", AF_INET, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);

        test_in_addr_prefix_from_string_one("", AF_INET6, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string_one("/", AF_INET6, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string_one("/8", AF_INET6, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string_one("::1", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 128, -ENOANO, 0, 0, 0);
        test_in_addr_prefix_from_string_one("::1/0", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 0, 0, 0, 0, 0);
        test_in_addr_prefix_from_string_one("::1/1", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 1, 0, 1, 0, 1);
        test_in_addr_prefix_from_string_one("::1/2", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 2, 0, 2, 0, 2);
        test_in_addr_prefix_from_string_one("::1/32", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 32, 0, 32, 0, 32);
        test_in_addr_prefix_from_string_one("::1/33", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 33, 0, 33, 0, 33);
        test_in_addr_prefix_from_string_one("::1/64", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 64, 0, 64, 0, 64);
        test_in_addr_prefix_from_string_one("::1/128", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 128, 0, 128, 0, 128);
        test_in_addr_prefix_from_string_one("::1/129", AF_INET6, -ERANGE, NULL, 0, -ERANGE, 0, -ERANGE, 0);
        test_in_addr_prefix_from_string_one("::1/-1", AF_INET6, -ERANGE, NULL, 0, -ERANGE, 0, -ERANGE, 0);
}

static void test_in_addr_prefix_to_string_valid(int family, const char *p) {
        _cleanup_free_ char *str = NULL;
        union in_addr_union u;
        unsigned char l;

        log_info("%s: %s", __func__, p);

        assert_se(in_addr_prefix_from_string(p, family, &u, &l) >= 0);
        assert_se(in_addr_prefix_to_string(family, &u, l, &str) >= 0);
        assert_se(streq(str, p));
}

static void test_in_addr_prefix_to_string_unoptimized(int family, const char *p) {
        _cleanup_free_ char *str1 = NULL, *str2 = NULL;
        union in_addr_union u1, u2;
        unsigned char len1, len2;

        log_info("%s: %s", __func__, p);

        assert_se(in_addr_prefix_from_string(p, family, &u1, &len1) >= 0);
        assert_se(in_addr_prefix_to_string(family, &u1, len1, &str1) >= 0);
        assert_se(in_addr_prefix_from_string(str1, family, &u2, &len2) >= 0);
        assert_se(in_addr_prefix_to_string(family, &u2, len2, &str2) >= 0);

        assert_se(streq(str1, str2));
        assert_se(len1 == len2);
        assert_se(in_addr_equal(family, &u1, &u2) > 0);
}

TEST(in_addr_prefix_to_string) {
        test_in_addr_prefix_to_string_valid(AF_INET, "0.0.0.0/32");
        test_in_addr_prefix_to_string_valid(AF_INET, "1.2.3.4/0");
        test_in_addr_prefix_to_string_valid(AF_INET, "1.2.3.4/24");
        test_in_addr_prefix_to_string_valid(AF_INET, "1.2.3.4/32");
        test_in_addr_prefix_to_string_valid(AF_INET, "255.255.255.255/32");

        test_in_addr_prefix_to_string_valid(AF_INET6, "::1/128");
        test_in_addr_prefix_to_string_valid(AF_INET6, "fd00:abcd::1/64");
        test_in_addr_prefix_to_string_valid(AF_INET6, "fd00:abcd::1234:1/64");
        test_in_addr_prefix_to_string_valid(AF_INET6, "1111:2222:3333:4444:5555:6666:7777:8888/128");

        test_in_addr_prefix_to_string_unoptimized(AF_INET, "0.0.0.0");
        test_in_addr_prefix_to_string_unoptimized(AF_INET, "192.168.0.1");

        test_in_addr_prefix_to_string_unoptimized(AF_INET6, "fd00:0000:0000:0000:0000:0000:0000:0001/64");
        test_in_addr_prefix_to_string_unoptimized(AF_INET6, "fd00:1111::0000:2222:3333:4444:0001/64");
}

TEST(in_addr_random_prefix) {
        _cleanup_free_ char *str = NULL;
        union in_addr_union a;

        assert_se(in_addr_from_string(AF_INET, "192.168.10.1", &a) >= 0);

        assert_se(in_addr_random_prefix(AF_INET, &a, 31, 32) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(STR_IN_SET(str, "192.168.10.0", "192.168.10.1"));
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET, &a, 24, 26) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(startswith(str, "192.168.10."));
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET, &a, 16, 24) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(fnmatch("192.168.[0-9]*.0", str, 0) == 0);
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET, &a, 8, 24) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(fnmatch("192.[0-9]*.[0-9]*.0", str, 0) == 0);
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET, &a, 8, 16) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(fnmatch("192.[0-9]*.0.0", str, 0) == 0);
        str = mfree(str);

        assert_se(in_addr_from_string(AF_INET6, "fd00::1", &a) >= 0);

        assert_se(in_addr_random_prefix(AF_INET6, &a, 16, 64) >= 0);
        assert_se(in_addr_to_string(AF_INET6, &a, &str) >= 0);
        assert_se(startswith(str, "fd00:"));
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET6, &a, 8, 16) >= 0);
        assert_se(in_addr_to_string(AF_INET6, &a, &str) >= 0);
        assert_se(fnmatch("fd??::", str, 0) == 0);
        str = mfree(str);
}

TEST(in_addr_is_null) {
        union in_addr_union i = {};

        assert_se(in_addr_is_null(AF_INET, &i) == true);
        assert_se(in_addr_is_null(AF_INET6, &i) == true);

        i.in.s_addr = 0x1000000;
        assert_se(in_addr_is_null(AF_INET, &i) == false);
        assert_se(in_addr_is_null(AF_INET6, &i) == false);

        assert_se(in_addr_is_null(-1, &i) == -EAFNOSUPPORT);
}

static void test_in_addr_prefix_intersect_one(unsigned f, const char *a, unsigned apl, const char *b, unsigned bpl, int result) {
        union in_addr_union ua, ub;

        assert_se(in_addr_from_string(f, a, &ua) >= 0);
        assert_se(in_addr_from_string(f, b, &ub) >= 0);

        assert_se(in_addr_prefix_intersect(f, &ua, apl, &ub, bpl) == result);
}

TEST(in_addr_prefix_intersect) {
        test_in_addr_prefix_intersect_one(AF_INET, "255.255.255.255", 32, "255.255.255.254", 32, 0);
        test_in_addr_prefix_intersect_one(AF_INET, "255.255.255.255", 0, "255.255.255.255", 32, 1);
        test_in_addr_prefix_intersect_one(AF_INET, "0.0.0.0", 0, "47.11.8.15", 32, 1);

        test_in_addr_prefix_intersect_one(AF_INET, "1.1.1.1", 24, "1.1.1.1", 24, 1);
        test_in_addr_prefix_intersect_one(AF_INET, "2.2.2.2", 24, "1.1.1.1", 24, 0);

        test_in_addr_prefix_intersect_one(AF_INET, "1.1.1.1", 24, "1.1.1.127", 25, 1);
        test_in_addr_prefix_intersect_one(AF_INET, "1.1.1.1", 24, "1.1.1.127", 26, 1);
        test_in_addr_prefix_intersect_one(AF_INET, "1.1.1.1", 25, "1.1.1.127", 25, 1);
        test_in_addr_prefix_intersect_one(AF_INET, "1.1.1.1", 25, "1.1.1.255", 25, 0);

        test_in_addr_prefix_intersect_one(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 128, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe", 128, 0);
        test_in_addr_prefix_intersect_one(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 0, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 128, 1);
        test_in_addr_prefix_intersect_one(AF_INET6, "::", 0, "beef:beef:beef:beef:beef:beef:beef:beef", 128, 1);

        test_in_addr_prefix_intersect_one(AF_INET6, "1::2", 64, "1::2", 64, 1);
        test_in_addr_prefix_intersect_one(AF_INET6, "2::2", 64, "1::2", 64, 0);

        test_in_addr_prefix_intersect_one(AF_INET6, "1::1", 120, "1::007f", 121, 1);
        test_in_addr_prefix_intersect_one(AF_INET6, "1::1", 120, "1::007f", 122, 1);
        test_in_addr_prefix_intersect_one(AF_INET6, "1::1", 121, "1::007f", 121, 1);
        test_in_addr_prefix_intersect_one(AF_INET6, "1::1", 121, "1::00ff", 121, 0);
}

static void test_in_addr_prefix_next_one(unsigned f, const char *before, unsigned pl, const char *after) {
        union in_addr_union ubefore, uafter, t;

        log_debug("/* %s(%s, prefixlen=%u) */", __func__, before, pl);

        assert_se(in_addr_from_string(f, before, &ubefore) >= 0);

        t = ubefore;
        assert_se((in_addr_prefix_next(f, &t, pl) >= 0) == !!after);

        if (after) {
                assert_se(in_addr_from_string(f, after, &uafter) >= 0);
                assert_se(in_addr_equal(f, &t, &uafter) > 0);
        }
}

TEST(in_addr_prefix_next) {
        test_in_addr_prefix_next_one(AF_INET, "192.168.0.0", 24, "192.168.1.0");
        test_in_addr_prefix_next_one(AF_INET, "192.168.0.0", 16, "192.169.0.0");
        test_in_addr_prefix_next_one(AF_INET, "192.168.0.0", 20, "192.168.16.0");

        test_in_addr_prefix_next_one(AF_INET, "0.0.0.0", 32, "0.0.0.1");
        test_in_addr_prefix_next_one(AF_INET, "255.255.255.254", 32, "255.255.255.255");
        test_in_addr_prefix_next_one(AF_INET, "255.255.255.255", 32, NULL);
        test_in_addr_prefix_next_one(AF_INET, "255.255.255.0", 24, NULL);

        test_in_addr_prefix_next_one(AF_INET6, "4400::", 128, "4400::0001");
        test_in_addr_prefix_next_one(AF_INET6, "4400::", 120, "4400::0100");
        test_in_addr_prefix_next_one(AF_INET6, "4400::", 127, "4400::0002");
        test_in_addr_prefix_next_one(AF_INET6, "4400::", 8, "4500::");
        test_in_addr_prefix_next_one(AF_INET6, "4400::", 7, "4600::");

        test_in_addr_prefix_next_one(AF_INET6, "::", 128, "::1");

        test_in_addr_prefix_next_one(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 128, NULL);
        test_in_addr_prefix_next_one(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff00", 120, NULL);
}

static void test_in_addr_prefix_nth_one(unsigned f, const char *before, unsigned pl, uint64_t nth, const char *after) {
        union in_addr_union ubefore, uafter, t;

        log_debug("/* %s(%s, prefixlen=%u, nth=%"PRIu64") */", __func__, before, pl, nth);

        assert_se(in_addr_from_string(f, before, &ubefore) >= 0);

        t = ubefore;
        assert_se((in_addr_prefix_nth(f, &t, pl, nth) >= 0) == !!after);

        if (after) {
                assert_se(in_addr_from_string(f, after, &uafter) >= 0);
                assert_se(in_addr_equal(f, &t, &uafter) > 0);
        }
}

TEST(in_addr_prefix_nth) {
        test_in_addr_prefix_nth_one(AF_INET, "192.168.0.0", 24, 0, "192.168.0.0");
        test_in_addr_prefix_nth_one(AF_INET, "192.168.0.123", 24, 0, "192.168.0.0");
        test_in_addr_prefix_nth_one(AF_INET, "192.168.0.123", 24, 1, "192.168.1.0");
        test_in_addr_prefix_nth_one(AF_INET, "192.168.0.0", 24, 4, "192.168.4.0");
        test_in_addr_prefix_nth_one(AF_INET, "192.168.0.0", 25, 1, "192.168.0.128");
        test_in_addr_prefix_nth_one(AF_INET, "192.168.255.0", 25, 1, "192.168.255.128");
        test_in_addr_prefix_nth_one(AF_INET, "192.168.255.0", 24, 0, "192.168.255.0");
        test_in_addr_prefix_nth_one(AF_INET, "255.255.255.255", 32, 1, NULL);
        test_in_addr_prefix_nth_one(AF_INET, "255.255.255.255", 0, 1, NULL);

        test_in_addr_prefix_nth_one(AF_INET6, "4400::", 8, 1, "4500::");
        test_in_addr_prefix_nth_one(AF_INET6, "4400::", 7, 1, "4600::");
        test_in_addr_prefix_nth_one(AF_INET6, "4400::", 64, 1, "4400:0:0:1::");
        test_in_addr_prefix_nth_one(AF_INET6, "4400::", 64, 2, "4400:0:0:2::");
        test_in_addr_prefix_nth_one(AF_INET6, "4400::", 64, 0xbad, "4400:0:0:0bad::");
        test_in_addr_prefix_nth_one(AF_INET6, "4400:0:0:ffff::", 64, 1, "4400:0:1::");
        test_in_addr_prefix_nth_one(AF_INET6, "4400::", 56, ((uint64_t)1<<48) -1, "44ff:ffff:ffff:ff00::");
        test_in_addr_prefix_nth_one(AF_INET6, "0000::", 8, 255, "ff00::");
        test_in_addr_prefix_nth_one(AF_INET6, "0000::", 8, 256, NULL);
        test_in_addr_prefix_nth_one(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 128, 1, NULL);
        test_in_addr_prefix_nth_one(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 0, 1, NULL);
        test_in_addr_prefix_nth_one(AF_INET6, "1234:5678:90ab:cdef:1234:5678:90ab:cdef", 12, 1, "1240::");
}

static void test_in_addr_prefix_range_one(
                int family,
                const char *in,
                unsigned prefixlen,
                const char *expected_start,
                const char *expected_end) {

        union in_addr_union a, s, e;

        log_debug("/* %s(%s, prefixlen=%u) */", __func__, in, prefixlen);

        assert_se(in_addr_from_string(family, in, &a) >= 0);
        assert_se((in_addr_prefix_range(family, &a, prefixlen, &s, &e) >= 0) == !!expected_start);

        if (expected_start) {
                union in_addr_union es;

                assert_se(in_addr_from_string(family, expected_start, &es) >= 0);
                assert_se(in_addr_equal(family, &s, &es) > 0);
        }
        if (expected_end) {
                union in_addr_union ee;

                assert_se(in_addr_from_string(family, expected_end, &ee) >= 0);
                assert_se(in_addr_equal(family, &e, &ee) > 0);
        }
}

TEST(in_addr_prefix_range) {
        test_in_addr_prefix_range_one(AF_INET, "192.168.123.123", 24, "192.168.123.0", "192.168.124.0");
        test_in_addr_prefix_range_one(AF_INET, "192.168.123.123", 16, "192.168.0.0", "192.169.0.0");

        test_in_addr_prefix_range_one(AF_INET6, "dead:beef::", 64, "dead:beef::", "dead:beef:0:1::");
        test_in_addr_prefix_range_one(AF_INET6, "dead:0:0:beef::", 64, "dead:0:0:beef::", "dead:0:0:bef0::");
        test_in_addr_prefix_range_one(AF_INET6, "2001::",  48, "2001::", "2001:0:1::");
        test_in_addr_prefix_range_one(AF_INET6, "2001::",  56, "2001::", "2001:0:0:0100::");
        test_in_addr_prefix_range_one(AF_INET6, "2001::",  65, "2001::", "2001::8000:0:0:0");
        test_in_addr_prefix_range_one(AF_INET6, "2001::",  66, "2001::", "2001::4000:0:0:0");
        test_in_addr_prefix_range_one(AF_INET6, "2001::", 127, "2001::", "2001::2");
}

static void test_in_addr_to_string_one(int f, const char *addr) {
        union in_addr_union ua;
        _cleanup_free_ char *r = NULL;

        assert_se(in_addr_from_string(f, addr, &ua) >= 0);
        assert_se(in_addr_to_string(f, &ua, &r) >= 0);
        printf("test_in_addr_to_string_one: %s == %s\n", addr, r);
        assert_se(streq(addr, r));
}

TEST(in_addr_to_string) {
        test_in_addr_to_string_one(AF_INET, "192.168.0.1");
        test_in_addr_to_string_one(AF_INET, "10.11.12.13");
        test_in_addr_to_string_one(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
        test_in_addr_to_string_one(AF_INET6, "::1");
        test_in_addr_to_string_one(AF_INET6, "fe80::");
}

DEFINE_TEST_MAIN(LOG_DEBUG);
