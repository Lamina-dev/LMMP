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

/* divide ↔ mulinv 切换门（耦合，设计稿 §4.3）：部署条件
   nb < DIV_MULINV_L_THRESHOLD || na < 2*DIV_MULINV_N_THRESHOLD 走 divide。
   L 扫描钉 N=1（na<2 恒假）隔离 nb 门；N 扫描钉 L=6（nb>=64 恒假）隔离 na 门，
   形状保 na>=2nb（divide 契约）。两路直调内核：low=divide_（DIV_DIVIDE 默认），
   high=inv_prediv_+div_mulinv_（prediv 计入，与部署同形）。mulinv 内部
   MODM/CACHE/MUL_FFT 均不绑定、沿默认梯子，无同阈值递归，sub_n=0。
   末尾对 (L_rec,N_rec) 与现值做联合复核。截断分支（lmmp_div_ 内 nq 门控，
   无 L 门）不在本程序测量域，输出注记。 */
enum {
    L_MIN_NB = 64, L_MAX_NB = 1024, L_STEP_NB = 64,      /* L 扫描：nq=4nb */
    L_CAND_MIN = 64, L_CAND_MAX = 1024, L_CAND_STEP = 32,
    N_NB = 768, N_MIN_NQ = 768, N_MAX_NQ = 6144, N_STEP_NQ = 256, /* N 扫描：nb 固定 */
    N_CAND_MIN = 768, N_CAND_MAX = 3456, N_CAND_STEP = 128,
    MAX_NA = N_NB + N_MAX_NQ
};

typedef struct {
    mp_ptr dstq;
    mp_ptr numa;
    mp_srcptr numa_orig;
    mp_srcptr numb;
    mp_size_t na;
    mp_size_t nb;
    mp_size_t ni;
    mp_limb_t inv21;
    mp_ptr invappr;
    mp_ptr prediv_src; /* numb+nb-ni 起 ni+1 limb 的副本（prediv 输入） */
    mp_size_t bind_l;
    mp_size_t bind_n;
} measure_context;

static void bind_thresholds(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_DIV_MULINV_L_THRESHOLD = context->bind_l;
    lmmp_tune_DIV_MULINV_N_THRESHOLD = context->bind_n;
}

static void reset_input(void* opaque) {
    measure_context* context = opaque;
    lmmp_copy(context->numa, context->numa_orig, context->na);
}

static mp_limb_t call_divide(void* opaque) {
    measure_context* context = opaque;
    mp_limb_t qh = lmmp_div_divide_(context->dstq, context->numa, context->na, context->numb, context->nb,
                                    context->inv21);
    return qh ^ context->dstq[0] ^ context->numa[0];
}

static mp_limb_t call_mulinv(void* opaque) {
    measure_context* context = opaque;
    lmmp_inv_prediv_(context->invappr, context->numb, context->nb, context->ni);
    mp_limb_t qh = lmmp_div_mulinv_(context->dstq, context->numa, context->na, context->numb, context->nb,
                                    context->invappr, context->ni);
    return qh ^ context->dstq[0] ^ context->numa[0];
}

static int check_pair(measure_context* low, measure_context* high) {
    lmmp_copy(low->numa, low->numa_orig, low->na);
    lmmp_copy(high->numa, high->numa_orig, high->na);
    mp_limb_t qh_low = lmmp_div_divide_(low->dstq, low->numa, low->na, low->numb, low->nb, low->inv21);
    lmmp_inv_prediv_(high->invappr, high->numb, high->nb, high->ni);
    mp_limb_t qh_high = lmmp_div_mulinv_(high->dstq, high->numa, high->na, high->numb, high->nb, high->invappr,
                                         high->ni);
    return qh_low == qh_high && lmmp_cmp_(low->dstq, high->dstq, low->na - low->nb) == 0 &&
           lmmp_cmp_(low->numa, high->numa, low->nb) == 0;
}

/* 锚点：与公共 div_s_（归一化分派，独立路径）的商余数全等。
   q_ref 为参考商缓冲，scratch 存原始被除数副本（算后为参考余数）。 */
static int check_anchor(mp_srcptr numa_orig, mp_size_t na, mp_srcptr numb, mp_size_t nb, mp_limb_t qh,
                        mp_srcptr q, mp_srcptr r, mp_ptr q_ref, mp_ptr scratch) {
    mp_size_t nq = na - nb;
    lmmp_copy(scratch, numa_orig, na);
    mp_limb_t qh_s = lmmp_div_s_(q_ref, scratch, na, numb, nb);
    return qh == qh_s && lmmp_cmp_(q, q_ref, nq) == 0 && lmmp_cmp_(r, scratch, nb) == 0;
}

/* 单形状测量：返回 0 成功；耗时经 low_us、high_us 出参返回 */
static int measure_shape(mp_ptr numa_orig, mp_ptr numb, mp_size_t na, mp_size_t nb,
                         mp_ptr numa_low, mp_ptr q_low, mp_ptr numa_high, mp_ptr q_high, mp_ptr invappr,
                         mp_ptr anchor, const lmmp_tune_options* options, mp_size_t bind_l,
                         mp_size_t bind_n, double* low_us, double* high_us, unsigned* stable_out) {
    mp_size_t nq = na - nb;
    mp_size_t ni = lmmp_div_inv_size_(nq, nb);
    mp_limb_t inv21 = lmmp_inv_2_1_(numb[nb - 1], numb[nb - 2]);
    measure_context low_context = {.dstq = q_low,
                                   .numa = numa_low,
                                   .numa_orig = numa_orig,
                                   .numb = numb,
                                   .na = na,
                                   .nb = nb,
                                   .ni = ni,
                                   .inv21 = inv21,
                                   .invappr = invappr,
                                   .bind_l = bind_l,
                                   .bind_n = bind_n};
    measure_context high_context = low_context;
    high_context.dstq = q_high;
    high_context.numa = numa_high;
    if (!check_pair(&low_context, &high_context)) {
        fprintf(stderr, "mulinv result mismatch at na=%llu nb=%llu\n", (unsigned long long)na,
                (unsigned long long)nb);
        return -1;
    }
    lmmp_copy(numa_low, numa_orig, na);
    mp_limb_t qh = lmmp_div_divide_(q_low, numa_low, na, numb, nb, inv21);
    if (!check_anchor(numa_orig, na, numb, nb, qh, q_low, numa_low, q_high, anchor)) {
        fprintf(stderr, "mulinv anchor mismatch na=%llu nb=%llu\n", (unsigned long long)na,
                (unsigned long long)nb);
        return -1;
    }
    lmmp_tune_bench low_bench = {
        .bind = bind_thresholds, .reset = reset_input, .call = call_divide, .context = &low_context};
    lmmp_tune_bench high_bench = {
        .bind = bind_thresholds, .reset = reset_input, .call = call_mulinv, .context = &high_context};
    double low_mad, high_mad;
    unsigned retries;
    bool stable;
    if (!lmmp_tune_measure_pair(&low_bench, &high_bench, options, low_us, &low_mad, high_us, &high_mad, &retries,
                                &stable)) {
        fprintf(stderr, "measurement failed at na=%llu nb=%llu\n", (unsigned long long)na, (unsigned long long)nb);
        return -1;
    }
    *stable_out = stable;
    printf("%llu %llu %.6f %.6f %.6f %.6f %u %u\n", (unsigned long long)nb, (unsigned long long)nq, *low_us, low_mad,
           *high_us, high_mad, retries, stable);
    return 0;
}

/* 候选按步长稀疏且各候选点数不同，按均值汇总，返回推荐值 */
static int recommend_stepped(const char* name, const double* scores, const unsigned long* counts, int cmin,
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
    return best;
}

static void print_info(void) {
    printf("program=tune_lmmpn_div_mulinv\n");
    printf("parameters=DIV_MULINV_L_THRESHOLD,DIV_MULINV_N_THRESHOLD (coupled)\n");
    printf("consumer=lmmp_div_s_/lmmp_div_ root gate; to_str/to_decimal preinv gate np>=L\n");
    printf("binding_mode=runtime-branch\n");
    printf("default_L=%llu default_N=%llu\n", (unsigned long long)DIV_MULINV_L_THRESHOLD,
           (unsigned long long)DIV_MULINV_N_THRESHOLD);
    printf("current_L=%llu current_N=%llu\n", (unsigned long long)lmmp_tune_DIV_MULINV_L_THRESHOLD,
           (unsigned long long)lmmp_tune_DIV_MULINV_N_THRESHOLD);
    printf("L_scan=nq=4nb nb %d..%d step %d, candidates %d..%d step %d (N pinned 1)\n", L_MIN_NB, L_MAX_NB, L_STEP_NB,
           L_CAND_MIN, L_CAND_MAX, L_CAND_STEP);
    printf("N_scan=nb=%d nq %d..%d step %d, candidates %d..%d step %d (L pinned 6)\n", N_NB, N_MIN_NQ, N_MAX_NQ,
           N_STEP_NQ, N_CAND_MIN, N_CAND_MAX, N_CAND_STEP);
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_div_mulinv");
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
        lmmp_tune_print_current("DIV_MULINV_L_THRESHOLD", (unsigned long long)DIV_MULINV_L_THRESHOLD,
                                (unsigned long long)lmmp_tune_DIV_MULINV_L_THRESHOLD);
        lmmp_tune_print_current("DIV_MULINV_N_THRESHOLD", (unsigned long long)DIV_MULINV_N_THRESHOLD,
                                (unsigned long long)lmmp_tune_DIV_MULINV_N_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t stride = MAX_NA + 4;
    mp_ptr storage = alloc_limbs(7 * stride);
    mp_ptr anchor = alloc_limbs(MAX_NA + 8);
    if (storage == NULL || anchor == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa_orig = storage;
    mp_ptr numb = storage + stride;
    mp_ptr numa_low = numb + stride;
    mp_ptr q_low = numa_low + stride;
    mp_ptr numa_high = q_low + stride;
    mp_ptr q_high = numa_high + stride;
    mp_ptr invappr = q_high + stride;
    double l_scores[L_CAND_MAX + 1] = {0};
    unsigned long l_counts[L_CAND_MAX + 1] = {0};
    double n_scores[N_CAND_MAX + 1] = {0};
    unsigned long n_counts[N_CAND_MAX + 1] = {0};
    mp_size_t saved_l = lmmp_tune_DIV_MULINV_L_THRESHOLD;
    mp_size_t saved_n = lmmp_tune_DIV_MULINV_N_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_div_mulinv\n");
    printf("# parameters=DIV_MULINV_L_THRESHOLD,DIV_MULINV_N_THRESHOLD (coupled, joint recheck at end)\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default_L=%llu default_N=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)DIV_MULINV_L_THRESHOLD, (unsigned long long)DIV_MULINV_N_THRESHOLD, options.seed,
           options.samples, options.min_ms, options.mad_limit);
    printf("# low=divide_ direct (DIV_DIVIDE default=%llu); high=inv_prediv_+div_mulinv_ (prediv timed)\n",
           (unsigned long long)lmmp_tune_DIV_DIVIDE_THRESHOLD);
    printf("# MODM/CACHE/MUL_FFT unbound (default ladder); truncated branch of lmmp_div_ not covered here\n");
    lmmp_tune_print_env_comment();

    int result = EXIT_SUCCESS;

    /* ---- L 扫描：nq=4nb，钉 N=1 ---- */
    printf("# phase=L_scan nb nq low_us low_mad high_us high_mad retries stable\n");
    for (mp_size_t nb = L_MIN_NB; nb <= L_MAX_NB; nb += L_STEP_NB) {
        mp_size_t na = 5 * nb;
        fill_random(numa_orig, na, (mp_limb_t)(options.seed + 8 * nb));
        fill_random(numb, nb, (mp_limb_t)(options.seed + 8 * nb + 1));
        double low_us, high_us;
        unsigned stable;
        if (measure_shape(numa_orig, numb, na, nb, numa_low, q_low, numa_high, q_high, invappr, anchor,
                          &options, saved_l, 1, &low_us, &high_us, &stable) != 0) {
            result = EXIT_FAILURE;
            break;
        }
        if (stable)
            ++stable_points;
        else
            ++unstable_points;
        for (int c = L_CAND_MIN; c <= L_CAND_MAX; c += L_CAND_STEP) {
            l_scores[c] += nb >= (mp_size_t)c ? high_us : low_us;
            ++l_counts[c];
        }
    }

    /* ---- N 扫描：nb 固定，钉 L=6 ---- */
    if (result == EXIT_SUCCESS) {
        printf("# phase=N_scan nb nq low_us low_mad high_us high_mad retries stable\n");
        fill_random(numb, N_NB, (mp_limb_t)(options.seed + 0x9e3779b97f4a7c15ULL));
        for (mp_size_t nq = N_MIN_NQ; nq <= N_MAX_NQ; nq += N_STEP_NQ) {
            mp_size_t na = N_NB + nq;
            fill_random(numa_orig, na, (mp_limb_t)(options.seed + 8 * nq + 3));
            double low_us, high_us;
            unsigned stable;
            if (measure_shape(numa_orig, numb, na, N_NB, numa_low, q_low, numa_high, q_high, invappr, anchor,
                              &options, 6, saved_n, &low_us, &high_us, &stable) != 0) {
                result = EXIT_FAILURE;
                break;
            }
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            for (int c = N_CAND_MIN; c <= N_CAND_MAX; c += N_CAND_STEP) {
                n_scores[c] += na >= 2 * (mp_size_t)c ? high_us : low_us;
                ++n_counts[c];
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            /* 两扫描点数不同，推荐必须按均值口径（总分偏向点数少的候选） */
            int l_rec = recommend_stepped("DIV_MULINV_L_THRESHOLD", l_scores, l_counts, L_CAND_MIN, L_CAND_MAX,
                                          L_CAND_STEP, saved_l);
            int n_rec = recommend_stepped("DIV_MULINV_N_THRESHOLD", n_scores, n_counts, N_CAND_MIN, N_CAND_MAX,
                                          N_CAND_STEP, saved_n);
            printf("# phase=joint_recheck L_rec=%d N_rec=%d vs current L=%llu N=%llu\n", l_rec, n_rec,
                   (unsigned long long)saved_l, (unsigned long long)saved_n);
            printf("# joint nb nq rec_us cur_us ratio\n");
            const mp_size_t joint_nb[] = {256, 512, 768};
            for (int s = 0; s < 3 && result == EXIT_SUCCESS; ++s) {
                mp_size_t nb = joint_nb[s];
                fill_random(numb, nb, (mp_limb_t)(options.seed + 16 * nb + 5));
                for (int k = 2; k <= 8; k *= 2) {
                    mp_size_t nq = (mp_size_t)k * nb, na = nb + nq;
                    fill_random(numa_orig, na, (mp_limb_t)(options.seed + 16 * nb + k));
                    /* 部署选择：nb<L || na<2N → divide，否则 mulinv */
                    int rec_mulinv = !(nb < (mp_size_t)l_rec || na < 2 * (mp_size_t)n_rec);
                    int cur_mulinv = !(nb < saved_l || na < 2 * saved_n);
                    double low_us, high_us;
                    unsigned stable;
                    if (measure_shape(numa_orig, numb, na, nb, numa_low, q_low, numa_high, q_high, invappr,
                                      anchor, &options, saved_l, saved_n, &low_us, &high_us, &stable) != 0) {
                        result = EXIT_FAILURE;
                        break;
                    }
                    double rec_us = rec_mulinv ? high_us : low_us;
                    double cur_us = cur_mulinv ? high_us : low_us;
                    printf("# joint %llu %llu %.6f %.6f %.6f\n", (unsigned long long)nb, (unsigned long long)nq,
                           rec_us, cur_us, rec_us / cur_us);
                }
            }
        }
    }
    lmmp_tune_DIV_MULINV_L_THRESHOLD = saved_l;
    lmmp_tune_DIV_MULINV_N_THRESHOLD = saved_n;
    lmmp_free(anchor);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
