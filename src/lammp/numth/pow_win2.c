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

#include "../../../include/lammp/impl/inlines.h"
#include "../../../include/lammp/impl/tmp_alloc.h"
#include "../../../include/lammp/lmmpn.h"
#include "../../../include/lammp/numth.h"


#define mul_b(_i_)                                     \
    if (rsq) {                                         \
        if (b##_i_##n >= rn)                           \
            lmmp_mul_(dst, b##_i_, b##_i_##n, sq, rn); \
        else                                           \
            lmmp_mul_(dst, sq, rn, b##_i_, b##_i_##n); \
        rn += b##_i_##n;                               \
        rn -= (dst[rn - 1] == 0) ? 1 : 0;              \
        rsq = false;                                   \
    } else {                                           \
        if (b##_i_##n >= rn)                           \
            lmmp_mul_(sq, b##_i_, b##_i_##n, dst, rn); \
        else                                           \
            lmmp_mul_(sq, dst, rn, b##_i_, b##_i_##n); \
        rn += b##_i_##n;                               \
        rn -= (sq[rn - 1] == 0) ? 1 : 0;               \
        rsq = true;                                    \
    }

mp_size_t lmmp_pow_win2_(mp_ptr restrict dst, mp_size_t rn, mp_srcptr restrict base, mp_size_t n, ulong exp) {
    lmmp_param_assert(exp > 0);
    lmmp_param_assert(n > 0);
    lmmp_param_assert(dst != NULL);
    TEMP_DECL;

#define b1 base
#define b1n n
#define new_b(_i_) mp_ptr restrict b##_i_ = TALLOC_TYPE(b##_i_##n, mp_limb_t)

    mp_size_t b2n = n << 1, b3n = b2n + n;
    new_b(2);
    new_b(3);

    lmmp_sqr_(b2, b1, b1n);
    b2n = b1n << 1;
    b2n -= b2[b2n - 1] == 0 ? 1 : 0;
    lmmp_mul_(b3, b2, b2n, b1, b1n);
    b3n = b2n + b1n;
    b3n -= b3[b3n - 1] == 0 ? 1 : 0;

    bool rsq = true;
    mp_ptr restrict sq = BALLOC_TYPE(rn, mp_limb_t);
    rn = 1;
    int i = 31, j;
    while ((exp & (0x3ull << ((i--) * 2))) == 0);

    /*
    模拟一次，让最后一次计算结果恰好是dst，避免最后一次可能的拷贝
    */
    for (j = i + 1; j != -1; --j) {
        int p = (exp & (0x3ull << (j * 2))) >> (j * 2);
        if (p != 0)
            rsq = !rsq;
    }
    if (rsq) {
        dst[0] = 1;
        rsq = false;
    } else {
        sq[0] = 1;
        rsq = true;
    }

    for (++i; i != -1; --i) {
        int p = (exp & (0x3ull << (i * 2))) >> (i * 2);
        if (p == 0) {
            ;
        } else if (p == 1) {
            mul_b(1)
        } else if (p == 2) {
            mul_b(2)
        } else {
            mul_b(3)
        }

        if (i > 0) {
            if (rsq) {
                lmmp_sqr_(dst, sq, rn);
                rn <<= 1;
                rn -= (dst[rn - 1] == 0) ? 1 : 0;
                lmmp_sqr_(sq, dst, rn);
                rn <<= 1;
                rn -= (sq[rn - 1] == 0) ? 1 : 0;
            } else {
                lmmp_sqr_(sq, dst, rn);
                rn <<= 1;
                rn -= (sq[rn - 1] == 0) ? 1 : 0;
                lmmp_sqr_(dst, sq, rn);
                rn <<= 1;
                rn -= (dst[rn - 1] == 0) ? 1 : 0;
            }
        }
    }
    lmmp_debug_assert(rsq == false);

    TEMP_FREE;
    return rn;

#undef b1
#undef b1n
#undef new_b
}