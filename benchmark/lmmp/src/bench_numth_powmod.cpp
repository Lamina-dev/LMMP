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
    奇模数模幂性能：Montgomery REDC + 滑动窗口。

    输入取 RSA 典型形态：指数与模数同规模（ebits ~ n*64），
    尺寸跨越 REDC_MERSENNE_THRESHOLD(401) 两侧，覆盖
    全积取高半与梅森折叠两条归约路径。
*/
void fill_odd_mod(mp_ptr m, mp_ptr b, mp_ptr e, size_t n, mp_limb_t seed) {
    lmmp_seed_random_(m, (mp_size_t)n, seed, 1);
    m[0] |= 1;
    m[n - 1] |= (mp_limb_t)1 << 63;
    lmmp_seed_random_(b, (mp_size_t)n, seed * 3 + 1, 1);
    lmmp_div_(NULL, b, b, (mp_size_t)n, m, (mp_size_t)n);
    lmmp_seed_random_(e, (mp_size_t)n, seed * 7 + 2, 1);
    e[n - 1] |= (mp_limb_t)1 << 63;
}

}  // namespace

double bench_powmod_odd(mp_size_t n, const BenchOpts& o) {
    mp_ptr m = alloc_limbs(n), b = alloc_limbs(n), e = alloc_limbs(n), dst = alloc_limbs(n);
    fill_odd_mod(m, b, e, n, 0x51ce77aa1bull + n);
    double us = measure([&] { lmmp_powmod_odd_(dst, b, e, n, m, n); sink += dst[0]; }, o);
    lmmp_free(m);
    lmmp_free(b);
    lmmp_free(e);
    lmmp_free(dst);
    return us;
}

/*
    偶模数模幂：CRT 结构 = 奇部 REDC 梯子 + 2^k 部 powlo + 一次合成。
    m = 2^(64*(nb2-1)+1)*odd：nb2=1 即 m=2*odd（浅移位），nb2=2 即
    m 含 2^65 因子（深移位）。e ~ 64n 时偶底数结果恒 0（2^k 部短路），
    故底数强制取奇以衡量典型负载。
*/
double bench_powmod_even(mp_size_t n, uint nb2, const BenchOpts& o) {
    if (n < 3 && nb2 > 1) nb2 = 1;
    if (nb2 < 1 || nb2 >= n) nb2 = 1;  /* 奇部至少占顶 1 limb */
    mp_ptr m = alloc_limbs(n), b = alloc_limbs(n), e = alloc_limbs(n), dst = alloc_limbs(n);
    lmmp_seed_random_(m, n, 0x51ce77ac0bull + nb2 * 131 + n, 1);
    if (nb2 == 1) {
        m[0] = (m[0] << 1) | 2; /* m = 2*odd */
    } else {
        lmmp_zero(m, nb2 - 1);
        m[nb2 - 1] = 2; /* m = 2^(64*(nb2-1)+1)*odd */
    }
    m[n - 1] |= (mp_limb_t)1 << 63;
    lmmp_seed_random_(b, n, 0x51ce77ac1bull + nb2 * 61 + n, 1);
    lmmp_div_(NULL, b, b, n, m, n);
    b[0] |= 1;
    lmmp_seed_random_(e, n, 0x25c0ff350bull + nb2 * 17 + n, 1);
    e[n - 1] |= (mp_limb_t)1 << 63;
    double us = measure([&] { lmmp_powmod_(dst, b, e, n, m, n); sink += dst[0]; }, o);
    lmmp_free(m);
    lmmp_free(b);
    lmmp_free(e);
    lmmp_free(dst);
    return us;
}

double bench_powlo(mp_size_t n, const BenchOpts& o) {
    mp_ptr b = alloc_limbs(n), e = alloc_limbs(n), dst = alloc_limbs(n);
    lmmp_seed_random_(b, n, 0x51ce77ab0bull + n, 1);
    b[0] |= 1;
    lmmp_seed_random_(e, n, 0x25c0ff340bull + n, 1);
    e[n - 1] |= (mp_limb_t)1 << 63;
    double us = measure([&] { lmmp_powlo_(dst, b, n, e, n); sink += dst[0]; }, o);
    lmmp_free(b);
    lmmp_free(e);
    lmmp_free(dst);
    return us;
}

}  // namespace lmmp_bench

/* ---- 扫描用例：阈值 401 两侧细档，其余几何阶梯 ---- */

using namespace lmmp_bench;

namespace {

/*
    档位含 REDC_MERSENNE_THRESHOLD(401) 两侧的 400/402。指数与模数同规模
    意味着单 op 成本随 n^3 增长（64L 已 ~9ms、400L 已 ~1s），默认阶梯到
    阈值上下即止；更大尺寸用 bench_powmod_odd(n) 单点按需测量。
*/
const mp_size_t POWMOD_SZ[] = {1, 8, 64, 200, 400, 402};
const mp_size_t POWLO_SZ[] = {1, 2, 8, 64, 200, 400, 1000};

}  // namespace

BENCH_CASE("numth/powmod", odd) {
    for (mp_size_t n : POWMOD_SZ) report_n("powmod_odd e=n", n, bench_powmod_odd(n));
}

BENCH_CASE("numth/powmod", even) {
    for (mp_size_t n : POWMOD_SZ) report_n("powmod_even k=1 e=n", n, bench_powmod_even(n, 1));
}

BENCH_CASE("numth/powmod", even2) {
    const mp_size_t SZ[] = {8, 64, 400};
    for (mp_size_t n : SZ) report_n("powmod_even k=65 e=n", n, bench_powmod_even(n, 2));
}

BENCH_CASE("numth/powlo", powlo) {
    for (mp_size_t n : POWLO_SZ) report_n("powlo e=n", n, bench_powlo(n));
}
