/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>
#include <sys/types.h>

typedef struct UidRange {
        uid_t start, nr;
} UidRange;

int uid_range_add(UidRange **p, size_t *n, uid_t start, uid_t nr);
int uid_range_add_str(UidRange **p, size_t *n, const char *s);

int uid_range_next_lower(const UidRange *p, size_t n, uid_t *uid);
bool uid_range_covers(const UidRange *p, size_t n, uid_t start, uid_t nr);

static inline bool uid_range_contains(const UidRange *p, size_t n, uid_t uid) {
        return uid_range_covers(p, n, uid, 1);
}

int uid_range_load_userns(UidRange **p, size_t *n, const char *path);
