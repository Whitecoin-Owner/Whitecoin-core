/**********************************************************************
 * Copyright (c) 2015 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_ECMULT_CONST_
#define _ETH_SECP256K1_ECMULT_CONST_

#include "scalar.h"
#include "group.h"

static void eth_secp256k1_ecmult_const(eth_secp256k1_gej *r, const eth_secp256k1_ge *a, const eth_secp256k1_scalar *q);

#endif
