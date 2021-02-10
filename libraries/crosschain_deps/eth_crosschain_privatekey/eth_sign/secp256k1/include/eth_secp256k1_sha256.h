/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_HASH_
#define _ETH_SECP256K1_HASH_

#include <stddef.h>
#include <stdint.h>

# ifdef __cplusplus
extern "C" {
# endif

typedef struct {
    uint32_t s[8];
    uint32_t buf[16]; /* In big endian */
    size_t bytes;
} eth_secp256k1_sha256_t;

void eth_secp256k1_sha256_initialize(eth_secp256k1_sha256_t *hash);
void eth_secp256k1_sha256_write(eth_secp256k1_sha256_t *hash, const unsigned char *data, size_t size);
void eth_secp256k1_sha256_finalize(eth_secp256k1_sha256_t *hash, unsigned char *out32);

typedef struct {
    eth_secp256k1_sha256_t inner, outer;
} eth_secp256k1_hmac_sha256_t;

void eth_secp256k1_hmac_sha256_initialize(eth_secp256k1_hmac_sha256_t *hash, const unsigned char *key, size_t size);
void eth_secp256k1_hmac_sha256_write(eth_secp256k1_hmac_sha256_t *hash, const unsigned char *data, size_t size);
void eth_secp256k1_hmac_sha256_finalize(eth_secp256k1_hmac_sha256_t *hash, unsigned char *out32);

typedef struct {
    unsigned char v[32];
    unsigned char k[32];
    int retry;
} eth_secp256k1_rfc6979_hmac_sha256_t;

void eth_secp256k1_rfc6979_hmac_sha256_initialize(eth_secp256k1_rfc6979_hmac_sha256_t *rng, const unsigned char *key, size_t keylen);
void eth_secp256k1_rfc6979_hmac_sha256_generate(eth_secp256k1_rfc6979_hmac_sha256_t *rng, unsigned char *out, size_t outlen);
void eth_secp256k1_rfc6979_hmac_sha256_finalize(eth_secp256k1_rfc6979_hmac_sha256_t *rng);

# ifdef __cplusplus
}
# endif

#endif
