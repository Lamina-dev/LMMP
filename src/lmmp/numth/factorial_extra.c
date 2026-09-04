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

mp_size_t lmmp_2factorial_size_(uint n, mp_bitcnt_t* restrict bits) {
    mp_size_t rn;
    uint k;
    if (n < 30) {
        rn = 64;
        k = n / 2;
        if (n % 2 == 0) {
            *bits = n - lmmp_limb_popcnt_(k);
        } else {
            *bits = 0;
        }
    } else {
        k = n / 2;
        if (n % 2 == 0) {
            // n=2k
            // n! = (2k)! = 2^k * (k!)
            rn = k + log2_fac_ceil(k);
            *bits = n - lmmp_limb_popcnt_(k);
        } else {
            // n=2k+1
            // n! = (2k+1)! = (2k+1)! / (2k)!! = (2k+1)! / (2^k * (k!))
            rn = log2_fac_ceil(n);
            rn -= k + log2_fac_floor(k);
            *bits = 0;
        }
    }
    rn = (rn + LIMB_BITS - 1) / LIMB_BITS + 2;  // more two limbs
    return rn;
}

/*
     N                      N/2                              N
    +--+                /  +--+                  \ 2     /  +--+                     \
    |  |  P_i ^ (e_i) = |  |  | P_i ^ (e_i / 2)  |    *  |  |  |  P_i ^ ( e_i mod 2) |  
    |  |                \  |  |                  /       \  |  |                     /
    i=0                    i=0                              i=0
*/

mp_size_t lmmp_factors_mul_ushort_(mp_ptr restrict dst, mp_size_t rn, fac_ptr restrict fac, ushort nfactors) {
    lmmp_param_assert(dst != NULL && fac != NULL);
    lmmp_param_assert(rn > 0 && nfactors > 0);
    if (nfactors <= FACTORS_MUL_N_THRESHOLD && rn < 20) {
        // 此处额外校验rn的大小，因为hyper阶乘和super阶乘在阶数较小时，仅有很少的因子，但其指数很大
        // 为了避免算法退化，此处校验rn，仅对rn较小时进行朴素乘法。
        dst[0] = 1;
        rn = 1;
        mp_limb_t t = 1;
        for (ushort i = 0; i < nfactors; i++) {
            ushort f = fac[i].f;
            uint j = fac[i].j;
            lmmp_debug_assert(j != 0);
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
        ushort new_nfactors = 0;
        ulongp restrict limbs = TALLOC_TYPE(nfactors / 4 + 1, ulong);
        ulong t = 1;
        mp_size_t limbn = 0;
        for (ushort i = 0; i < nfactors; ++i) {
            ushort f = fac[i].f;
            uint j = fac[i].j;
            if (j > 1) {
                fac[new_nfactors].f = f;
                fac[new_nfactors++].j = j >> 1;
            }
            if (j & 1) {
                t *= f;
#define MAX_T 0xffffffffffff
                if (t > MAX_T) {
                    limbs[limbn++] = t;
                    t = 1;
                }
#undef MAX_T
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
                tn = lmmp_factors_mul_ushort_(tp, tn, fac, new_nfactors);

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
                tn = lmmp_factors_mul_ushort_(tp, tn, fac, new_nfactors);
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

static inline uint count_2facodd_factors(fac_ptr fac, uint nfactors, uint n, uint k, uint p) {
    uint e = 0;
    uint pn = n;
    ulong inv = MP_ULONG_MAX / p + 1;
    while (pn > 0) {
        _udiv32by32_q_preinv(pn, pn, inv);
        e += pn;
    }
    pn = k;
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
 * @brief 计算奇数双阶乘（n!!）
 * @param dst 输出数组
 * @param rn 输出数组的长度
 * @param n 奇数，等于2k+1
 * @param k 双阶乘的参数
 * @return 输出数组的长度
 */
static mp_size_t lmmp_odd_2factorial_ushort_(mp_ptr restrict dst, mp_size_t rn, ushort n, ushort k) {
    TEMP_DECL;
    ushort primen = lmmp_prime_cnt16_(n);
    ushort nfactors = primen;
    fac_ptr restrict fac = TALLOC_TYPE(nfactors, fac_t);
    nfactors = 0;
    for (ushort i = 1; i < primen; ++i) {
        ushort p = prime_short_table[i];
        nfactors = count_2facodd_factors(fac, nfactors, n, k, p);
    }

    rn = lmmp_factors_mul_ushort_(dst, rn, fac, nfactors);

    TEMP_FREE;
    return rn;
}

/**
 * @brief 计算奇数双阶乘（n!!）
 * @param dst 输出数组
 * @param rn 输出数组的长度
 * @param n 奇数，等于2k+1
 * @param k 双阶乘的参数
 * @return 输出数组的长度
 */
static mp_size_t lmmp_odd_2factorial_uint_(mp_ptr restrict dst, mp_size_t rn, uint n, uint k) {
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
            nfactors = count_2facodd_factors(fac, nfactors, n, k, cache.pp[i]);
        }
    }
    lmmp_prime_cache_free_(&cache);

    rn = lmmp_factors_mul_(dst, rn, fac, nfactors);

    TEMP_B_FREE;
    return rn;
}

mp_size_t lmmp_2factorial_(mp_ptr restrict dst, mp_bitcnt_t bits, mp_size_t rn, uint n) {
    static const mp_limb_t odd_2factorial_table[17] = {1, 3, 15, 105, 945, 10395, 135135,
                                                       2027025, 34459425, 654729075,
                                                       13749310575, 316234143225,
                                                       7905853580625, 213458046676875,
                                                       6190283353629375, 191898783962510625,
                                                       6332659870762850625};

    lmmp_param_assert(dst != NULL);
    mp_size_t shw = bits / LIMB_BITS;
    lmmp_param_assert(rn > shw);
    bits %= LIMB_BITS;
    lmmp_zero(dst, shw);

    uint k = n / 2;
    if (n % 2 == 0) {
        if (k <= MP_USHORT_MAX)
            rn = lmmp_odd_nPr_ushort_(dst + shw, rn - shw, k, k);
        else
            rn = lmmp_odd_factorial_uint_(dst + shw, rn - shw, k);
    } else {
        if (k < 17) {
            dst[shw] = odd_2factorial_table[k];
            rn = 1;
        } else if (n <= MP_USHORT_MAX)
            rn = lmmp_odd_2factorial_ushort_(dst + shw, rn - shw, n, k);
        else
            rn = lmmp_odd_2factorial_uint_(dst + shw, rn - shw, n, k);
    }

    if (bits > 0) {
        dst[shw + rn] = lmmp_shl_(dst + shw, dst + shw, rn, bits);
        rn += shw + 1;
        rn -= dst[rn - 1] == 0;
    } else {
        rn += shw;
    }
    return rn;
}

static inline uint count_hyperfac_factors(ushort n, ushort p) {
    uint v = 0;
    uint npi = n / p;
    uint pi = p;
    while (npi > 0) {
        v += pi * npi * (npi + 1) / 2;
        npi /= p;
        pi *= p;
    }
    return v;
}

static inline uint count_superfac_factors(ushort n, ushort p) {
    uint v = 0;
    uint pi = p;
    uint N = (uint)n + 1;
    uint npi = n / pi;
    while (npi > 0) {
        v += npi * N - pi * npi * (npi + 1) / 2;
        pi *= p;
        npi /= p;
    }
    return v;
}

mp_size_t lmmp_hyperfac_size_(ushort n, mp_bitcnt_t* restrict bits) {
    if (n == 0 || n == 1) {
        *bits = 0;
        return 1;
    }
    // A = 1.2824271291006226369   Glaisher–Kinkelin constant
    const double logA = 0.24875447703378426;
    const double log2 = 0.69314718055994531;  // log(2)
    double n_sqr = (uint)n * n;
    double log_n = log(n);
    double r = 0.5 * n_sqr * log_n - 0.25 * n_sqr + 0.5 * log_n * (double)n + 1.0 / 12.0 * log_n + logA;
    mp_size_t rn = (mp_size_t)ceil(r / log2);
    rn = (rn + LIMB_BITS - 1) / LIMB_BITS + 2;  // more two limbs
    *bits = count_hyperfac_factors(n, 2);
    return rn;
}

mp_size_t lmmp_superfac_size_(ushort n, mp_bitcnt_t* restrict bits) {
    if (n == 0 || n == 1) {
        *bits = 0;
        return 1;
    }
    // zeta函数在-1处的导数
    // zeta(-1) = 1/12 - log(A) // A 即 Glaisher–Kinkelin constant
    const double zeta_diff_neg1 = -0.16542114370045093;
    const double log2 = 0.69314718055994531;     // log(2)
    const double log_2pi = 1.83787706640934548;  // log(2*pi)
    double z = (double)n + 1;
    double z_sqr = z * z;
    double log_z = log(z);
    double r = 0.5 * z_sqr * log_z - 0.75 * z_sqr + 0.5 * z * log_2pi - 1.0 / 12.0 * log_z + zeta_diff_neg1;
    mp_size_t rn = (mp_size_t)ceil(r / log2);
    rn = (rn + LIMB_BITS - 1) / LIMB_BITS + 2;  // more two limbs
    *bits = count_superfac_factors(n, 2);
    return rn;
}

mp_size_t lmmp_hyperfac_(mp_ptr restrict dst, mp_bitcnt_t bits, mp_size_t rn, ushort n) {
    lmmp_param_assert(dst != NULL);
    mp_size_t shw = bits / LIMB_BITS;
    bits %= LIMB_BITS;
    if (n == 0) {
        lmmp_debug_assert(shw == 0);
        dst[0] = 1;
        return 1;
    } else if (n <= 8) {
        lmmp_debug_assert(shw == 0);
        static const mp_limb_t odd_hyperfac_table[8] = {1, 1, 27, 27, 84375, 61509375, 50655615215625, 50655615215625};
        dst[0] = odd_hyperfac_table[n - 1];
        rn = 1;
    } else {
        lmmp_zero(dst, shw);
        TEMP_DECL;
        ushort primen = lmmp_prime_cnt16_(n);
        ushort nfactors = primen;
        fac_ptr restrict fac = TALLOC_TYPE(nfactors, fac_t);
        nfactors = 0;
        for (ushort i = 1; i < primen; ++i) {
            ushort p = prime_short_table[i];
            uint j = count_hyperfac_factors(n, p);
            fac[nfactors].f = p;
            fac[nfactors++].j = j;
        }

        lmmp_param_assert(rn > shw);
        rn = lmmp_factors_mul_ushort_(dst + shw, rn - shw, fac, nfactors);
        TEMP_FREE;
    }
    if (bits > 0) {
        dst[shw + rn] = lmmp_shl_(dst + shw, dst + shw, rn, bits);
        rn += shw + 1;
        rn -= dst[rn - 1] == 0;
    } else {
        rn += shw;
    }
    return rn;
}

mp_size_t lmmp_superfac_(mp_ptr restrict dst, mp_bitcnt_t bits, mp_size_t rn, ushort n) {
    lmmp_param_assert(dst != NULL);
    mp_size_t shw = bits / LIMB_BITS;
    bits %= LIMB_BITS;
    if (n == 0) {
        lmmp_debug_assert(shw == 0);
        dst[0] = 1;
        return 1;
    } else if (n <= 8) {
        lmmp_debug_assert(shw == 0);
        static const mp_limb_t odd_superfac_table[8] = {1, 1, 3, 9, 135, 6075, 1913625, 602791875};
        dst[0] = odd_superfac_table[n - 1];
        rn = 1;
    } else {
        lmmp_zero(dst, shw);
        TEMP_DECL;
        ushort primen = lmmp_prime_cnt16_(n);
        ushort nfactors = primen;
        fac_ptr restrict fac = TALLOC_TYPE(nfactors, fac_t);
        nfactors = 0;
        for (ushort i = 1; i < primen; ++i) {
            ushort p = prime_short_table[i];
            uint j = count_superfac_factors(n, p);
            fac[nfactors].f = p;
            fac[nfactors++].j = j;
        }

        lmmp_param_assert(rn > shw);
        rn = lmmp_factors_mul_ushort_(dst + shw, rn - shw, fac, nfactors);
        TEMP_FREE;
    }
    if (bits > 0) {
        dst[shw + rn] = lmmp_shl_(dst + shw, dst + shw, rn, bits);
        rn += shw + 1;
        rn -= dst[rn - 1] == 0;
    } else {
        rn += shw;
    }
    return rn;
}

mp_size_t lmmp_primefac_size_(uint n) {
    // Chebyshev's estimate
    // pn# < exp(1.000028*n)
    const double log2 = 0.69314718055994531;  // log(2)
    double l = 1.000028 * (double)n;
    l /= log2;
    mp_size_t rn = (mp_size_t)ceil(l);
    rn = (rn + LIMB_BITS - 1) / LIMB_BITS + 2;  // more two limbs
    return rn;
}

mp_size_t lmmp_primefac_(mp_ptr restrict dst, mp_size_t rn, uint n) {
    if (n < 2) {
        dst[0] = 1;
        return 1;
    } else if (n <= MP_USHORT_MAX) {
        TEMP_S_DECL;
        ushort primen = lmmp_prime_cnt16_(n);
        /* 至少4个质数的乘积才可能填满一个limb */
        ulongp restrict pp = SALLOC_TYPE(primen / 4 + 1, ulong);
        ulong t = 1;
        mp_size_t pn = 0;
        for (ushort i = 0; i < primen; i++) {
            t *= prime_short_table[i];
#define MAX_T 0xffffffffffff
            if (t > MAX_T) {
                pp[pn++] = t;
                t = 1;
            }
#undef MAX_T
        }
        if (t > 1) {
            pp[pn++] = t;
        }

        if (rn >= pn) {
            mp_ptr restrict tp = SALLOC_TYPE(pn, mp_limb_t);
            pn = lmmp_elem_mul_ulong_(dst, pp, pn, tp);
        } else {
            mp_ptr restrict prod = SALLOC_TYPE(pn * 2, mp_limb_t);
            pn = lmmp_elem_mul_ulong_(prod, pp, pn, prod + pn);
            lmmp_debug_assert(rn >= pn);
            lmmp_copy(dst, prod, pn);
        }
        TEMP_S_FREE;
        return pn;
    } else {
        TEMP_B_DECL;
        lmmp_prime_int_table_init_(n);
        uint pn = lmmp_prime_size_(n);
        /* 至少2个质数的乘积才可能填满一个limb */
        ulongp restrict pp = BALLOC_TYPE(pn / 2 + 1, ulong);
        pn = 0;
        ulong t = 2;  // 2 is the smallest prime number

        prime_cache_t cache;
        lmmp_prime_cache_init_(&cache, n);
        while (cache.is_end == 0) {
            lmmp_prime_cache_next_(&cache);
            for (uint i = 0; i < cache.size; i++) {
                t *= cache.pp[i];
                if (t > MP_UINT_MAX) {
                    pp[pn++] = t;
                    t = 1;
                }
            }
        }
        lmmp_prime_cache_free_(&cache);

        if (t > 1) {
            pp[pn++] = t;
        }

        if (rn >= pn) {
            mp_ptr restrict tp = BALLOC_TYPE(pn, mp_limb_t);
            pn = lmmp_elem_mul_ulong_(dst, pp, pn, tp);
        } else {
            mp_ptr restrict prod = BALLOC_TYPE(pn * 2, mp_limb_t);
            pn = lmmp_elem_mul_ulong_(prod, pp, pn, prod + pn);
            lmmp_debug_assert(rn >= pn);
            lmmp_copy(dst, prod, pn);
        }

        TEMP_B_FREE;
        return pn;
    }
}
