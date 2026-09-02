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

#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

/**
 * @brief 计算 [dst,n] = [xp,n]*[ap,n] div B^n
 * @param dst 结果指针
 * @param tp scratch space, need 2*n limbs
 * @warning [xp,n] * [ap,n] mod B^n == 1
 */
static inline void binvert_mulhi_(mp_ptr dst, mp_srcptr xp, mp_srcptr ap, mp_size_t n, mp_ptr tp) {
    if (n < MULHI_MERSENNE_THRESHOLD) {
        lmmp_mul_n_(tp, xp, ap, n);
        lmmp_copy(dst, tp + n, n);
    } else {
        mp_size_t m = lmmp_fft_next_size_((n * 2 + 1) >> 1);
        lmmp_debug_assert(n * 2 > m && m >= n);
        lmmp_mul_mersenne_(tp, m, xp, n, ap, n);
        lmmp_dec(tp);
        mp_size_t fn = m - n;   // 从 tp+n 开始的长度
        mp_size_t sn = n - fn;  // 从 tp 开始的长度
        lmmp_copy(dst, tp + n, fn);
        lmmp_copy(dst + fn, tp, sn);
    }
}

static inline void lmmp_sqrlo_n_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict numa,
    mp_size_t             n,
    mp_ptr    restrict   tp
) {
    if (n < MULLO_DC_THRESHOLD) {
        lmmp_sqrlo_dc_(dst, numa, tp, n);
    } else {
        lmmp_mullo_fft_(dst, numa, numa, n, tp);
    }
}

static inline void lmmp_mullo_n_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict numa,
    mp_srcptr restrict numb,
    mp_size_t             n,
    mp_ptr    restrict   tp
) {
    if (n < MULLO_DC_THRESHOLD) {
        lmmp_mullo_dc_(dst, numa, numb, tp, n);
    } else {
        lmmp_mullo_fft_(dst, numa, numb, n, tp);
    }
}

/*
balanced:
    a := [numa,2*n]
 we neead to find x such that x * a == 1 mod B^2n
 we know that   a == a_lo + a_hi * B^n
       and   x_lo == a_lo ^ -1 mod B^n
 means x_lo * a_lo == 1 + k * B^n and k < B^n
 
 x = x_lo * (2 - a * x_lo)  mod B^2n
   = x_lo * (2 - a_lo * x_lo - a_hi * x_lo * B^n)  mod B^2n
   = x_lo * (1 - k * B^n - a_hi * x_lo * B^n)  mod B^2n
   = x_lo - (k * x_lo + a_hi * x_lo^2) * B^n  mod B^2n
-----------------------------------------------------------------------------
unbalanced:
    a := [numa,na]
 我们需要求x，使得x * a == 1 mod B^n ，同时n远远大于na
 我们可以求出 x0 = a ^ -1 mod B^na，这是一个平衡的逆元
 接下来，我们使用线性递推法来求，我们以na个limb为基本处理单元
 假定现在已经求出 t 个，即 Xt = X0 + X1*B^na + X2*B^2na +... + X{t-1}*B^(t-1)*na
 且满足 a*Xt == 1 mod B^t*na
 可以写成 a*Xt = 1 + k * B^t*na, k < B^na
 我们需要求出下一个 p，使得X{t+1} = X{t} + p*B^na
 我们代入 a*X{t+1} = 1 mod B^(t+1)*na
 可以得到
        1 + k * B^t*na + a*p*B^t*na = 1 mod B^(t+1)*na
                            k + a*p = 0 mod B^na
                                  p = -k * a^-1 mod B^na
 此时，我们已经有了新的X{t+1}，我们需要更新 k 为 k'
 我们需要 k' 满足 
      a*X{t+1} = 1 + k' * B^(t+1)*na, k' < B^na
     k' * B^na = k + a*p 
            k' = (k + a*p) / B^na
*/


void lmmp_binvert_n_dc_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t n, mp_ptr restrict tp) {
    lmmp_param_assert(dst != NULL && tp != NULL);
    lmmp_param_assert(numa != NULL && n > 0);
    lmmp_param_assert(numa[0] % 2 == 1);
    if (n == 1) {
        dst[0] = lmmp_binvert_ulong_(numa[0]);
    } else if (n == 2) {
        lmmp_binvert_2_(dst, numa);
    } else if (n == 3) {
        lmmp_binvert_3_(dst, numa);
    } else if (n == 4) {
        lmmp_binvert_4_(dst, numa);
    } else if (n % 2 == 0) {
        mp_size_t halfn = n / 2;

#define k               (tp)              // [tp,          halfn]
#define alo             (numa)            // [numa,        halfn]
#define ahi             (numa + halfn)    // [numa+halfn,  halfn]
#define xlo             (dst)             // [dst,         halfn]
#define xhi             (dst + halfn)     // [dst+halfn,   halfn]
#define xlo_sqr         (tp + halfn)      // [tp+halfn,    halfn]
#define xlo_sqr_mul_ahi (tp + 2 * halfn)  // [tp+2*halfn,  halfn]
#define scratch         (tp + 3 * halfn)  // [tp+3*halfn,2*halfn]
//      ________________________________________________________________
// tp : |_________________________5*(n+1)/2____________________________|
//      |   k   | xlo_sqr | xlo_sqr_mul_ahi |   scratch   | remaining  |
//      |_halfn_|__halfn__|______halfn______|___2*halfn___|            |

        lmmp_binvert_n_dc_(xlo, alo, halfn, tp);
        binvert_mulhi_(k, xlo, alo, halfn, tp + halfn);
        lmmp_sqrlo_n_(xlo_sqr, xlo, halfn, scratch);
        lmmp_mullo_n_(xlo_sqr_mul_ahi, ahi, xlo_sqr, halfn, scratch);
        lmmp_mullo_n_(xhi, xlo, k, halfn, scratch);
        lmmp_add_n_(xhi, xhi, xlo_sqr_mul_ahi, halfn);
        lmmp_not_(xhi, xhi, halfn);
        lmmp_inc(xhi);
    } else {
        mp_size_t halfn = n / 2 + 1;
        mp_size_t ahin = n - halfn;

#define k               (tp)              // [tp,          halfn]
#define alo             (numa)            // [numa,        halfn]
#define ahi             (numa + halfn)    // [numa+halfn,   ahin]
#define xlo             (dst)             // [dst,         halfn]
#define xhi             (dst + halfn)     // [dst+halfn,    ahin]
#define xlo_sqr         (tp + halfn)      // [tp+halfn,     ahin]
#define xlo_sqr_mul_ahi (tp + 2 * halfn)  // [tp+2*halfn,   ahin]
#define scratch         (tp + 3 * halfn)  // [tp+3*halfn, 2*ahin]
//      ________________________________________________________________
// tp : |_________________________5*(n+1)/2____________________________|
//      |    k    | xlo_sqr | xlo_sqr_mul_ahi |   scratch  | remaining |
//      |__halfn__|__halfn__|______halfn______|___2*ahin___|           |

        lmmp_binvert_n_dc_(xlo, alo, halfn, tp);
        binvert_mulhi_(k, xlo, alo, halfn, tp + halfn);
        lmmp_sqrlo_n_(xlo_sqr, xlo, ahin, scratch);
        lmmp_mullo_n_(xlo_sqr_mul_ahi, ahi, xlo_sqr, ahin, scratch);
        lmmp_mullo_n_(xhi, xlo, k, ahin, scratch);
        lmmp_add_n_(xhi, xhi, xlo_sqr_mul_ahi, ahin);
        lmmp_not_(xhi, xhi, ahin);
        lmmp_inc(xhi);
    }
#undef k
#undef alo
#undef ahi
#undef xlo
#undef xhi
#undef xlo_sqr
#undef xlo_sqr_mul_ahi
#undef scratch
}

void lmmp_binvert_unbalanced_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_size_t n, mp_ptr restrict tp) {
    lmmp_param_assert(dst != NULL && numa != NULL && tp != NULL);
    lmmp_param_assert(numa[0] % 2 == 1);
    lmmp_param_assert(n > na && na > 0);

#define a_binvert (tp)             // [tp,              na]
#define k         (tp + 1 * na)    // [tp+na,           na]
#define scratch   (tp + 2 * na)    // [tp+2*na, 5*(na+1)/2]

    lmmp_binvert_n_dc_(a_binvert, numa, na, scratch);
    lmmp_copy(dst, a_binvert, na);
    binvert_mulhi_(k, a_binvert, numa, na, scratch);

    // a_binvert 低位不可能为0，故加一不会进位
    lmmp_debug_assert(a_binvert[0] != 0);
    lmmp_not_(a_binvert, a_binvert, na);
    a_binvert[0] += 1;

    mp_size_t i = na;
    for (; i < n - na; i += na) {
        lmmp_mullo_n_(dst + i, a_binvert, k, na, scratch);
        /*
        FIXME: 这里的循环中，第二个乘数numa，始终保持不变
               在拥有可以惰性初始化的FFT算法的情况下，可以节省numa的正变换
               在循环的情况下，这将会有可观的性能提升
        */
        lmmp_mul_n_(scratch, dst + i, numa, na);
        // now [scratch,2*na] = a * p
        if (lmmp_add_n_(scratch, scratch, k, na)) {
            lmmp_inc(scratch + na);
        }
        lmmp_copy(k, scratch + na, na);
    }

    lmmp_mullo_n_(dst + i, a_binvert, k, n - i, scratch);
#undef a_binvert
#undef k
#undef scratch
}

void lmmp_binvert_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_size_t n) {
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_param_assert(na > 0 && n >= na);
    lmmp_param_assert(numa[0] % 2 == 1);
    TEMP_DECL;
    if (n == na) {
        mp_ptr restrict tp = TALLOC_TYPE(5 * (n + 1) / 2, mp_limb_t);
        lmmp_binvert_n_dc_(dst, numa, na, tp);
    } else if (na == 1) {
        lmmp_binvert_unbalanced_1_(dst, numa[0], n);
    } else if (na == 2) {
        lmmp_binvert_unbalanced_2_(dst, numa, n);
    } else if (4 * n >= 5 * na) {
        // n/na >= 5/4 这是一个比较简单的调优结果
        mp_ptr restrict tp = TALLOC_TYPE((9 * n + 5) / 2, mp_limb_t);
        lmmp_binvert_unbalanced_(dst, numa, na, n, tp);
    } else {
        mp_ptr restrict ap = TALLOC_TYPE(n, mp_limb_t);
        mp_ptr restrict tp = TALLOC_TYPE((5 * n + 5) / 2, mp_limb_t);
        lmmp_copy(ap, numa, na);
        lmmp_zero(ap + na, n - na);
        lmmp_binvert_n_dc_(dst, ap, n, tp);
    }
    TEMP_FREE;
}
