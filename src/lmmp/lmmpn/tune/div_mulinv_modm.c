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

/* mulinv 块循环余数更新的三档梯子：
   A=plain 直乘（nb<MODM_T || mn>=nb+ni）
   B=梅森分发不缓存（MODM_T<=nb<CACHE_T，mn=mersenne_next_size(nb+1)）
   C=fft 缓存（nb>=CACHE_T，mn=fft_next_size(nb+1)）
   每点直调 div_mulinv_ 三次，逐路绑 (MODM_T,CACHE_T) 强制档位：
   A=(nb+1,默认)、B=(nb,nb+1)、C=(nb,nb)；mn>=nb+ni 时 B/C 退化 A，记可行性。
   评分按部署选择式逐候选选曲线：MODM 候选配 CACHE 默认、CACHE 候选配 MODM 默认。
   形状组：nq=4nb（ni=nb 四块）与 na=2nb（ni≈nb/2 两块，to_str/截断真实形状）。
   部署可达性：div.c 门 nb>=L 且 na>=2N；to_str/to_decimal 预逆门 np>=L。
   现值 L=CACHE=427 ⟹ nb<427 的 B 档与 MODM 低档不可达，推荐须与 L 联动解读。
   prediv 三路共同、排除计时；MUL_FFT/MERSENNE_* 不绑定。 */
enum {
    MIN_NB = 128, MAX_NB = 2048, STEP_NB = 64,
    MODM_CAND_MIN = 16, MODM_CAND_MAX = 512, MODM_CAND_STEP = 16,
    CACHE_CAND_MIN = 128, CACHE_CAND_MAX = 2048, CACHE_CAND_STEP = 64,
    MAX_NA = 5 * MAX_NB
};

typedef struct {
    mp_ptr dstq;
    mp_ptr numa;
    mp_srcptr numa_orig;
    mp_srcptr numb;
    mp_srcptr invappr;
    mp_size_t na;
    mp_size_t nb;
    mp_size_t ni;
    mp_size_t bind_modm;
    mp_size_t bind_cache;
} measure_context;

static void bind_thresholds(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_DIV_MULINV_MODM_THRESHOLD = context->bind_modm;
    lmmp_tune_DIV_MULINV_MODM_CACHE_THRESHOLD = context->bind_cache;
}

static void reset_input(void* opaque) {
    measure_context* context = opaque;
    lmmp_copy(context->numa, context->numa_orig, context->na);
}

static mp_limb_t call_mulinv(void* opaque) {
    measure_context* context = opaque;
    mp_limb_t qh = lmmp_div_mulinv_(context->dstq, context->numa, context->na, context->numb, context->nb,
                                    context->invappr, context->ni);
    return qh ^ context->dstq[0] ^ context->numa[0];
}

/* 候选按步长稀疏，公共 recommend 的逐整数遍历不适用，自行汇总 */
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
        printf("# badness candidate=%d avg_us=%.6f\n", c, scores[c] / counts[c]);
    printf("# recommended=%d\n", best);
    printf("# recommended_avg_us=%.6f\n", best_score);
    printf("# flat_within_1pct=%d..%d\n", flat_low, flat_high);
    printf("# current=%llu\n", (unsigned long long)current);
    printf("# points=%lu\n", counts[best]);
    printf("recommended %s = %d\n", name, best);
}

static void print_info(void) {
    printf("program=tune_lmmpn_div_mulinv_modm\n");
    printf("parameters=DIV_MULINV_MODM_THRESHOLD,DIV_MULINV_MODM_CACHE_THRESHOLD (three-rung ladder)\n");
    printf("consumer=div_mulinv_ block-loop remainder update; reached via div.c gate nb>=L && na>=2N, to_str np>=L\n");
    printf("binding_mode=runtime-branch\n");
    printf("default_modm=%llu default_cache=%llu\n", (unsigned long long)DIV_MULINV_MODM_THRESHOLD,
           (unsigned long long)DIV_MULINV_MODM_CACHE_THRESHOLD);
    printf("current_modm=%llu current_cache=%llu\n", (unsigned long long)lmmp_tune_DIV_MULINV_MODM_THRESHOLD,
           (unsigned long long)lmmp_tune_DIV_MULINV_MODM_CACHE_THRESHOLD);
    printf("input=nb %d..%d step %d x shapes{nq=4nb, na=2nb}\n", MIN_NB, MAX_NB, STEP_NB);
    printf("modm_candidates=%d..%d step %d; cache_candidates=%d..%d step %d\n", MODM_CAND_MIN, MODM_CAND_MAX,
           MODM_CAND_STEP, CACHE_CAND_MIN, CACHE_CAND_MAX, CACHE_CAND_STEP);
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_div_mulinv_modm");
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
        lmmp_tune_print_current("DIV_MULINV_MODM_THRESHOLD", (unsigned long long)DIV_MULINV_MODM_THRESHOLD,
                                (unsigned long long)lmmp_tune_DIV_MULINV_MODM_THRESHOLD);
        lmmp_tune_print_current("DIV_MULINV_MODM_CACHE_THRESHOLD", (unsigned long long)DIV_MULINV_MODM_CACHE_THRESHOLD,
                                (unsigned long long)lmmp_tune_DIV_MULINV_MODM_CACHE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t stride = MAX_NA + 4;
    mp_ptr storage = alloc_limbs(6 * stride);
    mp_ptr anchor = alloc_limbs(MAX_NA + 8);
    if (storage == NULL || anchor == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa_orig = storage;
    mp_ptr numb = storage + stride;
    mp_ptr numa_work = numb + stride;
    mp_ptr q_out = numa_work + stride;
    mp_ptr invappr = q_out + stride;
    mp_ptr q_ref = invappr + stride; /* A 档商副本，互比基准 */
    double modm_scores[MODM_CAND_MAX + 1] = {0};
    unsigned long modm_counts[MODM_CAND_MAX + 1] = {0};
    double cache_scores[CACHE_CAND_MAX + 1] = {0};
    unsigned long cache_counts[CACHE_CAND_MAX + 1] = {0};
    mp_size_t saved_modm = lmmp_tune_DIV_MULINV_MODM_THRESHOLD;
    mp_size_t saved_cache = lmmp_tune_DIV_MULINV_MODM_CACHE_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_div_mulinv_modm\n");
    printf("# parameters=DIV_MULINV_MODM_THRESHOLD,DIV_MULINV_MODM_CACHE_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default_modm=%llu default_cache=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)DIV_MULINV_MODM_THRESHOLD, (unsigned long long)DIV_MULINV_MODM_CACHE_THRESHOLD,
           options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# rungs: A=plain B=mersenne-dispatch C=fft-cache; per-point forced binding, prediv excluded\n");
    printf("# deployment reachability: nb>=L=%llu (div.c/to_str gate); B rung and low MODM unreachable below L\n",
           (unsigned long long)lmmp_tune_DIV_MULINV_L_THRESHOLD);
    printf("# scoring: MODM candidates paired with default CACHE=%llu; CACHE candidates with default MODM=%llu\n",
           (unsigned long long)saved_cache, (unsigned long long)saved_modm);
    lmmp_tune_print_env_comment();
    printf("# nb nq ni a_us a_mad b_us b_mad c_us c_mad b_feasible c_feasible retries stable\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t nb = MIN_NB; nb <= MAX_NB && result == EXIT_SUCCESS; nb += STEP_NB) {
        fill_random(numb, nb, (mp_limb_t)(options.seed + 8 * nb + 1));
        for (int shape = 0; shape < 2 && result == EXIT_SUCCESS; ++shape) {
            mp_size_t na = shape == 0 ? 5 * nb : 2 * nb;
            mp_size_t nq = na - nb;
            mp_size_t ni = lmmp_div_inv_size_(nq, nb);
            fill_random(numa_orig, na, (mp_limb_t)(options.seed + 8 * nb + shape));
            lmmp_inv_prediv_(invappr, numb, nb, ni); /* 三路共同，排除计时 */

            mp_size_t mn_mrs = lmmp_mersenne_next_size_(nb + 1);
            mp_size_t mn_fft = lmmp_fft_next_size_(nb + 1);
            int b_feasible = mn_mrs < nb + ni;
            int c_feasible = mn_fft < nb + ni;

            measure_context ctx = {.dstq = q_out,
                                   .numa = numa_work,
                                   .numa_orig = numa_orig,
                                   .numb = numb,
                                   .invappr = invappr,
                                   .na = na,
                                   .nb = nb,
                                   .ni = ni,
                                   .bind_modm = saved_modm,
                                   .bind_cache = saved_cache};
            lmmp_tune_bench bench = {.bind = bind_thresholds, .reset = reset_input, .call = call_mulinv, .context = &ctx};
            double us[3], mad[3];
            unsigned retries = 0;
            int all_stable = 1;
            for (int rung = 0; rung < 3; ++rung) {
                ctx.bind_modm = rung == 0 ? nb + 1 : nb;
                ctx.bind_cache = rung == 2 ? nb : (rung == 1 ? nb + 1 : saved_cache);
                unsigned r;
                if (!lmmp_tune_measure(&bench, &options, &us[rung], &mad[rung], &r)) {
                    fprintf(stderr, "measurement failed at nb=%llu shape=%d rung=%d\n", (unsigned long long)nb, shape,
                            rung);
                    result = EXIT_FAILURE;
                    break;
                }
                if (r > retries)
                    retries = r;
                if (mad[rung] > options.mad_limit)
                    all_stable = 0;
            }
            if (result != EXIT_SUCCESS)
                break;
            if (!b_feasible)
                us[1] = us[0];
            if (!c_feasible)
                us[2] = us[0];
            if (all_stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("%llu %llu %llu %.6f %.6f %.6f %.6f %.6f %.6f %d %d %u %d\n", (unsigned long long)nb,
                   (unsigned long long)nq, (unsigned long long)ni, us[0], mad[0], us[1], mad[1], us[2], mad[2],
                   b_feasible, c_feasible, retries, all_stable);

            /* 正确性：三档商余数全等（A 为基准）+ 重建锚点 */
            lmmp_copy(numa_work, numa_orig, na);
            lmmp_tune_DIV_MULINV_MODM_THRESHOLD = nb + 1;
            mp_limb_t qh_a = lmmp_div_mulinv_(q_out, numa_work, na, numb, nb, invappr, ni);
            lmmp_tune_DIV_MULINV_MODM_THRESHOLD = saved_modm;
            lmmp_copy(q_ref, q_out, nq);
            for (int rung = 1; rung < 3; ++rung) {
                lmmp_copy(numa_work, numa_orig, na);
                lmmp_tune_DIV_MULINV_MODM_THRESHOLD = nb;
                lmmp_tune_DIV_MULINV_MODM_CACHE_THRESHOLD = rung == 2 ? nb : nb + 1;
                mp_limb_t qh_x = lmmp_div_mulinv_(q_out, numa_work, na, numb, nb, invappr, ni);
                lmmp_tune_DIV_MULINV_MODM_THRESHOLD = saved_modm;
                lmmp_tune_DIV_MULINV_MODM_CACHE_THRESHOLD = saved_cache;
                if (qh_x != qh_a || lmmp_cmp_(q_out, q_ref, nq) != 0) {
                    fprintf(stderr, "rung result mismatch at nb=%llu shape=%d rung=%d\n", (unsigned long long)nb,
                            shape, rung);
                    result = EXIT_FAILURE;
                    break;
                }
            }
            if (result != EXIT_SUCCESS)
                break;
            /* 锚点：重算 A 路，商余数与公共 div_s_（归一化分派）全等 */
            lmmp_copy(numa_work, numa_orig, na);
            lmmp_tune_DIV_MULINV_MODM_THRESHOLD = nb + 1;
            lmmp_div_mulinv_(q_out, numa_work, na, numb, nb, invappr, ni);
            lmmp_tune_DIV_MULINV_MODM_THRESHOLD = saved_modm;
            lmmp_copy(anchor, numa_orig, na);
            mp_limb_t qh_s = lmmp_div_s_(q_ref, anchor, na, numb, nb);
            if (qh_s != qh_a || lmmp_cmp_(q_out, q_ref, nq) != 0 || lmmp_cmp_(numa_work, anchor, nb) != 0) {
                fprintf(stderr, "anchor mismatch at nb=%llu shape=%d\n", (unsigned long long)nb, shape);
                result = EXIT_FAILURE;
                break;
            }

            /* MODM 评分：候选 m 配默认 CACHE=c0；nb<m→A；mn>=nb+ni→A；否则 nb>=c0→C 余 B */
            for (int m = MODM_CAND_MIN; m <= MODM_CAND_MAX; m += MODM_CAND_STEP) {
                double pick;
                if (nb < (mp_size_t)m || !b_feasible)
                    pick = us[0];
                else if (nb >= saved_cache)
                    pick = c_feasible ? us[2] : us[0];
                else
                    pick = us[1];
                modm_scores[m] += pick;
                ++modm_counts[m];
            }
            /* CACHE 评分：候选 c 配默认 MODM=m0；nb<m0→A；nb>=c 且 fft 可行→C；否则梅森可行→B 余 A */
            for (int c = CACHE_CAND_MIN; c <= CACHE_CAND_MAX; c += CACHE_CAND_STEP) {
                double pick;
                if (nb < saved_modm)
                    pick = us[0];
                else if (nb >= (mp_size_t)c)
                    pick = c_feasible ? us[2] : us[0];
                else
                    pick = b_feasible ? us[1] : us[0];
                cache_scores[c] += pick;
                ++cache_counts[c];
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            printf("# note: candidates below deployment L=%llu are unreachable in current dispatch;\n",
                   (unsigned long long)lmmp_tune_DIV_MULINV_L_THRESHOLD);
            printf("#       interpret recommendations jointly with DIV_MULINV_L_THRESHOLD\n");
            recommend_stepped("DIV_MULINV_MODM_THRESHOLD", modm_scores, modm_counts, MODM_CAND_MIN, MODM_CAND_MAX,
                              MODM_CAND_STEP, saved_modm);
            recommend_stepped("DIV_MULINV_MODM_CACHE_THRESHOLD", cache_scores, cache_counts, CACHE_CAND_MIN,
                              CACHE_CAND_MAX, CACHE_CAND_STEP, saved_cache);
        }
    }
    lmmp_tune_DIV_MULINV_MODM_THRESHOLD = saved_modm;
    lmmp_tune_DIV_MULINV_MODM_CACHE_THRESHOLD = saved_cache;
    lmmp_free(anchor);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
