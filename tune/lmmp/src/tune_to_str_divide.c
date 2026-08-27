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

/* 阈值调优：TO_STR_DIVIDE_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

/* 该阈值位于 to_str_divide_ 的递归内部，不能化简为单点 A/B 两条曲线；
 * 这里固定 BASEPOW，在阈值整数域上对同一组输入直接测量每个候选阈值。 */

#define TUNE_TO_STR_DIVIDE_BASEPOW 128
#define TUNE_TO_STR_MAX_SAMPLES 8

typedef struct {
    mp_ptr num;
    mp_byte_t* buf;
    mp_size_t n;
} tostr_ctx;

static void tostr_ctx_init(tostr_ctx* c, mp_size_t n) {
    c->n = n;
    c->num = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    tune_fill_limbs(c->num, n, UINT64_C(0xdeadbeefcafebabe) + (uint64_t)n);
    c->num[n - 1] |= LIMB_B_2;
    c->buf = (mp_byte_t*)lmmp_alloc((size_t)lmmp_to_str_len_(c->num, n, 10) + 2);
}

static double bench_tostr(void* v) {
    tostr_ctx* c = (tostr_ctx*)v;
    (void)lmmp_to_str_(c->buf, c->num, c->n, 10);
    return 0.0;
}

int tune_run_to_str_divide(void) {
    static const mp_size_t sizes[TUNE_TO_STR_MAX_SAMPLES] = {128, 160, 192, 256, 384, 512};
    enum { N = 6, TLO = 3, THI = 128 };
    tostr_ctx ctx[N];
    double times[THI - TLO + 1][N];
    double per_candidate[TLO + THI - TLO + 1];
    const uint64_t old_value = lmmp_tune_TO_STR_DIVIDE_THRESHOLD;
    const uint64_t old_basepow = lmmp_tune_TO_STR_BASEPOW_THRESHOLD;

    for (int i = 0; i < N; ++i)
        tostr_ctx_init(&ctx[i], sizes[i]);
    lmmp_tune_TO_STR_BASEPOW_THRESHOLD = TUNE_TO_STR_DIVIDE_BASEPOW;

    printf("  online search range [%d,%d] over %d sizes...\n", TLO, THI, N);
    for (uint64_t t = TLO; t <= THI; ++t) {
        lmmp_tune_TO_STR_DIVIDE_THRESHOLD = t;
        for (int s = 0; s < N; ++s) {
            const tune_measure_t m = tune_measure(bench_tostr, &ctx[s],
                                                  g_tune.samples, g_tune.target_ms);
            times[t - TLO][s] = m.median_ns;
        }
        if ((t - TLO) % 16 == 0 || t == TLO || t == THI)
            printf("    t=%-4llu %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f ms\n",
                   (unsigned long long)t,
                   times[t - TLO][0] * 1e-6, times[t - TLO][1] * 1e-6,
                   times[t - TLO][2] * 1e-6, times[t - TLO][3] * 1e-6,
                   times[t - TLO][4] * 1e-6, times[t - TLO][5] * 1e-6);
    }

    /* 每个样本以最快候选为基准计算相对损失，再跨样本求和。 */
    double best_badness = 1e300;
    uint64_t best_t = old_value;
    for (uint64_t t = TLO; t <= THI; ++t) {
        double badness = 0.0;
        for (int s = 0; s < N; ++s) {
            double fastest = times[0][s];
            for (uint64_t u = TLO; u <= THI; ++u)
                if (times[u - TLO][s] < fastest)
                    fastest = times[u - TLO][s];
            if (fastest > 0.0)
                badness += times[t - TLO][s] / fastest - 1.0;
        }
        per_candidate[t] = badness;
        if (badness < best_badness) {
            best_badness = badness;
            best_t = t;
        }
    }

    lmmp_tune_TO_STR_DIVIDE_THRESHOLD = best_t;
    lmmp_tune_TO_STR_BASEPOW_THRESHOLD = old_basepow;
    for (int i = 0; i < N; ++i) {
        lmmp_free(ctx[i].num);
        lmmp_free(ctx[i].buf);
    }

    double old_badness = (old_value >= TLO && old_value <= THI)
                            ? per_candidate[old_value] : 1e300;
    printf("  -> TO_STR_DIVIDE_THRESHOLD old=%llu new=%llu old_badness=%.6f new_badness=%.6f\n",
           (unsigned long long)old_value, (unsigned long long)best_t, old_badness, best_badness);
    tune_record_add("TO_STR_DIVIDE_THRESHOLD", old_value, best_t,
                    old_badness, best_badness);
    return 0;
}
