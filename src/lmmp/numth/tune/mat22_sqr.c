/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
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

#include "mparam.h"
#include "numth/mat22_mul.h"
#include "support/perf.h"

typedef void (*sqr_fn)(lmmp_mat22_t*, const lmmp_mat22_t*, mp_ptr);

static lmmp_mat22_t matrix_view(mp_ptr storage, mp_size_t stride) {
    return (lmmp_mat22_t){.p = {{storage, storage + stride},
                              {storage + 2 * stride, storage + 3 * stride}}};
}

static bool matrix_equal(const lmmp_mat22_t* a, const lmmp_mat22_t* b) {
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            mp_size_t n = a->n[i][j];
            if (n != b->n[i][j] || lmmp_cmp_(a->p[i][j], b->p[i][j], n) != 0)
                return false;
        }
    }
    return true;
}

static double measure_sqr(sqr_fn fn, lmmp_mat22_t* dst, const lmmp_mat22_t* a, mp_ptr scratch) {
    const bench_options options = {BENCH_MIN, 1, 0.001, 0.05, 1ull << 22};
    double us;
    BENCH_MEASURE(us, &options, fn(dst, a, scratch););
    return us;
}

int main(void) {
    enum { MIN_N = 8, MAX_N = 80, SAMPLES = 5, OUT_CAP = 2 * MAX_N + 4 };
    lmmp_global_init();
    mp_ptr storage = alloc_limbs(4 * MAX_N + 8 * OUT_CAP + 15 * MAX_N + 14);
    lmmp_mat22_t a = matrix_view(storage, MAX_N);
    lmmp_mat22_t basecase = matrix_view(storage + 4 * MAX_N, OUT_CAP);
    lmmp_mat22_t strassen = matrix_view(storage + 4 * MAX_N + 4 * OUT_CAP, OUT_CAP);
    mp_ptr scratch = storage + 4 * MAX_N + 8 * OUT_CAP;

    printf("Current MAT22_SQR_STRASSEN_THRESHOLD = %u\n", (unsigned)MAT22_SQR_STRASSEN_THRESHOLD);
    puts("limbs     basecase us     Strassen us");
    unsigned streak = 0, crossover = 0;
    int result = EXIT_SUCCESS;

    for (unsigned n = MIN_N; n <= MAX_N; ++n) {
        for (size_t i = 0; i < 2; ++i) {
            for (size_t j = 0; j < 2; ++j) {
                a.n[i][j] = n;
                fill_random(a.p[i][j], n, 0x2733ba7a11665d4full + 4 * n + 2 * i + j);
            }
        }

        lmmp_internal_numth_mat22_sqr_basecase(&basecase, &a, scratch);
        lmmp_internal_numth_mat22_sqr_strassen(&strassen, &a, scratch);
        if (!matrix_equal(&basecase, &strassen)) {
            fprintf(stderr, "Matrix square results differ at %u limbs\n", n);
            result = EXIT_FAILURE;
            break;
        }

        double times_base[SAMPLES], times_strassen[SAMPLES];
        for (unsigned i = 0; i < SAMPLES; ++i) {
            if (i % 2 == 0) {
                times_base[i] = measure_sqr(lmmp_internal_numth_mat22_sqr_basecase, &basecase, &a, scratch);
                times_strassen[i] = measure_sqr(lmmp_internal_numth_mat22_sqr_strassen, &strassen, &a, scratch);
            } else {
                times_strassen[i] = measure_sqr(lmmp_internal_numth_mat22_sqr_strassen, &strassen, &a, scratch);
                times_base[i] = measure_sqr(lmmp_internal_numth_mat22_sqr_basecase, &basecase, &a, scratch);
            }
        }
        double base_us = bench_aggregate(times_base, SAMPLES, BENCH_MEDIAN);
        double strassen_us = bench_aggregate(times_strassen, SAMPLES, BENCH_MEDIAN);
        printf("%5u %15.4f %15.4f\n", n, base_us, strassen_us);
        streak = strassen_us < base_us ? streak + 1 : 0;
        if (streak == 3 && crossover == 0)
            crossover = n - 2;
    }

    if (result == EXIT_SUCCESS) {
        if (crossover)
            printf("First three consecutive Strassen wins begin at %u limbs\n", crossover);
        else
            puts("No three consecutive Strassen wins in the sampled range");
    }
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
