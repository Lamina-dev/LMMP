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

/* [lo,hi] 对池（65..128bit），hi 顶位起始；按 kind 过滤 */
static mp_ptr pool_2(uint bits, mp_limb_t seed, PrimePool kind) {
    if (bits < 65) bits = 65;
    if (bits > 128) bits = 128;
    uint hb = bits - 64;
    mp_ptr a = alloc_limbs(2 * POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + 2 * i, 2, seed + i, 1);
        a[2 * i + 1] &= ((mp_limb_t)-1) >> (64 - hb);
        a[2 * i + 1] |= (mp_limb_t)1 << (hb - 1);
        a[2 * i] |= 1;
        if (kind == PRIME_POOL_COPRIME) {
            /* 步进只改低 limb、回绕不进位到高 limb，值恒在指定位宽内 */
            while (small_factor_q(a + 2 * i, 2)) a[2 * i] += 2;
        } else if (kind == PRIME_POOL_PRIME) {
            while (lmmp_is_prime_2_(a[2 * i], a[2 * i + 1]) == 0) a[2 * i] += 2;
        }
    }
    return a;
}

static double bench_is_prime_2(uint bits, PrimePool kind, const bench_options *o) {
    if (bits < 65) bits = 65;
    if (bits > 128) bits = 128;
    mp_ptr a = pool_2(bits, 0xdeadbeefca01ull + bits, kind);
    size_t idx = 0;
    double us;
    BENCH_MEASURE(us, o,
        bench_sink += (mp_limb_t)lmmp_is_prime_2_(a[2 * (idx & (POOL - 1))],
                                            a[2 * (idx & (POOL - 1)) + 1]);
        idx++;
    );
    lmmp_free(a);
    return us;
}

static double bench_is_prime_2_notrial(uint bits, PrimePool kind, const bench_options *o) {
    if (bits < 65) bits = 65;
    if (bits > 128) bits = 128;
    mp_ptr a = pool_2(bits, 0xdeadbeefca02ull + bits, kind);
    size_t idx = 0;
    double us;
    BENCH_MEASURE(us, o,
        bench_sink += (mp_limb_t)lmmp_is_prime_2_notrial_(a[2 * (idx & (POOL - 1))],
                                                    a[2 * (idx & (POOL - 1)) + 1]);
        idx++;
    );
    lmmp_free(a);
    return us;
}

/*
    is_prime_2 档位对应 <psi_13（13 基底确定）与 >=psi_13（+强 Lucas）。
    notrial 与带试除版本的对照差即试除阶段成本（odd 池有试除早拒收益，
    coprime/prime 池已过滤小因子，差值接近纯试除开销）。
*/

static void run_is_prime_2(void) {
    const uint BITS[] = {65, 82, 96, 128};
    for (size_t i = 0; i < ARRAY_SIZE(BITS); ++i) {
        uint bits = BITS[i];
        report_bits("is_prime_2 odd", bits, bench_is_prime_2(bits, PRIME_POOL_ODD, &bench_default));
    }
    report_bits("is_prime_2 prime", 128, bench_is_prime_2(128, PRIME_POOL_PRIME, &bench_default));
}

static void run_is_prime_2_notrial(void) {
    const uint BITS[] = {65, 128};
    for (size_t i = 0; i < ARRAY_SIZE(BITS); ++i) {
        uint bits = BITS[i];
        report_bits("is_prime_2_notrial odd", bits, bench_is_prime_2_notrial(bits, PRIME_POOL_ODD, &bench_default));
    }
    report_bits("is_prime_2_notrial coprime", 128, bench_is_prime_2_notrial(128, PRIME_POOL_COPRIME, &bench_default));
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/prime/is_prime_2]");
    run_is_prime_2();
    puts("[numth/prime/is_prime_2_notrial]");
    run_is_prime_2_notrial();
    lmmp_global_deinit();
    return 0;
}
