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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/kernels.h"
#include "tune/params.h"

/* CBRT_INVNEWTON_EXP_SCALE（下称 A0）与 CBRT_INVNEWTON_MIN_ROOT（下称 C）是 lmmp_cbrt_ 根分派的
   double 阈值：!dstr 且 na/A0 + ln(C) <= ln(ns) 时走牛顿逆立方根（high），否则走 knorm +
   lmmp_cbrt_divide_（low）；ns = (na+2)/3+nf+1 是根长 limb 数，分界线 ns = C*exp(na/A0)。
   分派只读一次、两条分支内部都不再读这两个阈值 ⟹ 部署路径恒等于两条分支之一，强制绑定干净。

   测量：网格 na 列 x ns 档位行，每点两条分支各经完整入口 lmmp_cbrt_ 强制测一遍（恒走 newton：
   A0 极大且 C 极小；恒走 divide：C 极大），逐点配对。两路的 FFT 取整相位随 ns 不同，单点比值
   带确定的锯齿；每点的 ns 在档位上叠加种子派生的抖动（幅度不超过档位的 1/16），不同种子的
   轮次落到不同相位，跨轮合并即得平滑趋势。强制是否生效由 tune 库的分支计数
   lmmp_tune_cbrt_newton_hits 核对：强制 newton 的一次调用计数须恰增 1、强制 divide 须不增，
   计时期间累计计数须等于 high 路的调用次数（low 路每次调用都须走 divide）。
   oracle：low 过 floor 恒等式 s^3 <= numa*B^(3nf) < (s+1)^3，high 属于 {s, s+1}。
   不满足 lmmp_cbrt_newton_ 契约（nf >= 2 且 3*nf >= 2*na2+3，na2 = 3*ceil(na/3) 为归一化长度）
   的点不能强制 newton，不入网格。
   MAD 超限且重试耗尽的不稳定点读数不可靠，逐行打印但不入评分与交叉点。

   评分（部署选择式）：候选 (A0,C) 在每个网格点按分派式选路，累加所选路径耗时相对较快路径的
   相对超出后取平均，二维 argmin 为推荐值，另报平坦带。对照口径：逐列比值（newton/divide）
   保序回归后插值的实测交叉点，及其对 ln(ns) = ln(C) + na/A0 的最小二乘拟合。
   候选须对一切 na >= 1 给出分界根长 >= na+4（lmmp_cbrt_newton_ 契约的充分条件），否则拒绝。
   CBRT_NEWTON_MODM_THRESHOLD 与梅森乘法内部的各阈值保持默认。 */
enum {
    NA_COUNT = 24,
    NS_LEVELS = 24,      /* ns 档位表取 1500*2^(k/4) 的整数值 */
    NS_TOP_LEVEL = NS_LEVELS - 1,
    CAND_MAX = 32,       /* 每个参数的候选数上限（含并入的现值） */
    MAX_NA = 1024,
    MAX_NS = 85771,      /* 最高档位加最大抖动，缓冲区按此分配 */
    MAX_ROOT = MAX_NS + 2,
    DEFAULT_MAX_NS = 48000,
    JITTER_DIV = 16,     /* ns 抖动幅度不超过档位的 1/JITTER_DIV */
    A0_MIN = 90,         /* 默认候选 A0 = 90..168 步 6 */
    A0_MAX = 168,
    A0_STEP = 6,
    C_MIN = 2770,        /* 默认候选 C = 2770..4170 步 100 */
    C_MAX = 4170,
    C_STEP = 100
};
static const mp_size_t na_values[NA_COUNT] = {1,  2,  3,  4,  6,   8,   11,  16,  24,  32,  48,  64,
                                              96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 448, 512};
static const mp_size_t ns_levels[NS_LEVELS] = {1500,  1784,  2121,  2523,  3000,  3568,  4243,  5045,
                                               6000,  7135,  8485,  10091, 12000, 14270, 16971, 20182,
                                               24000, 28541, 33941, 40363, 48000, 57082, 67882, 80726};

typedef struct {
    double a0[CAND_MAX], c[CAND_MAX];
    unsigned a0_count, c_count;
    mp_size_t max_ns;
} cli_context;

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_size_t na, nf, nR;
    mp_limb_t calls;
} bench_context;

typedef struct {
    mp_ptr dst_low, dst_high, numa;
    mp_ptr sq, cb, t, big, sa, sb;
} buffers;

typedef struct {
    mp_size_t na, ns;
    double low_us, high_us, ln_ns;
} point;

/* 强制绑定的极值：A0 极大且 C 极小 ⟹ na/A0 + ln(C) << ln(ns) 恒走 newton；C 极大 ⟹ ns < C 恒走 divide */
static void bind_divide(void* opaque) {
    (void)opaque;
    lmmp_tune_CBRT_INVNEWTON_EXP_SCALE = 1.0;
    lmmp_tune_CBRT_INVNEWTON_MIN_ROOT = 1e300;
}

static void bind_newton(void* opaque) {
    (void)opaque;
    lmmp_tune_CBRT_INVNEWTON_EXP_SCALE = 1e300;
    lmmp_tune_CBRT_INVNEWTON_MIN_ROOT = 1e-300;
}

static void reset_output(void* opaque) {
    bench_context* context = opaque;
    lmmp_zero(context->dst, context->nR);
}

static mp_limb_t call_entry(void* opaque) {
    bench_context* context = opaque;
    ++context->calls;
    lmmp_cbrt_(context->dst, NULL, context->numa, context->na, context->nf);
    return context->dst[0] ^ context->dst[context->nR - 1];
}

/* floor 恒等式：s^3 <= numa*B^(3nf) < (s+1)^3（s 取归一化长度） */
static int check_floor(const buffers* b, mp_srcptr s, mp_size_t nR, mp_srcptr numa, mp_size_t na, mp_size_t nf) {
    mp_size_t rn = nR;
    while (rn > 0 && s[rn - 1] == 0) --rn;
    if (rn == 0)
        return 0;
    mp_size_t nl = na + 3 * nf;
    mp_size_t l = 3 * rn + 2 > nl ? 3 * rn + 2 : nl;
    mp_limb_t cy;
    lmmp_sqr_(b->sq, s, rn);
    lmmp_mul_(b->cb, b->sq, 2 * rn, s, rn);
    lmmp_zero(b->t, 3 * rn + 3);
    lmmp_copy(b->t, b->cb, 3 * rn);
    cy = lmmp_addmul_1_(b->t, b->sq, 2 * rn, 3);
    cy = lmmp_add_1_(b->t + 2 * rn, b->t + 2 * rn, rn + 1, cy);
    cy = lmmp_addmul_1_(b->t, s, rn, 3);
    lmmp_add_1_(b->t + rn, b->t + rn, 2 * rn + 2, cy);
    lmmp_add_1_(b->t, b->t, 3 * rn + 2, 1);
    lmmp_zero(b->big, l);
    lmmp_copy(b->big + 3 * nf, numa, na);
    lmmp_zero(b->sa, l);
    lmmp_zero(b->sb, l);
    lmmp_copy(b->sa, b->cb, 3 * rn);
    lmmp_copy(b->sb, b->big, l);
    if (lmmp_cmp_(b->sa, b->sb, l) > 0)
        return 0;
    lmmp_zero(b->sa, l);
    lmmp_zero(b->sb, l);
    lmmp_copy(b->sa, b->big, l);
    lmmp_copy(b->sb, b->t, 3 * rn + 2);
    return lmmp_cmp_(b->sa, b->sb, l) < 0;
}

/* high 属于 {low, low+1} */
static int check_rounded(const buffers* b, mp_srcptr high, mp_srcptr low, mp_size_t nR) {
    if (lmmp_cmp_(high, low, nR) == 0)
        return 1;
    lmmp_copy(b->t, low, nR);
    lmmp_add_1_(b->t, b->t, nR, 1);
    return lmmp_cmp_(high, b->t, nR) == 0;
}

/* 候选须对一切 na >= 1 给出分界根长 C*exp(na/A0) >= na+4（lmmp_cbrt_newton_ 对归一化长度
   3*ceil(na/3) 的契约 3*nf >= 2*na2+3 的充分条件）：
   phi(na) = ln(C) + na/A0 - ln(na+4) 是 na 的凸函数，A0 >= 5 时最小点在 na = A0-4，否则在 na = 1 */
static bool contract_safe(double a0, double c) {
    const double phi_min = a0 >= 5.0 ? log(c) + 1.0 - 4.0 / a0 - log(a0) : log(c) + 1.0 / a0 - log(5.0);
    return phi_min >= 0.0;
}

static bool parse_option(void* opaque, const char* option, const char* value) {
    cli_context* cli = opaque;
    if (strcmp(option, "--exp-scale") == 0 || strcmp(option, "--min-root") == 0) {
        const bool is_scale = option[2] == 'e';
        double* list = is_scale ? cli->a0 : cli->c;
        unsigned* count = is_scale ? &cli->a0_count : &cli->c_count;
        if (!lmmp_tune_parse_doubles(value, list, CAND_MAX - 1, count)) {
            fprintf(stderr, "%s requires 1..%d comma separated positive numbers\n", option, CAND_MAX - 1);
            return false;
        }
        return true;
    }
    if (strcmp(option, "--max-ns") == 0) {
        char* end;
        cli->max_ns = strtoull(value, &end, 10);
        if (end == value || *end != '\0' || cli->max_ns == 0 || cli->max_ns > MAX_NS) {
            fprintf(stderr, "--max-ns requires an integer in [1,%d]\n", MAX_NS);
            return false;
        }
        return true;
    }
    fprintf(stderr, "unknown option '%s'\n", option);
    return false;
}

/* 种子、列、档位派生的抖动量（SplitMix 末级混合） */
static mp_limb_t point_jitter(mp_limb_t seed, int col, int level) {
    mp_limb_t x = seed ^ (0x9e3779b97f4a7c15ull * (mp_limb_t)(col + 1)) ^
                  (0xd1b54a32d192ed03ull * (mp_limb_t)(level + 1));
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebull;
    return x ^ (x >> 31);
}

static void print_grid(const char* prefix) {
    printf("%sgrid_na_columns=%d: ", prefix, NA_COUNT);
    for (int i = 0; i < NA_COUNT; ++i)
        printf(i == 0 ? "%llu" : ",%llu", (unsigned long long)na_values[i]);
    putchar('\n');
    printf("%sgrid_ns_levels=%d (integer values of 1500*2^(k/4), ratio 2^(1/4)=%.6f): ", prefix, NS_LEVELS,
           pow(2.0, 0.25));
    for (int k = 0; k < NS_LEVELS; ++k)
        printf(k == 0 ? "%llu" : ",%llu", (unsigned long long)ns_levels[k]);
    putchar('\n');
    printf("%sgrid_ns_jitter: ns of a grid point = level + jitter(seed, column, level), |jitter| <= level/%d\n",
           prefix, JITTER_DIV);
    printf("%sgrid_rows: column a takes the levels from one step below min_C*exp(a/max_A0) to one step above "
           "max_C*exp(a/min_A0) over the candidates, capped at max_ns\n",
           prefix);
}

static void print_info(const cli_context* cli) {
    printf("program=tune_numth_cbrt_dispatch\n");
    printf("parameters=CBRT_INVNEWTON_EXP_SCALE,CBRT_INVNEWTON_MIN_ROOT (double; newton iff !dstr && na/A0 + "
           "ln(C) <= ln(ns))\n");
    printf("consumer=lmmp_cbrt_ root dispatch (dstr==NULL): lmmp_cbrt_newton_ vs knorm + lmmp_cbrt_divide_\n");
    printf("binding_mode=runtime-branch\n");
    printf("default_exp_scale=%.10g default_min_root=%.10g\n", CBRT_INVNEWTON_EXP_SCALE, CBRT_INVNEWTON_MIN_ROOT);
    printf("current_exp_scale=%.10g current_min_root=%.10g\n", lmmp_tune_CBRT_INVNEWTON_EXP_SCALE,
           lmmp_tune_CBRT_INVNEWTON_MIN_ROOT);
    lmmp_tune_print_double_candidates("CBRT_INVNEWTON_EXP_SCALE", cli->a0, cli->a0_count);
    lmmp_tune_print_double_candidates("CBRT_INVNEWTON_MIN_ROOT", cli->c, cli->c_count);
    print_grid("");
    printf("max_ns=%llu\n", (unsigned long long)cli->max_ns);
    printf("arms=both forced through the full lmmp_cbrt_ entry (always newton: A0 huge and C tiny; always divide: C "
           "huge), path verified via lmmp_tune_cbrt_newton_hits\n");
    printf("scoring=mean over grid points of (time of the arm selected by na/A0 + ln(C) <= ln(ns)) / (faster arm) - "
           "1; 2D argmin; flat band = within +0.01 of the best\n");
    lmmp_tune_print_env();
}

/* 逐列保序回归（比值随 ns 不增）后插值的实测交叉点：返回 1 表示网格内存在交叉，*cross 为插值的 ns；
   0 表示 newton 在全部行更快（交叉在网格下方）；-1 表示网格内无交叉（含该列无行） */
static int column_crossing(const mp_size_t* ns, const double* ratio, int count, double* cross) {
    double mean[NS_LEVELS], fit[NS_LEVELS];
    int length[NS_LEVELS], top = 0;
    *cross = 0.0;
    if (count == 0)
        return -1;
    for (int i = 0; i < count; ++i) {
        mean[top] = ratio[i];
        length[top] = 1;
        ++top;
        /* 前一块均值小于当前块即违反不增，合并 */
        while (top > 1 && mean[top - 2] < mean[top - 1]) {
            const int size = length[top - 2] + length[top - 1];
            mean[top - 2] = (mean[top - 2] * length[top - 2] + mean[top - 1] * length[top - 1]) / size;
            length[top - 2] = size;
            --top;
        }
    }
    for (int i = 0, block = 0; i < count; ++i) {
        fit[i] = mean[block];
        if (--length[block] == 0)
            ++block;
    }
    if (fit[0] < 1.0)
        return 0;
    for (int i = 0; i + 1 < count; ++i) {
        if (fit[i] >= 1.0 && fit[i + 1] < 1.0) {
            const double t = (fit[i] - 1.0) / (fit[i] - fit[i + 1]);
            *cross = exp(log((double)ns[i]) + t * (log((double)ns[i + 1]) - log((double)ns[i])));
            return 1;
        }
    }
    return -1;
}

/* 最小二乘直线 y = intercept + slope*x；斜率须为正 */
static bool lsq_line(const double* x, const double* y, int count, double* slope, double* intercept) {
    double mean_x = 0.0, mean_y = 0.0, sxx = 0.0, sxy = 0.0;
    if (count < 2)
        return false;
    for (int i = 0; i < count; ++i) {
        mean_x += x[i];
        mean_y += y[i];
    }
    mean_x /= count;
    mean_y /= count;
    for (int i = 0; i < count; ++i) {
        sxx += (x[i] - mean_x) * (x[i] - mean_x);
        sxy += (x[i] - mean_x) * (y[i] - mean_y);
    }
    if (sxx == 0.0 || sxy <= 0.0)
        return false;
    *slope = sxy / sxx;
    *intercept = mean_y - *slope * mean_x;
    return true;
}

/* 任意 (A0,C) 在全部网格点上的平均相对超出 */
static double pair_excess_of(const point* points, unsigned point_count, double a0, double c) {
    double sum = 0.0;
    const double ln_c = log(c);
    if (point_count == 0)
        return NAN;
    for (unsigned i = 0; i < point_count; ++i) {
        const double low = points[i].low_us, high = points[i].high_us;
        const double faster = low < high ? low : high;
        sum += ((double)points[i].na / a0 + ln_c <= points[i].ln_ns ? high : low) / faster - 1.0;
    }
    return sum / point_count;
}

/* (A0,C) 相对实测交叉点的 ln(ns) 残差均方根 */
static double pair_cross_rms(const mp_size_t* cross_na, const double* cross_ln_ns, int cross_count, double a0,
                             double c) {
    double sum = 0.0;
    if (cross_count == 0)
        return NAN;
    for (int i = 0; i < cross_count; ++i) {
        const double resid = cross_ln_ns[i] - (log(c) + (double)cross_na[i] / a0);
        sum += resid * resid;
    }
    return sqrt(sum / cross_count);
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    cli_context cli = {.max_ns = DEFAULT_MAX_NS};
    if (!lmmp_tune_parse_ex(argc, argv, &options, parse_option, &cli))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_cbrt_dispatch");
        puts("  --exp-scale LIST  CBRT_INVNEWTON_EXP_SCALE candidates, comma separated positive numbers");
        puts("  --min-root LIST   CBRT_INVNEWTON_MIN_ROOT candidates, comma separated positive numbers");
        printf("  --max-ns NS       cap on the actual (jittered) ns of grid rows, default %d, at most %d\n",
               DEFAULT_MAX_NS, MAX_NS);
        return EXIT_SUCCESS;
    }

    /* 候选表：命令行给定或表定等差，升序并入现值 */
    if (cli.a0_count == 0)
        for (int a0 = A0_MIN; a0 <= A0_MAX; a0 += A0_STEP) cli.a0[cli.a0_count++] = a0;
    if (cli.c_count == 0)
        for (int c = C_MIN; c <= C_MAX; c += C_STEP) cli.c[cli.c_count++] = c;
    if (!lmmp_tune_add_double(cli.a0, &cli.a0_count, CAND_MAX, lmmp_tune_CBRT_INVNEWTON_EXP_SCALE) ||
        !lmmp_tune_add_double(cli.c, &cli.c_count, CAND_MAX, lmmp_tune_CBRT_INVNEWTON_MIN_ROOT)) {
        fprintf(stderr, "candidate list full while merging the current value\n");
        return EXIT_FAILURE;
    }
    for (unsigned x = 0; x < cli.a0_count; ++x)
        for (unsigned y = 0; y < cli.c_count; ++y)
            if (!contract_safe(cli.a0[x], cli.c[y])) {
                fprintf(stderr, "candidate (%.10g,%.10g) violates the newton contract: C*exp(na/A0) < na+4\n",
                        cli.a0[x], cli.c[y]);
                return EXIT_FAILURE;
            }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    setvbuf(stdout, NULL, _IOLBF, 4096);
    if (options.action == LMMP_TUNE_INFO) {
        print_info(&cli);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current_double("CBRT_INVNEWTON_EXP_SCALE", CBRT_INVNEWTON_EXP_SCALE,
                                       lmmp_tune_CBRT_INVNEWTON_EXP_SCALE);
        lmmp_tune_print_current_double("CBRT_INVNEWTON_MIN_ROOT", CBRT_INVNEWTON_MIN_ROOT,
                                       lmmp_tune_CBRT_INVNEWTON_MIN_ROOT);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    /* 工作区：numa + 两个结果 + oracle */
    const mp_size_t l_oracle = 3 * MAX_ROOT + MAX_NA + 8;
    mp_ptr storage =
        alloc_limbs(MAX_NA + 2 * (MAX_ROOT + 2) + (2 * MAX_ROOT + 4) + 2 * (3 * MAX_ROOT + 8) + 3 * l_oracle);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    buffers b;
    b.numa = storage;
    b.dst_low = storage + MAX_NA;
    b.dst_high = b.dst_low + MAX_ROOT + 2;
    b.sq = b.dst_high + MAX_ROOT + 2;
    b.cb = b.sq + 2 * MAX_ROOT + 4;
    b.t = b.cb + 3 * MAX_ROOT + 8;
    b.big = b.t + 3 * MAX_ROOT + 8;
    b.sa = b.big + l_oracle;
    b.sb = b.sa + l_oracle;

    static double scores[CAND_MAX * CAND_MAX];
    static unsigned long counts[CAND_MAX * CAND_MAX];
    static point points[NA_COUNT * NS_LEVELS];
    unsigned point_count = 0, stable_points = 0, unstable_points = 0;
    double column_ratio[NA_COUNT][NS_LEVELS] = {{0}};  /* 逐列各档位 newton/divide；0 表示该档未测 */
    mp_size_t column_ns[NA_COUNT][NS_LEVELS] = {{0}};  /* 各档位实际测量的 ns（含抖动） */
    mp_size_t cross_na[NA_COUNT];
    double cross_ln_ns[NA_COUNT];
    int cross_count = 0;
    double ln_c[CAND_MAX];
    for (unsigned y = 0; y < cli.c_count; ++y) ln_c[y] = log(cli.c[y]);
    const double saved_scale = lmmp_tune_CBRT_INVNEWTON_EXP_SCALE;
    const double saved_root = lmmp_tune_CBRT_INVNEWTON_MIN_ROOT;

    printf("# program=tune_numth_cbrt_dispatch\n");
    printf("# parameters=CBRT_INVNEWTON_EXP_SCALE,CBRT_INVNEWTON_MIN_ROOT (double)\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=(%.10g,%.10g) current=(%.10g,%.10g) seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           CBRT_INVNEWTON_EXP_SCALE, CBRT_INVNEWTON_MIN_ROOT, saved_scale, saved_root, options.seed, options.samples,
           options.min_ms, options.mad_limit);
    printf("# low=divide arm high=newton arm, both forced through the full lmmp_cbrt_ entry\n");
    lmmp_tune_print_double_candidates("CBRT_INVNEWTON_EXP_SCALE", cli.a0, cli.a0_count);
    lmmp_tune_print_double_candidates("CBRT_INVNEWTON_MIN_ROOT", cli.c, cli.c_count);
    print_grid("# ");
    printf("# max_ns=%llu\n", (unsigned long long)cli.max_ns);
    lmmp_tune_print_env_comment();
    printf("# na nf ns divide_us divide_mad newton_us newton_mad ratio retries stable\n");

    const double start = bench_clock();
    int result = EXIT_SUCCESS;
    for (int col = 0; col < NA_COUNT && result == EXIT_SUCCESS; ++col) {
        const mp_size_t na = na_values[col];
        /* 候选分界范围 [min_C*exp(na/max_A0), max_C*exp(na/min_A0)] 上下各放宽一档 */
        int low_level = 0, high_level = NS_TOP_LEVEL;
        while (low_level < NS_LEVELS &&
               (double)ns_levels[low_level] < cli.c[0] * exp((double)na / cli.a0[cli.a0_count - 1]))
            ++low_level;
        while (high_level >= 0 && (double)ns_levels[high_level] > cli.c[cli.c_count - 1] * exp((double)na / cli.a0[0]))
            --high_level;
        const int from = low_level > 0 ? low_level - 1 : 0;
        const int to = high_level < NS_TOP_LEVEL ? high_level + 1 : NS_TOP_LEVEL;
        if (low_level >= NS_LEVELS) {
            printf("# column na=%llu skipped: candidate boundaries above the ns ladder\n", (unsigned long long)na);
            continue;
        }
        for (int level = from; level <= to && result == EXIT_SUCCESS; ++level) {
            const mp_size_t level_ns = ns_levels[level];
            const mp_size_t span = level_ns / JITTER_DIV;
            const mp_size_t ns = level_ns + point_jitter(options.seed, col, level) % (2 * span + 1) - span;
            const mp_size_t nf = ns - 1 - (na + 2) / 3, nl = na + 3 * nf, nR = (nl + 2) / 3 + 1;
            if (ns > cli.max_ns || nf < 2 || 3 * nf < 2 * ((na + 2) / 3 * 3) + 3)
                continue;  /* 实际 ns（含抖动）超出封顶，或 newton 契约之外：不能强制 newton */
            fill_random(b.numa, na, (mp_limb_t)(options.seed + 8 * nf + na));
            if ((col + level) % 3 == 1)
                b.numa[na - 1] >>= 2;
            else if ((col + level) % 3 == 2)
                b.numa[na - 1] >>= 32;
            if (b.numa[na - 1] == 0)
                b.numa[na - 1] = 1;

            /* 强制路径核对 + oracle：每路各经入口调一次，分支计数须与强制意图一致 */
            lmmp_tune_cbrt_newton_hits = 0;
            bind_divide(NULL);
            lmmp_zero(b.dst_low, nR);
            lmmp_cbrt_(b.dst_low, NULL, b.numa, na, nf);
            const mp_limb_t divide_hits = lmmp_tune_cbrt_newton_hits;
            lmmp_tune_cbrt_newton_hits = 0;
            bind_newton(NULL);
            lmmp_zero(b.dst_high, nR);
            lmmp_cbrt_(b.dst_high, NULL, b.numa, na, nf);
            const mp_limb_t newton_hits = lmmp_tune_cbrt_newton_hits;
            const bool forced_ok = divide_hits == 0 && newton_hits == 1;
            const bool oracle_ok = forced_ok && check_floor(&b, b.dst_low, nR, b.numa, na, nf) &&
                                   check_rounded(&b, b.dst_high, b.dst_low, nR);
            lmmp_tune_CBRT_INVNEWTON_EXP_SCALE = saved_scale;
            lmmp_tune_CBRT_INVNEWTON_MIN_ROOT = saved_root;
            if (!oracle_ok) {
                fprintf(stderr, "cbrt dispatch mismatch at na=%llu nf=%llu: divide/newton hits %llu/%llu\n",
                        (unsigned long long)na, (unsigned long long)nf, (unsigned long long)divide_hits,
                        (unsigned long long)newton_hits);
                result = EXIT_FAILURE;
                break;
            }

            bench_context low_ctx = {
                .dst = b.dst_low, .numa = b.numa, .na = na, .nf = nf, .nR = nR, .calls = 0};
            bench_context high_ctx = {
                .dst = b.dst_high, .numa = b.numa, .na = na, .nf = nf, .nR = nR, .calls = 0};
            lmmp_tune_bench low_bench = {
                .bind = bind_divide, .reset = reset_output, .call = call_entry, .context = &low_ctx};
            lmmp_tune_bench high_bench = {
                .bind = bind_newton, .reset = reset_output, .call = call_entry, .context = &high_ctx};
            double low_us, low_mad, high_us, high_mad;
            unsigned retries;
            bool stable;
            lmmp_tune_cbrt_newton_hits = 0;
            if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                        &retries, &stable)) {
                fprintf(stderr, "measurement failed at na=%llu nf=%llu\n", (unsigned long long)na,
                        (unsigned long long)nf);
                result = EXIT_FAILURE;
                break;
            }
            /* 计时期间：high 路每次调用计数增 1，low 路每次调用不增 */
            const bool path_ok = lmmp_tune_cbrt_newton_hits == high_ctx.calls;
            lmmp_tune_CBRT_INVNEWTON_EXP_SCALE = saved_scale;
            lmmp_tune_CBRT_INVNEWTON_MIN_ROOT = saved_root;
            if (!path_ok) {
                fprintf(stderr, "timed path mismatch at na=%llu nf=%llu: newton hits %llu, high calls %llu\n",
                        (unsigned long long)na, (unsigned long long)nf,
                        (unsigned long long)lmmp_tune_cbrt_newton_hits, (unsigned long long)high_ctx.calls);
                result = EXIT_FAILURE;
                break;
            }

            printf("%llu %llu %llu %.6f %.6f %.6f %.6f %.6f %u %u\n", (unsigned long long)na, (unsigned long long)nf,
                   (unsigned long long)ns, low_us, low_mad, high_us, high_mad, high_us / low_us, retries, stable);
            if (!stable) {
                /* 不稳定点的读数不可靠（瞬态噪声下可偏离三成以上），不入交叉点与评分 */
                ++unstable_points;
                continue;
            }
            ++stable_points;
            column_ratio[col][level] = high_us / low_us;
            column_ns[col][level] = ns;

            /* 部署评分：候选按分派式选路，累加相对较快路径的相对超出 */
            const double faster = low_us < high_us ? low_us : high_us;
            const double excess_low = low_us / faster - 1.0, excess_high = high_us / faster - 1.0;
            points[point_count] = (point){na, ns, low_us, high_us, log((double)ns)};
            for (unsigned x = 0; x < cli.a0_count; ++x) {
                for (unsigned y = 0; y < cli.c_count; ++y) {
                    const double excess =
                        (double)na / cli.a0[x] + ln_c[y] <= points[point_count].ln_ns ? excess_high : excess_low;
                    scores[x * cli.c_count + y] += excess;
                    ++counts[x * cli.c_count + y];
                }
            }
            ++point_count;
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) excluded from scoring\n", unstable_points);
            /* 逐列实测交叉点：比值随 ns 保序回归后在 ln(ns) 上线性插值过 1 的位置 */
            for (int col = 0; col < NA_COUNT; ++col) {
                mp_size_t ns_list[NS_LEVELS];
                double ratio_list[NS_LEVELS];
                int count = 0;
                for (int level = 0; level < NS_LEVELS; ++level) {
                    if (column_ratio[col][level] == 0.0)
                        continue;
                    ns_list[count] = column_ns[col][level];
                    ratio_list[count] = column_ratio[col][level];
                    ++count;
                }
                double cross = 0.0;
                const int kind = column_crossing(ns_list, ratio_list, count, &cross);
                if (kind == 1) {
                    printf("# crossover na=%llu ns_cross=%.0f rows=%d ns=%llu..%llu\n",
                           (unsigned long long)na_values[col], cross, count, (unsigned long long)ns_list[0],
                           (unsigned long long)ns_list[count - 1]);
                    cross_na[cross_count] = na_values[col];
                    cross_ln_ns[cross_count] = log(cross);
                    ++cross_count;
                } else if (kind == 0) {
                    printf("# crossover na=%llu below-grid: newton faster on all %d rows, bottom_ratio=%.4f\n",
                           (unsigned long long)na_values[col], count, ratio_list[0]);
                } else if (count > 0) {
                    printf("# crossover na=%llu none-within-grid: newton not faster on all %d rows, top_ratio=%.4f\n",
                           (unsigned long long)na_values[col], count, ratio_list[count - 1]);
                } else {
                    printf("# crossover na=%llu no rows\n", (unsigned long long)na_values[col]);
                }
            }
            /* 最小二乘拟合 ln(ns_cross) = ln(C) + na/A0 */
            double x[NA_COUNT], slope = 0.0, intercept = 0.0;
            for (int i = 0; i < cross_count; ++i)
                x[i] = (double)cross_na[i];
            const bool fitted = lsq_line(x, cross_ln_ns, cross_count, &slope, &intercept);
            const double lsq_a0 = fitted ? 1.0 / slope : NAN, lsq_c = fitted ? exp(intercept) : NAN;
            if (fitted)
                printf("# lsq_fit ln(ns_cross) = ln(C) + na/A0 over %d columns: CBRT_INVNEWTON_EXP_SCALE=%.10g "
                       "CBRT_INVNEWTON_MIN_ROOT=%.10g ln_ns_rms=%.4f\n",
                       cross_count, lsq_a0, lsq_c, pair_cross_rms(cross_na, cross_ln_ns, cross_count, lsq_a0, lsq_c));
            else
                printf("# lsq_fit withheld: only %d column(s) with a crossing inside the grid\n", cross_count);

            /* 对照：现值、拟合、部署评分 argmin */
            const lmmp_tune_pair_pick best = lmmp_tune_pair_argmin(scores, counts, cli.a0_count, cli.c_count);
            printf("# compare (mean_excess over %u grid points, ln_ns_rms over %d measured crossings):\n",
                   point_count, cross_count);
            printf("# compare current   a0=%.10g c=%.10g mean_excess=%.6f ln_ns_rms=%.4f\n", saved_scale, saved_root,
                   pair_excess_of(points, point_count, saved_scale, saved_root),
                   pair_cross_rms(cross_na, cross_ln_ns, cross_count, saved_scale, saved_root));
            if (fitted)
                printf("# compare lsq_fit   a0=%.10g c=%.10g mean_excess=%.6f ln_ns_rms=%.4f\n", lsq_a0, lsq_c,
                       pair_excess_of(points, point_count, lsq_a0, lsq_c),
                       pair_cross_rms(cross_na, cross_ln_ns, cross_count, lsq_a0, lsq_c));
            if (!isnan(best.excess))
                printf("# compare argmin    a0=%.10g c=%.10g mean_excess=%.6f ln_ns_rms=%.4f\n", cli.a0[best.x],
                       cli.c[best.y], best.excess,
                       pair_cross_rms(cross_na, cross_ln_ns, cross_count, cli.a0[best.x], cli.c[best.y]));
            /* 推荐值落在候选表端点，是候选范围可能不足的特征症状 */
            if (!isnan(best.excess) && (best.x == 0 || best.x + 1 == cli.a0_count))
                printf("# note: argmin at the candidate edge of CBRT_INVNEWTON_EXP_SCALE (%.10g)\n", cli.a0[best.x]);
            if (!isnan(best.excess) && (best.y == 0 || best.y + 1 == cli.c_count))
                printf("# note: argmin at the candidate edge of CBRT_INVNEWTON_MIN_ROOT (%.10g)\n", cli.c[best.y]);
            printf("# points=%u stable=%u unstable=%u elapsed_s=%.1f\n", point_count, stable_points, unstable_points,
                   bench_clock() - start);
            lmmp_tune_recommend_pair("CBRT_INVNEWTON_EXP_SCALE", "CBRT_INVNEWTON_MIN_ROOT", scores, counts, cli.a0,
                                     cli.a0_count, cli.c, cli.c_count, saved_scale, saved_root);
        }
    }
    lmmp_tune_CBRT_INVNEWTON_EXP_SCALE = saved_scale;
    lmmp_tune_CBRT_INVNEWTON_MIN_ROOT = saved_root;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
