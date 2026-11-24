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

#include <stdio.h>
#include <stdlib.h>

#include "lmmp/lmmpn.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

/* 梅森平方分发门（lmmp_sqr_mersenne_）：rn >= T 且 rn 为 fft 可行尺寸时走 SSA
   梅森平方 FFT，否则回落 sqr dc。门只在根层出现一次，dc/fft 内部递归均不读
   本阈值 ⟹ 全网格干净，sub_n=0。网格与候选均取 fft 可行尺寸格点（不可行 rn
   恒回落 dc，门无部署差异）。形状：主评分=满长 an=rn；副形状 an=3rn/4+1
   只输出测量行不参与评分。 */
enum { MIN_N = 128, MAX_N = 2048 };

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_size_t rn;
    mp_size_t na;
} measure_context;

static void bind_default(void* opaque) {
    (void)opaque; /* 分发门不参与两路内部决策，无需绑定 */
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->rn);
}

static mp_limb_t fingerprint(mp_srcptr p, mp_size_t n) {
    return p[0] + p[n - 1];
}

static mp_limb_t call_dc(void* opaque) {
    measure_context* context = opaque;
    lmmp_sqr_mersenne_dc_(context->dst, context->rn, context->numa, context->na);
    return fingerprint(context->dst, context->rn);
}

static mp_limb_t call_fft(void* opaque) {
    measure_context* context = opaque;
    lmmp_sqr_mersenne_fft_(context->dst, context->rn, context->numa, context->na);
    return fingerprint(context->dst, context->rn);
}

/* oracle：全积平方 + 循环折叠 + 零类归一化（B^rn-1 -> 0），与 test_mul_mersenne 同手段 */
static void mersenne_oracle(mp_ptr r, mp_size_t rn, mp_srcptr a, mp_size_t na, mp_ptr full) {
    lmmp_zero(full, 2 * rn + 2);
    lmmp_sqr_(full, a, na);
    mp_size_t pn = 2 * na;
    if (pn <= rn) {
        lmmp_copy(r, full, pn);
        lmmp_zero(r + pn, rn - pn);
    } else {
        mp_limb_t cy = lmmp_add_(r, full, rn, full + rn, pn - rn);
        lmmp_inc_1(r, cy);
    }
    mp_size_t i = 0;
    while (i < rn && r[i] == ~(mp_limb_t)0) ++i;
    if (i == rn) lmmp_zero(r, rn);
}

/* 候选=可行格点序列，稀疏候选按均值汇总 */
static void recommend_grid(const char* name, const double* scores, const unsigned long* counts, const mp_size_t* grid,
                           int grid_count, mp_size_t current) {
    int best = 0;
    for (int i = 1; i < grid_count; ++i)
        if (counts[i] && scores[i] / counts[i] < scores[best] / counts[best])
            best = i;
    double best_score = scores[best] / counts[best];
    int flat_low = best, flat_high = best;
    while (flat_low > 0 && scores[flat_low - 1] / counts[flat_low - 1] <= best_score * 1.01)
        --flat_low;
    while (flat_high + 1 < grid_count && scores[flat_high + 1] / counts[flat_high + 1] <= best_score * 1.01)
        ++flat_high;
    for (int i = 0; i < grid_count; ++i)
        if (counts[i])
            printf("# badness candidate=%llu avg_us=%.6f\n", (unsigned long long)grid[i], scores[i] / counts[i]);
    printf("# recommended=%llu\n", (unsigned long long)grid[best]);
    printf("# recommended_avg_us=%.6f\n", best_score);
    printf("# flat_within_1pct=%llu..%llu\n", (unsigned long long)grid[flat_low], (unsigned long long)grid[flat_high]);
    printf("# current=%llu\n", (unsigned long long)current);
    printf("# points=%lu\n", counts[best]);
    printf("recommended %s = %llu\n", name, (unsigned long long)grid[best]);
}

static void print_info(void) {
    printf("program=tune_lmmpn_sqr_mersenne_fft\n");
    printf("parameter=SQR_MERSENNE_FFT_THRESHOLD\n");
    printf("consumer=lmmp_sqr_mersenne_ root dispatch (fft-aligned rn only)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)SQR_MERSENNE_FFT_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_SQR_MERSENNE_FFT_THRESHOLD);
    printf("input_range=fft-feasible rn in %d..%d (every 4th feasible point measured)\n", MIN_N, MAX_N);
    printf("candidates=all feasible grid points; non-aligned rn always falls back to dc\n");
    printf("forced_root_equivalence=all_n (gate read once at root; dc/fft internals unbound)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_sqr_mersenne_fft");
        return EXIT_SUCCESS;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    setvbuf(stdout, NULL, _IOLBF, 4096);
    if (options.action == LMMP_TUNE_INFO) {
        print_info();
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current("SQR_MERSENNE_FFT_THRESHOLD", (unsigned long long)SQR_MERSENNE_FFT_THRESHOLD,
                                (unsigned long long)lmmp_tune_SQR_MERSENNE_FFT_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    /* 枚举可行格点：next_size 幂等，须从 rn+1 起步且越界即停。
       k=6 段（rn<1715）粒度 1，逐整数皆可行；其后粒度 128/256 跳变。
       候选域=全格点；测量网格每 GRID_STRIDE 取 1（全格点测量超时间预算），
       评分按部署选择式 rn>=c 逐候选复用测量行。 */
    enum { GRID_CAP = 2048, GRID_STRIDE = 4 };
    mp_size_t cands[GRID_CAP];
    int cand_count = 0;
    mp_size_t grid[GRID_CAP];
    int grid_count = 0;
    mp_size_t next = lmmp_fft_next_size_(MIN_N);
    while (next <= MAX_N && cand_count < GRID_CAP) {
        if (cand_count % GRID_STRIDE == 0)
            grid[grid_count++] = next;
        cands[cand_count++] = next;
        next = lmmp_fft_next_size_(next + 1);
    }

    const mp_size_t input_stride = MAX_N + 4;
    const mp_size_t output_stride = MAX_N + 4;
    mp_ptr storage = alloc_limbs(input_stride + 4 * output_stride);
    mp_ptr full = alloc_limbs(2 * MAX_N + 2);
    if (storage == NULL || full == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;
    mp_ptr low = numa + input_stride;
    mp_ptr high = low + output_stride;
    mp_ptr ref = high + output_stride;
    mp_ptr ones = ref + output_stride; /* 全 1 对抗输入 */
    for (mp_size_t i = 0; i < input_stride; ++i) ones[i] = ~(mp_limb_t)0;
    double* scores = calloc((size_t)cand_count, sizeof(double));
    unsigned long* counts = calloc((size_t)cand_count, sizeof(unsigned long));
    if (scores == NULL || counts == NULL) {
        fprintf(stderr, "cannot allocate score tables\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_size_t saved_threshold = lmmp_tune_SQR_MERSENNE_FFT_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_sqr_mersenne_fft\n");
    printf("# parameter=SQR_MERSENNE_FFT_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)SQR_MERSENNE_FFT_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# input=fft-feasible rn %d..%d, %d measured points (stride %d of %d feasible); candidates=all feasible\n",
           MIN_N, MAX_N, grid_count, GRID_STRIDE, cand_count);
    printf("# forced_root_equivalence=all_n (sub_n=0); DC/FERMAT/MODF keep defaults\n");
    printf("# shape0=full an=rn (scoring); shape1=an=3rn/4+1 (report only)\n");
    lmmp_tune_print_env_comment();
    printf("# rn shape low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (int gi = 0; gi < grid_count && result == EXIT_SUCCESS; ++gi) {
        mp_size_t rn = grid[gi];
        fill_random(numa, rn, (mp_limb_t)(options.seed + 4 * rn));
        for (int shape = 0; shape < 2 && result == EXIT_SUCCESS; ++shape) {
            mp_size_t na = shape == 0 ? rn : 3 * rn / 4 + 1;

            /* 正确性：dc ↔ fft ↔ 全积折叠 oracle；另测全 1 零类对抗输入 */
            lmmp_sqr_mersenne_dc_(low, rn, numa, na);
            lmmp_sqr_mersenne_fft_(high, rn, numa, na);
            mersenne_oracle(ref, rn, numa, na, full);
            if (lmmp_cmp_(low, high, rn) != 0 || lmmp_cmp_(low, ref, rn) != 0) {
                fprintf(stderr, "mersenne sqr result mismatch at rn=%llu shape=%d\n", (unsigned long long)rn, shape);
                result = EXIT_FAILURE;
                break;
            }
            lmmp_sqr_mersenne_dc_(low, rn, ones, na);
            lmmp_sqr_mersenne_fft_(high, rn, ones, na);
            mersenne_oracle(ref, rn, ones, na, full);
            if (lmmp_cmp_(low, high, rn) != 0 || lmmp_cmp_(low, ref, rn) != 0) {
                fprintf(stderr, "mersenne sqr all-ones mismatch at rn=%llu shape=%d\n", (unsigned long long)rn, shape);
                result = EXIT_FAILURE;
                break;
            }

            measure_context low_context = {.dst = low, .numa = numa, .rn = rn, .na = na};
            measure_context high_context = {.dst = high, .numa = numa, .rn = rn, .na = na};
            lmmp_tune_bench low_bench = {
                .bind = bind_default, .reset = reset_output, .call = call_dc, .context = &low_context};
            lmmp_tune_bench high_bench = {
                .bind = bind_default, .reset = reset_output, .call = call_fft, .context = &high_context};
            double low_us, low_mad, high_us, high_mad;
            unsigned retries;
            bool stable;
            if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                        &retries, &stable)) {
                fprintf(stderr, "measurement failed at rn=%llu shape=%d\n", (unsigned long long)rn, shape);
                result = EXIT_FAILURE;
                break;
            }
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("%llu %d %.6f %.6f %.6f %.6f %u %u\n", (unsigned long long)rn, shape, low_us, low_mad, high_us,
                   high_mad, retries, stable);
            if (shape == 0) {
                for (int ci = 0; ci < cand_count; ++ci) {
                    scores[ci] += rn >= cands[ci] ? high_us : low_us;
                    ++counts[ci];
                }
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            recommend_grid("SQR_MERSENNE_FFT_THRESHOLD", scores, counts, cands, cand_count, saved_threshold);
        }
    }
    lmmp_tune_SQR_MERSENNE_FFT_THRESHOLD = saved_threshold;
    free(scores);
    free(counts);
    lmmp_free(full);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
