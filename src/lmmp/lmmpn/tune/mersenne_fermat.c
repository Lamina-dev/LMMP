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

/* dc 递归内 mod B^n+1 半域积切换门（mrs_mul_core_：n>=T && mrs_fermat_usable_(n)
   走 lmmp_mul_fermat_，否则直乘+反向折叠）。阈值在递归树每层生效（INV_MODM 型），
   无法根层曲线复用 ⟹ 全 deployed 扫描：逐 (rn,c) 绑候选走完整 dc 入口实测。
   参考曲线 ref=绑超大 T（全树 subfold），逐点比值供交叉定位。
   粒度门 mrs_fermat_usable_：n<1715 要求 n%64==0（best_k=6），非对齐候选与
   下一对齐值部署完全等价 ⟹ 候选只取对齐格点：128..1664 步 64，1792..2048
   步 128（1715..1754 段粒度 128、1915 起 256，步 128 为其超集，等价类注记）。
   rn 网格取偶数（dc 分治要求），rn<2*DC 默认时 dc 退化为基例、门不可达，
   该段候选评分恒等（注记）。DC/MODF 保持默认。
   形状：主评分=平衡满长 na=nb=rn；副形状 nb=rn/2+1 只输出测量行。 */
enum {
    MIN_N = 256, MAX_N = 4096, STEP_N = 128,
    CAND_SMALL_MIN = 128, CAND_SMALL_MAX = 1664, CAND_SMALL_STEP = 64,
    CAND_BIG_MIN = 1792, CAND_BIG_MAX = 2048, CAND_BIG_STEP = 128,
    REF_DISABLE = 1u << 20
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
    lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = context->bind_threshold;
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

/* 候选格点表（对齐粒度），返回格点数 */
static int build_candidates(mp_size_t* cands) {
    int count = 0;
    for (mp_size_t c = CAND_SMALL_MIN; c <= CAND_SMALL_MAX; c += CAND_SMALL_STEP)
        cands[count++] = c;
    for (mp_size_t c = CAND_BIG_MIN; c <= CAND_BIG_MAX; c += CAND_BIG_STEP)
        cands[count++] = c;
    return count;
}

/* 稀疏候选按均值汇总 */
static void recommend_grid(const char* name, const double* scores, const unsigned long* counts, const mp_size_t* cands,
                           int cand_count, mp_size_t current) {
    int best = 0;
    for (int i = 1; i < cand_count; ++i)
        if (counts[i] && scores[i] / counts[i] < scores[best] / counts[best])
            best = i;
    double best_score = scores[best] / counts[best];
    int flat_low = best, flat_high = best;
    while (flat_low > 0 && scores[flat_low - 1] / counts[flat_low - 1] <= best_score * 1.01)
        --flat_low;
    while (flat_high + 1 < cand_count && scores[flat_high + 1] / counts[flat_high + 1] <= best_score * 1.01)
        ++flat_high;
    for (int i = 0; i < cand_count; ++i)
        if (counts[i])
            printf("# badness candidate=%llu avg_us=%.6f\n", (unsigned long long)cands[i], scores[i] / counts[i]);
    printf("# recommended=%llu\n", (unsigned long long)cands[best]);
    printf("# recommended_avg_us=%.6f\n", best_score);
    printf("# flat_within_1pct=%llu..%llu\n", (unsigned long long)cands[flat_low], (unsigned long long)cands[flat_high]);
    printf("# current=%llu\n", (unsigned long long)current);
    printf("# points=%lu\n", counts[best]);
    printf("recommended %s = %llu\n", name, (unsigned long long)cands[best]);
}

static void print_info(void) {
    printf("program=tune_lmmpn_mersenne_fermat\n");
    printf("parameter=MUL_MERSENNE_FERMAT_THRESHOLD\n");
    printf("consumer=mrs_mul_core_ half-domain product gate (every recursion level, alignment-qualified)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)MUL_MERSENNE_FERMAT_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD);
    printf("candidates=aligned grid %d..%d step %d + %d..%d step %d\n", CAND_SMALL_MIN, CAND_SMALL_MAX, CAND_SMALL_STEP,
           CAND_BIG_MIN, CAND_BIG_MAX, CAND_BIG_STEP);
    printf("input_range=even rn %d..%d step=%d (full deployed scan per candidate)\n", MIN_N, MAX_N, STEP_N);
    printf("forced_root_equivalence=none (gate inside recursion; all candidates measured deployed)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_mersenne_fermat");
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
        lmmp_tune_print_current("MUL_MERSENNE_FERMAT_THRESHOLD", (unsigned long long)MUL_MERSENNE_FERMAT_THRESHOLD,
                                (unsigned long long)lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    mp_size_t cands[64];
    int cand_count = build_candidates(cands);

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
    mp_ptr out = numb + input_stride;
    mp_ptr ref_out = out + output_stride;
    mp_ptr oracle_out = ref_out + output_stride;
    double scores[64] = {0};
    unsigned long counts[64] = {0};
    mp_size_t saved_threshold = lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_mersenne_fermat\n");
    printf("# parameter=MUL_MERSENNE_FERMAT_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)MUL_MERSENNE_FERMAT_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# candidates=aligned grid (%d points); input=even rn %d..%d step=%d; full deployed scan\n", cand_count,
           MIN_N, MAX_N, STEP_N);
    printf("# ref=all-subfold curve (T=%u); DC/MODF keep defaults\n", REF_DISABLE);
    printf("# gate unreachable while rn/2 < DC default or rn/2 unaligned; non-aligned candidates equivalent\n");
    printf("# shape0=balanced na=nb=rn (scoring); shape1=nb=rn/2+1 (report only)\n");
    lmmp_tune_print_env_comment();
    printf("# rn shape ref_us ref_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t rn = MIN_N; rn <= MAX_N && result == EXIT_SUCCESS; rn += STEP_N) {
        fill_random(numa, rn, (mp_limb_t)(options.seed + 4 * rn));
        for (int shape = 0; shape < 2 && result == EXIT_SUCCESS; ++shape) {
            mp_size_t na = rn, nb = shape == 0 ? rn : rn / 2 + 1;
            if (shape == 1)
                fill_random(numb, nb, (mp_limb_t)(options.seed + 4 * rn + 1));
            mp_srcptr bp = shape == 0 ? numa : numb;

            /* 正确性：默认门 ↔ 全 subfold ↔ 全积折叠 oracle；另测全 1 零类 */
            lmmp_mul_mersenne_dc_(out, rn, numa, na, bp, nb);
            lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = REF_DISABLE;
            lmmp_mul_mersenne_dc_(ref_out, rn, numa, na, bp, nb);
            lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = saved_threshold;
            mersenne_oracle(oracle_out, rn, numa, na, bp, nb, full);
            if (lmmp_cmp_(out, ref_out, rn) != 0 || lmmp_cmp_(out, oracle_out, rn) != 0) {
                fprintf(stderr, "mersenne fermat result mismatch at rn=%llu shape=%d\n", (unsigned long long)rn, shape);
                result = EXIT_FAILURE;
                break;
            }

            measure_context ref_context = {
                .dst = ref_out, .numa = numa, .numb = bp, .rn = rn, .na = na, .nb = nb, .bind_threshold = REF_DISABLE};
            lmmp_tune_bench ref_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &ref_context};
            double ref_us, ref_mad;
            unsigned ref_retries;
            if (!lmmp_tune_measure(&ref_bench, &options, &ref_us, &ref_mad, &ref_retries)) {
                fprintf(stderr, "ref measurement failed at rn=%llu shape=%d\n", (unsigned long long)rn, shape);
                result = EXIT_FAILURE;
                break;
            }
            int ref_stable = ref_mad <= options.mad_limit;
            printf("%llu %d %.6f %.6f %u %d\n", (unsigned long long)rn, shape, ref_us, ref_mad, ref_retries,
                   ref_stable);

            /* 全 deployed 扫描：逐候选绑 T 走完整 dc 入口 */
            for (int ci = 0; ci < cand_count && result == EXIT_SUCCESS; ++ci) {
                measure_context ctx = {.dst = out,
                                       .numa = numa,
                                       .numb = bp,
                                       .rn = rn,
                                       .na = na,
                                       .nb = nb,
                                       .bind_threshold = cands[ci]};
                lmmp_tune_bench bench = {.bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &ctx};
                double us, mad;
                unsigned retries;
                if (!lmmp_tune_measure(&bench, &options, &us, &mad, &retries)) {
                    fprintf(stderr, "deployed measurement failed at rn=%llu T=%llu\n", (unsigned long long)rn,
                            (unsigned long long)cands[ci]);
                    result = EXIT_FAILURE;
                    break;
                }
                int stable = mad <= options.mad_limit && ref_stable;
                if (stable)
                    ++stable_points;
                else
                    ++unstable_points;
                printf("# deployed rn=%llu shape=%d T=%llu us=%.6f mad=%.6f ratio=%.4f retries=%u stable=%d\n",
                       (unsigned long long)rn, shape, (unsigned long long)cands[ci], us, mad, us / ref_us, retries,
                       stable);
                if (shape == 0) {
                    scores[ci] += us;
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
            recommend_grid("MUL_MERSENNE_FERMAT_THRESHOLD", scores, counts, cands, cand_count, saved_threshold);
        }
    }
    lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = saved_threshold;
    lmmp_free(full);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
