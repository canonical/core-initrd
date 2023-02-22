/* SPDX-License-Identifier: LGPL-2.1-or-later */

#if HAVE_GLIB
#include <glib.h>
#endif

#include "sd-bus.h"

#include "alloc-util.h"
#include "bus-dump.h"
#include "bus-gvariant.h"
#include "bus-internal.h"
#include "bus-message.h"
#include "macro.h"
#include "tests.h"
#include "util.h"

TEST(bus_gvariant_is_fixed_size) {
        assert_se(bus_gvariant_is_fixed_size("") > 0);
        assert_se(bus_gvariant_is_fixed_size("()") == -EINVAL);
        assert_se(bus_gvariant_is_fixed_size("y") > 0);
        assert_se(bus_gvariant_is_fixed_size("u") > 0);
        assert_se(bus_gvariant_is_fixed_size("b") > 0);
        assert_se(bus_gvariant_is_fixed_size("n") > 0);
        assert_se(bus_gvariant_is_fixed_size("q") > 0);
        assert_se(bus_gvariant_is_fixed_size("i") > 0);
        assert_se(bus_gvariant_is_fixed_size("t") > 0);
        assert_se(bus_gvariant_is_fixed_size("d") > 0);
        assert_se(bus_gvariant_is_fixed_size("s") == 0);
        assert_se(bus_gvariant_is_fixed_size("o") == 0);
        assert_se(bus_gvariant_is_fixed_size("g") == 0);
        assert_se(bus_gvariant_is_fixed_size("h") > 0);
        assert_se(bus_gvariant_is_fixed_size("ay") == 0);
        assert_se(bus_gvariant_is_fixed_size("v") == 0);
        assert_se(bus_gvariant_is_fixed_size("(u)") > 0);
        assert_se(bus_gvariant_is_fixed_size("(uuuuy)") > 0);
        assert_se(bus_gvariant_is_fixed_size("(uusuuy)") == 0);
        assert_se(bus_gvariant_is_fixed_size("a{ss}") == 0);
        assert_se(bus_gvariant_is_fixed_size("((u)yyy(b(iiii)))") > 0);
        assert_se(bus_gvariant_is_fixed_size("((u)yyy(b(iiivi)))") == 0);
}

TEST(bus_gvariant_get_size) {
        assert_se(bus_gvariant_get_size("") == 0);
        assert_se(bus_gvariant_get_size("()") == -EINVAL);
        assert_se(bus_gvariant_get_size("y") == 1);
        assert_se(bus_gvariant_get_size("u") == 4);
        assert_se(bus_gvariant_get_size("b") == 1);
        assert_se(bus_gvariant_get_size("n") == 2);
        assert_se(bus_gvariant_get_size("q") == 2);
        assert_se(bus_gvariant_get_size("i") == 4);
        assert_se(bus_gvariant_get_size("t") == 8);
        assert_se(bus_gvariant_get_size("d") == 8);
        assert_se(bus_gvariant_get_size("s") < 0);
        assert_se(bus_gvariant_get_size("o") < 0);
        assert_se(bus_gvariant_get_size("g") < 0);
        assert_se(bus_gvariant_get_size("h") == 4);
        assert_se(bus_gvariant_get_size("ay") < 0);
        assert_se(bus_gvariant_get_size("v") < 0);
        assert_se(bus_gvariant_get_size("(u)") == 4);
        assert_se(bus_gvariant_get_size("(uuuuy)") == 20);
        assert_se(bus_gvariant_get_size("(uusuuy)") < 0);
        assert_se(bus_gvariant_get_size("a{ss}") < 0);
        assert_se(bus_gvariant_get_size("((u)yyy(b(iiii)))") == 28);
        assert_se(bus_gvariant_get_size("((u)yyy(b(iiivi)))") < 0);
        assert_se(bus_gvariant_get_size("((b)(t))") == 16);
        assert_se(bus_gvariant_get_size("((b)(b)(t))") == 16);
        assert_se(bus_gvariant_get_size("(bt)") == 16);
        assert_se(bus_gvariant_get_size("((t)(b))") == 16);
        assert_se(bus_gvariant_get_size("(tb)") == 16);
        assert_se(bus_gvariant_get_size("((b)(b))") == 2);
        assert_se(bus_gvariant_get_size("((t)(t))") == 16);
}

TEST(bus_gvariant_get_alignment) {
        assert_se(bus_gvariant_get_alignment("") == 1);
        assert_se(bus_gvariant_get_alignment("()") == -EINVAL);
        assert_se(bus_gvariant_get_alignment("y") == 1);
        assert_se(bus_gvariant_get_alignment("b") == 1);
        assert_se(bus_gvariant_get_alignment("u") == 4);
        assert_se(bus_gvariant_get_alignment("s") == 1);
        assert_se(bus_gvariant_get_alignment("o") == 1);
        assert_se(bus_gvariant_get_alignment("g") == 1);
        assert_se(bus_gvariant_get_alignment("v") == 8);
        assert_se(bus_gvariant_get_alignment("h") == 4);
        assert_se(bus_gvariant_get_alignment("i") == 4);
        assert_se(bus_gvariant_get_alignment("t") == 8);
        assert_se(bus_gvariant_get_alignment("x") == 8);
        assert_se(bus_gvariant_get_alignment("q") == 2);
        assert_se(bus_gvariant_get_alignment("n") == 2);
        assert_se(bus_gvariant_get_alignment("d") == 8);
        assert_se(bus_gvariant_get_alignment("ay") == 1);
        assert_se(bus_gvariant_get_alignment("as") == 1);
        assert_se(bus_gvariant_get_alignment("au") == 4);
        assert_se(bus_gvariant_get_alignment("an") == 2);
        assert_se(bus_gvariant_get_alignment("ans") == 2);
        assert_se(bus_gvariant_get_alignment("ant") == 8);
        assert_se(bus_gvariant_get_alignment("(ss)") == 1);
        assert_se(bus_gvariant_get_alignment("(ssu)") == 4);
        assert_se(bus_gvariant_get_alignment("a(ssu)") == 4);
        assert_se(bus_gvariant_get_alignment("(u)") == 4);
        assert_se(bus_gvariant_get_alignment("(uuuuy)") == 4);
        assert_se(bus_gvariant_get_alignment("(uusuuy)") == 4);
        assert_se(bus_gvariant_get_alignment("a{ss}") == 1);
        assert_se(bus_gvariant_get_alignment("((u)yyy(b(iiii)))") == 4);
        assert_se(bus_gvariant_get_alignment("((u)yyy(b(iiivi)))") == 8);
        assert_se(bus_gvariant_get_alignment("((b)(t))") == 8);
        assert_se(bus_gvariant_get_alignment("((b)(b)(t))") == 8);
        assert_se(bus_gvariant_get_alignment("(bt)") == 8);
        assert_se(bus_gvariant_get_alignment("((t)(b))") == 8);
        assert_se(bus_gvariant_get_alignment("(tb)") == 8);
        assert_se(bus_gvariant_get_alignment("((b)(b))") == 1);
        assert_se(bus_gvariant_get_alignment("((t)(t))") == 8);
}

TEST_RET(marshal) {
        _cleanup_(sd_bus_message_unrefp) sd_bus_message *m = NULL, *n = NULL;
        _cleanup_(sd_bus_flush_close_unrefp) sd_bus *bus = NULL;
        _cleanup_free_ void *blob = NULL;
        size_t sz;
        int r;

        r = sd_bus_open_user(&bus);
        if (r < 0)
                r = sd_bus_open_system(&bus);
        if (r < 0)
                return log_tests_skipped_errno(r, "Failed to connect to bus");

        bus->message_version = 2; /* dirty hack to enable gvariant */

        r = sd_bus_message_new_method_call(bus, &m, "a.service.name",
                                           "/an/object/path/which/is/really/really/long/so/that/we/hit/the/eight/bit/boundary/by/quite/some/margin/to/test/this/stuff/that/it/really/works",
                                           "an.interface.name", "AMethodName");
        assert_se(r >= 0);

        assert_cc(sizeof(struct bus_header) == 16);

        assert_se(sd_bus_message_append(m,
                                        "a(usv)", 3,
                                        4711, "first-string-parameter", "(st)", "X", (uint64_t) 1111,
                                        4712, "second-string-parameter", "(a(si))", 2, "Y", 5, "Z", 6,
                                        4713, "third-string-parameter", "(uu)", 1, 2) >= 0);

        assert_se(sd_bus_message_seal(m, 4711, 0) >= 0);

#if HAVE_GLIB
        {
                GVariant *v;
                char *t;

#if !defined(GLIB_VERSION_2_36)
                g_type_init();
#endif

                v = g_variant_new_from_data(G_VARIANT_TYPE("(yyyyuta{tv})"), m->header, sizeof(struct bus_header) + m->fields_size, false, NULL, NULL);
                assert_se(g_variant_is_normal_form(v));
                t = g_variant_print(v, TRUE);
                printf("%s\n", t);
                g_free(t);
                g_variant_unref(v);

                v = g_variant_new_from_data(G_VARIANT_TYPE("(a(usv))"), m->body.data, m->user_body_size, false, NULL, NULL);
                assert_se(g_variant_is_normal_form(v));
                t = g_variant_print(v, TRUE);
                printf("%s\n", t);
                g_free(t);
                g_variant_unref(v);
        }
#endif

        assert_se(sd_bus_message_dump(m, NULL, SD_BUS_MESSAGE_DUMP_WITH_HEADER) >= 0);

        assert_se(bus_message_get_blob(m, &blob, &sz) >= 0);

#if HAVE_GLIB
        {
                GVariant *v;
                char *t;

                v = g_variant_new_from_data(G_VARIANT_TYPE("(yyyyuta{tv}v)"), blob, sz, false, NULL, NULL);
                assert_se(g_variant_is_normal_form(v));
                t = g_variant_print(v, TRUE);
                printf("%s\n", t);
                g_free(t);
                g_variant_unref(v);
        }
#endif

        assert_se(bus_message_from_malloc(bus, blob, sz, NULL, 0, NULL, &n) >= 0);
        blob = NULL;

        assert_se(sd_bus_message_dump(n, NULL, SD_BUS_MESSAGE_DUMP_WITH_HEADER) >= 0);

        m = sd_bus_message_unref(m);

        assert_se(sd_bus_message_new_method_call(bus, &m, "a.x", "/a/x", "a.x", "Ax") >= 0);

        assert_se(sd_bus_message_append(m, "as", 0) >= 0);

        assert_se(sd_bus_message_seal(m, 4712, 0) >= 0);
        assert_se(sd_bus_message_dump(m, NULL, SD_BUS_MESSAGE_DUMP_WITH_HEADER) >= 0);

        return EXIT_SUCCESS;
}

DEFINE_TEST_MAIN(LOG_DEBUG);
