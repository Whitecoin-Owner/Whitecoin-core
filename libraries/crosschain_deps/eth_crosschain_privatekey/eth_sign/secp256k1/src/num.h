/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_NUM_
#define _ETH_SECP256K1_NUM_

#ifndef USE_NUM_NONE

#if defined HAVE_CONFIG_H
#include "libeth_secp256k1-config.h"
#endif

#if defined(USE_NUM_GMP)
#include "num_gmp.h"
#else
#error "Please select num implementation"
#endif

/** Copy a number. */
static void eth_secp256k1_num_copy(eth_secp256k1_num *r, const eth_secp256k1_num *a);

/** Convert a number's absolute value to a binary big-endian string.
 *  There must be enough place. */
static void eth_secp256k1_num_get_bin(unsigned char *r, unsigned int rlen, const eth_secp256k1_num *a);

/** Set a number to the value of a binary big-endian string. */
static void eth_secp256k1_num_set_bin(eth_secp256k1_num *r, const unsigned char *a, unsigned int alen);

/** Compute a modular inverse. The input must be less than the modulus. */
static void eth_secp256k1_num_mod_inverse(eth_secp256k1_num *r, const eth_secp256k1_num *a, const eth_secp256k1_num *m);

/** Compute the jacobi symbol (a|b). b must be positive and odd. */
static int eth_secp256k1_num_jacobi(const eth_secp256k1_num *a, const eth_secp256k1_num *b);

/** Compare the absolute value of two numbers. */
static int eth_secp256k1_num_cmp(const eth_secp256k1_num *a, const eth_secp256k1_num *b);

/** Test whether two number are equal (including sign). */
static int eth_secp256k1_num_eq(const eth_secp256k1_num *a, const eth_secp256k1_num *b);

/** Add two (signed) numbers. */
static void eth_secp256k1_num_add(eth_secp256k1_num *r, const eth_secp256k1_num *a, const eth_secp256k1_num *b);

/** Subtract two (signed) numbers. */
static void eth_secp256k1_num_sub(eth_secp256k1_num *r, const eth_secp256k1_num *a, const eth_secp256k1_num *b);

/** Multiply two (signed) numbers. */
static void eth_secp256k1_num_mul(eth_secp256k1_num *r, const eth_secp256k1_num *a, const eth_secp256k1_num *b);

/** Replace a number by its remainder modulo m. M's sign is ignored. The result is a number between 0 and m-1,
    even if r was negative. */
static void eth_secp256k1_num_mod(eth_secp256k1_num *r, const eth_secp256k1_num *m);

/** Right-shift the passed number by bits bits. */
static void eth_secp256k1_num_shift(eth_secp256k1_num *r, int bits);

/** Check whether a number is zero. */
static int eth_secp256k1_num_is_zero(const eth_secp256k1_num *a);

/** Check whether a number is one. */
static int eth_secp256k1_num_is_one(const eth_secp256k1_num *a);

/** Check whether a number is strictly negative. */
static int eth_secp256k1_num_is_neg(const eth_secp256k1_num *a);

/** Change a number's sign. */
static void eth_secp256k1_num_negate(eth_secp256k1_num *r);

#endif

#endif
