/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include "sd-bus.h"

#include "hashmap.h"

int bus_test_polkit(sd_bus_message *call, int capability, const char *action, const char **details, uid_t good_user, bool *_challenge, sd_bus_error *e);

int bus_verify_polkit_async(sd_bus_message *call, int capability, const char *action, const char **details, bool interactive, uid_t good_user, Hashmap **registry, sd_bus_error *error);
Hashmap *bus_verify_polkit_async_registry_free(Hashmap *registry);
