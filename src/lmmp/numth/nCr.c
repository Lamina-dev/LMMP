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
#include "../../../include/lmmp/numth.h"


mp_size_t lmmp_nCr_size_(uint n, uint r, mp_bitcnt_t* restrict bits) {
    lmmp_param_assert(r <= n / 2);
    lmmp_param_assert(bits != NULL);
    mp_size_t rn;
    if (r < 4 || n < ODD_FACTORIAL_SIZE) {
        rn = 3;
    } else if (n == MP_UINT_MAX) {
        uint mean = n - r / 2 + 1;
        uint64_t l1, l2;
        l1 = lmmp_pow_1_size_(mean, r);
        l2 = log2_fac_floor(r);
        l2 /= LIMB_BITS;
        rn = l1 - l2;
    } else {
        uint64_t l1, l2, l3;
        l1 = log2_fac_ceil(n);
        l2 = log2_fac_floor(r);
        if (n - r < ODD_FACTORIAL_SIZE)
            l3 = 0;
        else
            l3 = log2_fac_floor(n - r);
        rn = l1 - l2 - l3;
        rn = (rn + LIMB_BITS - 1) / LIMB_BITS;
    }
    (*bits) = n - lmmp_limb_popcnt_(n);
    (*bits) -= r - lmmp_limb_popcnt_(r);
    (*bits) -= n - r - lmmp_limb_popcnt_(n - r);
    return rn + 2; // more 2 limb
}

// 无分支，尽管可能导致溢出
#define mul_1(dst, rn, v)                             \
    do {                                              \
        mp_limb_t _c_ = lmmp_mul_1_(dst, dst, rn, v); \
        dst[rn] = _c_;                                \
        rn += _c_ > 0;                                \
    } while (0)

#define div_1(dst, rn, v)                          \
    do {                                           \
        mp_limb_t _dinv_ = lmmp_binvert_ulong_(v); \
        lmmp_divexact_1_(dst, dst, rn, v, _dinv_); \
        rn -= dst[rn - 1] == 0;                    \
    } while (0)

static inline uint factor_size_int(mp_size_t rn, uint n) {
    /*
     我们可以知道，nCr的大小一定不会超过B^rn，因此，B^rn的可以含有的质因数数量即为nCr可以含有的质因数数量的上限。
     同时，我们这里只计算的是奇数部分，比如我们可以用B^rn可以含有的3的质因数个数来估计nCr的质因数种类数，
     这是一个绝对上界，同时在不平衡时比pi(n)这个平方上界要紧得多。当然即使是这个上界，实际的质因数个数也可能远远
     小于这个上界。一个改进想法是，我们使用更大一点的质数，对于n>0xffff，我们选取这个质数为251，
     而log(B)/log(251)约等于8.02855802854906，我们近似视为8，这也是这里的常数的由来，当然，此估计可能存在低估，
     但是经过大量的校验，我们未发现任何低估的反例。
     为了同时处理不平衡与不平衡的情况，我们这里对两个估计进行比较，取较小的一个作为最终结果。不平衡时，approx1要更紧一些。
    */
    // 此处假定了LIMB_BITS为64
    ulong approx1 = rn * 8;
    lmmp_debug_assert(rn * 8 <= MP_UINT_MAX);
    ulong approx2 = lmmp_prime_size_(n);
    return approx1 < approx2 ? approx1 : approx2;
}

static inline ushort factor_size_short(mp_size_t rn) {
    /*
     经过大量的校验，*8即使是在ushort输入下，也未见低估，但是为了留有冗余，我们还是选择*10，大致相当于质数83。
    */
    // 此处假定了LIMB_BITS为64
    uint approx1 = rn * 10;
    lmmp_debug_assert(rn * 10 <= MP_USHORT_MAX);
    return (ushort)approx1;
}

static inline uint count_factors(fac_ptr fac, uint nfactors, uint n, uint r, uint nr, uint p) {
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
    pn = nr;
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

static inline mp_size_t lmmp_odd_factorial_(mp_ptr restrict dst, mp_size_t rn, uint n) {
    if (n <= NPR_SHORT_LIMIT)
        return lmmp_odd_nPr_ushort_(dst, rn, n, n);
    else
        return lmmp_odd_factorial_uint_(dst, rn, n);
}

typedef struct {
    mp_size_t nPr_n;      // nPr的limb数量
    mp_size_t fac_n;      // r! 的limb数量
    mp_bitcnt_t nPr_bits; // nPr的bit数量
    mp_bitcnt_t fac_bits; // r! 的bit数量
    uint n;
    uint r;
} bino_choose_t;

static mp_size_t lmmp_odd_nCr_div_(mp_ptr restrict dst, mp_size_t rn, bino_choose_t* restrict ctx) {
    lmmp_param_assert(rn > 0 && dst != NULL);
    TEMP_DECL;

    mp_ptr restrict nPr = TALLOC_TYPE(ctx->nPr_n, mp_limb_t);
    mp_size_t shw1 = ctx->nPr_bits / LIMB_BITS;
    mp_size_t nPr_n = lmmp_odd_nPr_product_(nPr, ctx->nPr_n - shw1, ctx->n, ctx->r);

    mp_size_t shw2 = ctx->fac_bits / LIMB_BITS;
    mp_ptr restrict fac = TALLOC_TYPE(ctx->fac_n, mp_limb_t);
    mp_size_t fac_n = lmmp_odd_factorial_(fac, ctx->fac_n - shw2, ctx->r);

    lmmp_debug_assert(rn >= nPr_n - fac_n + 1);
    lmmp_divexact_(dst, nPr, nPr_n, fac, fac_n);
    rn = nPr_n - fac_n + 1;
    rn -= dst[rn - 1] == 0;
    TEMP_FREE;
    return rn;
}

mp_size_t lmmp_odd_nCr_ushort_(mp_ptr restrict dst, mp_size_t rn, uint n, uint r) {
    lmmp_param_assert(n <= MP_USHORT_MAX);
    lmmp_param_assert(rn > 0 && dst != NULL);
    lmmp_param_assert(r <= n / 2);
    if (r < ODD_FACTORIAL_SIZE) {
        rn = lmmp_odd_nPr_ushort_(dst, rn, n, r);
        mp_limb_t t = 0;
        lmmp_odd_nPr_ushort_(&t, 1, r, r);
        div_1(dst, rn, t);
        return rn;
    } else if (rn < BINOMIAL_RN_BASECASE_THRESHOLD) {
        if (r <= 4 || n > 0xfff) {
            dst[0] = 1;
            rn = 1;
            ulong t, v;
            mp_bitcnt_t cnt;
            for (ulong i = 1; i <= r; ++i) {
                t = n - i + 1;
                ctz_shr_u64(v, t, cnt);
                mul_1(dst, rn, v);
                ctz_shr_u64(v, i, cnt);
                div_1(dst, rn, v);
            }
            return rn;
        } else {
            dst[0] = 1;
            rn = 1;
            ulong i = 1;
            ulong t = 1, d = 1, v;
            mp_bitcnt_t cnt;
            for (; i <= (ulong)r - 4; i += 4) {
                // 相邻四个数必含有3这个公共因子
                d = i * (i + 1) * (i + 2) * (i + 3);
                d /= 3;
                t = (n - i + 1) * (n - i) * (n - i - 1) * (n - i - 2);
                t /= 3;
                ctz_shr_u64(v, t, cnt);
                mul_1(dst, rn, v);
                ctz_shr_u64(v, d, cnt);
                div_1(dst, rn, v);
            }
            for (; i <= r; ++i) {
                t = n - i + 1;
                ctz_shr_u64(v, t, cnt);
                mul_1(dst, rn, v);
                ctz_shr_u64(v, i, cnt);
                div_1(dst, rn, v);
            }
            return rn;
        }
    } else {
        TEMP_DECL;
        ushort primen = lmmp_prime_cnt16_(n);
        ushort nfactors = factor_size_short(rn);
        nfactors = primen < nfactors ? primen : nfactors;
        fac_ptr restrict fac = TALLOC_TYPE(nfactors, fac_t);
        ushort nr = n - r;
        nfactors = 0;
        for (ushort i = 1; i < primen; ++i) {
            ushort p = prime_short_table[i];
            nfactors = count_factors(fac, nfactors, n, r, nr, p);
        }

        rn = lmmp_factors_mul_ushort_(dst, rn, fac, nfactors);

        TEMP_FREE;
        return rn;
    }
}

mp_size_t lmmp_odd_nCr_uint_(mp_ptr restrict dst, mp_size_t rn, uint n, uint r) {
    lmmp_param_assert(r <= (n / 2));
    lmmp_param_assert(rn > 0 && dst != NULL);
    if (r <= 3 || (n > 0xfffffff && rn < BINOMIAL_RN_BASECASE_THRESHOLD)) {
        dst[0] = 1;
        rn = 1;
        ulong t, v;
        mp_bitcnt_t cnt;
        for (ulong i = 1; i <= r; ++i) {
            t = n - i + 1;
            ctz_shr_u64(v, t, cnt);
            mul_1(dst, rn, v);
            ctz_shr_u64(v, i, cnt);
            div_1(dst, rn, v);
        }
        return rn;
    } else if (rn < BINOMIAL_RN_BASECASE_THRESHOLD) {
        dst[0] = 1;
        rn = 1;
        ulong i = 1;
        ulong t = 1, d = 1, v;
        mp_bitcnt_t cnt;
        for (; i <= (ulong)r - 2; i += 2) {
            d = i * (i + 1);
            t = (n - i + 1) * (n - i);
            ctz_shr_u64(v, t, cnt);
            mul_1(dst, rn, v);
            ctz_shr_u64(v, d, cnt);
            div_1(dst, rn, v);
        }
        for (; i <= r; ++i) {
            t = n - i + 1;
            ctz_shr_u64(v, t, cnt);
            mul_1(dst, rn, v);
            ctz_shr_u64(v, i, cnt);
            div_1(dst, rn, v);
        }
        return rn;
    } else {
        bino_choose_t ctx;
        ctx.nPr_n = lmmp_nPr_size_(n, r, &ctx.nPr_bits);
        ctx.fac_n = lmmp_factorial_size_(r, &ctx.fac_bits);
        if (50 * ctx.nPr_n > 89 * ctx.fac_n) {
            /* 这个调优值是在近似忽略了质数表的初始化开销，主要瓶颈集中在质数表的遍历的情况下测得的 */
            /* 因此，当质数表未初始化时，这个调优值将无法代表真实性能边界 */
            ctx.n = n;
            ctx.r = r;
            return lmmp_odd_nCr_div_(dst, rn, &ctx);
        } else {
            lmmp_prime_int_table_init_(n);
            TEMP_B_DECL;
            uint nfactors = factor_size_int(rn, n);
            fac_ptr restrict fac = BALLOC_TYPE(nfactors, fac_t);
            uint nr = n - r;

            nfactors = 0;
            prime_cache_t cache;
            lmmp_prime_cache_init_(&cache, n);
            while (cache.is_end == 0) {
                lmmp_prime_cache_next_(&cache);
                for (uint i = 0; i < cache.size; ++i) {
                    nfactors = count_factors(fac, nfactors, n, r, nr, cache.pp[i]);
                }
            }
            lmmp_prime_cache_free_(&cache);

            rn = lmmp_factors_mul_(dst, rn, fac, nfactors);

            TEMP_B_FREE;
            return rn;
        }
    }
}

mp_size_t lmmp_nCr_(mp_ptr restrict dst, mp_bitcnt_t bits, mp_size_t rn, uint n, uint r) {
    lmmp_param_assert(r <= (n / 2));
    lmmp_param_assert(dst != NULL);
    mp_size_t shw = bits / LIMB_BITS;
    lmmp_param_assert(rn > shw);

    bits %= LIMB_BITS;
    lmmp_zero(dst, shw);

    if (n <= NCR_SHORT_LIMIT)
        rn = lmmp_odd_nCr_ushort_(dst + shw, rn - shw, n, r);
    else
        rn = lmmp_odd_nCr_uint_(dst + shw, rn - shw, n, r);

    if (bits > 0) {
        dst[shw + rn] = lmmp_shl_(dst + shw, dst + shw, rn, bits);
        rn += shw + 1;
        rn -= dst[rn - 1] == 0;
    } else {
        rn += shw;
    }
    return rn;
}