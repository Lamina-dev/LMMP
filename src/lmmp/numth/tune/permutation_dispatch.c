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

/* 固定 n，先扫 r，再在交叉点两侧补点。拟合 r = (n + B) / K，最后按整数分派式比较候选。 */
enum { MAX_COLUMNS = 12, MAX_COLUMN_POINTS = 10 };

typedef struct {
    const char* group;
    const char* k_name;
    const char* b_name;
    mp_size_t default_k, default_b;
    combin_tune_entry entry;
    const uint* n;
    unsigned columns;
    const uint* verify_n;
    unsigned verify_columns;
} permutation_grid;

static const uint short_n[] = {4096, 6144, 8192, 12288, 16384, 24576, 32768, 40960, 49152, 57344, 65535};
static const uint int_n[] = {65536, 98304, 131072, 196608, 262144, 393216,
                             524288, 786432, 1048576, 1572864, 2097152, 3145728};
static const uint short_verify_n[] = {5120, 10240, 14336, 20480, 28672, 36864, 53248, 61440};
static const uint int_verify_n[] = {81920, 163840, 327680, 655360, 1310720, 1835008, 2621440, 4194304};

_Static_assert(sizeof(short_n) / sizeof(short_n[0]) <= MAX_COLUMNS, "ushort grid exceeds column storage");
_Static_assert(sizeof(int_n) / sizeof(int_n[0]) <= MAX_COLUMNS, "uint grid exceeds column storage");

typedef struct {
    uint n, r;
    combin_tune_times times;
} permutation_point;

typedef struct {
    permutation_point points[MAX_COLUMN_POINTS];
    unsigned count;
    double crossing;
} permutation_column;

static bool measure_point(const permutation_grid* grid,
                          uint n,
                          uint r,
                          const lmmp_tune_options* options,
                          permutation_column* column) {
    unsigned pos = 0;
    while (pos < column->count && column->points[pos].r < r)
        ++pos;
    if (pos < column->count && column->points[pos].r == r)
        return true;
    const combin_tune_input input = combin_input(false, n, r);
    const combin_tune_binding product = {LMMP_TUNE_NPR_PRODUCT, 0, 0};
    const combin_tune_binding factor = {LMMP_TUNE_NPR_FACTOR, (mp_size_t)n + 1, 0};
    permutation_point point = {.n = n, .r = r};
    if (!combin_measure(grid->group, grid->entry, &input, product, factor, options, &point.times))
        return false;
    memmove(column->points + pos + 1, column->points + pos, (column->count - pos) * sizeof(point));
    column->points[pos] = point;
    ++column->count;
    return true;
}

static double log_ratio(const permutation_point* point) {
    return log(point->times.low_us / point->times.high_us);
}

static bool find_bracket(const permutation_column* column, permutation_point* low, permutation_point* high) {
    /* 相邻稳定点须由累乘较快过渡到因子分解较快。 */
    const permutation_point* previous = NULL;
    for (unsigned i = 0; i < column->count; ++i) {
        const permutation_point* point = &column->points[i];
        if (!point->times.stable)
            continue;
        if (previous && log_ratio(previous) <= 0 && log_ratio(point) >= 0) {
            *low = *previous;
            *high = *point;
            return true;
        }
        previous = point;
    }
    return false;
}

static double interpolate(const permutation_point* low, const permutation_point* high) {
    const double a = log_ratio(low), b = log_ratio(high);
    return a == b ? 0.5 : -a / (b - a);
}

/* 打散按旧分界线成比例的采样位置，避免同一条候选线与整档网格重合。 */
static double grid_offset(uint n, unsigned level, uint64_t seed) {
    uint64_t x = (seed ^ n) + UINT64_C(0x9e3779b97f4a7c15) * (level + 1);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    return ((int)((x ^ (x >> 31)) % 201) - 100) / 1000.0;
}

static bool measure_column(const permutation_grid* grid,
                           uint n,
                           const lmmp_tune_options* options,
                           permutation_column* column) {
    const double center = (double)(n + *grid->entry.second) / (double)*grid->entry.first;
    const double coarse[] = {1.0 / 3, 2.0 / 3, 1.0, 1.5, 3.0};
    for (unsigned i = 0; i < sizeof(coarse) / sizeof(coarse[0]); ++i) {
        const uint r = (uint)fmin(n, fmax(11, center * coarse[i] * (1 + grid_offset(n, i, options->seed))));
        if (!measure_point(grid, n, r, options, column))
            return false;
    }

    permutation_point low, high;
    if (!find_bracket(column, &low, &high)) {
        const uint first = (uint)fmin(n, fmax(11, center / 6));
        const uint last = (uint)fmin(n, fmax(11, center * 6));
        if (!measure_point(grid, n, first, options, column) || !measure_point(grid, n, last, options, column))
            return false;
    }
    for (unsigned i = 0; i < 2 && find_bracket(column, &low, &high) && high.r > low.r + 1; ++i) {
        const double fraction = fmax(0.25, fmin(0.75, interpolate(&low, &high)));
        const uint r = low.r + (uint)fmax(1, fraction * (high.r - low.r));
        if (!measure_point(grid, n, r, options, column))
            return false;
    }
    if (find_bracket(column, &low, &high)) {
        column->crossing = low.r + interpolate(&low, &high) * (high.r - low.r);
        printf("# crossing group=%s n=%u r=%.2f bracket=%u..%u\n", grid->group, n, column->crossing, low.r, high.r);
    } else {
        printf("# crossing group=%s n=%u unresolved\n", grid->group, n);
    }
    return true;
}

static bool fit_crossings(const permutation_grid* grid, const permutation_column* columns, double* k, double* b) {
    unsigned count = 0;
    double x = 0, y = 0, xx = 0, xy = 0;
    for (unsigned i = 0; i < grid->columns; ++i) {
        const double n = grid->n[i], r = columns[i].crossing;
        if (r == 0)
            continue;
        ++count;
        x += n;
        y += r;
        xx += n * n;
        xy += n * r;
    }
    const double slope = count < 3 ? 0 : (count * xy - x * y) / (count * xx - x * x);
    if (slope <= 0)
        return false;
    *k = 1 / slope;
    *b = (y * *k - x) / count;
    double squared_error = 0;
    for (unsigned i = 0; i < grid->columns; ++i) {
        if (columns[i].crossing > 0) {
            const double error = (grid->n[i] + *b) / *k - columns[i].crossing;
            squared_error += error * error;
        }
    }
    printf("# fit group=%s crossings=%u K=%.4f B=%.2f r_rmse=%.2f\n", grid->group, count, *k, *b,
           sqrt(squared_error / count));
    return true;
}

static double score(const permutation_grid* grid, const permutation_column* columns, mp_size_t k, mp_size_t b) {
    double loss = 0;
    for (unsigned i = 0; i < grid->columns; ++i) {
        for (unsigned j = 0; j < columns[i].count; ++j) {
            const permutation_point* point = &columns[i].points[j];
            if (point->times.stable)
                loss += combin_loss(point->times, point->n + b > point->r * k);
        }
    }
    return loss;
}

typedef struct {
    mp_size_t k, b;
    double loss, distance;
} permutation_pick;

static void consider(const permutation_grid* grid,
                     const permutation_column* columns,
                     mp_size_t k,
                     mp_size_t b,
                     double fit_k,
                     double fit_b,
                     permutation_pick* pick) {
    const double loss = score(grid, columns, k, b);
    /* 同分时取更接近实测交叉线的一对。 */
    const double distance = fabs((double)k / fit_k - 1) + fabs((double)b - fit_b) / fmax(fabs(fit_b), 1);
    if (loss < pick->loss || (loss == pick->loss && distance < pick->distance))
        *pick = (permutation_pick){k, b, loss, distance};
}

static bool verify(const permutation_grid* grid, permutation_pick pick, const lmmp_tune_options* options) {
    const mp_size_t current_k = *grid->entry.first, current_b = *grid->entry.second;
    unsigned stable = 0, faster = 0;
    for (unsigned i = 0; i < grid->verify_columns; ++i) {
        const uint n = grid->verify_n[i];
        const mp_size_t current_r = (n + current_b + current_k - 1) / current_k;
        const mp_size_t tuned_r = (n + pick.b + pick.k - 1) / pick.k;
        const mp_size_t mid = (current_r + tuned_r) / 2;
        const uint r = mid > n ? n : (uint)mid;
        const bool product_current = r < current_r, product_tuned = r < tuned_r;
        if (r <= 10 || product_current == product_tuned)
            continue;
        const combin_tune_input input = combin_input(false, n, r);
        const combin_tune_binding current = {
            product_current ? LMMP_TUNE_NPR_PRODUCT : LMMP_TUNE_NPR_FACTOR, current_k, current_b};
        const combin_tune_binding tuned = {
            product_tuned ? LMMP_TUNE_NPR_PRODUCT : LMMP_TUNE_NPR_FACTOR, pick.k, pick.b};
        combin_tune_times times;
        if (!combin_measure("verify", grid->entry, &input, current, tuned, options, &times))
            return false;
        stable += times.stable;
        faster += times.stable && times.high_us < times.low_us;
    }
    printf("# verification group=%s stable=%u candidate_faster=%u low=compiled_defaults high=joint_candidate\n",
           grid->group, stable, faster);
    return true;
}

static bool recommend(const permutation_grid* grid,
                      const permutation_column* columns,
                      const lmmp_tune_options* options) {
    double fit_k, fit_b;
    if (!fit_crossings(grid, columns, &fit_k, &fit_b) || fit_k > 1024) {
        fprintf(stderr, "%s: insufficient crossings for a positive slope in the candidate range\n", grid->group);
        return false;
    }

    unsigned stable = 0, total = 0;
    for (unsigned i = 0; i < grid->columns; ++i) {
        total += columns[i].count;
        for (unsigned j = 0; j < columns[i].count; ++j)
            stable += columns[i].points[j].times.stable;
    }
    const mp_size_t current_k = *grid->entry.first, current_b = *grid->entry.second;
    const unsigned first_k = (unsigned)fmax(1, fmin(fit_k, (double)current_k) / 2);
    const unsigned last_k = (unsigned)ceil(2 * fmax(fit_k, (double)current_k));
    const mp_size_t rounded_k = (mp_size_t)fmax(1, round(fit_k));
    const mp_size_t rounded_b = (mp_size_t)fmax(0, round(fit_b));
    permutation_pick pick = {0, 0, INFINITY, INFINITY};
    for (unsigned k = first_k; k <= last_k; ++k) {
        consider(grid, columns, k, 0, fit_k, fit_b, &pick);
        consider(grid, columns, k, current_b, fit_k, fit_b, &pick);
        consider(grid, columns, k, rounded_b, fit_k, fit_b, &pick);
        /* 固定 K 时，B 只在 r*K - n 与 r*K - n + 1 之间改变某个输入的分派。 */
        for (unsigned i = 0; i < grid->columns; ++i) {
            for (unsigned j = 0; j < columns[i].count; ++j) {
                const permutation_point* point = &columns[i].points[j];
                const mp_size_t rk = (mp_size_t)point->r * k;
                if (point->times.stable && rk >= point->n) {
                    consider(grid, columns, k, rk - point->n, fit_k, fit_b, &pick);
                    consider(grid, columns, k, rk - point->n + 1, fit_k, fit_b, &pick);
                }
            }
        }
    }
    printf("# score group=%s points=%u stable=%u K_range=%u..%u current_loss_pct=%.6f fit_loss_pct=%.6f "
           "best_loss_pct=%.6f\n", grid->group, total, stable, first_k, last_k,
           100 * score(grid, columns, current_k, current_b) / stable,
           100 * score(grid, columns, rounded_k, rounded_b) / stable, 100 * pick.loss / stable);
    if (pick.k == first_k || pick.k == last_k) {
        printf("# %s minimum_reaches_candidate_boundary=%" PRIu64 "\n", grid->group, pick.k);
        return false;
    }
    if (!verify(grid, pick, options))
        return false;
    printf("recommended %s = %" PRIu64 "\n", grid->k_name, pick.k);
    printf("recommended %s = %" PRIu64 "\n", grid->b_name, pick.b);
    return true;
}

int main(int argc, char** argv) {
    const char* program = "tune_numth_permutation_dispatch";
    const permutation_grid grids[] = {
        {"ushort", "PERMUTATION_USHORT_K_THRESHOLD", "PERMUTATION_USHORT_B_THRESHOLD",
         PERMUTATION_USHORT_K_THRESHOLD, PERMUTATION_USHORT_B_THRESHOLD,
         {false, &lmmp_tune_PERMUTATION_USHORT_K_THRESHOLD, &lmmp_tune_PERMUTATION_USHORT_B_THRESHOLD},
         short_n, sizeof(short_n) / sizeof(short_n[0]), short_verify_n, sizeof(short_verify_n) / sizeof(short_verify_n[0])},
        {"uint", "PERMUTATION_UINT_K_THRESHOLD", "PERMUTATION_UINT_B_THRESHOLD",
         PERMUTATION_UINT_K_THRESHOLD, PERMUTATION_UINT_B_THRESHOLD,
         {false, &lmmp_tune_PERMUTATION_UINT_K_THRESHOLD, &lmmp_tune_PERMUTATION_UINT_B_THRESHOLD},
         int_n, sizeof(int_n) / sizeof(int_n[0]), int_verify_n, sizeof(int_verify_n) / sizeof(int_verify_n[0])},
    };
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    options.samples = 2;
    options.min_ms = 1.0;
    options.max_retry = 1;
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage(program);
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_INFO) {
        printf("program=%s\n", program);
        puts("paths=product,factor\nprime_table=warm\nushort_n=4096..65535\nuint_n=65536..3145728");
        puts("grid=5 seed-jittered coarse r per n, at most 2 outer and 2 crossing points per column");
        puts("verification=up to 8 additional n per width");
        lmmp_tune_print_env();
    }
    if (options.action == LMMP_TUNE_INFO || options.action == LMMP_TUNE_CURRENT) {
        for (unsigned i = 0; i < sizeof(grids) / sizeof(grids[0]); ++i) {
            const permutation_grid* grid = &grids[i];
            lmmp_tune_print_current(grid->k_name, grid->default_k, *grid->entry.first);
            lmmp_tune_print_current(grid->b_name, grid->default_b, *grid->entry.second);
        }
        return EXIT_SUCCESS;
    }

    printf("# seed=%llu\n", options.seed);
    combin_start(program, &options);
    const double start = bench_clock();
    bool ok = true;
    for (unsigned i = 0; i < sizeof(grids) / sizeof(grids[0]); ++i) {
        const permutation_grid* grid = &grids[i];
        permutation_column columns[MAX_COLUMNS] = {0};
        for (unsigned j = 0; j < grid->columns; ++j) {
            if (!measure_column(grid, grid->n[j], &options, &columns[j])) {
                lmmp_global_deinit();
                return EXIT_FAILURE;
            }
        }
        ok = recommend(grid, columns, &options) && ok;
    }
    printf("# elapsed_s=%.3f\n", bench_clock() - start);
    lmmp_global_deinit();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
