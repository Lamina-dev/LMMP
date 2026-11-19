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

bool lmmp_tune_parse(int argc, char** argv, lmmp_tune_options* options) {
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
        } else {
            fprintf(stderr, "unknown option '%s'\n", argv[i]);
            return false;
        }
    }
    return true;
}

void lmmp_tune_usage(const char* program_id) {
    printf("Usage: %s [--run|--info|--current|--help] [options]\n", program_id);
    puts("  --samples N       odd sample count, default 5");
    puts("  --min-ms MS       minimum time per sample block, default 5");
    puts("  --mad-limit X     MAD/median retry limit, default 0.05");
    puts("  --max-retry N     maximum unstable-point retries, default 2");
    puts("  --seed S          deterministic input seed");
}

static mp_size_t calibrate_(const lmmp_tune_bench* bench, const lmmp_tune_options* options) {
    const mp_size_t max_iters = 1ull << 26;
    mp_size_t iters = 1;
    bench->bind(bench->context);
    bench->reset(bench->context);
    for (;;) {
        double start = bench_clock();
        for (mp_size_t i = 0; i < iters; ++i) lmmp_tune_sink ^= bench->call(bench->context);
        double elapsed = bench_clock() - start;
        if (elapsed >= options->min_ms * 1e-3 || iters >= max_iters)
            break;
        iters *= 2;
    }
    return iters;
}

static double timed_block_(const lmmp_tune_bench* bench, mp_size_t iters) {
    bench->bind(bench->context);
    bench->reset(bench->context);
    double start = bench_clock();
    for (mp_size_t i = 0; i < iters; ++i) lmmp_tune_sink ^= bench->call(bench->context);
    return (bench_clock() - start) * 1e6 / (double)iters;
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
                               double* high_mad) {
    mp_size_t low_iters = calibrate_(low, options);
    mp_size_t high_iters = calibrate_(high, options);
    double low_samples[31], high_samples[31];
    for (unsigned sample = 0; sample < options->samples; ++sample) {
        low_samples[sample] = timed_block_(low, low_iters);
        high_samples[sample] = timed_block_(high, high_iters);
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
                            unsigned* retries) {
    for (unsigned retry = 0; retry <= options->max_retry; ++retry) {
        if (!measure_pair_once_(low, high, options, low_us, low_mad, high_us, high_mad))
            return false;
        if ((*low_mad <= options->mad_limit && *high_mad <= options->mad_limit) || retry == options->max_retry) {
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
                           double low_us,
                           double high_us) {
    for (int candidate = candidate_min; candidate <= candidate_max; ++candidate) {
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
