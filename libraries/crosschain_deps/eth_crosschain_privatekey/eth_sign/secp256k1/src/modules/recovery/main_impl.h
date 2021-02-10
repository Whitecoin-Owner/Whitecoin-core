/**********************************************************************
 * Copyright (c) 2013-2015 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_MODULE_RECOVERY_MAIN_
#define _ETH_SECP256K1_MODULE_RECOVERY_MAIN_

#include "include/eth_secp256k1_recovery.h"
#include <scalar.h>
#include <util.h>
#define ARG_CHECK(cond) do { \
    if (EXPECT(!(cond), 0)) { \
        eth_secp256k1_callback_call(&ctx->illegal_callback, #cond); \
        return 0; \
    } \
} while(0)

static void eth_secp256k1_ecdsa_recoverable_signature_load(const eth_secp256k1_context* ctx, eth_secp256k1_scalar* r, eth_secp256k1_scalar* s, int* recid, const eth_secp256k1_ecdsa_recoverable_signature* sig) {
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
    *recid = sig->data[64];
}

static void eth_secp256k1_ecdsa_recoverable_signature_save(eth_secp256k1_ecdsa_recoverable_signature* sig, const eth_secp256k1_scalar* r, const eth_secp256k1_scalar* s, int recid) {
    if (sizeof(eth_secp256k1_scalar) == 32) {
        memcpy(&sig->data[0], r, 32);
        memcpy(&sig->data[32], s, 32);
    } else {
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
    } else {
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
    } else {
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
    } else {
        memset(pubkey, 0, sizeof(*pubkey));
        return 0;
    }
}

#endif
