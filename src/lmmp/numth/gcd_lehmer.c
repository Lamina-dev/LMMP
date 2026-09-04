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

#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"

typedef struct {
    slong m11, m12;
    slong m21, m22;
} lmmp_gcd_lehmer_t;

#define LEHMER_MIN_V 0x100000000ll
#define LEHMER_EXACT_BITS 63

static void lmmp_gcd_lehmer_step_(slong u, slong v, lmmp_gcd_lehmer_t* gcd) {
#define A (gcd->m11)
#define B (gcd->m12)
#define C (gcd->m21)
#define D (gcd->m22)

    lmmp_debug_assert(u >= 0 && v >= 0);
    lmmp_debug_assert(u >= v);
    A = 1; B = 0;
    C = 0; D = 1;

    while (v != 0) {
        slong q = u / v;
        slong t = u % v;

        u = v;
        v = t;

        t = A - q * C;
        A = C;
        C = t;
        t = B - q * D;
        B = D;
        D = t;

        if (v < (slong)LEHMER_MIN_V) break;
    }

    return;
#undef A
#undef B
#undef C
#undef D
}

static void lmmp_lehmer_extract_(mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn, slong* restrict a, slong* restrict b) {
    lmmp_param_assert(un > 1 && vn > 1);
    lmmp_param_assert(un >= vn);
    lmmp_param_assert(up != NULL && vp != NULL);
    lmmp_param_assert(a != NULL && b != NULL);

    int kz = lmmp_limb_bits_(up[un - 1]);
    if (kz >= LEHMER_EXACT_BITS) {
        *a = up[un - 1] >> (kz - LEHMER_EXACT_BITS);
        if (vn == un)
            *b = vp[vn - 1] >> (kz - LEHMER_EXACT_BITS);
        else
            *b = 0;
    } else {
        *a = up[un - 1] << (LEHMER_EXACT_BITS - kz);
        *a |= up[un - 2] >> (LIMB_BITS - (LEHMER_EXACT_BITS - kz));
        if (un > vn + 1) {
            *b = 0;
        } else if (un == vn + 1) {
            *b = vp[vn - 1] >> (LIMB_BITS - (LEHMER_EXACT_BITS - kz));
        } else {
            *b = vp[vn - 1] << (LEHMER_EXACT_BITS - kz);
            *b |= vp[vn - 2] >> (LIMB_BITS - (LEHMER_EXACT_BITS - kz));
        }
    }
}

typedef struct {
    mp_ptr tp;
    mp_ptr mp;
    mp_ptr np;
    mp_size_t tn;
    mp_size_t mn;
    mp_size_t nn;
} lehmer_stack_t;

/**
 * @brief dst = |x - y|（大数减法取绝对值），返回归一化长度
 * @warning eqsep(dst,[x|y])
 */
static mp_size_t lmmp_lehmer_sub_mag_(mp_ptr dst, mp_srcptr x, mp_size_t xn, mp_srcptr y, mp_size_t yn) {
    mp_size_t rn;
    if (xn > yn) {
        lmmp_sub_(dst, x, xn, y, yn);
        rn = xn;
    } else if (xn < yn) {
        lmmp_sub_(dst, y, yn, x, xn);
        rn = yn;
    } else {
        int cmp = lmmp_cmp_(x, y, xn);
        if (cmp >= 0) {
            lmmp_sub_(dst, x, xn, y, yn);
        } else {
            lmmp_sub_(dst, y, yn, x, xn);
        }
        rn = xn;
    }
    while (rn > 0 && dst[rn - 1] == 0) {
        --rn;
    }
    return rn;
}

/**
 * @brief    / a \ = / A  B \ * / a \
 *           \ b /   \ C  D /   \ b /
 * @warning [a,an] > [b,bn]
 * @note 不保证返回结果 [a,an] > [b,bn]
 * @return a和b是否有一个为0；返回 true 时保证 [a,an] 非零（若 a 先归零
 *         则将 b 移入 a，gcd 即 a）
 */
static bool lmmp_lehmer_mul_(mp_ptr a, mp_size_t* an, mp_ptr b, mp_size_t* bn, lmmp_gcd_lehmer_t* M, lehmer_stack_t* ms) {
#define A (M->m11)
#define B (M->m12)
#define C (M->m21)
#define D (M->m22)
#define an (*an)
#define bn (*bn)
    if (A == 0) {
        /*     / 0  1 \ / a \   =  /        b        \
               \ 1 -q / \ b /      \ a - q*b (new a) /            */
        lmmp_debug_assert(B == 1 && C == 1 && D < 0);
        mp_limb_t c = lmmp_mul_1_(ms->tp, b, bn, -D);
        mp_size_t tn = bn;
        if (c != 0) {
            ++tn;
            (ms->tp)[tn - 1] = c;
        }
        // a = |a - q*b|, b = b
        an = lmmp_lehmer_sub_mag_(a, a, an, ms->tp, tn);
        if (an == 0) {
            // a - q*b == 0：b | a，gcd = b，移入 a
            lmmp_copy(a, b, bn);
            an = bn;
            b[0] = 0;
            bn = 0;
        }
        return bn == 0;
    } else {
        if (A < 0) {
            A = -A;
            D = -D;
        } else {
            B = -B;
            C = -C;
        }
        // ms->np = |A * a - (-B) * b|（B 取负后系数均为正，差的绝对值即线性组合）
        mp_limb_t ca = lmmp_mul_1_(ms->tp, a, an, A);
        ms->tn = an;
        if (ca != 0)
            (ms->tp)[ms->tn++] = ca;
        ca = lmmp_mul_1_(ms->mp, b, bn, B);
        ms->mn = bn;
        if (ca != 0)
            (ms->mp)[ms->mn++] = ca;
        ms->nn = lmmp_lehmer_sub_mag_(ms->np, ms->tp, ms->tn, ms->mp, ms->mn);

        // a = |C * a - (-D) * b|
        ca = lmmp_mul_1_(ms->tp, a, an, C);
        ms->tn = an;
        if (ca != 0)
            (ms->tp)[ms->tn++] = ca;
        ca = lmmp_mul_1_(ms->mp, b, bn, D);
        ms->mn = bn;
        if (ca != 0)
            (ms->mp)[ms->mn++] = ca;
        an = lmmp_lehmer_sub_mag_(a, ms->tp, ms->tn, ms->mp, ms->mn);

        // now       a = C * a + D * b
        //      ms->np = A * a + B * b
        if (an == 0) {
            // a 归零：gcd = ms->np，移入 a
            lmmp_copy(a, ms->np, ms->nn);
            an = ms->nn;
            b[0] = 0;
            bn = 0;
            return true;
        }
        if (ms->nn > 0) {
            lmmp_copy(b, ms->np, ms->nn);
            bn = ms->nn;
            return false;
        } else {
            b[0] = 0;
            bn = 0;
            return true;
        }
    }
#undef A
#undef B
#undef C
#undef D
#undef an
#undef bn
}

mp_size_t lmmp_gcd_lehmer_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn) {
    lmmp_param_assert(un > 0 && vn > 0);
    lmmp_param_assert(up != NULL && vp != NULL);
    lmmp_param_assert(dst != NULL);

    lmmp_param_assert(up[un - 1] != 0);
    lmmp_param_assert(vp[vn - 1] != 0);

    if (un < vn) {
        LMMP_SWAP(up, vp, mp_srcptr);
        LMMP_SWAP(un, vn, mp_size_t);
    } else if (un == vn) {
        int cmp = lmmp_cmp_(up, vp, un);
        if (cmp == 0) {
            lmmp_copy(dst, up, un);
            return un;
        } else if (cmp < 0) {
            LMMP_SWAP(up, vp, mp_srcptr);
        }
    }
    // u > v

    lmmp_gcd_lehmer_t M;
    slong x = 0, y = 0;

#define an un
#define bn vn
    TEMP_B_DECL;
    // [a,an+1] [b,bn+1]
    // A * a_old may overlow
    mp_ptr a = BALLOC_TYPE(an + 1, mp_limb_t);
    mp_ptr b = BALLOC_TYPE(bn + 1, mp_limb_t);
    lehmer_stack_t ms;
    mp_ptr temp = BALLOC_TYPE((an + 1) * 3, mp_limb_t);
    ms.tp = temp;
    ms.mp = temp + (an + 1);
    ms.np = temp + (an + 1) * 2;

    lmmp_copy(a, up, an);
    lmmp_copy(b, vp, bn);

    bool bzero = false;
    while (bzero == false) {
        if (an > 1 && bn == 1) {
            dst[0] = lmmp_gcd_1_(a, an, b[0]);
            TEMP_B_FREE;
            return 1;
        } else if (an == 1 && bn == 1) {
            dst[0] = lmmp_gcd_11_(a[0], b[0]);
            TEMP_B_FREE;
            return 1;
        }
        // a > b
        lmmp_lehmer_extract_(a, an, b, bn, &x, &y);
        lmmp_gcd_lehmer_step_(x, y, &M);

        if (M.m21 == 0) {
            lmmp_div_(NULL, dst, a, an, b, bn);
            lmmp_copy(a, b, bn);
            an = bn;
            while (bn > 0 && dst[bn - 1] == 0) {
                --bn;
            }
            if (bn == 0)
                bzero = true;
            else 
                lmmp_copy(b, dst, bn);
        } else {
            bzero = lmmp_lehmer_mul_(a, &an, b, &bn, &M, &ms);
            if ((an < bn) || (an == bn && lmmp_cmp_(a, b, an) < 0)) {
                LMMP_SWAP(a, b, mp_ptr);
                LMMP_SWAP(an, bn, mp_size_t);
            }
        }
    }
    lmmp_copy(dst, a, an);
    TEMP_B_FREE;
    return an;
#undef an
#undef bn
}