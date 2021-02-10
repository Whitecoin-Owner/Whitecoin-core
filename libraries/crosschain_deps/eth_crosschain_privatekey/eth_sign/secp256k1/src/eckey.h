/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_ECKEY_
#define _ETH_SECP256K1_ECKEY_

#include <stddef.h>

#include "group.h"
#include "scalar.h"
#include "ecmult.h"
#include "ecmult_gen.h"

static int eth_secp256k1_eckey_pubkey_parse(eth_secp256k1_ge *elem, const unsigned char *pub, size_t size);
static int eth_secp256k1_eckey_pubkey_serialize(eth_secp256k1_ge *elem, unsigned char *pub, size_t *size, int compressed);

static int eth_secp256k1_eckey_privkey_tweak_add(eth_secp256k1_scalar *key, const eth_secp256k1_scalar *tweak);
static int eth_secp256k1_eckey_pubkey_tweak_add(const eth_secp256k1_ecmult_context *ctx, eth_secp256k1_ge *key, const eth_secp256k1_scalar *tweak);
static int eth_secp256k1_eckey_privkey_tweak_mul(eth_secp256k1_scalar *key, const eth_secp256k1_scalar *tweak);
static int eth_secp256k1_eckey_pubkey_tweak_mul(const eth_secp256k1_ecmult_context *ctx, eth_secp256k1_ge *key, const eth_secp256k1_scalar *tweak);

#endif
