/* Copyright (c) 2014 Cryptography Research, Inc.
 * Released under the MIT License.  See LICENSE.txt for license information.
 */
#ifndef __gf_H__
#define __gf_H__ 1

#include "f_field.h"

#include <stdint.h>
#include <assert.h>

#include "word.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------- Inline functions begin here -------------- */

void gf_add_RAW (gf  *out, const gf  *a, const gf  *b) {
    unsigned int i;
    for (i=0; i<sizeof(*out)/sizeof(uint64xn_t); i++) {
        ((uint64xn_t*)out)[i] = ((const uint64xn_t*)a)[i] + ((const uint64xn_t*)b)[i];
    }
    /*
    unsigned int i;
    for (i=0; i<sizeof(*out)/sizeof(out->limb[0]); i++) {
        out->limb[i] = a->limb[i] + b->limb[i];
    }
    */
}

void gf_sub_RAW (gf  *out, const gf  *a, const gf  *b) {
    unsigned int i;
    for (i=0; i<sizeof(*out)/sizeof(uint64xn_t); i++) {
        ((uint64xn_t*)out)[i] = ((const uint64xn_t*)a)[i] - ((const uint64xn_t*)b)[i];
    }
    /*
    unsigned int i;
    for (i=0; i<sizeof(*out)/sizeof(out->limb[0]); i++) {
        out->limb[i] = a->limb[i] - b->limb[i];
    }
    */
}

void gf_copy (gf  *out, const gf  *a) {
    unsigned int i;
    for (i=0; i<sizeof(*out)/sizeof(big_register_t); i++) {
        ((big_register_t *)out)[i] = ((const big_register_t *)a)[i];
    }
}

void gf_bias (
    gf  *a, int amt
) {
    uint64_t co1 = ((1ull<<60)-1)*amt, co2 = co1-amt;
    
#if __AVX2__
    uint64x4_t lo = {co1,co1,co1,co1}, hi = {co2,co1,co1,co1};
    uint64x4_t *aa = (uint64x4_t*) a;
    aa[0] += lo;
    aa[1] += hi;
#elif __SSE2__
    uint64x2_t lo = {co1,co1}, hi = {co2,co1};
    uint64x2_t *aa = (uint64x2_t*) a;
    aa[0] += lo;
    aa[1] += lo;
    aa[2] += hi;
    aa[3] += lo;
#else
    unsigned int i;
    for (i=0; i<sizeof(*a)/sizeof(uint64_t); i++) {
        a->limb[i] += (i==4) ? co2 : co1;
    }
#endif
}

void gf_weak_reduce (gf  *a) {
    /* PERF: use pshufb/palignr if anyone cares about speed of this */
    uint64_t mask = (1ull<<60) - 1;
    uint64_t tmp = a->limb[7] >> 60;
    int i;
    a->limb[4] += tmp;
    for (i=7; i>0; i--) {
        a->limb[i] = (a->limb[i] & mask) + (a->limb[i-1]>>60);
    }
    a->limb[0] = (a->limb[0] & mask) + tmp;
}

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* __gf_H__ */
