/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lmmp/impl/ele_mul.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/lglg.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/prime_table.h"


#define mul_1(dst, rn, v)                             \
    do {                                              \
        mp_limb_t _c_ = lmmp_mul_1_(dst, dst, rn, v); \
        if (_c_ != 0) {                               \
            ++rn;                                     \
            dst[rn - 1] = _c_;                        \
        }                                             \
    } while (0)

static const ulong odd_factorial[25] = {1, 1, 3, 3, 15, 45, 315, 315,
                                        2835, 14175, 155925,
                                        467775, 6081075, 42567525,
                                        638512875, 638512875, 10854718875, 97692469875,
                                        1856156927625, 9280784638125, 194896477400625,
                                        2143861251406875, 49308808782358125,
                                        147926426347074375ull, 3698160658676859375ull};

mp_size_t lmmp_nPr_size_(ulong n, ulong r, mp_bitcnt_t* restrict bits) {
    lmmp_param_assert(n >= r);
    lmmp_param_assert(bits != NULL);
    mp_size_t shl = n - lmmp_limb_popcnt_(n);
    shl -= (n - r) - lmmp_limb_popcnt_(n - r);
    *bits = shl;
    if (n < ODD_FACTORIAL_SIZE || r <= 2) {
        return 3;
    } else if (n <= MP_UINT_MAX) {
        uint64_t l1, l2;
        l1 = log2_fac_ceil(n);
        if (n - r < ODD_FACTORIAL_SIZE)
            l2 = 0;
        else
            l2 = log2_fac_floor(n - r);
        mp_size_t rn = l1 - l2;
        return (rn + LIMB_BITS - 1) / LIMB_BITS + 2; // more 2 limb
    } else {
        // nPr < (n - r/2 + 1)^r
        ulong mean = n - r / 2 + 1;
        return lmmp_pow_1_size_(mean, r);
    }
}

static inline uint count_factors(fac_ptr fac, uint nfactors, uint n, uint r, uint p) {
    uint pn = n;
    uint e = 0;
    ulong inv = MP_ULONG_MAX / p + 1;
    while (pn > 0) {
        _udiv32by32_q_preinv(pn, pn, inv);
        e += pn;
    }
    pn = r;
    while (pn > 0) {
        _udiv32by32_q_preinv(pn, pn, inv);
        e -= pn;
    }
    if (e > 0) {
        fac[nfactors].f = p;
        fac[nfactors++].j = e;
    }
    return nfactors;
}

/**
 * @brief 使用累乘函数计算nPr（奇数部分）
 */
static mp_size_t lmmp_odd_nPr_product_(mp_ptr restrict dst, mp_size_t rn, uint n, uint r) {
    TEMP_DECL;
    ulongp restrict limbs = TALLOC_TYPE(r / 2 + 1, ulong);
    mp_size_t limbn = 0;
    ulong t = 1;
    uint v;
    mp_bitcnt_t cnt = 0;
    for (uint i = n - r + 1; i <= n; ++i) {
        ctz_shr_u32(v, i, cnt);
        t *= v;
        if (t > MP_UINT_MAX) {
            limbs[limbn++] = t;
            t = 1;
        }
    }
    if (t != 1)
        limbs[limbn++] = t;

    if (rn >= limbn) {
        mp_ptr restrict tp = TALLOC_TYPE(limbn, mp_limb_t);
        rn = lmmp_elem_mul_ulong_(dst, limbs, limbn, tp);
    } else {
        mp_ptr restrict tp = TALLOC_TYPE(limbn * 2, mp_limb_t);
        rn = lmmp_elem_mul_ulong_(tp, limbs, limbn, tp + limbn);
        lmmp_copy(dst, tp, rn);
    }
    TEMP_FREE;
    return rn;
}

mp_size_t lmmp_odd_nPr_ushort_(mp_ptr restrict dst, mp_size_t rn, ulong n, ulong r) {
    lmmp_param_assert(n >= r);
    lmmp_param_assert(n <= NPR_SHORT_LIMIT);
    if (n < ODD_FACTORIAL_SIZE) {
        if (n == 0) {
            dst[0] = 1;
        } else if (n == r) {
            dst[0] = odd_factorial[n - 1];
        } else {
            dst[0] = odd_factorial[n - 1] / odd_factorial[n - r - 1];
        }
        return 1;
    } else if (r <= 10) {
        dst[0] = 1;
        rn = 1;
        ulong t = 1, v;
        ulong i = n - r + 1;
        mp_bitcnt_t cnt = 0;
        lmmp_debug_assert(n >= 3);
        for (; i <= n - 3; i += 3) {
            t = i * (i + 1) * (i + 2);
            ctz_shr_u64(v, t, cnt);
            mul_1(dst, rn, v);
        }
        t = 1;
        for (; i <= n; ++i) {
            t *= i;
        }
        ctz_shr_u64(v, t, cnt);
        if (v != 1) {
            mul_1(dst, rn, v);
        }
        return rn;
    } else if (n <= MP_UCHAR_MAX) {
        lmmp_debug_assert(n >= 7);
        lmmp_debug_assert(r >= 2);
        dst[0] = 1;
        rn = 1;
        ulong t = 0, v;
        ulong i = n - r + 1;
        mp_bitcnt_t cnt;
        for (; i <= n - 7; i += 7) {
            t = i * (i + 1) * (i + 2) * (i + 3) * (i + 4) * (i + 5) * (i + 6);
            ctz_shr_u64(v, t, cnt);
            mul_1(dst, rn, v);
        }
        t = 1;
        for (; i <= n; ++i) {
            t *= i;
        }
        ctz_shr_u64(v, t, cnt);
        if (v != 1) {
            mul_1(dst, rn, v);
        }
        return rn;
    } else if (n <= 0xfff) {
        TEMP_S_DECL;
        ulongp restrict limbs = SALLOC_TYPE(r / 5 + 1, ulong);
        mp_size_t limbn = 0;
        ulong t, v;
        ulong i = n - r + 1;
        mp_bitcnt_t cnt;
        lmmp_debug_assert(n >= 5);
        for (; i <= (ulong)n - 5; i += 5) {
            t = i * (i + 1) * (i + 2) * (i + 3) * (i + 4);
            ctz_shr_u64(v, t, cnt);
            limbs[limbn++] = v;
        }
        t = 1;
        for (; i <= n; ++i) {
            t *= i;
        }
        ctz_shr_u64(v, t, cnt);
        if (v != 1)
            limbs[limbn++] = v;
        mp_ptr restrict tp = SALLOC_TYPE(limbn * 2, mp_limb_t);
        // 这里不能直接乘入dst，因为dst的大小可能小于limbn，导致溢出
        rn = lmmp_elem_mul_ulong_(tp, limbs, limbn, tp + limbn);
        lmmp_copy(dst, tp, rn);
        TEMP_S_FREE;
        return rn;
    } else if (n + PERMUTATION_USHORT_B_THRESHOLD > r * PERMUTATION_USHORT_K_THRESHOLD) {
        /* 测量发现，累乘法和因子分解法的算法耗时流形交线大致为直线 */
        return lmmp_odd_nPr_product_(dst, rn, n, r);
    } else {
        TEMP_DECL;
        ushort primen = lmmp_prime_cnt16_(n);
        ushort nfactors = primen;
        fac_ptr restrict fac = TALLOC_TYPE(nfactors, fac_t);
        r = n - r;
        nfactors = 0;
        for (ushort i = 1; i < primen; ++i) {
            ushort p = prime_short_table[i];
            nfactors = count_factors(fac, nfactors, n, r, p);
        }

        rn = lmmp_factors_mul_ushort_(dst, rn, fac, nfactors);

        TEMP_FREE;
        return rn;
    }
}

mp_size_t lmmp_odd_nPr_uint_(mp_ptr restrict dst, mp_size_t rn, ulong n, ulong r) {
    lmmp_param_assert(n >= r);
    lmmp_param_assert(n <= NPR_INT_LIMIT);
    if (r <= 10) {
        dst[0] = 1;
        rn = 1;
        ulong v;
        mp_bitcnt_t cnt;
        for (ulong i = n - r + 1; i <= n; ++i) {
            ctz_shr_u64(v, i, cnt);
            mul_1(dst, rn, v);
        }
        return rn;
    } else if (n + PERMUTATION_UINT_B_THRESHOLD > r * PERMUTATION_UINT_K_THRESHOLD) {
        /* 这个调优值是在近似忽略了质数表的初始化开销，主要瓶颈集中在质数表的遍历的情况下测得的 */
        /* 因此，当质数表未初始化时，这个调优值将无法代表真实性能边界 */
        return lmmp_odd_nPr_product_(dst, rn, n, r);
    } else{
        TEMP_B_DECL;
        
        lmmp_prime_int_table_init_(n);
        uint nfactors = lmmp_prime_size_(n);
        fac_ptr restrict fac = BALLOC_TYPE(nfactors, fac_t);
        r = n - r;
        nfactors = 0;

        prime_cache_t cache;
        lmmp_prime_cache_init_(&cache, n);
        while (cache.is_end == 0) {
            lmmp_prime_cache_next_(&cache);
            for (uint i = 0; i < cache.size; ++i) {
                nfactors = count_factors(fac, nfactors, n, r, cache.pp[i]);
            }
        }
        lmmp_prime_cache_free_(&cache);

        rn = lmmp_factors_mul_(dst, rn, fac, nfactors);

        TEMP_B_FREE;
        return rn;
    }
}

mp_size_t lmmp_odd_nPr_ulong_(mp_ptr restrict dst, mp_size_t rn, ulong n, ulong r) {
    lmmp_param_assert(n >= r);
    if (r < 10) {
        dst[0] = 1;
        rn = 1;
        ulong v;
        mp_bitcnt_t cnt;
        for (ulong i = n - r + 1; i <= n; ++i) {
            ctz_shr_u64(v, i, cnt);
            if (v != 1) {
                mul_1(dst, rn, v);
            }
        }
        return rn;
    }
    TEMP_DECL;
    ulongp restrict limbs = TALLOC_TYPE(r, ulong);
    mp_size_t limbn = 0;
    ulong v;
    mp_bitcnt_t cnt;
    for (ulong t = n - r + 1; t <= n; ++t) {
        ctz_shr_u64(v, t, cnt);
        if (v != 1)
            limbs[limbn++] = v;
    }

    if (rn >= limbn) {
        mp_ptr restrict tp = TALLOC_TYPE(limbn, mp_limb_t);
        rn = lmmp_elem_mul_ulong_(dst, limbs, limbn, tp);
    } else {
        mp_ptr restrict tp = TALLOC_TYPE(limbn * 2, mp_limb_t);
        rn = lmmp_elem_mul_ulong_(tp, limbs, limbn, tp + limbn);
        lmmp_copy(dst, tp, rn);
    }
    TEMP_FREE;
    return rn;
}

mp_size_t lmmp_nPr_(mp_ptr restrict dst, mp_bitcnt_t bits, mp_size_t rn, ulong n, ulong r) {
    lmmp_debug_assert(n >= r);
    lmmp_param_assert(dst != NULL);
    mp_size_t shw = bits / LIMB_BITS;
    lmmp_param_assert(rn > shw);

    bits %= LIMB_BITS;
    lmmp_zero(dst, shw);

    if (n <= NPR_SHORT_LIMIT)
        rn = lmmp_odd_nPr_ushort_(dst + shw, rn - shw, n, r);
    else if (n <= NPR_INT_LIMIT)
        rn = lmmp_odd_nPr_uint_(dst + shw, rn - shw, n, r);
    else
        rn = lmmp_odd_nPr_ulong_(dst + shw, rn - shw, n, r);

    if (bits > 0) {
        dst[shw + rn] = lmmp_shl_(dst + shw, dst + shw, rn, bits);
        rn += shw + 1;
        rn -= dst[rn - 1] == 0;
    } else {
        rn += shw;
    }
    return rn;
}
