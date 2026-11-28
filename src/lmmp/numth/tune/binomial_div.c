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

#include "combin_tune.h"

enum { MAX_R = 131072, FIRST_K = 36, LAST_K = 128 };

static const uint n_values[] = {65536, 98304, 131072, 196608, 262144, 393216,
                                 524288, 786432, 1048576, 1572864, 2097152};
static const unsigned k_levels[] = {48, 56, 62, 68, 69, 70, 71, 72, 74, 80, 88, 96, 104, 112};

/* 固定 n 后，随 r 增大，nPr_n / fac_n 整体下降。按候选分派式定位 r，再按实取的长度评分。 */
static uint r_for_k(uint n, uint low, uint high, unsigned k) {
    while (low < high) {
        const uint mid = low + (high - low) / 2;
        mp_size_t nPr_n, fac_n;
        binomial_div_lengths(n, mid, &nPr_n, &fac_n);
        if (k * nPr_n > 128 * fac_n)
            low = mid + 1;
        else
            high = mid;
    }
    return low;
}

int main(int argc, char** argv) {
    const char* program = "tune_numth_binomial_div";
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    options.samples = 3;
    options.min_ms = 2.0;
    options.max_retry = 1;
    binomial_tune_options current = binomial_current();
    if (!lmmp_tune_parse_ex(argc, argv, &options, binomial_option, &current))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage(program);
        puts("  --rn-threshold N   Minimum odd-part buffer length for the div/factor dispatch");
        puts("  --div-k K          Compare the recommendation with K");
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_INFO) {
        printf("program=%s\n", program);
        puts("paths=div,factor\nprime_table=warm\nn=65536..2097152\nr_max=131072");
        puts("grid=11 n columns, up to 14 r per column, with closer spacing near the current slope");
        lmmp_tune_print_env();
    }
    if (options.action == LMMP_TUNE_INFO || options.action == LMMP_TUNE_CURRENT) {
        binomial_print_current(current);
        return EXIT_SUCCESS;
    }

    const unsigned first = current.div_k < FIRST_K ? (unsigned)current.div_k : FIRST_K;
    const unsigned last = current.div_k > LAST_K ? (unsigned)current.div_k : LAST_K;
    double scores[257] = {0};
    unsigned stable = 0, unstable = 0, div_faster = 0, factor_faster = 0;
    printf("# rn_threshold=%" PRIu64 " current_div_k=%" PRIu64 "\n", current.rn_threshold, current.div_k);
    combin_start(program, &options);
    const double start = bench_clock();
    for (unsigned i = 0; i < sizeof(n_values) / sizeof(n_values[0]); ++i) {
        const uint n = n_values[i], high = n / 2 < MAX_R ? n / 2 : MAX_R;
        const uint low = binomial_r_for_rn(n, 4, high, current.rn_threshold);
        uint previous = 0;
        for (unsigned j = 0; j < sizeof(k_levels) / sizeof(k_levels[0]); ++j) {
            const uint r = r_for_k(n, low, high, k_levels[j]);
            if (r == previous)
                continue;
            previous = r;
            const combin_tune_input input = combin_input(true, n, r);
            if (input.rn < current.rn_threshold)
                continue;
            mp_size_t nPr_n, fac_n;
            binomial_div_lengths(n, r, &nPr_n, &fac_n);
            const combin_tune_binding div = {LMMP_TUNE_NCR_DIV, current.rn_threshold, 256};
            const combin_tune_binding factor = {LMMP_TUNE_NCR_FACTOR, current.rn_threshold, 0};
            combin_tune_times times;
            if (!combin_measure("div_factor", binomial_entry(), &input, div, factor, &options, &times)) {
                lmmp_global_deinit();
                return EXIT_FAILURE;
            }
            printf("# lengths n=%u r=%u nPr_n=%" PRIu64 " fac_n=%" PRIu64 "\n", n, r, nPr_n, fac_n);
            if (!times.stable) {
                ++unstable;
                continue;
            }
            ++stable;
            div_faster += times.low_us < times.high_us;
            factor_faster += times.high_us < times.low_us;
            for (unsigned k = first; k <= last; ++k)
                scores[k] += combin_loss(times, k * nPr_n > 128 * fac_n);
        }
    }
    printf("# elapsed_s=%.3f unstable=%u div_faster=%u factor_faster=%u\n", bench_clock() - start, unstable,
           div_faster, factor_faster);
    const bool ok = combin_recommend("BINOMIAL_DIV_K_THRESHOLD", scores, first, last, (unsigned)current.div_k, stable);
    lmmp_global_deinit();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
