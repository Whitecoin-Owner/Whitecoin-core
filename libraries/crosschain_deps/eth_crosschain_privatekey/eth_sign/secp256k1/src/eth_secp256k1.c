/**********************************************************************
 * Copyright (c) 2013-2015 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "include/eth_secp256k1.h"
#include "include/eth_secp256k1_recovery.h"

#include "util.h"
#include "num_impl.h"
#include "field_impl.h"
#include "scalar_impl.h"
#include "group_impl.h"
#include "ecmult_impl.h"
#include "ecmult_const_impl.h"
#include "ecmult_gen_impl.h"
#include "ecdsa_impl.h"
#include "eckey_impl.h"
#include "hash_impl.h"

#define ARG_CHECK(cond) do { \
    if (EXPECT(!(cond), 0)) { \
        eth_secp256k1_callback_call(&ctx->illegal_callback, #cond); \
        return 0; \
    } \
} while(0)

static void default_illegal_callback_fn(const char* str, void* data) {
    (void)data;
    fprintf(stderr, "[libeth_secp256k1] illegal argument: %s\n", str);
    abort();
}

static const eth_secp256k1_callback default_illegal_callback = {
    default_illegal_callback_fn,
    NULL
};

static void default_error_callback_fn(const char* str, void* data) {
    (void)data;
    fprintf(stderr, "[libeth_secp256k1] internal consistency check failed: %s\n", str);
    abort();
}

static const eth_secp256k1_callback default_error_callback = {
    default_error_callback_fn,
    NULL
};


struct eth_secp256k1_context_struct {
    eth_secp256k1_ecmult_context ecmult_ctx;
    eth_secp256k1_ecmult_gen_context ecmult_gen_ctx;
    eth_secp256k1_callback illegal_callback;
    eth_secp256k1_callback error_callback;
};

eth_secp256k1_context* eth_secp256k1_context_create(unsigned int flags) {
    eth_secp256k1_context* ret = (eth_secp256k1_context*)checked_malloc(&default_error_callback, sizeof(eth_secp256k1_context));
    ret->illegal_callback = default_illegal_callback;
    ret->error_callback = default_error_callback;

    if (EXPECT((flags & ETH_SECP256K1_FLAGS_TYPE_MASK) != ETH_SECP256K1_FLAGS_TYPE_CONTEXT, 0)) {
            eth_secp256k1_callback_call(&ret->illegal_callback,
                                    "Invalid flags");
            free(ret);
            return NULL;
    }

    eth_secp256k1_ecmult_context_init(&ret->ecmult_ctx);
    eth_secp256k1_ecmult_gen_context_init(&ret->ecmult_gen_ctx);

    if (flags & ETH_SECP256K1_FLAGS_BIT_CONTEXT_SIGN) {
        eth_secp256k1_ecmult_gen_context_build(&ret->ecmult_gen_ctx, &ret->error_callback);
    }
    if (flags & ETH_SECP256K1_FLAGS_BIT_CONTEXT_VERIFY) {
        eth_secp256k1_ecmult_context_build(&ret->ecmult_ctx, &ret->error_callback);
    }

    return ret;
}

eth_secp256k1_context* eth_secp256k1_context_clone(const eth_secp256k1_context* ctx) {
    eth_secp256k1_context* ret = (eth_secp256k1_context*)checked_malloc(&ctx->error_callback, sizeof(eth_secp256k1_context));
    ret->illegal_callback = ctx->illegal_callback;
    ret->error_callback = ctx->error_callback;
    eth_secp256k1_ecmult_context_clone(&ret->ecmult_ctx, &ctx->ecmult_ctx, &ctx->error_callback);
    eth_secp256k1_ecmult_gen_context_clone(&ret->ecmult_gen_ctx, &ctx->ecmult_gen_ctx, &ctx->error_callback);
    return ret;
}

void eth_secp256k1_context_destroy(eth_secp256k1_context* ctx) {
    if (ctx != NULL) {
        eth_secp256k1_ecmult_context_clear(&ctx->ecmult_ctx);
        eth_secp256k1_ecmult_gen_context_clear(&ctx->ecmult_gen_ctx);

        free(ctx);
    }
}

void eth_secp256k1_context_set_illegal_callback(eth_secp256k1_context* ctx, void (*fun)(const char* message, void* data), const void* data) {
    if (fun == NULL) {
        fun = default_illegal_callback_fn;
    }
    ctx->illegal_callback.fn = fun;
    ctx->illegal_callback.data = data;
}

void eth_secp256k1_context_set_error_callback(eth_secp256k1_context* ctx, void (*fun)(const char* message, void* data), const void* data) {
    if (fun == NULL) {
        fun = default_error_callback_fn;
    }
    ctx->error_callback.fn = fun;
    ctx->error_callback.data = data;
}

static int eth_secp256k1_pubkey_load(const eth_secp256k1_context* ctx, eth_secp256k1_ge* ge, const eth_secp256k1_pubkey* pubkey) {
    if (sizeof(eth_secp256k1_ge_storage) == 64) {
        /* When the eth_secp256k1_ge_storage type is exactly 64 byte, use its
         * representation inside eth_secp256k1_pubkey, as conversion is very fast.
         * Note that eth_secp256k1_pubkey_save must use the same representation. */
        eth_secp256k1_ge_storage s;
        memcpy(&s, &pubkey->data[0], 64);
        eth_secp256k1_ge_from_storage(ge, &s);
    } else {
        /* Otherwise, fall back to 32-byte big endian for X and Y. */
        eth_secp256k1_fe x, y;
        eth_secp256k1_fe_set_b32(&x, pubkey->data);
        eth_secp256k1_fe_set_b32(&y, pubkey->data + 32);
        eth_secp256k1_ge_set_xy(ge, &x, &y);
    }
    ARG_CHECK(!eth_secp256k1_fe_is_zero(&ge->x));
    return 1;
}

static void eth_secp256k1_pubkey_save(eth_secp256k1_pubkey* pubkey, eth_secp256k1_ge* ge) {
    if (sizeof(eth_secp256k1_ge_storage) == 64) {
        eth_secp256k1_ge_storage s;
        eth_secp256k1_ge_to_storage(&s, ge);
        memcpy(&pubkey->data[0], &s, 64);
    } else {
        VERIFY_CHECK(!eth_secp256k1_ge_is_infinity(ge));
        eth_secp256k1_fe_normalize_var(&ge->x);
        eth_secp256k1_fe_normalize_var(&ge->y);
        eth_secp256k1_fe_get_b32(pubkey->data, &ge->x);
        eth_secp256k1_fe_get_b32(pubkey->data + 32, &ge->y);
    }
}

int eth_secp256k1_ec_pubkey_parse(const eth_secp256k1_context* ctx, eth_secp256k1_pubkey* pubkey, const unsigned char *input, size_t inputlen) {
    eth_secp256k1_ge Q;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    memset(pubkey, 0, sizeof(*pubkey));
    ARG_CHECK(input != NULL);
    if (!eth_secp256k1_eckey_pubkey_parse(&Q, input, inputlen)) {
        return 0;
    }
    eth_secp256k1_pubkey_save(pubkey, &Q);
    eth_secp256k1_ge_clear(&Q);
    return 1;
}

int eth_secp256k1_ec_pubkey_serialize(const eth_secp256k1_context* ctx, unsigned char *output, size_t *outputlen, const eth_secp256k1_pubkey* pubkey, unsigned int flags) {
    eth_secp256k1_ge Q;
    size_t len;
    int ret = 0;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(outputlen != NULL);
    ARG_CHECK(*outputlen >= ((flags & ETH_SECP256K1_FLAGS_BIT_COMPRESSION) ? 33 : 65));
    len = *outputlen;
    *outputlen = 0;
    ARG_CHECK(output != NULL);
    memset(output, 0, len);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK((flags & ETH_SECP256K1_FLAGS_TYPE_MASK) == ETH_SECP256K1_FLAGS_TYPE_COMPRESSION);
    if (eth_secp256k1_pubkey_load(ctx, &Q, pubkey)) {
        ret = eth_secp256k1_eckey_pubkey_serialize(&Q, output, &len, flags & ETH_SECP256K1_FLAGS_BIT_COMPRESSION);
        if (ret) {
            *outputlen = len;
        }
    }
    return ret;
}

static void eth_secp256k1_ecdsa_signature_load(const eth_secp256k1_context* ctx, eth_secp256k1_scalar* r, eth_secp256k1_scalar* s, const eth_secp256k1_ecdsa_signature* sig) {
    (void)ctx;
    if (sizeof(eth_secp256k1_scalar) == 32) {
        /* When the eth_secp256k1_scalar type is exactly 32 byte, use its
         * representation inside eth_secp256k1_ecdsa_signature, as conversion is very fast.
         * Note that eth_secp256k1_ecdsa_signature_save must use the same representation. */
        memcpy(r, &sig->data[0], 32);
        memcpy(s, &sig->data[32], 32);
    } else {
        eth_secp256k1_scalar_set_b32(r, &sig->data[0], NULL);
        eth_secp256k1_scalar_set_b32(s, &sig->data[32], NULL);
    }
}

static void eth_secp256k1_ecdsa_signature_save(eth_secp256k1_ecdsa_signature* sig, const eth_secp256k1_scalar* r, const eth_secp256k1_scalar* s) {
    if (sizeof(eth_secp256k1_scalar) == 32) {
        memcpy(&sig->data[0], r, 32);
        memcpy(&sig->data[32], s, 32);
    } else {
        eth_secp256k1_scalar_get_b32(&sig->data[0], r);
        eth_secp256k1_scalar_get_b32(&sig->data[32], s);
    }
}

int eth_secp256k1_ecdsa_signature_parse_der(const eth_secp256k1_context* ctx, eth_secp256k1_ecdsa_signature* sig, const unsigned char *input, size_t inputlen) {
    eth_secp256k1_scalar r, s;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(input != NULL);

    if (eth_secp256k1_ecdsa_sig_parse(&r, &s, input, inputlen)) {
        eth_secp256k1_ecdsa_signature_save(sig, &r, &s);
        return 1;
    } else {
        memset(sig, 0, sizeof(*sig));
        return 0;
    }
}

int eth_secp256k1_ecdsa_signature_parse_compact(const eth_secp256k1_context* ctx, eth_secp256k1_ecdsa_signature* sig, const unsigned char *input64) {
    eth_secp256k1_scalar r, s;
    int ret = 1;
    int overflow = 0;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(input64 != NULL);

    eth_secp256k1_scalar_set_b32(&r, &input64[0], &overflow);
    ret &= !overflow;
    eth_secp256k1_scalar_set_b32(&s, &input64[32], &overflow);
    ret &= !overflow;
    if (ret) {
        eth_secp256k1_ecdsa_signature_save(sig, &r, &s);
    } else {
        memset(sig, 0, sizeof(*sig));
    }
    return ret;
}

int eth_secp256k1_ecdsa_signature_serialize_der(const eth_secp256k1_context* ctx, unsigned char *output, size_t *outputlen, const eth_secp256k1_ecdsa_signature* sig) {
    eth_secp256k1_scalar r, s;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output != NULL);
    ARG_CHECK(outputlen != NULL);
    ARG_CHECK(sig != NULL);

    eth_secp256k1_ecdsa_signature_load(ctx, &r, &s, sig);
    return eth_secp256k1_ecdsa_sig_serialize(output, outputlen, &r, &s);
}

int eth_secp256k1_ecdsa_signature_serialize_compact(const eth_secp256k1_context* ctx, unsigned char *output64, const eth_secp256k1_ecdsa_signature* sig) {
    eth_secp256k1_scalar r, s;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output64 != NULL);
    ARG_CHECK(sig != NULL);

    eth_secp256k1_ecdsa_signature_load(ctx, &r, &s, sig);
    eth_secp256k1_scalar_get_b32(&output64[0], &r);
    eth_secp256k1_scalar_get_b32(&output64[32], &s);
    return 1;
}

int eth_secp256k1_ecdsa_signature_normalize(const eth_secp256k1_context* ctx, eth_secp256k1_ecdsa_signature *sigout, const eth_secp256k1_ecdsa_signature *sigin) {
    eth_secp256k1_scalar r, s;
    int ret = 0;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sigin != NULL);

    eth_secp256k1_ecdsa_signature_load(ctx, &r, &s, sigin);
    ret = eth_secp256k1_scalar_is_high(&s);
    if (sigout != NULL) {
        if (ret) {
            eth_secp256k1_scalar_negate(&s, &s);
        }
        eth_secp256k1_ecdsa_signature_save(sigout, &r, &s);
    }

    return ret;
}

int eth_secp256k1_ecdsa_verify(const eth_secp256k1_context* ctx, const eth_secp256k1_ecdsa_signature *sig, const unsigned char *msg32, const eth_secp256k1_pubkey *pubkey) {
    eth_secp256k1_ge q;
    eth_secp256k1_scalar r, s;
    eth_secp256k1_scalar m;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(eth_secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(pubkey != NULL);

    eth_secp256k1_scalar_set_b32(&m, msg32, NULL);
    eth_secp256k1_ecdsa_signature_load(ctx, &r, &s, sig);
    return (!eth_secp256k1_scalar_is_high(&s) &&
            eth_secp256k1_pubkey_load(ctx, &q, pubkey) &&
            eth_secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &r, &s, &q, &m));
}

static int nonce_function_rfc6979(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int counter) {
   unsigned char keydata[112];
   int keylen = 64;
   eth_secp256k1_rfc6979_hmac_sha256_t rng;
   unsigned int i;
   /* We feed a byte array to the PRNG as input, consisting of:
    * - the private key (32 bytes) and message (32 bytes), see RFC 6979 3.2d.
    * - optionally 32 extra bytes of data, see RFC 6979 3.6 Additional Data.
    * - optionally 16 extra bytes with the algorithm name.
    * Because the arguments have distinct fixed lengths it is not possible for
    *  different argument mixtures to emulate each other and result in the same
    *  nonces.
    */
   memcpy(keydata, key32, 32);
   memcpy(keydata + 32, msg32, 32);
   if (data != NULL) {
       memcpy(keydata + 64, data, 32);
       keylen = 96;
   }
   if (algo16 != NULL) {
       memcpy(keydata + keylen, algo16, 16);
       keylen += 16;
   }
   eth_secp256k1_rfc6979_hmac_sha256_initialize(&rng, keydata, keylen);
   memset(keydata, 0, sizeof(keydata));
   for (i = 0; i <= counter; i++) {
       eth_secp256k1_rfc6979_hmac_sha256_generate(&rng, nonce32, 32);
   }
   eth_secp256k1_rfc6979_hmac_sha256_finalize(&rng);
   return 1;
}

const eth_secp256k1_nonce_function eth_secp256k1_nonce_function_rfc6979 = nonce_function_rfc6979;
const eth_secp256k1_nonce_function eth_secp256k1_nonce_function_default = nonce_function_rfc6979;

int eth_secp256k1_ecdsa_sign(const eth_secp256k1_context* ctx, eth_secp256k1_ecdsa_signature *signature, const unsigned char *msg32, const unsigned char *seckey, eth_secp256k1_nonce_function noncefp, const void* noncedata) {
    eth_secp256k1_scalar r, s;
    eth_secp256k1_scalar sec, non, msg;
    int ret = 0;
    int overflow = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(eth_secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(signature != NULL);
    ARG_CHECK(seckey != NULL);
    if (noncefp == NULL) {
        noncefp = eth_secp256k1_nonce_function_default;
    }

    eth_secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    /* Fail if the secret key is invalid. */
    if (!overflow && !eth_secp256k1_scalar_is_zero(&sec)) {
        unsigned char nonce32[32];
        unsigned int count = 0;
        eth_secp256k1_scalar_set_b32(&msg, msg32, NULL);
        while (1) {
            ret = noncefp(nonce32, msg32, seckey, NULL, (void*)noncedata, count);
            if (!ret) {
                break;
            }
            eth_secp256k1_scalar_set_b32(&non, nonce32, &overflow);
            if (!overflow && !eth_secp256k1_scalar_is_zero(&non)) {
                if (eth_secp256k1_ecdsa_sig_sign(&ctx->ecmult_gen_ctx, &r, &s, &sec, &msg, &non, NULL)) {
                    break;
                }
            }
            count++;
        }
        memset(nonce32, 0, 32);
        eth_secp256k1_scalar_clear(&msg);
        eth_secp256k1_scalar_clear(&non);
        eth_secp256k1_scalar_clear(&sec);
    }
    if (ret) {
        eth_secp256k1_ecdsa_signature_save(signature, &r, &s);
    } else {
        memset(signature, 0, sizeof(*signature));
    }
    return ret;
}

int eth_secp256k1_ec_seckey_verify(const eth_secp256k1_context* ctx, const unsigned char *seckey) {
    eth_secp256k1_scalar sec;
    int ret;
    int overflow;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(seckey != NULL);

    eth_secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    ret = !overflow && !eth_secp256k1_scalar_is_zero(&sec);
    eth_secp256k1_scalar_clear(&sec);
    return ret;
}

int eth_secp256k1_ec_pubkey_create(const eth_secp256k1_context* ctx, eth_secp256k1_pubkey *pubkey, const unsigned char *seckey) {
    eth_secp256k1_gej pj;
    eth_secp256k1_ge p;
    eth_secp256k1_scalar sec;
    int overflow;
    int ret = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    memset(pubkey, 0, sizeof(*pubkey));
    ARG_CHECK(eth_secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(seckey != NULL);

    eth_secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    ret = (!overflow) & (!eth_secp256k1_scalar_is_zero(&sec));
    if (ret) {
        eth_secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &pj, &sec);
        eth_secp256k1_ge_set_gej(&p, &pj);
        eth_secp256k1_pubkey_save(pubkey, &p);
    }
    eth_secp256k1_scalar_clear(&sec);
    return ret;
}

int eth_secp256k1_ec_privkey_negate(const eth_secp256k1_context* ctx, unsigned char *seckey) {
    eth_secp256k1_scalar sec;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(seckey != NULL);

    eth_secp256k1_scalar_set_b32(&sec, seckey, NULL);
    eth_secp256k1_scalar_negate(&sec, &sec);
    eth_secp256k1_scalar_get_b32(seckey, &sec);

    return 1;
}

int eth_secp256k1_ec_pubkey_negate(const eth_secp256k1_context* ctx, eth_secp256k1_pubkey *pubkey) {
    int ret = 0;
    eth_secp256k1_ge p;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);

    ret = eth_secp256k1_pubkey_load(ctx, &p, pubkey);
    memset(pubkey, 0, sizeof(*pubkey));
    if (ret) {
        eth_secp256k1_ge_neg(&p, &p);
        eth_secp256k1_pubkey_save(pubkey, &p);
    }
    return ret;
}

int eth_secp256k1_ec_privkey_tweak_add(const eth_secp256k1_context* ctx, unsigned char *seckey, const unsigned char *tweak) {
    eth_secp256k1_scalar term;
    eth_secp256k1_scalar sec;
    int ret = 0;
    int overflow = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(seckey != NULL);
    ARG_CHECK(tweak != NULL);

    eth_secp256k1_scalar_set_b32(&term, tweak, &overflow);
    eth_secp256k1_scalar_set_b32(&sec, seckey, NULL);

    ret = !overflow && eth_secp256k1_eckey_privkey_tweak_add(&sec, &term);
    memset(seckey, 0, 32);
    if (ret) {
        eth_secp256k1_scalar_get_b32(seckey, &sec);
    }

    eth_secp256k1_scalar_clear(&sec);
    eth_secp256k1_scalar_clear(&term);
    return ret;
}

int eth_secp256k1_ec_pubkey_tweak_add(const eth_secp256k1_context* ctx, eth_secp256k1_pubkey *pubkey, const unsigned char *tweak) {
    eth_secp256k1_ge p;
    eth_secp256k1_scalar term;
    int ret = 0;
    int overflow = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(eth_secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(tweak != NULL);

    eth_secp256k1_scalar_set_b32(&term, tweak, &overflow);
    ret = !overflow && eth_secp256k1_pubkey_load(ctx, &p, pubkey);
    memset(pubkey, 0, sizeof(*pubkey));
    if (ret) {
        if (eth_secp256k1_eckey_pubkey_tweak_add(&ctx->ecmult_ctx, &p, &term)) {
            eth_secp256k1_pubkey_save(pubkey, &p);
        } else {
            ret = 0;
        }
    }

    return ret;
}

int eth_secp256k1_ec_privkey_tweak_mul(const eth_secp256k1_context* ctx, unsigned char *seckey, const unsigned char *tweak) {
    eth_secp256k1_scalar factor;
    eth_secp256k1_scalar sec;
    int ret = 0;
    int overflow = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(seckey != NULL);
    ARG_CHECK(tweak != NULL);

    eth_secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    eth_secp256k1_scalar_set_b32(&sec, seckey, NULL);
    ret = !overflow && eth_secp256k1_eckey_privkey_tweak_mul(&sec, &factor);
    memset(seckey, 0, 32);
    if (ret) {
        eth_secp256k1_scalar_get_b32(seckey, &sec);
    }

    eth_secp256k1_scalar_clear(&sec);
    eth_secp256k1_scalar_clear(&factor);
    return ret;
}

int eth_secp256k1_ec_pubkey_tweak_mul(const eth_secp256k1_context* ctx, eth_secp256k1_pubkey *pubkey, const unsigned char *tweak) {
    eth_secp256k1_ge p;
    eth_secp256k1_scalar factor;
    int ret = 0;
    int overflow = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(eth_secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(tweak != NULL);

    eth_secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    ret = !overflow && eth_secp256k1_pubkey_load(ctx, &p, pubkey);
    memset(pubkey, 0, sizeof(*pubkey));
    if (ret) {
        if (eth_secp256k1_eckey_pubkey_tweak_mul(&ctx->ecmult_ctx, &p, &factor)) {
            eth_secp256k1_pubkey_save(pubkey, &p);
        } else {
            ret = 0;
        }
    }

    return ret;
}

int eth_secp256k1_context_randomize(eth_secp256k1_context* ctx, const unsigned char *seed32) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(eth_secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    eth_secp256k1_ecmult_gen_blind(&ctx->ecmult_gen_ctx, seed32);
    return 1;
}

int eth_secp256k1_ec_pubkey_combine(const eth_secp256k1_context* ctx, eth_secp256k1_pubkey *pubnonce, const eth_secp256k1_pubkey * const *pubnonces, size_t n) {
    size_t i;
    eth_secp256k1_gej Qj;
    eth_secp256k1_ge Q;

    ARG_CHECK(pubnonce != NULL);
    memset(pubnonce, 0, sizeof(*pubnonce));
    ARG_CHECK(n >= 1);
    ARG_CHECK(pubnonces != NULL);

    eth_secp256k1_gej_set_infinity(&Qj);

    for (i = 0; i < n; i++) {
        eth_secp256k1_pubkey_load(ctx, &Q, pubnonces[i]);
        eth_secp256k1_gej_add_ge(&Qj, &Qj, &Q);
    }
    if (eth_secp256k1_gej_is_infinity(&Qj)) {
        return 0;
    }
    eth_secp256k1_ge_set_gej(&Q, &Qj);
    eth_secp256k1_pubkey_save(pubnonce, &Q);
    return 1;
}



static void eth_secp256k1_ecdsa_recoverable_signature_load(const eth_secp256k1_context* ctx, eth_secp256k1_scalar* r, eth_secp256k1_scalar* s, int* recid, const eth_secp256k1_ecdsa_recoverable_signature* sig) {
	(void)ctx;
	if (sizeof(eth_secp256k1_scalar) == 32) {
		/* When the eth_secp256k1_scalar type is exactly 32 byte, use its
		* representation inside eth_secp256k1_ecdsa_signature, as conversion is very fast.
		* Note that eth_secp256k1_ecdsa_signature_save must use the same representation. */
		memcpy(r, &sig->data[0], 32);
		memcpy(s, &sig->data[32], 32);
	}
	else {
		eth_secp256k1_scalar_set_b32(r, &sig->data[0], NULL);
		eth_secp256k1_scalar_set_b32(s, &sig->data[32], NULL);
	}
	*recid = sig->data[64];
}

static void eth_secp256k1_ecdsa_recoverable_signature_save(eth_secp256k1_ecdsa_recoverable_signature* sig, const eth_secp256k1_scalar* r, const eth_secp256k1_scalar* s, int recid) {
	if (sizeof(eth_secp256k1_scalar) == 32) {
		memcpy(&sig->data[0], r, 32);
		memcpy(&sig->data[32], s, 32);
	}
	else {
		eth_secp256k1_scalar_get_b32(&sig->data[0], r);
		eth_secp256k1_scalar_get_b32(&sig->data[32], s);
	}
	sig->data[64] = recid;
}

int eth_secp256k1_ecdsa_recoverable_signature_parse_compact(const eth_secp256k1_context* ctx, eth_secp256k1_ecdsa_recoverable_signature* sig, const unsigned char *input64, int recid) {
	eth_secp256k1_scalar r, s;
	int ret = 1;
	int overflow = 0;

	(void)ctx;
	ARG_CHECK(sig != NULL);
	ARG_CHECK(input64 != NULL);
	ARG_CHECK(recid >= 0 && recid <= 3);

	eth_secp256k1_scalar_set_b32(&r, &input64[0], &overflow);
	ret &= !overflow;
	eth_secp256k1_scalar_set_b32(&s, &input64[32], &overflow);
	ret &= !overflow;
	if (ret) {
		eth_secp256k1_ecdsa_recoverable_signature_save(sig, &r, &s, recid);
	}
	else {
		memset(sig, 0, sizeof(*sig));
	}
	return ret;
}

int eth_secp256k1_ecdsa_recoverable_signature_serialize_compact(const eth_secp256k1_context* ctx, unsigned char *output64, int *recid, const eth_secp256k1_ecdsa_recoverable_signature* sig) {
	eth_secp256k1_scalar r, s;

	(void)ctx;
	ARG_CHECK(output64 != NULL);
	ARG_CHECK(sig != NULL);
	ARG_CHECK(recid != NULL);

	eth_secp256k1_ecdsa_recoverable_signature_load(ctx, &r, &s, recid, sig);
	eth_secp256k1_scalar_get_b32(&output64[0], &r);
	eth_secp256k1_scalar_get_b32(&output64[32], &s);
	return 1;
}
extern void eth_secp256k1_ecdsa_signature_save(eth_secp256k1_ecdsa_signature* sig, const eth_secp256k1_scalar* r, const eth_secp256k1_scalar* s);
int eth_secp256k1_ecdsa_recoverable_signature_convert(const eth_secp256k1_context* ctx, eth_secp256k1_ecdsa_signature* sig, const eth_secp256k1_ecdsa_recoverable_signature* sigin) {
	eth_secp256k1_scalar r, s;
	int recid;

	(void)ctx;
	ARG_CHECK(sig != NULL);
	ARG_CHECK(sigin != NULL);

	eth_secp256k1_ecdsa_recoverable_signature_load(ctx, &r, &s, &recid, sigin);
	eth_secp256k1_ecdsa_signature_save(sig, &r, &s);
	return 1;
}

static int eth_secp256k1_ecdsa_sig_recover(const eth_secp256k1_ecmult_context *ctx, const eth_secp256k1_scalar *sigr, const eth_secp256k1_scalar* sigs, eth_secp256k1_ge *pubkey, const eth_secp256k1_scalar *message, int recid) {
	unsigned char brx[32];
	eth_secp256k1_fe fx;
	eth_secp256k1_ge x;
	eth_secp256k1_gej xj;
	eth_secp256k1_scalar rn, u1, u2;
	eth_secp256k1_gej qj;
	int r;

	if (eth_secp256k1_scalar_is_zero(sigr) || eth_secp256k1_scalar_is_zero(sigs)) {
		return 0;
	}

	eth_secp256k1_scalar_get_b32(brx, sigr);
	r = eth_secp256k1_fe_set_b32(&fx, brx);
	(void)r;
	VERIFY_CHECK(r); /* brx comes from a scalar, so is less than the order; certainly less than p */
	if (recid & 2) {
		if (eth_secp256k1_fe_cmp_var(&fx, &eth_secp256k1_ecdsa_const_p_minus_order) >= 0) {
			return 0;
		}
		eth_secp256k1_fe_add(&fx, &eth_secp256k1_ecdsa_const_order_as_fe);
	}
	if (!eth_secp256k1_ge_set_xo_var(&x, &fx, recid & 1)) {
		return 0;
	}
	eth_secp256k1_gej_set_ge(&xj, &x);
	eth_secp256k1_scalar_inverse_var(&rn, sigr);
	eth_secp256k1_scalar_mul(&u1, &rn, message);
	eth_secp256k1_scalar_negate(&u1, &u1);
	eth_secp256k1_scalar_mul(&u2, &rn, sigs);
	eth_secp256k1_ecmult(ctx, &qj, &xj, &u2, &u1);
	eth_secp256k1_ge_set_gej_var(pubkey, &qj);
	return !eth_secp256k1_gej_is_infinity(&qj);
}

int eth_secp256k1_ecdsa_sign_recoverable(const eth_secp256k1_context* ctx, eth_secp256k1_ecdsa_recoverable_signature *signature, const unsigned char *msg32, const unsigned char *seckey, eth_secp256k1_nonce_function noncefp, const void* noncedata) {
	eth_secp256k1_scalar r, s;
	eth_secp256k1_scalar sec, non, msg;
	int recid;
	int ret = 0;
	int overflow = 0;
	VERIFY_CHECK(ctx != NULL);
	ARG_CHECK(eth_secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
	ARG_CHECK(msg32 != NULL);
	ARG_CHECK(signature != NULL);
	ARG_CHECK(seckey != NULL);
	if (noncefp == NULL) {
		noncefp = eth_secp256k1_nonce_function_default;
	}

	eth_secp256k1_scalar_set_b32(&sec, seckey, &overflow);
	/* Fail if the secret key is invalid. */
	if (!overflow && !eth_secp256k1_scalar_is_zero(&sec)) {
		unsigned char nonce32[32];
		unsigned int count = 0;
		eth_secp256k1_scalar_set_b32(&msg, msg32, NULL);
		while (1) {
			ret = noncefp(nonce32, msg32, seckey, NULL, (void*)noncedata, count);
			if (!ret) {
				break;
			}
			eth_secp256k1_scalar_set_b32(&non, nonce32, &overflow);
			if (!eth_secp256k1_scalar_is_zero(&non) && !overflow) {
				if (eth_secp256k1_ecdsa_sig_sign(&ctx->ecmult_gen_ctx, &r, &s, &sec, &msg, &non, &recid)) {
					break;
				}
			}
			count++;
		}
		memset(nonce32, 0, 32);
		eth_secp256k1_scalar_clear(&msg);
		eth_secp256k1_scalar_clear(&non);
		eth_secp256k1_scalar_clear(&sec);
	}
	if (ret) {
		eth_secp256k1_ecdsa_recoverable_signature_save(signature, &r, &s, recid);
	}
	else {
		memset(signature, 0, sizeof(*signature));
	}
	return ret;
}

int eth_secp256k1_ecdsa_recover(const eth_secp256k1_context* ctx, eth_secp256k1_pubkey *pubkey, const eth_secp256k1_ecdsa_recoverable_signature *signature, const unsigned char *msg32) {
	eth_secp256k1_ge q;
	eth_secp256k1_scalar r, s;
	eth_secp256k1_scalar m;
	int recid;
	VERIFY_CHECK(ctx != NULL);
	ARG_CHECK(eth_secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
	ARG_CHECK(msg32 != NULL);
	ARG_CHECK(signature != NULL);
	ARG_CHECK(pubkey != NULL);

	eth_secp256k1_ecdsa_recoverable_signature_load(ctx, &r, &s, &recid, signature);
	VERIFY_CHECK(recid >= 0 && recid < 4);  /* should have been caught in parse_compact */
	eth_secp256k1_scalar_set_b32(&m, msg32, NULL);
	if (eth_secp256k1_ecdsa_sig_recover(&ctx->ecmult_ctx, &r, &s, &q, &m, recid)) {
		eth_secp256k1_pubkey_save(pubkey, &q);
		return 1;
	}
	else {
		memset(pubkey, 0, sizeof(*pubkey));
		return 0;
	}
}




#ifdef ENABLE_MODULE_ECDH
# include "modules/ecdh/main_impl.h"
#endif

#ifdef ENABLE_MODULE_RECOVERY
# include "modules/recovery/main_impl.h"
#endif
