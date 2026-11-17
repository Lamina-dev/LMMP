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

/* 顶位起始的随机奇数池（<=64bit），元素个数 POOL；kind 过滤同 pool_n：
   PRIME 素数（next_prime 构造，全基底执行）/ COPRIME 试除幸存者 */
static mp_ptr pool_bits(uint bits, mp_limb_t seed, PrimePool kind) {
    if (bits < 2) bits = 2;  /* 1-bit 奇数仅有值 1：退化且与标注位宽不符 */
    if (bits > 64) bits = 64;
    mp_ptr a = alloc_limbs(POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + i, 1, seed + i, 1);
        if (bits < 64) a[i] &= ((mp_limb_t)-1) >> (64 - bits);
        a[i] |= (mp_limb_t)1 << (bits - 1);
        a[i] |= 1;
        if (kind == PRIME_POOL_COPRIME) {
            /* 步进不得越出指定位宽：将越界时重采样候选而非沿用更宽的值
               （bits>=10 域内幸存者密度 ~1/7，重采样几乎不触发；bits<=9
               域内无 >1000 素因子的幸存者，由入口拒绝） */
            uint tries = 0;
            while (small_factor_q(a + i, 1)) {
                if (bits < 64 && ((a[i] + 2) >> bits) != 0) {
                    lmmp_seed_random_(a + i, 1, seed + i + 0x9e37ull * ++tries, 1);
                    a[i] &= ((mp_limb_t)-1) >> (64 - bits);
                    a[i] |= (mp_limb_t)1 << (bits - 1);
                    a[i] |= 1;
                } else {
                    a[i] += 2;
                }
            }
        } else if (kind == PRIME_POOL_PRIME) {
            a[i] = lmmp_next_prime_ulong_(a[i]);
        }
    }
    return a;
}

static double bench_is_prime_ulong_notrial(uint bits, PrimePool kind, const bench_options *o) {
    /* COPRIME 域内须存在试除幸存者（最小 1009 需 10 位）：更窄位宽
       拒绝测量，不得以更宽的值冒充请求位宽 */
    if (kind == PRIME_POOL_COPRIME && bits < 10) return 0.0;
    mp_ptr a = pool_bits(bits, 0x51ce25c0ff33ull + bits, kind);
    size_t idx = 0;
    double us;
    BENCH_MEASURE(us, o, bench_sink += (mp_limb_t)lmmp_is_prime_ulong_notrial_(a[idx++ & (POOL - 1)]););
    lmmp_free(a);
    return us;
}

static double bench_is_prime_ulong(uint bits, PrimePool kind, const bench_options *o) {
    if (kind == PRIME_POOL_COPRIME && bits < 10) return 0.0;  /* 同 notrial：域宽下界 */
    mp_ptr a = pool_bits(bits, 0x13579bdf0246ull + bits, kind);
    size_t idx = 0;
    double us;
    BENCH_MEASURE(us, o, bench_sink += (mp_limb_t)lmmp_is_prime_ulong_(a[idx++ & (POOL - 1)]););
    lmmp_free(a);
    return us;
}

/*
    64bit 内档位对应 notrial 家族内部分支：40bit（双基底分支）、
    56bit（三基底 mont63 分支）、64bit 顶（三基底 mont64 分支）；
*/

static void run_notrial(void) {
    const uint BITS[] = {40, 56, 64};
    for (size_t i = 0; i < ARRAY_SIZE(BITS); ++i) {
        uint bits = BITS[i];
        report_bits("notrial odd", bits, bench_is_prime_ulong_notrial(bits, PRIME_POOL_ODD, &bench_default));
        report_bits("notrial prime", bits, bench_is_prime_ulong_notrial(bits, PRIME_POOL_PRIME, &bench_default));
    }
}

static void run_is_prime_ulong(void) {
    const uint BITS[] = {40, 56, 64};
    for (size_t i = 0; i < ARRAY_SIZE(BITS); ++i) {
        uint bits = BITS[i];
        report_bits("is_prime_ulong odd", bits, bench_is_prime_ulong(bits, PRIME_POOL_ODD, &bench_default));
        report_bits("is_prime_ulong prime", bits, bench_is_prime_ulong(bits, PRIME_POOL_PRIME, &bench_default));
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/prime/notrial]");
    run_notrial();
    puts("[numth/prime/is_prime_ulong]");
    run_is_prime_ulong();
    lmmp_global_deinit();
    return 0;
}
