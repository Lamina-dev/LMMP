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

#ifndef LMMP_NUMTH_COMBIN_TUNE_H
#define LMMP_NUMTH_COMBIN_TUNE_H

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/kernels.h"
#include "tune/params.h"

typedef struct {
    uint n, r;
    mp_size_t capacity, rn;
    mp_bitcnt_t bits;
} combin_tune_input;

typedef struct {
    bool binomial;
    mp_size_t* first;
    mp_size_t* second;
} combin_tune_entry;

typedef struct {
    lmmp_tune_combin_path branch;
    mp_size_t first, second;
} combin_tune_binding;

typedef struct {
    const combin_tune_entry* entry;
    const combin_tune_input* input;
    combin_tune_binding binding;
    mp_ptr dst;
    mp_limb_t calls;
} combin_tune_bench;

typedef struct {
    double low_us, high_us;
    bool stable;
} combin_tune_times;

typedef struct {
    mp_size_t rn_threshold, div_k;
} binomial_tune_options;

static inline combin_tune_input combin_input(bool binomial, uint n, uint r) {
    combin_tune_input input = {.n = n, .r = r};
    input.capacity = binomial ? lmmp_nCr_size_(n, r, &input.bits) : lmmp_nPr_size_(n, r, &input.bits);
    input.rn = input.capacity - input.bits / LIMB_BITS;
    return input;
}

static inline void combin_bind(void* opaque) {
    combin_tune_bench* bench = opaque;
    *bench->entry->first = bench->binding.first;
    *bench->entry->second = bench->binding.second;
}

static inline void combin_reset(void* opaque) {
    (void)opaque;
}

static inline mp_size_t combin_call(combin_tune_bench* bench) {
    const combin_tune_input* input = bench->input;
    ++bench->calls;
    if (bench->entry->binomial)
        return lmmp_nCr_(bench->dst, input->bits, input->capacity, input->n, input->r);
    return lmmp_nPr_(bench->dst, input->bits, input->capacity, input->n, input->r);
}

static inline mp_limb_t combin_bench_call(void* opaque) {
    combin_tune_bench* bench = opaque;
    mp_size_t rn = combin_call(bench);
    return rn ^ bench->dst[0];
}

static inline bool combin_check_hits(const combin_tune_bench* low, const combin_tune_bench* high) {
    return lmmp_tune_combin_hits[low->binding.branch] == low->calls &&
           lmmp_tune_combin_hits[high->binding.branch] == high->calls;
}

/* 先经完整入口核对两路，首次质数表初始化发生在计时前。 */
static inline bool combin_measure(const char* group,
                                  combin_tune_entry entry,
                                  const combin_tune_input* input,
                                  combin_tune_binding low,
                                  combin_tune_binding high,
                                  const lmmp_tune_options* options,
                                  combin_tune_times* times) {
    const size_t stride = input->capacity + 1;
    const mp_limb_t canary = UINT64_C(0x9c52b918af7d36e3);
    mp_ptr dst = malloc(2 * stride * sizeof(*dst));
    if (dst == NULL) {
        fprintf(stderr, "combin tune: allocation failed for n=%u r=%u\n", input->n, input->r);
        return false;
    }
    dst[stride - 1] = dst[2 * stride - 1] = canary;
    combin_tune_bench a = {&entry, input, low, dst, 0};
    combin_tune_bench b = {&entry, input, high, dst + stride, 0};
    const mp_size_t first = *entry.first, second = *entry.second;
    memset(lmmp_tune_combin_hits, 0, sizeof(lmmp_tune_combin_hits));
    combin_bind(&a);
    const mp_size_t an = combin_call(&a);
    const bool low_ok = lmmp_tune_combin_hits[low.branch] == 1 && lmmp_tune_combin_hits[high.branch] == 0;
    combin_bind(&b);
    const mp_size_t bn = combin_call(&b);
    bool ok = low_ok && combin_check_hits(&a, &b) && an != 0 && an == bn && an <= input->capacity &&
              dst[stride - 1] == canary && dst[2 * stride - 1] == canary && lmmp_cmp_(a.dst, b.dst, an) == 0;

    double a_mad, b_mad;
    unsigned retries;
    if (ok) {
        const lmmp_tune_bench low_bench = {
            .bind = combin_bind, .reset = combin_reset, .call = combin_bench_call, .context = &a};
        const lmmp_tune_bench high_bench = {
            .bind = combin_bind, .reset = combin_reset, .call = combin_bench_call, .context = &b};
        ok = lmmp_tune_measure_pair(&low_bench, &high_bench, options, &times->low_us, &a_mad, &times->high_us,
                                    &b_mad, &retries, &times->stable) &&
             combin_check_hits(&a, &b) && dst[stride - 1] == canary && dst[2 * stride - 1] == canary &&
             times->low_us > 0 && times->high_us > 0;
    }
    *entry.first = first;
    *entry.second = second;
    free(dst);
    if (!ok) {
        fprintf(stderr,
                "combin tune: %s check failed at n=%u r=%u rn=%" PRIu64
                " (lengths %" PRIu64 "/%" PRIu64 ", hits/calls %" PRIu64 "/%" PRIu64 " %" PRIu64 "/%" PRIu64 ")\n",
                group, input->n, input->r, input->rn, an, bn, lmmp_tune_combin_hits[low.branch], a.calls,
                lmmp_tune_combin_hits[high.branch], b.calls);
        return false;
    }
    printf("%s,%u,%u,%" PRIu64 ",%.6f,%.6f,%.6f,%.6f,%.6f,%u,%d\n", group, input->n, input->r, input->rn,
           times->low_us, times->high_us, times->low_us / times->high_us, a_mad, b_mad, retries, times->stable);
    return true;
}

static inline double combin_loss(combin_tune_times times, bool low) {
    return (low ? times.low_us : times.high_us) / fmin(times.low_us, times.high_us) - 1.0;
}

static inline void combin_start(const char* program, const lmmp_tune_options* options) {
    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    puts("# binding_mode=runtime-branch, prime_table=warm, unstable_points=excluded");
    printf("# program=%s samples=%u min_ms=%.3f mad_limit=%.4f max_retry=%u\n", program, options->samples,
           options->min_ms, options->mad_limit, options->max_retry);
    lmmp_tune_print_env_comment();
    puts("group,n,r,rn,low_us,high_us,ratio,low_mad,high_mad,retries,stable");
}

static inline bool combin_recommend(const char* parameter,
                                    const double* scores,
                                    unsigned first,
                                    unsigned last,
                                    unsigned current,
                                    unsigned count) {
    if (count == 0) {
        fprintf(stderr, "%s: no stable points\n", parameter);
        return false;
    }
    unsigned best = current;
    for (unsigned candidate = first; candidate <= last; ++candidate) {
        printf("# score %s candidate=%u loss_pct=%.6f\n", parameter, candidate, 100 * scores[candidate] / count);
        if (scores[candidate] < scores[best])
            best = candidate;
    }
    unsigned flat_low = best, flat_high = best;
    const double limit = scores[best] + 0.001 * count;
    while (flat_low > first && scores[flat_low - 1] <= limit)
        --flat_low;
    while (flat_high < last && scores[flat_high + 1] <= limit)
        ++flat_high;
    printf("# %s stable=%u best_loss_pct=%.6f current_loss_pct=%.6f flat_plus_0.1pp=%u..%u\n", parameter, count,
           100 * scores[best] / count, 100 * scores[current] / count, flat_low, flat_high);
    if (best == first || best == last) {
        printf("# %s minimum_reaches_candidate_boundary=%u\n", parameter, best);
        return false;
    }
    printf("recommended %s = %u\n", parameter, best);
    return true;
}

static inline binomial_tune_options binomial_current(void) {
    return (binomial_tune_options){lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD, lmmp_tune_BINOMIAL_DIV_K_THRESHOLD};
}

static inline bool binomial_option(void* opaque, const char* option, const char* value) {
    binomial_tune_options* options = opaque;
    if (strcmp(option, "--rn-threshold") != 0 && strcmp(option, "--div-k") != 0) {
        fprintf(stderr, "unknown option '%s'\n", option);
        return false;
    }
    errno = 0;
    char* end;
    unsigned long v = strtoul(value, &end, 10);
    if (errno != 0 || *value < '0' || *value > '9' || *end != '\0' || v < 1 || v > 256) {
        fprintf(stderr, "%s requires an integer in [1,256]\n", option);
        return false;
    }
    if (strcmp(option, "--rn-threshold") == 0)
        options->rn_threshold = v;
    else
        options->div_k = v;
    return true;
}

static inline void binomial_print_current(binomial_tune_options current) {
    lmmp_tune_print_current("BINOMIAL_RN_BASECASE_THRESHOLD", BINOMIAL_RN_BASECASE_THRESHOLD, current.rn_threshold);
    lmmp_tune_print_current("BINOMIAL_DIV_K_THRESHOLD", BINOMIAL_DIV_K_THRESHOLD, current.div_k);
}

static inline uint binomial_r_for_rn(uint n, uint low, uint high, mp_size_t rn) {
    while (low < high) {
        const uint mid = low + (high - low) / 2;
        if (combin_input(true, n, mid).rn < rn)
            low = mid + 1;
        else
            high = mid;
    }
    return low;
}

static inline void binomial_div_lengths(uint n, uint r, mp_size_t* nPr_n, mp_size_t* fac_n) {
    mp_bitcnt_t bits;
    *nPr_n = lmmp_nPr_size_(n, r, &bits);
    *fac_n = lmmp_factorial_size_(r, &bits);
}

static inline combin_tune_entry binomial_entry(void) {
    return (combin_tune_entry){true, &lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD, &lmmp_tune_BINOMIAL_DIV_K_THRESHOLD};
}

#endif
