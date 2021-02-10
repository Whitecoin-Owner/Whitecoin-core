/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_ECDSA_
#define _ETH_SECP256K1_ECDSA_

#include <stddef.h>

#include "scalar.h"
#include "group.h"
#include "ecmult.h"

static int eth_secp256k1_ecdsa_sig_parse(eth_secp256k1_scalar *r, eth_secp256k1_scalar *s, const unsigned char *sig, size_t size);
static int eth_secp256k1_ecdsa_sig_serialize(unsigned char *sig, size_t *size, const eth_secp256k1_scalar *r, const eth_secp256k1_scalar *s);
static int eth_secp256k1_ecdsa_sig_verify(const eth_secp256k1_ecmult_context *ctx, const eth_secp256k1_scalar* r, const eth_secp256k1_scalar* s, const eth_secp256k1_ge *pubkey, const eth_secp256k1_scalar *message);
static int eth_secp256k1_ecdsa_sig_sign(const eth_secp256k1_ecmult_gen_context *ctx, eth_secp256k1_scalar* r, eth_secp256k1_scalar* s, const eth_secp256k1_scalar *seckey, const eth_secp256k1_scalar *message, const eth_secp256k1_scalar *nonce, int *recid);

#endif
