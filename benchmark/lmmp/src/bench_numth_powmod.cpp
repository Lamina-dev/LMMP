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

/*
    奇模数模幂性能：Montgomery REDC + 滑动窗口。

    输入取 RSA 典型形态：指数与模数同规模（ebits ~ n*64），
    尺寸跨越 REDC_MERSENNE_THRESHOLD(401) 两侧，覆盖
    全积取高半与梅森折叠两条归约路径。
*/

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void fill_powmod_inputs(mp_ptr m, mp_ptr b, mp_ptr e, size_t n, size_t en, mp_limb_t seed) {
    lmmp_seed_random_(m, (mp_size_t)n, seed, 1);
    m[0] |= 1;
    m[n - 1] |= (mp_limb_t)1 << 63;
    lmmp_seed_random_(b, (mp_size_t)n, seed * 3 + 1, 1);
    lmmp_div_(NULL, b, b, (mp_size_t)n, m, (mp_size_t)n);
    lmmp_seed_random_(e, (mp_size_t)en, seed * 7 + 2, 1);
    e[en - 1] |= (mp_limb_t)1 << 63;
}

}  // namespace

#define BENCH_POWMOD(limbs)                                                                     \
    BENCH_CASE("numth/powmod", powmod_##limbs##_l) {                                            \
        const size_t n = (limbs);                                                               \
        mp_ptr m = alloc_limbs(n), b = alloc_limbs(n), e = alloc_limbs(n);                      \
        mp_ptr dst = alloc_limbs(n);                                                            \
        fill_powmod_inputs(m, b, e, n, n, 0x51ce77aa##limbs##ull);                              \
        auto mm = measure([&] { lmmp_powmod_odd_(dst, b, e, (mp_size_t)n, m, (mp_size_t)n); }); \
        report("powmod_odd n=" #limbs " e=n", mm);                                              \
        lmmp_free(m); lmmp_free(b); lmmp_free(e); lmmp_free(dst);                               \
    }

BENCH_POWMOD(1)
BENCH_POWMOD(8)
BENCH_POWMOD(64)
BENCH_POWMOD(200)
BENCH_POWMOD(400)
BENCH_POWMOD(402)
BENCH_POWMOD(600)
BENCH_POWMOD(1000)
BENCH_POWMOD(2000)

/*
    偶模数模幂性能：CRT 结构 = 奇部 REDC 梯子 + 2^k 部 powlo + 一次合成。
    取 m = 2*odd（k=1，nb2=1）与深移位 m 含 2^65 因子（nb2=2）两种形态，
    尺寸档与奇模数档对齐便于直接对照。e ~ 64n 时偶底数结果恒 0（2^k 部
    短路），故底数强制取奇以衡量典型负载。
*/
#define BENCH_POWMOD_EVEN(limbs)                                                            \
    BENCH_CASE("numth/powmod", powmod_even_##limbs##_l) {                                   \
        const size_t n = (limbs);                                                           \
        mp_ptr m = alloc_limbs(n), b = alloc_limbs(n), e = alloc_limbs(n);                  \
        mp_ptr dst = alloc_limbs(n);                                                        \
        lmmp_seed_random_(m, (mp_size_t)n, 0x51ce77ac##limbs##ull, 1);                      \
        m[0] = (m[0] << 1) | 2; /* k=1: m = 2*odd */                                        \
        m[n - 1] |= (mp_limb_t)1 << 63;                                                     \
        lmmp_seed_random_(b, (mp_size_t)n, 0x51ce77ac##limbs##ull * 3 + 1, 1);              \
        lmmp_div_(NULL, b, b, (mp_size_t)n, m, (mp_size_t)n);                               \
        b[0] |= 1;                                                                          \
        lmmp_seed_random_(e, (mp_size_t)n, 0x25c0ff35##limbs##ull, 1);                      \
        e[n - 1] |= (mp_limb_t)1 << 63;                                                     \
        auto mm = measure([&] { lmmp_powmod_(dst, b, e, (mp_size_t)n, m, (mp_size_t)n); }); \
        report("powmod_even n=" #limbs " e=n", mm);                                         \
        lmmp_free(m); lmmp_free(b); lmmp_free(e); lmmp_free(dst);                           \
    }

#define BENCH_POWMOD_EVEN2(limbs)                                                           \
    BENCH_CASE("numth/powmod", powmod_even2_##limbs##_l) {                                  \
        const size_t n = (limbs);                                                           \
        mp_ptr m = alloc_limbs(n), b = alloc_limbs(n), e = alloc_limbs(n);                  \
        mp_ptr dst = alloc_limbs(n);                                                        \
        lmmp_seed_random_(m, (mp_size_t)n, 0x51ce77ad##limbs##ull, 1);                      \
        m[0] = 0;                                                                           \
        m[1] = 2; /* k=65: m = 2^65*odd, nb2=2 */                                           \
        m[n - 1] |= (mp_limb_t)1 << 63;                                                     \
        lmmp_seed_random_(b, (mp_size_t)n, 0x51ce77ad##limbs##ull * 3 + 1, 1);              \
        lmmp_div_(NULL, b, b, (mp_size_t)n, m, (mp_size_t)n);                               \
        b[0] |= 1;                                                                          \
        lmmp_seed_random_(e, (mp_size_t)n, 0x25c0ff36##limbs##ull, 1);                      \
        e[n - 1] |= (mp_limb_t)1 << 63;                                                     \
        auto mm = measure([&] { lmmp_powmod_(dst, b, e, (mp_size_t)n, m, (mp_size_t)n); }); \
        report("powmod_even2 n=" #limbs " e=n", mm);                                        \
        lmmp_free(m); lmmp_free(b); lmmp_free(e); lmmp_free(dst);                           \
    }

BENCH_POWMOD_EVEN(1)
BENCH_POWMOD_EVEN(8)
BENCH_POWMOD_EVEN(64)
BENCH_POWMOD_EVEN(200)
BENCH_POWMOD_EVEN(400)
BENCH_POWMOD_EVEN(402)
BENCH_POWMOD_EVEN(600)
BENCH_POWMOD_EVEN(1000)
BENCH_POWMOD_EVEN(2000)

BENCH_POWMOD_EVEN2(8)
BENCH_POWMOD_EVEN2(64)
BENCH_POWMOD_EVEN2(400)
BENCH_POWMOD_EVEN2(1000)

#define BENCH_POWLO(limbs)                                                              \
    BENCH_CASE("numth/powlo", powlo_##limbs##_l) {                                      \
        const size_t n = (limbs);                                                       \
        mp_ptr b = alloc_limbs(n), e = alloc_limbs(n);                                  \
        mp_ptr dst = alloc_limbs(n);                                                    \
        lmmp_seed_random_(b, (mp_size_t)n, 0x51ce77ab##limbs##ull, 1);                  \
        b[0] |= 1;                                                                      \
        lmmp_seed_random_(e, (mp_size_t)n, 0x25c0ff34##limbs##ull, 1);                  \
        e[n - 1] |= (mp_limb_t)1 << 63;                                                 \
        auto mm = measure([&] { lmmp_powlo_(dst, b, (mp_size_t)n, e, (mp_size_t)n); }); \
        report("powlo n=" #limbs " e=n", mm);                                           \
        lmmp_free(b); lmmp_free(e); lmmp_free(dst);                                     \
    }

BENCH_POWLO(1)
BENCH_POWLO(2)
BENCH_POWLO(8)
BENCH_POWLO(64)
BENCH_POWLO(200)
BENCH_POWLO(400)
BENCH_POWLO(1000)
