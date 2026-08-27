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

/* 阈值调优：FROM_STR_DIVIDE_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

/* 与 to_str_divide 相同，该阈值位于递归内部；固定 BASEPOW 后直接在线扫描。 */

#define TUNE_FROM_STR_DIVIDE_BASEPOW 100
#define TUNE_FROM_STR_MAX_SAMPLES 8

typedef struct {
    mp_ptr dst;
    mp_byte_t* buf;
    mp_size_t len;
} fromstr_ctx;

static mp_size_t digits_for_limbs(uint64_t limbs) {
    return (mp_size_t)((limbs * 1927 + 50) / 100);
}

static void fromstr_ctx_init(fromstr_ctx* c, uint64_t limbs) {
    uint64_t state = UINT64_C(0x0123456789abcdef) + limbs;
    c->len = digits_for_limbs(limbs);
    c->buf = (mp_byte_t*)lmmp_alloc((size_t)c->len + 1);
    for (mp_size_t i = 0; i < c->len; ++i) {
        state += UINT64_C(0x9e3779b97f4a7c15);
        c->buf[i] = (mp_byte_t)('1' + (state % 9));
    }
    c->buf[c->len] = 0;
    c->dst = (mp_ptr)lmmp_alloc((size_t)(lmmp_from_str_len_(c->buf, c->len, 10) + 2)
                                * sizeof(mp_limb_t));
}

static double bench_fromstr(void* v) {
    fromstr_ctx* c = (fromstr_ctx*)v;
    (void)lmmp_from_str_(c->dst, c->buf, c->len, 10);
    return 0.0;
}

int tune_run_from_str_divide(void) {
    static const uint64_t limb_sizes[TUNE_FROM_STR_MAX_SAMPLES] = {128, 192, 256, 384, 512};
    enum { N = 5, TLO = 8, THI = 256 };
    fromstr_ctx ctx[N];
    double times[THI - TLO + 1][N];
    double per_candidate[TLO + THI - TLO + 1];
    const uint64_t old_value = lmmp_tune_FROM_STR_DIVIDE_THRESHOLD;
    const uint64_t old_basepow = lmmp_tune_FROM_STR_BASEPOW_THRESHOLD;

    for (int i = 0; i < N; ++i)
        fromstr_ctx_init(&ctx[i], limb_sizes[i]);
    lmmp_tune_FROM_STR_BASEPOW_THRESHOLD = TUNE_FROM_STR_DIVIDE_BASEPOW;

    printf("  online search range [%d,%d] over %d sizes...\n", TLO, THI, N);
    for (uint64_t t = TLO; t <= THI; ++t) {
        lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = t;
        for (int s = 0; s < N; ++s) {
            const tune_measure_t m = tune_measure(bench_fromstr, &ctx[s],
                                                  g_tune.samples, g_tune.target_ms);
            times[t - TLO][s] = m.median_ns;
        }
        if ((t - TLO) % 16 == 0 || t == TLO || t == THI)
            printf("    t=%-4llu %8.3f %8.3f %8.3f %8.3f %8.3f ms\n",
                   (unsigned long long)t,
                   times[t - TLO][0] * 1e-6, times[t - TLO][1] * 1e-6,
                   times[t - TLO][2] * 1e-6, times[t - TLO][3] * 1e-6,
                   times[t - TLO][4] * 1e-6);
    }

    double best_badness = 1e300;
    uint64_t best_t = old_value;
    for (uint64_t t = TLO; t <= THI; ++t) {
        double badness = 0.0;
        for (int s = 0; s < N; ++s) {
            double fastest = times[0][s];
            for (uint64_t u = TLO; u <= THI; ++u)
                if (times[u - TLO][s] < fastest)
                    fastest = times[u - TLO][s];
            badness += times[t - TLO][s] / fastest - 1.0;
        }
        per_candidate[t] = badness;
        if (badness < best_badness) {
            best_badness = badness;
            best_t = t;
        }
    }

    lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = best_t;
    lmmp_tune_FROM_STR_BASEPOW_THRESHOLD = old_basepow;
    for (int i = 0; i < N; ++i) {
        lmmp_free(ctx[i].dst);
        lmmp_free(ctx[i].buf);
    }

    double old_badness = (old_value >= TLO && old_value <= THI)
                            ? per_candidate[old_value] : 1e300;
    printf("  -> FROM_STR_DIVIDE_THRESHOLD old=%llu new=%llu old_badness=%.6f new_badness=%.6f\n",
           (unsigned long long)old_value, (unsigned long long)best_t, old_badness, best_badness);
    tune_record_add("FROM_STR_DIVIDE_THRESHOLD", old_value, best_t,
                    old_badness, best_badness);
    return 0;
}
