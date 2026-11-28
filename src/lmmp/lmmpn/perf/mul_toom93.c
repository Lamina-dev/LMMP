/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
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

/* Toom-9.3 不平衡乘法基准（平衡核入口已删除）：固定 nb 扫描 3:1 以上比例，
   对比 lmmp_mul_toom93_unbalance_ 与 FFT。FFT 路径严格复现 mul_ 规则，
   na<6nb 用 lmmp_mul_fft_，否则 lmmp_mul_fft_unbalance_。 */

#include "support/perf.h"
#include "lmmp/lmmpn.h"

typedef void (*mul_fn)(mp_ptr, mp_srcptr, mp_size_t, mp_srcptr, mp_size_t);

static double bench_fn(mul_fn fn, mp_size_t na, mp_size_t nb, const bench_options* o) {
    mp_ptr pa = alloc_limbs(na);
    mp_ptr pb = alloc_limbs(nb);
    mp_ptr d = alloc_limbs(na + nb + 2);
    fill_random(pa, na, 0x93b0a5e00001ull + na * 31 + nb);
    fill_random(pb, nb, 0x93b0a5e00002ull + nb * 17 + na);
    double us;
    BENCH_MEASURE(us, o, fn(d, pa, na, pb, nb););
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

static void run_grid(void) {
    const mp_size_t nbs[] = {500, 750, 1000, 1250, 1500};
    const double ratios[] = {3.0, 4.0, 5.0, 6.0, 8.0, 10.0, 12.0, 16.0};
    bench_options o = bench_default;
    o.agg = BENCH_MEDIAN;
    o.max_samples = 7;
    printf("  %-6s %-6s %-12s %12s %12s %12s %10s %10s %10s\n", "nb", "r", "fft_path", "toom93/us",
           "fft/us", "fft_unbal/us", "fft/toom", "fftU/toom", "winner");
    for (size_t bi = 0; bi < ARRAY_SIZE(nbs); ++bi) {
        mp_size_t nb = nbs[bi];
        for (size_t ri = 0; ri < ARRAY_SIZE(ratios); ++ri) {
            mp_size_t na = (mp_size_t)(nb * ratios[ri]);
            double t93 = bench_fn(lmmp_mul_toom93_unbalance_, na, nb, &o);
            bool use_fft_unbalance = na >= 6 * nb;
            double tfft = bench_fn(lmmp_mul_fft_, na, nb, &o);
            double tfft_unbalance = use_fft_unbalance ? bench_fn(lmmp_mul_fft_unbalance_, na, nb, &o) : 0.0;
            double selected = use_fft_unbalance ? tfft_unbalance : tfft;
            printf("  %-6llu %-6.1f %-12s %12.3f %12.3f %12.3f %10.3f %10.3f %10s\n",
                   (unsigned long long)nb, ratios[ri], use_fft_unbalance ? "fft_unbalance" : "fft",
                   t93, tfft, tfft_unbalance, tfft / t93,
                   tfft_unbalance > 0.0 ? tfft_unbalance / t93 : 0.0,
                   selected < t93 ? "fft" : "toom93");
        }
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 4096);
#if defined(LMMP_WINDOWS)
    /* 绑核到 P 核（LP0-15），避免大小核幻影比值 */
    SetProcessAffinityMask(GetCurrentProcess(), (DWORD_PTR)0xFFF);
#endif
    lmmp_global_init();
    puts("[lmmpn/mul_toom93/unbalance]");
    run_grid();
    lmmp_global_deinit();
    return 0;
}
