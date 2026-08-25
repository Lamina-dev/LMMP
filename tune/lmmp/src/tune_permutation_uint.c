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

/* 阈值调优：PERMUTATION_UINT_K/B_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

typedef struct {
    mp_ptr dst;
    mp_size_t rn;
    ulong n;
    ulong r;
} npr_ctx;

static void npr_ctx_init(npr_ctx* c, ulong n, ulong r) {
    mp_bitcnt_t bits = 0;
    c->n = n;
    c->r = r;
    c->rn = lmmp_nPr_size_(n, r, &bits);
    c->dst = (mp_ptr)lmmp_alloc((size_t)(c->rn + 2) * sizeof(mp_limb_t));
}

static double bench_npr(void* v) {
    npr_ctx* c = (npr_ctx*)v;
    (void)lmmp_odd_nPr_uint_(c->dst, c->rn, c->n, c->r);
    return 0.0;
}

static void* make_ctx(uint64_t n, uint64_t r) {
    npr_ctx* c = (npr_ctx*)lmmp_alloc(sizeof(npr_ctx));
    if (c != NULL)
        npr_ctx_init(c, (ulong)n, (ulong)r);
    return c;
}

static void free_ctx(void* v) {
    npr_ctx* c = (npr_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->dst);
        lmmp_free(c);
    }
}

int tune_run_permutation_uint(void) {
    static const uint64_t ns[] = {65536, 100000, 200000, 500000, 1000000};
    static const uint64_t rf[] = {1, 2, 5, 10, 20, 35, 50, 70};
    enum { MAXP = 64 };
    tune_line_point_t points[MAXP];
    size_t npoints = 0;

    const uint64_t old_k = lmmp_tune_PERMUTATION_UINT_K_THRESHOLD;
    const uint64_t old_b = lmmp_tune_PERMUTATION_UINT_B_THRESHOLD;

    printf("  measuring product/factor pairs (n, r fractions in %%)...\n");
    for (size_t i = 0; i < sizeof(ns) / sizeof(ns[0]); ++i) {
        for (size_t j = 0; j < sizeof(rf) / sizeof(rf[0]); ++j) {
            const uint64_t n = ns[i];
            const uint64_t r = n * rf[j] / 100;
            if (r < 12 || r > n)
                continue;
            tune_measure_t mp, mf;
            void* cp;
            void* cf;

            lmmp_tune_PERMUTATION_UINT_K_THRESHOLD = 0;
            lmmp_tune_PERMUTATION_UINT_B_THRESHOLD = 0;
            cp = make_ctx(n, r);
            lmmp_tune_PERMUTATION_UINT_K_THRESHOLD = UINT64_C(1000000000000);
            lmmp_tune_PERMUTATION_UINT_B_THRESHOLD = 0;
            cf = make_ctx(n, r);

            tune_measure_pair(bench_npr, cp, bench_npr, cf,
                              g_tune.samples, g_tune.target_ms, &mp, &mf);
            printf("    n=%-8llu r=%-8llu product=%10.3f ns factor=%10.3f ns\n",
                   (unsigned long long)n, (unsigned long long)r,
                   mp.median_ns, mf.median_ns);
            points[npoints].n = n;
            points[npoints].r = r;
            points[npoints].product_ns = mp.median_ns;
            points[npoints].factor_ns = mf.median_ns;
            ++npoints;

            free_ctx(cp);
            free_ctx(cf);
        }
    }

    const tune_line_choice_t choice = tune_choose_2d_line(
        points, npoints, 1, 1024, 0, UINT64_C(1) << 28,
        old_k, old_b, 8192);
    printf("  -> K=%llu B=%llu badness=%.6f\\n",
           (unsigned long long)choice.k, (unsigned long long)choice.b,
           choice.badness);

    lmmp_tune_PERMUTATION_UINT_K_THRESHOLD = choice.k;
    lmmp_tune_PERMUTATION_UINT_B_THRESHOLD = choice.b;
    tune_record_add("PERMUTATION_UINT_K_THRESHOLD", old_k, choice.k,
                    choice.badness, choice.badness);
    tune_record_add("PERMUTATION_UINT_B_THRESHOLD", old_b, choice.b,
                    choice.badness, choice.badness);
    return 0;
}
