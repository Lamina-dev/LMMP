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
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

constexpr size_t MAX_N = 128;

}  // namespace

TEST_CASE("low/limb", endianness) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
    bool little = (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
#else
    bool little = true;  // 项目硬性要求 64 位平台，当前测试平台均为小端。
#endif
    TEST_CHECK_EQ(lmmp_endian(), little);
}

TEST_CASE("low/limb", limb_bits) {
    TEST_CHECK_EQ(lmmp_limb_bits_(0), 0);
    TEST_CHECK_EQ(lmmp_limb_bits_(1), 1);
    TEST_CHECK_EQ(lmmp_limb_bits_(2), 2);
    TEST_CHECK_EQ(lmmp_limb_bits_(3), 2);
    TEST_CHECK_EQ(lmmp_limb_bits_(4), 3);
    TEST_CHECK_EQ(lmmp_limb_bits_(UINT64_MAX), 64);
    u64 seed = 0x12345678abcdef;
    for (int i = 0; i < 200; ++i) {
        u64 x = xorshift64(seed);
        int expect = 0;
        u64 t = x;
        while (t) { ++expect; t >>= 1; }
        TEST_CHECK_EQ(lmmp_limb_bits_(x), expect);
    }
}

TEST_CASE("low/limb", popcnt_clz_ctz) {
    u64 seed = 0x9e3779b97f4a7c15ull;
    for (int i = 0; i < 300; ++i) {
        u64 x = xorshift64(seed);
        int pc = 0;
        u64 t = x;
        while (t) { pc += (int)(t & 1); t >>= 1; }
        TEST_CHECK_EQ(lmmp_limb_popcnt_(x), pc);

        int lz = 0;
        for (int b = 63; b >= 0; --b) {
            if ((x >> b) & 1) break;
            ++lz;
        }
        if (x == 0) lz = 64;
        TEST_CHECK_EQ(lmmp_leading_zeros_(x), lz);

        int tz = 0;
        t = x;
        if (t == 0) tz = 64;
        else while ((t & 1) == 0) { ++tz; t >>= 1; }
        TEST_CHECK_EQ(lmmp_tailing_zeros_(x), tz);
    }
    TEST_CHECK_EQ(lmmp_leading_zeros_(0), 64);
    TEST_CHECK_EQ(lmmp_tailing_zeros_(0), 64);
    TEST_CHECK_EQ(lmmp_limb_popcnt_(0), 0);
}

/*
    mulh/mullh 无 u128 参考，改用三层验证：
      1) 写死常数：(2^64-1)^2 = 0xFFFFFFFFFFFFFFFE_0000000000000001 等代数已知值
      2) 结构恒等式：(2^64-1)*x = x*2^64 - x，故 hi=x-1、lo=-x（x>0）；
         2^63*x = x<<63，故 hi=x>>1、lo=x<<63；x*0=0；x*1=x
      3) 库内交叉：mullh 的积 hi:lo 用 div_2 除回乘数，须整除且商还原
*/
TEST_CASE("low/limb", mulh_mullh) {
    // 写死常数锚点
    mp_limb_t dst[2];
    lmmp_mullh_(0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, dst);
    TEST_CHECK_EQ(dst[0], 0x0000000000000001ull);
    TEST_CHECK_EQ(dst[1], 0xFFFFFFFFFFFFFFFEull);
    lmmp_mullh_(0xFFFFFFFFFFFFFFFFull, 2, dst);
    TEST_CHECK_EQ(dst[0], 0xFFFFFFFFFFFFFFFEull);
    TEST_CHECK_EQ(dst[1], 1ull);
    TEST_CHECK_EQ(lmmp_mulh_(0, 0x9e3779b97f4a7c15ull), 0ull);
    TEST_CHECK_EQ(lmmp_mulh_(1, 0x9e3779b97f4a7c15ull), 0ull);

    u64 seed = 0xdeadbeefcafebabeull;
    for (int i = 0; i < 300; ++i) {
        u64 a = xorshift64(seed);
        u64 b = xorshift64(seed);

        // 结构恒等式：(2^64-1)*x
        lmmp_mullh_(0xFFFFFFFFFFFFFFFFull, b, dst);
        TEST_CHECK_EQ(dst[0], (u64)(0 - b));
        TEST_CHECK_EQ(dst[1], b - 1);
        // 2^63*x
        lmmp_mullh_(0x8000000000000000ull, b, dst);
        TEST_CHECK_EQ(dst[0], b << 63);
        TEST_CHECK_EQ(dst[1], b >> 1);
        // x*1 与 x*0
        lmmp_mullh_(a, 1, dst);
        TEST_CHECK_EQ(dst[0], a);
        TEST_CHECK_EQ(dst[1], 0ull);
        lmmp_mullh_(a, 0, dst);
        TEST_CHECK_EQ(dst[0], 0ull);
        TEST_CHECK_EQ(dst[1], 0ull);

        // 交叉：hi:lo / b == a 且整除（b==0 跳过；单 limb 除数走 div_1）
        lmmp_mullh_(a, b, dst);
        if (b != 0) {
            TEST_CHECK_EQ(lmmp_mulh_(a, b), dst[1]);
            mp_limb_t q[2], dv[2];
            dv[0] = dst[0];
            dv[1] = dst[1];
            mp_limb_t rem = lmmp_div_1_(q, dv, 2, b);
            TEST_CHECK_EQ(q[0], a);
            TEST_CHECK_EQ(q[1], 0ull);
            TEST_CHECK_EQ(rem, 0ull);
        }
    }
}

/*
    add/sub 系列用互逆恒等式：dst = a+b-c*B^n ⟹ dst - b == a（借位==c），
    dst + b ≡ a（进位==c 的补），以及 add_nc(+1) == add_n 后加 1。
*/
TEST_CASE("low/addsub", add_sub_n) {
    u64 seed = 0x1111111122222222ull;
    for (size_t n = 1; n <= MAX_N; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), dst = alloc_limbs(n), r = alloc_limbs(n),
                   d2 = alloc_limbs(n);
            random_limbs(a, n, seed);
            random_limbs(b, n, seed);

            u64 carry = lmmp_add_n_(dst, a, b, n);
            // 互逆：dst - b == a，借位 == 进位
            u64 borrow = lmmp_sub_n_(r, dst, b, n);
            TEST_CHECK_EQ(borrow, carry);
            TEST_CHECK(mpn_eq(r, n, a, n));
            // 交换律：b + a 同结果同进位
            lmmp_add_n_(r, b, a, n);
            TEST_CHECK_EQ(memcmp(r, dst, n * 8), 0);

            // add_nc(+1) == add_n 结果再 +1（手工 limb 进位传播）
            u64 c0 = lmmp_add_nc_(d2, a, b, n, 0);
            TEST_CHECK_EQ(c0, carry);
            TEST_CHECK_EQ(memcmp(d2, dst, n * 8), 0);
            u64 c1 = lmmp_add_nc_(d2, a, b, n, 1);
            memcpy(r, dst, n * 8);
            int i = 0;
            while (i < (int)n && ++r[i] == 0) ++i;  // r = dst + 1
            TEST_CHECK_EQ(memcmp(d2, r, n * 8), 0);
            TEST_CHECK_EQ(c1, (i == (int)n) ? carry + 1 : carry);

            // sub_nc(0) 与 sub_n 一致；sub_nc(1) == sub_n 结果再 -1
            u64 b0 = lmmp_sub_nc_(d2, a, b, n, 0);
            u64 bn_ = lmmp_sub_n_(r, a, b, n);
            TEST_CHECK_EQ(b0, bn_);
            TEST_CHECK_EQ(memcmp(d2, r, n * 8), 0);
            u64 b1 = lmmp_sub_nc_(d2, a, b, n, 1);
            lmmp_sub_n_(r, a, b, n);  // r 复用为 sub_n 结果
            i = 0;
            while (i < (int)n && r[i]-- == 0) ++i;  // r = sub_n 结果 - 1
            TEST_CHECK_EQ(memcmp(d2, r, n * 8), 0);
            TEST_CHECK_EQ(b1, (i == (int)n) ? bn_ + 1 : bn_);

            lmmp_free(a); lmmp_free(b); lmmp_free(dst); lmmp_free(r); lmmp_free(d2);
        }
    }
}

/*
    add_n_sub_n：da = a+b（进位 c），db = a-b（借位 s）。
    验证 da - b == a（借位 c）与 db + b == a（进位 s）—— 两个互逆关系
    覆盖双输出的正确性；返回值编码 (c<<1)|s。
*/
TEST_CASE("low/addsub", add_n_sub_n) {
    u64 seed = 0x3333333344444444ull;
    for (size_t n = 1; n <= MAX_N; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), da = alloc_limbs(n), db = alloc_limbs(n),
                   r = alloc_limbs(n);
            random_limbs(a, n, seed);
            random_limbs(b, n, seed);

            mp_limb_t cb = lmmp_add_n_sub_n_(da, db, a, b, n);

            u64 carry = lmmp_add_n_(r, a, b, n);
            TEST_CHECK_EQ(memcmp(da, r, n * 8), 0);
            u64 borrow = lmmp_sub_n_(r, a, b, n);
            TEST_CHECK_EQ(memcmp(db, r, n * 8), 0);
            TEST_CHECK_EQ(cb, (carry ? 2u : 0u) | (borrow ? 1u : 0u));

            // 互逆关系
            u64 w = lmmp_sub_n_(r, da, b, n);
            TEST_CHECK_EQ(w, carry);
            TEST_CHECK(mpn_eq(r, n, a, n));
            u64 c2 = lmmp_add_n_(r, db, b, n);
            TEST_CHECK_EQ(c2, borrow);
            TEST_CHECK(mpn_eq(r, n, a, n));

            lmmp_free(a); lmmp_free(b); lmmp_free(da); lmmp_free(db); lmmp_free(r);
        }
    }
}

/*
    shl/shr 互逆：V = (a << s) 的 n+1 limb 表示 = [dst(n limbs) | ret]。
    对 V 做 shr(2, s) 必须还原 a 且返回 0；反向 (a>>s)<<s 补零低位与
    移出位重组还原 a 的低 s 位。
*/
TEST_CASE("low/shift", shl_shr) {
    u64 seed = 0x5555555566666666ull;
    for (size_t n = 1; n <= MAX_N; n += 3) {
        for (int iter = 0; iter < 5; ++iter) {
            mp_ptr a = alloc_limbs(n), dst = alloc_limbs(n), dst2 = alloc_limbs(n + 1),
                   vbuf = alloc_limbs(n + 1);
            random_limbs(a, n, seed);

            for (size_t s = 0; s < 64; ++s) {
                mp_limb_t ret = lmmp_shl_(dst, a, n, s);
                // V = dst ++ [ret]（ret 是移出高 s 位，s>0 时 < 2^s）
                memcpy(vbuf, dst, n * 8);
                vbuf[n] = ret;
                if (s == 0) {
                    TEST_CHECK_EQ(ret, 0ull);
                } else {
                    TEST_CHECK(ret < ((u64)1 << s) || ret == 0);
                    TEST_CHECK(ret == (a[n - 1] >> (64 - s)));  // 移出位 = a 顶 s 位
                }
                mp_limb_t r2 = lmmp_shr_(dst2, vbuf, n + 1, s);
                TEST_CHECK_EQ(r2, 0ull);
                TEST_CHECK_EQ(memcmp(dst2, a, n * 8), 0);  // (a<<s)>>s == a

                // 反向：shr 后 shl 还原低 s 位
                mp_limb_t r3 = lmmp_shr_(dst, a, n, s);
                if (s == 0) {
                    TEST_CHECK_EQ(r3, 0ull);
                    TEST_CHECK_EQ(memcmp(dst, a, n * 8), 0);
                } else {
                    // 返回值为移出的低 s 位置于最高 s 位
                    u64 expect_ret = (a[0] & (((u64)1 << s) - 1)) << (64 - s);
                    TEST_CHECK_EQ(r3, expect_ret);
                    // (a>>s)<<s == a 且低 s 位清零
                    mp_limb_t r4 = lmmp_shl_(dst2, dst, n, s);
                    TEST_CHECK_EQ(r4, 0ull);
                    u64 lowmask = ((u64)1 << s) - 1;
                    TEST_CHECK_EQ(dst2[0] & lowmask, 0ull);
                    u64 alow = a[0] & lowmask;
                    TEST_CHECK_EQ(dst2[0] | alow, a[0]);
                    TEST_CHECK_EQ(memcmp(dst2 + 1, a + 1, (n - 1) * 8), 0);
                }
            }

            lmmp_free(a); lmmp_free(dst); lmmp_free(dst2); lmmp_free(vbuf);
        }
    }
}

/*
    mul_1/addmul_1/submul_1 与除法互逆重建：
      mul_1: P = a*x（n+1 limbs: dst ++ [carry]），div_1(P, x) 整除且商 == a
      addmul_1: 等价于 mul_1 后 add_n 累加（两条路径比对）
      submul_1: 等价于 sub_n 减去 mul_1 积（借位复合 w1 + hi）
*/
TEST_CASE("low/mul1", mul_1_addmul_submul) {
    u64 seed = 0x7777777788888888ull;
    for (size_t n = 1; n <= MAX_N; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n), dst = alloc_limbs(n + 1), q = alloc_limbs(n + 1),
                   acc = alloc_limbs(n + 1), acc2 = alloc_limbs(n + 1), t = alloc_limbs(n + 2);
            random_limbs(a, n, seed);
            u64 x = xorshift64(seed) | 1;

            u64 carry = lmmp_mul_1_(dst, a, n, x);
            dst[n] = carry;
            {
                u64 rem = lmmp_div_1_(q, dst, n + 1, x);
                TEST_CHECK_EQ(rem, 0ull);
                TEST_CHECK(mpn_eq(q, n + 1, a, n));  // 商恰为 a（顶 limb 0）
            }
            // x=1 / x=B-1 锚点
            lmmp_mul_1_(dst, a, n, 1);
            TEST_CHECK_EQ(memcmp(dst, a, n * 8), 0);
            lmmp_mul_1_(dst, a, n, 0);
            for (size_t i = 0; i < n; ++i) TEST_CHECK_EQ(dst[i], 0ull);

            // addmul_1 交叉路径
            random_limbs(acc, n, seed);
            memcpy(acc2, acc, n * 8);
            acc2[n] = 0;
            u64 mcy = lmmp_mul_1_(dst, a, n, x);
            u64 c1 = lmmp_add_n_(t, acc, dst, n) + mcy;  // 进位=加进位+乘进位
            u64 c2 = lmmp_addmul_1_(acc2, a, n, x);
            TEST_CHECK_EQ(memcmp(acc2, t, n * 8), 0);
            TEST_CHECK_EQ(c2, c1);

            // submul_1 交叉路径：borrow = w_sub + hi(a*x)
            memcpy(acc2, acc, n * 8);
            acc2[n] = 0;
            u64 w = lmmp_sub_n_(t, acc, dst, n);
            u64 c3 = lmmp_submul_1_(acc2, a, n, x);
            TEST_CHECK_EQ(memcmp(acc2, t, n * 8), 0);
            TEST_CHECK_EQ(c3, w + carry);

            lmmp_free(a); lmmp_free(dst); lmmp_free(q); lmmp_free(acc); lmmp_free(acc2);
            lmmp_free(t);
        }
    }
}

/*
    not/shlnot/addshl1_n/subshl1_n 恒等式：
      not: 逐 limb 取反（a + not(a) == 全 F）
      shlnot: == not(shl(a, s)) 的低 n limbs
      addshl1_n: a + 2b == add_n(a, add_n(b,b))，进位复合 c1+c2
      subshl1_n: a - 2b == sub_n(a, 2b)，借位复合 w1+c1
*/
TEST_CASE("low/misc", shlnot_not_addshl1_subshl1) {
    u64 seed = 0x99999999aaaaaaaaull;
    for (size_t n = 1; n <= MAX_N; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), dst = alloc_limbs(n + 1),
                   t = alloc_limbs(n + 1), b2 = alloc_limbs(n + 1);
            random_limbs(a, n, seed);
            random_limbs(b, n, seed);

            lmmp_not_(dst, a, n);
            for (size_t i = 0; i < n; ++i) TEST_CHECK_EQ(dst[i], ~a[i]);
            // a + not(a) == 全 F（逐 limb 和为 B-1，无进位传出）
            u64 c = lmmp_add_n_(t, a, dst, n);
            TEST_CHECK_EQ(c, 0ull);
            for (size_t i = 0; i < n; ++i) TEST_CHECK_EQ(t[i], ~(u64)0);

            for (size_t s = 0; s < 64; ++s) {
                mp_limb_t r1 = lmmp_shl_(t, a, n, s);
                mp_limb_t r2 = lmmp_shlnot_(dst, a, n, s);
                lmmp_not_(b2, t, n);  // b2 复用：not(shl body)
                TEST_CHECK_EQ(memcmp(dst, b2, n * 8), 0);
                TEST_CHECK_EQ(r2, r1);  // shlnot 移出位与 shl 一致
            }

            // addshl1_n == a + 2b 复合路径
            u64 c1 = lmmp_add_n_(b2, b, b, n);      // 2b 低 n limbs，进位 c1
            u64 c2 = lmmp_add_n_(t, a, b2, n);      // a + 2b 低 n limbs，进位 c2
            u64 cy = lmmp_addshl1_n_(dst, a, b, n);
            TEST_CHECK_EQ(memcmp(dst, t, n * 8), 0);
            TEST_CHECK_EQ(cy, c1 + c2);

            // subshl1_n == a - 2b 复合路径
            u64 w1 = lmmp_sub_n_(t, a, b2, n);      // a - 2b 低 n limbs，借位 w1
            u64 bo = lmmp_subshl1_n_(dst, a, b, n);
            TEST_CHECK_EQ(memcmp(dst, t, n * 8), 0);
            TEST_CHECK_EQ(bo, w1 + c1);

            lmmp_free(a); lmmp_free(b); lmmp_free(dst); lmmp_free(t); lmmp_free(b2);
        }
    }
}
