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

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

using namespace lmmp_test_utils;

/*
    模逆元验证（无自建算法 oracle）：
      1. 标量档：64 位 % 欧几里得判互素 + inv*u mod m == 1（mullh 全积
         + mod_1 取余）+ inv < m + m=1/不可逆写死语义
      2. 多 limb：互素性经库 gcd_ 判定；逆元经库乘除重建 inv*u mod m == 1
      3. dst 与 u 别名（eqsep 契约）稳定性
*/

namespace {

u64 gcd_u64(u64 a, u64 b) {
    while (b) {
        u64 t = a % b;
        a = b;
        b = t;
    }
    return a;
}

}  // namespace

TEST_CASE("numth/modinv", modinv_ulong) {
    // 穷举小值（u*inv mod m 经 mod_1 判模）
    for (u64 m = 1; m <= 200; ++m) {
        for (u64 u = 1; u <= 200; ++u) {
            ulong inv = lmmp_modinv_ulong_(u, m);
            if (m == 1) {
                TEST_CHECK_MSG(inv == 0, "m=1 inverse is 0");
                continue;
            }
            u64 r = u % m;
            ulong g = (r == 0) ? m : gcd_u64(r, m);
            if (g != 1) {
                TEST_CHECK_MSG(inv == 0, "non-invertible");
            } else {
                mp_limb_t prod[2];
                lmmp_mullh_(u, inv, prod);
                TEST_CHECK_MSG(lmmp_mod_1_(prod, 2, m) == 1 && inv < m, "inverse");
            }
        }
    }
    // 随机 64 位
    u64 seed = 0x51ce1b7ea4df7c21ull;
    for (int i = 0; i < 20000; ++i) {
        u64 m = xorshift64(seed);
        u64 u = xorshift64(seed);
        if (m == 0) m = 1;
        if (u == 0) u = 1;
        ulong inv = lmmp_modinv_ulong_(u, m);
        u64 r = u % m;
        ulong g = (r == 0) ? m : gcd_u64(r, m);
        if (m == 1) {
            TEST_CHECK_MSG(inv == 0, "m=1");
        } else if (g != 1) {
            TEST_CHECK_MSG(inv == 0, "non-invertible random");
        } else {
            mp_limb_t prod[2];
            lmmp_mullh_(u, inv, prod);
            TEST_CHECK_MSG(lmmp_mod_1_(prod, 2, m) == 1 && inv < m, "inverse random");
        }
    }
}

TEST_CASE("numth/modinv", modinv_big) {
    u64 seed = 0x2b44ae9d4d6c8f13ull;
    for (mp_size_t mn : {2, 3, 5, 9, 17, 33, 60}) {
        for (int iter = 0; iter < 7; ++iter) {
            mp_ptr m = alloc_limbs(mn);
            random_limbs(m, mn, seed);
            m[0] |= 1;
            mp_ptr u;
            mp_size_t un;
            if (iter == 6) {
                // un < mn：u 直接取短随机数（覆盖 modinv 短分支）
                un = mn / 2 > 0 ? mn / 2 : 1;
                u = alloc_limbs(un);
                random_limbs(u, un, seed);
                u[0] |= 1;
            } else if (iter % 2 == 0) {
                // u = m + 1（与 m 互素）
                un = mn;
                u = alloc_limbs(mn + 1);
                lmmp_copy(u, m, mn);
                lmmp_zero(u + mn, 1);
                add_small_inplace(u, mn + 1, 1);
                un = norm_len(u, mn + 1);
            } else {
                un = mn;
                u = alloc_limbs(un);
                random_limbs(u, un, seed);
                u[0] |= 1;
            }
            mp_ptr dst = alloc_limbs(mn);
            mp_size_t rn = lmmp_modinv_(dst, u, un, m, mn);
            // 互素性与逆元验证（库交叉）：inv*u mod m == 1
            mp_ptr gr = alloc_limbs(un > mn ? un : mn);
            mp_size_t gn = lmmp_gcd_(gr, u, un, m, mn);
            if (gn == 1 && gr[0] == 1) {
                TEST_CHECK_MSG(rn > 0, "inverse exists");
                mp_ptr prod = alloc_limbs(mn + un + 1), q = alloc_limbs(mn + un + 1),
                       r = alloc_limbs(mn);
                if (rn >= un) lmmp_mul_(prod, dst, rn, u, un);
                else lmmp_mul_(prod, u, un, dst, rn);
                lmmp_div_(q, r, prod, rn + un, m, mn);
                TEST_CHECK_MSG(norm_len(r, mn) == 1 && r[0] == 1, "inv*u mod m == 1");
                TEST_CHECK_MSG(mpn_cmp(dst, rn, m, mn) < 0, "inverse in range");
                lmmp_free(prod); lmmp_free(q); lmmp_free(r);
            } else {
                TEST_CHECK_MSG(rn == 0, "non-invertible reported");
            }
            lmmp_free(gr);
            // dst 与 u 别名（eqsep 契约）
            mp_ptr u2 = alloc_limbs(un > mn ? un : mn);
            lmmp_copy(u2, u, un);
            mp_size_t rn2 = lmmp_modinv_(u2, u2, un, m, mn);
            TEST_CHECK_MSG(rn2 == rn, "alias stable");
            lmmp_free(m); lmmp_free(u); lmmp_free(dst); lmmp_free(u2);
        }
    }
}
