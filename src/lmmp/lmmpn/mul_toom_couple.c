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

#include "../../../include/lmmp/impl/toom_interp.h"

/*
   输入 {pp,n} 与 (sign ? -{np,n} : {np,n})，一次完成：
       {pp,n} <- ({pp,n}+{np,n})/2^(ps+1)
       {np,n} <- ({pp,n}-{np,n})/2^(ns+1)
   最后按 off 拼回：
       {pp,n+off} <- {pp,n} + {np,n}*B^off

   toom6h/8h 各 ± 点对的两个 (n+1) 位乘积经本函数混合为插值输入：
   和半边落 pp（供偶系数），差半边落 np 顶部（供奇系数）。 */

void lmmp_toom_couple_handling_(mp_ptr pp, mp_size_t n, mp_ptr np, int nsign, mp_size_t off, int ps, int ns) {
    mp_limb_t cy;

    lmmp_param_assert(n > 0);
    lmmp_param_assert(off < n);
    lmmp_param_assert(ps >= 0 && ns >= 0);

    if (nsign)
        lmmp_shr1sub_n_(np, pp, np, n);
    else
        lmmp_shr1add_n_(np, pp, np, n);

    if (ps == 1)
        lmmp_shr1sub_n_(pp, pp, np, n);
    else {
        lmmp_sub_n_(pp, pp, np, n);
        if (ps > 0)
            lmmp_shr_(pp, pp, n, ps);
    }
    if (ns > 0)
        lmmp_shr_(np, np, n, ns);

    pp[n] = lmmp_add_n_(pp + off, pp + off, np, n - off);
    cy = lmmp_add_1_(pp + n, np + n - off, off, pp[n]);
    lmmp_debug_assert(cy == 0);
}
