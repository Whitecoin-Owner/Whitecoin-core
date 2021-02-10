/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_ECMULT_
#define _ETH_SECP256K1_ECMULT_

#include "num.h"
#include "group.h"

typedef struct {
    /* For accelerating the computation of a*P + b*G: */
    eth_secp256k1_ge_storage (*pre_g)[];    /* odd multiples of the generator */
#ifdef USE_ENDOMORPHISM
    eth_secp256k1_ge_storage (*pre_g_128)[]; /* odd multiples of 2^128*generator */
#endif
} eth_secp256k1_ecmult_context;

static void eth_secp256k1_ecmult_context_init(eth_secp256k1_ecmult_context *ctx);
static void eth_secp256k1_ecmult_context_build(eth_secp256k1_ecmult_context *ctx, const eth_secp256k1_callback *cb);
static void eth_secp256k1_ecmult_context_clone(eth_secp256k1_ecmult_context *dst,
                                           const eth_secp256k1_ecmult_context *src, const eth_secp256k1_callback *cb);
static void eth_secp256k1_ecmult_context_clear(eth_secp256k1_ecmult_context *ctx);
static int eth_secp256k1_ecmult_context_is_built(const eth_secp256k1_ecmult_context *ctx);

/** Double multiply: R = na*A + ng*G */
static void eth_secp256k1_ecmult(const eth_secp256k1_ecmult_context *ctx, eth_secp256k1_gej *r, const eth_secp256k1_gej *a, const eth_secp256k1_scalar *na, const eth_secp256k1_scalar *ng);

#endif
