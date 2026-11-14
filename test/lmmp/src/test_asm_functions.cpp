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
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

/*
    全部用库内组合恒等式验证：add_nc == add_n 再补 c、div 系列用
    q*d + r == n 重建（乘法用 mul_1/mul，加法用 add_n）。
*/

TEST_CASE("asm/addsub", add_nc_sub_nc_random) {
    u64 seed = 0xa11ce01234567890ull;
    for (mp_size_t n = 1; n <= 65; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n), t = alloc_limbs(n);
            random_limbs(a, n, seed, false);  // 高位允许为零（非归一化输入）
            random_limbs(b, n, seed, false);

            u64 carry0 = lmmp_add_nc_(t, a, b, n, 0);
            u64 borrow0 = lmmp_sub_nc_(t, a, b, n, 0);
            for (u64 c : {0u, 1u}) {
                // add_nc(a,b,c) == add_n(a,b) 再 +c
                u64 cy = lmmp_add_nc_(d, a, b, n, c);
                mp_ptr e = alloc_limbs(n);
                lmmp_add_n_(e, a, b, n);
                u64 ovf = c ? add_small_inplace(e, n, 1) : 0;
                TEST_CHECK_EQ(memcmp(d, e, n * 8), 0);
                TEST_CHECK_EQ(cy, carry0 + ovf);

                // sub_nc(a,b,c) == sub_n(a,b) 再 -c
                u64 bw = lmmp_sub_nc_(d, a, b, n, c);
                lmmp_sub_n_(e, a, b, n);
                u64 unf = c ? sub_small_inplace(e, n, 1) : 0;
                TEST_CHECK_EQ(memcmp(d, e, n * 8), 0);
                TEST_CHECK_EQ(bw, borrow0 + unf);
                lmmp_free(e);
            }

            lmmp_free(a); lmmp_free(b); lmmp_free(d); lmmp_free(t);
        }
    }
}

TEST_CASE("asm/mul1", mul_1_addmul_submul_random_inplace) {
    u64 seed = 0xb0b0c0d0e0f01020ull;
    for (mp_size_t n = 1; n <= 65; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1), t = alloc_limbs(n + 2),
                   acc = alloc_limbs(n + 1), acc2 = alloc_limbs(n + 1);
            random_limbs(a, n, seed, false);
            u64 x = xorshift64(seed) | 1;

            // mul_1 与 div_1 互逆重建
            u64 cy = lmmp_mul_1_(d, a, n, x);
            d[n] = cy;
            u64 rem = lmmp_div_1_(t, d, n + 1, x);
            TEST_CHECK_EQ(rem, 0ull);
            TEST_CHECK(mpn_eq(t, n + 1, a, n));

            // in-place mul_1(a2,a2) 与分离调用一致
            mp_ptr a2 = alloc_limbs(n);
            memcpy(a2, a, n * 8);
            memcpy(d, a, n * 8);
            u64 cy2 = lmmp_mul_1_(a2, a2, n, x);
            u64 cy_ref = lmmp_mul_1_(d, a, n, x);
            TEST_CHECK_EQ(cy2, cy_ref);
            TEST_CHECK_EQ(memcmp(a2, d, n * 8), 0);
            TEST_CHECK_EQ(cy2, cy);  // 与首段同输入同输出

            // addmul_1 == mul_1 后 add_n（两路径）
            random_limbs(acc, n, seed, false);
            memcpy(acc2, acc, n * 8);
            acc2[n] = 0;
            u64 mcy = lmmp_mul_1_(d, a, n, x);
            u64 c1 = lmmp_add_n_(t, acc, d, n) + mcy;  // 进位=加进位+乘进位
            u64 c2 = lmmp_addmul_1_(acc2, a, n, x);
            TEST_CHECK_EQ(memcmp(acc2, t, n * 8), 0);
            TEST_CHECK_EQ(c2, c1);

            // in-place addmul_1(acc2, acc2)：x*acc + acc
            memcpy(acc2, acc, n * 8);
            acc2[n] = 0;
            u64 mcyb = lmmp_mul_1_(d, acc, n, x);
            u64 c1b = lmmp_add_n_(t, acc, d, n) + mcyb;
            u64 c2b = lmmp_addmul_1_(acc2, acc2, n, x);
            TEST_CHECK_EQ(memcmp(acc2, t, n * 8), 0);
            TEST_CHECK_EQ(c2b, c1b);

            // submul_1 == sub_n(acc, mul_1 积)，借位复合
            memcpy(acc2, acc, n * 8);
            acc2[n] = 0;
            lmmp_mul_1_(d, a, n, x);
            u64 w = lmmp_sub_n_(t, acc, d, n);
            u64 hi = cy;  // mul_1 高位（同 a、x）
            u64 c3 = lmmp_submul_1_(acc2, a, n, x);
            TEST_CHECK_EQ(memcmp(acc2, t, n * 8), 0);
            TEST_CHECK_EQ(c3, w + hi);

            // in-place submul_1(acc2, acc2)
            memcpy(acc2, acc, n * 8);
            acc2[n] = 0;
            u64 hi2 = lmmp_mul_1_(d, acc, n, x);
            u64 w2 = lmmp_sub_n_(t, acc, d, n);
            u64 c3b = lmmp_submul_1_(acc2, acc2, n, x);
            TEST_CHECK_EQ(memcmp(acc2, t, n * 8), 0);
            TEST_CHECK_EQ(c3b, w2 + hi2);

            lmmp_free(a); lmmp_free(a2); lmmp_free(d); lmmp_free(t); lmmp_free(acc); lmmp_free(acc2);
        }
    }
}

TEST_CASE("asm/addmul2", addmul_2_vs_composition_and_fullmul) {
    u64 seed = 0xa44a2badcafe01ull;
    auto run = [&](mp_ptr a, mp_ptr rp, mp_size_t n, mp_limb_t v0, mp_limb_t v1) {
        // 被测:asm/C addmul_2,契约为 [rp,n+1] 覆写 rp[n] + 返回第 n+1 limb
        mp_ptr r1 = alloc_limbs(n + 3), r2 = alloc_limbs(n + 3), r3 = alloc_limbs(n + 3),
               t = alloc_limbs(n + 2);
        mp_ptr vv = alloc_limbs(2);
        vv[0] = v0; vv[1] = v1;
        memset(r1, 0xCC, (n + 3) * 8);
        memcpy(r1, rp, n * 8);
        mp_limb_t tail0 = r1[n + 2];
        u64 ret1 = lmmp_addmul_2_(r1, a, n, vv);

        // 参考 1:两遍 addmul_1 组合(同计算另一路径)
        memset(r2, 0xCC, (n + 3) * 8);
        memcpy(r2, rp, n * 8);
        r2[n] = 0;
        mp_limb_t cy0 = lmmp_addmul_1_(r2, a, n, v0);
        mp_limb_t cy1 = lmmp_addmul_1_(r2 + 1, a, n, v1);
        mp_limb_t hn = r2[n] + cy0;
        r2[n] = hn;
        u64 ret2 = cy1 + (mp_limb_t)(hn < cy0);

        // 参考 2:mul_ 全乘 + add_n 组合(n==1 时换算子序保持 na>=nb)
        if (n == 1) lmmp_mul_(t, vv, 2, a, 1);
        else lmmp_mul_(t, a, n, vv, 2);
        mp_limb_t c = lmmp_add_n_(r3, rp, t, n);
        mp_limb_t e2 = t[n] + c;
        r3[n] = e2;
        u64 ret3 = t[n + 1] + (mp_limb_t)(e2 < c);

        TEST_CHECK_EQ(memcmp(r1, r2, (n + 1) * 8), 0);
        TEST_CHECK_EQ(ret1, ret2);
        TEST_CHECK_EQ(memcmp(r1, r3, (n + 1) * 8), 0);
        TEST_CHECK_EQ(ret1, ret3);
        // 写入长度契约:n+2 limb 之外不得写
        TEST_CHECK_EQ(r1[n + 2], tail0);

        // eqsep 原位形态:与同值分离形(r4=a 值拷贝,numb 仍读 a)交叉
        mp_ptr r4 = alloc_limbs(n + 1), a2 = alloc_limbs(n + 1);
        memcpy(r4, a, n * 8);
        memcpy(a2, a, n * 8);
        u64 ret4 = lmmp_addmul_2_(r4, r4, n, vv);
        u64 ret5 = lmmp_addmul_2_(a2, a, n, vv);
        TEST_CHECK_EQ(ret4, ret5);
        TEST_CHECK_EQ(memcmp(r4, a2, (n + 1) * 8), 0);

        lmmp_free(r1); lmmp_free(r2); lmmp_free(r3); lmmp_free(r4); lmmp_free(a2);
        lmmp_free(t); lmmp_free(vv);
    };

    for (mp_size_t n = 1; n <= 65; ++n) {
        for (int iter = 0; iter < 16; ++iter) {
            mp_ptr a = alloc_limbs(n), rp = alloc_limbs(n);
            random_limbs(a, n, seed, false);
            random_limbs(rp, n, seed, false);
            mp_limb_t v[2] = {xorshift64(seed) | 1, xorshift64(seed) | 1};
            run(a, rp, n, v[0], v[1]);
            lmmp_free(a); lmmp_free(rp);
        }
    }
    // 极端输入:全 FF / 全 0 / v 极值(进位保存不变量边界情形)
    for (mp_size_t n = 1; n <= 33; ++n) {
        mp_ptr a = alloc_limbs(n), rp = alloc_limbs(n);
        memset(a, 0xFF, n * 8);
        memset(rp, 0xFF, n * 8);
        run(a, rp, n, ~(mp_limb_t)0, ~(mp_limb_t)0);
        memset(rp, 0, n * 8);
        run(a, rp, n, ~(mp_limb_t)0, ~(mp_limb_t)0);
        memset(a, 0, n * 8);
        run(a, rp, n, ~(mp_limb_t)0, 1);
        run(a, rp, n, 1, ~(mp_limb_t)0);
        lmmp_free(a); lmmp_free(rp);
    }
}

TEST_CASE("asm/shift", shl_shr_carry_random) {
    u64 seed = 0xc1c2c3c4c5c6c7c8ull;
    for (mp_size_t n = 1; n <= 17; ++n) {
        for (int iter = 0; iter < 5; ++iter) {
            mp_ptr a = alloc_limbs(n), d = alloc_limbs(n), tsh = alloc_limbs(n),
                   tshr = alloc_limbs(n);
            random_limbs(a, n, seed, false);

            for (mp_size_t s = 0; s < 64; ++s) {
                // 独立基准：shl 与 shr 各自缓冲（互逆性由 low/shift 覆盖）
                u64 r1 = lmmp_shl_(tsh, a, n, s);
                u64 r2 = lmmp_shr_(tshr, a, n, s);
                u64 lowmask = s ? (((u64)1 << s) - 1) : 0;

                // shl_c：body == shl（limb[1,n) 与 limb0 高位一致），
                // 低 s 位为注入的 c，返回同 shl
                u64 c_shl = s ? (xorshift64(seed) & lowmask) : 0;
                u64 r3 = lmmp_shl_c_(d, a, n, s, c_shl);
                if (n > 1) TEST_CHECK_EQ(memcmp(d + 1, tsh + 1, (n - 1) * 8), 0);
                if (s) {
                    TEST_CHECK_EQ(d[0] & lowmask, c_shl);
                    TEST_CHECK_EQ(d[0] >> s, tsh[0] >> s);
                } else {
                    TEST_CHECK_EQ(memcmp(d, tsh, n * 8), 0);
                }
                TEST_CHECK_EQ(r3, r1);

                // shr_c：body == shr（limb[0,n-1) 一致 + 顶 limb 低位一致），
                // 顶 s 位为注入的 c（已置于高位），返回同 shr
                u64 c_shr = s ? (xorshift64(seed) << (64 - s)) : 0;
                u64 topmask = s ? (~0ull << (64 - s)) : 0;
                u64 r4 = lmmp_shr_c_(d, a, n, s, c_shr);
                if (n > 1) TEST_CHECK_EQ(memcmp(d, tshr, (n - 1) * 8), 0);
                TEST_CHECK_EQ(d[n - 1] & ~topmask, tshr[n - 1] & ~topmask);
                if (s) TEST_CHECK_EQ(d[n - 1] & topmask, c_shr & topmask);
                TEST_CHECK_EQ(r4, r2);
            }

            lmmp_free(a); lmmp_free(d); lmmp_free(tsh); lmmp_free(tshr);
        }
    }
}

TEST_CASE("asm/shift", shlnot_not_addshl1_subshl1_random) {
    u64 seed = 0xd1d2d3d4d5d6d7d8ull;
    for (mp_size_t n = 1; n <= 65; ++n) {
        for (int iter = 0; iter < 10; ++iter) {
            mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n + 1),
                   t = alloc_limbs(n + 1), b2 = alloc_limbs(n + 1);
            random_limbs(a, n, seed, false);
            random_limbs(b, n, seed, false);

            lmmp_not_(d, a, n);
            for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(d[i], ~a[i]);

            for (mp_size_t s = 0; s < 64; ++s) {
                u64 r1 = lmmp_shl_(t, a, n, s);
                u64 ret = lmmp_shlnot_(d, a, n, s);
                lmmp_not_(b2, t, n);
                TEST_CHECK_EQ(memcmp(d, b2, n * 8), 0);
                TEST_CHECK_EQ(ret, r1);  // 返回移出位与 shl 一致（不取反）
            }

            // addshl1_n == a + 2b（复合路径），进位复合 c1+c2
            u64 c1 = lmmp_add_n_(b2, b, b, n);
            u64 c2 = lmmp_add_n_(t, a, b2, n);
            u64 cy = lmmp_addshl1_n_(d, a, b, n);
            TEST_CHECK_EQ(memcmp(d, t, n * 8), 0);
            TEST_CHECK_EQ(cy, c1 + c2);

            // subshl1_n == a - 2b（模 B^n 语义），借位复合 w1+c1
            u64 w1 = lmmp_sub_n_(t, a, b2, n);
            u64 bw = lmmp_subshl1_n_(d, a, b, n);
            TEST_CHECK_EQ(memcmp(d, t, n * 8), 0);
            TEST_CHECK_EQ(bw, w1 + c1);

            lmmp_free(a); lmmp_free(b); lmmp_free(d); lmmp_free(t); lmmp_free(b2);
        }
    }
}

/*
    shr1add_nc / shr1sub_nc：先算 (a±(b+c))（add_nc/sub_nc 路径），再右移
    1 位；进位/借位落位于结果顶位，返回值为差/和的最低位。
*/
TEST_CASE("asm/rsh1", shr1add_shr1sub_random) {
    u64 seed = 0xe1e2e3e4e5e6e7e8ull;
    for (mp_size_t n = 1; n <= 65; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n), t = alloc_limbs(n);
            random_limbs(a, n, seed, false);
            random_limbs(b, n, seed, false);

            for (u64 c : {0u, 1u}) {
                u64 cout = lmmp_add_nc_(t, a, b, n, c);
                u64 l = lmmp_shr1add_nc_(d, a, b, n, c);
                u64 low = shr1_inplace(t, n, cout);
                TEST_CHECK_EQ(memcmp(d, t, n * 8), 0);
                TEST_CHECK_EQ(l, low);

                u64 bout = lmmp_sub_nc_(t, a, b, n, c);
                u64 l2 = lmmp_shr1sub_nc_(d, a, b, n, c);
                u64 low2 = shr1_inplace(t, n, bout);
                TEST_CHECK_EQ(memcmp(d, t, n * 8), 0);
                TEST_CHECK_EQ(l2, low2);
            }

            lmmp_free(a); lmmp_free(b); lmmp_free(d); lmmp_free(t);
        }
    }
}

/*
    div_3_2：用库 mul_1 + add 构造 n = q*d + r（q < B、MSB(d)=1、r < d，
    则 n < B^3），判决须还原 q 并把 [np,2] 改写为 r。
*/
TEST_CASE("asm/div", div_3_2_random) {
    u64 seed = 0xf1f2f3f4f5f6f7f8ull;
    for (int iter = 0; iter < 500; ++iter) {
        mp_ptr np = alloc_limbs(3), npw = alloc_limbs(3), dp = alloc_limbs(2);
        dp[1] = xorshift64(seed) | LIMB_B_2;
        dp[0] = xorshift64(seed);
        u64 q = xorshift64(seed);
        u64 r = dp[0] - 1;  // 单 limb 余数，恒 < d（d >= 2^63*B > B > r）

        // np = q*d + r（q*d 经 mul_1 得 3 limbs，再 + r）
        np[0] = dp[0];
        np[1] = dp[1];
        u64 hi = lmmp_mul_1_(np, np, 2, q);
        np[2] = hi;
        add_small_inplace(np, 3, r);

        mp_limb_t inv21 = lmmp_inv_2_1_(dp[1], dp[0]);
        memcpy(npw, np, 3 * 8);
        mp_limb_t qq = lmmp_div_3_2_(npw, dp, inv21);
        TEST_CHECK_EQ(qq, q);
        TEST_CHECK_EQ(npw[0], r);
        TEST_CHECK_EQ(npw[1], 0ull);

        lmmp_free(np); lmmp_free(npw); lmmp_free(dp);
    }
}

/*
    div_1/mod_1：恒等式 q*x + r == a 且 r < x；mod_1 与 div_1 余数一致；
    div_1_s 与 div_1 同输入同结果（破坏性入参上重建）。
*/
TEST_CASE("asm/div", div_1_mod_1_random_extra) {
    u64 seed = 0x0123456789abcdefull;
    for (mp_size_t n = 1; n <= 70; ++n) {
        for (int iter = 0; iter < 10; ++iter) {
            mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), t = alloc_limbs(n + 2),
                   a2 = alloc_limbs(n);
            random_limbs(a, n, seed, false);
            u64 x = xorshift64(seed) | 1;

            u64 r_mod = lmmp_mod_1_(a, n, x);
            u64 r_div = lmmp_div_1_(q, a, n, x);
            TEST_CHECK_EQ(r_mod, r_div);
            TEST_CHECK(r_div < x);

            // 重建：q*x + r == a
            u64 hi = lmmp_mul_1_(t, q, n, x);
            t[n] = hi;
            add_small_inplace(t, n + 1, r_div);
            TEST_CHECK(mpn_eq(t, n + 1, a, n));

            // div_1_s 与 div_1 一致（MSB(x)=1 档）：qh:[qs,n-1] 合成完整商
            if (n > 1 && (x | LIMB_B_2) == x) {
                memcpy(a2, a, n * 8);
                mp_ptr qs = alloc_limbs(n + 1);
                u64 qh = lmmp_div_1_s_(qs, a2, n, x);
                u64 rs = a2[0];
                TEST_CHECK_EQ(memcmp(qs, q, (n - 1) * 8), 0);
                TEST_CHECK_EQ(qh, q[n - 1]);
                TEST_CHECK_EQ(rs, r_div);
                lmmp_free(qs);
            }

            lmmp_free(a); lmmp_free(q); lmmp_free(t); lmmp_free(a2);
        }
    }
}

/*
    div_2/mod_2：恒等式 q*d + r == a（mul 重建）且 r < d；三个入口
    （div_2、mod_2、div_2_s）结果互相一致。
*/
TEST_CASE("asm/div", div_2_mod_2_random_extra) {
    u64 seed = 0x0badf00dcafefaceull;
    for (mp_size_t n = 2; n <= 40; ++n) {
        for (int iter = 0; iter < 10; ++iter) {
            mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), b = alloc_limbs(2),
                   b2 = alloc_limbs(2), a2 = alloc_limbs(n), t = alloc_limbs(n + 2);
            random_limbs(a, n, seed, false);
            b[1] = xorshift64(seed) | LIMB_B_2;
            b[0] = xorshift64(seed);

            // mod_2（余数写入 numb；div_2 会复用 b2，先保存）
            memcpy(a2, a, n * 8);
            memcpy(b2, b, 2 * 8);
            lmmp_mod_2_(a2, n, b2);
            mp_limb_t modrem[2] = {b2[0], b2[1]};

            // div_2：商 [q, n-1]，余数写回 [b2,2]
            memcpy(b2, b, 2 * 8);
            lmmp_div_2_(q, a, n, b2);
            TEST_CHECK(mpn_eq(b2, 2, modrem, 2));  // 两入口余数一致
            TEST_CHECK(mpn_cmp(b2, 2, b, 2) < 0);  // r < d

            // 重建：q*d + r == a（n==2 时商为单 limb，用 mul_1）
            if (n == 2) {
                u64 hi = lmmp_mul_1_(t, b, 2, q[0]);
                t[2] = hi;
            } else {
                lmmp_mul_(t, q, n - 1, b, 2);
            }
            add_small_inplace(t, n + 1, b2[0]);
            add_small_inplace(t + 1, n, b2[1]);  // 进位不可能（和 < B^(n+1)）
            TEST_CHECK(mpn_eq(t, n + 1, a, n));

            // div_2_s 与 div_2 一致：qh:[qs,n-2] 合成完整商（先存余数再覆写）
            if (n > 2) {
                u64 rem0 = b2[0], rem1 = b2[1];
                memcpy(a2, a, n * 8);
                memcpy(b2, b, 2 * 8);
                mp_ptr qs = alloc_limbs(n + 1);
                mp_limb_t qh = lmmp_div_2_s_(qs, a2, n, b2);
                TEST_CHECK_EQ(memcmp(qs, q, (n - 2) * 8), 0);
                TEST_CHECK_EQ(qh, q[n - 2]);
                TEST_CHECK_EQ(a2[0], rem0);
                TEST_CHECK_EQ(a2[1], rem1);
                lmmp_free(qs);
            }

            lmmp_free(a); lmmp_free(q); lmmp_free(b); lmmp_free(b2); lmmp_free(a2); lmmp_free(t);
        }
    }
}

/*
    inv_1 契约：x*(B+inv) ∈ (B^2-1-x, B^2-1]。积用 mul_1 取双 limb，
    x*B 部分折进高 limb（无溢出：积 < B^2）。
*/
TEST_CASE("asm/inv", inv_1_inv_2_1_random) {
    u64 seed = 0x1111222233334444ull;
    for (int i = 0; i < 2000; ++i) {
        u64 x = xorshift64(seed) | LIMB_B_2;
        u64 inv = lmmp_inv_1_(x);
        mp_limb_t prod[2];
        u64 hi = lmmp_mul_1_(prod, &x, 1, inv);  // prod = x*inv 双 limb
        prod[1] = hi;
        u64 p_hi = prod[1] + x;  // + x*B 的高半（不溢出）
        u64 p_lo = prod[0];
        // prod <= B^2-1 且 > B^2-1-x
        TEST_CHECK_EQ(p_hi, ~(u64)0);
        TEST_CHECK(p_lo >= (u64)(0 - x));

        u64 xh = xorshift64(seed) | LIMB_B_2;
        u64 xl = xorshift64(seed);
        mp_limb_t inv21 = lmmp_inv_2_1_(xh, xl);
        TEST_CHECK_MSG(inv21 != 0, "inv_2_1 nonzero");
    }
}
