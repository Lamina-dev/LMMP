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

/* 阈值调优：MUL_FFT_MODF_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

#include <stdlib.h>

typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr d;
    mp_size_t n;
} fft_ctx;

static void fft_ctx_init(fft_ctx* c, mp_size_t n) {
    c->n = n;
    c->a = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)(2 * n + 1) * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, n, UINT64_C(0x9b05688c2b3e6c1f));
    tune_fill_limbs(c->b, n, UINT64_C(0x1f83d9abfb41bd6b));
    c->a[n - 1] |= LIMB_B_2;
    c->b[n - 1] |= LIMB_B_2;
}

static double bench_mul_fft(void* v) {
    fft_ctx* c = (fft_ctx*)v;
    lmmp_mul_fft_(c->d, c->a, c->n, c->b, c->n);
    return 0.0;
}

static double* measure_candidates(const uint64_t* cand, size_t ncand,
                                  fft_ctx* ctx, size_t nsizes) {
    double* times = (double*)calloc(ncand * nsizes, sizeof(double));
    if (times == NULL)
        return NULL;
    for (size_t c = 0; c < ncand; ++c) {
        lmmp_tune_MUL_FFT_MODF_THRESHOLD = cand[c];
        printf("    T=%-6llu", (unsigned long long)cand[c]);
        for (size_t s = 0; s < nsizes; ++s) {
            const tune_measure_t m = tune_measure(bench_mul_fft, &ctx[s],
                                                  g_tune.samples,
                                                  g_tune.target_ms);
            times[c * nsizes + s] = m.median_ns;
            printf(" %8.3f ms", m.median_ns * 1e-6);
        }
        printf("\n");
        fflush(stdout);
    }
    return times;
}

static double candidate_badness(const double* times, size_t ncand, size_t nsizes,
                                size_t c) {
    double badness = 0.0;
    for (size_t s = 0; s < nsizes; ++s) {
        double fastest = times[s];
        for (size_t k = 0; k < ncand; ++k)
            if (times[k * nsizes + s] < fastest)
                fastest = times[k * nsizes + s];
        if (fastest > 0.0)
            badness += times[c * nsizes + s] / fastest - 1.0;
    }
    return badness;
}

int tune_run_mul_fft_modf(void) {
    static const uint64_t sizes[] = {512, 1024, 1536, 2048, 3072, 4096};
    enum { NS = 6, MAXC = 256 };
    const uint64_t old_value = lmmp_tune_MUL_FFT_MODF_THRESHOLD;
    uint64_t coarse[MAXC];
    uint64_t fine[MAXC];
    const uint64_t lo = 128;
    const uint64_t hi = 1024;
    const size_t ncoarse = tune_build_points(lo, hi, old_value, coarse, MAXC);
    size_t ncand = ncoarse;

    fft_ctx ctx[NS];
    for (size_t s = 0; s < NS; ++s)
        fft_ctx_init(&ctx[s], (mp_size_t)sizes[s]);

    printf("  online candidate scan over [%llu,%llu], sizes:", 
           (unsigned long long)lo, (unsigned long long)hi);
    for (size_t s = 0; s < NS; ++s)
        printf(" %llu", (unsigned long long)sizes[s]);
    printf("\n");

    double* times = measure_candidates(coarse, ncoarse, ctx, NS);
    if (times == NULL) {
        for (size_t s = 0; s < NS; ++s) {
            lmmp_free(ctx[s].a); lmmp_free(ctx[s].b); lmmp_free(ctx[s].d);
        }
        return -1;
    }

    double best = 1e300;
    uint64_t best_t = old_value;
    for (size_t c = 0; c < ncoarse; ++c) {
        const double b = candidate_badness(times, ncoarse, NS, c);
        if (b < best) { best = b; best_t = coarse[c]; }
    }

    /* 在粗网格最优点附近做一次加密，并把两次数据合并后统一选择。 */
    const uint64_t rlo = best_t > lo + best_t / 4 ? best_t - best_t / 4 : lo;
    const uint64_t rhi = best_t + best_t / 4 < hi ? best_t + best_t / 4 : hi;
    const size_t nfine = tune_build_points(rlo, rhi, best_t, fine, MAXC);
    double* times2 = measure_candidates(fine, nfine, ctx, NS);
    if (times2 != NULL) {
        double* merged = (double*)realloc(times,
                                          (ncoarse + nfine) * NS * sizeof(double));
        if (merged != NULL) {
            times = merged;
            for (size_t c = 0; c < nfine; ++c)
                for (size_t s = 0; s < NS; ++s)
                    times[(ncoarse + c) * NS + s] = times2[c * NS + s];
            for (size_t c = 0; c < nfine; ++c)
                coarse[ncoarse + c] = fine[c];
            ncand = ncoarse + nfine;
        }
        free(times2);
    }

    best = 1e300;
    best_t = old_value;
    for (size_t c = 0; c < ncand; ++c) {
        const double b = candidate_badness(times, ncand, NS, c);
        if (b < best) { best = b; best_t = coarse[c]; }
    }

    /* 平坦区间优先保留旧默认值，避免在等价区域内随机游走。 */
    const double tol = 0.01 * (best > 1.0 ? best : 1.0) + 1e-12;
    for (size_t c = 0; c < ncand; ++c) {
        if (coarse[c] == old_value &&
            candidate_badness(times, ncand, NS, c) <= best + tol) {
            best_t = old_value;
            best = candidate_badness(times, ncand, NS, c);
            break;
        }
    }

    double old_badness = 1e300;
    for (size_t c = 0; c < ncand; ++c) {
        if (coarse[c] == old_value) {
            old_badness = candidate_badness(times, ncand, NS, c);
            break;
        }
    }

    lmmp_tune_MUL_FFT_MODF_THRESHOLD = best_t;
    printf("  -> MUL_FFT_MODF_THRESHOLD old=%llu new=%llu old_badness=%.6f new_badness=%.6f\n",
           (unsigned long long)old_value, (unsigned long long)best_t,
           old_badness, best);
    tune_record_add("MUL_FFT_MODF_THRESHOLD", old_value, best_t,
                    old_badness, best);

    for (size_t s = 0; s < NS; ++s) {
        lmmp_free(ctx[s].a); lmmp_free(ctx[s].b); lmmp_free(ctx[s].d);
    }
    free(times);
    return 0;
}
