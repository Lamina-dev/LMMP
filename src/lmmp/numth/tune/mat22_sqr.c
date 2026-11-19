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

#include "mparam.h"
#include "numth/mat22_mul.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

enum { MIN_N = 8, MAX_N = 80, SHAPE_COUNT = 4, CANDIDATE_MIN = 2, CANDIDATE_MAX = MAX_N + 4 };

typedef struct {
    mp_size_t n[2][2];
} matrix_shape;

typedef struct {
    lmmp_mat22_t* input;
    lmmp_mat22_t* output;
    mp_ptr scratch;
    mp_size_t threshold;
} measure_context;

static const matrix_shape shapes[SHAPE_COUNT] = {
    {{{0, 0}, {0, 0}}},
    {{{0, 1}, {2, 3}}},
    {{{3, 1}, {2, 0}}},
    {{{2, 3}, {1, 0}}},
};

static lmmp_mat22_t matrix_view(mp_ptr storage, mp_size_t stride) {
    return (lmmp_mat22_t){
        .p = {{storage, storage + stride}, {storage + 2 * stride, storage + 3 * stride}},
    };
}

static bool matrix_equal(const lmmp_mat22_t* a, const lmmp_mat22_t* b) {
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            mp_size_t n = a->n[i][j];
            if (n != b->n[i][j])
                return false;
            if (n != 0 && lmmp_cmp_(a->p[i][j], b->p[i][j], n) != 0)
                return false;
        }
    }
    return true;
}

static mp_limb_t matrix_fingerprint(const lmmp_mat22_t* matrix) {
    mp_limb_t result = 0;
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            mp_size_t n = matrix->n[i][j];
            result ^= n;
            if (n != 0)
                result ^= matrix->p[i][j][0] + matrix->p[i][j][n - 1];
        }
    }
    return result;
}

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD = context->threshold;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 2; ++j) context->output->n[i][j] = 0;
}

static mp_limb_t call_square(void* opaque) {
    measure_context* context = opaque;
    lmmp_mat22_sqr_(context->output, context->input, context->scratch);
    return matrix_fingerprint(context->output);
}

static mp_size_t shape_value(const matrix_shape* shape, mp_size_t n, size_t i, size_t j) {
    switch (shape->n[i][j]) {
        case 0:
            return n;
        case 1:
            return n + 1;
        case 2:
            return n + 2;
        default:
            return n + 3;
    }
}

static mp_size_t shape_min(const matrix_shape* shape, mp_size_t n) {
    mp_size_t result = shape_value(shape, n, 0, 0);
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            mp_size_t value = shape_value(shape, n, i, j);
            if (value < result)
                result = value;
        }
    }
    return result;
}

static void fill_matrix(lmmp_mat22_t* matrix, const matrix_shape* shape, mp_size_t n, unsigned long long seed) {
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            matrix->n[i][j] = shape_value(shape, n, i, j);
            fill_random(matrix->p[i][j], matrix->n[i][j], (mp_limb_t)(seed + 4 * n + 2 * i + j));
        }
    }
}

static void tune_abort(lmmp_error_t type, const char* message, const char* function, int line) {
    fprintf(stderr, "LMMP abort %d in %s:%d: %s\n", (int)type, function, line, message);
    _Exit(EXIT_FAILURE);
}

static void print_info(void) {
    printf("program=tune_numth_mat22_sqr\n");
    printf("parameter=MAT22_SQR_STRASSEN_THRESHOLD\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)MAT22_SQR_STRASSEN_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD);
    printf("candidate_range=%d..%d\n", CANDIDATE_MIN, CANDIDATE_MAX);
    printf("input_range=%d..%d\n", MIN_N, MAX_N);
#if defined(LMMP_ASM_X64)
    puts("asm_backend=x64");
#elif defined(LMMP_ASM_ARM64)
    puts("asm_backend=arm64");
#else
    puts("asm_backend=generic");
#endif
#if defined(LMMP_WINDOWS)
    puts("target_system=windows");
#elif defined(LMMP_LINUX)
    puts("target_system=linux");
#elif defined(LMMP_MACOS)
    puts("target_system=macos");
#else
    puts("target_system=unknown");
#endif
    printf("build_type=%s\n", LMMP_BUILD_TYPE_STR);
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_mat22_sqr");
        return EXIT_SUCCESS;
    }

    lmmp_set_abort_fn(tune_abort);
    lmmp_global_init();
    if (options.action == LMMP_TUNE_INFO) {
        print_info();
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        printf("MAT22_SQR_STRASSEN_THRESHOLD default=%llu current=%llu\n",
               (unsigned long long)MAT22_SQR_STRASSEN_THRESHOLD,
               (unsigned long long)lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t input_stride = MAX_N + 4;
    const mp_size_t output_stride = 2 * input_stride + 4;
    const mp_size_t scratch_size = 15 * input_stride + 14;
    mp_ptr storage = alloc_limbs(4 * input_stride + 8 * output_stride + scratch_size);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    lmmp_mat22_t input = matrix_view(storage, input_stride);
    lmmp_mat22_t low = matrix_view(storage + 4 * input_stride, output_stride);
    lmmp_mat22_t high = matrix_view(storage + 4 * input_stride + 4 * output_stride, output_stride);
    mp_ptr scratch = storage + 4 * input_stride + 8 * output_stride;
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD;
    FILE* output = lmmp_tune_open_output(&options);
    if (output == NULL) {
        lmmp_free(storage);
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }

    fprintf(output, "# program=tune_numth_mat22_sqr\n");
    fprintf(output, "# parameter=MAT22_SQR_STRASSEN_THRESHOLD\n");
    fprintf(output, "# binding_mode=runtime-branch\n");
    fprintf(output, "# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
            (unsigned long long)MAT22_SQR_STRASSEN_THRESHOLD, options.seed, options.samples, options.min_ms,
            options.mad_limit);
    fprintf(output, "# candidate_range=%d..%d input_range=%d..%d shapes=%d\n", CANDIDATE_MIN, CANDIDATE_MAX, MIN_N,
            MAX_N, SHAPE_COUNT);
    fprintf(output, "# build_type=%s compiler=%s\n", LMMP_BUILD_TYPE_STR, __VERSION__);
#if defined(LMMP_WINDOWS)
    fprintf(output, "# target_system=windows\n");
#elif defined(LMMP_LINUX)
    fprintf(output, "# target_system=linux\n");
#elif defined(LMMP_MACOS)
    fprintf(output, "# target_system=macos\n");
#else
    fprintf(output, "# target_system=unknown\n");
#endif
#if defined(LMMP_ASM_X64)
    fprintf(output, "# asm_backend=x64\n");
#elif defined(LMMP_ASM_ARM64)
    fprintf(output, "# asm_backend=arm64\n");
#else
    fprintf(output, "# asm_backend=generic\n");
#endif
    fprintf(output, "# shape n min low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (unsigned shape_id = 0; shape_id < SHAPE_COUNT && result == EXIT_SUCCESS; ++shape_id) {
        const matrix_shape* shape = &shapes[shape_id];
        for (mp_size_t n = MIN_N; n <= MAX_N; ++n) {
            mp_size_t minimum = shape_min(shape, n);
            fill_matrix(&input, shape, n, options.seed + 0x9e3779b97f4a7c15ULL * (shape_id + 1));

            measure_context low_context = {
                .input = &input, .output = &low, .scratch = scratch, .threshold = minimum + 1};
            measure_context high_context = {.input = &input, .output = &high, .scratch = scratch, .threshold = minimum};
            lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD = low_context.threshold;
            lmmp_mat22_sqr_(&low, &input, scratch);
            lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD = high_context.threshold;
            lmmp_mat22_sqr_(&high, &input, scratch);
            if (!matrix_equal(&low, &high)) {
                fprintf(stderr, "matrix result mismatch at shape %u, n=%llu\n", shape_id, (unsigned long long)n);
                result = EXIT_FAILURE;
                break;
            }

            lmmp_tune_bench low_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_square, .context = &low_context};
            lmmp_tune_bench high_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_square, .context = &high_context};
            double low_us, low_mad, high_us, high_mad;
            unsigned low_retries, high_retries;
            if (!lmmp_tune_measure(&low_bench, &options, &low_us, &low_mad, &low_retries) ||
                !lmmp_tune_measure(&high_bench, &options, &high_us, &high_mad, &high_retries)) {
                fprintf(stderr, "measurement failed at shape %u, n=%llu\n", shape_id, (unsigned long long)n);
                result = EXIT_FAILURE;
                break;
            }
            unsigned retries = low_retries > high_retries ? low_retries : high_retries;
            unsigned stable = low_mad <= options.mad_limit && high_mad <= options.mad_limit;
            fprintf(output, "%u %llu %llu %.6f %.6f %.6f %.6f %u %u\n", shape_id, (unsigned long long)n,
                    (unsigned long long)minimum, low_us, low_mad, high_us, high_mad, retries, stable);
            for (int candidate = CANDIDATE_MIN; candidate <= CANDIDATE_MAX; ++candidate) {
                scores[candidate] += minimum >= (mp_size_t)candidate ? high_us : low_us;
                ++counts[candidate];
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        int best = CANDIDATE_MIN;
        for (int candidate = CANDIDATE_MIN + 1; candidate <= CANDIDATE_MAX; ++candidate) {
            if (scores[candidate] < scores[best])
                best = candidate;
        }
        double best_score = scores[best] / (double)counts[best];
        int flat_low = best;
        int flat_high = best;
        while (flat_low > CANDIDATE_MIN && scores[flat_low - 1] / (double)counts[flat_low - 1] <= best_score * 1.01)
            --flat_low;
        while (flat_high < CANDIDATE_MAX && scores[flat_high + 1] / (double)counts[flat_high + 1] <= best_score * 1.01)
            ++flat_high;
        for (int candidate = CANDIDATE_MIN; candidate <= CANDIDATE_MAX; ++candidate)
            fprintf(output, "# badness candidate=%d avg_us=%.6f\n", candidate,
                    scores[candidate] / (double)counts[candidate]);
        fprintf(output, "# recommended=%d\n", best);
        fprintf(output, "# recommended_avg_us=%.6f\n", best_score);
        fprintf(output, "# flat_within_1pct=%d..%d\n", flat_low, flat_high);
        fprintf(output, "# current=%llu\n", (unsigned long long)saved_threshold);
        fprintf(output, "# points=%lu\n", counts[best]);
        printf("recommended MAT22_SQR_STRASSEN_THRESHOLD = %d\n", best);
    }
    lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD = saved_threshold;
    lmmp_tune_close_output(&options, output);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
