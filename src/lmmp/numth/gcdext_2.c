/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

/*
扩展欧几里得（n limb x 2 limb 层）。

经 lmmp_mod_2_ 归约至 r = u mod v 后转 lmmp_gcdext_lehmer_ 求 g 与 s。
s 对 (u, v) 直接有效（s*u = s*r (mod v)，极小界 |s| <= v/(2g) 不变），但
归约对的 t' 不适用于原对（差 s*(u-r)/v）——t 须按恒等式导出：

    t = (g - s*u) / v

经一次乘法（|s| x un）、一次符号-幅度加减与一次 2 limb 除法精确得到
（整除性由恒等式保证；|t| <= u/(2g) <= un 个limb）。
*/

mp_size_t lmmp_gcdext_2_(mp_ptr dstg, mp_ptr dsts, mp_ssize_t* sn, mp_ptr dstt, mp_ssize_t* tn,
                         mp_srcptr up, mp_size_t un, mp_srcptr vp) {
    lmmp_param_assert(un > 2);
    lmmp_param_assert(up != NULL && vp != NULL && dstg != NULL && dsts != NULL && sn != NULL);
    lmmp_param_assert(vp[1] != 0);

    mp_limb_t r[2] = {vp[0], vp[1]};
    lmmp_mod_2_(up, un, r);
    if (r[0] == 0 && r[1] == 0) {
        // v | u：g = v，s = 0、t = 1
        lmmp_copy(dstg, vp, 2);
        dsts[0] = 0;
        *sn = 0;
        if (dstt != NULL) {
            dstt[0] = 1;
            *tn = 1;
        }
        return 2;
    }
    mp_size_t rn = (r[1] != 0) ? 2 : 1;

    if (dstt == NULL)
        return lmmp_gcdext_lehmer_(dstg, dsts, sn, NULL, NULL, r, rn, vp, 2);

    TEMP_B_DECL;
    mp_ptr gb = BALLOC_TYPE(3, mp_limb_t);
    mp_ptr sb = BALLOC_TYPE(3, mp_limb_t);
    mp_ptr x = BALLOC_TYPE(un + 2, mp_limb_t);
    mp_ptr nb = BALLOC_TYPE(un + 2, mp_limb_t);
    mp_ptr tq = BALLOC_TYPE(un + 2, mp_limb_t);
    mp_ssize_t snn;
    mp_size_t gn = lmmp_gcdext_lehmer_(gb, sb, &snn, NULL, NULL, r, rn, vp, 2);
    mp_size_t snabs = snn < 0 ? -snn : snn;
    lmmp_debug_assert(snabs > 0 && snabs <= 2); // r>0 且 v∤r → s≠0，|s| <= v/(2g) <= 2 limb

    /* t = (g - s*u)/v：X = |s|*u；s>0 时 N = -(X-g)（X >= u > g），s<0 时 N = X+g */
    if (snabs >= un)
        lmmp_mul_(x, sb, snabs, up, un);
    else
        lmmp_mul_(x, up, un, sb, snabs);
    mp_size_t xn = snabs + un;
    while (xn > 0 && x[xn - 1] == 0)
        --xn;
    mp_size_t nn;
    int tneg = 0;
    if (snn > 0) {
        lmmp_sub_(nb, x, xn, gb, gn);
        nn = xn;
        while (nn > 0 && nb[nn - 1] == 0)
            --nn;
        tneg = nn != 0;
    } else {
        mp_limb_t cy = lmmp_add_(nb, x, xn, gb, gn);
        nn = xn;
        if (cy != 0)
            nb[nn++] = cy;
    }
    mp_size_t tqn = 0;
    if (nn != 0) {
        lmmp_div_(tq, NULL, nb, nn, vp, 2);
        /* 商至多 nn-2+1 个limb（div 契约），高位未写区域不可读 */
        tqn = nn - 1;
        while (tqn > 0 && tq[tqn - 1] == 0)
            --tqn;
    }

    lmmp_copy(dstg, gb, gn);
    lmmp_copy(dsts, sb, snabs);
    *sn = snn;
    lmmp_copy(dstt, tq, tqn);
    if (tqn == 0)
        dstt[0] = 0;
    *tn = tneg ? -(mp_ssize_t)tqn : (mp_ssize_t)tqn;
    TEMP_B_FREE;
    return gn;
}
