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

enum { FIRST_RN = 8, LAST_RN = 144, MAX_FACTOR_N = 2097152 };

static const uint n_values[] = {512, 2048, 4095, 4096, 8192, 32768, 65535,
                                 65536, 131072, 524288, 2097152, 0x10000003, MP_UINT_MAX};
static const unsigned rn_levels[] = {6, 8, 12, 16, 17, 18, 19, 20, 22, 24, 28, 32,
                                     36, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160};

static lmmp_tune_combin_path binomial_large_path(const combin_tune_input* input, mp_size_t div_k) {
    if (input->n <= NCR_SHORT_LIMIT)
        return LMMP_TUNE_NCR_FACTOR;
    mp_size_t nPr_n, fac_n;
    binomial_div_lengths(input->n, input->r, &nPr_n, &fac_n);
    return div_k * nPr_n > 128 * fac_n ? LMMP_TUNE_NCR_DIV : LMMP_TUNE_NCR_FACTOR;
}

static lmmp_tune_combin_path binomial_path(const combin_tune_input* input, binomial_tune_options binding) {
    return input->rn < binding.rn_threshold ? LMMP_TUNE_NCR_BASECASE : binomial_large_path(input, binding.div_k);
}

/* 用不同的 n 直接比较两套分派，只测选择不同算法的输入。 */
static bool verify(unsigned candidate, binomial_tune_options current, const lmmp_tune_options* options) {
    static const uint verify_n[] = {3072, 5120, 16384, 49152, 98304, 786432, 1572864, 0x20000001};
    const binomial_tune_options tuned = {candidate, current.div_k};
    const binomial_tune_options deployed = {BINOMIAL_RN_BASECASE_THRESHOLD, BINOMIAL_DIV_K_THRESHOLD};
    const mp_size_t low = candidate < deployed.rn_threshold ? candidate : deployed.rn_threshold;
    const mp_size_t high = candidate > deployed.rn_threshold ? candidate : deployed.rn_threshold;
    const mp_size_t lengths[] = {low, (low + high) / 2, high, high + 16};
    unsigned measured = 0;
    for (unsigned i = 0; i < sizeof(verify_n) / sizeof(verify_n[0]); ++i) {
        const uint n = verify_n[i], min_r = n <= NCR_SHORT_LIMIT ? ODD_FACTORIAL_SIZE : 4;
        uint previous = 0;
        for (unsigned j = 0; j < sizeof(lengths) / sizeof(lengths[0]); ++j) {
            const uint r = binomial_r_for_rn(n, min_r, n / 2, lengths[j]);
            if (r == previous)
                continue;
            previous = r;
            const combin_tune_input input = combin_input(true, n, r);
            const lmmp_tune_combin_path old_path = binomial_path(&input, deployed);
            const lmmp_tune_combin_path new_path = binomial_path(&input, tuned);
            if (old_path == new_path)
                continue;
            if (n > MAX_FACTOR_N && (old_path == LMMP_TUNE_NCR_FACTOR || new_path == LMMP_TUNE_NCR_FACTOR))
                continue;
            const combin_tune_binding old = {old_path, deployed.rn_threshold, deployed.div_k};
            const combin_tune_binding next = {new_path, tuned.rn_threshold, tuned.div_k};
            combin_tune_times times;
            if (!combin_measure("verify", binomial_entry(), &input, old, next, options, &times))
                return false;
            measured += times.stable;
        }
    }
    printf("# verification stable=%u low=compiled_defaults high=joint_candidate\n", measured);
    return true;
}

int main(int argc, char** argv) {
    const char* program = "tune_numth_binomial_basecase";
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
        puts("  --div-k K          Select the div/factor path using K");
        puts("  --rn-threshold N   Compare the recommendation with N");
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_INFO) {
        printf("program=%s\n", program);
        puts("paths=basecase,deployed_div_or_factor\nprime_table=warm\ngrid=13 n columns, up to 23 rn per column");
        puts("ushort_n=512..65535\nuint_n=65536..4294967295\nuint_factor_n_max=2097152\nrn_targets=6..160");
        lmmp_tune_print_env();
    }
    if (options.action == LMMP_TUNE_INFO || options.action == LMMP_TUNE_CURRENT) {
        binomial_print_current(current);
        return EXIT_SUCCESS;
    }

    if (current.rn_threshold < FIRST_RN) {
        fprintf(stderr, "--rn-threshold requires an integer in [%u,256] for this grid\n", FIRST_RN);
        return EXIT_FAILURE;
    }
    const unsigned first = FIRST_RN;
    const unsigned last = current.rn_threshold > LAST_RN ? (unsigned)current.rn_threshold : LAST_RN;
    double scores[257] = {0}, by_group[3][257] = {{0}};
    unsigned counts[3] = {0}, stable = 0, unstable = 0;
    const char* groups[] = {"ushort_factor", "uint_div", "uint_factor"};
    printf("# current_rn=%" PRIu64 " div_k=%" PRIu64 "\n", current.rn_threshold, current.div_k);
    combin_start(program, &options);
    const double start = bench_clock();
    for (unsigned i = 0; i < sizeof(n_values) / sizeof(n_values[0]); ++i) {
        const uint n = n_values[i], min_r = n <= NCR_SHORT_LIMIT ? ODD_FACTORIAL_SIZE : 4;
        uint previous = 0;
        for (unsigned j = 0; j < sizeof(rn_levels) / sizeof(rn_levels[0]); ++j) {
            const uint r = binomial_r_for_rn(n, min_r, n / 2, rn_levels[j]);
            const combin_tune_input input = combin_input(true, n, r);
            if (r == previous || input.rn < rn_levels[j])
                continue;
            previous = r;
            const lmmp_tune_combin_path large = binomial_large_path(&input, current.div_k);
            if (n > MAX_FACTOR_N && large == LMMP_TUNE_NCR_FACTOR) {
                printf("# skipped n=%u r=%u rn=%" PRIu64 " factor exceeds grid bound\n", n, r, input.rn);
                continue;
            }
            const unsigned group = n <= NCR_SHORT_LIMIT ? 0 : (large == LMMP_TUNE_NCR_DIV ? 1 : 2);
            const combin_tune_binding basecase = {LMMP_TUNE_NCR_BASECASE, input.rn + 1, current.div_k};
            const combin_tune_binding other = {large, 0, current.div_k};
            combin_tune_times times;
            if (!combin_measure(groups[group], binomial_entry(), &input, basecase, other, &options, &times)) {
                lmmp_global_deinit();
                return EXIT_FAILURE;
            }
            if (!times.stable) {
                ++unstable;
                continue;
            }
            ++stable;
            ++counts[group];
            for (unsigned candidate = first; candidate <= last; ++candidate) {
                const double loss = combin_loss(times, input.rn < candidate);
                scores[candidate] += loss;
                by_group[group][candidate] += loss;
            }
        }
    }
    unsigned best = (unsigned)current.rn_threshold;
    for (unsigned candidate = first; candidate <= last; ++candidate) {
        if (scores[candidate] < scores[best])
            best = candidate;
    }
    for (unsigned g = 0; g < sizeof(counts) / sizeof(counts[0]); ++g) {
        if (counts[g] == 0)
            continue;
        unsigned local = best;
        for (unsigned candidate = first; candidate <= last; ++candidate) {
            if (by_group[g][candidate] < by_group[g][local])
                local = candidate;
        }
        printf("# group=%s stable=%u group_best=%u group_loss_pct=%.6f shared_loss_pct=%.6f\n", groups[g], counts[g],
               local, 100 * by_group[g][local] / counts[g], 100 * by_group[g][best] / counts[g]);
    }
    const bool eligible = stable != 0 && best > first && best < last;
    const bool verified = eligible && verify(best, current, &options);
    const bool ok = (verified || !eligible) &&
                    combin_recommend("BINOMIAL_RN_BASECASE_THRESHOLD", scores, first, last,
                                     (unsigned)current.rn_threshold, stable);
    printf("# elapsed_s=%.3f unstable=%u\n", bench_clock() - start, unstable);
    lmmp_global_deinit();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
