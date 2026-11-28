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
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "prime_pool.h"

static double bench_is_prime_n(mp_size_t n, int strength, PrimePool kind, const bench_options *o) {
    int cnt = pool_cnt(n);
    mp_ptr a = pool_n(n, cnt, 0x51ce77ac01ull + (mp_limb_t)strength * 131 + n, kind);
    size_t idx = 0;
    double us;
    BENCH_MEASURE(us, o, bench_sink += (mp_limb_t)lmmp_is_prime_n_(a + n * (idx++ % cnt), n, strength););
    lmmp_free(a);
    return us;
}

static double bench_is_prime_n_notrial(mp_size_t n, int strength, PrimePool kind, const bench_options *o) {
    int cnt = pool_cnt(n);
    mp_ptr a = pool_n(n, cnt, 0x51ce77ac03ull + (mp_limb_t)strength * 131 + n, kind);
    size_t idx = 0;
    double us;
    BENCH_MEASURE(us, o, bench_sink += (mp_limb_t)lmmp_is_prime_n_notrial_(a + n * (idx++ % cnt), n, strength););
    lmmp_free(a);
    return us;
}

/*
    MR 轮的指数为模数全长，单 op 成本随 n^3 增长（200L 已 ~1s），
    默认阶梯到 200L 止；更大尺寸用 bench_is_prime_n(n, ...) 单点测量。
*/
static void run_is_prime_n(void) {
    const mp_size_t SZ[] = {3, 8, 16, 32, 64, 200};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("is_prime_n s4 coprime", n, bench_is_prime_n(n, 4, PRIME_POOL_COPRIME, &bench_default), 0);
    }
}

/* 素数池最坏情形与强度档位扩展（素数池构造昂贵，仅取小尺寸） */
static void run_is_prime_n_prime(void) {
    const mp_size_t SZ[] = {8, 64};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("is_prime_n s4 prime", n, bench_is_prime_n(n, 4, PRIME_POOL_PRIME, &bench_default), 0);
    }
}

/* coprime 池已过试除，notrial 与全函数对照差即试除阶段的纯开销 */
static void run_is_prime_n_notrial(void) {
    const mp_size_t SZ[] = {3, 8, 16, 64};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("is_prime_n_notrial s4 coprime", n, bench_is_prime_n_notrial(n, 4, PRIME_POOL_COPRIME, &bench_default), 0);
    }
}

static void run_is_prime_n_tier(void) {
    const int STR[] = {0, 2, 4, 7};
    for (size_t i = 0; i < ARRAY_SIZE(STR); ++i) {
        int s = STR[i];
        char buf[64];
        snprintf(buf, sizeof(buf), "is_prime_n s%d prime(8L)", s);
        report(buf, bench_is_prime_n(8, s, PRIME_POOL_PRIME, &bench_default), 0);
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/prime/is_prime_n]");
    run_is_prime_n();
    puts("[numth/prime/is_prime_n_prime]");
    run_is_prime_n_prime();
    puts("[numth/prime/is_prime_n_notrial]");
    run_is_prime_n_notrial();
    puts("[numth/prime/is_prime_n_tier]");
    run_is_prime_n_tier();
    lmmp_global_deinit();
    return 0;
}
