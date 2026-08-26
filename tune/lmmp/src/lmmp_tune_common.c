/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "lmmp_tune_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

/* ============================== 全局配置 ============================== */

tune_options_t g_tune = {
    7,     /* samples */
    10.0,  /* target_ms */
    0.25,  /* mad_limit */
    1,     /* max_retry */
    0,     /* write */
    NULL,  /* only */
    NULL   /* out_path */
};

/* ============================== 计时 ============================== */

static double g_tick_ns = 1.0;

void tune_timer_init(void) {
#ifdef _WIN32
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq) && freq.QuadPart > 0)
        g_tick_ns = 1e9 / (double)freq.QuadPart;
#else
    (void)g_tick_ns;
#endif
}

double tune_now_ns(void) {
#ifdef _WIN32
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * g_tick_ns;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
#endif
}

/* ============================== 测量 ============================== */

#define TUNE_MAX_LOOPS ((1ull << 28) - 1)

static unsigned tune_calibrate_prepared(tune_bench_fn fn, void* ctx,
                                        tune_prepare_fn prep, void* prep_ctx,
                                        double target_ms) {
    const double target_ns = target_ms * 1e6;
    unsigned long long loops = 1;

    /* 每个计时批次开始前重新绑定路径。首个调用同时充当 warm-up。 */
    if (prep != NULL)
        prep(prep_ctx);
    (void)fn(ctx);

    for (;;) {
        if (prep != NULL)
            prep(prep_ctx);
        const double t0 = tune_now_ns();
        for (unsigned long long i = 0; i < loops; ++i)
            (void)fn(ctx);
        const double elapsed = tune_now_ns() - t0;

        if (elapsed >= target_ns || loops >= TUNE_MAX_LOOPS)
            break;

        if (elapsed > 0.0) {
            const double need = (target_ns / elapsed) * (double)loops;
            if (need > (double)TUNE_MAX_LOOPS)
                loops = TUNE_MAX_LOOPS;
            else if (need > (double)loops + 1.0)
                loops = (unsigned long long)need;
            else
                loops <<= 1;
        } else {
            loops <<= 1;
        }
        if (loops == 0)
            loops = 1;
    }
    return (unsigned)loops;
}

unsigned tune_calibrate(tune_bench_fn fn, void* ctx, double target_ms) {
    return tune_calibrate_prepared(fn, ctx, NULL, NULL, target_ms);
}

static int compare_double(const void* a, const void* b) {
    const double x = *(const double*)a;
    const double y = *(const double*)b;
    return (x > y) - (x < y);
}

static double median_sorted(const double* v, size_t n) {
    return n == 0 ? 0.0 : v[n / 2];
}

static double mad_sorted(const double* v, size_t n, double med) {
    if (n == 0) return 0.0;
    double* d = (double*)malloc(n * sizeof(double));
    if (d == NULL) return 0.0;
    for (size_t i = 0; i < n; ++i)
        d[i] = fabs(v[i] - med);
    qsort(d, n, sizeof(double), compare_double);
    const double r = d[n / 2];
    free(d);
    return r;
}

static void fill_stats(tune_measure_t* out, const double* times, size_t n,
                       unsigned loops, unsigned retries) {
    out->loops = loops;
    out->samples = (unsigned)n;
    out->retries = retries;
    if (n == 0) {
        out->median_ns = out->min_ns = out->max_ns = out->mad_ns = 0.0;
        return;
    }
    out->median_ns = median_sorted(times, n);
    out->min_ns = times[0];
    out->max_ns = times[n - 1];
    out->mad_ns = mad_sorted(times, n, out->median_ns);
}

static int unstable(const tune_measure_t* m) {
    if (m->median_ns <= 0.0 || m->samples < 5)
        return 0;
    return m->mad_ns / m->median_ns > g_tune.mad_limit;
}

static tune_measure_t measure_with_loops_prepared(tune_bench_fn fn,
                                                  tune_prepare_fn prep,
                                                  void* ctx,
                                                  unsigned loops,
                                                  unsigned samples) {
    tune_measure_t result;
    memset(&result, 0, sizeof(result));

    if (samples == 0)
        return result;

    double* times = (double*)malloc((size_t)samples * sizeof(double));
    if (times == NULL) {
        result.median_ns = 1e300;
        return result;
    }

    for (unsigned attempt = 0; attempt <= g_tune.max_retry; ++attempt) {
        for (unsigned s = 0; s < samples; ++s) {
            if (prep != NULL)
                prep(ctx);
            const double t0 = tune_now_ns();
            for (unsigned i = 0; i < loops; ++i)
                (void)fn(ctx);
            times[s] = (tune_now_ns() - t0) / (double)loops;
        }
        qsort(times, samples, sizeof(double), compare_double);
        fill_stats(&result, times, samples, loops, attempt);
        if (!unstable(&result))
            break;
    }

    free(times);
    return result;
}

tune_measure_t tune_measure(tune_bench_fn fn, void* ctx,
                            unsigned samples, double target_ms) {
    if (samples == 0)
        samples = 1;
    samples |= 1u; /* 保持奇数，使中位数直接取有序数组中间项 */
    const unsigned loops = tune_calibrate(fn, ctx, target_ms);
    return measure_with_loops_prepared(fn, NULL, ctx, loops, samples);
}

static void tune_measure_pair_prepared(tune_bench_fn fn_low, void* ctx_low,
                                       tune_prepare_fn prep_low, void* prep_ctx_low,
                                       tune_bench_fn fn_high, void* ctx_high,
                                       tune_prepare_fn prep_high, void* prep_ctx_high,
                                       unsigned samples, double target_ms,
                                       tune_measure_t* low, tune_measure_t* high) {
    memset(low, 0, sizeof(*low));
    memset(high, 0, sizeof(*high));
    if (samples == 0)
        samples = 1;
    samples |= 1u;

    const unsigned loops_low = tune_calibrate_prepared(fn_low, ctx_low,
                                                       prep_low, prep_ctx_low,
                                                       target_ms);
    const unsigned loops_high = tune_calibrate_prepared(fn_high, ctx_high,
                                                        prep_high, prep_ctx_high,
                                                        target_ms);

    double* tl = (double*)malloc((size_t)samples * sizeof(double));
    double* th = (double*)malloc((size_t)samples * sizeof(double));
    if (tl == NULL || th == NULL) {
        free(tl);
        free(th);
        low->median_ns = high->median_ns = 1e300;
        return;
    }

    for (unsigned attempt = 0; attempt <= g_tune.max_retry; ++attempt) {
        for (unsigned s = 0; s < samples; ++s) {
            if (prep_low != NULL)
                prep_low(prep_ctx_low);
            double t0 = tune_now_ns();
            for (unsigned i = 0; i < loops_low; ++i)
                (void)fn_low(ctx_low);
            tl[s] = (tune_now_ns() - t0) / (double)loops_low;

            if (prep_high != NULL)
                prep_high(prep_ctx_high);
            t0 = tune_now_ns();
            for (unsigned i = 0; i < loops_high; ++i)
                (void)fn_high(ctx_high);
            th[s] = (tune_now_ns() - t0) / (double)loops_high;
        }

        qsort(tl, samples, sizeof(double), compare_double);
        qsort(th, samples, sizeof(double), compare_double);
        fill_stats(low, tl, samples, loops_low, attempt);
        fill_stats(high, th, samples, loops_high, attempt);
        if (!unstable(low) && !unstable(high))
            break;
    }

    free(tl);
    free(th);
}

void tune_measure_pair(tune_bench_fn fn_low, void* ctx_low,
                       tune_bench_fn fn_high, void* ctx_high,
                       unsigned samples, double target_ms,
                       tune_measure_t* low, tune_measure_t* high) {
    tune_measure_pair_prepared(fn_low, ctx_low, NULL, NULL,
                               fn_high, ctx_high, NULL, NULL,
                               samples, target_ms, low, high);
}

/* ============================== 样本点生成 ============================== */

static void point_push(uint64_t* out, size_t* n, size_t cap, uint64_t v) {
    for (size_t i = 0; i < *n; ++i)
        if (out[i] == v)
            return;
    if (*n < cap)
        out[(*n)++] = v;
}

static int compare_u64(const void* a, const void* b) {
    const uint64_t x = *(const uint64_t*)a;
    const uint64_t y = *(const uint64_t*)b;
    return (x > y) - (x < y);
}

size_t tune_build_points(uint64_t lo, uint64_t hi, uint64_t hint,
                         uint64_t* out, size_t cap) {
    size_t n = 0;
    if (hi < lo) hi = lo;
    if (hint < lo) hint = lo;
    if (hint > hi) hint = hi;
    if (cap < 2) return 0;

    if (hi - lo <= 255) {
        for (uint64_t v = lo; v <= hi && n < cap; ++v)
            out[n++] = v;
    } else {
        const double ratio = 1.06;
        double v = lo == 0 ? 1.0 : (double)lo;
        point_push(out, &n, cap, lo);
        while ((uint64_t)(v + 0.5) < hi && n + 16 < cap) {
            point_push(out, &n, cap, (uint64_t)(v + 0.5));
            v *= ratio;
            if (v > (double)hi)
                v = (double)hi;
        }
        point_push(out, &n, cap, hi);

        /* 在 hint 附近额外加密，交叉点通常距离旧默认值不远。 */
        static const int64_t deltas[] = {0, 1, -1, 2, -2, 4, -4, 8, -8, 16, -16};
        for (size_t i = 0; i < sizeof(deltas) / sizeof(deltas[0]); ++i) {
            const int64_t d = deltas[i];
            if ((d < 0 && hint < (uint64_t)(-d)) || (d > 0 && hint > hi - (uint64_t)d))
                continue;
            point_push(out, &n, cap, (uint64_t)((int64_t)hint + d));
        }
    }

    qsort(out, n, sizeof(uint64_t), compare_u64);
    return n;
}

/* ============================== 1D badness 搜索 ============================== */

static double point_badness(const tune_point_t* p, int use_high) {
    const double chosen = use_high ? p->high_ns : p->low_ns;
    const double other = use_high ? p->low_ns : p->high_ns;
    const double faster = chosen < other ? chosen : other;
    if (faster <= 0.0 || chosen <= 0.0)
        return 0.0;
    return chosen / faster - 1.0;
}

static int uses_high(tune_pred_t pred, uint64_t size, uint64_t threshold) {
    return pred == TUNE_HIGH_WHEN_GT ? size > threshold : size >= threshold;
}

tune_choice_t tune_choose_1d(const tune_point_t* points, size_t npoints,
                             uint64_t lo, uint64_t hi, uint64_t hint,
                             tune_pred_t pred) {
    tune_choice_t result;
    memset(&result, 0, sizeof(result));
    result.threshold = hint;
    result.plateau_lo = lo;
    result.plateau_hi = hi;
    result.margin = 0.0;
    if (npoints == 0 || hi < lo) {
        result.badness = 1e300;
        result.total_ns = 1e300;
        return result;
    }
    if (hint < lo) hint = lo;
    if (hint > hi) hint = hi;

    double best_badness = 1e300;
    for (uint64_t t = lo; t <= hi; ++t) {
        double badness = 0.0;
        for (size_t i = 0; i < npoints; ++i) {
            const int use_high = uses_high(pred, points[i].size, t);
            badness += point_badness(&points[i], use_high);
        }
        if (badness < best_badness)
            best_badness = badness;
    }

    /* 平坦区间判定。容差既覆盖 0 badness，也允许极小浮点漂移。 */
    const double tol = 0.01 * (best_badness > 1.0 ? best_badness : 1.0) + 1e-12;
    uint64_t plateau_lo = hi;
    uint64_t plateau_hi = lo;
    for (uint64_t t = lo; t <= hi; ++t) {
        double badness = 0.0;
        for (size_t i = 0; i < npoints; ++i)
            badness += point_badness(&points[i], uses_high(pred, points[i].size, t));
        if (badness <= best_badness + tol) {
            if (t < plateau_lo) plateau_lo = t;
            if (t > plateau_hi) plateau_hi = t;
        }
    }

    /* 平台期优先靠近先验值，避免纯噪声在平坦区域里随机游走。 */
    uint64_t best = hint;
    if (hint >= plateau_lo && hint <= plateau_hi) {
        best = hint;
    } else {
        uint64_t best_dist = UINT64_MAX;
        for (uint64_t t = plateau_lo; t <= plateau_hi; ++t) {
            const uint64_t dist = t > hint ? t - hint : hint - t;
            if (dist < best_dist) {
                best_dist = dist;
                best = t;
            }
        }
    }

    double second_badness = 1e300;
    for (uint64_t t = lo; t <= hi; ++t) {
        if (t >= plateau_lo && t <= plateau_hi)
            continue;
        double badness = 0.0;
        for (size_t i = 0; i < npoints; ++i)
            badness += point_badness(&points[i], uses_high(pred, points[i].size, t));
        if (badness < second_badness)
            second_badness = badness;
    }

    double total_ns = 0.0;
    for (size_t i = 0; i < npoints; ++i) {
        const int use_high = uses_high(pred, points[i].size, best);
        total_ns += use_high ? points[i].high_ns : points[i].low_ns;
    }

    result.threshold = best;
    result.badness = best_badness;
    result.total_ns = total_ns;
    result.margin = second_badness < 1e299 ? second_badness - best_badness : 1e300;
    result.plateau_lo = plateau_lo;
    result.plateau_hi = plateau_hi;
    return result;
}

/* ============================== 确定性数据 ============================== */

static uint64_t tune_next_u64(uint64_t* state) {
    uint64_t z = (*state += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

void tune_fill_limbs(mp_ptr p, mp_size_t n, uint64_t seed) {
    uint64_t state = seed;
    for (mp_size_t i = 0; i < n; ++i)
        p[i] = tune_next_u64(&state);
}

void tune_fill_bytes(mp_byte_t* p, mp_size_t n, uint64_t seed) {
    uint64_t state = seed;
    for (mp_size_t i = 0; i < n; ++i)
        p[i] = (mp_byte_t)tune_next_u64(&state);
}

/* ============================== 打印 ============================== */

void tune_print_measure(const char* tag, const tune_measure_t* m) {
    printf("    %-12s %10.3f ns/call  (min %.3f, max %.3f, MAD %.3f, loops=%u, retries=%u)\n",
           tag, m->median_ns, m->min_ns, m->max_ns, m->mad_ns,
           m->loops, m->retries);
}

void tune_print_points_1d(const char* low_name, const char* high_name,
                          const tune_point_t* points, size_t npoints) {
    printf("    size        %-18s %-18s  faster\n", low_name, high_name);
    for (size_t i = 0; i < npoints; ++i) {
        const int low_wins = points[i].low_ns <= points[i].high_ns;
        printf("    %-10llu %14.3f ns    %14.3f ns    %s\n",
               (unsigned long long)points[i].size,
               points[i].low_ns, points[i].high_ns,
               low_wins ? low_name : high_name);
    }
}

/* ============================== 标准一维任务 ============================== */

double tune_badness_at(const tune_point_t* points, size_t npoints,
                       uint64_t threshold, tune_pred_t pred) {
    double badness = 0.0;
    for (size_t i = 0; i < npoints; ++i)
        badness += point_badness(&points[i], uses_high(pred, points[i].size, threshold));
    return badness;
}

typedef struct {
    const tune_1d_spec_t* spec;
    uint64_t size;
    int use_high;
} tune_path_prep_t;

static void tune_apply_path_for_size(const tune_1d_spec_t* spec,
                                     uint64_t size, int use_high) {
    if (spec->apply_path != NULL) {
        spec->apply_path(size, use_high);
    } else if (spec->pred == TUNE_HIGH_WHEN_GT) {
        if (use_high)
            spec->set(size > spec->lo ? size - 1 : spec->lo);
        else
            spec->set(size);
    } else {
        if (use_high)
            spec->set(spec->lo);
        else
            spec->set(size + 1);
    }
}

static void tune_prepare_path(void* v) {
    const tune_path_prep_t* prep = (const tune_path_prep_t*)v;
    tune_apply_path_for_size(prep->spec, prep->size, prep->use_high);
}

int tune_run_1d(const tune_1d_spec_t* spec) {
    const uint64_t old_value = spec->get();
    const uint64_t hint = old_value >= spec->lo && old_value <= spec->hi
                              ? old_value : (spec->lo + spec->hi) / 2;
    const uint64_t sample_lo = spec->sample_lo != 0 ? spec->sample_lo : spec->lo;
    const uint64_t sample_hi = spec->sample_hi != 0 ? spec->sample_hi : spec->hi;
    const uint64_t sample_hint = old_value >= sample_lo && old_value <= sample_hi
                                     ? old_value : (sample_lo + sample_hi) / 2;
    uint64_t raw_points[1024];
    const size_t npoints = tune_build_points(sample_lo, sample_hi, sample_hint,
                                             raw_points, 1024);
    tune_point_t* points = (tune_point_t*)calloc(npoints, sizeof(tune_point_t));
    if (points == NULL)
        return -1;

    printf("  macro : %s\n", spec->macro_name);
    printf("  range : [%llu, %llu]  hint/old : %llu\n",
           (unsigned long long)spec->lo, (unsigned long long)spec->hi,
           (unsigned long long)old_value);
    printf("  paths : low=%s  high=%s\n", spec->low_name, spec->high_name);
    printf("  points: %llu samples x %u repetitions x %.1f ms target\n",
           (unsigned long long)npoints, g_tune.samples, g_tune.target_ms);
    printf("  measuring ...\n");
    fflush(stdout);

    for (size_t i = 0; i < npoints; ++i) {
        const uint64_t size = raw_points[i];
        void* ctx_low;
        void* ctx_high;
        tune_path_prep_t prep_low;
        tune_path_prep_t prep_high;
        tune_measure_t mlow, mhigh;

        /* make_ctx 可能依赖阈值，因此创建上下文前先应用对应路径。 */
        tune_apply_path_for_size(spec, size, 0);
        ctx_low = spec->make_ctx(size, 0);

        tune_apply_path_for_size(spec, size, 1);
        ctx_high = spec->make_ctx(size, 1);

        /* 关键：测量框架在每个计时批次前都会调用 prepare，重新绑定
         * low/high 阈值。否则最后一次 high 设置会污染 low 校准和采样。 */
        prep_low.spec = spec;
        prep_low.size = size;
        prep_low.use_high = 0;
        prep_high.spec = spec;
        prep_high.size = size;
        prep_high.use_high = 1;

        tune_measure_pair_prepared(spec->bench, ctx_low,
                                   tune_prepare_path, &prep_low,
                                   spec->bench, ctx_high,
                                   tune_prepare_path, &prep_high,
                                   g_tune.samples, g_tune.target_ms,
                                   &mlow, &mhigh);
        points[i].size = spec->map_size != NULL ? spec->map_size(size) : size;
        points[i].low_ns = mlow.median_ns;
        points[i].high_ns = mhigh.median_ns;
        points[i].low_mad = mlow.mad_ns;
        points[i].high_mad = mhigh.mad_ns;
        printf("    size=%-8llu %-14s %10.3f ns   %-14s %10.3f ns\n",
               (unsigned long long)size, spec->low_name, points[i].low_ns,
               spec->high_name, points[i].high_ns);
        fflush(stdout);

        spec->free_ctx(ctx_low);
        spec->free_ctx(ctx_high);
    }

    const tune_choice_t choice = tune_choose_1d(points, npoints, spec->lo, spec->hi,
                                                hint, spec->pred);
    const double old_badness = tune_badness_at(points, npoints, old_value, spec->pred);
    const double new_badness = tune_badness_at(points, npoints, choice.threshold, spec->pred);

    printf("  curve summary:\n");
    tune_print_points_1d(spec->low_name, spec->high_name, points, npoints);
    printf("  -> %-36s %llu\n", spec->macro_name,
           (unsigned long long)choice.threshold);
    printf("     old=%llu (badness %.6f)  new=%llu (badness %.6f)  margin=%.6f\n",
           (unsigned long long)old_value, old_badness,
           (unsigned long long)choice.threshold, new_badness, choice.margin);
    printf("     plateau=[%llu,%llu]  total_time=%.3f ms\n",
           (unsigned long long)choice.plateau_lo,
           (unsigned long long)choice.plateau_hi,
           choice.total_ns * 1e-6);

    spec->set(choice.threshold);
    tune_record_add(spec->macro_name, old_value, choice.threshold,
                    old_badness, new_badness);
    free(points);
    return 0;
}

/* ============================== nPr 二维直线搜索 ============================== */

static double line_point_badness(const tune_line_point_t* p, int use_factor) {
    const double chosen = use_factor ? p->factor_ns : p->product_ns;
    const double faster = chosen < (use_factor ? p->product_ns : p->factor_ns)
                              ? chosen : (use_factor ? p->product_ns : p->factor_ns);
    if (faster <= 0.0 || chosen <= 0.0)
        return 0.0;
    return chosen / faster - 1.0;
}

static int line_uses_factor(const tune_line_point_t* p, uint64_t k, uint64_t b) {
    return p->n + b <= p->r * k;
}

static double line_badness(const tune_line_point_t* points, size_t npoints,
                           uint64_t k, uint64_t b) {
    double badness = 0.0;
    for (size_t i = 0; i < npoints; ++i)
        badness += line_point_badness(&points[i], line_uses_factor(&points[i], k, b));
    return badness;
}

tune_line_choice_t tune_choose_2d_line(const tune_line_point_t* points, size_t npoints,
                                       uint64_t k_lo, uint64_t k_hi,
                                       uint64_t b_lo, uint64_t b_hi,
                                       uint64_t k_hint, uint64_t b_hint,
                                       size_t b_steps) {
    tune_line_choice_t result;
    memset(&result, 0, sizeof(result));
    result.k = k_hint;
    result.b = b_hint;
    if (npoints == 0 || k_hi < k_lo || b_hi < b_lo)
        return result;
    if (b_steps < 8) b_steps = 8;
    if (b_steps > 65536) b_steps = 65536;

    double best = 1e300;
    uint64_t best_k = k_hint;
    uint64_t best_b = b_hint;
    const double log_lo = log((double)(b_lo > 0 ? b_lo : 1));
    const double log_hi = log((double)(b_hi > 0 ? b_hi : 1));

    for (uint64_t k = k_lo; k <= k_hi; ++k) {
        for (size_t j = 0; j <= b_steps; ++j) {
            uint64_t b;
            if (b_lo == 0 && j == 0) {
                b = 0;
            } else {
                const double lv = log_lo + (log_hi - log_lo) * (double)j / (double)b_steps;
                b = (uint64_t)(exp(lv) + 0.5);
                if (b < b_lo) b = b_lo;
                if (b > b_hi) b = b_hi;
            }
            const double badness = line_badness(points, npoints, k, b);
            if (badness < best) {
                best = badness;
                best_k = k;
                best_b = b;
            }
        }
    }

    /* 把 hint 显式加入候选，并最终在平台期优先选择更接近先验值的点。 */
    const double tol = 0.01 * (best > 1.0 ? best : 1.0) + 1e-12;
    const double hint_badness = line_badness(points, npoints, k_hint, b_hint);
    if (hint_badness <= best + tol) {
        best_k = k_hint;
        best_b = b_hint;
    } else {
        const double log_kh = log((double)k_hint + 1.0);
        const double log_bh = log((double)b_hint + 1.0);
        double best_dist = 1e300;
        for (uint64_t k = k_lo; k <= k_hi; ++k) {
            for (size_t j = 0; j <= b_steps; ++j) {
                uint64_t b;
                if (b_lo == 0 && j == 0) {
                    b = 0;
                } else {
                    const double lv = log_lo + (log_hi - log_lo) * (double)j / (double)b_steps;
                    b = (uint64_t)(exp(lv) + 0.5);
                    if (b < b_lo) b = b_lo;
                    if (b > b_hi) b = b_hi;
                }
                if (line_badness(points, npoints, k, b) > best + tol)
                    continue;
                const double dk = log((double)k + 1.0) - log_kh;
                const double db = log((double)b + 1.0) - log_bh;
                const double dist = sqrt(dk * dk + db * db);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_k = k;
                    best_b = b;
                }
            }
        }
    }

    result.k = best_k;
    result.b = best_b;
    result.badness = best;
    result.margin = 0.0;
    return result;
}

/* ============================== 结果记录 ============================== */

typedef struct {
    const char* macro_name;
    uint64_t old_value;
    uint64_t new_value;
    double old_badness;
    double new_badness;
} tune_record_t;

static tune_record_t* g_records = NULL;
static size_t g_record_count = 0;
static size_t g_record_cap = 0;

void tune_record_add(const char* macro_name, uint64_t old_value, uint64_t new_value,
                     double old_badness, double new_badness) {
    if (g_record_count == g_record_cap) {
        const size_t next_cap = g_record_cap == 0 ? 32 : g_record_cap * 2;
        tune_record_t* next = (tune_record_t*)realloc(g_records,
                                                      next_cap * sizeof(tune_record_t));
        if (next == NULL)
            return;
        g_records = next;
        g_record_cap = next_cap;
    }
    g_records[g_record_count].macro_name = macro_name;
    g_records[g_record_count].old_value = old_value;
    g_records[g_record_count].new_value = new_value;
    g_records[g_record_count].old_badness = old_badness;
    g_records[g_record_count].new_badness = new_badness;
    ++g_record_count;
}

int tune_record_write_files(const char* txt_path, const char* h_path) {
    if (g_record_count == 0)
        return 0;

    FILE* ftxt = txt_path != NULL ? fopen(txt_path, "w") : NULL;
    if (ftxt == NULL) {
        fprintf(stderr, "Warning: cannot open result file %s\n",
                txt_path != NULL ? txt_path : "(null)");
        return -1;
    }
    fprintf(ftxt, "# LMMP tuning result (generated; machine specific)\n");
    fprintf(ftxt, "# samples_per_path=%u target_ms=%.1f\n",
            g_tune.samples, g_tune.target_ms);
    fprintf(ftxt, "macro\told\tnew\n");
    for (size_t i = 0; i < g_record_count; ++i) {
        fprintf(ftxt, "%s\t%llu\t%llu\n",
                g_records[i].macro_name,
                (unsigned long long)g_records[i].old_value,
                (unsigned long long)g_records[i].new_value);
    }
    fclose(ftxt);

    FILE* fh = h_path != NULL ? fopen(h_path, "w") : NULL;
    if (fh == NULL) {
        fprintf(stderr, "Warning: cannot open result header %s\n",
                h_path != NULL ? h_path : "(null)");
        return -1;
    }
    fprintf(fh, "/* Generated by lmmp_tune.  Machine specific; review before use. */\n");
    fprintf(fh, "#ifndef LMMP_TUNE_RESULTS_H\n#define LMMP_TUNE_RESULTS_H\n\n");
    for (size_t i = 0; i < g_record_count; ++i) {
        fprintf(fh, "#define %s %llu\n",
                g_records[i].macro_name,
                (unsigned long long)g_records[i].new_value);
    }
    fprintf(fh, "\n#endif /* LMMP_TUNE_RESULTS_H */\n");
    fclose(fh);
    return 0;
}

void tune_record_print_summary(void) {
    if (g_record_count == 0) {
        printf("No tuning result recorded.\n");
        return;
    }
    printf("\n==================== Tune summary ====================\n");
    printf("%-38s %12s %12s\n", "macro", "old", "new");
    for (size_t i = 0; i < g_record_count; ++i) {
        printf("%-38s %12llu %12llu\n",
               g_records[i].macro_name,
               (unsigned long long)g_records[i].old_value,
               (unsigned long long)g_records[i].new_value);
    }
}

int tune_write_mparam(const char* path, const char* backup_path) {
    if (g_record_count == 0)
        return 0;
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "Warning: cannot open %s for update.\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    const long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return -1;
    }
    char* buf = (char*)malloc((size_t)sz + 1);
    if (buf == NULL) {
        fclose(f);
        return -1;
    }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf);
        fclose(f);
        return -1;
    }
    buf[sz] = '\0';
    fclose(f);

    if (backup_path != NULL) {
        FILE* bk = fopen(backup_path, "wb");
        if (bk != NULL) {
            fwrite(buf, 1, (size_t)sz, bk);
            fclose(bk);
            printf("Backup written: %s\n", backup_path);
        }
    }

    for (size_t i = 0; i < g_record_count; ++i) {
        char default_name[96];
        char needle[128];
        snprintf(default_name, sizeof(default_name), "LMMP_DEFAULT_%s",
                 g_records[i].macro_name);
        snprintf(needle, sizeof(needle), "#define %s ", default_name);
        char* pos = strstr(buf, needle);
        if (pos == NULL) {
            fprintf(stderr, "Warning: default literal %s not found; skipped.\n",
                    default_name);
            continue;
        }
        char* line_end = strchr(pos, '\n');
        const size_t old_len = line_end != NULL ? (size_t)(line_end - pos)
                                                : strlen(pos);
        char replacement[160];
        snprintf(replacement, sizeof(replacement), "#define %s %llu",
                 default_name, (unsigned long long)g_records[i].new_value);
        const size_t new_len = strlen(replacement);
        const size_t tail = strlen(pos);
        if (new_len != old_len)
            memmove(pos + new_len, pos + old_len, tail - old_len + 1);
        memcpy(pos, replacement, new_len);
    }

    f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "Warning: cannot write %s.\n", path);
        free(buf);
        return -1;
    }
    fwrite(buf, 1, strlen(buf), f);
    fclose(f);
    free(buf);
    printf("Updated %s\n", path);
    return 0;
}

/* ============================== 模块过滤/执行 ============================== */

static int module_id_matches(const char* token, size_t token_len,
                            const tune_module_t* module) {
    if (module->id != NULL &&
        strlen(module->id) == token_len &&
        strncmp(token, module->id, token_len) == 0)
        return 1;
    if (module->aliases == NULL)
        return 0;
    const char* p = module->aliases;
    while (p != NULL && *p != '\0') {
        const char* q = strchr(p, ',');
        const size_t len = q != NULL ? (size_t)(q - p) : strlen(p);
        if (len == token_len && strncmp(p, token, len) == 0)
            return 1;
        p = q != NULL ? q + 1 : NULL;
    }
    return 0;
}

static int token_wanted(const char* only, const tune_module_t* module) {
    if (only == NULL || only[0] == '\0')
        return 1;
    const char* p = only;
    while (p != NULL && *p != '\0') {
        const char* q = strchr(p, ',');
        const size_t len = q != NULL ? (size_t)(q - p) : strlen(p);
        if (module_id_matches(p, len, module))
            return 1;
        p = q != NULL ? q + 1 : NULL;
    }
    return 0;
}

int tune_module_run(const tune_module_t* modules, size_t count,
                    const char* only) {
    int ran_any = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!token_wanted(only, &modules[i]))
            continue;
        printf("\n============================================================\n");
        printf("[tune] %s (%s)\n", modules[i].id, modules[i].title);
        printf("============================================================\n");
        fflush(stdout);
        const int rc = modules[i].run();
        if (rc != 0) {
            fprintf(stderr, "tune module %s failed (rc=%d)\n", modules[i].id, rc);
            return rc;
        }
        ran_any = 1;
    }
    return ran_any ? 0 : 1;
}
