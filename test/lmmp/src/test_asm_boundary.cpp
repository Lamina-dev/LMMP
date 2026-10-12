/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "lmmp/lmmpn.h"
#include "lmmp/impl/mparam.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstdio>
#include <cstring>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

}  // namespace

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

TEST_CASE("asm/boundary", mul_1_addmul_submul_extremes) {
    const mp_size_t n = 6;
    mp_ptr a = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) a[i] = UINT64_MAX;

    BigInt ba(a, n);
    u64 x = UINT64_MAX;
    BigInt prod = BigInt::mul_school(ba, BigInt(x));
    mp_limb_t cy = lmmp_mul_1_(d, a, n, x);
    TEST_CHECK(limb_vec_eq(BigInt(prod.d.data(), n), d, n));
    TEST_CHECK_EQ(cy, prod.d[n]);

    mp_ptr acc = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) acc[i] = UINT64_MAX;
    acc[n] = 0;
    BigInt bacc(acc, n);
    BigInt sum = BigInt::add_abs(bacc, prod);
    u64 c2 = lmmp_addmul_1_(acc, a, n, x);
    TEST_CHECK(limb_vec_eq(BigInt(sum.d.data(), sum.d.size() > n ? n : sum.d.size()), acc, n));
    TEST_CHECK_EQ(c2, sum.d.size() > n ? sum.d[n] : 0u);

    // submul_1：构造足够大的被加数（借位版）仅检查返回 limb 关系
    BigInt accbig(acc, n);
    mp_ptr acc2 = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) acc2[i] = acc[i];
    acc2[n] = 0;
    u64 c3 = lmmp_submul_1_(acc2, a, n, x);
    BigInt prod_low;
    size_t pl = prod.d.size() < n ? prod.d.size() : n;
    prod_low.d.assign(pl, 0);
    for (size_t i = 0; i < pl; ++i) prod_low.d[i] = prod.d[i];
    prod_low.trim();
    u64 expect_borrow = (prod.d.size() > n ? prod.d[n] : 0) + (accbig < prod_low ? 1u : 0u);
    TEST_CHECK_EQ(c3, expect_borrow);

    lmmp_free(a); lmmp_free(d); lmmp_free(acc); lmmp_free(acc2);
}

TEST_CASE("asm/boundary", shl_shr_inplace) {
    u64 seed = 0x1234567890abcdeful;
    const mp_size_t n = 16;
    mp_ptr a = alloc_limbs(n);
    mp_ptr orig = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) a[i] = xorshift64(seed);
    BigInt ba(a, n);
    std::memcpy(orig, a, n * 8);

    for (size_t s : {1, 7, 31, 63}) {
        std::memcpy(a, orig, n * 8);
        BigInt expected = BigInt::shl_bits(ba, s);
        mp_limb_t ret = lmmp_shl_(a, a, n, s);  // 原地左移
        BigInt got = from_limbs(a, n);
        (void)ret;
        TEST_CHECK_MSG(got == BigInt(expected.d.data(), expected.d.size() > n ? n : expected.d.size()),
                       "inplace shl body");

        std::memcpy(a, orig, n * 8);
        BigInt expected_shr = BigInt::shr_bits(ba, s);
        ret = lmmp_shr_(a, a, n, s);  // 原地右移
        got = from_limbs(a, n);
        (void)ret;
        TEST_CHECK_MSG(got == expected_shr, "inplace shr body");
    }

    lmmp_free(a); lmmp_free(orig);
}

namespace {

// 错位重叠族公共骨架：buf 内 [G, G+n) 为 numa，dst=numa+w（w=100 表示分离块
// sep）。每次调用前重填随机源并快照，调用后校验 [dst,dst+n) 外哨兵不变。
struct overlap_buf_t {
    mp_ptr buf, snap, sep, numa;
    size_t cap, g, n;

    overlap_buf_t(mp_size_t n_, size_t g_)
        : cap((size_t)n_ + 2 * g_), g(g_), n((size_t)n_) {
        buf = alloc_limbs(cap);
        snap = alloc_limbs(cap);
        sep = alloc_limbs(n);
        numa = buf + g;
    }
    ~overlap_buf_t() {
        lmmp_free(buf); lmmp_free(snap); lmmp_free(sep);
    }

    void refill(const BigInt& ba, int w) {
        for (size_t i = 0; i < cap; ++i) buf[i] = 0xA5A5DEADBEEF00F0ull ^ (i * 0x9e37u + (u64)w * 7u);
        for (size_t i = 0; i < n; ++i) numa[i] = ba.d[i];
        std::memcpy(snap, buf, cap * 8);
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
    // w 为 dst-numa 的 limb 偏移。
    u64 seed = 0xfeedbeefcafe1234ul;
    for (mp_size_t n = 1; n <= 24; ++n) {
        overlap_buf_t ob(n, 4);
        for (mp_size_t i = 0; i < n; ++i) ob.numa[i] = xorshift64(seed);
        ob.numa[n - 1] |= 0x8000000000000000ull;  // 防 BigInt trim 掉顶 limb
        BigInt ba(ob.numa, n);

        for (size_t k = 0; k < 64; ++k) {
            BigInt eshl = BigInt::shl_bits(ba, k);
            size_t shl_len = eshl.d.size() < (size_t)n ? eshl.d.size() : (size_t)n;
            BigInt eshr = BigInt::shr_bits(ba, k);
            mp_limb_t ret_shl = k ? (ba.d[n - 1] >> (64 - k)) : 0;
            mp_limb_t ret_shr = k ? (ba.d[0] << (64 - k)) : 0;

            const int ws[] = {100, 0, -1, -2, -3};
            for (int w : ws) {
                ob.refill(ba, w);
                mp_limb_t r = lmmp_shl_(ob.dst_of(w), ob.numa, n, k);
                TEST_CHECK_EQ(r, ret_shl);
                TEST_CHECK_MSG(limb_vec_eq(BigInt(eshl.d.data(), shl_len), ob.dst_of(w), n), "shl body");
                ob.check_guard(w, "shl overlap family guard");

                ob.refill(ba, w);
                r = lmmp_shr_(ob.dst_of(w), ob.numa, n, k);
                TEST_CHECK_EQ(r, ret_shr);
                TEST_CHECK_MSG(limb_vec_eq(eshr, ob.dst_of(w), n), "shr body");
                ob.check_guard(w, "shr overlap family guard");
            }
        }
    }
}

TEST_CASE("asm/boundary", shl_shr_c_overlap_family) {
    // _c 变体的错位重叠族（c 满足契约：shl_c 的 c 高 64-k 位为 0，
    // shr_c 的 c 低 64-k 位为 0）
    u64 seed = 0xc0ffee1234567890ul;
    for (mp_size_t n = 2; n <= 12; ++n) {
        overlap_buf_t ob(n, 4);
        for (mp_size_t i = 0; i < n; ++i) ob.numa[i] = xorshift64(seed);
        ob.numa[n - 1] |= 0x8000000000000000ull;  // 防 BigInt trim 掉顶 limb
        BigInt ba(ob.numa, n);

        for (size_t k : {1, 5, 32, 63}) {
            mp_limb_t cshl = xorshift64(seed) & (((mp_limb_t)1 << k) - 1);
            mp_limb_t cshr = (xorshift64(seed) >> (64 - k)) << (64 - k);  // 仅保留顶 k 位
            BigInt eshl = BigInt::add_abs(BigInt::shl_bits(ba, k), BigInt(cshl));
            size_t shl_len = eshl.d.size() < (size_t)n ? eshl.d.size() : (size_t)n;
            BigInt eshr = BigInt::shr_bits(ba, k);
            eshr.d[n - 1] |= cshr;
            mp_limb_t ret_shl = ba.d[n - 1] >> (64 - k);
            mp_limb_t ret_shr = ba.d[0] << (64 - k);

            const int ws[] = {100, 0, -1, -2, -3};
            for (int w : ws) {
                ob.refill(ba, w);
                mp_limb_t r = lmmp_shl_c_(ob.dst_of(w), ob.numa, n, k, cshl);
                TEST_CHECK_EQ(r, ret_shl);
                TEST_CHECK_MSG(limb_vec_eq(BigInt(eshl.d.data(), shl_len), ob.dst_of(w), n), "shl_c body");
                ob.check_guard(w, "shl_c overlap family guard");

                ob.refill(ba, w);
                r = lmmp_shr_c_(ob.dst_of(w), ob.numa, n, k, cshr);
                TEST_CHECK_EQ(r, ret_shr);
                TEST_CHECK_MSG(limb_vec_eq(eshr, ob.dst_of(w), n), "shr_c body");
                ob.check_guard(w, "shr_c overlap family guard");
            }
        }
    }
}

TEST_CASE("asm/boundary", div_1_by_one_and_power_of_two) {
    u64 seed = 0xf00dface12345678ull;
    for (mp_size_t n : {1, 2, 5, 16}) {
        mp_ptr a = alloc_limbs(n);
        mp_ptr q = alloc_limbs(n + 1);
        for (mp_size_t i = 0; i < n; ++i) a[i] = xorshift64(seed);
        BigInt ba(a, n);

        // x == 1：商等于原数，余数 0
        std::memcpy(q, a, n * 8);
        u64 r = lmmp_div_1_(q, q, n, 1);
        TEST_CHECK_EQ(r, 0u);
        TEST_CHECK(from_limbs(q, n) == ba);

        // x 为 2 的幂：直接与参考比较
        u64 x = 1ull << (seed & 63 ? (seed & 63) : 1);
        x = (x == 1) ? 2 : x;
        u64 ref_r = BigInt::mod_small(ba, x);
        BigInt ref_q = BigInt::div_small(ba, x, ref_r);
        std::memcpy(q, a, n * 8);
        r = lmmp_div_1_(q, q, n, x);
        TEST_CHECK_EQ(r, ref_r);
        TEST_CHECK(from_limbs(q, n) == ref_q);

        lmmp_free(a); lmmp_free(q);
    }
}

TEST_CASE("asm/boundary", inv_and_mulmod_boundary) {
    u64 seed = 0x0123456789abcdeful;
    const u128 max128(UINT64_MAX, UINT64_MAX);  // 2^128 - 1
    const u128 B = u128(1) << 64;
    for (int i = 0; i < 200; ++i) {
        u64 x = xorshift64(seed) | LIMB_B_2;
        u64 inv = lmmp_inv_1_(x);
        u128 q = (u128)inv + B;
        u128 prod = q * x;
        TEST_CHECK_MSG(prod <= max128 && prod > max128 - x, "inv_1 boundary");

        u64 mod = xorshift64(seed) | 3;
        u64 a = xorshift64(seed) % mod;
        u64 b = xorshift64(seed) % mod;
        u64 qq = 0;
        u64 r = lmmp_mulmod_ulong_(a, b, mod, &qq);
        TEST_CHECK_MSG((u128)qq * mod + r == (u128)a * b, "mulmod boundary relation");
    }
}
