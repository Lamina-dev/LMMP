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
#include "lmmp/impl/mparam.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

TEST_CASE("asm/boundary", add_nc_sub_nc_all_ones) {
    const mp_size_t n = 8;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) {
        a[i] = UINT64_MAX;
        b[i] = 0;
    }
    b[0] = 1;

    mp_limb_t cy = lmmp_add_nc_(d, a, b, n, 1);
    TEST_CHECK_EQ(cy, 1u);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(d[i], (i == 0) ? 1u : 0u);

    // a - a - borrow = all ones, borrow 1
    for (mp_size_t i = 0; i < n; ++i) a[i] = UINT64_MAX;
    mp_limb_t bw = lmmp_sub_nc_(d, a, a, n, 1);
    TEST_CHECK_EQ(bw, 1u);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(d[i], UINT64_MAX);

    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

/*
    全 F 极值乘法写死常数：(B^n-1)*(B-1) = (B-1)(B^n-1)，limbs =
    [1, B-1, ..., B-1, B-2]；addmul/submul 用组合路径复核。
*/
TEST_CASE("asm/boundary", mul_1_addmul_submul_extremes) {
    const mp_size_t n = 6;
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1), t = alloc_limbs(n + 2),
           acc = alloc_limbs(n + 1), acc2 = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) a[i] = UINT64_MAX;
    u64 x = UINT64_MAX;

    mp_limb_t cy = lmmp_mul_1_(d, a, n, x);
    TEST_CHECK_EQ(d[0], 1ull);
    for (mp_size_t i = 1; i < n; ++i) TEST_CHECK_EQ(d[i], UINT64_MAX);
    TEST_CHECK_EQ(cy, UINT64_MAX - 1);

    // addmul：acc(F..F) + prod == prod<<.. 组合路径
    for (mp_size_t i = 0; i < n; ++i) acc[i] = UINT64_MAX;
    acc[n] = 0;
    u64 c1 = lmmp_add_n_(t, acc, d, n) + cy;  // 进位=加进位+乘进位
    memcpy(acc2, acc, n * 8);
    acc2[n] = 0;
    u64 c2 = lmmp_addmul_1_(acc2, a, n, x);
    TEST_CHECK_EQ(memcmp(acc2, t, n * 8), 0);
    TEST_CHECK_EQ(c2, c1);

    // submul 组合路径：borrow = w_sub + hi
    u64 w = lmmp_sub_n_(t, acc, d, n);
    memcpy(acc2, acc, n * 8);
    acc2[n] = 0;
    u64 c3 = lmmp_submul_1_(acc2, a, n, x);
    TEST_CHECK_EQ(memcmp(acc2, t, n * 8), 0);
    TEST_CHECK_EQ(c3, w + cy);

    lmmp_free(a); lmmp_free(d); lmmp_free(t); lmmp_free(acc); lmmp_free(acc2);
}

TEST_CASE("asm/boundary", shl_shr_inplace) {
    u64 seed = 0x1234567890abcdeful;
    const mp_size_t n = 16;
    mp_ptr a = alloc_limbs(n), orig = alloc_limbs(n), ref = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) a[i] = xorshift64(seed);
    memcpy(orig, a, n * 8);

    // 原地调用与分离缓冲调用同结果（库内交叉；每次迭代重置源）
    for (size_t s : {1, 7, 31, 63}) {
        memcpy(a, orig, n * 8);
        mp_limb_t re = lmmp_shl_(ref, orig, n, s);
        mp_limb_t r1 = lmmp_shl_(a, a, n, s);
        TEST_CHECK_EQ(r1, re);
        TEST_CHECK_EQ(memcmp(a, ref, n * 8), 0);

        memcpy(a, orig, n * 8);
        re = lmmp_shr_(ref, orig, n, s);
        r1 = lmmp_shr_(a, a, n, s);
        TEST_CHECK_EQ(r1, re);
        TEST_CHECK_EQ(memcmp(a, ref, n * 8), 0);
    }

    lmmp_free(a); lmmp_free(orig); lmmp_free(ref);
}

namespace {

// 错位重叠族公共骨架：buf 内 [G, G+n) 为 numa，dst=numa+w（w=100 表示分离块
// sep）。每次调用前重填随机源并快照，调用后校验 [dst,dst+n) 外哨兵不变。
// 期望值由分离缓冲调用（契约内路径）交叉给出。
struct overlap_buf_t {
    mp_ptr buf, snap, sep, numa, src;
    size_t cap, g, n;

    overlap_buf_t(mp_size_t n_, size_t g_)
        : cap((size_t)n_ + 2 * g_), g(g_), n((size_t)n_) {
        buf = alloc_limbs(cap);
        snap = alloc_limbs(cap);
        sep = alloc_limbs(n);
        src = alloc_limbs(n);
        numa = buf + g;
    }
    ~overlap_buf_t() {
        lmmp_free(buf); lmmp_free(snap); lmmp_free(sep); lmmp_free(src);
    }

    void set_src() {
        u64 s = 0x9e3779b97f4a7c15ull ^ (n * 31 + g);
        for (size_t i = 0; i < n; ++i) src[i] = xorshift64(s);
        src[n - 1] |= (u64)1 << 63;
    }
    void refill(int w) {
        for (size_t i = 0; i < cap; ++i) buf[i] = 0xA5A5DEADBEEF00F0ull ^ (i * 0x9e37u + (u64)w * 7u);
        memcpy(numa, src, n * 8);
        memcpy(snap, buf, cap * 8);
    }
    mp_ptr dst_of(int w) { return (w == 100) ? sep : numa + w; }
    void check_guard(int w, const char* tag) {
        mp_ptr d = dst_of(w);
        for (size_t i = 0; i < cap; ++i) {
            if (buf + i >= d && buf + i < d + n) continue;
            if (buf[i] != snap[i]) {
                TEST_CHECK_MSG(false, tag);
                return;
            }
        }
    }
};

}  // namespace

TEST_CASE("asm/boundary", shl_shr_overlap_family) {
    // 第五轮契约：shl 仅 eqsep（w 非 0/分离即契约外，不做断言）；shr 文档化
    // 允许 dst<=numa（升序天然支持，nthroot 依赖），w>0 家族为契约外。
    // 期望值取分离缓冲调用；返回值由输入 limbs 直接推出。
    for (mp_size_t n = 1; n <= 24; ++n) {
        overlap_buf_t ob(n, 4);
        ob.set_src();

        for (size_t k = 0; k < 64; ++k) {
            mp_ptr eshl = alloc_limbs(n), eshr = alloc_limbs(n);
            mp_limb_t ret_shl = k ? (ob.src[n - 1] >> (64 - k)) : 0;
            mp_limb_t ret_shr = k ? (ob.src[0] << (64 - k)) : 0;
            lmmp_shl_(eshl, ob.src, n, k);
            lmmp_shr_(eshr, ob.src, n, k);

            const int ws_shl[] = {100, 0};  // shl 纯 eqsep：部分重叠族契约外，不做断言
            for (int w : ws_shl) {
                ob.refill(w);
                mp_limb_t r = lmmp_shl_(ob.dst_of(w), ob.numa, n, k);
                TEST_CHECK_EQ(r, ret_shl);
                TEST_CHECK_EQ(memcmp(ob.dst_of(w), eshl, n * 8), 0);
                ob.check_guard(w, "shl overlap family guard");
            }
            const int ws_shr[] = {100, 0, -1, -2, -3};
            for (int w : ws_shr) {
                ob.refill(w);
                mp_limb_t r = lmmp_shr_(ob.dst_of(w), ob.numa, n, k);
                TEST_CHECK_EQ(r, ret_shr);
                TEST_CHECK_EQ(memcmp(ob.dst_of(w), eshr, n * 8), 0);
                ob.check_guard(w, "shr overlap family guard");
            }
            lmmp_free(eshl);
            lmmp_free(eshr);
        }
    }
}

TEST_CASE("asm/boundary", shl_shr_c_overlap_family) {
    // _c 变体的错位重叠族（c 满足契约：shl_c 的 c 高 64-k 位为 0，
    // shr_c 的 c 低 64-k 位为 0）；期望 = 分离调用 + 手工并位。
    u64 seed = 0xc0ffee1234567890ul;
    for (mp_size_t n = 2; n <= 12; ++n) {
        overlap_buf_t ob(n, 4);
        ob.set_src();

        for (size_t k : {1, 5, 32, 63}) {
            mp_limb_t cshl = xorshift64(seed) & (((mp_limb_t)1 << k) - 1);
            mp_limb_t cshr = (xorshift64(seed) >> (64 - k)) << (64 - k);
            mp_limb_t ret_shl = ob.src[n - 1] >> (64 - k);
            mp_limb_t ret_shr = ob.src[0] << (64 - k);

            mp_ptr eshl = alloc_limbs(n), eshr = alloc_limbs(n);
            lmmp_shl_(eshl, ob.src, n, k);
            eshl[0] |= cshl;
            lmmp_shr_(eshr, ob.src, n, k);
            eshr[n - 1] |= cshr;

            const int ws_shl[] = {100, 0};  // shl_c 纯 eqsep，同 shl_
            for (int w : ws_shl) {
                ob.refill(w);
                mp_limb_t r = lmmp_shl_c_(ob.dst_of(w), ob.numa, n, k, cshl);
                TEST_CHECK_EQ(r, ret_shl);
                TEST_CHECK_EQ(memcmp(ob.dst_of(w), eshl, n * 8), 0);
                ob.check_guard(w, "shl_c overlap family guard");
            }
            const int ws_shr[] = {100, 0, -1, -2, -3};
            for (int w : ws_shr) {
                ob.refill(w);
                mp_limb_t r = lmmp_shr_c_(ob.dst_of(w), ob.numa, n, k, cshr);
                TEST_CHECK_EQ(r, ret_shr);
                TEST_CHECK_EQ(memcmp(ob.dst_of(w), eshr, n * 8), 0);
                ob.check_guard(w, "shr_c overlap family guard");
            }
            lmmp_free(eshl);
            lmmp_free(eshr);
        }
    }
}

TEST_CASE("asm/boundary", div_1_by_one_and_power_of_two) {
    u64 seed = 0xf00dface12345678ull;
    for (mp_size_t n : {1, 2, 5, 16}) {
        mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), t = alloc_limbs(n + 2);
        for (mp_size_t i = 0; i < n; ++i) a[i] = xorshift64(seed);

        // x == 1：商等于原数，余数 0
        memcpy(q, a, n * 8);
        u64 r = lmmp_div_1_(q, q, n, 1);
        TEST_CHECK_EQ(r, 0u);
        TEST_CHECK_EQ(memcmp(q, a, n * 8), 0);

        // x 为 2 的幂：重建 q*x + r == a
        u64 x = 1ull << ((seed & 63) ? (seed & 63) : 1);
        memcpy(q, a, n * 8);
        r = lmmp_div_1_(q, q, n, x);
        TEST_CHECK(r < x);
        u64 hi = lmmp_mul_1_(t, q, n, x);
        t[n] = hi;
        u64 ovf = add_small_inplace(t, n + 1, r);
        (void)ovf;
        TEST_CHECK(mpn_eq(t, n + 1, a, n));

        lmmp_free(a); lmmp_free(q); lmmp_free(t);
    }
}

/*
    inv_1 界恒等式（积经 mul_1 双 limb 折算）；mulmod_ulong 与
    mullh + div_1 路径交叉：qq*mod + r == a*b ⟺ 商余一致。
*/
TEST_CASE("asm/boundary", inv_and_mulmod_boundary) {
    u64 seed = 0x0123456789abcdeful;
    for (int i = 0; i < 200; ++i) {
        u64 x = xorshift64(seed) | LIMB_B_2;
        u64 inv = lmmp_inv_1_(x);
        mp_limb_t prod[2];
        u64 hi = lmmp_mul_1_(prod, &x, 1, inv);
        prod[1] = hi;
        u64 p_hi = prod[1] + x;  // x*(B+inv) 的高半（无溢出）
        TEST_CHECK_EQ(p_hi, ~(u64)0);
        TEST_CHECK(prod[0] >= (u64)(0 - x));

        u64 mod = xorshift64(seed) | 3;
        u64 a = xorshift64(seed) % mod;
        u64 b = xorshift64(seed) % mod;
        u64 qq = 0;
        u64 r = lmmp_mulmod_ulong_(a, b, mod, &qq);
        mp_limb_t ab[2], q2[3];
        lmmp_mullh_(a, b, ab);
        mp_limb_t abw[2] = {ab[0], ab[1]};
        u64 r2 = lmmp_div_1_(q2, abw, 2, mod);
        TEST_CHECK_EQ(r2, r);
        TEST_CHECK_EQ(q2[0], qq);
        TEST_CHECK_EQ(norm_len(q2 + 1, 1), (mp_size_t)0);
    }
}
