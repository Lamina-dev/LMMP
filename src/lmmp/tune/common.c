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

#include "tune/common.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "support/perf.h"

static volatile mp_limb_t lmmp_tune_sink;

static bool parse_unsigned_(const char* text, unsigned long long* value) {
    char* end;
    unsigned long long parsed;
    while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\v' || *text == '\f' || *text == '\r')
        ++text;
    if (*text == '-')  /* strtoull 会把负数回绕成大无符号值，须显式拒绝 */
        return false;
    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0')
        return false;
    *value = parsed;
    return true;
}

static bool parse_positive_double_(const char* text, double* value) {
    char* end;
    double parsed;
    errno = 0;
    parsed = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !isfinite(parsed) || parsed <= 0)
        return false;
    *value = parsed;
    return true;
}

static int compare_double_(const void* lhs, const void* rhs) {
    double a = *(const double*)lhs;
    double b = *(const double*)rhs;
    return (a > b) - (a < b);
}

static double median_(double* values, unsigned count) {
    qsort(values, count, sizeof(*values), compare_double_);
    return (values[(count - 1) / 2] + values[count / 2]) * 0.5;
}

static double mad_ratio_(const double* values, unsigned count, double median) {
    double deviations[31];
    double scale = fabs(median);
    if (count > sizeof(deviations) / sizeof(deviations[0]))
        return 1.0;
    for (unsigned i = 0; i < count; ++i) deviations[i] = fabs(values[i] - median);
    double mad = median_(deviations, count);
    if (scale < 1e-12)
        scale = 1e-12;
    return mad / scale;
}

void lmmp_tune_options_init(lmmp_tune_options* options) {
    *options = (lmmp_tune_options){
        .action = LMMP_TUNE_RUN,
        .samples = 5,
        .min_ms = 5.0,
        .mad_limit = 0.05,
        .max_retry = 2,
        .seed = 0x2733ba7a11665d4fULL,
    };
}

static bool need_value_(int argc, char** argv, int* index, const char* option, const char** value) {
    if (*index + 1 >= argc) {
        fprintf(stderr, "%s requires a value\n", option);
        return false;
    }
    *value = argv[*index + 1];
    ++*index;
    return true;
}

bool lmmp_tune_parse_ex(int argc,
                        char** argv,
                        lmmp_tune_options* options,
                        lmmp_tune_option_fn option_fn,
                        void* context) {
    for (int i = 1; i < argc; ++i) {
        const char* value;
        unsigned long long parsed;
        if (strcmp(argv[i], "--help") == 0) {
            options->action = LMMP_TUNE_HELP;
        } else if (strcmp(argv[i], "--info") == 0) {
            options->action = LMMP_TUNE_INFO;
        } else if (strcmp(argv[i], "--current") == 0) {
            options->action = LMMP_TUNE_CURRENT;
        } else if (strcmp(argv[i], "--run") == 0) {
            options->action = LMMP_TUNE_RUN;
        } else if (strcmp(argv[i], "--samples") == 0) {
            if (!need_value_(argc, argv, &i, argv[i], &value) || !parse_unsigned_(value, &parsed) || parsed == 0 ||
                parsed > 31 || (parsed & 1) == 0) {
                fprintf(stderr, "--samples requires an odd integer in [1,31]\n");
                return false;
            }
            options->samples = (unsigned)parsed;
        } else if (strcmp(argv[i], "--min-ms") == 0) {
            if (!need_value_(argc, argv, &i, argv[i], &value) || !parse_positive_double_(value, &options->min_ms)) {
                fprintf(stderr, "--min-ms requires a positive number\n");
                return false;
            }
        } else if (strcmp(argv[i], "--mad-limit") == 0) {
            if (!need_value_(argc, argv, &i, argv[i], &value) || !parse_positive_double_(value, &options->mad_limit)) {
                fprintf(stderr, "--mad-limit requires a positive number\n");
                return false;
            }
        } else if (strcmp(argv[i], "--max-retry") == 0) {
            if (!need_value_(argc, argv, &i, argv[i], &value) || !parse_unsigned_(value, &parsed) || parsed > 100) {
                fprintf(stderr, "--max-retry requires an integer in [0,100]\n");
                return false;
            }
            options->max_retry = (unsigned)parsed;
        } else if (strcmp(argv[i], "--seed") == 0) {
            if (!need_value_(argc, argv, &i, argv[i], &value) || !parse_unsigned_(value, &parsed)) {
                fprintf(stderr, "--seed requires an unsigned integer\n");
                return false;
            }
            options->seed = parsed;
        } else if (option_fn == NULL) {
            fprintf(stderr, "unknown option '%s'\n", argv[i]);
            return false;
        } else {
            /* 程序私有选项：取值恒为其后紧邻的一个参数 */
            const char* option = argv[i];
            if (!need_value_(argc, argv, &i, option, &value))
                return false;
            if (!option_fn(context, option, value))
                return false;
        }
    }
    return true;
}

bool lmmp_tune_parse(int argc, char** argv, lmmp_tune_options* options) {
    return lmmp_tune_parse_ex(argc, argv, options, NULL, NULL);
}

void lmmp_tune_usage(const char* program_id) {
    printf("Usage: %s [--run|--info|--current|--help] [options]\n", program_id);
    puts("  --samples N       odd sample count, default 5");
    puts("  --min-ms MS       minimum time per sample block, default 5");
    puts("  --mad-limit X     MAD/median retry limit, default 0.05");
    puts("  --max-retry N     maximum unstable-point retries, default 2");
    puts("  --seed S          deterministic input seed");
}

/* 跑 iters 次调用返回总耗时（秒）。只读输入的基准整块计时；破坏输入的基准分段铺副本、
   分段计时，每份副本只被调用一次。每块至多 64 段、每段至多 LMMP_TUNE_BATCH_COPIES 份，
   兼顾计时分辨率与工作区占用。 */
static double run_block_(const lmmp_tune_bench* bench, mp_size_t iters) {
    bench->bind(bench->context);
    if (bench->reset_batch == NULL) {
        bench->reset(bench->context);
        double start = bench_clock();
        for (mp_size_t i = 0; i < iters; ++i) lmmp_tune_sink ^= bench->call(bench->context);
        return bench_clock() - start;
    }
    mp_size_t chunk = (iters + 63) / 64;
    if (chunk > LMMP_TUNE_BATCH_COPIES)
        chunk = LMMP_TUNE_BATCH_COPIES;
    double elapsed = 0;
    for (mp_size_t done = 0; done < iters; done += chunk) {
        mp_size_t count = iters - done < chunk ? iters - done : chunk;
        bench->reset_batch(bench->context, count);
        double start = bench_clock();
        for (mp_size_t i = 0; i < count; ++i) lmmp_tune_sink ^= bench->call(bench->context);
        elapsed += bench_clock() - start;
    }
    return elapsed;
}

static mp_size_t calibrate_(const lmmp_tune_bench* bench, const lmmp_tune_options* options) {
    const mp_size_t max_iters = 1ull << 26;
    mp_size_t iters = 1;
    for (;;) {
        if (run_block_(bench, iters) >= options->min_ms * 1e-3 || iters >= max_iters)
            break;
        iters *= 2;
    }
    return iters;
}

static double timed_block_(const lmmp_tune_bench* bench, mp_size_t iters) {
    return run_block_(bench, iters) * 1e6 / (double)iters;
}

static bool summarize_(double* samples, unsigned count, double* median_us, double* mad_ratio) {
    *median_us = median_(samples, count);
    *mad_ratio = mad_ratio_(samples, count, *median_us);
    return isfinite(*median_us) && isfinite(*mad_ratio);
}

static bool measure_once_(const lmmp_tune_bench* bench,
                          const lmmp_tune_options* options,
                          double* median_us,
                          double* mad_ratio) {
    mp_size_t iters = calibrate_(bench, options);
    double samples[31];
    for (unsigned sample = 0; sample < options->samples; ++sample)
        samples[sample] = timed_block_(bench, iters);
    return summarize_(samples, options->samples, median_us, mad_ratio);
}

bool lmmp_tune_measure(const lmmp_tune_bench* bench,
                       const lmmp_tune_options* options,
                       double* median_us,
                       double* mad_ratio,
                       unsigned* retries) {
    for (unsigned retry = 0; retry <= options->max_retry; ++retry) {
        if (!measure_once_(bench, options, median_us, mad_ratio))
            return false;
        if (*mad_ratio <= options->mad_limit || retry == options->max_retry) {
            *retries = retry;
            return true;
        }
    }
    return false;
}

static bool measure_pair_once_(const lmmp_tune_bench* low,
                               const lmmp_tune_bench* high,
                               const lmmp_tune_options* options,
                               double* low_us,
                               double* low_mad,
                               double* high_us,
                               double* high_mad,
                               lmmp_tune_pair_trace* trace) {
    mp_size_t low_iters = calibrate_(low, options);
    mp_size_t high_iters = calibrate_(high, options);
    double low_samples[31], high_samples[31];
    for (unsigned sample = 0; sample < options->samples; ++sample) {
        /* 逐样本交替先后顺序，抵消固定先测一路带来的漂移与热状态不对称 */
        bool low_first = (sample & 1) == 0;
        double first = low_first ? timed_block_(low, low_iters) : timed_block_(high, high_iters);
        double second = low_first ? timed_block_(high, high_iters) : timed_block_(low, low_iters);
        low_samples[sample] = low_first ? first : second;
        high_samples[sample] = low_first ? second : first;
        if (trace) {
            trace->low_first[sample] = low_first;
            trace->low_us[sample] = low_samples[sample];
            trace->high_us[sample] = high_samples[sample];
        }
    }
    return summarize_(low_samples, options->samples, low_us, low_mad) &&
           summarize_(high_samples, options->samples, high_us, high_mad);
}

bool lmmp_tune_measure_pair(const lmmp_tune_bench* low,
                            const lmmp_tune_bench* high,
                            const lmmp_tune_options* options,
                            double* low_us,
                            double* low_mad,
                            double* high_us,
                            double* high_mad,
                            unsigned* retries,
                            bool* stable) {
    return lmmp_tune_measure_pair_traced(low, high, options, low_us, low_mad, high_us, high_mad, retries, stable, NULL);
}

bool lmmp_tune_measure_pair_traced(const lmmp_tune_bench* low,
                                   const lmmp_tune_bench* high,
                                   const lmmp_tune_options* options,
                                   double* low_us,
                                   double* low_mad,
                                   double* high_us,
                                   double* high_mad,
                                   unsigned* retries,
                                   bool* stable,
                                   lmmp_tune_pair_trace* trace) {
    for (unsigned retry = 0; retry <= options->max_retry; ++retry) {
        if (!measure_pair_once_(low, high, options, low_us, low_mad, high_us, high_mad, trace))
            return false;
        *stable = *low_mad <= options->mad_limit && *high_mad <= options->mad_limit;
        if (*stable || retry == options->max_retry) {
            /* 重试耗尽仍超限时返回测量数据供诊断，stable=0 标记结果不可靠 */
            *retries = retry;
            return true;
        }
    }
    return false;
}

void lmmp_tune_abort(lmmp_error_t type, const char* message, const char* function, int line) {
    fprintf(stderr, "LMMP abort %d in %s:%d: %s\n", (int)type, function, line, message);
    _Exit(EXIT_FAILURE);
}

static const char* asm_backend_name_(void) {
#if defined(LMMP_ASM_X64)
    return "x64";
#elif defined(LMMP_ASM_ARM64)
    return "arm64";
#else
    return "generic";
#endif
}

static const char* target_system_name_(void) {
#if defined(LMMP_WINDOWS)
    return "windows";
#elif defined(LMMP_LINUX)
    return "linux";
#elif defined(LMMP_MACOS)
    return "macos";
#else
    return "unknown";
#endif
}

void lmmp_tune_print_env(void) {
    printf("asm_backend=%s\n", asm_backend_name_());
    printf("target_system=%s\n", target_system_name_());
    printf("build_type=%s\n", LMMP_BUILD_TYPE_STR);
}

void lmmp_tune_print_env_comment(void) {
    printf("# build_type=%s compiler=%s\n", LMMP_BUILD_TYPE_STR, __VERSION__);
    printf("# target_system=%s\n", target_system_name_());
    printf("# asm_backend=%s\n", asm_backend_name_());
}

void lmmp_tune_print_current(const char* name, unsigned long long default_value, unsigned long long current_value) {
    printf("%s default=%llu current=%llu\n", name, default_value, current_value);
}

void lmmp_tune_score_point(double* scores,
                           unsigned long* counts,
                           int candidate_min,
                           int candidate_max,
                           mp_size_t n,
                           mp_size_t sub_n,
                           double low_us,
                           double high_us) {
    for (int candidate = candidate_min; candidate <= candidate_max; ++candidate) {
        if ((mp_size_t)candidate <= sub_n)
            /* 该候选会让高路的递归子问题改走分治，与本行高路测量路径不同，
               复用会低估其成本；此类候选须由调用方按部署路径另行实测。 */
            continue;
        scores[candidate] += n >= (mp_size_t)candidate ? high_us : low_us;
        ++counts[candidate];
    }
}

void lmmp_tune_recommend(const char* name,
                         const double* scores,
                         const unsigned long* counts,
                         int candidate_min,
                         int candidate_max,
                         unsigned long long current_value) {
    bool any_point = false;
    for (int candidate = candidate_min; candidate <= candidate_max; ++candidate)
        any_point = any_point || counts[candidate] != 0;
    if (!any_point) {
        printf("# no measurement points; recommendation withheld\n");
        return;
    }
    int best = candidate_min;
    for (int candidate = candidate_min + 1; candidate <= candidate_max; ++candidate) {
        if (scores[candidate] < scores[best])
            best = candidate;
    }
    double best_score = scores[best] / (double)counts[best];
    int flat_low = best;
    int flat_high = best;
    while (flat_low > candidate_min && scores[flat_low - 1] / (double)counts[flat_low - 1] <= best_score * 1.01)
        --flat_low;
    while (flat_high < candidate_max && scores[flat_high + 1] / (double)counts[flat_high + 1] <= best_score * 1.01)
        ++flat_high;
    for (int candidate = candidate_min; candidate <= candidate_max; ++candidate)
        printf("# badness candidate=%d avg_us=%.6f\n", candidate, scores[candidate] / (double)counts[candidate]);
    printf("# recommended=%d\n", best);
    printf("# recommended_avg_us=%.6f\n", best_score);
    printf("# flat_within_1pct=%d..%d\n", flat_low, flat_high);
    printf("# current=%llu\n", current_value);
    printf("# points=%lu\n", counts[best]);
    printf("recommended %s = %d\n", name, best);
}

void lmmp_tune_print_current_double(const char* name, double default_value, double current_value) {
    printf("%s default=%.10g current=%.10g\n", name, default_value, current_value);
}

bool lmmp_tune_parse_doubles(const char* text, double* values, unsigned max_count, unsigned* count) {
    unsigned parsed_count = 0;
    for (const char* p = text;;) {
        char* end;
        errno = 0;
        double parsed = strtod(p, &end);
        if (end == p || errno != 0 || !isfinite(parsed) || parsed <= 0 || parsed_count == max_count)
            return false;
        values[parsed_count++] = parsed;
        while (*end == ' ' || *end == '\t')
            ++end;
        if (*end == '\0')
            break;
        if (*end != ',')
            return false;
        p = end + 1;
    }
    *count = parsed_count;
    return true;
}

bool lmmp_tune_add_double(double* values, unsigned* count, unsigned max_count, double value) {
    unsigned at = 0;
    while (at < *count && values[at] < value)
        ++at;
    if (at < *count && values[at] == value)
        return true;
    if (*count == max_count)
        return false;
    for (unsigned i = *count; i > at; --i)
        values[i] = values[i - 1];
    values[at] = value;
    ++*count;
    return true;
}

void lmmp_tune_print_double_candidates(const char* name, const double* values, unsigned count) {
    const double step = count > 1 ? values[1] - values[0] : 0.0;
    bool regular = count > 0;
    for (unsigned i = 2; i < count; ++i)
        regular = regular && fabs(values[i] - values[i - 1] - step) <= 1e-9 * fabs(step);
    if (regular) {
        printf("# candidates %s: count=%u range=%.10g..%.10g step=%.10g\n", name, count, values[0], values[count - 1],
               step);
        return;
    }
    printf("# candidates %s: count=%u step=irregular values=", name, count);
    for (unsigned i = 0; i < count; ++i)
        printf(i == 0 ? "%.10g" : ",%.10g", values[i]);
    putchar('\n');
}

/* 联合候选 (x,y) 的平均相对超出；scores 行主序，行宽 y_count */
static double pair_excess_(const double* scores, const unsigned long* counts, unsigned y_count, unsigned x, unsigned y) {
    return scores[x * y_count + y] / (double)counts[x * y_count + y];
}

lmmp_tune_pair_pick lmmp_tune_pair_argmin(const double* scores,
                                          const unsigned long* counts,
                                          unsigned x_count,
                                          unsigned y_count) {
    lmmp_tune_pair_pick pick = {0, 0, NAN};
    for (unsigned x = 0; x < x_count; ++x) {
        for (unsigned y = 0; y < y_count; ++y) {
            if (counts[x * y_count + y] == 0)
                continue;
            const double excess = pair_excess_(scores, counts, y_count, x, y);
            if (isnan(pick.excess) || excess < pick.excess)
                pick = (lmmp_tune_pair_pick){x, y, excess};
        }
    }
    return pick;
}

/* 候选 (x,y) 有测量点且平均相对超出不超过 flat_limit */
static bool pair_in_flat_(const double* scores,
                          const unsigned long* counts,
                          unsigned x_count,
                          unsigned y_count,
                          double flat_limit,
                          unsigned x,
                          unsigned y) {
    return x < x_count && y < y_count && counts[x * y_count + y] != 0 &&
           pair_excess_(scores, counts, y_count, x, y) <= flat_limit;
}

lmmp_tune_pair_pick lmmp_tune_recommend_pair(const char* name_x,
                                             const char* name_y,
                                             const double* scores,
                                             const unsigned long* counts,
                                             const double* candidates_x,
                                             unsigned x_count,
                                             const double* candidates_y,
                                             unsigned y_count,
                                             double current_x,
                                             double current_y) {
    const lmmp_tune_pair_pick best = lmmp_tune_pair_argmin(scores, counts, x_count, y_count);
    if (isnan(best.excess)) {
        printf("# no measurement points; recommendation withheld\n");
        return best;
    }
    /* 平坦带：平均相对超出不超过最优值加 1 个百分点 */
    const double flat_limit = best.excess + 0.01;
    unsigned flat_pairs = 0, pairs_x_low = best.x, pairs_x_high = best.x;
    unsigned pairs_y_low = best.y, pairs_y_high = best.y;
    for (unsigned x = 0; x < x_count; ++x) {
        for (unsigned y = 0; y < y_count; ++y) {
            if (!pair_in_flat_(scores, counts, x_count, y_count, flat_limit, x, y))
                continue;
            ++flat_pairs;
            pairs_x_low = x < pairs_x_low ? x : pairs_x_low;
            pairs_x_high = x > pairs_x_high ? x : pairs_x_high;
            pairs_y_low = y < pairs_y_low ? y : pairs_y_low;
            pairs_y_high = y > pairs_y_high ? y : pairs_y_high;
        }
    }
    /* 过最优点的两条轴切片：沿轴连续留在带内的范围 */
    unsigned slice_x_low = best.x, slice_x_high = best.x, slice_y_low = best.y, slice_y_high = best.y;
    while (slice_x_low > 0 && pair_in_flat_(scores, counts, x_count, y_count, flat_limit, slice_x_low - 1, best.y))
        --slice_x_low;
    while (pair_in_flat_(scores, counts, x_count, y_count, flat_limit, slice_x_high + 1, best.y))
        ++slice_x_high;
    while (slice_y_low > 0 && pair_in_flat_(scores, counts, x_count, y_count, flat_limit, best.x, slice_y_low - 1))
        --slice_y_low;
    while (pair_in_flat_(scores, counts, x_count, y_count, flat_limit, best.x, slice_y_high + 1))
        ++slice_y_high;

    printf("# badness matrix (percent): rows %s, columns %s\n", name_x, name_y);
    printf("#%12s", "");
    for (unsigned y = 0; y < y_count; ++y)
        printf(" %9.6g", candidates_y[y]);
    putchar('\n');
    for (unsigned x = 0; x < x_count; ++x) {
        printf("# %11.6g", candidates_x[x]);
        for (unsigned y = 0; y < y_count; ++y)
            printf(" %9.3f", 100.0 * pair_excess_(scores, counts, y_count, x, y));
        putchar('\n');
    }
    /* 两个一维视图：固定一轴时另一轴的最优搭档 */
    for (unsigned y = 0; y < y_count; ++y) {
        unsigned best_x = 0;
        for (unsigned x = 1; x < x_count; ++x)
            if (pair_excess_(scores, counts, y_count, x, y) < pair_excess_(scores, counts, y_count, best_x, y))
                best_x = x;
        printf("# axis %s=%.10g best_%s=%.10g excess=%.6f\n", name_y, candidates_y[y], name_x, candidates_x[best_x],
               pair_excess_(scores, counts, y_count, best_x, y));
    }
    for (unsigned x = 0; x < x_count; ++x) {
        unsigned best_y = 0;
        for (unsigned y = 1; y < y_count; ++y)
            if (pair_excess_(scores, counts, y_count, x, y) < pair_excess_(scores, counts, y_count, x, best_y))
                best_y = y;
        printf("# axis %s=%.10g best_%s=%.10g excess=%.6f\n", name_x, candidates_x[x], name_y, candidates_y[best_y],
               pair_excess_(scores, counts, y_count, x, best_y));
    }
    printf("# joint_best=(%.10g,%.10g) excess=%.6f\n", candidates_x[best.x], candidates_y[best.y], best.excess);
    printf("# flat_within_1pct_excess: pairs=%u %s=%.10g..%.10g %s=%.10g..%.10g; slices %s=%.10g..%.10g at "
           "%s=%.10g, %s=%.10g..%.10g at %s=%.10g\n",
           flat_pairs, name_x, candidates_x[pairs_x_low], candidates_x[pairs_x_high], name_y,
           candidates_y[pairs_y_low], candidates_y[pairs_y_high], name_x, candidates_x[slice_x_low],
           candidates_x[slice_x_high], name_y, candidates_y[best.y], name_y, candidates_y[slice_y_low],
           candidates_y[slice_y_high], name_x, candidates_x[best.x]);
    int current_x_index = -1, current_y_index = -1;
    for (unsigned x = 0; x < x_count && current_x_index < 0; ++x)
        if (candidates_x[x] == current_x)
            current_x_index = (int)x;
    for (unsigned y = 0; y < y_count && current_y_index < 0; ++y)
        if (candidates_y[y] == current_y)
            current_y_index = (int)y;
    if (current_x_index >= 0 && current_y_index >= 0 && counts[current_x_index * y_count + current_y_index] != 0)
        printf("# current=(%.10g,%.10g) excess=%.6f\n", current_x, current_y,
               pair_excess_(scores, counts, y_count, (unsigned)current_x_index, (unsigned)current_y_index));
    else
        printf("# current=(%.10g,%.10g) off candidate grid\n", current_x, current_y);
    printf("# points=%lu\n", counts[best.x * y_count + best.y]);
    printf("recommended joint (%s,%s) = (%.10g,%.10g)\n", name_x, name_y, candidates_x[best.x], candidates_y[best.y]);
    return best;
}
