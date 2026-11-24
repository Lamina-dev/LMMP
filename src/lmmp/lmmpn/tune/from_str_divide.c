/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
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
#include "impl/str_conv.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

/* 字符串→limb 递归门：FROM_STR_DIVIDE_THRESHOLD 在 lmmp_from_str_divide_ 每层以
   该段数字串的结果 limb 估计值判 basecase。节点先沿幂表下降（len<=pdigits 时同
   长度换低一层幂），到首个 pdigits<len 的层分裂为 pdigits 与 len-pdigits 两半，
   子问题估计 ≈n/2。低路直调 basecase；高路绑 T=n 只影响估计>=n 的节点（含整条
   同长度下降链，与部署路径一致），恰好代表候选 (sub_n,n]；T<=sub_n 的候选会让
   子问题再入分治，逐候选绑 T 直调 divide_ 顶部 deployed 实测。
   幂表用全局十进制幂表的去归一化副本，与 from_decimal_ 部署同构；from_str_
   任意基数的逐调用幂表同为逐层减半几何，近似等价。FROM_STR_BASEPOW 门不在
   递归内，保持默认。
   部署约束：候选须 >=3——递归调用要求幂层 N=np+zeros>=2，而第 0 层幂
   N=1；触达第 0 层的节点结果 limb 估计 <=2，T>=3 时它们恒走 basecase，
   契约不可达（探针实证：T=2 时全尺寸错值，越界读）。 */
enum {
    MIN_N = 8, MAX_N = 256, STEP_N = 4, CANDIDATE_MIN = MIN_N, CANDIDATE_MAX = MAX_N, CANDIDATE_STEP = 4,
    CANDIDATE_LAST = CANDIDATE_MIN + (CANDIDATE_MAX - CANDIDATE_MIN) / CANDIDATE_STEP * CANDIDATE_STEP
};

typedef struct {
    mp_ptr out;             /* 该路结果缓冲 */
    mp_ptr scratch;         /* divide_ 递归 tpq 暂存（仅 high/deployed 用） */
    mp_basepow_t* upows;    /* 去归一化幂表副本 */
    int ctop;
    const mp_byte_t* src;
    mp_size_t len;
    mp_size_t bind_threshold;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = context->bind_threshold;
}

/* 两路输入只读、输出全写：批次前仅触碰输出首 limb */
static void reset_outputs(void* opaque) {
    measure_context* context = opaque;
    context->out[0] = 0;
}

static mp_limb_t call_basecase(void* opaque) {
    measure_context* context = opaque;
    mp_size_t limbs = lmmp_from_str_basecase_(context->out, context->src, context->len, DEC_BASE, '0');
    return (mp_limb_t)limbs ^ context->out[0] ^ context->out[limbs - 1];
}

static mp_limb_t call_divide(void* opaque) {
    measure_context* context = opaque;
    mp_size_t limbs =
        lmmp_from_str_divide_(context->out, context->src, context->len, context->upows + context->ctop, context->scratch, '0');
    return (mp_limb_t)limbs ^ context->out[0] ^ context->out[limbs - 1];
}

/* 双路互比：limb 数与内容全等（high 绑 T=n 强制根层分治） */
static int check_pair(measure_context* low, measure_context* high) {
    mp_size_t saved = lmmp_tune_FROM_STR_DIVIDE_THRESHOLD;
    mp_size_t limbs_low = lmmp_from_str_basecase_(low->out, low->src, low->len, DEC_BASE, '0');
    lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = high->bind_threshold;
    mp_size_t limbs_high =
        lmmp_from_str_divide_(high->out, high->src, high->len, high->upows + high->ctop, high->scratch, '0');
    lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = saved;
    if (limbs_low != limbs_high || lmmp_cmp_(low->out, high->out, limbs_low) != 0)
        return 0;
    return 1;
}

/* 往返锚点：用对侧 to_decimal_ 转回数字串，须与源串逐位相等（首位非零已保证） */
static int check_roundtrip(const mp_byte_t* src, mp_size_t len, mp_srcptr limbs_buf, mp_size_t limbs, char* rt) {
    mp_size_t digits = lmmp_to_decimal_(rt, limbs_buf, limbs);
    if (digits != len)
        return 0;
    for (mp_size_t i = 0; i < len; ++i)
        if (rt[i] != (char)src[i])
            return 0;
    return 1;
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
    printf("program=tune_lmmpn_from_str_divide\n");
    printf("parameter=FROM_STR_DIVIDE_THRESHOLD\n");
    printf("consumer=lmmp_from_str_divide_ recursion gate (from_str_/from_decimal_ ladder)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)FROM_STR_DIVIDE_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_FROM_STR_DIVIDE_THRESHOLD);
    printf("candidate_range=%d..%d step=%d (scan upper bound %d)\n", CANDIDATE_MIN, CANDIDATE_LAST, CANDIDATE_STEP,
           CANDIDATE_MAX);
    printf("input_range=decimal digits, result limb estimate n %d..%d step=%d\n", MIN_N, MAX_N, STEP_N);
    printf("forced_root_equivalence=sub_n=limb estimate of the larger child (~n/2); candidates T<=sub_n via deployed scan\n");
    printf("constraint=candidates must satisfy T>=3 (level-0 power N=1 must stay basecase)\n");
    printf("note=power ladder mirrors from_decimal_ (global cached table, un-normalized copies)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_from_str_divide");
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
        lmmp_tune_print_current("FROM_STR_DIVIDE_THRESHOLD", (unsigned long long)FROM_STR_DIVIDE_THRESHOLD,
                                (unsigned long long)lmmp_tune_FROM_STR_DIVIDE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    /* 最大数字串长度：MAX_N limb 对应的十进制位数上取齐 */
    mp_size_t max_len = 0;
    while (lmmp_from_str_len_(0, max_len, DEC_BASE) <= MAX_N) ++max_len;
    max_len += 2;

    const mp_size_t pow_region = 4 * MAX_N + 64; /* 结果+各层幂副本（np 之和约 2n） */
    mp_ptr storage = alloc_limbs(pow_region + 4 * (MAX_N + 4));
    mp_byte_t* src = lmmp_alloc(max_len);
    char* rt = lmmp_alloc(max_len + 2);
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    double deployed_scores[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_FROM_STR_DIVIDE_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0, unstable_deployed = 0;

    printf("# program=tune_lmmpn_from_str_divide\n");
    printf("# parameter=FROM_STR_DIVIDE_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)FROM_STR_DIVIDE_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# candidate_range=%d..%d step=%d (scan upper bound %d) input=result limb estimate n %d..%d step=%d\n",
           CANDIDATE_MIN, CANDIDATE_LAST, CANDIDATE_STEP, CANDIDATE_MAX, MIN_N, MAX_N, STEP_N);
    printf("# high=from_str_divide_ with T=n (root-only incl. same-length power descent); T<=sub_n candidates measured deployed\n");
    lmmp_tune_print_env_comment();
    printf("# n low_us low_mad high_us high_mad retries stable sub_n\n");

    int result = EXIT_SUCCESS;
    mp_size_t len_probe = 1;
    for (mp_size_t n = MIN_N; n <= MAX_N && result == EXIT_SUCCESS; n += STEP_N) {
        while (lmmp_from_str_len_(0, len_probe, DEC_BASE) < n) ++len_probe;
        if (lmmp_from_str_len_(0, len_probe, DEC_BASE) != n)
            continue; /* 估计值跳过该整数（步进 0/1，理论上不发生） */
        mp_size_t len = len_probe;

        /* 随机数字串（ASCII 形式，配 off='0'），最高位非零防入口裁剪 */
        mp_limb_t seed = options.seed + 2 * n;
        for (mp_size_t i = 0; i < len; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            src[i] = (mp_byte_t)('0' + seed % 10);
        }
        if (src[len - 1] == '0')
            src[len - 1] = '1';

        /* 幂表与去归一化副本（镜像 from_decimal_） */
        mp_size_t grps = (len - 1) / lmmp_bases_table[DEC_BASE - 2].digits_in_limb + 1;
        int ctop;
        mp_basepow_t* pows = lmmp_dec_pow_table_(grps, &ctop);
        mp_basepow_t upows[LIMB_BITS];
        mp_ptr cp = storage;
        for (int i = 0; i <= ctop; ++i) {
            upows[i] = pows[i];
            lmmp_copy(cp, pows[i].p, pows[i].np);
            if (pows[i].norm_cnt)
                lmmp_shr_(cp, cp, pows[i].np, pows[i].norm_cnt);
            upows[i].p = cp;
            cp += pows[i].np + 1;
        }

        /* sub_n：从顶层降到首个 pdigits<len 的层，较大子问题的估计值 */
        mp_size_t pd = pows[ctop].digits;
        while (len <= pd)
            pd /= 2;
        mp_size_t sub_n = lmmp_from_str_len_(0, pd, DEC_BASE);

        mp_ptr low_out = cp;
        mp_ptr high_out = low_out + MAX_N + 4;
        mp_ptr high_scratch = high_out + MAX_N + 4;
        measure_context low_context = {.out = low_out,
                                       .scratch = NULL,
                                       .upows = upows,
                                       .ctop = ctop,
                                       .src = src,
                                       .len = len,
                                       .bind_threshold = saved_threshold};
        measure_context high_context = {.out = high_out,
                                        .scratch = high_scratch,
                                        .upows = upows,
                                        .ctop = ctop,
                                        .src = src,
                                        .len = len,
                                        .bind_threshold = n};
        if (!check_pair(&low_context, &high_context)) {
            fprintf(stderr, "from_str divide pair mismatch at n=%llu len=%llu\n", (unsigned long long)n,
                    (unsigned long long)len);
            result = EXIT_FAILURE;
            break;
        }
        if (!check_roundtrip(src, len, low_out, lmmp_from_str_basecase_(low_out, src, len, DEC_BASE, '0'), rt)) {
            fprintf(stderr, "from_str roundtrip mismatch at n=%llu len=%llu\n", (unsigned long long)n,
                    (unsigned long long)len);
            result = EXIT_FAILURE;
            break;
        }

        lmmp_tune_bench low_bench = {
            .bind = bind_threshold, .reset = reset_outputs, .call = call_basecase, .context = &low_context};
        lmmp_tune_bench high_bench = {
            .bind = bind_threshold, .reset = reset_outputs, .call = call_divide, .context = &high_context};
        double low_us, low_mad, high_us, high_mad;
        unsigned retries;
        bool stable;
        if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                    &retries, &stable)) {
            fprintf(stderr, "measurement failed at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }
        if (stable)
            ++stable_points;
        else
            ++unstable_points;
        printf("%llu %.6f %.6f %.6f %.6f %u %u %llu\n", (unsigned long long)n, low_us, low_mad, high_us, high_mad,
               retries, stable, (unsigned long long)sub_n);
        /* 只累加步长候选（与 deployed 扫描同域） */
        for (int c = CANDIDATE_MIN; c <= CANDIDATE_MAX; c += CANDIDATE_STEP) {
            if ((mp_size_t)c <= sub_n)
                continue;
            scores[c] += n >= (mp_size_t)c ? high_us : low_us;
            ++counts[c];
        }

        /* deployed 扫描：候选 T<=sub_n 时子问题再入分治，逐候选绑 T 直调顶部实测 */
        for (int candidate = CANDIDATE_MIN; candidate <= (int)sub_n && candidate <= CANDIDATE_MAX;
             candidate += CANDIDATE_STEP) {
            measure_context dep_context = {.out = high_out,
                                           .scratch = high_scratch,
                                           .upows = upows,
                                           .ctop = ctop,
                                           .src = src,
                                           .len = len,
                                           .bind_threshold = (mp_size_t)candidate};
            lmmp_tune_bench dep_bench = {
                .bind = bind_threshold, .reset = reset_outputs, .call = call_divide, .context = &dep_context};
            double dep_us, dep_mad;
            unsigned dep_retries;
            if (!lmmp_tune_measure(&dep_bench, &options, &dep_us, &dep_mad, &dep_retries)) {
                fprintf(stderr, "deployed measurement failed at n=%llu T=%d\n", (unsigned long long)n, candidate);
                result = EXIT_FAILURE;
                break;
            }
            /* 与配对测量同一规则：重试耗尽仍超限的样本保留计分，但计入不稳定并在汇总中提示 */
            if (dep_mad > options.mad_limit)
                ++unstable_deployed;
            deployed_scores[candidate] += dep_us;
            ++counts[candidate];
            printf("# deployed n=%llu T=%d us=%.6f mad=%.6f retries=%u\n", (unsigned long long)n, candidate, dep_us,
                   dep_mad, dep_retries);
        }
    }

    if (result == EXIT_SUCCESS) {
        /* 合并两来源：复用段 (sub_n,n] + deployed 段 T<=sub_n */
        for (int candidate = CANDIDATE_MIN; candidate <= CANDIDATE_MAX; candidate += CANDIDATE_STEP)
            scores[candidate] += deployed_scores[candidate];
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            if (unstable_deployed > 0)
                printf("# note: %u unstable deployed sample(s) retained in scoring\n", unstable_deployed);
            recommend_stepped("FROM_STR_DIVIDE_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX,
                              CANDIDATE_STEP, saved_threshold);
        }
    }
    lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = saved_threshold;
    lmmp_free(src);
    lmmp_free(rt);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
