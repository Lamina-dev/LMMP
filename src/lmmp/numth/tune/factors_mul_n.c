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
#include <string.h>

#include "lmmp/lmmpn.h"
#include "mparam.h"
#include "numth/ele_mul.h"
#include "numth/tune/product_threshold.h"
#include "tune/params.h"

enum { CANDIDATE_MAX = 96, MAX_FACTORS = 96, RESULT_CAPACITY = 512 };

typedef mp_size_t (*factor_mul)(mp_ptr, mp_size_t, fac_ptr, uint);

static mp_size_t factors_mul_ushort(mp_ptr dst, mp_size_t rn, fac_ptr fac, uint n) {
    return lmmp_factors_mul_ushort_(dst, rn, fac, (ushort)n);
}

typedef struct {
    const char* name;
    factor_mul mul;
    bool short_variant;
    bool square;
} factor_shape;

static const factor_shape shapes[] = {
    {"factorial", lmmp_factors_mul_, false, false},
    {"factorial_squared", lmmp_factors_mul_, false, true},
    {"double_factorial", factors_mul_ushort, true, false},
    {"double_factorial_squared", factors_mul_ushort, true, true},
};

typedef struct {
    factor_mul mul;
    const fac_t* input;
    fac_ptr pool;
    fac_ptr next;
    mp_ptr dst;
    mp_size_t rn;
    uint n;
    mp_size_t threshold;
} factor_context;

static void bind_threshold(void* opaque) {
    factor_context* context = opaque;
    lmmp_tune_FACTORS_MUL_N_THRESHOLD = context->threshold;
}

static void reset_batch(void* opaque, mp_size_t count) {
    factor_context* context = opaque;
    context->next = context->pool;
    for (mp_size_t i = 0; i < count; ++i)
        memcpy(context->pool + i * context->n, context->input, context->n * sizeof(*context->input));
}

static mp_size_t run_product(factor_context* context) {
    fac_ptr fac = context->next;
    context->next += context->n;
    return context->mul(context->dst, context->rn, fac, context->n);
}

static mp_limb_t call_product(void* opaque) {
    factor_context* context = opaque;
    mp_size_t rn = run_product(context);
    return context->dst[0] ^ context->dst[rn - 1] ^ rn;
}

static uint factorial_exponent(uint n, uint prime) {
    uint exponent = 0;
    while ((n /= prime) != 0) exponent += n;
    return exponent;
}

static void fill_factors(fac_ptr fac, const uint* primes, uint n, const factor_shape* shape) {
    uint last_prime = primes[n - 1];
    for (uint i = 0; i < n; ++i) {
        uint prime = primes[i];
        uint exponent = factorial_exponent(last_prime, prime);
        if (shape->short_variant)
            exponent -= factorial_exponent(last_prime / 2, prime);
        fac[i] = (fac_t){prime, exponent << shape->square};
    }
}

/* 由单 limb 乘法验证完整乘积，并求出 ushort 递归各层的奇指数乘积长度。 */
static mp_size_t reference_product(mp_ptr dst, const fac_t* fac, uint n, unsigned shift, bool odd_only) {
    dst[0] = 1;
    mp_size_t rn = 1;
    for (uint i = 0; i < n; ++i) {
        uint exponent = fac[i].j >> shift;
        if (odd_only)
            exponent &= 1;
        for (uint e = 0; e < exponent; ++e) {
            dst[rn] = lmmp_mul_1_(dst, dst, rn, fac[i].f);
            rn += dst[rn] != 0;
        }
    }
    return rn;
}

/* 指数每次减半，因子个数可能连续几层相同。只在实际可以进入 basecase 的层记录候选分界。 */
static void find_path_starts(bool* starts, const fac_t* fac, uint n, mp_size_t rn, bool short_variant) {
    mp_limb_t odd[RESULT_CAPACITY];
    for (unsigned shift = 0;; ++shift) {
        uint remaining = 0;
        for (uint i = 0; i < n; ++i) remaining += (fac[i].j >> shift) != 0;
        if (remaining <= PRODUCT_TUNE_MIN)
            return;
        if (!short_variant || rn < 20)
            starts[remaining] = true;
        if (short_variant) {
            mp_size_t odd_n = reference_product(odd, fac, n, shift, true);
            if (odd_n == 1 && odd[0] == 1)
                odd_n = 0;
            rn = ((rn - odd_n) >> 1) + 1;
        }
    }
}

static bool check_product(void* opaque, mp_srcptr expected, mp_size_t expected_n) {
    factor_context* context = opaque;
    bind_threshold(context);
    reset_batch(context, 1);
    return run_product(context) == expected_n && lmmp_cmp_(context->dst, expected, expected_n) == 0;
}

static bool measure_shape(const factor_shape* shape,
                          const uint* primes,
                          mp_size_t current,
                          const lmmp_tune_options* options) {
    product_tune_scores scores = {0};
    fac_t fac[MAX_FACTORS], pool[LMMP_TUNE_BATCH_COPIES * MAX_FACTORS];
    mp_limb_t expected[RESULT_CAPACITY], dst[RESULT_CAPACITY];
    factor_context baseline = {.mul = shape->mul, .input = fac, .pool = pool, .dst = dst, .threshold = current};
    factor_context candidate = baseline;
    product_tune_pair pair = {
        .baseline = {.bind = bind_threshold, .reset_batch = reset_batch, .call = call_product, .context = &baseline},
        .candidate = {.bind = bind_threshold, .reset_batch = reset_batch, .call = call_product, .context = &candidate},
        .current = current,
        .threshold = &candidate.threshold,
        .check = check_product,
    };

    for (uint n = PRODUCT_TUNE_MIN + 1; n <= MAX_FACTORS; ++n) {
        fill_factors(fac, primes, n, shape);
        mp_size_t expected_n = reference_product(expected, fac, n, 0, false);
        baseline.n = candidate.n = n;
        baseline.rn = candidate.rn = expected_n + 2;

        bool starts[CANDIDATE_MAX + 1] = {0};
        find_path_starts(starts, fac, n, baseline.rn, shape->short_variant);
        product_tune_input point = {shape->name, n, baseline.rn, expected, expected_n, starts};
        if (!measure_product_input(&pair, &point, CANDIDATE_MAX, options, &scores))
            return false;
    }
    print_product_scores("FACTORS_MUL_N_THRESHOLD", shape->name, &scores, CANDIDATE_MAX, current);
    return true;
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_factors_mul_n");
        return EXIT_SUCCESS;
    }
    mp_size_t current = lmmp_tune_FACTORS_MUL_N_THRESHOLD;
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current("FACTORS_MUL_N_THRESHOLD", FACTORS_MUL_N_THRESHOLD, current);
        return EXIT_SUCCESS;
    }

    if (options.action == LMMP_TUNE_INFO) {
        puts("program=tune_numth_factors_mul_n");
        puts("parameter=FACTORS_MUL_N_THRESHOLD");
        puts("binding_mode=runtime-branch");
        printf("default=%llu\ncurrent=%llu\n", (unsigned long long)FACTORS_MUL_N_THRESHOLD,
               (unsigned long long)current);
        printf("candidate_range=%d..%d\ninput_range=%d..%d\n", PRODUCT_TUNE_MIN, CANDIDATE_MAX, PRODUCT_TUNE_MIN + 1,
               MAX_FACTORS);
        lmmp_tune_print_env();
        return EXIT_SUCCESS;
    }

    printf("# program=tune_numth_factors_mul_n current=%llu candidate_range=%d..%d input_range=%d..%d\n",
           (unsigned long long)current, PRODUCT_TUNE_MIN, CANDIDATE_MAX, PRODUCT_TUNE_MIN + 1, MAX_FACTORS);
    puts("# score=mean candidate/current ratio over common stable inputs, separately for each shape");
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
    uint primes[MAX_FACTORS], prime = 2;
    for (uint i = 0; i < MAX_FACTORS; ++i) {
        prime = (uint)lmmp_next_prime_ulong_(prime);
        primes[i] = prime;
    }
    bool ok = true;
    for (unsigned i = 0; ok && i < sizeof(shapes) / sizeof(shapes[0]); ++i)
        ok = measure_shape(&shapes[i], primes, current, &options);
    lmmp_tune_FACTORS_MUL_N_THRESHOLD = current;
    lmmp_global_deinit();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
