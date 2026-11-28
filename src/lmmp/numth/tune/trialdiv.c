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

#include <math.h>
#include <stdlib.h>

#include "impl/longlong.h"
#include "impl/trialdiv.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"

enum { PREINV, CHUNK4, CHUNK8, PATH_COUNT, MAX_N = 512 };

static const char* const program = "tune_numth_trialdiv";
static const char* const preinv_name = "TRIALDIV_PREINV_THRESHOLD";
static const char* const k8_name = "TRIALDIV_K8_THRESHOLD";
/* 首个完整试除块的素数乘积。 */
static const mp_limb_t divisor = UINT64_C(0x08a5b6470af95);

typedef struct {
    mp_ptr np;
    mp_size_t nn;
    mp_limb_t inv, k, dinv, r2, base, mi;
} trialdiv_context;

typedef struct {
    double excess[PATH_COUNT];
} trialdiv_score;

static void no_setup(void* opaque) {
    (void)opaque;
}

static mp_limb_t call_preinv(void* opaque) {
    const trialdiv_context* ctx = opaque;
    return lmmp_mod_preinv_1_(ctx->np, ctx->nn, divisor, ctx->inv, ctx->k);
}

static mp_limb_t call_chunk4(void* opaque) {
    const trialdiv_context* ctx = opaque;
    return lmmp_mod_4chunk_1_(ctx->np, ctx->nn, divisor, ctx->dinv, ctx->r2, ctx->base, ctx->mi);
}

static mp_limb_t call_chunk8(void* opaque) {
    const trialdiv_context* ctx = opaque;
    return lmmp_mod_8chunk_1_(ctx->np, ctx->nn, divisor, ctx->dinv, ctx->r2, ctx->base, ctx->mi);
}

/* prefix[n] 包含 [1,n] 中各路的相对超出之和。两个阈值所在的尺寸均计入后一档。 */
static double joint_cost(const trialdiv_score* prefix, int preinv, int k8) {
    return prefix[preinv - 1].excess[PREINV] +
           (prefix[k8 - 1].excess[CHUNK4] - prefix[preinv - 1].excess[CHUNK4]) +
           (prefix[MAX_N].excess[CHUNK8] - prefix[k8 - 1].excess[CHUNK8]);
}

static void recommend(const trialdiv_score* prefix, unsigned scored, double current_cost) {
    int best_preinv = 2, best_k8 = 3;
    double best = INFINITY;
    for (int preinv = 2; preinv < MAX_N; ++preinv) {
        for (int k8 = preinv + 1; k8 <= MAX_N; ++k8) {
            double cost = joint_cost(prefix, preinv, k8) / scored;
            if (cost < best) {
                best = cost;
                best_preinv = preinv;
                best_k8 = k8;
            }
        }
    }

    int preinv_min = MAX_N, preinv_max = 0, k8_min = MAX_N, k8_max = 0;
    for (int preinv = 2; preinv < MAX_N; ++preinv) {
        for (int k8 = preinv + 1; k8 <= MAX_N; ++k8) {
            if (joint_cost(prefix, preinv, k8) / scored > best + 0.001) continue;
            if (preinv < preinv_min) preinv_min = preinv;
            if (preinv > preinv_max) preinv_max = preinv;
            if (k8 < k8_min) k8_min = k8;
            if (k8 > k8_max) k8_max = k8;
        }
    }

    printf("# mean_excess=%.4f%% current_mean_excess=%.4f%%\n", 100 * best, 100 * current_cost / scored);
    printf("# within_0.1pp_axis_ranges: %s=%d..%d %s=%d..%d\n",
           preinv_name, preinv_min, preinv_max, k8_name, k8_min, k8_max);
    printf("#define %s %d\n#define %s %d\n", preinv_name, best_preinv, k8_name, best_k8);
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    options.min_ms = 1.0;
    if (!lmmp_tune_parse(argc, argv, &options)) return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage(program);
        puts("  Trialdiv uses --min-ms 1 by default.");
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_INFO) {
        printf("program=%s\nparameters=%s,%s\n", program, preinv_name, k8_name);
        printf("paths=lmmp_mod_preinv_1_,lmmp_mod_4chunk_1_,lmmp_mod_8chunk_1_ (direct calls)\n");
        printf("nn=1..%d step=1 divisor=0x%llx\ncandidates=2 <= %s < %s <= %d\n",
               MAX_N, (unsigned long long)divisor, preinv_name, k8_name, MAX_N);
        printf("compiler=%s\n", __VERSION__);
        lmmp_tune_print_env();
    }
    if (options.action == LMMP_TUNE_CURRENT || options.action == LMMP_TUNE_INFO) {
        lmmp_tune_print_current(preinv_name, TRIALDIV_PREINV_THRESHOLD, TRIALDIV_PREINV_THRESHOLD);
        lmmp_tune_print_current(k8_name, TRIALDIV_K8_THRESHOLD, TRIALDIV_K8_THRESHOLD);
        return EXIT_SUCCESS;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    mp_limb_t np[MAX_N];
    trialdiv_context ctx = {.np = np, .k = lmmp_leading_zeros_(divisor)};
    mp_limb_t normalized = divisor << ctx.k;
    ctx.inv = lmmp_inv_1_(normalized);
    ctx.dinv = lmmp_binvert_ulong_(divisor);
    ctx.mi = -ctx.dinv;
    ctx.r2 = _bpow_preinv_(normalized, ctx.k, 2, ctx.inv);
    ctx.base = _bpow_preinv_(normalized, ctx.k, 1, ctx.inv);
    const lmmp_tune_bench bench[PATH_COUNT] = {
        [PREINV] = {.bind = no_setup, .reset = no_setup, .call = call_preinv, .context = &ctx},
        [CHUNK4] = {.bind = no_setup, .reset = no_setup, .call = call_chunk4, .context = &ctx},
        [CHUNK8] = {.bind = no_setup, .reset = no_setup, .call = call_chunk8, .context = &ctx},
    };

    trialdiv_score prefix[MAX_N + 1] = {0};
    double current_cost = 0;
    unsigned scored = 0;
    printf("# program=%s divisor=0x%llx samples=%u min_ms=%.3f seed=0x%llx\n",
           program, (unsigned long long)divisor, options.samples, options.min_ms, options.seed);
    lmmp_tune_print_env_comment();
    puts("nn,preinv_us,chunk4_vs_preinv_us,chunk4_vs_chunk8_us,chunk8_us,stable");
    for (int n = 1; n <= MAX_N; ++n) {
        ctx.nn = n;
        fill_random(np, n, options.seed ^ (mp_limb_t)n);
        mp_limb_t expected = lmmp_mod_1_(np, n, divisor);
        if (call_preinv(&ctx) != expected || call_chunk4(&ctx) != expected || call_chunk8(&ctx) != expected) {
            fprintf(stderr, "trialdiv remainder mismatch at nn=%d\n", n);
            lmmp_global_deinit();
            return EXIT_FAILURE;
        }

        double preinv, chunk4_low, chunk4_high, chunk8, low_mad, high_mad;
        unsigned retries;
        bool low_stable, high_stable;
        if (!lmmp_tune_measure_pair(&bench[PREINV], &bench[CHUNK4], &options,
                                    &preinv, &low_mad, &chunk4_low, &high_mad, &retries, &low_stable) ||
            !lmmp_tune_measure_pair(&bench[CHUNK4], &bench[CHUNK8], &options,
                                    &chunk4_high, &low_mad, &chunk8, &high_mad, &retries, &high_stable)) {
            fprintf(stderr, "trialdiv measurement failed at nn=%d\n", n);
            lmmp_global_deinit();
            return EXIT_FAILURE;
        }
        bool stable = low_stable && high_stable;
        printf("%d,%.8f,%.8f,%.8f,%.8f,%d\n", n, preinv, chunk4_low, chunk4_high, chunk8, stable);
        prefix[n] = prefix[n - 1];
        if (!stable) continue;

        /* 两次配对分别以各自的 chunk-4 耗时为单位。每个 nn 的基准是三路中最快的一路。 */
        double cost[PATH_COUNT] = {preinv / chunk4_low, 1.0, chunk8 / chunk4_high};
        double fastest = fmin(cost[PREINV], fmin(cost[CHUNK4], cost[CHUNK8]));
        for (int path = 0; path < PATH_COUNT; ++path) {
            cost[path] = cost[path] / fastest - 1.0;
            prefix[n].excess[path] += cost[path];
        }
        int current_path = n < TRIALDIV_PREINV_THRESHOLD ? PREINV : n < TRIALDIV_K8_THRESHOLD ? CHUNK4 : CHUNK8;
        current_cost += cost[current_path];
        ++scored;
    }

    lmmp_global_deinit();
    printf("# stable_points=%u/%d\n", scored, MAX_N);
    if (scored == 0) return EXIT_FAILURE;
    recommend(prefix, scored, current_cost);
    return EXIT_SUCCESS;
}
