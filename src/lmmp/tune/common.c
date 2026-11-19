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
        .output_path = NULL,
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
        } else if (strcmp(argv[i], "--out") == 0) {
            if (!need_value_(argc, argv, &i, argv[i], &value))
                return false;
            options->output_path = value;
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
    puts("  --out FILE        write results to FILE");
}

FILE* lmmp_tune_open_output(const lmmp_tune_options* options) {
    if (options->output_path == NULL)
        return stdout;
#if defined(LMMP_WINDOWS)
    FILE* output = NULL;
    if (fopen_s(&output, options->output_path, "w") != 0)
        output = NULL;
#else
    FILE* output = fopen(options->output_path, "w");
#endif
    if (output == NULL)
        fprintf(stderr, "cannot open output '%s'\n", options->output_path);
    return output;
}

void lmmp_tune_close_output(const lmmp_tune_options* options, FILE* output) {
    if (output != NULL && output != stdout && options->output_path != NULL)
        fclose(output);
}

static bool measure_once_(const lmmp_tune_bench* bench,
                          const lmmp_tune_options* options,
                          double* median_us,
                          double* mad_ratio) {
    const mp_size_t max_iters = 1ull << 26;
    mp_size_t iters = 1;
    double elapsed;
    bench->bind(bench->context);
    bench->reset(bench->context);
    for (;;) {
        double start = bench_clock();
        for (mp_size_t i = 0; i < iters; ++i) lmmp_tune_sink ^= bench->call(bench->context);
        elapsed = bench_clock() - start;
        if (elapsed >= options->min_ms * 1e-3 || iters >= max_iters)
            break;
        iters *= 2;
    }

    double samples[31];
    for (unsigned sample = 0; sample < options->samples; ++sample) {
        bench->bind(bench->context);
        bench->reset(bench->context);
        double start = bench_clock();
        for (mp_size_t i = 0; i < iters; ++i) lmmp_tune_sink ^= bench->call(bench->context);
        samples[sample] = (bench_clock() - start) * 1e6 / (double)iters;
    }
    *median_us = median_(samples, options->samples);
    *mad_ratio = mad_ratio_(samples, options->samples, *median_us);
    return isfinite(*median_us) && isfinite(*mad_ratio);
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
