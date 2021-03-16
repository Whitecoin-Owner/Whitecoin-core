/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_GROUP_
#define _ETH_SECP256K1_GROUP_

#include "num.h"
#include "field.h"

/** A group element of the eth_secp256k1 curve, in affine coordinates. */
typedef struct {
    eth_secp256k1_fe x;
    eth_secp256k1_fe y;
    int infinity; /* whether this represents the point at infinity */
} eth_secp256k1_ge;

#define ETH_SECP256K1_GE_CONST(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {ETH_SECP256K1_FE_CONST((a),(b),(c),(d),(e),(f),(g),(h)), ETH_SECP256K1_FE_CONST((i),(j),(k),(l),(m),(n),(o),(p)), 0}
#define ETH_SECP256K1_GE_CONST_INFINITY {ETH_SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), ETH_SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), 1}

/** A group element of the eth_secp256k1 curve, in jacobian coordinates. */
typedef struct {
    eth_secp256k1_fe x; /* actual X: x/z^2 */
    eth_secp256k1_fe y; /* actual Y: y/z^3 */
    eth_secp256k1_fe z;
    int infinity; /* whether this represents the point at infinity */
} eth_secp256k1_gej;

#define ETH_SECP256K1_GEJ_CONST(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {ETH_SECP256K1_FE_CONST((a),(b),(c),(d),(e),(f),(g),(h)), ETH_SECP256K1_FE_CONST((i),(j),(k),(l),(m),(n),(o),(p)), ETH_SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1), 0}
#define ETH_SECP256K1_GEJ_CONST_INFINITY {ETH_SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), ETH_SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), ETH_SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), 1}

typedef struct {
    eth_secp256k1_fe_storage x;
    eth_secp256k1_fe_storage y;
} eth_secp256k1_ge_storage;

#define ETH_SECP256K1_GE_STORAGE_CONST(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {ETH_SECP256K1_FE_STORAGE_CONST((a),(b),(c),(d),(e),(f),(g),(h)), ETH_SECP256K1_FE_STORAGE_CONST((i),(j),(k),(l),(m),(n),(o),(p))}

#define ETH_SECP256K1_GE_STORAGE_CONST_GET(t) ETH_SECP256K1_FE_STORAGE_CONST_GET(t.x), ETH_SECP256K1_FE_STORAGE_CONST_GET(t.y)

/** Set a group element equal to the point with given X and Y coordinates */
static void eth_secp256k1_ge_set_xy(eth_secp256k1_ge *r, const eth_secp256k1_fe *x, const eth_secp256k1_fe *y);

/** Set a group element (affine) equal to the point with the given X coordinate
 *  and a Y coordinate that is a quadratic residue modulo p. The return value
 *  is true iff a coordinate with the given X coordinate exists.
 */
static int eth_secp256k1_ge_set_xquad(eth_secp256k1_ge *r, const eth_secp256k1_fe *x);

/** Set a group element (affine) equal to the point with the given X coordinate, and given oddness
 *  for Y. Return value indicates whether the result is valid. */
static int eth_secp256k1_ge_set_xo_var(eth_secp256k1_ge *r, const eth_secp256k1_fe *x, int odd);

/** Check whether a group element is the point at infinity. */
static int eth_secp256k1_ge_is_infinity(const eth_secp256k1_ge *a);

/** Check whether a group element is valid (i.e., on the curve). */
static int eth_secp256k1_ge_is_valid_var(const eth_secp256k1_ge *a);

static void eth_secp256k1_ge_neg(eth_secp256k1_ge *r, const eth_secp256k1_ge *a);

/** Set a group element equal to another which is given in jacobian coordinates */
static void eth_secp256k1_ge_set_gej(eth_secp256k1_ge *r, eth_secp256k1_gej *a);

/** Set a batch of group elements equal to the inputs given in jacobian coordinates */
static void eth_secp256k1_ge_set_all_gej_var(eth_secp256k1_ge *r, const eth_secp256k1_gej *a, size_t len, const eth_secp256k1_callback *cb);

/** Set a batch of group elements equal to the inputs given in jacobian
 *  coordinates (with known z-ratios). zr must contain the known z-ratios such
 *  that mul(a[i].z, zr[i+1]) == a[i+1].z. zr[0] is ignored. */
static void eth_secp256k1_ge_set_table_gej_var(eth_secp256k1_ge *r, const eth_secp256k1_gej *a, const eth_secp256k1_fe *zr, size_t len);

/** Bring a batch inputs given in jacobian coordinates (with known z-ratios) to
 *  the same global z "denominator". zr must contain the known z-ratios such
 *  that mul(a[i].z, zr[i+1]) == a[i+1].z. zr[0] is ignored. The x and y
 *  coordinates of the result are stored in r, the common z coordinate is
 *  stored in globalz. */
static void eth_secp256k1_ge_globalz_set_table_gej(size_t len, eth_secp256k1_ge *r, eth_secp256k1_fe *globalz, const eth_secp256k1_gej *a, const eth_secp256k1_fe *zr);

/** Set a group element (jacobian) equal to the point at infinity. */
static void eth_secp256k1_gej_set_infinity(eth_secp256k1_gej *r);

/** Set a group element (jacobian) equal to another which is given in affine coordinates. */
static void eth_secp256k1_gej_set_ge(eth_secp256k1_gej *r, const eth_secp256k1_ge *a);

/** Compare the X coordinate of a group element (jacobian). */
static int eth_secp256k1_gej_eq_x_var(const eth_secp256k1_fe *x, const eth_secp256k1_gej *a);

/** Set r equal to the inverse of a (i.e., mirrored around the X axis) */
static void eth_secp256k1_gej_neg(eth_secp256k1_gej *r, const eth_secp256k1_gej *a);

/** Check whether a group element is the point at infinity. */
static int eth_secp256k1_gej_is_infinity(const eth_secp256k1_gej *a);

/** Check whether a group element's y coordinate is a quadratic residue. */
static int eth_secp256k1_gej_has_quad_y_var(const eth_secp256k1_gej *a);

/** Set r equal to the double of a. If rzr is not-NULL, r->z = a->z * *rzr (where infinity means an implicit z = 0).
 * a may not be zero. Constant time. */
static void eth_secp256k1_gej_double_nonzero(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, eth_secp256k1_fe *rzr);

/** Set r equal to the double of a. If rzr is not-NULL, r->z = a->z * *rzr (where infinity means an implicit z = 0). */
static void eth_secp256k1_gej_double_var(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, eth_secp256k1_fe *rzr);

/** Set r equal to the sum of a and b. If rzr is non-NULL, r->z = a->z * *rzr (a cannot be infinity in that case). */
static void eth_secp256k1_gej_add_var(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, const eth_secp256k1_gej *b, eth_secp256k1_fe *rzr);

/** Set r equal to the sum of a and b (with b given in affine coordinates, and not infinity). */
static void eth_secp256k1_gej_add_ge(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, const eth_secp256k1_ge *b);

/** Set r equal to the sum of a and b (with b given in affine coordinates). This is more efficient
    than eth_secp256k1_gej_add_var. It is identical to eth_secp256k1_gej_add_ge but without constant-time
    guarantee, and b is allowed to be infinity. If rzr is non-NULL, r->z = a->z * *rzr (a cannot be infinity in that case). */
static void eth_secp256k1_gej_add_ge_var(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, const eth_secp256k1_ge *b, eth_secp256k1_fe *rzr);

/** Set r equal to the sum of a and b (with the inverse of b's Z coordinate passed as bzinv). */
static void eth_secp256k1_gej_add_zinv_var(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, const eth_secp256k1_ge *b, const eth_secp256k1_fe *bzinv);

#ifdef USE_ENDOMORPHISM
/** Set r to be equal to lambda times a, where lambda is chosen in a way such that this is very fast. */
static void eth_secp256k1_ge_mul_lambda(eth_secp256k1_ge *r, const eth_secp256k1_ge *a);
#endif

/** Clear a eth_secp256k1_gej to prevent leaking sensitive information. */
static void eth_secp256k1_gej_clear(eth_secp256k1_gej *r);

/** Clear a eth_secp256k1_ge to prevent leaking sensitive information. */
static void eth_secp256k1_ge_clear(eth_secp256k1_ge *r);

/** Convert a group element to the storage type. */
static void eth_secp256k1_ge_to_storage(eth_secp256k1_ge_storage *r, const eth_secp256k1_ge *a);

/** Convert a group element back from the storage type. */
static void eth_secp256k1_ge_from_storage(eth_secp256k1_ge *r, const eth_secp256k1_ge_storage *a);

/** If flag is true, set *r equal to *a; otherwise leave it. Constant-time. */
static void eth_secp256k1_ge_storage_cmov(eth_secp256k1_ge_storage *r, const eth_secp256k1_ge_storage *a, int flag);

/** Rescale a jacobian point by b which must be non-zero. Constant-time. */
static void eth_secp256k1_gej_rescale(eth_secp256k1_gej *r, const eth_secp256k1_fe *b);

#endif
