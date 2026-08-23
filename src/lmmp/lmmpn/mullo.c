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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


void lmmp_mullo_fft_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_ptr scratch) {
    lmmp_param_assert(n > 0);
    mp_size_t hn = lmmp_fft_next_size_((n + n + 1) >> 1);
    lmmp_debug_assert(n <= hn);
    mp_ptr tp = ALLOC_TYPE(hn + 1, mp_limb_t);

    if (numa == numb) {
        lmmp_sqr_mersenne_(scratch, hn, numa, n);
        lmmp_sqr_fermat_(tp, hn, numa, n);
    } else {
        lmmp_mul_mersenne_(scratch, hn, numa, n, numb, n);
        lmmp_mul_fermat_(tp, hn, numa, n, numb, n);
    }

    mp_limb_t cy = lmmp_shr1add_nc_(scratch, scratch, tp, hn, tp[hn]);
    cy <<= LIMB_BITS - 1;
    scratch[hn - 1] += cy;
    if (scratch[hn - 1] < cy)
        lmmp_inc(scratch);

    if (n == hn) {
        cy = tp[hn] + lmmp_sub_n_(scratch + hn, scratch, tp, hn);
        // cy==1 means [tp,hn+1]!=0, then [dst,hn]!=0
        // cy==2 is impossible since [tp,hn+1] is normalized.
        // so the following dec won't overflow.
        lmmp_dec_1(scratch, cy);
    } else {
        mp_size_t n2 = 2 * n;
        cy = lmmp_sub_n_(scratch + hn, scratch, tp, n2 - hn);
        cy = tp[hn] + lmmp_sub_nc_(tp + n2 - hn, scratch + n2 - hn, tp + n2 - hn, 2 * hn - n2, cy);
        cy = lmmp_sub_1_(scratch, scratch, n2, cy);
    }
    lmmp_free(tp);
    lmmp_copy(dst, scratch, n);
}

/*
       <---t---><---m--->
       |--a1---|---a0---|
       |--b1---|---b0---|

  ,
  |\
  | \
  |  \
  +-----,
  |     |
  |     |\
  |     | \
  |     |  \
  +-----+---`
  ^  m  ^ t ^

 此算法是一种不平衡分块的算法，朴素的想法是计算平衡分块，计算一次完整的乘法，然后两次递归的调用此函数计算低位，
 事实上，我们也可以不平衡的分块，以减少递归深度，具体分析如下：
 取a和b的低位一定宽度为m，高位宽度为t，则有：
 计算一次完整的平衡乘法m，然后递归调用计算mullo，长度为t
 复杂度模型：
   ML(n) = 2*ML(a*n) + M((1-a)*n)
 其中ML为mullo的复杂度，M为mul_n的复杂度
 我们可以假定 M(n)=O(n^e) 即多项式复杂度
 则有：
   ML(n) = C(a) * n^e
   C(a) = a^e / (1-2*(1-a)^e)
 我们希望C(a)尽可能小，即希望ML(n)尽可能小，则有：
   a_opt = 1 - 2^(-1/(e-1))
 e=log(3)/log(2)  [Toom-2] -> a ~= 0.694
 e=log(5)/log(3)  [Toom-3] -> a ~= 0.775
 e=log(7)/log(4)  [Toom-4] -> a ~= 0.820
 e=log(11)/log(6) [Toom-6] -> a ~= 0.871
 e=log(15)/log(8) [Toom-8] -> a ~= 0.899
*/

#define MUL_TOOM66_THRESHOLD MUL_FFT_THRESHOLD
#define MUL_TOOM88_THRESHOLD 2921

void lmmp_mullo_dc_(
    mp_ptr    restrict  dst, 
    mp_srcptr restrict numa, 
    mp_srcptr restrict numb, 
    mp_ptr    restrict   tp,
    mp_size_t             n
) {
    if (n < MULLO_BASECASE_THRESHOLD) {
        lmmp_mullo_basecase_(dst, numa, numb, n);
        return;
    } else {
        mp_size_t m, t;
        if (n < MUL_TOOM33_THRESHOLD) {
            m = 25 * n / 36;
        } else if (n < MUL_TOOM44_THRESHOLD) {
            m = 31 * n / 40;
        } else if (n < MUL_TOOM66_THRESHOLD) {
            m = 32 * n / 39;
        } else if (n < MUL_TOOM88_THRESHOLD) {
            m = 27 * n / 31;
        } else {
            m = 9 * n / 10;
        }
        t = n - m;
        lmmp_debug_assert(2 * n > 4 * t);

#define a0 (numa)             // [numa,     m]
#define a1 (numa + m)         // [numa+m,   t]
#define b0 (numb)             // [numb,     m]
#define b1 (numb + m)         // [numb+m,   t]
#define c0 (dst)              // [dst,      m]
#define c1 (dst + m)          // [dst+m,    t]
#define lo1 (tp)              // [tp,       t]
#define lo2 (tp + t)          // [tp+t,     t]
#define scratch (tp + 2 * t)  // [tp+2*t, 2*t]
        lmmp_mul_n_(tp, a0, b0, m);
        lmmp_copy(c0, tp, n);
        lmmp_mullo_dc_(lo1, a1, b0, scratch, t);
        lmmp_mullo_dc_(lo2, a0, b1, scratch, t);
        lmmp_add_n_(c1, c1, lo1, t);
        lmmp_add_n_(c1, c1, lo2, t);
        return;
    }
#undef a0
#undef a1
#undef b0
#undef b1
#undef c0
#undef c1
#undef lo1
#undef lo2
#undef scratch
}

void lmmp_sqrlo_dc_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_ptr restrict tp, mp_size_t n) {
    if (n < MULLO_BASECASE_THRESHOLD) {
        lmmp_mullo_basecase_(dst, numa, numa, n);
        return;
    } else {
        mp_size_t m, t;
        if (n < MUL_TOOM33_THRESHOLD) {
            m = 25 * n / 36;
        } else if (n < MUL_TOOM44_THRESHOLD) {
            m = 31 * n / 40;
        } else if (n < MUL_TOOM66_THRESHOLD) {
            m = 32 * n / 39;
        } else if (n < MUL_TOOM88_THRESHOLD) {
            m = 27 * n / 31;
        } else {
            m = 9 * n / 10;
        }
        t = n - m;

#define a0 (numa)
#define a1 (numa + m)
#define c0 (dst)
#define c1 (dst + m)
#define lo (tp)          // [tp, t]
#define scratch (tp + t) // [tp+t, 2*t]
        lmmp_sqr_(tp, a0, m);
        lmmp_copy(c0, tp, n);
        lmmp_mullo_dc_(lo, a0, a1, scratch, t);
        lmmp_addshl1_n_(c1, c1, lo, t);
    }
#undef a0
#undef a1
#undef c0
#undef c1
#undef lo
#undef scratch
}

void lmmp_mullo_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_srcptr restrict numb, mp_size_t n) {
    lmmp_param_assert(n > 0);
    if (n < MULLO_DC_THRESHOLD) {
        if (numa == numb) {
            TEMP_DECL;
            mp_ptr restrict tp = TALLOC_TYPE(2 * n, mp_limb_t);
            lmmp_sqrlo_dc_(dst, numa, tp, n);
            TEMP_FREE;
            return;
        }
        TEMP_DECL;
        mp_ptr restrict tp = TALLOC_TYPE(2 * n, mp_limb_t);
        lmmp_mullo_dc_(dst, numa, numb, tp, n);
        TEMP_FREE;
        return;
    } else {
        TEMP_DECL;
        mp_ptr restrict tp = TALLOC_TYPE(2 * n, mp_limb_t);
        lmmp_mullo_fft_(dst, numa, numb, n, tp);
        TEMP_FREE;
        return;
    }
}