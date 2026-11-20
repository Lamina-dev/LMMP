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
    lmmp_mat22_t* a;
    lmmp_mat22_t* b;
    lmmp_mat22_t* output;
    mp_ptr scratch;
    mp_size_t threshold;
} measure_context;

/* 形状 0 为四元素等长的平衡矩阵；其余为合法的非完全相同形状（偏移 1..3）。 */
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
    lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD = context->threshold;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 2; ++j) context->output->n[i][j] = 0;
}

static mp_limb_t call_mul(void* opaque) {
    measure_context* context = opaque;
    lmmp_mat22_mul_(context->output, context->a, context->b, context->scratch);
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

static void print_info(void) {
    printf("program=tune_numth_mat22_mul\n");
    printf("parameter=MAT22_MUL_STRASSEN_THRESHOLD\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)MAT22_MUL_STRASSEN_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD);
    printf("candidate_range=%d..%d\n", CANDIDATE_MIN, CANDIDATE_MAX);
    printf("input_range=%d..%d\n", MIN_N, MAX_N);
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_mat22_mul");
        return EXIT_SUCCESS;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    if (options.action == LMMP_TUNE_INFO) {
        print_info();
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current("MAT22_MUL_STRASSEN_THRESHOLD", (unsigned long long)MAT22_MUL_STRASSEN_THRESHOLD,
                                (unsigned long long)lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    /* 输入元素最长 MAX_N+3；结果元素容量 mx(a)+mx(b)+4；scratch 8*(mx(a)+mx(b))+16 */
    const mp_size_t input_stride = MAX_N + 4;
    const mp_size_t output_stride = 2 * input_stride + 4;
    const mp_size_t scratch_size = 16 * input_stride + 16;
    mp_ptr storage = alloc_limbs(8 * input_stride + 8 * output_stride + scratch_size);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    lmmp_mat22_t a = matrix_view(storage, input_stride);
    lmmp_mat22_t b = matrix_view(storage + 4 * input_stride, input_stride);
    lmmp_mat22_t low = matrix_view(storage + 8 * input_stride, output_stride);
    lmmp_mat22_t high = matrix_view(storage + 8 * input_stride + 4 * output_stride, output_stride);
    mp_ptr scratch = storage + 8 * input_stride + 8 * output_stride;

    /* 按形状独立评分：Strassen 门控 mat22_all_big_ 要求 A、B 全部元素 >= 阈值，
       含小元素的形状在候选高于其最小元素时 high 被门控退化为 basecase，
       混合累计会抬高"关闭 Strassen"候选。推荐值以平衡形状（shape 0）为准，
       其余形状只输出测量行供形状依赖分析。 */
    double scores[SHAPE_COUNT][CANDIDATE_MAX + 1] = {{0}};
    unsigned long counts[SHAPE_COUNT][CANDIDATE_MAX + 1] = {{0}};
    mp_size_t saved_threshold = lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD;
    /* 稳定计数按形状分列：推荐只依据 shape 0（平衡形状），门控也只看它的稳定点 */
    unsigned stable_points[SHAPE_COUNT] = {0}, unstable_points[SHAPE_COUNT] = {0};

    printf("# program=tune_numth_mat22_mul\n");
    printf("# parameter=MAT22_MUL_STRASSEN_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)MAT22_MUL_STRASSEN_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# candidate_range=%d..%d input_range=%d..%d shapes=%d\n", CANDIDATE_MIN, CANDIDATE_MAX, MIN_N, MAX_N,
           SHAPE_COUNT);
    lmmp_tune_print_env_comment();
    printf("# shape n min low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (unsigned shape_id = 0; shape_id < SHAPE_COUNT && result == EXIT_SUCCESS; ++shape_id) {
        const matrix_shape* shape = &shapes[shape_id];
        for (mp_size_t n = MIN_N; n <= MAX_N; ++n) {
            mp_size_t minimum = shape_min(shape, n);
            fill_matrix(&a, shape, n, options.seed + 0x9e3779b97f4a7c15ULL * (shape_id + 1));
            fill_matrix(&b, shape, n, options.seed + 0x9e3779b97f4a7c15ULL * (shape_id + 1) + 0x1234567ULL);

            measure_context low_context = {
                .a = &a, .b = &b, .output = &low, .scratch = scratch, .threshold = minimum + 1};
            measure_context high_context = {
                .a = &a, .b = &b, .output = &high, .scratch = scratch, .threshold = minimum};
            lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD = low_context.threshold;
            lmmp_mat22_mul_(&low, &a, &b, scratch);
            lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD = high_context.threshold;
            lmmp_mat22_mul_(&high, &a, &b, scratch);
            if (!matrix_equal(&low, &high)) {
                fprintf(stderr, "matrix result mismatch at shape %u, n=%llu\n", shape_id, (unsigned long long)n);
                result = EXIT_FAILURE;
                break;
            }

            lmmp_tune_bench low_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_mul, .context = &low_context};
            lmmp_tune_bench high_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_mul, .context = &high_context};
            double low_us, low_mad, high_us, high_mad;
            unsigned retries;
            bool stable;
            if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                        &retries, &stable)) {
                fprintf(stderr, "measurement failed at shape %u, n=%llu\n", shape_id, (unsigned long long)n);
                result = EXIT_FAILURE;
                break;
            }
            if (stable)
                ++stable_points[shape_id];
            else
                ++unstable_points[shape_id];
            printf("%u %llu %llu %.6f %.6f %.6f %.6f %u %u\n", shape_id, (unsigned long long)n,
                   (unsigned long long)minimum, low_us, low_mad, high_us, high_mad, retries, stable);
            /* Strassen 子矩阵乘经完整 lmmp_mat22_mul_ 但阈值随行绑定（minimum），
               子问题规模折半后总低于本行阈值，无同阈值递归，sub_n=0 */
            lmmp_tune_score_point(scores[shape_id], counts[shape_id], CANDIDATE_MIN, CANDIDATE_MAX, minimum, 0, low_us,
                                  high_us);
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points[0] == 0) {
            printf("# no stable measurement points in shape 0; recommendation withheld\n");
        } else {
            printf("# recommend_basis=shape0_balanced\n");
            if (unstable_points[0] > 0)
                printf("# note: %u unstable point(s) retained in shape 0 scoring\n", unstable_points[0]);
            lmmp_tune_recommend("MAT22_MUL_STRASSEN_THRESHOLD", scores[0], counts[0], CANDIDATE_MIN, CANDIDATE_MAX,
                                (unsigned long long)saved_threshold);
        }
    }
    lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD = saved_threshold;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
