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

/* ---- Proth 形状：N = k*2^t + 1（k<=2^32） ---- */

static mp_size_t proth_limbs(mp_ptr dst, uint k, uint t) {
    mp_size_t n = (mp_size_t)t / 64 + 2;
    lmmp_zero(dst, n + 1);
    uint sh = t % 64;
    dst[t / 64] |= (mp_limb_t)k << sh;
    if (sh != 0) dst[t / 64 + 1] |= (mp_limb_t)k >> (64 - sh);
    dst[0] |= 1;
    while (n > 1 && dst[n - 1] == 0) n--;
    return n;
}

typedef struct {
    mp_ptr a;         /* cnt * stride，元素为 k*2^t+1 */
    uint ks[8];       /* 各元素的 k */
    mp_size_t nn[8];  /* 各元素规范化 limb 数 */
    mp_size_t stride;
    int cnt;
} ProthPool;

/* want_prime：Proth 素数池（被测函数搜索）；否则试除幸存合数池 */
static ProthPool pool_proth(uint t, int cnt, bool want_prime) {
    ProthPool p;
    p.stride = (mp_size_t)t / 64 + 3;
    p.cnt = cnt;
    p.a = alloc_limbs((size_t)p.stride * cnt);
    uint k = 3;
    for (int i = 0; i < cnt; i++) {
        for (;;) {
            mp_size_t nn = proth_limbs(p.a + p.stride * i, k, t);
            bool ok = want_prime ? lmmp_is_prothprime_(k, (mp_size_t)t)
                                 : !small_factor_q(p.a + p.stride * i, nn);
            if (ok) break;
            k += 2;
        }
        p.ks[i] = k;
        p.nn[i] = proth_limbs(p.a + p.stride * i, k, t);
        k += 2;
    }
    return p;
}

static double bench_is_prothprime(uint t, bool prime_pool, const bench_options *o) {
    if (t < 1) t = 1;  /* is_prothprime_ 契约 n>0 */
    ProthPool p = pool_proth(t, 8, prime_pool);
    size_t idx = 0;
    double us;
    BENCH_MEASURE(us, o, bench_sink += (mp_limb_t)lmmp_is_prothprime_(p.ks[idx++ % p.cnt], t););
    lmmp_free(p.a);
    return us;
}

static double bench_is_prime_n_proth(uint t, int strength, bool prime_pool, const bench_options *o) {
    if (t < 1) t = 1;  /* 同 bench_is_prothprime：契约 n>0 */
    ProthPool p = pool_proth(t, 8, prime_pool);
    size_t idx = 0;
    double us;
    BENCH_MEASURE(us, o,
        bench_sink += (mp_limb_t)lmmp_is_prime_n_(p.a + p.stride * (idx % p.cnt),
                                            p.nn[idx % p.cnt], strength);
        idx++;
    );
    lmmp_free(p.a);
    return us;
}

/*
    Proth 形状：lmmp_is_prothprime_ 单梯子定理判决 vs lmmp_is_prime_n_
    通用分档（同池对照）。素数池 = 分档全轮完整执行的最坏情形（Proth
    路径的收益所在）；合数池 = 试除幸存者，两路径都支付完整梯子，
    用于确认 Proth 路径在合数侧无回归。素数池构造昂贵，仅取 <=16 limb。
*/
static void run_proth(void) {
    const uint TS[] = {480, 992};
    for (size_t i = 0; i < ARRAY_SIZE(TS); ++i) {
        uint t = TS[i];
        char buf[64];
        snprintf(buf, sizeof(buf), "is_prothprime prime(t=%u)", t);
        report(buf, bench_is_prothprime(t, true, &bench_default), 0);
        snprintf(buf, sizeof(buf), "is_prime_n s4 proth-prime(t=%u)", t);
        report(buf, bench_is_prime_n_proth(t, 4, true, &bench_default), 0);
    }
    const uint TC[] = {2016, 4032};
    for (size_t i = 0; i < ARRAY_SIZE(TC); ++i) {
        uint t = TC[i];
        char buf[64];
        snprintf(buf, sizeof(buf), "is_prothprime composite(t=%u)", t);
        report(buf, bench_is_prothprime(t, false, &bench_default), 0);
        snprintf(buf, sizeof(buf), "is_prime_n s4 proth-composite(t=%u)", t);
        report(buf, bench_is_prime_n_proth(t, 4, false, &bench_default), 0);
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/prime/proth]");
    run_proth();
    lmmp_global_deinit();
    return 0;
}
