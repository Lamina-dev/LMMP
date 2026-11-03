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

/*
    gcd 标量与小尺寸入口验证（铁律：无自建算法 oracle）：
      1. 标量参考：gcd_11/gcd_1 用 64 位 % 欧几里得标量循环 + 写死锚点
      2. 库内交叉：gcd_22/gcd_2（特化核）与 gcd_lehmer_（通用 Lehmer 路径，
         bn>=2 时不经特化核）同输入比对
      3. 写死形状锚点：低 limb 为 0 的移位组合给出精确已知 gcd（B 的倍数）
      4. 恒等式：整除性（g|u、g|v）、u=k*v 时 gcd 恰为 v、gcd(u,0)=u
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

// 构造 (x << s) 的双 limb 表示，s ∈ [0,127]
void shifted_pair(u64 x, int s, mp_ptr out) {
    if (s < 64) {
        out[0] = x << s;
        out[1] = (s == 0) ? 0 : (x >> (64 - s));
    } else {
        out[0] = 0;
        out[1] = x << (s - 64);
    }
}

}  // namespace

TEST_CASE("numth/gcd", gcd_11) {
    u64 seed = 0x6d2b79f5ull;
    for (int i = 0; i < 500; ++i) {
        u64 a = xorshift64(seed) | 1;
        u64 b = xorshift64(seed) | 1;
        TEST_CHECK_EQ(lmmp_gcd_11_(a, b), gcd_u64(a, b));
    }
    TEST_CHECK_EQ(lmmp_gcd_11_(1, 1), 1u);
    TEST_CHECK_EQ(lmmp_gcd_11_(UINT64_MAX, UINT64_MAX), UINT64_MAX);
    // 写死锚点：gcd(2^63, 2^62)=2^62；gcd(2^64-1, 2^63)=1（费马数互素）
    TEST_CHECK_EQ(lmmp_gcd_11_((u64)1 << 63, (u64)1 << 62), (u64)1 << 62);
    TEST_CHECK_EQ(lmmp_gcd_11_(UINT64_MAX, (u64)1 << 63), 1u);
}

TEST_CASE("numth/gcd", gcd_1) {
    u64 seed = 0x3c6ef372fe94f82bull;
    for (size_t un = 1; un <= 40; ++un) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr u = alloc_limbs(un);
            random_limbs(u, un, seed);
            u64 v = xorshift64(seed) | 1;
            // 恒等式一步：gcd(u, v) == gcd(u mod v, v)（mod_1 求余后标量参考）
            u64 r = lmmp_mod_1_(u, un, v);
            TEST_CHECK_EQ(lmmp_gcd_1_(u, un, v), gcd_u64(r, v));
            // 整除性
            TEST_CHECK_EQ(lmmp_mod_1_(u, un, lmmp_gcd_1_(u, un, v)), 0ull);
            lmmp_free(u);
        }
    }
}

TEST_CASE("numth/gcd", gcd_22_and_gcd_2) {
    u64 seed = 0x9e3779b97f4a7c15ull;
    mp_ptr u = alloc_limbs(2), v = alloc_limbs(2), dst = alloc_limbs(2), gr = alloc_limbs(2);
    // gcd_22：与 gcd_lehmer_（通用 Lehmer 路径）交叉 + 整除性
    for (int iter = 0; iter < 200; ++iter) {
        random_limbs(u, 2, seed);
        random_limbs(v, 2, seed);
        mp_size_t gn = lmmp_gcd_22_(dst, u, v);
        mp_size_t ln = lmmp_gcd_lehmer_(gr, u, 2, v, 2);
        TEST_CHECK(mpn_eq(dst, gn, gr, ln));
        TEST_CHECK_MSG(divides_all(dst, gn, u, 2), "gcd_22 divides u");
        TEST_CHECK_MSG(divides_all(dst, gn, v, 2), "gcd_22 divides v");
    }
    // gcd_2：多 limb u 对 2 limb v —— 与 gcd_lehmer_ 交叉（Lehmer 原生处理不平衡）
    for (int iter = 0; iter < 100; ++iter) {
        mp_size_t un = 3 + (iter % 40);
        mp_ptr ub = alloc_limbs(un);
        random_limbs(ub, un, seed);
        random_limbs(v, 2, seed);
        mp_size_t gn = lmmp_gcd_2_(dst, ub, un, v);
        mp_size_t ln = lmmp_gcd_lehmer_(gr, ub, un, v, 2);
        TEST_CHECK(mpn_eq(dst, gn, gr, ln));
        TEST_CHECK_MSG(divides_all(dst, gn, ub, un), "gcd_2 divides u");
        lmmp_free(ub);
    }
    lmmp_free(u); lmmp_free(v); lmmp_free(dst); lmmp_free(gr);
}

TEST_CASE("numth/gcd", gcd_22_shapes) {
    // 覆盖单/双 limb 与低位为 0 的形态组合。强制高位的随机测试几乎不可能
    // 命中这些分支（单个 limb 为 0 的概率约 2^-64），gcd_22 曾因此在
    // u0==v0==0 的路径上漏检（漏乘 B），此用例系统性遍历移位组合。
    // 期望 = gcd(x,y)*2^min(a,b)（x,y 奇）。构造约束：x,y < 2^63 且
    // s <= 64，保证 x*2^s < 2^127 不溢出 128 位（溢出截断会使期望式失效）
    u64 seed = 0x51ce1b7ea4df7c21ull;
    mp_ptr u = alloc_limbs(2), v = alloc_limbs(2), d = alloc_limbs(2), e = alloc_limbs(2);
    for (int a = 0; a <= 64; ++a) {
        for (int b = 0; b <= 64; b += 4) {
            u64 x = (xorshift64(seed) & (((u64)1 << 63) - 1)) | 1;  // 奇且 < 2^63
            u64 y = (xorshift64(seed) & (((u64)1 << 63) - 1)) | 1;
            shifted_pair(x, a, u);
            shifted_pair(y, b, v);
            mp_size_t gn = lmmp_gcd_22_(d, u, v);
            // 期望 = gcd(x,y)*2^min(a,b)（标量 gcd + 移位装配，精确已知）
            int m = a < b ? a : b;
            u64 g = gcd_u64(x, y);
            shifted_pair(g, m, e);
            mp_size_t en = norm_len(e, 2);
            TEST_CHECK(mpn_eq(d, gn, e, en));
        }
    }
    // 确定性用例：写死精确期望（B 的倍数 / 单 limb / 相等 / 混合形态）
    struct { u64 u0, u1, v0, v1; mp_size_t n; u64 w0, w1; } fixed[] = {
        {0, 1, 0, 3, 2, 0, 1},          // gcd(B, 3B) = B
        {0, 9, 0, 6, 2, 0, 3},          // gcd(9B, 6B) = 3B
        {0, 2, 0, 6, 2, 0, 2},          // gcd(2B, 6B) = 2B
        {0, 1, 0, 1ull << 63, 2, 0, 1}, // gcd(B, 2^63*B) = B
        {1, 0, 3, 0, 1, 1, 0},          // 双方单 limb：gcd(1,3)=1
        {12, 0, 18, 0, 1, 6, 0},        // gcd(12,18)=6
        {0, 1, 5, 0, 1, 1, 0},          // gcd(B,5)=1
        {7, 9, 7, 9, 2, 7, 9},          // 相等
        {10, 2, 0, 1, 1, 2, 0},         // gcd(2B+10, B) = gcd(10, B) = 2
        {0, 1, 0, 1, 2, 0, 1},          // 双方相等且均为 B
    };
    for (const auto& t : fixed) {
        u64 pu[2] = {t.u0, t.u1}, pv[2] = {t.v0, t.v1};
        mp_size_t gn = lmmp_gcd_22_(d, pu, pv);
        TEST_CHECK_MSG(gn == t.n && d[0] == t.w0 && d[1] == t.w1, "gcd_22 fixed shape");
    }
    lmmp_free(u); lmmp_free(v); lmmp_free(d); lmmp_free(e);
}

TEST_CASE("numth/gcd", gcd_2_multiple_and_gcd_1_zero) {
    // gcd_2：u 为 v 的整数倍 → 余数为 0 → 直接返回 v（u = k*v 经库 mul_1 构造）
    u64 seed = 0x2b44ae9d4d6c8f13ull;
    for (int iter = 0; iter < 20; ++iter) {
        mp_ptr v = alloc_limbs(2);
        random_limbs(v, 2, seed);
        u64 k = 3 + (xorshift64(seed) & 0xf);
        mp_ptr up = alloc_limbs(4);
        u64 hi = lmmp_mul_1_(up, v, 2, k);  // u = k*v
        mp_size_t un = hi ? 3 : 2;
        if (hi) up[2] = hi;
        mp_ptr d = alloc_limbs(2);
        mp_size_t gn = lmmp_gcd_2_(d, up, un, v);
        TEST_CHECK(mpn_eq(d, gn, v, 2));
        lmmp_free(v); lmmp_free(up); lmmp_free(d);
    }

    // gcd_1：[up,un] == 0 → gcd(0,v) = v（契约允许 u 为 0）
    mp_ptr z = alloc_limbs(5);
    lmmp_zero(z, 5);
    TEST_CHECK_EQ(lmmp_gcd_1_(z, 5, 7), 7u);
    TEST_CHECK_EQ(lmmp_gcd_1_(z, 1, 7), 7u);
    TEST_CHECK_EQ(lmmp_gcd_1_(z + 1, 4, 12), 12u);
    lmmp_free(z);
}

TEST_CASE("numth/gcd", gcd_empty) {
    // gcd(B^k, B^k) = B^k：lehmer/hgcd 两条路径的顶 limb 传播
    for (mp_size_t n : {2, 3, 5, 8, 16, 53, 64, 120, 200, 321}) {
        mp_ptr a = alloc_limbs(n);
        mp_ptr b = alloc_limbs(n);
        mp_ptr c = alloc_limbs(n);
        mp_ptr d = alloc_limbs(n);

        lmmp_zero(a, n - 1);
        lmmp_zero(b, n - 1);
        a[n - 1] = 1;
        b[n - 1] = 1;
        mp_size_t cn = lmmp_gcd_lehmer_(c, a, n, b, n);
        TEST_CHECK_MSG(cn == n && c[n - 1] == 1, "lehmer gcd high limb is one");
        for (mp_size_t i = 0; i < cn - 1; i++) {
            TEST_CHECK_MSG(c[i] == 0, "lehmer gcd low limbs is zero");
        }

        mp_size_t dn = lmmp_gcd_hgcd_(d, a, n, b, n);
        TEST_CHECK_MSG(dn == n && d[n - 1] == 1, "hgcd gcd is equal to B^n");
        for (mp_size_t i = 0; i < dn - 1; i++) {
            TEST_CHECK_MSG(d[i] == 0, "hgcd gcd low limbs is zero");
        }
        lmmp_free(a); lmmp_free(b); lmmp_free(c); lmmp_free(d);
    }
}
