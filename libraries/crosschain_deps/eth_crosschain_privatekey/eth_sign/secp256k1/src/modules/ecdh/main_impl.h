/**********************************************************************
 * Copyright (c) 2015 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_MODULE_ECDH_MAIN_
#define _ETH_SECP256K1_MODULE_ECDH_MAIN_

#include "include/eth_secp256k1_ecdh.h"
#include "ecmult_const_impl.h"

int eth_secp256k1_ecdh_raw(const eth_secp256k1_context* ctx, unsigned char *result, const eth_secp256k1_pubkey *point, const unsigned char *scalar) {
    int ret = 0;
    int overflow = 0;
    eth_secp256k1_gej res;
    eth_secp256k1_ge pt;
    eth_secp256k1_scalar s;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(result != NULL);
    ARG_CHECK(point != NULL);
    ARG_CHECK(scalar != NULL);

    eth_secp256k1_pubkey_load(ctx, &pt, point);
    eth_secp256k1_scalar_set_b32(&s, scalar, &overflow);
    if (overflow || eth_secp256k1_scalar_is_zero(&s)) {
        ret = 0;
    } else {
        eth_secp256k1_ecmult_const(&res, &pt, &s);
        eth_secp256k1_ge_set_gej(&pt, &res);
        /* Output the point in compressed form.
         * Note we cannot use eth_secp256k1_eckey_pubkey_serialize here since it does not
         * expect its output to be secret and has a timing sidechannel. */
        eth_secp256k1_fe_normalize(&pt.x);
        eth_secp256k1_fe_normalize(&pt.y);
        result[0] = 0x02 | eth_secp256k1_fe_is_odd(&pt.y);
        eth_secp256k1_fe_get_b32(&result[1], &pt.x);
        ret = 1;
    }

    eth_secp256k1_scalar_clear(&s);
    return ret;
}

int eth_secp256k1_ecdh(const eth_secp256k1_context* ctx, unsigned char *result, const eth_secp256k1_pubkey *point, const unsigned char *scalar) {
    unsigned char shared[33];
    eth_secp256k1_sha256_t sha;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(result != NULL);

    if (!eth_secp256k1_ecdh_raw(ctx, shared, point, scalar)) {
        return 0;
    }

    eth_secp256k1_sha256_initialize(&sha);
    eth_secp256k1_sha256_write(&sha, shared, sizeof(shared));
    eth_secp256k1_sha256_finalize(&sha, result);
    return 1;
}

#endif
