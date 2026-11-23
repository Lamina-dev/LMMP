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

/* dc 递归入口门（mrs_mul_core_ 每层判 (rn&1)||rn<T）：T 自递归生效，子问题
   rn/2 按同一阈值再判。低路绑 T=rn+1（奇数 ⟹ 根层直乘折叠基例）；高路绑
   T=rn 仅当 T>sub_n=rn/2 时代表部署形态（子问题落基例，与候选 (sub_n,rn]
   的部署逐点一致）；T<=sub_n 的候选让子问题再入分治，逐候选绑 T 经完整
   lmmp_mul_mersenne_dc_ 入口 deployed 实测。
   FERMAT 门保持默认：高路根层半域 n=rn/2，rn>=2*默认FERMAT 时走费马引擎，
   与部署一致；子问题深度 n'=rn/4 起低于默认 FERMAT，走直乘折叠。
   注意：本阈值还决定 lmmp_mersenne_next_size_ 的偶/4/8 取整档位，消费方
   padding 形状随候选移动；本程序在固定 rn 网格评分，该耦合不在评分内，
   由 tmp/ 变体重编探针另行复核。
   形状：主评分=平衡满长 na=nb=rn；副形状 nb=rn/2+1 只输出测量行。 */
enum {
    MIN_N = 64, MAX_N = 2048, STEP_N = 32,
    CANDIDATE_MIN = 32, CANDIDATE_MAX = 2048, CANDIDATE_STEP = 8
};

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_srcptr numb;
    mp_size_t rn;
    mp_size_t na;
    mp_size_t nb;
    mp_size_t bind_threshold;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_MUL_MERSENNE_DC_THRESHOLD = context->bind_threshold;
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
    lmmp_mul_mersenne_dc_(context->dst, context->rn, context->numa, context->na, context->numb, context->nb);
    return fingerprint(context->dst, context->rn);
}

/* oracle：全积 + 循环折叠 + 零类归一化（B^rn-1 -> 0），与 test_mul_mersenne 同手段 */
static void mersenne_oracle(mp_ptr r, mp_size_t rn, mp_srcptr a, mp_size_t na, mp_srcptr b, mp_size_t nb,
                            mp_ptr full) {
    lmmp_zero(full, 2 * rn + 2);
    if (na >= nb)
        lmmp_mul_(full, a, na, b, nb);
    else
        lmmp_mul_(full, b, nb, a, na);
    mp_size_t pn = na + nb;
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

/* 候选按步长稀疏，公共 recommend 的逐整数遍历不适用，自行按均值汇总 */
static void recommend_stepped(const char* name, const double* scores, const unsigned long* counts, int cmin,
                              int cmax, int cstep, mp_size_t current) {
    int best = cmin;
    for (int c = cmin + cstep; c <= cmax; c += cstep)
        if (counts[c] && scores[c] / counts[c] < scores[best] / counts[best])
            best = c;
    double best_score = scores[best] / counts[best];
    int flat_low = best, flat_high = best;
    while (flat_low - cstep >= cmin && scores[flat_low - cstep] / counts[flat_low - cstep] <= best_score * 1.01)
        flat_low -= cstep;
    while (flat_high + cstep <= cmax && scores[flat_high + cstep] / counts[flat_high + cstep] <= best_score * 1.01)
        flat_high += cstep;
    for (int c = cmin; c <= cmax; c += cstep)
        if (counts[c])
            printf("# badness candidate=%d avg_us=%.6f\n", c, scores[c] / counts[c]);
    printf("# recommended=%d\n", best);
    printf("# recommended_avg_us=%.6f\n", best_score);
    printf("# flat_within_1pct=%d..%d\n", flat_low, flat_high);
    printf("# current=%llu\n", (unsigned long long)current);
    printf("# points=%lu\n", counts[best]);
    printf("recommended %s = %d\n", name, best);
}

static void print_info(void) {
    printf("program=tune_lmmpn_mersenne_dc\n");
    printf("parameter=MUL_MERSENNE_DC_THRESHOLD\n");
    printf("consumer=mrs_mul_core_ recursion gate (every level); also mersenne_next_size_ rounding tiers\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)MUL_MERSENNE_DC_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_MUL_MERSENNE_DC_THRESHOLD);
    printf("candidate_range=%d..%d step=%d\n", CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP);
    printf("input_range=even rn %d..%d step=%d\n", MIN_N, MAX_N, STEP_N);
    printf("forced_root_equivalence=sub_n=rn/2; candidates T<=sub_n via deployed scan\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_mersenne_dc");
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
        lmmp_tune_print_current("MUL_MERSENNE_DC_THRESHOLD", (unsigned long long)MUL_MERSENNE_DC_THRESHOLD,
                                (unsigned long long)lmmp_tune_MUL_MERSENNE_DC_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t input_stride = MAX_N + 4;
    const mp_size_t output_stride = MAX_N + 4;
    mp_ptr storage = alloc_limbs(2 * input_stride + 3 * output_stride);
    mp_ptr full = alloc_limbs(2 * MAX_N + 2);
    if (storage == NULL || full == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;
    mp_ptr numb = numa + input_stride;
    mp_ptr low = numb + input_stride;
    mp_ptr high = low + output_stride;
    mp_ptr ref = high + output_stride;
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    double deployed_scores[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_MUL_MERSENNE_DC_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_mersenne_dc\n");
    printf("# parameter=MUL_MERSENNE_DC_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)MUL_MERSENNE_DC_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# candidate_range=%d..%d step=%d input=even rn %d..%d step=%d\n", CANDIDATE_MIN, CANDIDATE_MAX,
           CANDIDATE_STEP, MIN_N, MAX_N, STEP_N);
    printf("# low=T=rn+1 forced basecase; high=T=rn (sub_n=rn/2, root-only); T<=sub_n deployed via dc entry\n");
    printf("# FERMAT/MODF keep defaults; next_size rounding-tier coupling not scored here (tmp variant probe)\n");
    printf("# shape0=balanced na=nb=rn (scoring); shape1=nb=rn/2+1 (report only)\n");
    lmmp_tune_print_env_comment();
    printf("# rn shape low_us low_mad high_us high_mad retries stable sub_n\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t rn = MIN_N; rn <= MAX_N && result == EXIT_SUCCESS; rn += STEP_N) {
        fill_random(numa, rn, (mp_limb_t)(options.seed + 4 * rn));
        mp_size_t sub_n = rn >> 1;
        for (int shape = 0; shape < 2 && result == EXIT_SUCCESS; ++shape) {
            mp_size_t na = rn, nb = shape == 0 ? rn : rn / 2 + 1;
            if (shape == 1)
                fill_random(numb, nb, (mp_limb_t)(options.seed + 4 * rn + 1));
            mp_srcptr bp = shape == 0 ? numa : numb;

            /* 正确性：低路（T=rn+1 基例）↔ 高路（T=rn 分治）↔ 全积折叠 oracle */
            lmmp_tune_MUL_MERSENNE_DC_THRESHOLD = rn + 1;
            lmmp_mul_mersenne_dc_(low, rn, numa, na, bp, nb);
            lmmp_tune_MUL_MERSENNE_DC_THRESHOLD = rn;
            lmmp_mul_mersenne_dc_(high, rn, numa, na, bp, nb);
            lmmp_tune_MUL_MERSENNE_DC_THRESHOLD = saved_threshold;
            mersenne_oracle(ref, rn, numa, na, bp, nb, full);
            if (lmmp_cmp_(low, high, rn) != 0 || lmmp_cmp_(low, ref, rn) != 0) {
                fprintf(stderr, "mersenne dc result mismatch at rn=%llu shape=%d\n", (unsigned long long)rn, shape);
                result = EXIT_FAILURE;
                break;
            }

            measure_context low_context = {
                .dst = low, .numa = numa, .numb = bp, .rn = rn, .na = na, .nb = nb, .bind_threshold = rn + 1};
            measure_context high_context = {
                .dst = high, .numa = numa, .numb = bp, .rn = rn, .na = na, .nb = nb, .bind_threshold = rn};
            lmmp_tune_bench low_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &low_context};
            lmmp_tune_bench high_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &high_context};
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
            printf("%llu %d %.6f %.6f %.6f %.6f %u %u %llu\n", (unsigned long long)rn, shape, low_us, low_mad, high_us,
                   high_mad, retries, stable, (unsigned long long)sub_n);
            if (shape == 0) {
                /* 复用段：候选 T>sub_n 时高路测量即部署形态 */
                for (int c = CANDIDATE_MIN; c <= CANDIDATE_MAX; c += CANDIDATE_STEP) {
                    if ((mp_size_t)c <= sub_n)
                        continue;
                    scores[c] += rn >= (mp_size_t)c ? high_us : low_us;
                    ++counts[c];
                }
                /* deployed 扫描：T<=sub_n 时候选让子问题再入分治，逐候选实测 */
                for (int c = CANDIDATE_MIN; c <= (int)sub_n && c <= CANDIDATE_MAX; c += CANDIDATE_STEP) {
                    measure_context dep_context = {.dst = high,
                                                   .numa = numa,
                                                   .numb = bp,
                                                   .rn = rn,
                                                   .na = na,
                                                   .nb = nb,
                                                   .bind_threshold = (mp_size_t)c};
                    lmmp_tune_bench dep_bench = {
                        .bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &dep_context};
                    double dep_us, dep_mad;
                    unsigned dep_retries;
                    if (!lmmp_tune_measure(&dep_bench, &options, &dep_us, &dep_mad, &dep_retries)) {
                        fprintf(stderr, "deployed measurement failed at rn=%llu T=%d\n", (unsigned long long)rn, c);
                        result = EXIT_FAILURE;
                        break;
                    }
                    deployed_scores[c] += dep_us;
                    ++counts[c];
                    printf("# deployed rn=%llu T=%d us=%.6f mad=%.6f retries=%u\n", (unsigned long long)rn, c, dep_us,
                           dep_mad, dep_retries);
                }
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        for (int c = CANDIDATE_MIN; c <= CANDIDATE_MAX; c += CANDIDATE_STEP)
            scores[c] += deployed_scores[c];
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            recommend_stepped("MUL_MERSENNE_DC_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP,
                              saved_threshold);
        }
    }
    lmmp_tune_MUL_MERSENNE_DC_THRESHOLD = saved_threshold;
    lmmp_free(full);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
