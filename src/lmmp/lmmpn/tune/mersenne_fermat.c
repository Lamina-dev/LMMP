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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lmmp/lmmpn.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

/* dc 递归内 mod B^n+1 半域积切换阈值（mrs_mul_core_：n>=T && mrs_fermat_usable_(n)
   走 lmmp_mul_fermat_，否则直乘后对 B^n+1 取余）。阈值在递归树每层生效（INV_MODM 型），
   无法根层曲线复用。可用粒度：mrs_fermat_usable_ 要求 n 是 2^(best_k-6) 的倍数；
   候选域 128..2048 内 best_k 几乎全是 6，即逐整数可行，只有个别区间要求偶数。
   故 64 格点之外的整数都是独立的一类阈值，不存在"非对齐候选与下一对齐值等价"。
   两阶段扫描：
     粗扫：候选取 64 格点上的可用尺寸，逐 (rn,T) 绑候选走完整 dc 入口实测。其半域
           长度全是 64 的倍数，分不开格点之间的阈值，只把交叉定位到 ±128；
     细扫：窗口=粗扫最优 ±128，候选为窗口内每个整数。网格点 rn=2^k·n（k=1..3，n 取
           窗口内每个可用整数）上按部署路径给候选分类：阈值判别只出现在半域积节点 …n/2、n、2n、
           4n 上，每类绑一个代表阈值走完整 dc 入口实测（low/high 配对，第三路单独测，
           分类与适用条件见 scan_fine）。每个候选在每个网格点恰有一条与其部署路径相同的
           测量 ⟹ 点集对全部候选一致，窗口被 CAND_MIN 钳制时上半部候选仍由 n>=T 的
           网格点区分，不会因共用 low 路得分相同。逐点差值带每次运行
           各异的系统偏移（数 µs 量级），只有趋势可复现，最小值位置以多轮平坦带交集
           为准。最优贴近窗口边缘时整体外移重扫（至多 FINE_MAX_SHIFTS 次）；推荐值
           以细扫为准。
   参考曲线 ref=绑超大 T（全树都走直乘后对 B^n+1 取余），逐点比值供交叉定位。
   DC/MODF 保持默认；rn 取偶数（dc 分治要求），rn<2*DC 默认时 dc 退化为基例、
   该阈值不可达，该段候选评分恒等（注记）。
   形状：粗测两形状，主评分=平衡满长 na=nb=rn，副形状 nb=rn/2+1 只输出测量行；
   细扫只测主形状。 */
enum {
    MIN_N = 256, MAX_N = 4096, STEP_N = 128,
    CAND_MIN = 128, CAND_MAX = 2048, COARSE_STEP = 64,
    FINE_HALF_WIDTH = 128, FINE_LEVELS = 3, FINE_MAX_SHIFTS = 1, FINE_EDGE_MARGIN = 16,
    FINE_MAX_RN = (1 << FINE_LEVELS) * CAND_MAX, MAX_COARSE_CANDS = CAND_MAX / COARSE_STEP + 1,
    REF_DISABLE = 1u << 20
};
/* 逐点路径分类（scan_fine）的前提：窗口上沿 < 4*窗口下沿，使 n/4 恒低于 cand_min、4n 恒不低于
   cand_max、deep 与 upper 不并存 */
_Static_assert(2 * FINE_HALF_WIDTH < 3 * CAND_MIN, "fine window too wide for the per-point path classes");

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

/* oracle：全积 + 梅森折叠 + 零类归一化（B^rn-1 -> 0），与 test_mul_mersenne 同手段 */
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

/* 费马变换可行尺寸，与库内 mrs_fermat_usable_ 同判据（n 是 2^(best_k-6) 的倍数）：
   经公共 fft_next_size 的幂等性判定，不内嵌副本 */
static int fermat_usable(mp_size_t n) {
    return lmmp_fft_next_size_(n) == n;
}

/* 粗扫候选：[CAND_MIN,CAND_MAX] 内 COARSE_STEP 格点上的可用尺寸，返回个数 */
static int build_coarse(mp_size_t* cands) {
    int count = 0;
    for (mp_size_t c = CAND_MIN; c <= CAND_MAX; c += COARSE_STEP)
        if (fermat_usable(c))
            cands[count++] = c;
    return count;
}

/* 稀疏候选按均值取最优下标 */
static int best_index(const double* scores, const unsigned long* counts, int cand_count) {
    int best = 0;
    for (int i = 1; i < cand_count; ++i)
        if (counts[i] && scores[i] / counts[i] < scores[best] / counts[best])
            best = i;
    return best;
}

typedef struct {
    mp_ptr out;
    mp_ptr ref_out;
    mp_ptr oracle_out;
    mp_ptr full;
} work_buffers;

/* 测一个粗扫 (rn,shape) 点并返回 0：先核正确性（默认阈值 ↔ 全部走直乘后对 B^n+1 取余 ↔ 全积梅森折叠 oracle），
   再测参考曲线与逐候选 deployed 耗时；scoring 非零时把候选耗时累加进评分 */
static int scan_coarse(mp_size_t rn, int shape, mp_srcptr numa, mp_srcptr numb, mp_size_t na, mp_size_t nb,
                       const mp_size_t* cands, int cand_count, double* scores, unsigned long* counts, int scoring,
                       const work_buffers* work, const lmmp_tune_options* options, mp_size_t saved_threshold,
                       unsigned* stable_points, unsigned* unstable_points) {
    /* 上一点的候选循环把阈值留在最后一个候选上，默认阈值校验前显式恢复 */
    lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = saved_threshold;
    lmmp_mul_mersenne_dc_(work->out, rn, numa, na, numb, nb);
    lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = REF_DISABLE;
    lmmp_mul_mersenne_dc_(work->ref_out, rn, numa, na, numb, nb);
    lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = saved_threshold;
    mersenne_oracle(work->oracle_out, rn, numa, na, numb, nb, work->full);
    if (lmmp_cmp_(work->out, work->ref_out, rn) != 0 || lmmp_cmp_(work->out, work->oracle_out, rn) != 0) {
        fprintf(stderr, "mersenne fermat result mismatch at rn=%llu shape=%d\n", (unsigned long long)rn, shape);
        return -1;
    }

    measure_context ref_context = {
        .dst = work->ref_out, .numa = numa, .numb = numb, .rn = rn, .na = na, .nb = nb, .bind_threshold = REF_DISABLE};
    lmmp_tune_bench ref_bench = {
        .bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &ref_context};
    double ref_us, ref_mad;
    unsigned ref_retries;
    if (!lmmp_tune_measure(&ref_bench, options, &ref_us, &ref_mad, &ref_retries)) {
        fprintf(stderr, "ref measurement failed at rn=%llu shape=%d\n", (unsigned long long)rn, shape);
        return -1;
    }
    int ref_stable = ref_mad <= options->mad_limit;
    printf("%llu %d %.6f %.6f %u %d\n", (unsigned long long)rn, shape, ref_us, ref_mad, ref_retries, ref_stable);

    /* 全 deployed 扫描：逐候选绑 T 走完整 dc 入口 */
    for (int ci = 0; ci < cand_count; ++ci) {
        measure_context ctx = {
            .dst = work->out, .numa = numa, .numb = numb, .rn = rn, .na = na, .nb = nb, .bind_threshold = cands[ci]};
        lmmp_tune_bench bench = {.bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &ctx};
        double us, mad;
        unsigned retries;
        if (!lmmp_tune_measure(&bench, options, &us, &mad, &retries)) {
            fprintf(stderr, "deployed measurement failed at rn=%llu T=%llu\n", (unsigned long long)rn,
                    (unsigned long long)cands[ci]);
            return -1;
        }
        int stable = mad <= options->mad_limit && ref_stable;
        if (stable)
            ++*stable_points;
        else
            ++*unstable_points;
        printf("# deployed rn=%llu shape=%d T=%llu us=%.6f mad=%.6f ratio=%.4f retries=%u stable=%d\n",
               (unsigned long long)rn, shape, (unsigned long long)cands[ci], us, mad, us / ref_us, retries, stable);
        if (scoring) {
            scores[ci] += us;
            ++counts[ci];
        }
    }
    return 0;
}

/* 细扫逐点评分：候选按部署路径归类，取所属那一路的耗时，每个候选计 1 点。
   deep/upper 非零时 extra_us 分别是 deep 路（T<=n/2）与 upper 路（T>2n）的耗时 */
static void score_fine_point(double* scores, unsigned long* counts, mp_size_t cand_min, mp_size_t cand_max,
                             mp_size_t n, bool deep, bool upper, double low_us, double high_us, double extra_us) {
    for (mp_size_t c = cand_min; c <= cand_max; ++c) {
        double us = c <= n ? high_us : low_us;
        if (deep && c <= n / 2) us = extra_us;
        if (upper && c > 2 * n) us = extra_us;
        scores[c] += us;
        ++counts[c];
    }
}

/* 测一个细扫网格点 rn=2^level·n 并返回 0：先核正确性（各路绑定与默认阈值 ↔ 全部走直乘后对 B^n+1 取余 ↔ 全积
   梅森折叠 oracle），再测各路耗时，交 score_fine_point。阈值判别只出现在半域积节点 …n/2、n、2n、4n 上
   （节点 s>=T 且可用时走费马），同一可用节点两侧的候选部署路径不同，故按路径分类、每类绑
   一个代表阈值走完整 dc 入口：
     low   T=n+1     T>n
     high  T=n       n/2<T<=n
     deep  T=n/2     T<=n/2；仅当节点 n/2 由阈值决定走向（n 偶、n>=DC、可用）且不低于 cand_min
     upper T=2n+1    T>2n；仅当 level>=2 且可用节点 2n 落在窗口内
   更深的节点 n/4 恒低于 cand_min（窗口宽 <=256、cand_min>=128），更浅的 4n 恒不低于 cand_max，
   都不区分候选；deep 与 upper 不能并存（并存要求 cand_max>4*cand_min）。每个候选在每个网格点
   恰有一条与其部署路径相同的测量，点集对全部候选一致。 */
static int scan_fine(mp_size_t rn, mp_size_t n, int level, mp_srcptr numa, const work_buffers* work,
                     const lmmp_tune_options* options, mp_size_t saved_threshold, mp_size_t cand_min,
                     mp_size_t cand_max, double* scores, unsigned long* counts, unsigned* stable_points,
                     unsigned* unstable_points) {
    const bool deep = n >= 2 * cand_min && (n & 1) == 0 && n >= MUL_MERSENNE_DC_THRESHOLD && fermat_usable(n / 2);
    const bool upper = level >= 2 && 2 * n < cand_max && fermat_usable(2 * n);
    const mp_size_t extra_gate = deep ? n / 2 : 2 * n + 1;
    lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = REF_DISABLE;
    lmmp_mul_mersenne_dc_(work->ref_out, rn, numa, rn, numa, rn);
    lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = saved_threshold;
    mersenne_oracle(work->oracle_out, rn, numa, rn, numa, rn, work->full);
    if (lmmp_cmp_(work->ref_out, work->oracle_out, rn) != 0) {
        fprintf(stderr, "mersenne fermat ref/oracle mismatch at rn=%llu\n", (unsigned long long)rn);
        return -1;
    }
    const mp_size_t gates[4] = {saved_threshold, n + 1, n, extra_gate};
    for (int gi = 0; gi < (deep || upper ? 4 : 3); ++gi) {
        lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = gates[gi];
        lmmp_mul_mersenne_dc_(work->out, rn, numa, rn, numa, rn);
        if (lmmp_cmp_(work->out, work->ref_out, rn) != 0) {
            lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = saved_threshold;
            fprintf(stderr, "mersenne fermat result mismatch at rn=%llu T=%llu\n", (unsigned long long)rn,
                    (unsigned long long)gates[gi]);
            return -1;
        }
    }
    lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = saved_threshold;

    measure_context low_context = {
        .dst = work->out, .numa = numa, .numb = numa, .rn = rn, .na = rn, .nb = rn, .bind_threshold = n + 1};
    measure_context high_context = low_context;
    high_context.bind_threshold = n;
    lmmp_tune_bench low_bench = {
        .bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &low_context};
    lmmp_tune_bench high_bench = {
        .bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &high_context};
    double low_us, low_mad, high_us, high_mad;
    unsigned retries;
    bool stable;
    if (!lmmp_tune_measure_pair(&low_bench, &high_bench, options, &low_us, &low_mad, &high_us, &high_mad, &retries,
                                &stable)) {
        fprintf(stderr, "pair measurement failed at rn=%llu n=%llu level=%d\n", (unsigned long long)rn,
                (unsigned long long)n, level);
        return -1;
    }
    /* 第三路单独测量：它与配对两路之间只差单点的系统偏移，只影响跨该节点的相邻候选 */
    double extra_us = 0.0, extra_mad = 0.0;
    unsigned extra_retries = 0;
    if (deep || upper) {
        measure_context extra_context = low_context;
        extra_context.bind_threshold = extra_gate;
        lmmp_tune_bench extra_bench = {
            .bind = bind_threshold, .reset = reset_output, .call = call_dc, .context = &extra_context};
        if (!lmmp_tune_measure(&extra_bench, options, &extra_us, &extra_mad, &extra_retries)) {
            fprintf(stderr, "extra path measurement failed at rn=%llu n=%llu level=%d\n", (unsigned long long)rn,
                    (unsigned long long)n, level);
            return -1;
        }
        stable = stable && extra_mad <= options->mad_limit;
    }
    if (stable)
        ++*stable_points;
    else
        ++*unstable_points;
    printf("# fine n=%llu level=%d rn=%llu low_us=%.6f low_mad=%.6f high_us=%.6f high_mad=%.6f retries=%u stable=%d",
           (unsigned long long)n, level, (unsigned long long)rn, low_us, low_mad, high_us, high_mad, retries, stable);
    if (deep || upper) {
        const char* tag = deep ? "deep" : "upper";
        printf(" %s_us=%.6f %s_mad=%.6f %s_retries=%u", tag, extra_us, tag, extra_mad, tag, extra_retries);
    }
    printf("\n");
    score_fine_point(scores, counts, cand_min, cand_max, n, deep, upper, low_us, high_us, extra_us);
    return 0;
}

static void print_info(void) {
    printf("program=tune_lmmpn_mersenne_fermat\n");
    printf("parameter=MUL_MERSENNE_FERMAT_THRESHOLD\n");
    printf("consumer=mrs_mul_core_ half-domain product gate (every recursion level, fermat-usable sizes only)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)MUL_MERSENNE_FERMAT_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD);
    printf("fermat_usable=n is a multiple of 2^(best_k-6); most of the candidate domain has granularity 1\n");
    printf("coarse: candidates=usable sizes on the %d-lattice %d..%d; input=even rn %d..%d step=%d\n", COARSE_STEP,
           CAND_MIN, CAND_MAX, MIN_N, MAX_N, STEP_N);
    printf("        shapes{na=nb=rn scoring, nb=rn/2+1 report only}; locates the fine window only\n");
    printf("fine: candidates=every integer within coarse best +/-%d (up to %d edge shift of %d)\n", FINE_HALF_WIDTH,
           FINE_MAX_SHIFTS, FINE_HALF_WIDTH);
    printf("      input=rn in {2n,4n,8n} over every usable n of the window, shape0; candidates are\n");
    printf("      grouped by deployed path per point: paired T=n+1 (T>n) vs T=n (n/2<T<=n), plus a\n");
    printf("      single T=n/2 run when the n/2 node is gated (T<=n/2) or T=2n+1 when 2n<window_hi\n");
    printf("      (T>2n, level>=2); every candidate is measured at every point (same point set for all);\n");
    printf("      recommendation comes from this phase\n");
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

    mp_size_t cands[MAX_COARSE_CANDS];
    int cand_count = build_coarse(cands);

    const mp_size_t input_stride = FINE_MAX_RN + 4; /* 细扫网格点 rn 可达 8*CAND_MAX */
    const mp_size_t output_stride = FINE_MAX_RN + 4;
    mp_ptr storage = alloc_limbs(2 * input_stride + 3 * output_stride);
    mp_ptr full = alloc_limbs(2 * FINE_MAX_RN + 2);
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
    work_buffers work = {.out = out, .ref_out = ref_out, .oracle_out = oracle_out, .full = full};
    double coarse_scores[MAX_COARSE_CANDS] = {0};
    unsigned long coarse_counts[MAX_COARSE_CANDS] = {0};
    double fine_scores[CAND_MAX + 1] = {0};
    unsigned long fine_counts[CAND_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD;
    unsigned coarse_stable = 0, coarse_unstable = 0, fine_stable = 0, fine_unstable = 0;

    printf("# program=tune_lmmpn_mersenne_fermat\n");
    printf("# parameter=MUL_MERSENNE_FERMAT_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)MUL_MERSENNE_FERMAT_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# candidates: coarse=usable sizes on the %d-lattice (%d points); fine=every integer within coarse best\n",
           COARSE_STEP, cand_count);
    printf("#   +/-%d; off-lattice thresholds are distinct classes, not equivalents\n", FINE_HALF_WIDTH);
    printf("# per-point paired deltas carry a run-specific offset of a few us; only the trend repeats across runs,\n");
    printf("#   so read the fine result as its flat band intersected over rounds, not as an exact minimum\n");
    printf("# ref=no-fermat curve (T=%u); DC/MODF keep defaults\n", REF_DISABLE);
    printf("# gate unreachable while rn/2 < DC default; coarse grid half-sizes are all multiples of 64\n");
    printf("# shape0=balanced na=nb=rn (scoring); shape1=nb=rn/2+1 (coarse only, report only)\n");
    lmmp_tune_print_env_comment();

    /* ---- 粗扫：64 格点候选 × 两形状，只用于定位细扫窗口 ---- */
    printf("# phase=coarse rn shape ref_us ref_mad retries stable\n");
    int result = EXIT_SUCCESS;
    for (mp_size_t rn = MIN_N; rn <= MAX_N && result == EXIT_SUCCESS; rn += STEP_N) {
        fill_random(numa, rn, (mp_limb_t)(options.seed + 4 * rn));
        for (int shape = 0; shape < 2 && result == EXIT_SUCCESS; ++shape) {
            mp_size_t na = rn, nb = shape == 0 ? rn : rn / 2 + 1;
            if (shape == 1)
                fill_random(numb, nb, (mp_limb_t)(options.seed + 4 * rn + 1));
            mp_srcptr bp = shape == 0 ? numa : numb;
            if (scan_coarse(rn, shape, numa, bp, na, nb, cands, cand_count, coarse_scores, coarse_counts, shape == 0,
                            &work, &options, saved_threshold, &coarse_stable, &coarse_unstable) != 0)
                result = EXIT_FAILURE;
        }
    }

    mp_size_t window_lo = CAND_MIN, window_hi = CAND_MAX;
    if (result == EXIT_SUCCESS) {
        mp_size_t center = cands[best_index(coarse_scores, coarse_counts, cand_count)];
        for (int ci = 0; ci < cand_count; ++ci)
            if (coarse_counts[ci])
                printf("# coarse badness candidate=%llu avg_us=%.6f\n", (unsigned long long)cands[ci],
                       coarse_scores[ci] / coarse_counts[ci]);
        window_lo = center > CAND_MIN + FINE_HALF_WIDTH ? center - FINE_HALF_WIDTH : CAND_MIN;
        window_hi = center + FINE_HALF_WIDTH < CAND_MAX ? center + FINE_HALF_WIDTH : CAND_MAX;
        printf("# coarse_recommended=%llu (64-lattice, locates the fine window only)\n", (unsigned long long)center);
    }

    /* ---- 细扫：窗口内逐整数候选，按部署路径分类测量；最优落在窗口边缘时整体外移重扫 ---- */
    for (int shifts = 0; result == EXIT_SUCCESS; ++shifts) {
        memset(fine_scores, 0, sizeof(fine_scores));
        memset(fine_counts, 0, sizeof(fine_counts));
        fine_stable = 0;
        fine_unstable = 0;
        printf("# phase=fine window=%llu..%llu step=1 (shift %d of %d)\n", (unsigned long long)window_lo,
               (unsigned long long)window_hi, shifts, FINE_MAX_SHIFTS);
        printf("# fine n level rn low_us low_mad high_us high_mad retries stable [deep|upper _us _mad _retries]\n");
        for (mp_size_t n = window_lo; n <= window_hi && result == EXIT_SUCCESS; ++n) {
            if (!fermat_usable(n))
                continue;
            for (int level = 1; level <= FINE_LEVELS && result == EXIT_SUCCESS; ++level) {
                mp_size_t rn = ((mp_size_t)1 << level) * n;
                if (rn > FINE_MAX_RN)
                    break;
                fill_random(numa, rn, (mp_limb_t)(options.seed + 4 * rn + 2));
                if (scan_fine(rn, n, level, numa, &work, &options, saved_threshold, window_lo, window_hi, fine_scores,
                              fine_counts, &fine_stable, &fine_unstable) != 0)
                    result = EXIT_FAILURE;
            }
        }
        if (result != EXIT_SUCCESS || fine_stable == 0 || shifts >= FINE_MAX_SHIFTS)
            break;
        mp_size_t best = window_lo;
        while (best < window_hi && fine_counts[best] == 0)
            ++best;
        for (mp_size_t c = best + 1; c <= window_hi; ++c)
            if (fine_counts[c] && fine_scores[c] / fine_counts[c] < fine_scores[best] / fine_counts[best])
                best = c;
        if (best <= window_lo + FINE_EDGE_MARGIN && window_lo > CAND_MIN) {
            /* 下缘钳制：窗口向下外移一格半宽 */
            window_hi = window_lo;
            window_lo = window_lo > CAND_MIN + FINE_HALF_WIDTH ? window_lo - FINE_HALF_WIDTH : CAND_MIN;
            printf("# fine best=%llu sits near the lower window edge; shifting the window down\n",
                   (unsigned long long)best);
        } else if (best + FINE_EDGE_MARGIN >= window_hi && window_hi < CAND_MAX) {
            window_lo = window_hi;
            window_hi = window_hi + FINE_HALF_WIDTH < CAND_MAX ? window_hi + FINE_HALF_WIDTH : CAND_MAX;
            printf("# fine best=%llu sits near the upper window edge; shifting the window up\n",
                   (unsigned long long)best);
        } else {
            break;
        }
    }

    if (result == EXIT_SUCCESS) {
        if (coarse_unstable > 0)
            printf("# note: %u unstable coarse point(s) retained in scoring\n", coarse_unstable);
        if (fine_stable == 0) {
            printf("# no stable fine-scan points; recommendation withheld\n");
        } else {
            if (fine_unstable > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", fine_unstable);
            lmmp_tune_recommend("MUL_MERSENNE_FERMAT_THRESHOLD", fine_scores, fine_counts, (int)window_lo,
                                (int)window_hi, (unsigned long long)saved_threshold);
        }
    }
    lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD = saved_threshold;
    lmmp_free(full);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
