/* SPDX-License-Identifier: LGPL-2.1-or-later */

#ifdef SD_BOOT
#  include <efi.h>
#  include "macro-fundamental.h"
#  include "util.h"
#  define TEST_STATIC
#else
/* Provide our own "EFI API" if we are running as a unit test. */
#  include <stddef.h>
#  include <stdint.h>
#  include <uchar.h>
#  include "string-util-fundamental.h"

#  define CHAR8 char
#  define CHAR16 char16_t
#  define UINT8 uint8_t
#  define UINT16 uint16_t
#  define UINT32 uint32_t
#  define UINT64 uint64_t
#  define UINTN size_t
#  define strlena(s) strlen(s)
#  define strncaseeqa(a, b, n) strncaseeq((a), (b), (n))
#  define TEST_STATIC static
#endif

enum {
        SIG_BASE_BLOCK  = 1718052210, /* regf */
        SIG_KEY         = 27502,      /* nk */
        SIG_SUBKEY_FAST = 26220,      /* lf */
        SIG_KEY_VALUE   = 27510,      /* vk */
};

enum {
        REG_SZ       = 1,
        REG_MULTI_SZ = 7,
};

/* These structs contain a lot more members than we care for. They have all
 * been squashed into _padN for our convenience. */

typedef struct {
        UINT32 sig;
        UINT32 primary_seqnum;
        UINT32 secondary_seqnum;
        UINT64 _pad1;
        UINT32 version_major;
        UINT32 version_minor;
        UINT32 type;
        UINT32 _pad2;
        UINT32 root_cell_offset;
        UINT64 _pad3[507];
} _packed_ BaseBlock;
assert_cc(sizeof(BaseBlock) == 4096);
assert_cc(offsetof(BaseBlock, sig) == 0);
assert_cc(offsetof(BaseBlock, primary_seqnum) == 4);
assert_cc(offsetof(BaseBlock, secondary_seqnum) == 8);
assert_cc(offsetof(BaseBlock, version_major) == 20);
assert_cc(offsetof(BaseBlock, version_minor) == 24);
assert_cc(offsetof(BaseBlock, type) == 28);
assert_cc(offsetof(BaseBlock, root_cell_offset) == 36);

/* All offsets are relative to the base block and technically point to a hive
 * cell struct. But for our usecase we don't need to bother about that one,
 * so skip over the cell_size UINT32. */
#define HIVE_CELL_OFFSET (sizeof(BaseBlock) + 4)

typedef struct {
        UINT16 sig;
        UINT16 _pad1[13];
        UINT32 subkeys_offset;
        UINT32 _pad2;
        UINT32 n_key_values;
        UINT32 key_values_offset;
        UINT32 _pad3[7];
        UINT16 key_name_len;
        UINT16 _pad4;
        CHAR8 key_name[];
} _packed_ Key;
assert_cc(offsetof(Key, sig) == 0);
assert_cc(offsetof(Key, subkeys_offset) == 28);
assert_cc(offsetof(Key, n_key_values) == 36);
assert_cc(offsetof(Key, key_values_offset) == 40);
assert_cc(offsetof(Key, key_name_len) == 72);
assert_cc(offsetof(Key, key_name) == 76);

typedef struct {
        UINT16 sig;
        UINT16 n_entries;
        struct SubkeyFastEntry {
                UINT32 key_offset;
                CHAR8 name_hint[4];
        } _packed_ entries[];
} _packed_ SubkeyFast;
assert_cc(offsetof(SubkeyFast, sig) == 0);
assert_cc(offsetof(SubkeyFast, n_entries) == 2);
assert_cc(offsetof(SubkeyFast, entries) == 4);

typedef struct {
        UINT16 sig;
        UINT16 name_len;
        UINT32 data_size;
        UINT32 data_offset;
        UINT32 data_type;
        UINT32 _pad;
        CHAR8 name[];
} _packed_ KeyValue;
assert_cc(offsetof(KeyValue, sig) == 0);
assert_cc(offsetof(KeyValue, name_len) == 2);
assert_cc(offsetof(KeyValue, data_size) == 4);
assert_cc(offsetof(KeyValue, data_offset) == 8);
assert_cc(offsetof(KeyValue, data_type) == 12);
assert_cc(offsetof(KeyValue, name) == 20);

#define BAD_OFFSET(offset, len, max) \
        ((UINT64) (offset) + (len) >= (max))

#define BAD_STRUCT(type, offset, max) \
        ((UINT64) (offset) + sizeof(type) >= (max))

#define BAD_ARRAY(type, array, offset, array_len, max) \
        ((UINT64) (offset) + offsetof(type, array) + \
         sizeof((type){}.array[0]) * (UINT64) (array_len) >= (max))

static const Key *get_key(const UINT8 *bcd, UINT32 bcd_len, UINT32 offset, const CHAR8 *name);

static const Key *get_subkey(const UINT8 *bcd, UINT32 bcd_len, UINT32 offset, const CHAR8 *name) {
        assert(bcd);
        assert(name);

        if (BAD_STRUCT(SubkeyFast, offset, bcd_len))
                return NULL;

        const SubkeyFast *subkey = (const SubkeyFast *) (bcd + offset);
        if (subkey->sig != SIG_SUBKEY_FAST)
                return NULL;

        if (BAD_ARRAY(SubkeyFast, entries, offset, subkey->n_entries, bcd_len))
                return NULL;

        for (UINT16 i = 0; i < subkey->n_entries; i++) {
                if (!strncaseeqa(name, subkey->entries[i].name_hint, sizeof(subkey->entries[i].name_hint)))
                        continue;

                const Key *key = get_key(bcd, bcd_len, subkey->entries[i].key_offset, name);
                if (key)
                        return key;
        }

        return NULL;
}

/* We use NUL as registry path separators for convenience. To start from the root, begin
 * name with a NUL. Name must end with two NUL. The lookup depth is not restricted, so
 * name must be properly validated before calling get_key(). */
static const Key *get_key(const UINT8 *bcd, UINT32 bcd_len, UINT32 offset, const CHAR8 *name) {
        assert(bcd);
        assert(name);

        if (BAD_STRUCT(Key, offset, bcd_len))
                return NULL;

        const Key *key = (const Key *) (bcd + offset);
        if (key->sig != SIG_KEY)
                return NULL;

        if (BAD_ARRAY(Key, key_name, offset, key->key_name_len, bcd_len))
                return NULL;

        if (*name) {
                if (strncaseeqa(name, key->key_name, key->key_name_len) && strlena(name) == key->key_name_len)
                        name += key->key_name_len;
                else
                        return NULL;
        }

        name++;
        return *name ? get_subkey(bcd, bcd_len, key->subkeys_offset, name) : key;
}

static const KeyValue *get_key_value(const UINT8 *bcd, UINT32 bcd_len, const Key *key, const CHAR8 *name) {
        assert(bcd);
        assert(key);
        assert(name);

        if (key->n_key_values == 0)
                return NULL;

        if (BAD_OFFSET(key->key_values_offset, sizeof(UINT32) * (UINT64) key->n_key_values, bcd_len) ||
            (UINTN)(bcd + key->key_values_offset) % sizeof(UINT32) != 0)
                return NULL;

        const UINT32 *key_value_list = (const UINT32 *) (bcd + key->key_values_offset);
        for (UINT32 i = 0; i < key->n_key_values; i++) {
                UINT32 offset = *(key_value_list + i);

                if (BAD_STRUCT(KeyValue, offset, bcd_len))
                        continue;

                const KeyValue *kv = (const KeyValue *) (bcd + offset);
                if (kv->sig != SIG_KEY_VALUE)
                        continue;

                if (BAD_ARRAY(KeyValue, name, offset, kv->name_len, bcd_len))
                        continue;

                /* If most significant bit is set, data is stored in data_offset itself, but
                 * we are only interested in UTF16 strings. The only strings that could fit
                 * would have just one char in it, so let's not bother with this. */
                if (FLAGS_SET(kv->data_size, UINT32_C(1) << 31))
                        continue;

                if (BAD_OFFSET(kv->data_offset, kv->data_size, bcd_len))
                        continue;

                if (strncaseeqa(name, kv->name, kv->name_len) && strlena(name) == kv->name_len)
                        return kv;
        }

        return NULL;
}

/* The BCD store is really just a regular windows registry hive with a rather cryptic internal
 * key structure. On a running system it gets mounted to HKEY_LOCAL_MACHINE\BCD00000000.
 *
 * Of interest to us are the these two keys:
 * - \Objects\{bootmgr}\Elements\24000001
 *   This key is the "displayorder" property and contains a value of type REG_MULTI_SZ
 *   with the name "Element" that holds a {GUID} list (UTF16, NUL-separated).
 * - \Objects\{GUID}\Elements\12000004
 *   This key is the "description" property and contains a value of type REG_SZ with the
 *   name "Element" that holds a NUL-terminated UTF16 string.
 *
 * The GUIDs and properties are as reported by "bcdedit.exe /v".
 *
 * To get a title for the BCD store we first look at the displayorder property of {bootmgr}
 * (it always has the GUID 9dea862c-5cdd-4e70-acc1-f32b344d4795). If it contains more than
 * one GUID, the BCD is multi-boot and we stop looking. Otherwise we take that GUID, look it
 * up, and return its description property. */
TEST_STATIC CHAR16 *get_bcd_title(UINT8 *bcd, UINTN bcd_len) {
        assert(bcd);

        if (HIVE_CELL_OFFSET >= bcd_len)
                return NULL;

        BaseBlock *base_block = (BaseBlock *) bcd;
        if (base_block->sig != SIG_BASE_BLOCK ||
            base_block->version_major != 1 ||
            base_block->version_minor != 3 ||
            base_block->type != 0 ||
            base_block->primary_seqnum != base_block->secondary_seqnum)
                return NULL;

        bcd += HIVE_CELL_OFFSET;
        bcd_len -= HIVE_CELL_OFFSET;

        const Key *objects_key = get_key(
                bcd, bcd_len,
                base_block->root_cell_offset,
                (const CHAR8 *) "\0Objects\0");
        if (!objects_key)
                return NULL;

        const Key *displayorder_key = get_subkey(
                bcd, bcd_len,
                objects_key->subkeys_offset,
                (const CHAR8 *) "{9dea862c-5cdd-4e70-acc1-f32b344d4795}\0Elements\00024000001\0");
        if (!displayorder_key)
                return NULL;

        const KeyValue *displayorder_value = get_key_value(
                bcd, bcd_len,
                displayorder_key,
                (const CHAR8 *) "Element");
        if (!displayorder_value)
                return NULL;

        CHAR8 order_guid[sizeof("{00000000-0000-0000-0000-000000000000}\0")];
        if (displayorder_value->data_type != REG_MULTI_SZ ||
            displayorder_value->data_size != sizeof(CHAR16[sizeof(order_guid)]) ||
            (UINTN)(bcd + displayorder_value->data_offset) % sizeof(CHAR16) != 0)
                /* BCD is multi-boot. */
                return NULL;

        /* Keys are stored as ASCII in registry hives if the data fits (and GUIDS always should). */
        CHAR16 *order_guid_utf16 = (CHAR16 *) (bcd + displayorder_value->data_offset);
        for (UINTN i = 0; i < sizeof(order_guid) - 2; i++) {
                CHAR16 c = order_guid_utf16[i];
                switch (c) {
                case '-':
                case '{':
                case '}':
                case '0' ... '9':
                case 'a' ... 'f':
                case 'A' ... 'F':
                        order_guid[i] = c;
                        break;
                default:
                        /* Not a valid GUID. */
                        return NULL;
                }
        }
        /* Our functions expect the lookup key to be double-derminated. */
        order_guid[sizeof(order_guid) - 2] = '\0';
        order_guid[sizeof(order_guid) - 1] = '\0';

        const Key *default_key = get_subkey(bcd, bcd_len, objects_key->subkeys_offset, order_guid);
        if (!default_key)
                return NULL;

        const Key *description_key = get_subkey(
                bcd, bcd_len,
                default_key->subkeys_offset,
                (const CHAR8 *) "Elements\00012000004\0");
        if (!description_key)
                return NULL;

        const KeyValue *description_value = get_key_value(
                bcd, bcd_len,
                description_key,
                (const CHAR8 *) "Element");
        if (!description_value)
                return NULL;

        if (description_value->data_type != REG_SZ ||
            description_value->data_size < sizeof(CHAR16) ||
            description_value->data_size % sizeof(CHAR16) != 0 ||
            (UINTN)(bcd + description_value->data_offset) % sizeof(CHAR16))
                return NULL;

        /* The data should already be NUL-terminated. */
        CHAR16 *title = (CHAR16 *) (bcd + description_value->data_offset);
        title[description_value->data_size / sizeof(CHAR16) - 1] = '\0';
        return title;
}
