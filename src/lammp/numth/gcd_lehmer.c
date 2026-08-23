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

#include "../../../include/lammp/lmmpn.h"
#include "../../../include/lammp/numth.h"
#include "../../../include/lammp/impl/tmp_alloc.h"

typedef struct {
    slong m11, m12;
    slong m21, m22;
} mp_gcd_lehmer_t;

#define LEHMER_MIN_V 0x100000000ll
#define LEHMER_EXACT_BITS 63

static void lmmp_gcd_lehmer_step_(slong u, slong v, mp_gcd_lehmer_t* gcd) {
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
 * @brief    / a \ = / A  B \ * / a \
 *           \ b /   \ C  D /   \ b /            
 * @warning [a,an] > [b,bn]
 * @note 不保证返回结果 [a,an] > [b,bn]
 * @return a和b是否有一个为0
 */
static bool lmmp_lehmer_mul_(mp_ptr a, mp_size_t* an, mp_ptr b, mp_size_t* bn, mp_gcd_lehmer_t* M, lehmer_stack_t* ms) {
#define A (M->m11)
#define B (M->m12)
#define C (M->m21)
#define D (M->m22)
#define an (*an)
#define bn (*bn)
    if (A == 0) {
        /*     / 0  1 \ / a \
               \ 1 -q / \ b /            */
        lmmp_debug_assert(B == 1 && C == 1 && D < 0);
        mp_limb_t c = lmmp_mul_1_(ms->tp, b, bn, -D);
        if (c != 0) {
            ++bn;
            (ms->tp)[bn - 1] = c;
        }
        if (an > bn) {
            lmmp_sub_(a, a, an, b, bn);
        } else if (an == bn) {
            int cmp = lmmp_cmp_(a, b, an);
            if (cmp >= 0) 
                lmmp_sub_(a, a, an, b, bn);
            else
                lmmp_sub_(a, b, bn, a, an);
        } else {
            lmmp_sub_(a, b, bn, a, an);
            an = bn;
        }
        while (a[an - 1] == 0 && an > 0) {
            --an;
        }
        // return  b = b
        //         a = a - q * b
        return an == 0;
    } else {
        if (A < 0) {
            A = -A;
            D = -D;
        } else {
            B = -B;
            C = -C;
        }
        // A * a + B * b
        mp_limb_t ca = lmmp_mul_1_(ms->tp, a, an, A);
        if (ca == 0)
            ms->tn = an;
        else {
            ms->tn = an + 1;
            (ms->tp)[ms->tn - 1] = ca;
        }
        ca = lmmp_mul_1_(ms->mp, b, bn, B);
        if (ca == 0)
            ms->mn = bn;
        else {
            ms->mn = bn + 1;
            (ms->mp)[ms->mn - 1] = ca;
        }

        if (ms->tn > ms->mn) {
            lmmp_sub_(ms->np, ms->tp, ms->tn, ms->mp, ms->mn);
            ms->nn = ms->tn;
        } else if (ms->mn > ms->tn) {
            lmmp_sub_(ms->np, ms->mp, ms->mn, ms->tp, ms->tn);
            ms->nn = ms->mn;
        } else {
            int cmp = lmmp_cmp_(ms->tp, ms->mp, ms->tn);
            if (cmp >= 0) {
                lmmp_sub_(ms->np, ms->tp, ms->tn, ms->mp, ms->mn);
                ms->nn = ms->tn;
            } else {
                lmmp_sub_(ms->np, ms->mp, ms->mn, ms->tp, ms->tn);
                ms->nn = ms->mn;
            }
        }
        while (ms->np[ms->nn - 1] == 0 && ms->nn > 0) {
            --(ms->nn);
        }

        // C * a + D * b
        ca = lmmp_mul_1_(ms->tp, a, an, C);
        if (ca == 0)
            ms->tn = an;
        else {
            ms->tn = an + 1;
            (ms->tp)[ms->tn - 1] = ca;
        }
        ca = lmmp_mul_1_(ms->mp, b, bn, D);
        if (ca == 0)
            ms->mn = bn;
        else {
            ms->mn = bn + 1;
            (ms->mp)[ms->mn - 1] = ca;
        }

        if (ms->tn > ms->mn) {
            lmmp_sub_(a, ms->tp, ms->tn, ms->mp, ms->mn);
            an = ms->tn;
        } else if (ms->mn > ms->tn) {
            lmmp_sub_(a, ms->mp, ms->mn, ms->tp, ms->tn);
            an = ms->mn;
        } else {
            int cmp = lmmp_cmp_(ms->tp, ms->mp, ms->tn);
            if (cmp >= 0) {
                lmmp_sub_(a, ms->tp, ms->tn, ms->mp, ms->mn);
                an = ms->tn;
            } else {
                lmmp_sub_(a, ms->mp, ms->mn, ms->tp, ms->tn);
                an = ms->mn;
            }
        }
        while (a[an - 1] == 0 && an > 0) {
            --an;
        }

        // now       a = C * a + D * b
        //      ms->np = A * a + B * b
        if (ms->nn > 0) {
            lmmp_copy(b, ms->np, ms->nn);
            bn = ms->nn;
            return an == 0;
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

    mp_gcd_lehmer_t M;
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
            return 1;
        } else if (an == 1 && bn == 1) {
            dst[0] = lmmp_gcd_11_(a[0], b[0]);
            return 1;
        } 
        // a > b
        lmmp_lehmer_extract_(a, an, b, bn, &x, &y);
        lmmp_gcd_lehmer_step_(x, y, &M);

        if (M.m21 == 0) {
            lmmp_div_(NULL, dst, a, an, b, bn);
            lmmp_copy(a, b, bn);
            an = bn;
            while (dst[bn - 1] == 0 && bn > 0) {
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