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

#include <cstring>

using namespace lmmp_test_utils;

namespace {

// 验证 [dst,n] * [num,na] ≡ 1 (mod B^n)：库乘法取低 n limbs 应为 1 后随全零
void verify_inverse(const mp_limb_t* num, mp_size_t na, const mp_limb_t* dst, mp_size_t n) {
    mp_ptr t = alloc_limbs(n + na + 1);
    if (na == 1) {
        lmmp_mul_1_(t, dst, n, num[0]);
    } else {
        lmmp_mul_(t, dst, n, num, na);  // 契约 na_dst=n >= na
    }
    TEST_CHECK_EQ(t[0], 1ull);
    for (mp_size_t i = 1; i < n; ++i) TEST_CHECK_EQ(t[i], 0ull);
    lmmp_free(t);
}

}  // namespace

TEST_CASE("numth/binvert", binvert_uint_ulong) {
    u64 seed = 0x123456789abcdef0ull;
    for (int i = 0; i < 500; ++i) {
        u32 a = (u32)(xorshift64(seed) | 1);
        u32 inv = lmmp_binvert_uint_(a);
        TEST_CHECK_MSG((uint)(a * inv) == 1u, "binvert_uint inverse");

        u64 b = xorshift64(seed) | 1;
        u64 inv64 = lmmp_binvert_ulong_(b);
        mp_limb_t iv[2];
        lmmp_mullh_(b, inv64, iv);
        TEST_CHECK_EQ(iv[0], 1ull);  // b*inv64 ≡ 1 (mod B)：低 limb 为 1 即可
    }
}

TEST_CASE("numth/binvert", binvert_2_3_4) {
    u64 seed = 0x1020304050607080ull;
    for (int i = 0; i < 300; ++i) {
        mp_limb_t a2[2], inv2[2];
        random_limbs(a2, 2, seed);
        a2[0] |= 1;
        lmmp_binvert_2_(inv2, a2);
        verify_inverse(a2, 2, inv2, 2);

        mp_limb_t a3[3], inv3[3];
        random_limbs(a3, 3, seed);
        a3[0] |= 1;
        lmmp_binvert_3_(inv3, a3);
        verify_inverse(a3, 3, inv3, 3);

        mp_limb_t a4[4], inv4[4];
        random_limbs(a4, 4, seed);
        a4[0] |= 1;
        lmmp_binvert_4_(inv4, a4);
        verify_inverse(a4, 4, inv4, 4);
    }
}

TEST_CASE("numth/binvert", binvert_n_dc) {
    u64 seed = 0x0f1e2d3c4b5a6978ull;
    for (mp_size_t n : {1, 2, 3, 4, 5, 10, 20, 40, 80, 120}) {
        mp_ptr a = alloc_limbs(n), inv = alloc_limbs(n), tp = alloc_limbs((5 * n + 5) / 2);
        random_limbs(a, n, seed);
        a[0] |= 1;
        lmmp_binvert_n_dc_(inv, a, n, tp);
        verify_inverse(a, n, inv, n);
        lmmp_free(a); lmmp_free(inv); lmmp_free(tp);
    }
}

TEST_CASE("numth/binvert", binvert_near_balanced) {
    // 回归：4n < 5na 的平衡化分支曾以长度 n 拷贝 numa（实际只有 na 个 limb）而越界读
    u64 seed = 0xc0ffee1234567890ull;
    for (mp_size_t na : {8, 40, 80}) {
        mp_size_t n = na + na / 5;
        mp_ptr a = alloc_limbs(na), inv = alloc_limbs(n);
        random_limbs(a, na, seed);
        a[0] |= 1;
        lmmp_binvert_(inv, a, na, n);
        verify_inverse(a, na, inv, n);
        lmmp_free(a);
        lmmp_free(inv);
    }
}

TEST_CASE("numth/binvert", bninv_power2_top_limb) {
    // 回归：顶 limb 为 2 的幂时，归一化后顶 limb 恰为 2^63，曾触发
    // appr_newton 的严格大于断言失败（发布版为 assume UB）
    u64 seed = 0x9d1f2e3c4b5a6978ull;
    for (mp_size_t na : {3, 8, 40}) {
        for (u64 top : {1ull, 4ull, 1ull << 32, 1ull << 63}) {
            for (mp_size_t ni0 : {na, na / 2}) {
                mp_size_t ni = ni0 < 1 ? 1 : ni0;
                mp_ptr a = alloc_limbs(na);
                mp_ptr q = alloc_limbs(na + ni + 2);
                random_limbs(a, na, seed);
                a[0] |= 1;
                a[na - 1] = top;
                lmmp_bninv_(q, a, na, ni);

                // 参照（库内构造）：ref = B^(2*(na+ni)) / (numa * B^ni)，
                // 文档误差 +[0|1]，放宽到 2
                mp_size_t dn = na + ni;
                mp_ptr den = alloc_limbs(dn);
                lmmp_zero(den, dn);
                memcpy(den + ni, a, (size_t)na * 8);  // numa * B^ni
                mp_size_t nn = 2 * dn + 1;
                mp_ptr numer = alloc_limbs(nn);
                lmmp_zero(numer, nn);
                numer[2 * dn] = 1;  // B^(2*(na+ni))
                mp_ptr quot = alloc_limbs(nn - dn + 1), rem = alloc_limbs(dn);
                lmmp_div_(quot, rem, numer, nn, den, dn);

                // |got - ref| <= 2
                mp_size_t ln = dn + 1;
                mp_ptr diff = alloc_limbs(ln + 1);
                mp_limb_t bw = lmmp_sub_(diff, q, ln, quot, ln);
                bool small = true;
                if (bw) {  // got < ref，反向相减
                    lmmp_sub_(diff, quot, ln, q, ln);
                }
                for (mp_size_t i = 1; i < ln; ++i) small = small && (diff[i] == 0);
                TEST_CHECK_MSG(small && diff[0] <= 2, "bninv close to reference quotient");

                lmmp_free(a); lmmp_free(q); lmmp_free(den); lmmp_free(numer);
                lmmp_free(quot); lmmp_free(rem); lmmp_free(diff);
            }
        }
    }
}

TEST_CASE("numth/binvert", binvert_unbalanced_variants) {
    u64 seed = 0xa1b2c3d4e5f60718ull;

    // unbalance_1：单 limb 底数
    for (mp_size_t n : {2, 3, 5, 10, 30, 80}) {
        mp_ptr a = alloc_limbs(1), inv = alloc_limbs(n);
        a[0] = xorshift64(seed) | 1;
        lmmp_binvert_unbalance_1_(inv, a[0], n);
        verify_inverse(a, 1, inv, n);
        lmmp_free(a); lmmp_free(inv);
    }

    // unbalance_2：双 limb 底数
    for (mp_size_t n : {3, 4, 10, 30, 80}) {
        mp_ptr a = alloc_limbs(2), inv = alloc_limbs(n);
        random_limbs(a, 2, seed);
        a[0] |= 1;
        lmmp_binvert_unbalance_2_(inv, a, n);
        verify_inverse(a, 2, inv, n);
        lmmp_free(a); lmmp_free(inv);
    }

    // unbalance 和通用入口
    for (mp_size_t n : {3, 5, 10, 30, 80}) {
        mp_size_t na = (n + 1) / 2;
        if (na < 1) na = 1;
        if (na >= n) na = n - 1;
        mp_ptr a = alloc_limbs(na), inv = alloc_limbs(n), tp = alloc_limbs((9 * na + 5) / 2 + 1);
        random_limbs(a, na, seed);
        a[0] |= 1;
        lmmp_binvert_unbalance_(inv, a, na, n, tp);
        verify_inverse(a, na, inv, n);

        mp_ptr inv2 = alloc_limbs(n);
        lmmp_binvert_(inv2, a, na, n);
        verify_inverse(a, na, inv2, n);
        lmmp_free(a); lmmp_free(inv); lmmp_free(inv2); lmmp_free(tp);
    }
}
