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

/* >128 位档素性检验：lmmp_is_strong_lucas_ / lmmp_is_prime_n_ 判定类测试
   （不与参考逐值对拍）：

      素数侧（不能错判：错拒素数即失败，硬断言）
        - 已证明 Mersenne 素数 M_p = 2^p-1（p ∈ {521,607,1279,4423}；release
          额外跑 p=19937 覆盖梅森折叠 REDC 层）；
        - Proth 定理运行时认证（证书经 lmmp_powmod_ 独立梯子）。
      合数侧（允许概率性误判（返回"素数"合法）；不允许崩溃或非法返回值）
        - 构造性 SPRP-2：Mersenne 合数、Fermat 数、Carmichael 对抗语料；
        - p1*p2 / p1*p2*p3 乘积（素因子均为已证素数或确定性 u64 素数）。
      确定性拒绝侧（硬断言 ==0）
        - 偶数；小因子乘积（因子 <= 全档最小试除上界 100）；
        - 素数平方：D 搜索对平方数恒 (D|n) = +1 不命中 -1，必行至
          Dabs==17 的延迟 perfsqr（或素因子整除 D 的 gcd 出口）；
        - gcd(|D|,n) > 1：5|n 的乘积在首个 D=5 处 (5|n)=0 命中 j==0。
*/

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_prime_ref.hpp"

using namespace lmmp_test_utils;
using namespace lmmp_test_prime_ref;

/* 已证明素数：错拒即失败（不能错判）。全档位轮数差异已由小尺寸覆盖，
   此处抽 {0,4,7}（MR-only / BPSW / 最重混合档）控制耗时 */
TEST_CASE("numth/prime", is_prime_n_proven_primes) {
    mp_limb_t buf[512];
    for (int p : MP_EXP) {
        mp_size_t nn = mersenne_limbs(buf, p);
        for (int s : {0, 4, 7})
            TEST_CHECK_MSG(lmmp_is_prime_n_(buf, nn, s) == 2, "proven mersenne prime tiers");
    }
#ifdef NDEBUG
    /* 梅森折叠 REDC 层（312 limb >= REDC_MERSENNE_THRESHOLD=309）：单档 BPSW，
       秒级成本，仅 release 执行 */
    mp_size_t nn = mersenne_limbs(buf, MP_FOLD_EXP);
    TEST_CHECK_MSG(lmmp_is_prime_n_(buf, nn, 4) == 2, "proven mersenne prime fold layer");
#endif
}

/* notrial 变体：判定集合与全函数精确一致（试除仅承担合数侧廉价早拒）。
   小因子合数在全函数走试除拒、在 notrial 走 MR 拒，结论须相同；
   随机奇数流按档位全量对照 */
TEST_CASE("numth/prime", is_prime_n_notrial_agree) {
    const int STR[] = {0, 4, 7};
    mp_limb_t p[512], t1[512];

    /* 已证素数（Mersenne）：notrial 与全函数同为 2 */
    mp_size_t pn = mersenne_limbs(p, 521);
    for (int s : STR)
        TEST_CHECK_MSG(lmmp_is_prime_n_notrial_(p, pn, s) == lmmp_is_prime_n_(p, pn, s), "proven prime agree");

    /* 偶数：同拒（notrial 的奇偶短路） */
    lmmp_copy(t1, p, pn);
    lmmp_dec(t1);
    for (int s : STR)
        TEST_CHECK_MSG(lmmp_is_prime_n_notrial_(t1, pn, s) == lmmp_is_prime_n_(t1, pn, s), "even agree");

    /* 小因子 3 / 97：全函数试除拒，notrial MR 拒 */
    for (u64 f : {3ull, 97ull}) {
        mp_limb_t fb[1] = {(mp_limb_t)f};
        lmmp_mul_(t1, p, pn, fb, 1);
        mp_size_t fn = limbs_norm(t1, pn + 1);
        for (int s : STR)
            TEST_CHECK_MSG(lmmp_is_prime_n_notrial_(t1, fn, s) == lmmp_is_prime_n_(t1, fn, s),
                           "small factor agree");
    }

    /* 随机奇数流（3L/8L，含素数与各类合数）全量对照 */
    u64 seed = 0x6e6f747269616c6eull;
    for (mp_size_t n : {mp_size_t(3), mp_size_t(8)}) {
        mp_ptr a = alloc_limbs(n);
        for (int i = 0; i < 60; ++i) {
            for (mp_size_t j = 0; j < n; ++j) a[j] = xorshift64(seed);
            a[0] |= 1;
            for (int s : STR)
                TEST_CHECK_MSG(lmmp_is_prime_n_notrial_(a, n, s) == lmmp_is_prime_n_(a, n, s),
                               "random odd agree");
        }
        lmmp_free(a);
    }
}

/* 确定性拒绝：偶数 / 小因子（<= 全档最小试除上界 100）/ 素数平方（Lucas 半部） */
TEST_CASE("numth/prime", is_prime_n_deterministic_reject) {
    mp_limb_t p[512], t1[512], t2[1024];
    mp_size_t pn = mersenne_limbs(p, 521);

    /* 偶数：M521 - 1（奇偶位翻转，位数不变） */
    lmmp_copy(t1, p, pn);
    lmmp_dec(t1);
    for (int s = 0; s <= 7; ++s)
        TEST_CHECK_MSG(lmmp_is_prime_n_(t1, pn, s) == 0, "even rejected");

    /* 小因子 3 / 97：试除必拦（全档上界 >= 100） */
    for (u64 f : {3ull, 97ull}) {
        mp_limb_t fb[1] = {(mp_limb_t)f};
        lmmp_mul_(t1, p, pn, fb, 1);
        mp_size_t fn = limbs_norm(t1, pn + 1);
        for (int s = 0; s <= 7; ++s)
            TEST_CHECK_MSG(lmmp_is_prime_n_(t1, fn, s) == 0, "small factor rejected");
    }

    /* 素数平方：s >= 4 的 Lucas 半部（D 搜索必行至延迟 perfsqr/gcd 出口）
       硬断言 ==0；s <= 3 的纯 MR 轮按合数容许域处理（允许误判） */
    for (int pe : {521, 607}) {
        mp_size_t qn = mersenne_limbs(t1, pe);
        lmmp_sqr_(t2, t1, qn);
        mp_size_t sn = limbs_norm(t2, 2 * qn);
        for (int s = 0; s <= 7; ++s) {
            int r = lmmp_is_prime_n_(t2, sn, s);
            if (s >= 4)
                TEST_CHECK_MSG(r == 0, "prime square rejected by lucas tiers");
            else
                TEST_CHECK_MSG(r == 0 || r == 2, "prime square at MR tiers (misjudge allowed)");
        }
    }
}

/*
    强 Lucas 单测：素数必过（定理级，硬断言）；平方数与 gcd(|D|,n)>1
    必拒（D 搜索必经出口，硬断言）；一般构造合数允许误判
*/
TEST_CASE("numth/prime", is_strong_lucas_n) {
    mp_limb_t p[512], q[512], t1[1024], t2[1024];

    /* 已证素数必过（Mersenne，各 D 命中深度） */
    for (int e : MP_EXP) {
        mp_size_t pn = mersenne_limbs(p, e);
        TEST_CHECK_MSG(lmmp_is_strong_lucas_(p, pn) == 1, "lucas proven prime pass");
    }

    /* 素数平方必拒 */
    for (int e : {521, 607}) {
        mp_size_t qn = mersenne_limbs(q, e);
        lmmp_sqr_(t1, q, qn);
        mp_size_t sn = limbs_norm(t1, 2 * qn);
        TEST_CHECK_MSG(lmmp_is_strong_lucas_(t1, sn) == 0, "lucas square reject");
    }

    /* gcd(|D|,n) > 1 必拒：5|n 的乘积在首个 D=5 处 (5|n)=0 → j==0 */
    mp_size_t pn = mersenne_limbs(p, 521);
    for (u64 f : {5ull, 15ull, 35ull}) {
        mp_limb_t fb[1] = {(mp_limb_t)f};
        lmmp_mul_(t1, p, pn, fb, 1);
        mp_size_t fn = limbs_norm(t1, pn + 1);
        TEST_CHECK_MSG(lmmp_is_strong_lucas_(t1, fn) == 0, "lucas gcd-D reject");
    }

    /* 非平方含平方因子（p^2*q / p^3）与一般构造合数：允许误判（==1），
       覆盖阶梯终点判据与返回值合法性 */
    {
        mp_size_t qn = mersenne_limbs(q, 521);
        mp_size_t rn = mersenne_limbs(p, 607);
        lmmp_sqr_(t2, q, qn);                     /* p^2 */
        mp_size_t sn = limbs_norm(t2, 2 * qn);
        lmmp_mul_(t1, t2, sn, p, rn);             /* p^2 * q */
        int r = lmmp_is_strong_lucas_(t1, limbs_norm(t1, sn + rn));
        TEST_CHECK_MSG(r == 0 || r == 1, "lucas p^2*q (misjudge allowed)");
        lmmp_mul_(t1, t2, sn, q, qn);             /* p^3 */
        r = lmmp_is_strong_lucas_(t1, limbs_norm(t1, sn + qn));
        TEST_CHECK_MSG(r == 0 || r == 1, "lucas p^3 (misjudge allowed)");

        mp_size_t nn = k2t_limbs(t1, 128, 1, 1); /* F7 = 2^128+1 */
        r = lmmp_is_strong_lucas_(t1, nn);
        TEST_CHECK_MSG(r == 0 || r == 1, "lucas F7 (misjudge allowed)");

        nn = mersenne_limbs(t1, 137);            /* M137（Mersenne 合数） */
        r = lmmp_is_strong_lucas_(t1, nn);
        TEST_CHECK_MSG(r == 0 || r == 1, "lucas mersenne composite (misjudge allowed)");
    }
}

/*
    构造性 SPRP-2 与 Carmichael 对抗：基 2 轮构造性放行，随机轮/Lucas 按
    概率拒判——允许误判（返回 2 合法），仅拒绝崩溃与非法返回值。
*/
TEST_CASE("numth/prime", is_prime_n_constructive_spsp) {
    for (u64 n : {2047ull, 3277ull, 4033ull, 4681ull, 8321ull, 15841ull, 29341ull}) {
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(n) == false, "classic spsp2 rejected (ulong)");
        TEST_CHECK_MSG(lmmp_is_prime_ulong_notrial_(n) == false, "classic spsp2 rejected (notrial)");
    }

    mp_limb_t buf[512];
    /* Mersenne 合数（素指数 p ∈ (127,521)）：2^d ≡ 1 (mod M_p) 构造性
       通过基 2，只能由随机轮（或试除）拒判 */
    for (int p : MC_EXP) {
        mp_size_t nn = mersenne_limbs(buf, p);
        for (int s = 0; s <= 7; ++s) {
            int r = lmmp_is_prime_n_(buf, nn, s);
            if (p == 131 && s >= 1)
                TEST_CHECK_MSG(r == 0, "M131 factor 263 caught by trial bound >=300");
            else
                TEST_CHECK_MSG(r == 0 || r == 2, "mersenne composite (misjudge allowed)");
        }
    }

    /* Fermat 数 F7 = 2^128+1、F8 = 2^256+1（已知合数；d=1 平方链末端
       2^(2^k) ≡ -1 构造性通过基 2） */
    for (int k : {7, 8}) {
        mp_size_t nn = k2t_limbs(buf, 1 << k, 1, 1);
        for (int s = 0; s <= 7; ++s) {
            int r = lmmp_is_prime_n_(buf, nn, s);
            TEST_CHECK_MSG(r == 0 || r == 2, "fermat composite (misjudge allowed)");
        }
    }

    /* Carmichael 对抗语料（Korselt 线性同余 + CRT 构造，~190.6 bit）：
       p1p2p3 全 ≡ 3 (mod 8)、v2(n-1) = 1，2^((n-1)/2) ≡ -1 (mod n) 构造性
       通过基 2；随机基底单轮通过率 = 1/4（Monier-Rabin 最坏形态） */
    struct {
        mp_limb_t l[3];
        const char* fac;
    } carm[] = {
        {{0xff15f6ff84b4f0f3ull, 0xe106b441973e19bfull, 0x0e7000000110a524ull},
         "3458764513840342771*8070450532294133131*12682136550747923491"},
        {{0x66fe7ee3b284e7c3ull, 0x310b7aaab318c4a5ull, 0x0e7000000164c3bfull},
         "3458764513846452259*8070450532308388603*12682136550770324947"},
        {{0x3805ef3326bcc70bull, 0x7928975dd02cf97dull, 0x0e700000029ee088ull},
         "3458764513869265819*8070450532361620243*12682136550853974667"},
    };
    for (const auto& c : carm) {
        for (int s = 0; s <= 7; ++s) {
            int r = lmmp_is_prime_n_(c.l, 3, s);
            TEST_CHECK_MSG(r == 0 || r == 2, "carmichael adversarial (misjudge allowed)");
        }
    }
}

/* p1*p2 / p1*p2*p3 乘积合数：素因子均为已证素数或确定性 u64 素数，
   无小因子（试除不可达），允许概率性误判 */
TEST_CASE("numth/prime", is_prime_n_product_composites) {
    mp_limb_t a[512], b[512], t1[1024], t2[1024];
    mp_size_t an = mersenne_limbs(a, 521); /* 9 limb */
    mp_size_t bn = mersenne_limbs(b, 607); /* 10 limb */

    /* M521 * M607（半素数，大操作数在前） */
    lmmp_mul_(t1, b, bn, a, an);
    mp_size_t dn = limbs_norm(t1, an + bn);
    for (int s = 0; s <= 7; ++s) {
        int r = lmmp_is_prime_n_(t1, dn, s);
        TEST_CHECK_MSG(r == 0 || r == 2, "semiprime product (misjudge allowed)");
    }

    /* M521 * M607 * M1279（三素子积，抽样档位） */
    mp_size_t cn = mersenne_limbs(b, 1279); /* 20 limb */
    lmmp_mul_(t2, b, cn, t1, dn);
    mp_size_t en = limbs_norm(t2, cn + dn);
    for (int s : {0, 4, 7}) {
        int r = lmmp_is_prime_n_(t2, en, s);
        TEST_CHECK_MSG(r == 0 || r == 2, "triprime product (misjudge allowed)");
    }

    /* 随机 u64 素数三积（~2^190，3 limb） */
    u64 seed = 0x6d6f726e696e67ull;
    mp_limb_t f[3][2];
    for (int i = 0; i < 3; i++) {
        f[i][0] = random_u64_prime(seed);
        f[i][1] = 0;
    }
    lmmp_mul_(t1, f[0], 2, f[1], 2);
    lmmp_mul_(t2, t1, 4, f[2], 2);
    mp_size_t nn = (t2[3] != 0) ? 4 : 3;
    for (int s = 0; s <= 7; ++s) {
        int r = lmmp_is_prime_n_(t2, nn, s);
        TEST_CHECK_MSG(r == 0 || r == 2, "u64 triple product (misjudge allowed)");
    }
}

/*
    v2(n-1) 跨 limb 边界：n = 3*2^t+1（v2(n-1) = t 恰为给定值），t 取
    {127,128,129} / {191,192,193} / {255,256,257}（nn = 3/4/5）。素性经
    Proth 证书判定：认证为素数则硬断言全档 ==2，否则按合数容许域
*/
TEST_CASE("numth/prime", is_prime_n_v2_boundary) {
    mp_limb_t n[512];
    for (int t : {127, 128, 129, 191, 192, 193, 255, 256, 257}) {
        mp_size_t nn = k2t_limbs(n, t, 3, 1);
        bool prime = proth_certified(n, nn, t);
        for (int s = 0; s <= 7; ++s) {
            int r = lmmp_is_prime_n_(n, nn, s);
            if (prime)
                TEST_CHECK_MSG(r == 2, "proth-certified prime at v2 boundary");
            else
                TEST_CHECK_MSG(r == 0 || r == 2, "v2 boundary composite (misjudge allowed)");
        }
    }
}
