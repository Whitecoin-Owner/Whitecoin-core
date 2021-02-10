/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_ECKEY_IMPL_H_
#define _ETH_SECP256K1_ECKEY_IMPL_H_

#include "eckey.h"

#include "scalar.h"
#include "field.h"
#include "group.h"
#include "ecmult_gen.h"

static int eth_secp256k1_eckey_pubkey_parse(eth_secp256k1_ge *elem, const unsigned char *pub, size_t size) {
    if (size == 33 && (pub[0] == 0x02 || pub[0] == 0x03)) {
        eth_secp256k1_fe x;
        return eth_secp256k1_fe_set_b32(&x, pub+1) && eth_secp256k1_ge_set_xo_var(elem, &x, pub[0] == 0x03);
    } else if (size == 65 && (pub[0] == 0x04 || pub[0] == 0x06 || pub[0] == 0x07)) {
        eth_secp256k1_fe x, y;
        if (!eth_secp256k1_fe_set_b32(&x, pub+1) || !eth_secp256k1_fe_set_b32(&y, pub+33)) {
            return 0;
        }
        eth_secp256k1_ge_set_xy(elem, &x, &y);
        if ((pub[0] == 0x06 || pub[0] == 0x07) && eth_secp256k1_fe_is_odd(&y) != (pub[0] == 0x07)) {
            return 0;
        }
        return eth_secp256k1_ge_is_valid_var(elem);
    } else {
        return 0;
    }
}

static int eth_secp256k1_eckey_pubkey_serialize(eth_secp256k1_ge *elem, unsigned char *pub, size_t *size, int compressed) {
    if (eth_secp256k1_ge_is_infinity(elem)) {
        return 0;
    }
    eth_secp256k1_fe_normalize_var(&elem->x);
    eth_secp256k1_fe_normalize_var(&elem->y);
    eth_secp256k1_fe_get_b32(&pub[1], &elem->x);
    if (compressed) {
        *size = 33;
        pub[0] = 0x02 | (eth_secp256k1_fe_is_odd(&elem->y) ? 0x01 : 0x00);
    } else {
        *size = 65;
        pub[0] = 0x04;
        eth_secp256k1_fe_get_b32(&pub[33], &elem->y);
    }
    return 1;
}

static int eth_secp256k1_eckey_privkey_tweak_add(eth_secp256k1_scalar *key, const eth_secp256k1_scalar *tweak) {
    eth_secp256k1_scalar_add(key, key, tweak);
    if (eth_secp256k1_scalar_is_zero(key)) {
        return 0;
    }
    return 1;
}

static int eth_secp256k1_eckey_pubkey_tweak_add(const eth_secp256k1_ecmult_context *ctx, eth_secp256k1_ge *key, const eth_secp256k1_scalar *tweak) {
    eth_secp256k1_gej pt;
    eth_secp256k1_scalar one;
    eth_secp256k1_gej_set_ge(&pt, key);
    eth_secp256k1_scalar_set_int(&one, 1);
    eth_secp256k1_ecmult(ctx, &pt, &pt, &one, tweak);

    if (eth_secp256k1_gej_is_infinity(&pt)) {
        return 0;
    }
    eth_secp256k1_ge_set_gej(key, &pt);
    return 1;
}

static int eth_secp256k1_eckey_privkey_tweak_mul(eth_secp256k1_scalar *key, const eth_secp256k1_scalar *tweak) {
    if (eth_secp256k1_scalar_is_zero(tweak)) {
        return 0;
    }

    eth_secp256k1_scalar_mul(key, key, tweak);
    return 1;
}

static int eth_secp256k1_eckey_pubkey_tweak_mul(const eth_secp256k1_ecmult_context *ctx, eth_secp256k1_ge *key, const eth_secp256k1_scalar *tweak) {
    eth_secp256k1_scalar zero;
    eth_secp256k1_gej pt;
    if (eth_secp256k1_scalar_is_zero(tweak)) {
        return 0;
    }

    eth_secp256k1_scalar_set_int(&zero, 0);
    eth_secp256k1_gej_set_ge(&pt, key);
    eth_secp256k1_ecmult(ctx, &pt, &pt, tweak, &zero);
    eth_secp256k1_ge_set_gej(key, &pt);
    return 1;
}

#endif
