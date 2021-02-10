#include <stdlib.h>
#include <stdint.h>
#include "org_bitcoin_Secp256k1Context.h"
#include "include/eth_secp256k1.h"

ETH_SECP256K1_API jlong JNICALL Java_org_bitcoin_Secp256k1Context_eth_secp256k1_1init_1context
  (JNIEnv* env, jclass classObject)
{
  eth_secp256k1_context *ctx = eth_secp256k1_context_create(ETH_SECP256K1_CONTEXT_SIGN | ETH_SECP256K1_CONTEXT_VERIFY);

  (void)classObject;(void)env;

  return (uintptr_t)ctx;
}

