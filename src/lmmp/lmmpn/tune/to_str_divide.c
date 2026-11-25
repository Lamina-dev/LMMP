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

/* limb→字符串 递归门：TO_STR_DIVIDE_THRESHOLD 在 lmmp_to_str_divide_ 每层以该节点
   na 判 basecase。节点先沿幂表下降（numa<p 时同长度换低一层幂），到首个可整除层
   分裂为商 nq≈na-(np+zeros) 与余数 nr<=np+zeros，较大子问题 ≈na/2。低路直调
   basecase（只读输入）；高路绑 T=na 只影响 na>=T 的节点（含同长度下降链），
   恰好代表候选 (sub_n,na]；T<=sub_n 的候选让子问题再入分治，逐候选绑 T 直调
   divide_ 顶部 deployed 实测。divide_ 破坏 numa：高路/deployed 走分段批处理
   计时（每份副本只调用一次）。幂表用全局十进制幂表（归一化+预逆元），与
   to_decimal_ 部署同构；to_str_ 任意基数的逐调用幂表同为减半几何，近似等价。
   TO_STR_BASEPOW 门不在递归内，保持默认。
   部署约束：候选须 >=3（powers[0] 永不可达的前提）且 <= TO_STR_BASEPOW+1
   （basecase 栈数组按 BASEPOW 定长，DIVIDE 超过它会在生产构建越界）。 */
enum {
    MIN_N = 4, MAX_N = 256, STEP_N = 4, CANDIDATE_MIN = 3, CANDIDATE_MAX = MAX_N, CANDIDATE_STEP = 4,
    CANDIDATE_LAST = CANDIDATE_MIN + (CANDIDATE_MAX - CANDIDATE_MIN) / CANDIDATE_STEP * CANDIDATE_STEP
};

typedef struct {
    mp_byte_t* dst;         /* 输出数字缓冲 */
    mp_srcptr numa;         /* low 路：原始输入直读 */
    mp_ptr pool;            /* high/deployed 路：输入副本池基址 */
    mp_ptr cursor;          /* 批内下一份输入 */
    mp_ptr buf;             /* 工作区：numa 副本 + 递归 tpq */
    mp_size_t pool_stride;
    mp_basepow_t* pows;     /* 全局幂表（归一化，含 invp） */
    int ctop;
    mp_size_t na;
    mp_size_t bind_threshold;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_TO_STR_DIVIDE_THRESHOLD = context->bind_threshold;
}

static void reset_low(void* opaque) {
    measure_context* context = opaque;
    context->dst[0] = 0;
}

/* divide_ 破坏输入：每段计时前铺 count 份副本，每次调用从原始输入出发 */
static void reset_batch(void* opaque, mp_size_t count) {
    measure_context* context = opaque;
    for (mp_size_t i = 0; i < count; ++i)
        lmmp_copy(context->pool + i * context->pool_stride, context->numa, context->na);
    context->cursor = context->pool;
}

static mp_limb_t call_basecase(void* opaque) {
    measure_context* context = opaque;
    mp_size_t digits = lmmp_to_str_basecase_(context->dst, context->numa, context->na, DEC_BASE, '0');
    return (mp_limb_t)digits ^ context->dst[0] ^ context->dst[digits - 1];
}

static mp_limb_t call_divide(void* opaque) {
    measure_context* context = opaque;
    lmmp_copy(context->buf, context->cursor, context->na);
    context->cursor += context->pool_stride;
    mp_size_t digits =
        lmmp_to_str_divide_(context->dst, context->buf, context->na, context->pows + context->ctop,
                            context->buf + context->na + 1, '0');
    return (mp_limb_t)digits ^ context->dst[0] ^ context->dst[digits - 1];
}

/* 双路互比：数字串长度与逐字节相等（high 绑 T=na 强制根层分治）；
   成功时返回位数（high 路缓冲内容保留供往返锚点复用），失配返回 -1 */
static mp_size_t check_pair(measure_context* low, measure_context* high) {
    mp_size_t saved = lmmp_tune_TO_STR_DIVIDE_THRESHOLD;
    mp_size_t digits_low = lmmp_to_str_basecase_(low->dst, low->numa, low->na, DEC_BASE, '0');
    lmmp_copy(high->buf, high->numa, high->na);
    lmmp_tune_TO_STR_DIVIDE_THRESHOLD = high->bind_threshold;
    mp_size_t digits_high = lmmp_to_str_divide_(high->dst, high->buf, high->na, high->pows + high->ctop,
                                                high->buf + high->na + 1, '0');
    lmmp_tune_TO_STR_DIVIDE_THRESHOLD = saved;
    if (digits_low != digits_high)
        return (mp_size_t)-1;
    for (mp_size_t i = 0; i < digits_low; ++i)
        if (low->dst[i] != high->dst[i])
            return (mp_size_t)-1;
    return digits_low;
}

/* 往返锚点：用对侧 from_decimal_ 转回 limb，须与原输入全等 */
static int check_roundtrip(mp_srcptr numa, mp_size_t na, const mp_byte_t* digits, mp_size_t ndigits, mp_ptr back) {
    mp_size_t limbs = lmmp_from_decimal_(back, (const char*)digits, ndigits);
    if (limbs != na || lmmp_cmp_(back, numa, na) != 0)
        return 0;
    return 1;
}

/* 候选按步长稀疏，公共 recommend 的逐整数遍历不适用，自行汇总。
   选择域封顶 saved_basepow_bound()：basecase 栈数组 tp[1+TO_STR_BASEPOW_THRESHOLD]
   以 na<=TO_STR_BASEPOW 为界，divide 递归以 na<T 调 basecase ⟹ 安全 T<=BASEPOW+1；
   超限候选仍参与测量与 badness 诊断输出，只是不进入推荐与平坦带 */
static mp_size_t saved_basepow_bound(void) {
    return lmmp_tune_TO_STR_BASEPOW_THRESHOLD + 1;
}

static void recommend_stepped(const char* name, const double* scores, const unsigned long* counts, int cmin,
                              int cmax, int cstep, mp_size_t current) {
    int select_max = (int)saved_basepow_bound();
    if (select_max > cmax)
        select_max = cmax;
    int best = cmin;
    for (int c = cmin + cstep; c <= select_max; c += cstep)
        if (counts[c] && scores[c] / counts[c] < scores[best] / counts[best])
            best = c;
    double best_score = scores[best] / counts[best];
    int flat_low = best, flat_high = best;
    while (flat_low - cstep >= cmin && scores[flat_low - cstep] / counts[flat_low - cstep] <= best_score * 1.01)
        flat_low -= cstep;
    while (flat_high + cstep <= select_max && scores[flat_high + cstep] / counts[flat_high + cstep] <= best_score * 1.01)
        flat_high += cstep;
    for (int c = cmin; c <= cmax; c += cstep)
        if (counts[c])
            printf("# badness candidate=%d avg_us=%.6f\n", c, scores[c] / counts[c]);
    printf("# select_max=%d (TO_STR_BASEPOW+1, production stack bound)\n", select_max);
    printf("# recommended=%d\n", best);
    printf("# recommended_avg_us=%.6f\n", best_score);
    printf("# flat_within_1pct=%d..%d\n", flat_low, flat_high);
    printf("# current=%llu\n", (unsigned long long)current);
    printf("# points=%lu\n", counts[best]);
    printf("recommended %s = %d\n", name, best);
}

static void print_info(void) {
    printf("program=tune_lmmpn_to_str_divide\n");
    printf("parameter=TO_STR_DIVIDE_THRESHOLD\n");
    printf("consumer=lmmp_to_str_divide_ recursion gate (to_str_/to_decimal_ ladder)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)TO_STR_DIVIDE_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_TO_STR_DIVIDE_THRESHOLD);
    printf("candidate_range=%d..%d step=%d (scan upper bound %d)\n", CANDIDATE_MIN, CANDIDATE_LAST, CANDIDATE_STEP,
           CANDIDATE_MAX);
    printf("input_range=na %d..%d step=%d\n", MIN_N, MAX_N, STEP_N);
    printf("forced_root_equivalence=sub_n=limbs of the larger child (~na/2); candidates T<=sub_n via deployed scan\n");
    printf("constraint=candidates must satisfy 3 <= T; selection capped at TO_STR_BASEPOW+1 (basecase stack array bound)\n");
    printf("note=power ladder mirrors to_decimal_ (global cached table, normalized with precomputed inverses)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_to_str_divide");
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
        lmmp_tune_print_current("TO_STR_DIVIDE_THRESHOLD", (unsigned long long)TO_STR_DIVIDE_THRESHOLD,
                                (unsigned long long)lmmp_tune_TO_STR_DIVIDE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t digitspl = lmmp_bases_table[DEC_BASE - 2].digits_in_limb;
    const mp_size_t max_digits = MAX_N * LIMB_BITS / 3 + 16; /* 位数上界（log10(2)<1/3） */
    const mp_size_t work_size = 4 * MAX_N + 64;              /* numa 副本 + 各层商空间（约 2n） */
    const mp_size_t pool_stride = MAX_N + 2;
    mp_ptr storage = alloc_limbs(2 * (MAX_N + 4) + work_size + LMMP_TUNE_BATCH_COPIES * pool_stride);
    mp_byte_t* digits_low = lmmp_alloc(max_digits);
    mp_byte_t* digits_high = lmmp_alloc(max_digits);
    mp_ptr anchor = alloc_limbs(MAX_N + 4);
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    double deployed_scores[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_TO_STR_DIVIDE_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0, unstable_deployed = 0;

    printf("# program=tune_lmmpn_to_str_divide\n");
    printf("# parameter=TO_STR_DIVIDE_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)TO_STR_DIVIDE_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# candidate_range=%d..%d step=%d (scan upper bound %d) input=na %d..%d step=%d\n", CANDIDATE_MIN,
           CANDIDATE_LAST, CANDIDATE_STEP, CANDIDATE_MAX, MIN_N, MAX_N, STEP_N);
    printf("# high=to_str_divide_ with T=na (root-only incl. same-length power descent); T<=sub_n candidates measured deployed\n");
    printf("# constraint=3 <= T <= TO_STR_BASEPOW+1 (current bound %llu)\n",
           (unsigned long long)(lmmp_tune_TO_STR_BASEPOW_THRESHOLD + 1));
    lmmp_tune_print_env_comment();
    printf("# na low_us low_mad high_us high_mad retries stable sub_n\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t na = MIN_N; na <= MAX_N && result == EXIT_SUCCESS; na += STEP_N) {
        mp_ptr numa_orig = storage;
        fill_random(numa_orig, na, options.seed + 2 * na);

        /* 幂表层次（镜像 to_decimal_） */
        mp_size_t digits = lmmp_to_str_len_(numa_orig, na, DEC_BASE);
        mp_size_t grps = (digits - 1) / digitspl + 1;
        int ctop;
        mp_basepow_t* pows = lmmp_dec_pow_table_(grps, &ctop);

        /* sub_n：从顶层降到首个可整除层，较大子问题规模（adjust<=1 计入上界） */
        int k = ctop;
        while (k > 1 && na + 1 <= pows[k].np + pows[k].zeros)
            --k;
        mp_size_t nr = pows[k].np + pows[k].zeros;
        mp_size_t sub_n = nr > na + 1 - nr ? nr : na + 1 - nr;

        mp_ptr pool = numa_orig + MAX_N + 4;
        mp_ptr buf = pool + LMMP_TUNE_BATCH_COPIES * pool_stride;
        measure_context low_context = {.dst = digits_low,
                                       .numa = numa_orig,
                                       .pool = NULL,
                                       .cursor = NULL,
                                       .buf = NULL,
                                       .pool_stride = pool_stride,
                                       .pows = pows,
                                       .ctop = ctop,
                                       .na = na,
                                       .bind_threshold = saved_threshold};
        measure_context high_context = {.dst = digits_high,
                                        .numa = numa_orig,
                                        .pool = pool,
                                        .cursor = pool,
                                        .buf = buf,
                                        .pool_stride = pool_stride,
                                        .pows = pows,
                                        .ctop = ctop,
                                        .na = na,
                                        .bind_threshold = na};
        mp_size_t digits_cnt = check_pair(&low_context, &high_context);
        if (digits_cnt == (mp_size_t)-1) {
            fprintf(stderr, "to_str divide pair mismatch at na=%llu\n", (unsigned long long)na);
            result = EXIT_FAILURE;
            break;
        }
        if (!check_roundtrip(numa_orig, na, digits_high, digits_cnt, anchor)) {
            fprintf(stderr, "to_str roundtrip mismatch at na=%llu\n", (unsigned long long)na);
            result = EXIT_FAILURE;
            break;
        }

        lmmp_tune_bench low_bench = {
            .bind = bind_threshold, .reset = reset_low, .call = call_basecase, .context = &low_context};
        lmmp_tune_bench high_bench = {
            .bind = bind_threshold, .reset_batch = reset_batch, .call = call_divide, .context = &high_context};
        double low_us, low_mad, high_us, high_mad;
        unsigned retries;
        bool stable;
        if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                    &retries, &stable)) {
            fprintf(stderr, "measurement failed at na=%llu\n", (unsigned long long)na);
            result = EXIT_FAILURE;
            break;
        }
        if (stable)
            ++stable_points;
        else
            ++unstable_points;
        printf("%llu %.6f %.6f %.6f %.6f %u %u %llu\n", (unsigned long long)na, low_us, low_mad, high_us, high_mad,
               retries, stable, (unsigned long long)sub_n);
        /* 只累加步长候选（与 deployed 扫描同域） */
        for (int c = CANDIDATE_MIN; c <= CANDIDATE_MAX; c += CANDIDATE_STEP) {
            if ((mp_size_t)c <= sub_n)
                continue;
            scores[c] += na >= (mp_size_t)c ? high_us : low_us;
            ++counts[c];
        }

        /* deployed 扫描：候选 T<=sub_n 时子问题再入分治，逐候选绑 T 直调顶部实测 */
        for (int candidate = CANDIDATE_MIN; candidate <= (int)sub_n && candidate <= CANDIDATE_MAX;
             candidate += CANDIDATE_STEP) {
            measure_context dep_context = {.dst = digits_high,
                                           .numa = numa_orig,
                                           .pool = pool,
                                           .cursor = pool,
                                           .buf = buf,
                                           .pool_stride = pool_stride,
                                           .pows = pows,
                                           .ctop = ctop,
                                           .na = na,
                                           .bind_threshold = (mp_size_t)candidate};
            lmmp_tune_bench dep_bench = {
                .bind = bind_threshold, .reset_batch = reset_batch, .call = call_divide, .context = &dep_context};
            double dep_us, dep_mad;
            unsigned dep_retries;
            if (!lmmp_tune_measure(&dep_bench, &options, &dep_us, &dep_mad, &dep_retries)) {
                fprintf(stderr, "deployed measurement failed at na=%llu T=%d\n", (unsigned long long)na, candidate);
                result = EXIT_FAILURE;
                break;
            }
            /* 与配对测量同一规则：重试耗尽仍超限的样本保留计分，但计入不稳定并在汇总中提示 */
            if (dep_mad > options.mad_limit)
                ++unstable_deployed;
            deployed_scores[candidate] += dep_us;
            ++counts[candidate];
            printf("# deployed na=%llu T=%d us=%.6f mad=%.6f retries=%u\n", (unsigned long long)na, candidate, dep_us,
                   dep_mad, dep_retries);
        }
    }

    if (result == EXIT_SUCCESS) {
        /* 合并两来源：复用段 (sub_n,na] + deployed 段 T<=sub_n */
        for (int candidate = CANDIDATE_MIN; candidate <= CANDIDATE_MAX; candidate += CANDIDATE_STEP)
            scores[candidate] += deployed_scores[candidate];
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            if (unstable_deployed > 0)
                printf("# note: %u unstable deployed sample(s) retained in scoring\n", unstable_deployed);
            recommend_stepped("TO_STR_DIVIDE_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX,
                              CANDIDATE_STEP, saved_threshold);
        }
    }
    lmmp_tune_TO_STR_DIVIDE_THRESHOLD = saved_threshold;
    lmmp_free(digits_low);
    lmmp_free(digits_high);
    lmmp_free(anchor);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
