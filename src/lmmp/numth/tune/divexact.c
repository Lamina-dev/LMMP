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
#include "lmmp/numth.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

/* 精确除法根分派单阈值：dn<DIVEXACT_BASECASE_THRESHOLD 走 basecase_，
   否则 qn<dn 走 divide_、qn>=dn 走 unbalance_/mu（按 nn 分派的阈值已退役——独立探针
 * 证实 dn<=64 时 basecase/mu 比值对 nn 是渐近常数 0.78-0.96，nn 到 4M limb
 *  无交叉，nn 钳制无意义）。DIVEXACT_BASECASE_THRESHOLD 只在 lmmp_divexact_ 根层读一次，
   divide_/unbalance_/mu_run_ 内部不再引用（递归读的是 BINVERT_、MULLO_
   等独立阈值，均不绑定）⟹ 强制绑定对任意 (nn,dn) 干净，sub_n=0、无
   deployed 扫描。每点配对测量：low 绑 DIVEXACT_BASECASE_THRESHOLD=dn+1
   强制 basecase；high 绑 DIVEXACT_BASECASE_THRESHOLD=dn 强制跳过，竞争
   对手由形状决定（qn<dn→divide_，qn>=dn→unbalance_）。
   完整入口对 np 只读（basecase 分支经 TEMP 副本），dst 分离，无需副本池。
   输入构造 np = d×q（镜像 numth/perf/divexact.c），oracle = 商逐 limb 全等。
   评分：候选 c 逐点取 dn<c ? low : high，等权平均（大 nn 点已覆盖到
   12288，dn<DIVEXACT_BASECASE_THRESHOLD 段对 nn 不敏感，见程序头引用的
   探针结论）。 */
enum {
    MIN_DN = 8, MAX_DN = 256, STEP_DN = 4, SHAPE_COUNT = 3,
    CAND_MIN = 8, CAND_MAX = 256, CAND_STEP = 4,
    NN_SCAN_DN_COUNT = 20, /* dn = 8..56 步 8 + 64..256 步 16 */
    NN_SCAN_NN_COUNT = 27, /* 见下方 nn 表 */
    MAX_NN = 12288,
    MAX_POINTS = SHAPE_COUNT * MAX_DN + NN_SCAN_DN_COUNT * NN_SCAN_NN_COUNT
};

static const mp_size_t nn_scan_dn[NN_SCAN_DN_COUNT] = {8,  16, 24, 32,  40,  48,  56,  64,  80,  96,
                                                       112, 128, 144, 160, 176, 192, 208, 224, 240, 256};
static const mp_size_t nn_scan_nn[NN_SCAN_NN_COUNT] = {96,   160,  224,  288,  352,  416,  480,  544,  608,
                                                       672,  704,  768,  896,  1024, 1280, 1536, 1792, 2048,
                                                       2560, 3072, 4096, 5120, 6144, 7168, 8192, 10240, 12288};

typedef struct {
    mp_ptr dst;
    mp_srcptr np;
    mp_srcptr dp;
    mp_size_t nn;
    mp_size_t dn;
    mp_size_t bind_bc;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_DIVEXACT_BASECASE_THRESHOLD = context->bind_bc;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->nn - context->dn + 2);
}

static mp_limb_t call_entry(void* opaque) {
    measure_context* context = opaque;
    lmmp_divexact_(context->dst, context->np, context->nn, context->dp, context->dn);
    return context->dst[0] ^ context->dst[context->nn - context->dn] ^ context->np[0];
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
    while (flat_low - cstep >= cmin && counts[flat_low - cstep] &&
           scores[flat_low - cstep] / counts[flat_low - cstep] <= best_score * 1.01)
        flat_low -= cstep;
    while (flat_high + cstep <= cmax && counts[flat_high + cstep] &&
           scores[flat_high + cstep] / counts[flat_high + cstep] <= best_score * 1.01)
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

/* 按绑定强制跑完整入口并与构造商 q 互比（oracle 锚点） */
static int check_path(measure_context* context, mp_srcptr q, mp_size_t qa, mp_size_t bind_bc) {
    context->bind_bc = bind_bc;
    bind_threshold(context);
    lmmp_zero(context->dst, context->nn - context->dn + 2);
    lmmp_divexact_(context->dst, context->np, context->nn, context->dp, context->dn);
    return lmmp_cmp_(context->dst, q, qa) == 0 && context->dst[qa] == 0;
}

/* 构造 np = d×q：q 占 qa=qn-1 limb、d 占 dn limb，积恰写满 [np,nn)（高位 0 容忍） */
static void build_input(mp_ptr np, mp_srcptr q, mp_srcptr d, mp_size_t qa, mp_size_t dn) {
    if (qa >= dn)
        lmmp_mul_(np, q, qa, d, dn);
    else
        lmmp_mul_(np, d, dn, q, qa);
}

static void print_info(void) {
    printf("program=tune_numth_divexact\n");
    printf("parameter=DIVEXACT_BASECASE_THRESHOLD (root gate, nn unbounded)\n");
    printf("consumer=lmmp_divexact_ root dispatch only (basecase/divide/unbalance)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)DIVEXACT_BASECASE_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_DIVEXACT_BASECASE_THRESHOLD);
    printf("candidates=%d..%d step=%d\n", CAND_MIN, CAND_MAX, CAND_STEP);
    printf("input=shape scan: dn %d..%d step %d x shapes{nn=1.5dn,2dn,4dn}; nn scan: dn 8..256 x nn 96..12288\n",
           MIN_DN, MAX_DN, STEP_DN);
    printf("forced_root_equivalence=all_n (gate read once at root, no recursion)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_divexact");
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
        lmmp_tune_print_current("DIVEXACT_BASECASE_THRESHOLD", (unsigned long long)DIVEXACT_BASECASE_THRESHOLD,
                                (unsigned long long)lmmp_tune_DIVEXACT_BASECASE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t stride = MAX_NN + 4;
    mp_ptr storage = alloc_limbs(4 * stride + MAX_DN + 4);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr np_orig = storage;          /* [0, nn) 构造被除数 d*q */
    mp_ptr q_ref = storage + stride;   /* [0, qa) 构造商 */
    mp_ptr d_buf = storage + 2 * stride;
    mp_ptr dst_low = storage + 2 * stride + MAX_DN + 4;
    mp_ptr dst_high = storage + 3 * stride + MAX_DN + 4;
    double scores[CAND_MAX + 1] = {0};
    unsigned long counts[CAND_MAX + 1] = {0};
    const mp_size_t saved_bc = lmmp_tune_DIVEXACT_BASECASE_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_numth_divexact\n");
    printf("# parameter=DIVEXACT_BASECASE_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)DIVEXACT_BASECASE_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# low=basecase forced (DIVEXACT_BASECASE_THRESHOLD=dn+1); high=DIVEXACT_BASECASE_THRESHOLD=dn forced, competitor by shape (qn<dn divide, qn>=dn unbalance)\n");
    printf("# nn gate retired: dn<DIVEXACT_BASECASE_THRESHOLD applies at any nn (probe: ratio flat 0.78-0.96 to 4M limbs for dn<=64)\n");
    lmmp_tune_print_env_comment();
    printf("# nn dn qn competitor low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (int scan = 0; scan < 2 && result == EXIT_SUCCESS; ++scan) {
        /* scan 0：形状网格（dn 扫满候选域 × 三形状，小中 nn 覆盖）；
           scan 1：nn 阶梯（跨 dns 的 96..12288，大 nn 覆盖） */
        const int scan0_steps = (MAX_DN - MIN_DN) / STEP_DN + 1;
        const int scan0_count = scan0_steps * SHAPE_COUNT;
        const int scan1_count = NN_SCAN_DN_COUNT * NN_SCAN_NN_COUNT;
        const int total = scan == 0 ? scan0_count : scan1_count;
        for (int i = 0; i < total && result == EXIT_SUCCESS; ++i) {
            mp_size_t dn, nn;
            if (scan == 0) {
                dn = MIN_DN + (mp_size_t)(i / SHAPE_COUNT) * STEP_DN;
                const int shape = i % SHAPE_COUNT;
                nn = shape == 0 ? dn + dn / 2 + 2 : (shape == 1 ? 2 * dn : 4 * dn);
            } else {
                dn = nn_scan_dn[i / NN_SCAN_NN_COUNT];
                nn = nn_scan_nn[i % NN_SCAN_NN_COUNT];
                if (nn < dn + 2)
                    continue; /* 入口契约 nn>=dn，留 qn>=3 余量 */
            }
            const mp_size_t qn = nn - dn + 1;
            const mp_size_t qa = qn - 1;
            const char* competitor = qn < dn ? "divide" : "unbalance";

            fill_random(q_ref, qa, (mp_limb_t)(options.seed + 8 * nn + dn));
            fill_random(d_buf, dn, (mp_limb_t)(options.seed + 8 * nn + dn + 1));
            d_buf[0] |= 1;
            build_input(np_orig, q_ref, d_buf, qa, dn);

            measure_context low_context = {
                .dst = dst_low, .np = np_orig, .dp = d_buf, .nn = nn, .dn = dn, .bind_bc = dn + 1};
            measure_context high_context = {
                .dst = dst_high, .np = np_orig, .dp = d_buf, .nn = nn, .dn = dn, .bind_bc = dn};
            if (!check_path(&low_context, q_ref, qa, dn + 1) || !check_path(&high_context, q_ref, qa, dn)) {
                fprintf(stderr, "divexact result mismatch at nn=%llu dn=%llu\n", (unsigned long long)nn,
                        (unsigned long long)dn);
                result = EXIT_FAILURE;
                break;
            }

            lmmp_tune_bench low_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_entry, .context = &low_context};
            lmmp_tune_bench high_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_entry, .context = &high_context};
            double low_us, low_mad, high_us, high_mad;
            unsigned retries;
            bool stable;
            if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                        &retries, &stable)) {
                fprintf(stderr, "measurement failed at nn=%llu dn=%llu\n", (unsigned long long)nn,
                        (unsigned long long)dn);
                result = EXIT_FAILURE;
                break;
            }
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("%llu %llu %llu %s %.6f %.6f %.6f %.6f %u %u\n", (unsigned long long)nn, (unsigned long long)dn,
                   (unsigned long long)qn, competitor, low_us, low_mad, high_us, high_mad, retries, stable);

            for (int c = CAND_MIN; c <= CAND_MAX; c += CAND_STEP) {
                scores[c] += dn < (mp_size_t)c ? low_us : high_us;
                ++counts[c];
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            recommend_stepped("DIVEXACT_BASECASE_THRESHOLD", scores, counts, CAND_MIN, CAND_MAX, CAND_STEP, saved_bc);
        }
    }
    lmmp_tune_DIVEXACT_BASECASE_THRESHOLD = saved_bc;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
