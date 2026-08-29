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
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed, bool msb = true) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0 && msb) p[n - 1] |= (u64)1 << 63;
}

}  // namespace

TEST_CASE("numth/gcd", gcd_11) {
    u64 seed = 0x6d2b79f5ull;
    for (int i = 0; i < 500; ++i) {
        u64 a = xorshift64(seed) | 1;
        u64 b = xorshift64(seed) | 1;
        BigInt ba(a), bb(b);
        BigInt g = BigInt::gcd_euclid(ba, bb);
        TEST_CHECK_EQ(lmmp_gcd_11_(a, b), g.d[0]);
    }
    TEST_CHECK_EQ(lmmp_gcd_11_(1, 1), 1u);
    TEST_CHECK_EQ(lmmp_gcd_11_(UINT64_MAX, UINT64_MAX), UINT64_MAX);
}

TEST_CASE("numth/gcd", gcd_1) {
    u64 seed = 0x3c6ef372fe94f82bull;
    for (size_t un = 1; un <= 40; ++un) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr u = alloc_limbs(un);
            random_limbs(u, un, seed);
            u64 v = xorshift64(seed) | 1;
            BigInt bu(u, un), bv(v);
            BigInt g = BigInt::gcd_euclid(bu, bv);
            TEST_CHECK_EQ(lmmp_gcd_1_(u, un, v), g.d[0]);
            lmmp_free(u);
        }
    }
}

TEST_CASE("numth/gcd", gcd_22_and_gcd_2) {
    u64 seed = 0x9e3779b97f4a7c15ull;
    for (int iter = 0; iter < 200; ++iter) {
        mp_ptr u = alloc_limbs(2);
        mp_ptr v = alloc_limbs(2);
        mp_ptr dst = alloc_limbs(2);
        random_limbs(u, 2, seed);
        random_limbs(v, 2, seed);
        BigInt bu(u, 2), bv(v, 2);
        BigInt g = BigInt::gcd_euclid(bu, bv);
        mp_size_t gn = lmmp_gcd_22_(dst, u, v);
        TEST_CHECK_MSG(from_limbs(dst, gn) == g, "gcd_22 value");
        lmmp_free(u); lmmp_free(v); lmmp_free(dst);
    }

    for (int iter = 0; iter < 100; ++iter) {
        mp_size_t un = 3 + (iter % 40);
        mp_ptr u = alloc_limbs(un);
        mp_ptr v = alloc_limbs(2);
        mp_ptr dst = alloc_limbs(un);
        random_limbs(u, un, seed);
        random_limbs(v, 2, seed);
        BigInt bu(u, un), bv(v, 2);
        BigInt g = BigInt::gcd_euclid(bu, bv);
        mp_size_t gn = lmmp_gcd_2_(dst, u, un, v);
        TEST_CHECK_MSG(from_limbs(dst, gn) == g, "gcd_2 value");
        lmmp_free(u); lmmp_free(v); lmmp_free(dst);
    }
}

TEST_CASE("numth/gcd", gcd_lehmer_vs_bigint) {
    u64 seed = 0x5ac635d8aa3a93e7ull;
    for (mp_size_t n : {1, 2, 5, 10, 20, 40}) {
        for (int iter = 0; iter < 8; ++iter) {
            mp_size_t un = n + (iter % 5);
            mp_size_t vn = n > 1 ? n : 1;
            if (vn > un) vn = un;
            mp_ptr u = alloc_limbs(un);
            mp_ptr v = alloc_limbs(vn);
            mp_ptr d2 = alloc_limbs(un + vn);
            random_limbs(u, un, seed);
            random_limbs(v, vn, seed);
            BigInt bu(u, un), bv(v, vn);
            BigInt g = BigInt::gcd_euclid(bu, bv);

            mp_size_t g2 = lmmp_gcd_lehmer_(d2, u, un, v, vn);
            TEST_CHECK_MSG(from_limbs(d2, g2) == g, "gcd_lehmer value");
            lmmp_free(u); lmmp_free(v); lmmp_free(d2);
        }
    }
}

TEST_CASE("numth/gcd", gcd_lehmer_large) {
    // 回归：A==0 分支曾用 b 而非 q*b 作减数，且 bn 被乘积进位污染，
    // 未初始化 limb 可能混入结果。大尺寸与悬殊比例都会经过 Lehmer 矩阵路径。
    u64 seed = 0x2545f4914f6cdd1dull;
    struct { mp_size_t un, vn; } sizes[] = {{60, 3}, {120, 7}, {200, 50}, {300, 299}, {400, 2}};
    for (auto& s : sizes) {
        for (int iter = 0; iter < 3; ++iter) {
            mp_ptr u = alloc_limbs(s.un);
            mp_ptr v = alloc_limbs(s.vn);
            mp_ptr d = alloc_limbs(s.vn);
            random_limbs(u, s.un, seed);
            random_limbs(v, s.vn, seed);
            BigInt bu(u, s.un), bv(v, s.vn);

            mp_size_t gn = lmmp_gcd_lehmer_(d, u, s.un, v, s.vn);
            TEST_CHECK_MSG(gn > 0, "gcd positive length");

            // 除源性：u mod d == 0 且 v mod d == 0（余数仅写入 r[0..gn)）
            mp_ptr r = alloc_limbs(gn);
            lmmp_div_(NULL, r, u, s.un, d, gn);
            bool divides_u = true;
            for (mp_size_t i = 0; i < gn; ++i) if (r[i] != 0) divides_u = false;
            TEST_CHECK_MSG(divides_u, "gcd divides u");
            lmmp_div_(NULL, r, v, s.vn, d, gn);
            bool divides_v = true;
            for (mp_size_t i = 0; i < gn; ++i) if (r[i] != 0) divides_v = false;
            TEST_CHECK_MSG(divides_v, "gcd divides v");
            lmmp_free(r);

            lmmp_free(u); lmmp_free(v); lmmp_free(d);
        }
    }
}

namespace {

// 读取 hgcd 矩阵元素为 BigInt（去除零填充）
BigInt hgcd_elem(const lmmp_hgcd_matrix_t* M, int i, int j) {
    return BigInt(M->m[i][j], (size_t)M->n[i][j]);
}

// 检验 [p,n] 是否被 [d,dn] 整除
bool divides_all(mp_srcptr d, mp_size_t dn, mp_srcptr p, mp_size_t pn) {
    if (dn == 0) return false;
    mp_ptr r = (mp_ptr)lmmp_alloc(pn * sizeof(mp_limb_t));
    lmmp_div_(NULL, r, p, pn, d, dn);
    bool ok = true;
    for (mp_size_t i = 0; i < dn; ++i)
        if (r[i] != 0) ok = false;
    lmmp_free(r);
    return ok;
}

}  // namespace

TEST_CASE("numth/gcd", hgcd_matrix_reduction) {
    // hgcd 结构正确性：矩阵关系 (a;b) == M*(a';b')、行列式 ±1、规模折半、规范序。
    // 注意：刻意不使用 lmmp_gcd_lehmer_ 作为参照（其提取函数在长度不等的输入上存在
    // 独立缺陷），gcd 不变性由矩阵关系（幺模变换）数学保证。
    u64 seed = 0x51ce1b7ea4df7c21ull;
    for (mp_size_t n : {3, 4, 5, 8, 16, 31, 32, 33, 50, 64, 100, 200}) {
        for (int bmode = 0; bmode < 4; ++bmode) {
            for (int iter = 0; iter < 3; ++iter) {
                mp_ptr a = alloc_limbs(n);
                mp_ptr b = alloc_limbs(n);
                if (bmode == 3) {
                    // 相邻 Fibonacci 对（商全 1 的最坏情形），长度凑到 n
                    BigInt x(1), y(1);
                    while ((mp_size_t)y.d.size() < n) {
                        BigInt t = BigInt::add_abs(x, y);
                        x = y;
                        y = t;
                    }
                    to_limbs(x, a, n);
                    to_limbs(y, b, n);
                } else {
                    random_limbs(a, n, seed);
                    if (bmode == 0) {
                        random_limbs(b, n, seed);
                        b[n - 1] >>= 1;
                        if (b[n - 1] == 0) b[n - 1] = 1;
                    } else if (bmode == 1) {
                        mp_size_t bn = n / 2 + 1;
                        random_limbs(b, bn, seed);
                        for (mp_size_t i = bn; i < n; ++i) b[i] = 0;
                    } else {
                        b[0] = 0x9e3779b97f4a7c15ull | 1;
                        for (mp_size_t i = 1; i < n; ++i) b[i] = 0;
                    }
                }
                BigInt a0(a, n), b0(b, n);
                if (a0 < b0) {
                    mp_ptr t = a; a = b; b = t;
                    BigInt tb = a0; a0 = b0; b0 = tb;
                }
                if (a0 == b0) continue; // hgcd 契约要求 a > b

                mp_ptr wa = alloc_limbs(n);
                mp_ptr wb = alloc_limbs(n);
                lmmp_copy(wa, a, n);
                lmmp_copy(wb, b, n);
                lmmp_hgcd_matrix_t M;
                lmmp_hgcd_matrix_init_(&M, n + 2);
                mp_size_t nn = lmmp_hgcd_(&M, wa, wb, n);
                if (nn > 0) {
                    BigInt ap(wa, (size_t)nn), bp(wb, (size_t)nn);
                    BigInt m00 = hgcd_elem(&M, 0, 0), m01 = hgcd_elem(&M, 0, 1);
                    BigInt m10 = hgcd_elem(&M, 1, 0), m11 = hgcd_elem(&M, 1, 1);
                    BigInt r0 = BigInt::mul_school(m00, ap);
                    r0 = BigInt::add_abs(r0, BigInt::mul_school(m01, bp));
                    BigInt r1 = BigInt::mul_school(m10, ap);
                    r1 = BigInt::add_abs(r1, BigInt::mul_school(m11, bp));
                    TEST_CHECK_MSG(r0 == a0, "hgcd row0 relation");
                    TEST_CHECK_MSG(r1 == b0, "hgcd row1 relation");
                    // det = m00*m11 - m01*m10 == ±1（用无符号加减比较实现）
                    BigInt p0 = BigInt::mul_school(m00, m11), p1 = BigInt::mul_school(m01, m10);
                    TEST_CHECK_MSG(p0 == BigInt::add_abs(p1, BigInt(1)) || p1 == BigInt::add_abs(p0, BigInt(1)),
                                   "hgcd det == +-1");
                    TEST_CHECK_MSG(ap >= bp, "hgcd canonical order");
                    TEST_CHECK_MSG(nn <= n / 2 + 2, "hgcd size halved");
                } else {
                    // 无归约合法（如下限阻止），此时数对应未被破坏
                    TEST_CHECK_MSG(from_limbs(wa, n) == a0 && from_limbs(wb, n) == b0, "hgcd no-reduction keeps pair");
                }
                lmmp_hgcd_matrix_free_(&M);
                lmmp_free(a); lmmp_free(b); lmmp_free(wa); lmmp_free(wb);
            }
        }
    }
}

TEST_CASE("numth/gcd", gcd_hgcd_correctness) {
    // gcd_hgcd 正确性：小尺寸对照 BigInt 欧几里得参照；大尺寸用整除性与已知构造验证
    u64 seed = 0x2b44ae9d4d6c8f13ull;
    for (mp_size_t n : {1, 2, 3, 5, 8, 16, 33, 64, 120, 200}) {
        for (int mode = 0; mode < 5; ++mode) {
            for (int iter = 0; iter < 3; ++iter) {
                mp_ptr a = alloc_limbs(n);
                mp_ptr b = alloc_limbs(n);
                mp_size_t bn = n;
                BigInt expect;
                bool have_expect = false;
                if (mode == 0) {  // 随机同长
                    random_limbs(a, n, seed);
                    random_limbs(b, n, seed);
                    if (n > 1) b[n - 1] >>= 1;
                    if (b[n - 1] == 0) b[n - 1] = 1;
                } else if (mode == 1) {  // b = 1
                    random_limbs(a, n, seed);
                    b[0] = 1;
                    for (mp_size_t i = 1; i < n; ++i) b[i] = 0;
                    bn = 1;
                    expect = BigInt(1);
                    have_expect = true;
                } else if (mode == 2) {  // a = b+1
                    random_limbs(b, n, seed);
                    lmmp_copy(a, b, n);
                    mp_limb_t carry = 1;
                    for (mp_size_t i = 0; i < n && carry; ++i) {
                        mp_limb_t t = a[i] + carry;
                        carry = t < carry;
                        a[i] = t;
                    }
                    expect = BigInt(1);
                    have_expect = true;
                } else if (mode == 3) {  // a = k*b（gcd 恰为 b）
                    random_limbs(b, n, seed);
                    b[0] |= 1;
                    BigInt bb(b, n);
                    u64 k = 3 + (xorshift64(seed) & 0xff);
                    BigInt aa = BigInt::mul_school(bb, BigInt(k));
                    lmmp_free(a);
                    a = alloc_limbs((mp_size_t)aa.d.size());
                    to_limbs(aa, a, (mp_size_t)aa.d.size());
                    expect = bb;
                    have_expect = true;
                    n = (mp_size_t)aa.d.size();  // 本轮以 a 的长度为准
                } else {  // 相邻 Fibonacci（gcd = 1）
                    BigInt x(1), y(1);
                    while ((mp_size_t)y.d.size() < n || (mp_size_t)x.d.size() < n) {
                        BigInt t = BigInt::add_abs(x, y);
                        x = y;
                        y = t;
                    }
                    if (x < y) { BigInt t = x; x = y; y = t; }
                    lmmp_free(a);
                    lmmp_free(b);
                    a = alloc_limbs((mp_size_t)x.d.size());
                    to_limbs(x, a, (mp_size_t)x.d.size());
                    b = alloc_limbs((mp_size_t)y.d.size());
                    to_limbs(y, b, (mp_size_t)y.d.size());
                    n = (mp_size_t)x.d.size();
                    bn = (mp_size_t)y.d.size();
                    expect = BigInt(1);
                    have_expect = true;
                }

                BigInt ba(a, n), bb(b, bn);
                if (ba < bb) {
                    mp_ptr t = a; a = b; b = t;
                    BigInt tb = ba; ba = bb; bb = tb;
                    mp_size_t tn = n; n = bn; bn = tn;
                }
                if (ba == bb) {
                    lmmp_free(a); lmmp_free(b);
                    continue;
                }

                if (n <= 16 && !have_expect) {
                    expect = BigInt::gcd_euclid(ba, bb);
                    have_expect = true;
                }

                mp_ptr g = alloc_limbs(n);
                mp_size_t gn = lmmp_gcd_hgcd_(g, a, n, b, bn);
                BigInt got(g, (size_t)gn);
                if (have_expect) {
                    TEST_CHECK_MSG(got == expect, "gcd_hgcd known result");
                }
                // 整除性（所有尺寸）
                TEST_CHECK_MSG(divides_all(g, gn, a, n), "gcd divides a");
                TEST_CHECK_MSG(divides_all(g, gn, b, bn), "gcd divides b");
                lmmp_free(g);
                lmmp_free(a);
                lmmp_free(b);
            }
        }
    }
}
