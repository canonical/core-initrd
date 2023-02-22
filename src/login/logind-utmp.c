/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <errno.h>
#include <unistd.h>

#include "sd-messages.h"

#include "alloc-util.h"
#include "audit-util.h"
#include "bus-common-errors.h"
#include "bus-error.h"
#include "bus-util.h"
#include "event-util.h"
#include "format-util.h"
#include "logind.h"
#include "path-util.h"
#include "special.h"
#include "strv.h"
#include "unit-name.h"
#include "user-util.h"
#include "utmp-wtmp.h"

_const_ static usec_t when_wall(usec_t n, usec_t elapse) {

        usec_t left;
        unsigned i;
        static const int wall_timers[] = {
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                25, 40, 55, 70, 100, 130, 150, 180,
        };

        /* If the time is already passed, then don't announce */
        if (n >= elapse)
                return 0;

        left = elapse - n;

        for (i = 1; i < ELEMENTSOF(wall_timers); i++)
                if (wall_timers[i] * USEC_PER_MINUTE >= left)
                        return left - wall_timers[i-1] * USEC_PER_MINUTE;

        return left % USEC_PER_HOUR;
}

bool logind_wall_tty_filter(const char *tty, bool is_local, void *userdata) {
        Manager *m = ASSERT_PTR(userdata);

        assert(m->scheduled_shutdown_action);

        const char *p = path_startswith(tty, "/dev/");
        if (!p)
                return true;

        /* Do not send information about events which do not destroy local sessions to local terminals. We
         * can assume that if the system enters sleep or hibernation, this will be visible in an obvious way
         * for any local user. And once the systems exits sleep or hibernation, the notication would be just
         * noise, in particular for auto-suspend. */
        if (is_local &&
            IN_SET(m->scheduled_shutdown_action->handle,
                   HANDLE_SUSPEND,
                   HANDLE_HIBERNATE,
                   HANDLE_HYBRID_SLEEP,
                   HANDLE_SUSPEND_THEN_HIBERNATE))
                return false;

        return !streq_ptr(p, m->scheduled_shutdown_tty);
}

static int warn_wall(Manager *m, usec_t n) {
        _cleanup_free_ char *l = NULL, *username = NULL;
        usec_t left;
        int r;

        assert(m);

        if (!m->enable_wall_messages || !m->scheduled_shutdown_action)
                return 0;

        left = m->scheduled_shutdown_timeout > n;

        r = asprintf(&l, "%s%sThe system is going down for %s %s%s!",
                     strempty(m->wall_message),
                     isempty(m->wall_message) ? "" : "\n",
                     handle_action_to_string(m->scheduled_shutdown_action->handle),
                     left ? "at " : "NOW",
                     left ? FORMAT_TIMESTAMP(m->scheduled_shutdown_timeout) : "");
        if (r < 0) {
                log_oom();
                return 0;
        }

        username = uid_to_name(m->scheduled_shutdown_uid);
        utmp_wall(l, username, m->scheduled_shutdown_tty, logind_wall_tty_filter, m);

        return 1;
}

static int wall_message_timeout_handler(
                        sd_event_source *s,
                        uint64_t usec,
                        void *userdata) {

        Manager *m = userdata;
        usec_t n, next;
        int r;

        assert(m);
        assert(s == m->wall_message_timeout_source);

        n = now(CLOCK_REALTIME);

        r = warn_wall(m, n);
        if (r == 0)
                return 0;

        next = when_wall(n, m->scheduled_shutdown_timeout);
        if (next > 0) {
                r = sd_event_source_set_time(s, n + next);
                if (r < 0)
                        return log_error_errno(r, "sd_event_source_set_time() failed. %m");

                r = sd_event_source_set_enabled(s, SD_EVENT_ONESHOT);
                if (r < 0)
                        return log_error_errno(r, "sd_event_source_set_enabled() failed. %m");
        }

        return 0;
}

int manager_setup_wall_message_timer(Manager *m) {

        usec_t n, elapse;
        int r;

        assert(m);

        n = now(CLOCK_REALTIME);
        elapse = m->scheduled_shutdown_timeout;

        /* wall message handling */

        if (!m->scheduled_shutdown_action)
                return 0;

        if (elapse > 0 && elapse < n)
                return 0;

        /* Warn immediately if less than 15 minutes are left */
        if (elapse == 0 || elapse - n < 15 * USEC_PER_MINUTE) {
                r = warn_wall(m, n);
                if (r == 0)
                        return 0;
        }

        elapse = when_wall(n, elapse);
        if (elapse == 0)
                return 0;

        r = event_reset_time(m->event, &m->wall_message_timeout_source,
                             CLOCK_REALTIME,
                             n + elapse, 0,
                             wall_message_timeout_handler, m,
                             0, "wall-message-timer", true);

        if (r < 0) {
                m->wall_message_timeout_source = sd_event_source_unref(m->wall_message_timeout_source);
                return log_error_errno(r, "Failed to set up wall message timer: %m");
        }

        return 0;
}
