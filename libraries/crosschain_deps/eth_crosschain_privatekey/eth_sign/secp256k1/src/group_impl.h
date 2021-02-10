/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _ETH_SECP256K1_GROUP_IMPL_H_
#define _ETH_SECP256K1_GROUP_IMPL_H_

#include "num.h"
#include "field.h"
#include "group.h"

/* These points can be generated in sage as follows:
 *
 * 0. Setup a worksheet with the following parameters.
 *   b = 4  # whatever CURVE_B will be set to
 *   F = FiniteField (0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F)
 *   C = EllipticCurve ([F (0), F (b)])
 *
 * 1. Determine all the small orders available to you. (If there are
 *    no satisfactory ones, go back and change b.)
 *   print C.order().factor(limit=1000)
 *
 * 2. Choose an order as one of the prime factors listed in the above step.
 *    (You can also multiply some to get a composite order, though the
 *    tests will crash trying to invert scalars during signing.) We take a
 *    random point and scale it to drop its order to the desired value.
 *    There is some probability this won't work; just try again.
 *   order = 199
 *   P = C.random_point()
 *   P = (int(P.order()) / int(order)) * P
 *   assert(P.order() == order)
 *
 * 3. Print the values. You'll need to use a vim macro or something to
 *    split the hex output into 4-byte chunks.
 *   print "%x %x" % P.xy()
 */
#if defined(EXHAUSTIVE_TEST_ORDER)
#  if EXHAUSTIVE_TEST_ORDER == 199
const eth_secp256k1_ge eth_secp256k1_ge_const_g = ETH_SECP256K1_GE_CONST(
    0xFA7CC9A7, 0x0737F2DB, 0xA749DD39, 0x2B4FB069,
    0x3B017A7D, 0xA808C2F1, 0xFB12940C, 0x9EA66C18,
    0x78AC123A, 0x5ED8AEF3, 0x8732BC91, 0x1F3A2868,
    0x48DF246C, 0x808DAE72, 0xCFE52572, 0x7F0501ED
);

const int ETH_CURVE_B = 4;
#  elif EXHAUSTIVE_TEST_ORDER == 13
const eth_secp256k1_ge eth_secp256k1_ge_const_g = ETH_SECP256K1_GE_CONST(
    0xedc60018, 0xa51a786b, 0x2ea91f4d, 0x4c9416c0,
    0x9de54c3b, 0xa1316554, 0x6cf4345c, 0x7277ef15,
    0x54cb1b6b, 0xdc8c1273, 0x087844ea, 0x43f4603e,
    0x0eaf9a43, 0xf6effe55, 0x939f806d, 0x37adf8ac
);
const int ETH_CURVE_B = 2;
#  else
#    error No known generator for the specified exhaustive test group order.
#  endif
#else
/** Generator for eth_secp256k1, value 'g' defined in
 *  "Standards for Efficient Cryptography" (SEC2) 2.7.1.
 */
static const eth_secp256k1_ge eth_secp256k1_ge_const_g = ETH_SECP256K1_GE_CONST(
    0x79BE667EUL, 0xF9DCBBACUL, 0x55A06295UL, 0xCE870B07UL,
    0x029BFCDBUL, 0x2DCE28D9UL, 0x59F2815BUL, 0x16F81798UL,
    0x483ADA77UL, 0x26A3C465UL, 0x5DA4FBFCUL, 0x0E1108A8UL,
    0xFD17B448UL, 0xA6855419UL, 0x9C47D08FUL, 0xFB10D4B8UL
);

const int ETH_CURVE_B = 7;
#endif

static void eth_secp256k1_ge_set_gej_zinv(eth_secp256k1_ge *r, const eth_secp256k1_gej *a, const eth_secp256k1_fe *zi) {
    eth_secp256k1_fe zi2;
    eth_secp256k1_fe zi3;
    eth_secp256k1_fe_sqr(&zi2, zi);
    eth_secp256k1_fe_mul(&zi3, &zi2, zi);
    eth_secp256k1_fe_mul(&r->x, &a->x, &zi2);
    eth_secp256k1_fe_mul(&r->y, &a->y, &zi3);
    r->infinity = a->infinity;
}

static void eth_secp256k1_ge_set_xy(eth_secp256k1_ge *r, const eth_secp256k1_fe *x, const eth_secp256k1_fe *y) {
    r->infinity = 0;
    r->x = *x;
    r->y = *y;
}

static int eth_secp256k1_ge_is_infinity(const eth_secp256k1_ge *a) {
    return a->infinity;
}

static void eth_secp256k1_ge_neg(eth_secp256k1_ge *r, const eth_secp256k1_ge *a) {
    *r = *a;
    eth_secp256k1_fe_normalize_weak(&r->y);
    eth_secp256k1_fe_negate(&r->y, &r->y, 1);
}

static void eth_secp256k1_ge_set_gej(eth_secp256k1_ge *r, eth_secp256k1_gej *a) {
    eth_secp256k1_fe z2, z3;
    r->infinity = a->infinity;
    eth_secp256k1_fe_inv(&a->z, &a->z);
    eth_secp256k1_fe_sqr(&z2, &a->z);
    eth_secp256k1_fe_mul(&z3, &a->z, &z2);
    eth_secp256k1_fe_mul(&a->x, &a->x, &z2);
    eth_secp256k1_fe_mul(&a->y, &a->y, &z3);
    eth_secp256k1_fe_set_int(&a->z, 1);
    r->x = a->x;
    r->y = a->y;
}

static void eth_secp256k1_ge_set_gej_var(eth_secp256k1_ge *r, eth_secp256k1_gej *a) {
    eth_secp256k1_fe z2, z3;
    r->infinity = a->infinity;
    if (a->infinity) {
        return;
    }
    eth_secp256k1_fe_inv_var(&a->z, &a->z);
    eth_secp256k1_fe_sqr(&z2, &a->z);
    eth_secp256k1_fe_mul(&z3, &a->z, &z2);
    eth_secp256k1_fe_mul(&a->x, &a->x, &z2);
    eth_secp256k1_fe_mul(&a->y, &a->y, &z3);
    eth_secp256k1_fe_set_int(&a->z, 1);
    r->x = a->x;
    r->y = a->y;
}

static void eth_secp256k1_ge_set_all_gej_var(eth_secp256k1_ge *r, const eth_secp256k1_gej *a, size_t len, const eth_secp256k1_callback *cb) {
    eth_secp256k1_fe *az;
    eth_secp256k1_fe *azi;
    size_t i;
    size_t count = 0;
    az = (eth_secp256k1_fe *)checked_malloc(cb, sizeof(eth_secp256k1_fe) * len);
    for (i = 0; i < len; i++) {
        if (!a[i].infinity) {
            az[count++] = a[i].z;
        }
    }

    azi = (eth_secp256k1_fe *)checked_malloc(cb, sizeof(eth_secp256k1_fe) * count);
    eth_secp256k1_fe_inv_all_var(azi, az, count);
    free(az);

    count = 0;
    for (i = 0; i < len; i++) {
        r[i].infinity = a[i].infinity;
        if (!a[i].infinity) {
            eth_secp256k1_ge_set_gej_zinv(&r[i], &a[i], &azi[count++]);
        }
    }
    free(azi);
}

static void eth_secp256k1_ge_set_table_gej_var(eth_secp256k1_ge *r, const eth_secp256k1_gej *a, const eth_secp256k1_fe *zr, size_t len) {
    size_t i = len - 1;
    eth_secp256k1_fe zi;

    if (len > 0) {
        /* Compute the inverse of the last z coordinate, and use it to compute the last affine output. */
        eth_secp256k1_fe_inv(&zi, &a[i].z);
        eth_secp256k1_ge_set_gej_zinv(&r[i], &a[i], &zi);

        /* Work out way backwards, using the z-ratios to scale the x/y values. */
        while (i > 0) {
            eth_secp256k1_fe_mul(&zi, &zi, &zr[i]);
            i--;
            eth_secp256k1_ge_set_gej_zinv(&r[i], &a[i], &zi);
        }
    }
}

static void eth_secp256k1_ge_globalz_set_table_gej(size_t len, eth_secp256k1_ge *r, eth_secp256k1_fe *globalz, const eth_secp256k1_gej *a, const eth_secp256k1_fe *zr) {
    size_t i = len - 1;
    eth_secp256k1_fe zs;

    if (len > 0) {
        /* The z of the final point gives us the "global Z" for the table. */
        r[i].x = a[i].x;
        r[i].y = a[i].y;
        *globalz = a[i].z;
        r[i].infinity = 0;
        zs = zr[i];

        /* Work our way backwards, using the z-ratios to scale the x/y values. */
        while (i > 0) {
            if (i != len - 1) {
                eth_secp256k1_fe_mul(&zs, &zs, &zr[i]);
            }
            i--;
            eth_secp256k1_ge_set_gej_zinv(&r[i], &a[i], &zs);
        }
    }
}

static void eth_secp256k1_gej_set_infinity(eth_secp256k1_gej *r) {
    r->infinity = 1;
    eth_secp256k1_fe_clear(&r->x);
    eth_secp256k1_fe_clear(&r->y);
    eth_secp256k1_fe_clear(&r->z);
}

static void eth_secp256k1_gej_clear(eth_secp256k1_gej *r) {
    r->infinity = 0;
    eth_secp256k1_fe_clear(&r->x);
    eth_secp256k1_fe_clear(&r->y);
    eth_secp256k1_fe_clear(&r->z);
}

static void eth_secp256k1_ge_clear(eth_secp256k1_ge *r) {
    r->infinity = 0;
    eth_secp256k1_fe_clear(&r->x);
    eth_secp256k1_fe_clear(&r->y);
}

static int eth_secp256k1_ge_set_xquad(eth_secp256k1_ge *r, const eth_secp256k1_fe *x) {
    eth_secp256k1_fe x2, x3, c;
    r->x = *x;
    eth_secp256k1_fe_sqr(&x2, x);
    eth_secp256k1_fe_mul(&x3, x, &x2);
    r->infinity = 0;
    eth_secp256k1_fe_set_int(&c, ETH_CURVE_B);
    eth_secp256k1_fe_add(&c, &x3);
    return eth_secp256k1_fe_sqrt(&r->y, &c);
}

static int eth_secp256k1_ge_set_xo_var(eth_secp256k1_ge *r, const eth_secp256k1_fe *x, int odd) {
    if (!eth_secp256k1_ge_set_xquad(r, x)) {
        return 0;
    }
    eth_secp256k1_fe_normalize_var(&r->y);
    if (eth_secp256k1_fe_is_odd(&r->y) != odd) {
        eth_secp256k1_fe_negate(&r->y, &r->y, 1);
    }
    return 1;

}

static void eth_secp256k1_gej_set_ge(eth_secp256k1_gej *r, const eth_secp256k1_ge *a) {
   r->infinity = a->infinity;
   r->x = a->x;
   r->y = a->y;
   eth_secp256k1_fe_set_int(&r->z, 1);
}

static int eth_secp256k1_gej_eq_x_var(const eth_secp256k1_fe *x, const eth_secp256k1_gej *a) {
    eth_secp256k1_fe r, r2;
    VERIFY_CHECK(!a->infinity);
    eth_secp256k1_fe_sqr(&r, &a->z); eth_secp256k1_fe_mul(&r, &r, x);
    r2 = a->x; eth_secp256k1_fe_normalize_weak(&r2);
    return eth_secp256k1_fe_equal_var(&r, &r2);
}

static void eth_secp256k1_gej_neg(eth_secp256k1_gej *r, const eth_secp256k1_gej *a) {
    r->infinity = a->infinity;
    r->x = a->x;
    r->y = a->y;
    r->z = a->z;
    eth_secp256k1_fe_normalize_weak(&r->y);
    eth_secp256k1_fe_negate(&r->y, &r->y, 1);
}

static int eth_secp256k1_gej_is_infinity(const eth_secp256k1_gej *a) {
    return a->infinity;
}

static int eth_secp256k1_gej_is_valid_var(const eth_secp256k1_gej *a) {
    eth_secp256k1_fe y2, x3, z2, z6;
    if (a->infinity) {
        return 0;
    }
    /** y^2 = x^3 + 7
     *  (Y/Z^3)^2 = (X/Z^2)^3 + 7
     *  Y^2 / Z^6 = X^3 / Z^6 + 7
     *  Y^2 = X^3 + 7*Z^6
     */
    eth_secp256k1_fe_sqr(&y2, &a->y);
    eth_secp256k1_fe_sqr(&x3, &a->x); eth_secp256k1_fe_mul(&x3, &x3, &a->x);
    eth_secp256k1_fe_sqr(&z2, &a->z);
    eth_secp256k1_fe_sqr(&z6, &z2); eth_secp256k1_fe_mul(&z6, &z6, &z2);
    eth_secp256k1_fe_mul_int(&z6, ETH_CURVE_B);
    eth_secp256k1_fe_add(&x3, &z6);
    eth_secp256k1_fe_normalize_weak(&x3);
    return eth_secp256k1_fe_equal_var(&y2, &x3);
}

static int eth_secp256k1_ge_is_valid_var(const eth_secp256k1_ge *a) {
    eth_secp256k1_fe y2, x3, c;
    if (a->infinity) {
        return 0;
    }
    /* y^2 = x^3 + 7 */
    eth_secp256k1_fe_sqr(&y2, &a->y);
    eth_secp256k1_fe_sqr(&x3, &a->x); eth_secp256k1_fe_mul(&x3, &x3, &a->x);
    eth_secp256k1_fe_set_int(&c, ETH_CURVE_B);
    eth_secp256k1_fe_add(&x3, &c);
    eth_secp256k1_fe_normalize_weak(&x3);
    return eth_secp256k1_fe_equal_var(&y2, &x3);
}

static void eth_secp256k1_gej_double_var(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, eth_secp256k1_fe *rzr) {
    /* Operations: 3 mul, 4 sqr, 0 normalize, 12 mul_int/add/negate.
     *
     * Note that there is an implementation described at
     *     https://hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-0.html#doubling-dbl-2009-l
     * which trades a multiply for a square, but in practice this is actually slower,
     * mainly because it requires more normalizations.
     */
    eth_secp256k1_fe t1,t2,t3,t4;
    /** For eth_secp256k1, 2Q is infinity if and only if Q is infinity. This is because if 2Q = infinity,
     *  Q must equal -Q, or that Q.y == -(Q.y), or Q.y is 0. For a point on y^2 = x^3 + 7 to have
     *  y=0, x^3 must be -7 mod p. However, -7 has no cube root mod p.
     *
     *  Having said this, if this function receives a point on a sextic twist, e.g. by
     *  a fault attack, it is possible for y to be 0. This happens for y^2 = x^3 + 6,
     *  since -6 does have a cube root mod p. For this point, this function will not set
     *  the infinity flag even though the point doubles to infinity, and the result
     *  point will be gibberish (z = 0 but infinity = 0).
     */
    r->infinity = a->infinity;
    if (r->infinity) {
        if (rzr != NULL) {
            eth_secp256k1_fe_set_int(rzr, 1);
        }
        return;
    }

    if (rzr != NULL) {
        *rzr = a->y;
        eth_secp256k1_fe_normalize_weak(rzr);
        eth_secp256k1_fe_mul_int(rzr, 2);
    }

    eth_secp256k1_fe_mul(&r->z, &a->z, &a->y);
    eth_secp256k1_fe_mul_int(&r->z, 2);       /* Z' = 2*Y*Z (2) */
    eth_secp256k1_fe_sqr(&t1, &a->x);
    eth_secp256k1_fe_mul_int(&t1, 3);         /* T1 = 3*X^2 (3) */
    eth_secp256k1_fe_sqr(&t2, &t1);           /* T2 = 9*X^4 (1) */
    eth_secp256k1_fe_sqr(&t3, &a->y);
    eth_secp256k1_fe_mul_int(&t3, 2);         /* T3 = 2*Y^2 (2) */
    eth_secp256k1_fe_sqr(&t4, &t3);
    eth_secp256k1_fe_mul_int(&t4, 2);         /* T4 = 8*Y^4 (2) */
    eth_secp256k1_fe_mul(&t3, &t3, &a->x);    /* T3 = 2*X*Y^2 (1) */
    r->x = t3;
    eth_secp256k1_fe_mul_int(&r->x, 4);       /* X' = 8*X*Y^2 (4) */
    eth_secp256k1_fe_negate(&r->x, &r->x, 4); /* X' = -8*X*Y^2 (5) */
    eth_secp256k1_fe_add(&r->x, &t2);         /* X' = 9*X^4 - 8*X*Y^2 (6) */
    eth_secp256k1_fe_negate(&t2, &t2, 1);     /* T2 = -9*X^4 (2) */
    eth_secp256k1_fe_mul_int(&t3, 6);         /* T3 = 12*X*Y^2 (6) */
    eth_secp256k1_fe_add(&t3, &t2);           /* T3 = 12*X*Y^2 - 9*X^4 (8) */
    eth_secp256k1_fe_mul(&r->y, &t1, &t3);    /* Y' = 36*X^3*Y^2 - 27*X^6 (1) */
    eth_secp256k1_fe_negate(&t2, &t4, 2);     /* T2 = -8*Y^4 (3) */
    eth_secp256k1_fe_add(&r->y, &t2);         /* Y' = 36*X^3*Y^2 - 27*X^6 - 8*Y^4 (4) */
}

static ETH_SECP256K1_INLINE void eth_secp256k1_gej_double_nonzero(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, eth_secp256k1_fe *rzr) {
    VERIFY_CHECK(!eth_secp256k1_gej_is_infinity(a));
    eth_secp256k1_gej_double_var(r, a, rzr);
}

static void eth_secp256k1_gej_add_var(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, const eth_secp256k1_gej *b, eth_secp256k1_fe *rzr) {
    /* Operations: 12 mul, 4 sqr, 2 normalize, 12 mul_int/add/negate */
    eth_secp256k1_fe z22, z12, u1, u2, s1, s2, h, i, i2, h2, h3, t;

    if (a->infinity) {
        VERIFY_CHECK(rzr == NULL);
        *r = *b;
        return;
    }

    if (b->infinity) {
        if (rzr != NULL) {
            eth_secp256k1_fe_set_int(rzr, 1);
        }
        *r = *a;
        return;
    }

    r->infinity = 0;
    eth_secp256k1_fe_sqr(&z22, &b->z);
    eth_secp256k1_fe_sqr(&z12, &a->z);
    eth_secp256k1_fe_mul(&u1, &a->x, &z22);
    eth_secp256k1_fe_mul(&u2, &b->x, &z12);
    eth_secp256k1_fe_mul(&s1, &a->y, &z22); eth_secp256k1_fe_mul(&s1, &s1, &b->z);
    eth_secp256k1_fe_mul(&s2, &b->y, &z12); eth_secp256k1_fe_mul(&s2, &s2, &a->z);
    eth_secp256k1_fe_negate(&h, &u1, 1); eth_secp256k1_fe_add(&h, &u2);
    eth_secp256k1_fe_negate(&i, &s1, 1); eth_secp256k1_fe_add(&i, &s2);
    if (eth_secp256k1_fe_normalizes_to_zero_var(&h)) {
        if (eth_secp256k1_fe_normalizes_to_zero_var(&i)) {
            eth_secp256k1_gej_double_var(r, a, rzr);
        } else {
            if (rzr != NULL) {
                eth_secp256k1_fe_set_int(rzr, 0);
            }
            r->infinity = 1;
        }
        return;
    }
    eth_secp256k1_fe_sqr(&i2, &i);
    eth_secp256k1_fe_sqr(&h2, &h);
    eth_secp256k1_fe_mul(&h3, &h, &h2);
    eth_secp256k1_fe_mul(&h, &h, &b->z);
    if (rzr != NULL) {
        *rzr = h;
    }
    eth_secp256k1_fe_mul(&r->z, &a->z, &h);
    eth_secp256k1_fe_mul(&t, &u1, &h2);
    r->x = t; eth_secp256k1_fe_mul_int(&r->x, 2); eth_secp256k1_fe_add(&r->x, &h3); eth_secp256k1_fe_negate(&r->x, &r->x, 3); eth_secp256k1_fe_add(&r->x, &i2);
    eth_secp256k1_fe_negate(&r->y, &r->x, 5); eth_secp256k1_fe_add(&r->y, &t); eth_secp256k1_fe_mul(&r->y, &r->y, &i);
    eth_secp256k1_fe_mul(&h3, &h3, &s1); eth_secp256k1_fe_negate(&h3, &h3, 1);
    eth_secp256k1_fe_add(&r->y, &h3);
}

static void eth_secp256k1_gej_add_ge_var(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, const eth_secp256k1_ge *b, eth_secp256k1_fe *rzr) {
    /* 8 mul, 3 sqr, 4 normalize, 12 mul_int/add/negate */
    eth_secp256k1_fe z12, u1, u2, s1, s2, h, i, i2, h2, h3, t;
    if (a->infinity) {
        VERIFY_CHECK(rzr == NULL);
        eth_secp256k1_gej_set_ge(r, b);
        return;
    }
    if (b->infinity) {
        if (rzr != NULL) {
            eth_secp256k1_fe_set_int(rzr, 1);
        }
        *r = *a;
        return;
    }
    r->infinity = 0;

    eth_secp256k1_fe_sqr(&z12, &a->z);
    u1 = a->x; eth_secp256k1_fe_normalize_weak(&u1);
    eth_secp256k1_fe_mul(&u2, &b->x, &z12);
    s1 = a->y; eth_secp256k1_fe_normalize_weak(&s1);
    eth_secp256k1_fe_mul(&s2, &b->y, &z12); eth_secp256k1_fe_mul(&s2, &s2, &a->z);
    eth_secp256k1_fe_negate(&h, &u1, 1); eth_secp256k1_fe_add(&h, &u2);
    eth_secp256k1_fe_negate(&i, &s1, 1); eth_secp256k1_fe_add(&i, &s2);
    if (eth_secp256k1_fe_normalizes_to_zero_var(&h)) {
        if (eth_secp256k1_fe_normalizes_to_zero_var(&i)) {
            eth_secp256k1_gej_double_var(r, a, rzr);
        } else {
            if (rzr != NULL) {
                eth_secp256k1_fe_set_int(rzr, 0);
            }
            r->infinity = 1;
        }
        return;
    }
    eth_secp256k1_fe_sqr(&i2, &i);
    eth_secp256k1_fe_sqr(&h2, &h);
    eth_secp256k1_fe_mul(&h3, &h, &h2);
    if (rzr != NULL) {
        *rzr = h;
    }
    eth_secp256k1_fe_mul(&r->z, &a->z, &h);
    eth_secp256k1_fe_mul(&t, &u1, &h2);
    r->x = t; eth_secp256k1_fe_mul_int(&r->x, 2); eth_secp256k1_fe_add(&r->x, &h3); eth_secp256k1_fe_negate(&r->x, &r->x, 3); eth_secp256k1_fe_add(&r->x, &i2);
    eth_secp256k1_fe_negate(&r->y, &r->x, 5); eth_secp256k1_fe_add(&r->y, &t); eth_secp256k1_fe_mul(&r->y, &r->y, &i);
    eth_secp256k1_fe_mul(&h3, &h3, &s1); eth_secp256k1_fe_negate(&h3, &h3, 1);
    eth_secp256k1_fe_add(&r->y, &h3);
}

static void eth_secp256k1_gej_add_zinv_var(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, const eth_secp256k1_ge *b, const eth_secp256k1_fe *bzinv) {
    /* 9 mul, 3 sqr, 4 normalize, 12 mul_int/add/negate */
    eth_secp256k1_fe az, z12, u1, u2, s1, s2, h, i, i2, h2, h3, t;

    if (b->infinity) {
        *r = *a;
        return;
    }
    if (a->infinity) {
        eth_secp256k1_fe bzinv2, bzinv3;
        r->infinity = b->infinity;
        eth_secp256k1_fe_sqr(&bzinv2, bzinv);
        eth_secp256k1_fe_mul(&bzinv3, &bzinv2, bzinv);
        eth_secp256k1_fe_mul(&r->x, &b->x, &bzinv2);
        eth_secp256k1_fe_mul(&r->y, &b->y, &bzinv3);
        eth_secp256k1_fe_set_int(&r->z, 1);
        return;
    }
    r->infinity = 0;

    /** We need to calculate (rx,ry,rz) = (ax,ay,az) + (bx,by,1/bzinv). Due to
     *  eth_secp256k1's isomorphism we can multiply the Z coordinates on both sides
     *  by bzinv, and get: (rx,ry,rz*bzinv) = (ax,ay,az*bzinv) + (bx,by,1).
     *  This means that (rx,ry,rz) can be calculated as
     *  (ax,ay,az*bzinv) + (bx,by,1), when not applying the bzinv factor to rz.
     *  The variable az below holds the modified Z coordinate for a, which is used
     *  for the computation of rx and ry, but not for rz.
     */
    eth_secp256k1_fe_mul(&az, &a->z, bzinv);

    eth_secp256k1_fe_sqr(&z12, &az);
    u1 = a->x; eth_secp256k1_fe_normalize_weak(&u1);
    eth_secp256k1_fe_mul(&u2, &b->x, &z12);
    s1 = a->y; eth_secp256k1_fe_normalize_weak(&s1);
    eth_secp256k1_fe_mul(&s2, &b->y, &z12); eth_secp256k1_fe_mul(&s2, &s2, &az);
    eth_secp256k1_fe_negate(&h, &u1, 1); eth_secp256k1_fe_add(&h, &u2);
    eth_secp256k1_fe_negate(&i, &s1, 1); eth_secp256k1_fe_add(&i, &s2);
    if (eth_secp256k1_fe_normalizes_to_zero_var(&h)) {
        if (eth_secp256k1_fe_normalizes_to_zero_var(&i)) {
            eth_secp256k1_gej_double_var(r, a, NULL);
        } else {
            r->infinity = 1;
        }
        return;
    }
    eth_secp256k1_fe_sqr(&i2, &i);
    eth_secp256k1_fe_sqr(&h2, &h);
    eth_secp256k1_fe_mul(&h3, &h, &h2);
    r->z = a->z; eth_secp256k1_fe_mul(&r->z, &r->z, &h);
    eth_secp256k1_fe_mul(&t, &u1, &h2);
    r->x = t; eth_secp256k1_fe_mul_int(&r->x, 2); eth_secp256k1_fe_add(&r->x, &h3); eth_secp256k1_fe_negate(&r->x, &r->x, 3); eth_secp256k1_fe_add(&r->x, &i2);
    eth_secp256k1_fe_negate(&r->y, &r->x, 5); eth_secp256k1_fe_add(&r->y, &t); eth_secp256k1_fe_mul(&r->y, &r->y, &i);
    eth_secp256k1_fe_mul(&h3, &h3, &s1); eth_secp256k1_fe_negate(&h3, &h3, 1);
    eth_secp256k1_fe_add(&r->y, &h3);
}


static void eth_secp256k1_gej_add_ge(eth_secp256k1_gej *r, const eth_secp256k1_gej *a, const eth_secp256k1_ge *b) {
    /* Operations: 7 mul, 5 sqr, 4 normalize, 21 mul_int/add/negate/cmov */
    static const eth_secp256k1_fe fe_1 = ETH_SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1);
    eth_secp256k1_fe zz, u1, u2, s1, s2, t, tt, m, n, q, rr;
    eth_secp256k1_fe m_alt, rr_alt;
    int infinity, degenerate;
    VERIFY_CHECK(!b->infinity);
    VERIFY_CHECK(a->infinity == 0 || a->infinity == 1);

    /** In:
     *    Eric Brier and Marc Joye, Weierstrass Elliptic Curves and Side-Channel Attacks.
     *    In D. Naccache and P. Paillier, Eds., Public Key Cryptography, vol. 2274 of Lecture Notes in Computer Science, pages 335-345. Springer-Verlag, 2002.
     *  we find as solution for a unified addition/doubling formula:
     *    lambda = ((x1 + x2)^2 - x1 * x2 + a) / (y1 + y2), with a = 0 for eth_secp256k1's curve equation.
     *    x3 = lambda^2 - (x1 + x2)
     *    2*y3 = lambda * (x1 + x2 - 2 * x3) - (y1 + y2).
     *
     *  Substituting x_i = Xi / Zi^2 and yi = Yi / Zi^3, for i=1,2,3, gives:
     *    U1 = X1*Z2^2, U2 = X2*Z1^2
     *    S1 = Y1*Z2^3, S2 = Y2*Z1^3
     *    Z = Z1*Z2
     *    T = U1+U2
     *    M = S1+S2
     *    Q = T*M^2
     *    R = T^2-U1*U2
     *    X3 = 4*(R^2-Q)
     *    Y3 = 4*(R*(3*Q-2*R^2)-M^4)
     *    Z3 = 2*M*Z
     *  (Note that the paper uses xi = Xi / Zi and yi = Yi / Zi instead.)
     *
     *  This formula has the benefit of being the same for both addition
     *  of distinct points and doubling. However, it breaks down in the
     *  case that either point is infinity, or that y1 = -y2. We handle
     *  these cases in the following ways:
     *
     *    - If b is infinity we simply bail by means of a VERIFY_CHECK.
     *
     *    - If a is infinity, we detect this, and at the end of the
     *      computation replace the result (which will be meaningless,
     *      but we compute to be constant-time) with b.x : b.y : 1.
     *
     *    - If a = -b, we have y1 = -y2, which is a degenerate case.
     *      But here the answer is infinity, so we simply set the
     *      infinity flag of the result, overriding the computed values
     *      without even needing to cmov.
     *
     *    - If y1 = -y2 but x1 != x2, which does occur thanks to certain
     *      properties of our curve (specifically, 1 has nontrivial cube
     *      roots in our field, and the curve equation has no x coefficient)
     *      then the answer is not infinity but also not given by the above
     *      equation. In this case, we cmov in place an alternate expression
     *      for lambda. Specifically (y1 - y2)/(x1 - x2). Where both these
     *      expressions for lambda are defined, they are equal, and can be
     *      obtained from each other by multiplication by (y1 + y2)/(y1 + y2)
     *      then substitution of x^3 + 7 for y^2 (using the curve equation).
     *      For all pairs of nonzero points (a, b) at least one is defined,
     *      so this covers everything.
     */

    eth_secp256k1_fe_sqr(&zz, &a->z);                       /* z = Z1^2 */
    u1 = a->x; eth_secp256k1_fe_normalize_weak(&u1);        /* u1 = U1 = X1*Z2^2 (1) */
    eth_secp256k1_fe_mul(&u2, &b->x, &zz);                  /* u2 = U2 = X2*Z1^2 (1) */
    s1 = a->y; eth_secp256k1_fe_normalize_weak(&s1);        /* s1 = S1 = Y1*Z2^3 (1) */
    eth_secp256k1_fe_mul(&s2, &b->y, &zz);                  /* s2 = Y2*Z1^2 (1) */
    eth_secp256k1_fe_mul(&s2, &s2, &a->z);                  /* s2 = S2 = Y2*Z1^3 (1) */
    t = u1; eth_secp256k1_fe_add(&t, &u2);                  /* t = T = U1+U2 (2) */
    m = s1; eth_secp256k1_fe_add(&m, &s2);                  /* m = M = S1+S2 (2) */
    eth_secp256k1_fe_sqr(&rr, &t);                          /* rr = T^2 (1) */
    eth_secp256k1_fe_negate(&m_alt, &u2, 1);                /* Malt = -X2*Z1^2 */
    eth_secp256k1_fe_mul(&tt, &u1, &m_alt);                 /* tt = -U1*U2 (2) */
    eth_secp256k1_fe_add(&rr, &tt);                         /* rr = R = T^2-U1*U2 (3) */
    /** If lambda = R/M = 0/0 we have a problem (except in the "trivial"
     *  case that Z = z1z2 = 0, and this is special-cased later on). */
    degenerate = eth_secp256k1_fe_normalizes_to_zero(&m) &
                 eth_secp256k1_fe_normalizes_to_zero(&rr);
    /* This only occurs when y1 == -y2 and x1^3 == x2^3, but x1 != x2.
     * This means either x1 == beta*x2 or beta*x1 == x2, where beta is
     * a nontrivial cube root of one. In either case, an alternate
     * non-indeterminate expression for lambda is (y1 - y2)/(x1 - x2),
     * so we set R/M equal to this. */
    rr_alt = s1;
    eth_secp256k1_fe_mul_int(&rr_alt, 2);       /* rr = Y1*Z2^3 - Y2*Z1^3 (2) */
    eth_secp256k1_fe_add(&m_alt, &u1);          /* Malt = X1*Z2^2 - X2*Z1^2 */

    eth_secp256k1_fe_cmov(&rr_alt, &rr, !degenerate);
    eth_secp256k1_fe_cmov(&m_alt, &m, !degenerate);
    /* Now Ralt / Malt = lambda and is guaranteed not to be 0/0.
     * From here on out Ralt and Malt represent the numerator
     * and denominator of lambda; R and M represent the explicit
     * expressions x1^2 + x2^2 + x1x2 and y1 + y2. */
    eth_secp256k1_fe_sqr(&n, &m_alt);                       /* n = Malt^2 (1) */
    eth_secp256k1_fe_mul(&q, &n, &t);                       /* q = Q = T*Malt^2 (1) */
    /* These two lines use the observation that either M == Malt or M == 0,
     * so M^3 * Malt is either Malt^4 (which is computed by squaring), or
     * zero (which is "computed" by cmov). So the cost is one squaring
     * versus two multiplications. */
    eth_secp256k1_fe_sqr(&n, &n);
    eth_secp256k1_fe_cmov(&n, &m, degenerate);              /* n = M^3 * Malt (2) */
    eth_secp256k1_fe_sqr(&t, &rr_alt);                      /* t = Ralt^2 (1) */
    eth_secp256k1_fe_mul(&r->z, &a->z, &m_alt);             /* r->z = Malt*Z (1) */
    infinity = eth_secp256k1_fe_normalizes_to_zero(&r->z) * (1 - a->infinity);
    eth_secp256k1_fe_mul_int(&r->z, 2);                     /* r->z = Z3 = 2*Malt*Z (2) */
    eth_secp256k1_fe_negate(&q, &q, 1);                     /* q = -Q (2) */
    eth_secp256k1_fe_add(&t, &q);                           /* t = Ralt^2-Q (3) */
    eth_secp256k1_fe_normalize_weak(&t);
    r->x = t;                                           /* r->x = Ralt^2-Q (1) */
    eth_secp256k1_fe_mul_int(&t, 2);                        /* t = 2*x3 (2) */
    eth_secp256k1_fe_add(&t, &q);                           /* t = 2*x3 - Q: (4) */
    eth_secp256k1_fe_mul(&t, &t, &rr_alt);                  /* t = Ralt*(2*x3 - Q) (1) */
    eth_secp256k1_fe_add(&t, &n);                           /* t = Ralt*(2*x3 - Q) + M^3*Malt (3) */
    eth_secp256k1_fe_negate(&r->y, &t, 3);                  /* r->y = Ralt*(Q - 2x3) - M^3*Malt (4) */
    eth_secp256k1_fe_normalize_weak(&r->y);
    eth_secp256k1_fe_mul_int(&r->x, 4);                     /* r->x = X3 = 4*(Ralt^2-Q) */
    eth_secp256k1_fe_mul_int(&r->y, 4);                     /* r->y = Y3 = 4*Ralt*(Q - 2x3) - 4*M^3*Malt (4) */

    /** In case a->infinity == 1, replace r with (b->x, b->y, 1). */
    eth_secp256k1_fe_cmov(&r->x, &b->x, a->infinity);
    eth_secp256k1_fe_cmov(&r->y, &b->y, a->infinity);
    eth_secp256k1_fe_cmov(&r->z, &fe_1, a->infinity);
    r->infinity = infinity;
}

static void eth_secp256k1_gej_rescale(eth_secp256k1_gej *r, const eth_secp256k1_fe *s) {
    /* Operations: 4 mul, 1 sqr */
    eth_secp256k1_fe zz;
    VERIFY_CHECK(!eth_secp256k1_fe_is_zero(s));
    eth_secp256k1_fe_sqr(&zz, s);
    eth_secp256k1_fe_mul(&r->x, &r->x, &zz);                /* r->x *= s^2 */
    eth_secp256k1_fe_mul(&r->y, &r->y, &zz);
    eth_secp256k1_fe_mul(&r->y, &r->y, s);                  /* r->y *= s^3 */
    eth_secp256k1_fe_mul(&r->z, &r->z, s);                  /* r->z *= s   */
}

static void eth_secp256k1_ge_to_storage(eth_secp256k1_ge_storage *r, const eth_secp256k1_ge *a) {
    eth_secp256k1_fe x, y;
    VERIFY_CHECK(!a->infinity);
    x = a->x;
    eth_secp256k1_fe_normalize(&x);
    y = a->y;
    eth_secp256k1_fe_normalize(&y);
    eth_secp256k1_fe_to_storage(&r->x, &x);
    eth_secp256k1_fe_to_storage(&r->y, &y);
}

static void eth_secp256k1_ge_from_storage(eth_secp256k1_ge *r, const eth_secp256k1_ge_storage *a) {
    eth_secp256k1_fe_from_storage(&r->x, &a->x);
    eth_secp256k1_fe_from_storage(&r->y, &a->y);
    r->infinity = 0;
}

static ETH_SECP256K1_INLINE void eth_secp256k1_ge_storage_cmov(eth_secp256k1_ge_storage *r, const eth_secp256k1_ge_storage *a, int flag) {
    eth_secp256k1_fe_storage_cmov(&r->x, &a->x, flag);
    eth_secp256k1_fe_storage_cmov(&r->y, &a->y, flag);
}

#ifdef USE_ENDOMORPHISM
static void eth_secp256k1_ge_mul_lambda(eth_secp256k1_ge *r, const eth_secp256k1_ge *a) {
    static const eth_secp256k1_fe beta = ETH_SECP256K1_FE_CONST(
        0x7ae96a2bul, 0x657c0710ul, 0x6e64479eul, 0xac3434e9ul,
        0x9cf04975ul, 0x12f58995ul, 0xc1396c28ul, 0x719501eeul
    );
    *r = *a;
    eth_secp256k1_fe_mul(&r->x, &r->x, &beta);
}
#endif

static int eth_secp256k1_gej_has_quad_y_var(const eth_secp256k1_gej *a) {
    eth_secp256k1_fe yz;

    if (a->infinity) {
        return 0;
    }

    /* We rely on the fact that the Jacobi symbol of 1 / a->z^3 is the same as
     * that of a->z. Thus a->y / a->z^3 is a quadratic residue iff a->y * a->z
       is */
    eth_secp256k1_fe_mul(&yz, &a->y, &a->z);
    return eth_secp256k1_fe_is_quad_var(&yz);
}

#endif
