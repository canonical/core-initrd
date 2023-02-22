/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "env-util.h"
#include "fileio.h"

/* The entry point into the fuzzer */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static inline FILE* data_to_file(const uint8_t *data, size_t size) {
        if (size == 0)
                return fopen("/dev/null", "re");
        else
                return fmemopen_unlocked((char*) data, size, "re");
}

/* Check if we are within the specified size range.
 * The upper limit is ignored if FUZZ_USE_SIZE_LIMIT is unset.
 */
static inline bool outside_size_range(size_t size, size_t lower, size_t upper) {
        if (size < lower)
                return true;
        if (size > upper)
                return FUZZ_USE_SIZE_LIMIT;
        return false;
}
