/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "openssl-util.h"
#include "alloc-util.h"
#include "hexdecoct.h"

#if HAVE_OPENSSL
int openssl_hash(const EVP_MD *alg,
                 const void *msg,
                 size_t msg_len,
                 uint8_t *ret_hash,
                 size_t *ret_hash_len) {

        _cleanup_(EVP_MD_CTX_freep) EVP_MD_CTX *ctx = NULL;
        unsigned len;
        int r;

        ctx = EVP_MD_CTX_new();
        if (!ctx)
                /* This function just calls OPENSSL_zalloc, so failure
                 * here is almost certainly a failed allocation. */
                return -ENOMEM;

        /* The documentation claims EVP_DigestInit behaves just like
         * EVP_DigestInit_ex if passed NULL, except it also calls
         * EVP_MD_CTX_reset, which deinitializes the context. */
        r = EVP_DigestInit_ex(ctx, alg, NULL);
        if (r == 0)
                return -EIO;

        r = EVP_DigestUpdate(ctx, msg, msg_len);
        if (r == 0)
                return -EIO;

        r = EVP_DigestFinal_ex(ctx, ret_hash, &len);
        if (r == 0)
                return -EIO;

        if (ret_hash_len)
                *ret_hash_len = len;

        return 0;
}

int rsa_encrypt_bytes(
                EVP_PKEY *pkey,
                const void *decrypted_key,
                size_t decrypted_key_size,
                void **ret_encrypt_key,
                size_t *ret_encrypt_key_size) {

        _cleanup_(EVP_PKEY_CTX_freep) EVP_PKEY_CTX *ctx = NULL;
        _cleanup_free_ void *b = NULL;
        size_t l;

        ctx = EVP_PKEY_CTX_new(pkey, NULL);
        if (!ctx)
                return log_debug_errno(SYNTHETIC_ERRNO(EIO), "Failed to allocate public key context");

        if (EVP_PKEY_encrypt_init(ctx) <= 0)
                return log_debug_errno(SYNTHETIC_ERRNO(EIO), "Failed to initialize public key context");

        if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0)
                return log_debug_errno(SYNTHETIC_ERRNO(EIO), "Failed to configure PKCS#1 padding");

        if (EVP_PKEY_encrypt(ctx, NULL, &l, decrypted_key, decrypted_key_size) <= 0)
                return log_debug_errno(SYNTHETIC_ERRNO(EIO), "Failed to determine encrypted key size");

        b = malloc(l);
        if (!b)
                return -ENOMEM;

        if (EVP_PKEY_encrypt(ctx, b, &l, decrypted_key, decrypted_key_size) <= 0)
                return log_debug_errno(SYNTHETIC_ERRNO(EIO), "Failed to determine encrypted key size");

        *ret_encrypt_key = TAKE_PTR(b);
        *ret_encrypt_key_size = l;

        return 0;
}

int rsa_pkey_to_suitable_key_size(
                EVP_PKEY *pkey,
                size_t *ret_suitable_key_size) {

        size_t suitable_key_size;
        int bits;

        assert_se(pkey);
        assert_se(ret_suitable_key_size);

        /* Analyzes the specified public key and that it is RSA. If so, will return a suitable size for a
         * disk encryption key to encrypt with RSA for use in PKCS#11 security token schemes. */

        if (EVP_PKEY_base_id(pkey) != EVP_PKEY_RSA)
                return log_debug_errno(SYNTHETIC_ERRNO(EBADMSG), "X.509 certificate does not refer to RSA key.");

        bits = EVP_PKEY_bits(pkey);
        log_debug("Bits in RSA key: %i", bits);

        /* We use PKCS#1 padding for the RSA cleartext, hence let's leave some extra space for it, hence only
         * generate a random key half the size of the RSA length */
        suitable_key_size = bits / 8 / 2;

        if (suitable_key_size < 1)
                return log_debug_errno(SYNTHETIC_ERRNO(EIO), "Uh, RSA key size too short?");

        *ret_suitable_key_size = suitable_key_size;
        return 0;
}

#  if PREFER_OPENSSL
int string_hashsum(
                const char *s,
                size_t len,
                const EVP_MD *md_algorithm,
                char **ret) {

        uint8_t hash[EVP_MAX_MD_SIZE];
        size_t hash_size;
        char *enc;
        int r;

        hash_size = EVP_MD_size(md_algorithm);
        assert(hash_size > 0);

        r = openssl_hash(md_algorithm, s, len, hash, NULL);
        if (r < 0)
                return r;

        enc = hexmem(hash, hash_size);
        if (!enc)
                return -ENOMEM;

        *ret = enc;
        return 0;

}
#  endif
#endif
