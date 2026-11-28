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

#include <stdlib.h>

#include "lmmp/lmmpn.h"
#include "mparam.h"
#include "numth/ele_mul.h"
#include "numth/tune/product_threshold.h"
#include "support/perf.h"
#include "tune/params.h"

enum { MAX_ELEMENTS = 160, CANDIDATE_MAX = MAX_ELEMENTS + 1 };

typedef struct {
    const char* name;
    unsigned min_bits, max_bits;
} element_shape;

static const element_shape shapes[] = {
    {"packed_33_to_48_bits", 33, 48},
    {"packed_49_to_64_bits", 49, 64},
    {"full_width", 64, 64},
};

typedef struct {
    ulongp input;
    mp_ptr dst;
    mp_ptr scratch;
    mp_size_t n;
    mp_size_t threshold;
} element_context;

static void bind_threshold(void* opaque) {
    element_context* context = opaque;
    lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD = context->threshold;
}

static void reset(void* opaque) { (void)opaque; }

static mp_size_t run_product(const element_context* context) {
    return lmmp_elem_mul_ulong_(context->dst, context->input, context->n, context->scratch);
}

static mp_limb_t call_product(void* opaque) {
    const element_context* context = opaque;
    mp_size_t rn = run_product(context);
    return context->dst[0] ^ context->dst[rn - 1] ^ rn;
}

static void fill_elements(ulongp input, mp_size_t n, const element_shape* shape, mp_limb_t seed) {
    fill_random(input, n, seed);
    for (mp_size_t i = 0; i < n; ++i) {
        unsigned bits = shape->min_bits + (unsigned)(input[i] % (shape->max_bits - shape->min_bits + 1));
        input[i] = (input[i] >> (LIMB_BITS - bits)) | ((mp_limb_t)1 << (bits - 1)) | 1;
    }
}

/* 每层的子问题只可能有向下取整和向上取整两种长度。阈值跨过其中一个长度时，路径才可能改变。 */
static void find_path_starts(bool* starts, mp_size_t n) {
    for (mp_size_t low = n, high = n; high >= PRODUCT_TUNE_MIN; low /= 2, high = (high + 1) / 2) {
        starts[high + 1] = true;
        if (low >= PRODUCT_TUNE_MIN)
            starts[low + 1] = true;
    }
}

static bool check_product(void* opaque, mp_srcptr expected, mp_size_t expected_n) {
    element_context* context = opaque;
    bind_threshold(context);
    return run_product(context) == expected_n && lmmp_cmp_(context->dst, expected, expected_n) == 0;
}

static bool measure_shape(const element_shape* shape, mp_size_t current, const lmmp_tune_options* options) {
    product_tune_scores scores = {0};
    ulong input[MAX_ELEMENTS];
    mp_limb_t expected[MAX_ELEMENTS], dst[MAX_ELEMENTS], scratch[MAX_ELEMENTS];
    element_context baseline = {.input = input, .dst = dst, .scratch = scratch, .threshold = current};
    element_context candidate = baseline;
    product_tune_pair pair = {
        .baseline = {.bind = bind_threshold, .reset = reset, .call = call_product, .context = &baseline},
        .candidate = {.bind = bind_threshold, .reset = reset, .call = call_product, .context = &candidate},
        .current = current,
        .threshold = &candidate.threshold,
        .check = check_product,
    };

    for (mp_size_t n = PRODUCT_TUNE_MIN; n <= MAX_ELEMENTS; n += n < 96 ? 1 : 4) {
        fill_elements(input, n, shape, (mp_limb_t)options->seed + n);
        baseline.n = candidate.n = n;

        bool starts[CANDIDATE_MAX + 1] = {0};
        find_path_starts(starts, n);
        lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD = n + 1;
        mp_size_t expected_n = lmmp_elem_mul_ulong_(expected, input, n, scratch);
        product_tune_input point = {shape->name, n, expected_n, expected, expected_n, starts};
        if (!measure_product_input(&pair, &point, CANDIDATE_MAX, options, &scores))
            return false;
    }
    print_product_scores("ELEM_MUL_BASECASE_THRESHOLD", shape->name, &scores, CANDIDATE_MAX, current);
    return true;
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_elem_mul_basecase");
        return EXIT_SUCCESS;
    }
    mp_size_t current = lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD;
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current("ELEM_MUL_BASECASE_THRESHOLD", ELEM_MUL_BASECASE_THRESHOLD, current);
        return EXIT_SUCCESS;
    }

    if (options.action == LMMP_TUNE_INFO) {
        puts("program=tune_numth_elem_mul_basecase");
        puts("parameter=ELEM_MUL_BASECASE_THRESHOLD");
        puts("binding_mode=runtime-branch");
        printf("default=%llu\ncurrent=%llu\n", (unsigned long long)ELEM_MUL_BASECASE_THRESHOLD,
               (unsigned long long)current);
        printf("candidate_range=%d..%d\ninput_range=%d..%d\n", PRODUCT_TUNE_MIN, CANDIDATE_MAX, PRODUCT_TUNE_MIN,
               MAX_ELEMENTS);
        puts("input_step=1 through 96, then 4");
        lmmp_tune_print_env();
        return EXIT_SUCCESS;
    }

    printf("# program=tune_numth_elem_mul_basecase current=%llu candidate_range=%d..%d input_range=%d..%d\n",
           (unsigned long long)current, PRODUCT_TUNE_MIN, CANDIDATE_MAX, PRODUCT_TUNE_MIN, MAX_ELEMENTS);
    puts("# score=mean candidate/current ratio over common stable inputs, separately for each shape");
    puts("# input_step=1 through 96, then 4");
    puts("# aa=1 compares the current threshold with itself");
    if (current < PRODUCT_TUNE_MIN || current > CANDIDATE_MAX) {
        fprintf(stderr, "Current threshold is outside the candidate range\n");
        return EXIT_FAILURE;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    lmmp_tune_print_env_comment();
    printf("# seed=%llu samples=%u min_ms=%.3f mad_limit=%.4f max_retry=%u\n", options.seed, options.samples,
           options.min_ms, options.mad_limit, options.max_retry);
    puts("shape,n,rn,first,last,aa,current_us,candidate_us,ratio,current_mad,candidate_mad,retries,stable");
    bool ok = true;
    for (unsigned i = 0; ok && i < sizeof(shapes) / sizeof(shapes[0]); ++i)
        ok = measure_shape(&shapes[i], current, &options);
    lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD = current;
    lmmp_global_deinit();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
