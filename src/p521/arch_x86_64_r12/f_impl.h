/* Copyright (c) 2014 Cryptography Research, Inc.
 * Released under the MIT License.  See LICENSE.txt for license information.
 */
#ifndef __P521_H__
#define __P521_H__ 1

#include "f_field.h"

#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "constant_time.h"

/* FIXME: Currenmtlty desn't work at all, because the struct is declared [9] and not [12] */
#define LIMBPERM(x) (((x)%3)*4 + (x)/3)
#define USE_P521_3x3_TRANSPOSE

#ifdef __cplusplus
extern "C" {
#endif

/* -------------- Inline functions begin here -------------- */

typedef uint64x4_t uint64x3_t; /* fit it in a vector register */

static const uint64x3_t mask58 = { (1ull<<58) - 1, (1ull<<58) - 1, (1ull<<58) - 1, 0 };

/* Currently requires CLANG.  Sorry. */
static inline uint64x3_t timesW (uint64x3_t u) {
    return u.zxyw + u.zwww;
}

void gf_add_RAW (gf  *out, const gf  *a, const gf  *b) {
    unsigned int i;
    for (i=0; i<sizeof(*out)/sizeof(uint64xn_t); i++) {
        ((uint64xn_t*)out)[i] = ((const uint64xn_t*)a)[i] + ((const uint64xn_t*)b)[i];
    }
}

void gf_sub_RAW (gf  *out, const gf  *a, const gf  *b) {
    unsigned int i;
    for (i=0; i<sizeof(*out)/sizeof(uint64xn_t); i++) {
        ((uint64xn_t*)out)[i] = ((const uint64xn_t*)a)[i] - ((const uint64xn_t*)b)[i];
    }
}

void gf_bias (gf  *a, int amt) {
    uint64_t co0 = ((1ull<<58)-2)*amt, co1 = ((1ull<<58)-1)*amt;
    uint64x4_t vlo = { co0, co1, co1, 0 }, vhi = { co1, co1, co1, 0 };
    ((uint64x4_t*)a)[0] += vlo;
    ((uint64x4_t*)a)[1] += vhi;
    ((uint64x4_t*)a)[2] += vhi;
}

void gf_weak_reduce (gf  *a) {
#if 0
    int i;
    assert(a->limb[3] == 0 && a->limb[7] == 0 && a->limb[11] == 0);
    for (i=0; i<12; i++) {
        assert(a->limb[i] < 3ull<<61);
    }
#endif
    uint64x3_t
        ot0 = ((uint64x4_t*)a)[0],
        ot1 = ((uint64x4_t*)a)[1],
        ot2 = ((uint64x4_t*)a)[2];
    
    uint64x3_t out0 = (ot0 & mask58) + timesW(ot2>>58);
    uint64x3_t out1 = (ot1 & mask58) + (ot0>>58);
    uint64x3_t out2 = (ot2 & mask58) + (ot1>>58);

    ((uint64x4_t*)a)[0] = out0;
    ((uint64x4_t*)a)[1] = out1;
    ((uint64x4_t*)a)[2] = out2;
}

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* __P521_H__ */
