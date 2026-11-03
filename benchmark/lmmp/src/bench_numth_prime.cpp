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

#include "lmmp/lmmp.h"
#include "lmmp/lmmpn.h"
#include "lmmp/mprand.h"
#include "lmmp/numth.h"
#include "lmmp_bench.hpp"
#include "lmmp_bench_lib.hpp"

namespace lmmp_bench {

namespace {

/*
    素性检验的输入池：单次判定成本对输入性质极度敏感（小因子合数
    试除即拒、素数全基底执行），故以池轮转代替单输入，保证测得
    目标总体的齐次负载：
      ODD     随机奇数（合数为主，平均负载）
      PRIME   素数（全轮完整执行的最坏情形）
      COPRIME 试除幸存者（无 <=1000 素因子；强 Lucas 对含小因子输入
              经 Jacobi gcd 提前退出，且 measure 取最小值会被任一
              快速元素掩盖，须过滤到试除上界之上保证池内齐次）
*/

constexpr int POOL = 512;

int pool_cnt(mp_size_t n) { return n <= 64 ? 32 : 8; }

/* 顶位起始的随机奇数池（<=64bit），元素个数 POOL */
mp_ptr pool_bits(uint bits, mp_limb_t seed) {
    if (bits < 1) bits = 1;
    if (bits > 64) bits = 64;
    mp_ptr a = alloc_limbs(POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + i, 1, seed + i, 1);
        if (bits < 64) a[i] &= ((mp_limb_t)-1) >> (64 - bits);
        a[i] |= (mp_limb_t)1 << (bits - 1);
        a[i] |= 1;
    }
    return a;
}

/* 素数池（lmmp_next_prime_ulong_ 构造，保证全基底执行） */
mp_ptr pool_bits_prime(uint bits, mp_limb_t seed) {
    mp_ptr a = pool_bits(bits, seed);
    for (int i = 0; i < POOL; i++) a[i] = lmmp_next_prime_ulong_(a[i]);
    return a;
}

/* [lo,hi] 对池（65..128bit），hi 顶位起始 */
mp_ptr pool_2(uint bits, mp_limb_t seed) {
    if (bits < 65) bits = 65;
    if (bits > 128) bits = 128;
    uint hb = bits - 64;
    mp_ptr a = alloc_limbs(2 * POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + 2 * i, 2, seed + i, 1);
        a[2 * i + 1] &= ((mp_limb_t)-1) >> (64 - hb);
        a[2 * i + 1] |= (mp_limb_t)1 << (hb - 1);
        a[2 * i] |= 1;
    }
    return a;
}

/* n limbs 归一化奇数池（元素个数 cnt） */
mp_ptr pool_n(mp_size_t n, int cnt, mp_limb_t seed, PrimePool kind) {
    mp_ptr a = alloc_limbs((size_t)n * cnt);
    for (int i = 0; i < cnt; i++) {
        mp_ptr cur = a + n * i;
        lmmp_seed_random_(cur, n, seed + i * 131, 1);
        cur[n - 1] |= (mp_limb_t)1 << 63;
        cur[0] |= 1;
        if (kind == PrimePool::COPRIME) {
            while (lmmp_trialdiv_(cur, n, 1000)) cur[0] += 2;
        } else if (kind == PrimePool::PRIME) {
            while (lmmp_is_prime_n_(cur, n, 4) == 0) cur[0] += 2;
        }
    }
    return a;
}

/* ---- Proth 形状：N = k*2^t + 1（k<=2^32） ---- */

mp_size_t proth_limbs(mp_ptr dst, uint k, uint t) {
    mp_size_t n = (mp_size_t)t / 64 + 2;
    lmmp_zero(dst, n + 1);
    uint sh = t % 64;
    dst[t / 64] |= (mp_limb_t)k << sh;
    if (sh != 0) dst[t / 64 + 1] |= (mp_limb_t)k >> (64 - sh);
    dst[0] |= 1;
    while (n > 1 && dst[n - 1] == 0) n--;
    return n;
}

struct ProthPool {
    mp_ptr a;         /* cnt * stride，元素为 k*2^t+1 */
    uint ks[8];       /* 各元素的 k */
    mp_size_t nn[8];  /* 各元素规范化 limb 数 */
    mp_size_t stride;
    int cnt;
};

/* want_prime：Proth 素数池（被测函数搜索）；否则试除幸存合数池 */
ProthPool pool_proth(uint t, int cnt, bool want_prime) {
    ProthPool p;
    p.stride = (mp_size_t)t / 64 + 3;
    p.cnt = cnt;
    p.a = alloc_limbs((size_t)p.stride * cnt);
    uint k = 3;
    for (int i = 0; i < cnt; i++) {
        for (;;) {
            mp_size_t nn = proth_limbs(p.a + p.stride * i, k, t);
            bool ok = want_prime ? lmmp_is_prothprime_(k, (mp_size_t)t)
                                 : !lmmp_trialdiv_(p.a + p.stride * i, nn, 1000);
            if (ok) break;
            k += 2;
        }
        p.ks[i] = k;
        p.nn[i] = proth_limbs(p.a + p.stride * i, k, t);
        k += 2;
    }
    return p;
}

}  // namespace

double bench_is_prime_notrial(uint bits, PrimePool kind, const BenchOpts& o) {
    mp_ptr a = kind == PrimePool::PRIME ? pool_bits_prime(bits, 0x51ce25c0ff33ull + bits)
                                        : pool_bits(bits, 0x51ce25c0ff33ull + bits);
    size_t idx = 0;
    double us = measure([&] { sink += (mp_limb_t)lmmp_is_prime_notrial_(a[idx++ & (POOL - 1)]); }, o);
    lmmp_free(a);
    return us;
}

double bench_is_prime_ulong(uint bits, PrimePool kind, const BenchOpts& o) {
    mp_ptr a = kind == PrimePool::PRIME ? pool_bits_prime(bits, 0x13579bdf0246ull + bits)
                                        : pool_bits(bits, 0x13579bdf0246ull + bits);
    size_t idx = 0;
    double us = measure([&] { sink += (mp_limb_t)lmmp_is_prime_ulong_(a[idx++ & (POOL - 1)]); }, o);
    lmmp_free(a);
    return us;
}

double bench_is_prime_2(uint bits, PrimePool kind, const BenchOpts& o) {
    if (bits < 65) bits = 65;
    if (bits > 128) bits = 128;
    mp_ptr a = pool_2(bits, 0xdeadbeefca01ull + bits);
    if (kind == PrimePool::PRIME)
        for (int i = 0; i < POOL; i++)
            while (lmmp_is_prime_2_(a[2 * i], a[2 * i + 1]) == 0) a[2 * i] += 2;
    size_t idx = 0;
    double us = measure(
        [&] {
            sink += (mp_limb_t)lmmp_is_prime_2_(a[2 * (idx & (POOL - 1))],
                                                a[2 * (idx & (POOL - 1)) + 1]);
            idx++;
        },
        o);
    lmmp_free(a);
    return us;
}

double bench_is_prime_n(mp_size_t n, int strength, PrimePool kind, const BenchOpts& o) {
    int cnt = pool_cnt(n);
    mp_ptr a = pool_n(n, cnt, 0x51ce77ac01ull + (mp_limb_t)strength * 131 + n, kind);
    size_t idx = 0;
    double us = measure(
        [&] {
            sink += (mp_limb_t)lmmp_is_prime_n_(a + n * (idx++ % cnt), n, strength);
        },
        o);
    lmmp_free(a);
    return us;
}

double bench_is_strong_lucas(mp_size_t n, PrimePool kind, const BenchOpts& o) {
    int cnt = pool_cnt(n);
    mp_ptr a = pool_n(n, cnt, 0x51ce77ac02ull + n, kind);
    size_t idx = 0;
    double us = measure(
        [&] { sink += (mp_limb_t)lmmp_is_strong_lucas_(a + n * (idx++ % cnt), n); }, o);
    lmmp_free(a);
    return us;
}

double bench_is_prothprime(uint t, bool prime_pool, const BenchOpts& o) {
    ProthPool p = pool_proth(t, 8, prime_pool);
    size_t idx = 0;
    double us = measure([&] { sink += (mp_limb_t)lmmp_is_prothprime_(p.ks[idx++ % p.cnt], t); }, o);
    lmmp_free(p.a);
    return us;
}

double bench_is_prime_n_proth(uint t, int strength, bool prime_pool, const BenchOpts& o) {
    ProthPool p = pool_proth(t, 8, prime_pool);
    size_t idx = 0;
    double us = measure(
        [&] {
            sink += (mp_limb_t)lmmp_is_prime_n_(p.a + p.stride * (idx % p.cnt),
                                                p.nn[idx % p.cnt], strength);
            idx++;
        },
        o);
    lmmp_free(p.a);
    return us;
}

}  // namespace lmmp_bench

/* ---- 扫描用例 ---- */

using namespace lmmp_bench;

/*
    64bit 内档位对应 notrial_/ulong_ 内部分支：40bit（双基底分支）、
    56bit（三基底 mont63 分支）、64bit 顶（三基底 mont64 分支）；
    is_prime_2 档位对应 <psi_13（13 基底确定）与 >=psi_13（+强 Lucas）。
*/
BENCH_CASE("numth/prime", notrial) {
    const uint BITS[] = {40, 56, 64};
    for (uint bits : BITS) {
        report_bits("notrial odd", bits, bench_is_prime_notrial(bits, PrimePool::ODD));
        report_bits("notrial prime", bits, bench_is_prime_notrial(bits, PrimePool::PRIME));
    }
}

BENCH_CASE("numth/prime", is_prime_ulong) {
    const uint BITS[] = {40, 56, 64};
    for (uint bits : BITS) {
        report_bits("is_prime_ulong odd", bits, bench_is_prime_ulong(bits, PrimePool::ODD));
        report_bits("is_prime_ulong prime", bits, bench_is_prime_ulong(bits, PrimePool::PRIME));
    }
}

BENCH_CASE("numth/prime", is_prime_2) {
    const uint BITS[] = {65, 82, 96, 128};
    for (uint bits : BITS) report_bits("is_prime_2 odd", bits, bench_is_prime_2(bits, PrimePool::ODD));
    report_bits("is_prime_2 prime", 128, bench_is_prime_2(128, PrimePool::PRIME));
}

/*
    MR 轮的指数为模数全长，单 op 成本随 n^3 增长（200L 已 ~1s），
    默认阶梯到 200L 止；更大尺寸用 bench_is_prime_n(n, ...) 单点测量。
*/
BENCH_CASE("numth/prime", is_prime_n) {
    const mp_size_t SZ[] = {3, 8, 16, 32, 64, 200};
    for (mp_size_t n : SZ) report_n("is_prime_n s4 coprime", n, bench_is_prime_n(n, 4, PrimePool::COPRIME));
}

/* 素数池最坏情形与强度档位扩展（素数池构造昂贵，仅取小尺寸） */
BENCH_CASE("numth/prime", is_prime_n_prime) {
    const mp_size_t SZ[] = {8, 64};
    for (mp_size_t n : SZ) report_n("is_prime_n s4 prime", n, bench_is_prime_n(n, 4, PrimePool::PRIME));
}

BENCH_CASE("numth/prime", is_prime_n_tier) {
    const int STR[] = {0, 2, 4, 7};
    for (int s : STR) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "is_prime_n s%d prime(8L)", s);
        report(buf, bench_is_prime_n(8, s, PrimePool::PRIME));
    }
}

BENCH_CASE("numth/prime", strong_lucas) {
    const mp_size_t SZ[] = {3, 8, 16, 32, 64, 200};
    for (mp_size_t n : SZ) report_n("strong_lucas coprime", n, bench_is_strong_lucas(n, PrimePool::COPRIME));
}

/*
    Proth 形状：lmmp_is_prothprime_ 单梯子定理判决 vs lmmp_is_prime_n_
    通用分档（同池对照）。素数池 = 分档全轮完整执行的最坏情形（Proth
    路径的收益所在）；合数池 = 试除幸存者，两路径都支付完整梯子，
    用于确认 Proth 路径在合数侧无回归。素数池构造昂贵，仅取 <=16 limb。
*/
BENCH_CASE("numth/prime", proth) {
    const uint TS[] = {480, 992};
    for (uint t : TS) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "is_prothprime prime(t=%u)", t);
        report(buf, bench_is_prothprime(t, true));
        std::snprintf(buf, sizeof(buf), "is_prime_n s4 proth-prime(t=%u)", t);
        report(buf, bench_is_prime_n_proth(t, 4, true));
    }
    const uint TC[] = {2016, 4032};
    for (uint t : TC) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "is_prothprime composite(t=%u)", t);
        report(buf, bench_is_prothprime(t, false));
        std::snprintf(buf, sizeof(buf), "is_prime_n s4 proth-composite(t=%u)", t);
        report(buf, bench_is_prime_n_proth(t, 4, false));
    }
}
