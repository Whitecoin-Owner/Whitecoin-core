#ifndef ETH_SECP256K1_ECDH_H
#define ETH_SECP256K1_ECDH_H

#include "eth_secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Compute an EC Diffie-Hellman secret in constant time
 *  Returns: 1: exponentiation was successful
 *           0: scalar was invalid (zero or overflow)
 *  Args:    ctx:        pointer to a context object (cannot be NULL)
 *  Out:     result:     a 32-byte array which will be populated by an ECDH
 *                       secret computed from the point and scalar
 *  In:      pubkey:     a pointer to a eth_secp256k1_pubkey containing an
 *                       initialized public key
 *           privkey:    a 32-byte scalar with which to multiply the point
 */
ETH_SECP256K1_API ETH_SECP256K1_WARN_UNUSED_RESULT int eth_secp256k1_ecdh(
  const eth_secp256k1_context* ctx,
  unsigned char *result,
  const eth_secp256k1_pubkey *pubkey,
  const unsigned char *privkey
) ETH_SECP256K1_ARG_NONNULL(1) ETH_SECP256K1_ARG_NONNULL(2) ETH_SECP256K1_ARG_NONNULL(3) ETH_SECP256K1_ARG_NONNULL(4);

#ifdef __cplusplus
}
#endif

#endif /* ETH_SECP256K1_ECDH_H */
