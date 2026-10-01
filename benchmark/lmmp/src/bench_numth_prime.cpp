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
#include "lmmp/mprand.h"
#include "lmmp/numth.h"
#include "lmmp_bench.hpp"

using namespace lmmp_bench;

namespace {

const int POOL = 512;

/*
    素性检验的性能基线。

    随机奇数输入：绝大多数为合数，基底2快速拒判；
    素数输入：全部基底完整执行，代表最坏情况（也是筛素数场景的主开销）。
    输入按 notril_ 内部分支的规模分档：40bit（双基底分支）、
    56bit（三基底 mont63 分支）、64bit 顶（三基底 mont64 分支）。
*/

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

// 生成 topbit 起始的随机奇数池
mp_ptr pool_odd(int bits, mp_limb_t seed) {
    mp_ptr a = alloc_limbs(POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + i, 1, seed + i, 1);
        a[i] &= ((mp_limb_t)-1) >> (64 - bits);
        a[i] |= (mp_limb_t)1 << (bits - 1);
        a[i] |= 1;
    }
    return a;
}

// 生成 topbit 起始的随机素数池（lmmp_next_prime 构造，保证全基底执行）
mp_ptr pool_prime(int bits, mp_limb_t seed) {
    mp_ptr a = pool_odd(bits, seed);
    for (int i = 0; i < POOL; i++) a[i] = lmmp_next_prime_ulong_(a[i]);
    return a;
}

}  // namespace

BENCH_CASE("numth/prime", notrial_random_40b) {
    mp_ptr a = pool_odd(40, 0x51ce25c0ff33ull);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_notrial_(a[idx++ & (POOL - 1)]); });
    report("notrial random 40bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", notrial_random_56b) {
    mp_ptr a = pool_odd(56, 0x25c0ff3351ceull);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_notrial_(a[idx++ & (POOL - 1)]); });
    report("notril random 56bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", notrial_random_64b) {
    mp_ptr a = pool_odd(64, 0xc0ff3351ce25ull);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_notrial_(a[idx++ & (POOL - 1)]); });
    report("notrial random 64bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", notrial_prime_40b) {
    mp_ptr a = pool_prime(40, 0x88aa77bb55ccull);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_notrial_(a[idx++ & (POOL - 1)]); });
    report("notrial prime 40bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", notrial_prime_56b) {
    mp_ptr a = pool_prime(56, 0x77bb55cc88aaull);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_notrial_(a[idx++ & (POOL - 1)]); });
    report("notrial prime 56bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", notrial_prime_64b) {
    mp_ptr a = pool_prime(64, 0x55cc88aa77bbull);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_notrial_(a[idx++ & (POOL - 1)]); });
    report("notrial prime 64bit", m);
    lmmp_free(a);
}

/* 128 比素性检验：hi 强制非零，按 <SWbound（13基底确定）与 >=SWbound
  （13基底+强Lucas）分档 */
BENCH_CASE("numth/prime", is_prime_2_random_65b) {
    mp_ptr a = alloc_limbs(2 * POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + 2 * i, 2, 0x65b0deadbeefull + i, 1);
        a[2 * i + 1] &= 1;
        a[2 * i + 1] |= 1;
        a[2 * i] |= 1;
    }
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_2_(a[2 * (idx & (POOL - 1))], a[2 * (idx & (POOL - 1)) + 1]); idx++; });
    report("is_prime_2 random 65bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", is_prime_2_random_82b) {
    mp_ptr a = alloc_limbs(2 * POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + 2 * i, 2, 0x82b1feedfaceull + i, 1);
        a[2 * i + 1] &= 0x3ffff;
        a[2 * i + 1] |= 0x20000;
        a[2 * i] |= 1;
    }
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_2_(a[2 * (idx & (POOL - 1))], a[2 * (idx & (POOL - 1)) + 1]); idx++; });
    report("is_prime_2 random 82bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", is_prime_2_random_96b) {
    mp_ptr a = alloc_limbs(2 * POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + 2 * i, 2, 0x96b2cafebab0ull + i, 1);
        a[2 * i + 1] &= 0xffffffffull;
        a[2 * i + 1] |= 0x80000000ull;
        a[2 * i] |= 1;
    }
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_2_(a[2 * (idx & (POOL - 1))], a[2 * (idx & (POOL - 1)) + 1]); idx++; });
    report("is_prime_2 random 96bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", is_prime_2_random_128b) {
    mp_ptr a = alloc_limbs(2 * POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + 2 * i, 2, 0xf0f0f0f0f0f0ull + i, 1);
        a[2 * i + 1] |= 0x8000000000000000ull;
        a[2 * i] |= 1;
    }
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_2_(a[2 * (idx & (POOL - 1))], a[2 * (idx & (POOL - 1)) + 1]); idx++; });
    report("is_prime_2 random 128bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", is_prime_2_prime_128b) {
    mp_ptr a = alloc_limbs(2 * POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + 2 * i, 2, 0xdeadbeefcafeull + i, 1);
        a[2 * i + 1] |= 0x8000000000000000ull;
        a[2 * i] |= 1;
        /* 构造素数：向后搜索 */
        while (lmmp_is_prime_2_(a[2 * i], a[2 * i + 1]) == 0) a[2 * i] += 2;
    }
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_2_(a[2 * (idx & (POOL - 1))], a[2 * (idx & (POOL - 1)) + 1]); idx++; });
    report("is_prime_2 prime 128bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", is_prime_ulong_random_64b) {
    mp_ptr a = pool_odd(64, 0x13579bdf0246ull);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_ulong_(a[idx++ & (POOL - 1)]); });
    report("is_prime_ulong random 64bit", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", is_prime_ulong_prime_64b) {
    mp_ptr a = pool_prime(64, 0x2468ace13579ull);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_ulong_(a[idx++ & (POOL - 1)]); });
    report("is_prime_ulong prime 64bit", m);
    lmmp_free(a);
}

/*
    >128 位素性检验性能：单轮 MR（基底 2 特化 vs 通用基底）、强 Lucas、
    is_prime_n_ 档位。

    随机奇数池：合数为主，试除幸存者支付完整基底 2 梯子（合数在梯子末端
    才被拒判，成本与通过者相近），代表平均负载；素数池：全部轮次完整执行
    的最坏情形。素数池以 lmmp_is_prime_n_ 自身搜索构造（仅性能用途），
    大尺寸下搜索成本高，仅取 <=64 limb。
*/

namespace {

mp_ptr pool_odd_n(size_t n, mp_limb_t seed, int cnt) {
    mp_ptr a = alloc_limbs(n * cnt);
    for (int i = 0; i < cnt; i++) {
        lmmp_seed_random_(a + n * i, (mp_size_t)n, seed + i, 1);
        a[n * i + n - 1] |= (mp_limb_t)1 << 63;
        a[n * i] |= 1;
    }
    return a;
}

mp_ptr pool_prime_n(size_t n, mp_limb_t seed, int cnt) {
    mp_ptr a = pool_odd_n(n, seed, cnt);
    for (int i = 0; i < cnt; i++)
        while (lmmp_is_prime_n_(a + n * i, (mp_size_t)n, 4) == 0) a[n * i] += 2;
    return a;
}

/* 试除幸存者池（无 <=1000 的素因子）：对应 is_prime_n_ 强度 4 档真实
   输入的幸存者总体。强 Lucas 对含小因子输入会经 Jacobi gcd 探测提前
   退出（微秒级，D=9 即覆盖 3|n），is_prime_n_ 的试除亦然；且 measure()
   取多样本最小值，池内混入任一快速元素都会掩盖完整负载——须过滤到
   试除上界之上，保证池内齐次 */
mp_ptr pool_coprime_n(size_t n, mp_limb_t seed, int cnt) {
    mp_ptr a = alloc_limbs(n * cnt);
    for (int i = 0; i < cnt; i++) {
        mp_ptr cur = a + n * i;
        lmmp_seed_random_(cur, (mp_size_t)n, seed + i * 131, 1);
        cur[n - 1] |= (mp_limb_t)1 << 63;
        cur[0] |= 1;
        ushort rn;
        ushortp divs;
        while ((divs = lmmp_trialdiv_(cur, (mp_size_t)n, 1000, &rn)) != NULL) {
            lmmp_free(divs);
            cur[0] += 2;
        }
    }
    return a;
}

}  // namespace

#define BENCH_IPN_ONE(limbs)                                                          \
    BENCH_CASE("numth/prime", ipn_sprp2_##limbs##_l) {                                \
        const size_t n = (limbs);                                                     \
        const int cnt = (limbs) <= 64 ? 32 : 8;                                       \
        mp_ptr a = pool_odd_n(n, 0x51ce77aa##limbs##ull, cnt);                        \
        mp_ptr b2 = alloc_limbs(n);                                                   \
        lmmp_zero(b2, (mp_size_t)n);                                                  \
        b2[0] = 2;                                                                    \
        size_t idx = 0;                                                               \
        auto m = measure([&] {                                                        \
            lmmp_is_sprp_(a + n * (idx++ % cnt), (mp_size_t)n, b2);                   \
        });                                                                           \
        report("is_sprp base2 random n=" #limbs, m);                                  \
        lmmp_free(a);                                                                 \
        lmmp_free(b2);                                                                \
    }

#define BENCH_IPN(limbs)                                                              \
    BENCH_IPN_ONE(limbs)                                                              \
    BENCH_CASE("numth/prime", ipn_sprpg_##limbs##_l) {                                \
        const size_t n = (limbs);                                                     \
        const int cnt = (limbs) <= 64 ? 32 : 8;                                       \
        mp_ptr a = pool_odd_n(n, 0x51ce77ab##limbs##ull, cnt);                        \
        mp_ptr bg = alloc_limbs(n);                                                   \
        lmmp_zero(bg, (mp_size_t)n);                                                  \
        bg[0] = 0x9e3779b97f4a7c15ull;                                                \
        size_t idx = 0;                                                               \
        auto m = measure([&] {                                                        \
            lmmp_is_sprp_(a + n * (idx++ % cnt), (mp_size_t)n, bg);                   \
        });                                                                           \
        report("is_sprp generic random n=" #limbs, m);                                \
        lmmp_free(a);                                                                 \
        lmmp_free(bg);                                                                \
    }                                                                                 \
    BENCH_CASE("numth/prime", ipn_lucas_##limbs##_l) {                                \
        const size_t n = (limbs);                                                     \
        const int cnt = (limbs) <= 64 ? 32 : 8;                                       \
        mp_ptr a = pool_coprime_n(n, 0x51ce77ac##limbs##ull, cnt);                    \
        size_t idx = 0;                                                               \
        auto m = measure([&] {                                                        \
            lmmp_is_strong_lucas_(a + n * (idx++ % cnt), (mp_size_t)n);               \
        });                                                                           \
        report("strong_lucas coprime n=" #limbs, m);                                  \
        lmmp_free(a);                                                                 \
    }                                                                                 \
    BENCH_CASE("numth/prime", ipn_s4_##limbs##_l) {                                   \
        const size_t n = (limbs);                                                     \
        const int cnt = (limbs) <= 64 ? 32 : 8;                                       \
        mp_ptr a = pool_coprime_n(n, 0x51ce77ad##limbs##ull, cnt);                    \
        size_t idx = 0;                                                               \
        auto m = measure([&] {                                                        \
            lmmp_is_prime_n_(a + n * (idx++ % cnt), (mp_size_t)n, 4);                 \
        });                                                                           \
        report("is_prime_n s4 coprime n=" #limbs, m);                                 \
        lmmp_free(a);                                                                 \
    }

BENCH_IPN(3)
BENCH_IPN(8)
BENCH_IPN(16)
BENCH_IPN(32)
BENCH_IPN(64)
BENCH_IPN(200)
BENCH_IPN(1000)

/* 素数池最坏情形与强度档位扩展 */
BENCH_CASE("numth/prime", ipn_s4_prime_8l) {
    const size_t n = 8;
    const int cnt = 32;
    mp_ptr a = pool_prime_n(n, 0x88aa77bb01ull, cnt);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_n_(a + n * (idx++ % cnt), (mp_size_t)n, 4); });
    report("is_prime_n s4 prime 8l", m);
    lmmp_free(a);
}

BENCH_CASE("numth/prime", ipn_s4_prime_64l) {
    const size_t n = 64;
    const int cnt = 16;
    mp_ptr a = pool_prime_n(n, 0x88aa77bb64ull, cnt);
    size_t idx = 0;
    auto m = measure([&] { lmmp_is_prime_n_(a + n * (idx++ % cnt), (mp_size_t)n, 4); });
    report("is_prime_n s4 prime 64l", m);
    lmmp_free(a);
}

#define BENCH_IPN_TIER(strength, tag)                                                 \
    BENCH_CASE("numth/prime", ipn_s##tag##_prime_8l) {                                \
        const size_t n = 8;                                                           \
        const int cnt = 32;                                                           \
        mp_ptr a = pool_prime_n(n, 0x99bb88cc##tag##ull, cnt);                        \
        size_t idx = 0;                                                               \
        auto m = measure([&] {                                                        \
            lmmp_is_prime_n_(a + n * (idx++ % cnt), (mp_size_t)n, strength);          \
        });                                                                           \
        report("is_prime_n s" #tag " prime 8l", m);                                   \
        lmmp_free(a);                                                                 \
    }

BENCH_IPN_TIER(0, 0)
BENCH_IPN_TIER(2, 2)
BENCH_IPN_TIER(7, 7)
