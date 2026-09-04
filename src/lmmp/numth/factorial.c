/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lmmp/impl/ele_mul.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/lglg.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/prime_table.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


#define MUL(dst, ap, an, bp, bn)        \
    if (an >= bn)                       \
        lmmp_mul_(dst, ap, an, bp, bn); \
    else                                \
        lmmp_mul_(dst, bp, bn, ap, an)

// 无分支，尽管_c_为0时
#define mul_1(dst, rn, v)                             \
    do {                                              \
        mp_limb_t _c_ = lmmp_mul_1_(dst, dst, rn, v); \
        dst[rn] = _c_;                                \
        rn += _c_ > 0;                                \
    } while (0)

mp_size_t lmmp_factorial_size_(uint n, mp_bitcnt_t* restrict bits) {
    mp_size_t rn;
    if (n < 20) {
        rn = 64;
    } else {
        rn = log2_fac_ceil(n);
    }
    rn = (rn + LIMB_BITS - 1) / LIMB_BITS + 2;  // more two limbs
    *bits = n - lmmp_limb_popcnt_(n);
    return rn;
}

/*
     N                      N/2                              N
    +--+                /  +--+                  \ 2     /  +--+                     \
    |  |  P_i ^ (e_i) = |  |  | P_i ^ (e_i / 2)  |    *  |  |  |  P_i ^ ( e_i mod 2) |  
    |  |                \  |  |                  /       \  |  |                     /
    i=0                    i=0                              i=0
*/

mp_size_t lmmp_factors_mul_(mp_ptr restrict dst, mp_size_t rn, fac_ptr restrict fac, uint nfactors) {
    lmmp_param_assert(dst != NULL && fac != NULL);
    lmmp_param_assert(rn > 0 && nfactors > 0);
    if (nfactors <= FACTORS_MUL_N_THRESHOLD) {
        // 绝大多数情况下，大的质因数的指数都很小，所以这里只需要考虑小的质因数。
        dst[0] = 1;
        rn = 1;
        mp_limb_t t = 1;
        for (uint i = 0; i < nfactors; i++) {
            uint f = fac[i].f;
            uint j = fac[i].j;
            lmmp_debug_assert(j != 0 && f <= MP_USHORT_MAX);
#define MAX_T 0xffffffffffff
            for (uint e = 0; e < j; e++) {
                t *= f;
                if (t > MAX_T) {
                    mul_1(dst, rn, t);
                    t = 1;
                }
            }
        }
        if (t != 1) {
            mul_1(dst, rn, t);
        }
#undef MAX_T
        return rn;
    } else {
        TEMP_DECL;
        uint new_nfactors = 0;
        ulongp restrict limbs = TALLOC_TYPE(nfactors / 2 + 1, ulong);
        ulong t = 1;
        mp_size_t limbn = 0;
        for (uint i = 0; i < nfactors; ++i) {
            uint f = fac[i].f;
            uint j = fac[i].j;
            if (j > 1) {
                fac[new_nfactors].f = f;
                fac[new_nfactors++].j = j >> 1;
            }
            if (j & 1) {
                t *= f;
                if (t > MP_UINT_MAX) {
                    limbs[limbn++] = t;
                    t = 1;
                }
            }
        }
        if (t != 1) {
            limbs[limbn++] = t;
        }

        mp_ptr restrict mp = TALLOC_TYPE(limbn * 2, mp_limb_t);
        mp_size_t mpn = 0;

        if (new_nfactors > 0) {
            if (limbn > 0) {
                mpn = lmmp_elem_mul_ulong_(mp, limbs, limbn, mp + limbn);
                lmmp_debug_assert(rn >= mpn);
                mp_size_t tn = ((rn - mpn) >> 1) + 1;
                // 这里根据mpn的大小估计剩余因子乘积的长度，额外分配两倍的tn，以进行平方。
                mp_ptr restrict tp = BALLOC_TYPE(3 * tn + 3, mp_limb_t);
                tn = lmmp_factors_mul_(tp, tn, fac, new_nfactors);

                mp_ptr restrict tp2 = tp + tn + 1;
                lmmp_sqr_(tp2, tp, tn);
                tn <<= 1;
                tn -= tp2[tn - 1] == 0;
                MUL(dst, tp2, tn, mp, mpn);
                rn = tn + mpn;
                rn -= dst[rn - 1] == 0;
            } else {
                mp_size_t tn = (rn >> 1) + 1;
                mp_ptr restrict tp = TALLOC_TYPE(tn, mp_limb_t);
                tn = lmmp_factors_mul_(tp, tn, fac, new_nfactors);
                lmmp_sqr_(dst, tp, tn);
                rn = tn << 1;
                rn -= dst[rn - 1] == 0;
            }
        } else {
            lmmp_debug_assert(limbn > 0);
            // 这里不能直接乘入dst，因为dst的大小可能小于limbn，导致溢出
            if (rn >= limbn) {
                rn = lmmp_elem_mul_ulong_(dst, limbs, limbn, mp);
            } else {
                rn = lmmp_elem_mul_ulong_(mp, limbs, limbn, mp + limbn);
                lmmp_copy(dst, mp, rn);
            }
        }
        TEMP_FREE;
        return rn;
    }
}

static inline void count_factors(fac_ptr fac, uint nfactors, uint n, uint p) {
    uint pn = n;
    uint e = 0;
    while (pn > 0) {
        pn /= p;
        e += pn;
    }
    fac[nfactors].f = p;
    fac[nfactors].j = e;
}

mp_size_t lmmp_odd_factorial_uint_(mp_ptr restrict dst, mp_size_t rn, uint n) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(rn > 0);
    lmmp_param_assert(n > MP_USHORT_MAX);

    lmmp_prime_int_table_init_(n);
    TEMP_B_DECL;
    uint nfactors = lmmp_prime_size_(n);
    fac_ptr restrict fac = BALLOC_TYPE(nfactors, fac_t);
    nfactors = 0;

    prime_cache_t cache;
    lmmp_prime_cache_init_(&cache, n);
    while (cache.is_end == 0) {
        lmmp_prime_cache_next_(&cache);
        for (uint i = 0; i < cache.size; i++) {
            // 对于阶乘n!，对于所有小于等于n的质数，贡献都至少为1
            count_factors(fac, nfactors++, n, cache.pp[i]);
        }
    }
    lmmp_prime_cache_free_(&cache);

    rn = lmmp_factors_mul_(dst, rn, fac, nfactors);

    TEMP_B_FREE;
    return rn;
}

mp_size_t lmmp_factorial_(mp_ptr restrict dst, mp_bitcnt_t bits, mp_size_t rn, uint n) {
    lmmp_param_assert(dst != NULL);
    mp_size_t shw = bits / LIMB_BITS;
    lmmp_param_assert(rn > shw);
    bits %= LIMB_BITS;
    lmmp_zero(dst, shw);

    if (n <= NPR_SHORT_LIMIT)
        rn = lmmp_odd_nPr_ushort_(dst + shw, rn - shw, n, n);
    else
        rn = lmmp_odd_factorial_uint_(dst + shw, rn - shw, n);

    if (bits > 0) {
        dst[shw + rn] = lmmp_shl_(dst + shw, dst + shw, rn, bits);
        rn += shw + 1;
        rn -= dst[rn - 1] == 0;
    } else {
        rn += shw;
    }
    return rn;
}