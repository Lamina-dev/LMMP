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
    Lehmer / hgcd / 分发入口验证（铁律：无自建算法 oracle，库除法欧几里得
    参考已删）。验证网：
      1. 精确锚点：gcd(fib(m), fib(n)) == fib(gcd(m,n))（fib 库构造）、
         b=1 / a=b+1 / 相邻 fib → gcd=1、a=k*b → gcd=b、u=v*B^d → gcd=v
      2. 恒等式：乘法消去律 gcd(k*u, k*v) == k*gcd(u,v)、整除性 g|u、g|v
      3. 库内交叉：lehmer（矩阵路径）× hgcd（分治归约）× 分发器三路径
         同输入一致——三条路径的归约序列互不相同，错一则失一致
      4. hgcd 结构：矩阵行关系 (a;b)==M*(a';b')、det==±1、规范序
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

// 读取 hgcd 矩阵元素（去零填充）的有效长度
mp_size_t hgcd_len(const lmmp_hgcd_matrix_t* M, int i, int j) { return M->n[i][j]; }

}  // namespace

TEST_CASE("numth/gcd", gcd_lehmer_identities) {
    u64 seed = 0x5ac635d8aa3a93e7ull;

    // 小尺寸随机：lehmer × hgcd 双路径交叉 + 整除性
    for (mp_size_t n : {1, 2, 5, 10, 20, 40}) {
        for (int iter = 0; iter < 8; ++iter) {
            mp_size_t un = n + (iter % 5);
            mp_size_t vn = n > 1 ? n : 1;
            if (vn > un) vn = un;
            mp_ptr u = alloc_limbs(un), v = alloc_limbs(vn), d2 = alloc_limbs(un),
                   d3 = alloc_limbs(un);
            random_limbs(u, un, seed);
            random_limbs(v, vn, seed);
            mp_size_t g2 = lmmp_gcd_lehmer_(d2, u, un, v, vn);
            mp_size_t g3 = lmmp_gcd_hgcd_(d3, u, un, v, vn);
            TEST_CHECK(mpn_eq(d2, g2, d3, g3));
            TEST_CHECK_MSG(divides_all(d2, g2, u, un), "lehmer divides u");
            TEST_CHECK_MSG(divides_all(d2, g2, v, vn), "lehmer divides v");
            lmmp_free(u); lmmp_free(v); lmmp_free(d2); lmmp_free(d3);
        }
    }

    // 斐波那契恒等式：gcd(fib(m), fib(n)) == fib(gcd(m,n))（大尺寸精确锚点）。
    // 长度一律取 fibonacci_ 返回的实际长度（size_ 含 2-4 limb 冗余，缓冲
    // 上方是未初始化内存，不得作为输入长度）；dst 容量取 min(rm,rn)。
    for (ulong m : {240ul, 601ul, 1500ul}) {
        for (ulong n : {90ul, 240ul, 977ul}) {
            if (m == n) continue;
            mp_size_t rm = lmmp_fibonacci_size_(m), rn_ = lmmp_fibonacci_size_(n);
            mp_size_t rg = lmmp_fibonacci_size_(gcd_u64(m, n));
            mp_ptr fm = alloc_limbs(rm), fn = alloc_limbs(rn_),
                   g = alloc_limbs(rn_ < rm ? rn_ : rm), fg = alloc_limbs(rg);
            mp_size_t lm = lmmp_fibonacci_(fm, rm, m);
            mp_size_t ln = lmmp_fibonacci_(fn, rn_, n);
            mp_size_t lg = lmmp_fibonacci_(fg, rg, gcd_u64(m, n));
            mp_size_t gn = lmmp_gcd_lehmer_(g, fm, lm, fn, ln);
            TEST_CHECK(mpn_eq(g, gn, fg, lg));
            lmmp_free(fm); lmmp_free(fn); lmmp_free(g); lmmp_free(fg);
        }
    }

    // 乘法消去律：gcd(k*u, k*v) == k*gcd(u,v)
    for (mp_size_t n : {7, 33, 120}) {
        mp_ptr u = alloc_limbs(n), v = alloc_limbs(n), g = alloc_limbs(n), ku = alloc_limbs(n + 1),
               kv = alloc_limbs(n + 1), kg = alloc_limbs(n + 1), t = alloc_limbs(n + 2);
        random_limbs(u, n, seed);
        random_limbs(v, n, seed);
        mp_size_t gn = lmmp_gcd_lehmer_(g, u, n, v, n);
        u64 k = xorshift64(seed) | 1;
        u64 h1 = lmmp_mul_1_(ku, u, n, k);
        u64 h2 = lmmp_mul_1_(kv, v, n, k);
        u64 h3 = lmmp_mul_1_(kg, g, gn, k);
        mp_size_t kun = h1 ? n + 1 : n, kvn = h2 ? n + 1 : n;
        if (h1) ku[n] = h1;
        if (h2) kv[n] = h2;
        mp_size_t kgn = h3 ? gn + 1 : gn;
        if (h3) kg[gn] = h3;
        mp_size_t g2 = lmmp_gcd_lehmer_(t, ku, kun, kv, kvn);
        TEST_CHECK(mpn_eq(t, g2, kg, kgn));
        lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(ku); lmmp_free(kv);
        lmmp_free(kg); lmmp_free(t);
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
            mp_size_t gn = lmmp_gcd_lehmer_(d, u, s.un, v, s.vn);
            TEST_CHECK_MSG(gn > 0, "gcd positive length");
            TEST_CHECK_MSG(divides_all(d, gn, u, s.un), "gcd divides u");
            TEST_CHECK_MSG(divides_all(d, gn, v, s.vn), "gcd divides v");
            // 与 hgcd 入口交叉一致
            mp_ptr d2 = alloc_limbs(s.vn);
            mp_size_t g2 = lmmp_gcd_hgcd_(d2, u, s.un, v, s.vn);
            TEST_CHECK(mpn_eq(d, gn, d2, g2));
            lmmp_free(u); lmmp_free(v); lmmp_free(d); lmmp_free(d2);
        }
    }
}

TEST_CASE("numth/gcd", hgcd_matrix_reduction) {
    // hgcd 结构正确性：矩阵关系 (a;b) == M*(a';b')、行列式 [1|-1]、规模折半、规范序。
    // gcd 不变性由矩阵关系（幺模变换）数学保证。关系重建全用库乘加。
    // 零长矩阵元素（单位阵提前返回形态）跳过乘法，零垫乘积段。
    u64 seed = 0x51ce1b7ea4df7c21ull;
    for (mp_size_t n : {3, 5, 8, 16, 33, 50, 64, 100, 217, 511}) {
        int iters = n >= 217 ? 2 : 3;  // 大尺寸乘法重建贵，减迭代保时限
        for (int bmode = 0; bmode < 4; ++bmode) {
            for (int iter = 0; iter < iters; ++iter) {
                mp_size_t N = n;  // 本迭代工作长度（fib 分支取 F(k+1) 实际长度）
                mp_ptr a = alloc_limbs(n + 8);
                mp_ptr b = alloc_limbs(n + 8);
                if (bmode == 3) {
                    // 相邻 Fibonacci 对（商全 1 的最坏情形），fib_pair 库构造；
                    // 按实际长度 N 拷贝保证 a 顶 limb 非零（hgcd 归一化契约）
                    mp_ptr fx = alloc_limbs(n + 8), fy = alloc_limbs(n + 8);
                    N = fib_pair(fx, fy, n + 8, n);
                    lmmp_zero(a, n + 8);
                    lmmp_zero(b, n + 8);
                    memcpy(a, fy, (size_t)N * 8);  // a = F(k+1)（大者）
                    memcpy(b, fx, (size_t)N * 8);  // b = F(k)（高位已零垫）
                    lmmp_free(fx);
                    lmmp_free(fy);
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
                if (mpn_cmp(a, N, b, N) < 0) {
                    mp_ptr t = a; a = b; b = t;
                }
                if (mpn_eq(a, N, b, N)) {
                    lmmp_free(a); lmmp_free(b);
                    continue;  // hgcd 契约要求 a > b
                }
                mp_ptr wa = alloc_limbs(N), wb = alloc_limbs(N);
                lmmp_copy(wa, a, N);
                lmmp_copy(wb, b, N);
                lmmp_hgcd_matrix_t M;
                lmmp_hgcd_matrix_init_(&M, N + 2);
                mp_size_t nn = lmmp_hgcd_(&M, wa, wb, N);
                if (nn > 0) {
                    // 行关系：m00*a' + m01*b' == a；m10*a' + m11*b' == b
                    for (int row = 0; row < 2; ++row) {
                        mp_size_t lm0 = hgcd_len(&M, row, 0), lm1 = hgcd_len(&M, row, 1);
                        mp_size_t mx = lm0 + nn + lm1 + nn + 4;
                        mp_ptr p1 = alloc_limbs(mx), p2 = alloc_limbs(mx), sum = alloc_limbs(mx);
                        lmmp_zero(p1, lm0 + nn);  // 零长元素形态下乘积段保持零
                        if (lm0 > 0) {
                            if (lm0 >= nn) lmmp_mul_(p1, M.m[row][0], lm0, wa, nn);
                            else lmmp_mul_(p1, wa, nn, M.m[row][0], lm0);
                        }
                        lmmp_zero(p2, lm1 + nn);
                        if (lm1 > 0) {
                            if (lm1 >= nn) lmmp_mul_(p2, M.m[row][1], lm1, wb, nn);
                            else lmmp_mul_(p2, wb, nn, M.m[row][1], lm1);
                        }
                        mp_size_t l1 = lm0 + nn, l2 = lm1 + nn;
                        pad_add(p1, l1, p2, l2, mx, sum);
                        const mp_limb_t* expectv = row == 0 ? a : b;
                        TEST_CHECK_MSG(mpn_eq(sum, mx, expectv, N), "hgcd row relation");
                        lmmp_free(p1); lmmp_free(p2); lmmp_free(sum);
                    }
                    // det = m00*m11 - m01*m10 == ±1（等长零垫后相减）
                    {
                        mp_size_t l00 = hgcd_len(&M, 0, 0), l11 = hgcd_len(&M, 1, 1);
                        mp_size_t l01 = hgcd_len(&M, 0, 1), l10 = hgcd_len(&M, 1, 0);
                        mp_size_t l0 = l00 + l11, l1 = l01 + l10;
                        mp_size_t mx = (l0 > l1 ? l0 : l1) + 1;
                        mp_ptr p0 = alloc_limbs(mx), p1 = alloc_limbs(mx), q0 = alloc_limbs(mx),
                               q1 = alloc_limbs(mx), diff = alloc_limbs(mx);
                        lmmp_zero(q0, l0);  // 任一因子零长时乘积段保持零
                        if (l00 > 0 && l11 > 0) {
                            if (l00 >= l11) lmmp_mul_(q0, M.m[0][0], l00, M.m[1][1], l11);
                            else lmmp_mul_(q0, M.m[1][1], l11, M.m[0][0], l00);
                        }
                        lmmp_zero(q1, l1);
                        if (l01 > 0 && l10 > 0) {
                            if (l01 >= l10) lmmp_mul_(q1, M.m[0][1], l01, M.m[1][0], l10);
                            else lmmp_mul_(q1, M.m[1][0], l10, M.m[0][1], l01);
                        }
                        lmmp_zero(p0, mx); memcpy(p0, q0, (size_t)l0 * 8);
                        lmmp_zero(p1, mx); memcpy(p1, q1, (size_t)l1 * 8);
                        lmmp_sub_(diff, p0, mx, p1, mx);
                        mp_size_t dl = norm_len(diff, mx);
                        bool det_ok = (dl <= 1 && (dl == 0 || diff[0] == 1));  // 差恰为 +1
                        if (!det_ok) {
                            lmmp_sub_(diff, p1, mx, p0, mx);  // 反向：差为 +1 ⟺ 原差为 -1
                            dl = norm_len(diff, mx);
                            det_ok = (dl <= 1 && (dl == 0 || diff[0] == 1));
                        }
                        TEST_CHECK_MSG(det_ok, "hgcd det == +-1");
                        lmmp_free(p0); lmmp_free(p1); lmmp_free(q0); lmmp_free(q1); lmmp_free(diff);
                    }
                    TEST_CHECK_MSG(mpn_cmp(wa, nn, wb, nn) >= 0, "hgcd canonical order");
                } else {
                    // 无归约合法（如下限阻止），此时数对应未被破坏
                    TEST_CHECK_MSG(mpn_eq(wa, N, a, N) && mpn_eq(wb, N, b, N),
                                   "hgcd no-reduction keeps pair");
                }
                lmmp_hgcd_matrix_free_(&M);
                lmmp_free(a); lmmp_free(b); lmmp_free(wa); lmmp_free(wb);
            }
        }
    }
}

TEST_CASE("numth/gcd", gcd_hgcd_correctness) {
    // gcd_hgcd：构造形态（b=1、a=b+1、a=k*b、相邻 fib）给出精确已知 gcd；
    // 随机形态与 lehmer（独立归约序列）交叉；全部形态附整除性。
    u64 seed = 0x2b44ae9d4d6c8f13ull;
    for (mp_size_t n : {1, 3, 5, 8, 16, 33, 64, 120, 200, 321}) {
        int iters = n >= 200 ? 2 : 3;
        for (int mode = 0; mode < 5; ++mode) {
            for (int iter = 0; iter < iters; ++iter) {
                mp_ptr a = alloc_limbs(n + 2);
                mp_ptr b = alloc_limbs(n + 2);
                lmmp_zero(a, n + 2);  // 清掉容量冗余段：norm_len(a,n+2) 不得吃到垃圾
                lmmp_zero(b, n + 2);
                mp_ptr exp = alloc_limbs(n + 2);  // 已知期望值缓冲
                mp_size_t bn = n;
                mp_size_t expect_len = 0;  // 0 = 无已知期望
                if (mode == 0) {  // 随机同长
                    random_limbs(a, n, seed);
                    random_limbs(b, n, seed);
                    if (n > 1) b[n - 1] >>= 1;
                    if (b[n - 1] == 0) b[n - 1] = 1;
                } else if (mode == 1) {  // b = 1 → gcd = 1
                    random_limbs(a, n, seed);
                    b[0] = 1;
                    for (mp_size_t i = 1; i < n; ++i) b[i] = 0;
                    bn = 1;
                    exp[0] = 1;
                    expect_len = 1;
                } else if (mode == 2) {  // a = b+1 → gcd = 1
                    random_limbs(b, n, seed);
                    lmmp_copy(a, b, n);
                    mp_limb_t carry = 1;
                    for (mp_size_t i = 0; i < n && carry; ++i) {
                        mp_limb_t t = a[i] + carry;
                        carry = t < carry;
                        a[i] = t;
                    }
                    if (carry) a[n] = 1;  // b 全 F 形态：a = B^n，gcd 仍为 1
                    exp[0] = 1;
                    expect_len = 1;
                } else if (mode == 3) {  // a = k*b（gcd 恰为 b，库 mul_1 构造）
                    random_limbs(b, n, seed);
                    b[0] |= 1;
                    u64 k = 3 + (xorshift64(seed) & 0xff);
                    lmmp_zero(a, n + 2);
                    u64 hi = lmmp_mul_1_(a, b, n, k);
                    if (hi) a[n] = hi;
                    memcpy(exp, b, (size_t)n * 8);
                    expect_len = n;
                } else {  // 相邻 Fibonacci（gcd = 1，数学已知）
                    // 容量含 size_ 冗余（2-4 limb）：fib_pair 内部需以 need 调
                    // fibonacci2_，缓冲不足会跳过唯一可行档位
                    mp_ptr fx = alloc_limbs(n + 8), fy = alloc_limbs(n + 8);
                    mp_size_t rn = fib_pair(fx, fy, n + 8, n);
                    lmmp_zero(a, n + 2);
                    lmmp_zero(b, n + 2);
                    memcpy(a, fy, (size_t)rn * 8);
                    memcpy(b, fx, (size_t)rn * 8);
                    exp[0] = 1;
                    expect_len = 1;
                    lmmp_free(fx);
                    lmmp_free(fy);
                }

                mp_size_t an = norm_len(a, n + 2);
                if (mpn_cmp(a, an, b, bn) < 0) {
                    mp_ptr t = a; a = b; b = t;
                    mp_size_t tn = an; an = bn; bn = tn;
                }
                if (mpn_eq(a, an, b, bn)) {
                    lmmp_free(a); lmmp_free(b);
                    lmmp_free(exp);
                    continue;
                }

                mp_ptr g = alloc_limbs(an);
                mp_size_t gn = lmmp_gcd_hgcd_(g, a, an, b, bn);
                if (expect_len) {
                    TEST_CHECK_MSG(mpn_eq(g, gn, exp, expect_len), "gcd_hgcd known result");
                } else {
                    // 随机形态：与 lehmer 交叉（两条归约路径必同值）
                    mp_ptr d2 = alloc_limbs(an);
                    mp_size_t g2 = lmmp_gcd_lehmer_(d2, a, an, b, bn);
                    TEST_CHECK_MSG(mpn_eq(g, gn, d2, g2), "gcd_hgcd vs lehmer");
                    lmmp_free(d2);
                }
                TEST_CHECK_MSG(divides_all(g, gn, a, an), "gcd divides a");
                TEST_CHECK_MSG(divides_all(g, gn, b, bn), "gcd divides b");
                lmmp_free(g);
                lmmp_free(a);
                lmmp_free(b);
                lmmp_free(exp);
            }
        }
    }
}

TEST_CASE("numth/gcd", gcd_dispatch) {
    // 通用分发入口：随机输入与 Lehmer 交叉 + 整除性（<GCD_HGCD_THRESHOLD 分发
    // 到 lehmer/gcd_1/gcd_2，交叉覆盖各档一致性；>=86 走 hgcd 与 lehmer 独立）
    u64 seed = 0x853c49e6748fea9bull;
    for (mp_size_t n : {1, 2, 3, 5, 9, 17, 33, 60}) {
        for (int iter = 0; iter < 5; ++iter) {
            mp_size_t un = n;
            mp_size_t vn = 1 + (mp_size_t)(xorshift64(seed) % (u64)n);
            mp_ptr u = alloc_limbs(un), v = alloc_limbs(vn), d = alloc_limbs(un),
                   gr = alloc_limbs(un);
            random_limbs(u, un, seed);
            random_limbs(v, vn, seed);
            mp_size_t gn = lmmp_gcd_(d, u, un, v, vn);
            mp_size_t ln = lmmp_gcd_lehmer_(gr, u, un, v, vn);
            TEST_CHECK(mpn_eq(d, gn, gr, ln));
            TEST_CHECK_MSG(divides_all(d, gn, u, un), "gcd_ divides u");
            lmmp_free(u); lmmp_free(v); lmmp_free(d); lmmp_free(gr);
        }
    }

    // 大尺寸（Lehmer 与 hgcd 两条路径）交叉一致 + 整除性
    for (mp_size_t n : {700, 900, 1300}) {
        mp_ptr u = alloc_limbs(n);
        mp_ptr v = alloc_limbs(n);
        random_limbs(u, n, seed);
        random_limbs(v, n, seed);
        v[n - 1] >>= 1;
        if (v[n - 1] == 0) v[n - 1] = 1;
        mp_ptr d1 = alloc_limbs(n);
        mp_ptr d2 = alloc_limbs(n);
        mp_size_t g1 = lmmp_gcd_lehmer_(d1, u, n, v, n);
        mp_size_t g2 = lmmp_gcd_(d2, u, n, v, n);
        TEST_CHECK_MSG(mpn_eq(d1, g1, d2, g2), "gcd_ matches lehmer");
        TEST_CHECK_MSG(divides_all(d2, g2, u, n), "gcd_ divides u");
        lmmp_free(u); lmmp_free(v); lmmp_free(d1); lmmp_free(d2);
    }
}

TEST_CASE("numth/gcd", gcd_unbalanced) {
    // 不平衡输入系统覆盖：预归约路径（un >= 2*vn 触发）、2:1 边界两侧、
    // 整除早退（r == 0）、链式取模（余数仍显著短于除数）、单/双 limb 除数。
    // 与 Lehmer（原生处理不平衡）交叉对照 + 整除性验证
    u64 seed = 0x853c49e6748fea9bull;
    struct { mp_size_t un, vn; } sizes[] = {
        {90, 45},   {90, 44},   {100, 50},  {100, 51},  {130, 64},
        {130, 65},  {200, 99},  {200, 100}, {200, 50},  {200, 25},
        {200, 10},  {200, 4},   {333, 166}, {333, 167}, {333, 83},
        {333, 40},  {333, 5},   {333, 2},   {333, 1},
    };
    for (auto& s : sizes) {
        for (int mode = 0; mode < 3; ++mode) {
            mp_size_t d = s.un - s.vn;
            if (mode == 2 && d <= 3) continue;  // 链式取模形态需要余数空间
            for (int iter = 0; iter < 3; ++iter) {
                mp_ptr v = alloc_limbs(s.vn);
                random_limbs(v, s.vn, seed);
                mp_ptr u = alloc_limbs(s.un);
                if (mode == 0) {
                    // 随机（余数均匀分布，通常 1~2 次预归约后平衡）
                    random_limbs(u, s.un, seed);
                } else if (mode == 1) {
                    // u = v * B^d：整除，gcd = v（预归约 r == 0 早退）
                    lmmp_zero(u, d);
                    lmmp_copy(u + d, v, s.vn);
                } else {
                    // u = v * B^d + tiny：余数仅 3 limb，触发链式取模
                    lmmp_zero(u, d);
                    lmmp_copy(u + d, v, s.vn);
                    u[0] = 0x9e3779b97f4a7c15ull | 1;
                    u[1] = 0x51ce1b7ea4df7c21ull | 1;
                    u[2] = 0x2b44ae9d4d6c8f13ull | 1;
                }

                mp_ptr d1 = alloc_limbs(s.un);
                mp_ptr d2 = alloc_limbs(s.un);
                mp_size_t g1 = lmmp_gcd_(d1, u, s.un, v, s.vn);
                mp_size_t g2 = lmmp_gcd_lehmer_(d2, u, s.un, v, s.vn);
                TEST_CHECK_MSG(mpn_eq(d1, g1, d2, g2), "unbalanced gcd_ matches lehmer");
                TEST_CHECK_MSG(g1 > 0, "unbalanced gcd positive length");
                TEST_CHECK_MSG(divides_all(d1, g1, u, s.un), "unbalanced gcd divides u");
                TEST_CHECK_MSG(divides_all(d1, g1, v, s.vn), "unbalanced gcd divides v");
                if (mode == 1) {
                    TEST_CHECK_MSG(mpn_eq(d1, g1, v, s.vn), "unbalanced exact-multiple gcd is v");
                }
                // 大输入直接走 hgcd 分治入口（含预归约）复核
                if (s.un >= 100) {
                    mp_ptr d3 = alloc_limbs(s.un);
                    mp_size_t g3 = lmmp_gcd_hgcd_(d3, u, s.un, v, s.vn);
                    TEST_CHECK_MSG(mpn_eq(d3, g3, d1, g1), "unbalanced gcd_hgcd matches");
                    lmmp_free(d3);
                }
                lmmp_free(u); lmmp_free(v); lmmp_free(d1); lmmp_free(d2);
            }
        }
    }
}
