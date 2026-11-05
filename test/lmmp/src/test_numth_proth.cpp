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

/* ============ lmmp_is_prothprime_：普罗斯定理单梯子确定性判决 ============

    - 小域（N < 2^64）：ref_is_prime64 穷举 + 随机，双向硬断言；
    - 驱动域（N >= 2^64）：proth_cert_g 证书器走 lmmp_powmod_ 通用梯子，
      与被测路径实现独立；
    - 构造合数（硬断言 false）：Fermat 数、奇指数代数分解、
      小因子 Jacobi=0 出口。
*/

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_prime_ref.hpp"

using namespace lmmp_test_utils;
using namespace lmmp_test_prime_ref;

TEST_CASE("numth/proth", is_prothprime_small) {
    u64 seed = 0x70726f7468313233ull; /* "proth123" */
    /* 穷举：奇 k ∈ [1,399] × n ∈ [1,20]（契约域 k>0 奇、n>0） */
    for (u32 k = 1; k < 400; k += 2) {
        for (u32 n = 1; n <= 20; ++n) {
            u64 N = ((u64)k << n) + 1;
            TEST_CHECK_MSG(lmmp_is_prothprime_(k, (mp_size_t)n) == (ref_is_prime64(N) ? 1 : 0),
                           "proth small exhaustive");
        }
    }
    /* 随机：全域 32 位奇 k × n ∈ [1,32]（N < 2^64 不溢出） */
    for (int i = 0; i < 800; ++i) {
        u32 k = (u32)xorshift64(seed) | 1;
        u32 n = 1 + (u32)(xorshift64(seed) % 32);
        u64 N = ((u64)k << n) + 1;
        TEST_CHECK_MSG(lmmp_is_prothprime_(k, (mp_size_t)n) == (ref_is_prime64(N) ? 1 : 0),
                       "proth small random");
    }
    /* 跨 64 位边界：ne+bitlen(kk)=64 走 ulong 档 */
    TEST_CHECK_MSG(lmmp_is_prothprime_(1, 63) == (ref_is_prime64((1ull << 63) + 1) ? 1 : 0),
                   "proth ulong boundary 2^63+1");
    TEST_CHECK_MSG(lmmp_is_prothprime_(3, 62) == (ref_is_prime64((3ull << 62) + 1) ? 1 : 0),
                   "proth ulong boundary 3*2^62+1");
}

TEST_CASE("numth/proth", is_prothprime_driver) {
    u64 seed = 0x6b33706137686975ull;
    /* 中尺寸双向对拍：t=33/34 顶值 k 使 nn=2（双 limb 蒙域），65 起恒
       驱动域；k=3 分量保证 Jacobi=+1 跳过路径 */
    for (int t : {33, 34, 40, 65, 100, 150}) {
        for (int i = 0; i < 12; ++i) {
            u32 k;
            if (i < 3) k = 3;
            else if (i < 6) k = 0xffffffffu;
            else k = (u32)(xorshift64(seed) | 1);
            int lm = lmmp_is_prothprime_(k, t);
            bool ce = proth_cert_g(k, t);
            if (t >= 64) /* 定理驱动域：精确 0/1（契约 k 奇 ⟹ ne = t） */
                TEST_CHECK_MSG(lm == (ce ? 1 : 0), "proth driver exact (ne>=64)");
            else /* 双 limb/ulong 段：素数非零，合数 0（BPSW 误判 2 容许） */
                TEST_CHECK_MSG(ce ? lm != 0 : (lm == 0 || lm == 2),
                               "proth two-limb tri-state (ne<64)");
        }
    }

    /* 大尺寸：被测预筛（单梯子），命中即证书复核；抽样合数反向验证
       （证书有 ⟹ 被测必 true，硬断言）。found 硬断言仅限密度充裕档 */
    struct { int t; int cap; bool hard; } bigs[] = {
        {200, 400, true}, {384, 200, false}, {521, 150, false},
        {1000, 100, false}, {2048, 60, false},
    };
    /* 固定正例锚点（证书已知素）：被测恒返回 0 的回归在抽样合数分支
       （k&63==3）大概率无素例可撞、found 硬断言又只保 t=200 档 */
    TEST_CHECK_MSG(lmmp_is_prothprime_(233, 521) == 1, "proth anchor 233*2^521+1 prime");
    TEST_CHECK_MSG(proth_cert_g(233, 521), "proth anchor 233*2^521+1 certified");
    TEST_CHECK_MSG(lmmp_is_prothprime_(13, 1000) == 1, "proth anchor 13*2^1000+1 prime");
    TEST_CHECK_MSG(proth_cert_g(13, 1000), "proth anchor 13*2^1000+1 certified");
    for (const auto& b : bigs) {
        int found = 0;
        for (u32 k = 3; (int)k < 2 * b.cap; k += 2) {
            if (lmmp_is_prothprime_(k, b.t)) {
                TEST_CHECK_MSG(proth_cert_g(k, b.t), "proth big prime certified");
                found++;
            } else if ((k & 63) == 3) {
                TEST_CHECK_MSG(!proth_cert_g(k, b.t), "proth big composite consistent");
            }
        }
        if (b.hard) TEST_CHECK_MSG(found >= 1, "proth big prime found");
    }

#ifdef NDEBUG
    /* 梅森折叠 REDC 档（nn=313 >= REDC_MERSENNE_THRESHOLD=309）：单例，
       秒级成本，仅 release 执行。合数时以单基短证书反向验证 */
    bool lm = lmmp_is_prothprime_(3, 19969) != 0;
    if (lm)
        TEST_CHECK_MSG(proth_cert_g(3, 19969), "proth fold prime certified");
    else
        TEST_CHECK_MSG(!proth_cert_g(3, 19969, 1), "proth fold composite consistent");
#endif
}

TEST_CASE("numth/proth", is_prothprime_constructed) {
    /* Fermat 合数（k=1，已证 F6..F9 均合）：nn 覆盖 2/3/5/9 limb */
    for (int t : {64, 128, 256, 512})
        TEST_CHECK_MSG(!lmmp_is_prothprime_(1, t), "fermat composite");

    /* 奇指数代数分解：2^odd ≡ -1 (mod 3) ⟹ 3 | 2^189+1 */
    TEST_CHECK_MSG(!lmmp_is_prothprime_(1, 189), "algebraic factor 3");

    /* 小因子 Jacobi=0 出口：t 偶 ⟹ 2^t ≡ 1 (mod 3)，k ≡ 2 (mod 3) 即
       3 | k*2^t+1，首个 a=3 即命中 gcd 出口 */
    TEST_CHECK_MSG(!lmmp_is_prothprime_(5, 100), "jacobi-zero factor 3");
}
