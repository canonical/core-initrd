/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <sys/types.h>

#include "ask-password-api.h"
#include "cryptsetup-util.h"
#include "log.h"
#include "time-util.h"
#include "tpm2-util.h"

#if HAVE_TPM2

int acquire_tpm2_key(
                const char *volume_name,
                const char *device,
                uint32_t pcr_mask,
                uint16_t pcr_bank,
                uint16_t primary_alg,
                const char *key_file,
                size_t key_file_size,
                uint64_t key_file_offset,
                const void *key_data,
                size_t key_data_size,
                const void *policy_hash,
                size_t policy_hash_size,
                TPM2Flags flags,
                usec_t until,
                bool headless,
                AskPasswordFlags ask_password_flags,
                void **ret_decrypted_key,
                size_t *ret_decrypted_key_size);

int find_tpm2_auto_data(
                struct crypt_device *cd,
                uint32_t search_pcr_mask,
                int start_token,
                uint32_t *ret_pcr_mask,
                uint16_t *ret_pcr_bank,
                uint16_t *ret_primary_alg,
                void **ret_blob,
                size_t *ret_blob_size,
                void **ret_policy_hash,
                size_t *ret_policy_hash_size,
                int *ret_keyslot,
                int *ret_token,
                TPM2Flags *ret_flags);

#else

static inline int acquire_tpm2_key(
                const char *volume_name,
                const char *device,
                uint32_t pcr_mask,
                uint16_t pcr_bank,
                uint16_t primary_alg,
                const char *key_file,
                size_t key_file_size,
                uint64_t key_file_offset,
                const void *key_data,
                size_t key_data_size,
                const void *policy_hash,
                size_t policy_hash_size,
                TPM2Flags flags,
                usec_t until,
                bool headless,
                AskPasswordFlags ask_password_flags,
                void **ret_decrypted_key,
                size_t *ret_decrypted_key_size) {

        return log_error_errno(SYNTHETIC_ERRNO(EOPNOTSUPP),
                               "TPM2 support not available.");
}

static inline int find_tpm2_auto_data(
                struct crypt_device *cd,
                uint32_t search_pcr_mask,
                int start_token,
                uint32_t *ret_pcr_mask,
                uint16_t *ret_pcr_bank,
                uint16_t *ret_primary_alg,
                void **ret_blob,
                size_t *ret_blob_size,
                void **ret_policy_hash,
                size_t *ret_policy_hash_size,
                int *ret_keyslot,
                int *ret_token,
                TPM2Flags *ret_flags) {

        return log_error_errno(SYNTHETIC_ERRNO(EOPNOTSUPP),
                               "TPM2 support not available.");
}

#endif
