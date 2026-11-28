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

#include <math.h>

#include "impl/inlines.h"
#include "numth/powmod.h"
#include "support/perf.h"
#include "tune/common.h"

/* 从 powmod.c 提炼 REDC 分派，调优时直调各档。basecase 和高半积提取复用
   powmod.h 的实现。无上下文入口计入 basecase 的输入复制。其余工作缓冲区
   在计时前备好，两种入口都沿用库内的乘法分派。 */
enum { BASECASE, FULL, MERSENNE, CACHE, PATH_COUNT };
enum { MONT, ONE_SHOT, ENTRY_COUNT };
enum { BASE_MAX = 256, MAX_N = 1536, MAX_POINTS = 300, MAX_ROUNDS = 9 };

static const char* const program = "tune_numth_redc";
static const char* const parameter[] = {"REDC_BASECASE_THRESHOLD", "REDC_MERSENNE_THRESHOLD",
                                        "REDC_MERSENNE_CACHE_THRESHOLD"};
static const int current[] = {REDC_BASECASE_THRESHOLD, REDC_MERSENNE_THRESHOLD, REDC_MERSENNE_CACHE_THRESHOLD};
static const char* const entry_name[] = {"mont", "one_shot"};
static const char* const path_name[] = {"basecase", "full", "mersenne", "cache"};

typedef struct {
    mp_size_t n, mersenne_n, fft_n;
    mp_ptr m, ninv, work, dst;
    mp_srcptr input;
    mp_ptr copies, next;
    fft_gr_cache warm;
    fft_gr_cache* cold;
    mp_size_t cold_used;
    bool warm_ready;
} redc_context;

typedef struct {
    redc_context* redc;
    int path;
    bool cold;
} bench_context;

typedef struct {
    int n;
    double ratio[ENTRY_COUNT][PATH_COUNT][MAX_ROUNDS];
} point;

static void normalize(mp_ptr dst, mp_limb_t cy, mp_srcptr m, mp_size_t n) {
    if (cy || lmmp_cmp_(dst, m, n) >= 0)
        lmmp_sub_n_(dst, dst, m, n);
}

static mp_limb_t mul_redc(redc_context* ctx, mp_srcptr tp, int path, fft_gr_cache* cold) {
    mp_size_t n = ctx->n;
    mp_ptr q = ctx->work;
    mp_ptr scratch = q + n;
    mp_ptr hi = scratch + 3 * n;
    mp_limb_t carry = !lmmp_zero_q_(tp, n);
    lmmp_mullo_dc_(q, tp, ctx->ninv, scratch, n);

    if (path == FULL) {
        lmmp_mul_n_(scratch + 2 * n, q, ctx->m, n);
    } else {
        mp_ptr lo = scratch + 2 * n;
        mp_ptr v = hi + n;
        mp_size_t msz = path == CACHE ? ctx->fft_n : ctx->mersenne_n;
        if (path == MERSENNE) {
            lmmp_mul_mersenne_(v, msz, q, n, ctx->m, n);
        } else if (cold != NULL) {
            lmmp_mul_mersenne_cache_init_(v, msz, q, n, ctx->m, n, cold);
        } else if (!ctx->warm_ready) {
            lmmp_mul_mersenne_cache_init_(v, msz, q, n, ctx->m, n, &ctx->warm);
            ctx->warm_ready = true;
        } else {
            lmmp_mul_mersenne_cache_(v, q, &ctx->warm);
        }
        if (carry) {
            lmmp_not_(lo, tp, n);
            lmmp_inc(lo);
        } else {
            lmmp_zero(lo, n);
        }
        lmmp_mont_fold_hi_(hi, v, lo, n, msz);
    }
    return lmmp_add_nc_(ctx->dst, tp + n, hi, n, carry);
}

static mp_limb_t basecase(redc_context* ctx, mp_ptr tp) {
    mp_size_t n = ctx->n;
    return lmmp_mont_redc_bc_(ctx->dst, tp, ctx->m, n, ctx->ninv[0], n > 1 ? ctx->ninv[1] : 0);
}

static void free_cold(redc_context* ctx) {
    while (ctx->cold_used != 0) lmmp_fft_gr_cache_free_(&ctx->cold[--ctx->cold_used]);
}

static void reset_batch(void* opaque, mp_size_t count) {
    bench_context* bench = opaque;
    redc_context* ctx = bench->redc;
    if (bench->cold)
        free_cold(ctx);
    ctx->next = ctx->copies;
    for (mp_size_t i = 0; i < count; ++i) lmmp_copy(ctx->copies + 2 * ctx->n * i, ctx->input, 2 * ctx->n);
}

static mp_limb_t call_mont(void* opaque) {
    bench_context* bench = opaque;
    redc_context* ctx = bench->redc;
    mp_ptr tp = ctx->next;
    ctx->next += 2 * ctx->n;
    fft_gr_cache* cold = bench->cold ? &ctx->cold[ctx->cold_used++] : NULL;
    mp_limb_t cy = bench->path == BASECASE ? basecase(ctx, tp) : mul_redc(ctx, tp, bench->path, cold);
    normalize(ctx->dst, cy, ctx->m, ctx->n);
    return ctx->dst[0];
}

static void noop(void* opaque) { (void)opaque; }

static mp_limb_t call_one_shot(void* opaque) {
    bench_context* bench = opaque;
    redc_context* ctx = bench->redc;
    mp_limb_t cy;
    if (bench->path == BASECASE) {
        lmmp_copy(ctx->work, ctx->input, 2 * ctx->n);
        cy = basecase(ctx, ctx->work);
    } else {
        cy = mul_redc(ctx, ctx->input, bench->path, NULL);
    }
    return ctx->dst[0] ^ cy;
}

static bool measure_pair(redc_context* ctx,
                         const lmmp_tune_options* options,
                         int entry,
                         int low_path,
                         int high_path,
                         bool cold,
                         double* ratio,
                         unsigned* unstable) {
    bench_context lo = {ctx, low_path, false}, hi = {ctx, high_path, cold};
    lmmp_tune_bench low = {.bind = noop, .context = &lo};
    if (entry == MONT) {
        low.reset_batch = reset_batch;
        low.call = call_mont;
    } else {
        low.reset = noop;
        low.call = call_one_shot;
    }
    lmmp_tune_bench high = low;
    high.context = &hi;
    double low_us, low_mad, high_us, high_mad;
    unsigned retries;
    bool stable;
    lmmp_tune_pair_trace trace;
    if (!lmmp_tune_measure_pair_traced(&low, &high, options, &low_us, &low_mad, &high_us, &high_mad, &retries, &stable,
                                       &trace) ||
        low_us <= 0 || high_us <= 0)
        return false;
    *unstable += !stable;
    free_cold(ctx);
    *ratio = 0;
    if (!stable)
        return true;
    double samples[ARRAY_SIZE(trace.low_us)];
    for (unsigned i = 0; i < options->samples; ++i) samples[i] = trace.high_us[i] / trace.low_us[i];
    *ratio = bench_aggregate(samples, options->samples, BENCH_MEDIAN);
    return isfinite(*ratio) && *ratio > 0;
}

/* 同一输入检查进位和未规范化结果，再与蒙域入口及同余式对拍。
   [tp,2n] 的三个值分别为随机乘积、低半为零、m*B^n-1。 */
static bool check_paths(redc_context* ctx, mp_ptr tp, mp_ptr ref, mp_ptr prod, mp_ptr arena) {
    mp_size_t n = ctx->n;
    lmmp_mont_t deployed;
    lmmp_mont_init_(&deployed, ctx->m, n, arena, 0, prod);
    bool ok = true;
    for (int shape = 0; shape < 3 && ok; ++shape) {
        if (shape == 1) {
            lmmp_zero(tp, n);
            lmmp_copy(tp + n, ctx->m, n);
            lmmp_dec(tp + n);
        } else if (shape == 2) {
            memset(tp, 0xff, n * sizeof(*tp));
        }
        mp_limb_t cy = lmmp_redc_(ref, tp, ctx->ninv, ctx->m, n);
        for (int path = BASECASE; path < PATH_COUNT; ++path) {
            mp_limb_t got;
            if (path == BASECASE) {
                lmmp_copy(ctx->copies, tp, 2 * n);
                got = basecase(ctx, ctx->copies);
            } else {
                got = mul_redc(ctx, tp, path, NULL);
            }
            if (got != cy || lmmp_cmp_(ctx->dst, ref, n) != 0) {
                fprintf(stderr, "REDC mismatch at n=%llu shape=%d path=%s\n", (unsigned long long)n, shape,
                        path_name[path]);
                ok = false;
                break;
            }
        }
        normalize(ref, cy, ctx->m, n);
        lmmp_copy(ctx->copies, tp, 2 * n);
        lmmp_mont_redc_(ctx->dst, ctx->copies, &deployed);
        ok = ok && lmmp_cmp_(ctx->dst, ref, n) == 0 && lmmp_cmp_(ref, ctx->m, n) < 0;
        lmmp_mont_redcify_(ctx->work, ref, n, prod, ctx->m);
        lmmp_div_(NULL, ctx->dst, tp, 2 * n, ctx->m, n);
        ok = ok && lmmp_cmp_(ctx->work, ctx->dst, n) == 0;
    }
    lmmp_mont_free_(&deployed);
    return ok;
}

static bool run_point(point* row, unsigned round, const lmmp_tune_options* options, unsigned* unstable) {
    mp_size_t n = row->n;
    mp_size_t mersenne_n = lmmp_mersenne_next_size_(n), fft_n = lmmp_fft_next_size_(n);
    if (n > mersenne_n || mersenne_n >= 2 * n || n > fft_n || fft_n >= 2 * n || lmmp_fft_next_size_(fft_n) != fft_n)
        return false;

    mp_ptr storage = alloc_limbs((25 + 2 * LMMP_TUNE_BATCH_COPIES) * n);
    mp_ptr tp = storage + 2 * n, prod = tp + 2 * n, arena = prod + 2 * n;
    mp_ptr ref = arena + 8 * n, input = ref + n;
    mp_ptr work = input + 2 * n, dst = work + 7 * n;
    redc_context ctx = {.n = n,
                        .mersenne_n = mersenne_n,
                        .fft_n = fft_n,
                        .m = storage,
                        .ninv = storage + n,
                        .work = work,
                        .dst = dst,
                        .input = input,
                        .copies = dst + n};
    mp_limb_t seed = options->seed + ((mp_limb_t)round << 32) + 4 * n;
    fill_random(ctx.m, n, seed);
    ctx.m[0] |= 1;
    lmmp_binvert_(ctx.ninv, ctx.m, n, n);
    lmmp_not_(ctx.ninv, ctx.ninv, n);
    lmmp_inc(ctx.ninv);
    lmmp_seed_random_(prod, 2 * n, seed + 1, 1);
    prod[n - 1] >>= 1;
    prod[2 * n - 1] >>= 1;
    lmmp_mul_n_(input, prod, prod + n, n);
    lmmp_copy(tp, input, 2 * n);
    bool ok = check_paths(&ctx, tp, ref, prod, arena);

    for (int entry = MONT; entry < ENTRY_COUNT && ok; ++entry) {
        double base_ratio = 0, mersenne_ratio, cache_ratio = 0;
        ok = (n > BASE_MAX || measure_pair(&ctx, options, entry, FULL, BASECASE, false, &base_ratio, unstable)) &&
             measure_pair(&ctx, options, entry, FULL, MERSENNE, false, &mersenne_ratio, unstable) &&
             (entry == ONE_SHOT || measure_pair(&ctx, options, entry, MERSENNE, CACHE, false, &cache_ratio, unstable));
        if (!ok)
            break;
        /* cache 相对于 full 的比值由同一轮的两组稳定读数相乘得到。 */
        double times[] = {base_ratio, base_ratio > 0 || mersenne_ratio > 0 ? 1 : 0, mersenne_ratio,
                          mersenne_ratio * cache_ratio};
        for (int path = BASECASE; path < PATH_COUNT; ++path) row->ratio[entry][path][round] = times[path];
        printf("%u %d %s %.6f %.6f %.6f\n", round + 1, row->n, entry_name[entry], times[BASECASE], times[MERSENNE],
               times[CACHE]);
    }
    if (ok && (n == 128 || n == 256 || n == REDC_MERSENNE_CACHE_THRESHOLD || n == 768 || n == MAX_N)) {
        ctx.cold = lmmp_alloc(LMMP_TUNE_BATCH_COPIES * sizeof(*ctx.cold));
        double first_ratio;
        ok = measure_pair(&ctx, options, MONT, CACHE, CACHE, true, &first_ratio, unstable);
        if (ok && first_ratio > 0)
            printf("# first_cache round=%u n=%d first_over_reuse=%.6f\n", round + 1, row->n, first_ratio);
        free_cold(&ctx);
        lmmp_free(ctx.cold);
    }
    if (ctx.warm_ready)
        lmmp_fft_gr_cache_free_(&ctx.warm);
    lmmp_free(storage);
    if (!ok)
        fprintf(stderr, "REDC check or measurement failed at n=%d\n", row->n);
    return ok;
}

static double stable_median(const double* ratios, unsigned rounds) {
    double stable[MAX_ROUNDS];
    unsigned count = 0;
    for (unsigned r = 0; r < rounds; ++r)
        if (ratios[r] > 0)
            stable[count++] = ratios[r];
    return count > 0 ? bench_aggregate(stable, count, BENCH_MEDIAN) : 0;
}

static double ladder_score(double p[][PATH_COUNT], unsigned count, unsigned scored, unsigned b, unsigned m,
                           unsigned c) {
    double loss = p[b][BASECASE] + (p[m][FULL] - p[b][FULL]) + (p[c][MERSENNE] - p[m][MERSENNE]) +
                  (p[count][CACHE] - p[c][CACHE]);
    return loss / scored;
}

/* 同一入口的所有候选共用计分点，只纳入候选可选路径均有稳定读数的尺寸。
   各点按本点最快路径计相对耗时损失。MAX_N+1 表示扫描范围内不切换。 */
static bool recommend(const point* rows, unsigned count, unsigned rounds) {
    double prefix[ENTRY_COUNT][MAX_POINTS + 1][PATH_COUNT] = {0};
    unsigned scored[ENTRY_COUNT] = {0};
    unsigned at[MAX_N + 2];
    unsigned index = 0;
    for (int n = 1; n <= MAX_N + 1; ++n) {
        while (index < count && rows[index].n < n) ++index;
        at[n] = index;
    }
    for (int entry = MONT; entry < ENTRY_COUNT; ++entry) {
        double (*p)[PATH_COUNT] = prefix[entry];
        const int last_path = entry == MONT ? CACHE : MERSENNE;
        bool base_covered = false;
        for (unsigned i = 0; i < count; ++i) {
            memcpy(p[i + 1], p[i], sizeof(p[i]));
            const int first_path = rows[i].n <= BASE_MAX ? BASECASE : FULL;
            double time[PATH_COUNT], fastest = INFINITY;
            for (int path = first_path; path <= last_path; ++path) {
                time[path] = stable_median(rows[i].ratio[entry][path], rounds);
                fastest = LMMP_MIN(fastest, time[path]);
            }
            if (fastest == 0)
                continue;
            ++scored[entry];
            base_covered |= first_path == BASECASE;
            for (int path = first_path; path <= last_path; ++path)
                p[i + 1][path] += time[path] / fastest - 1;
        }
        /* 有一个计分点覆盖 basecase 时，该入口的各路径均有覆盖。 */
        if (!base_covered)
            return false;
    }
    int mont_best[3];
    for (int entry = MONT; entry < ENTRY_COUNT; ++entry) {
        double (*p)[PATH_COUNT] = prefix[entry];
        const unsigned points = scored[entry];
        int best[3] = {0};
        double best_score = INFINITY;
        int best_distance = MAX_N * 3;
        for (int b = 1; b <= BASE_MAX + 1; ++b) {
            if (b != current[0] && b > 1 && at[b] == at[b - 1])
                continue;
            for (int m = 1; m <= MAX_N + 1; ++m) {
                if (m != current[1] && m > 1 && at[m] == at[m - 1])
                    continue;
                for (int c = entry == MONT ? LMMP_MAX(b, m) : MAX_N + 1; c <= MAX_N + 1; ++c) {
                    if (entry == MONT && c > LMMP_MAX(b, m) && c != current[2] && at[c] == at[c - 1])
                        continue;
                    double score = ladder_score(p, count, points, at[b], at[LMMP_MAX(b, m)], at[c]);
                    int distance =
                        abs(b - current[0]) + abs(m - current[1]) + (entry == MONT ? abs(c - current[2]) : 0);
                    if (score < best_score - 1e-10 || (fabs(score - best_score) <= 1e-10 && distance < best_distance)) {
                        best_score = score;
                        best_distance = distance;
                        best[0] = b;
                        best[1] = m;
                        best[2] = c;
                    }
                }
            }
        }
        if (entry == MONT) {
            memcpy(mont_best, best, sizeof(best));
            int first = best[2], last = best[2];
            int start = LMMP_MAX(best[0], best[1]);
            for (int c = start; c <= MAX_N + 1; ++c) {
                double score = ladder_score(p, count, points, at[best[0]], at[start], at[c]);
                if (score <= best_score + 0.001) {
                    first = LMMP_MIN(first, c);
                    last = LMMP_MAX(last, c);
                }
            }
            printf("# cache_threshold_within_0.1pp min=%d max=%d (other thresholds fixed at recommendation)\n", first,
                   last);
        }
        int current_cache = entry == MONT ? LMMP_MAX(current[0], current[2]) : MAX_N + 1;
        int mont_cache = entry == MONT ? mont_best[2] : MAX_N + 1;
        double current_score = ladder_score(p, count, points, at[current[0]], at[LMMP_MAX(current[0], current[1])],
                                            at[current_cache]);
        double mont_score = ladder_score(p, count, points, at[mont_best[0]], at[LMMP_MAX(mont_best[0], mont_best[1])],
                                         at[mont_cache]);
        printf("# %s current_loss=%.5f%% best_loss=%.5f%% mont_recommendation_loss=%.5f%%\n", entry_name[entry],
               100 * current_score, 100 * best_score, 100 * mont_score);
        for (int k = 0; k < (entry == MONT ? 3 : 2); ++k)
            printf("%s %s = %d\n", entry == MONT ? "recommended" : "one_shot_best", parameter[k], best[k]);
    }
    return true;
}

static bool parse_rounds(void* opaque, const char* option, const char* value) {
    char* end;
    unsigned long n = strtoul(value, &end, 10);
    if (strcmp(option, "--rounds") == 0 && *end == '\0' && n >= 1 && n <= MAX_ROUNDS && (n & 1)) {
        *(unsigned*)opaque = (unsigned)n;
        return true;
    }
    fprintf(stderr, "expected --rounds with an odd integer in [1,%d]\n", MAX_ROUNDS);
    return false;
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    options.min_ms = 0.25;
    options.max_retry = 0;
    unsigned rounds = 5;
    if (!lmmp_tune_parse_ex(argc, argv, &options, parse_rounds, &rounds))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage(program);
        puts("  --rounds N        odd scan count, default 5; REDC defaults: --min-ms 0.25 --max-retry 0");
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        for (int i = 0; i < 3; ++i) lmmp_tune_print_current(parameter[i], current[i], current[i]);
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_INFO) {
        printf("program=%s\npaths=basecase,full,mersenne,cache\nrecommendation=mont_cache_reuse\n", program);
        puts("input_limb_grid=1..96/1,100..256/4,264..768/8,784..1536/16 plus 540..660/4 and current boundary +/- 1");
        puts("one_shot=basecase,full,mersenne; workspace reserved before timing; basecase copy timed");
        lmmp_tune_print_env();
        return EXIT_SUCCESS;
    }
    if (MAX_N >= MULLO_DC_THRESHOLD || current[0] > BASE_MAX + 1 || current[1] > MAX_N + 1 || current[2] > MAX_N + 1 ||
        LMMP_MAX(current[0], current[2]) < current[1]) {
        fputs("REDC grid requires covered thresholds, valid cache workspace sizing and divide-and-conquer mullo\n",
              stderr);
        return EXIT_FAILURE;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    point* rows = lmmp_alloc(MAX_POINTS * sizeof(*rows));
    unsigned count = 0;
    for (int n = 1; n <= MAX_N; ++n) {
        bool on_grid = n <= 96 || n % (n <= 256 ? 4 : n <= 768 ? 8 : 16) == 0 || (n >= 540 && n <= 660 && n % 4 == 0);
        for (int k = 0; k < 3; ++k) on_grid |= abs(n - current[k]) <= 1;
        if (on_grid)
            rows[count++].n = n;
    }
    printf("# program=%s points=%u rounds=%u samples=%u min_ms=%.3f seed=%llu\n", program, count, rounds,
           options.samples, options.min_ms, options.seed);
    lmmp_tune_print_env_comment();
    puts("# round n entry basecase_over_full mersenne_over_full cache_over_full (0=no stable ratio)");
    bool ok = true;
    for (unsigned round = 0; round < rounds && ok; ++round) {
        unsigned unstable = 0;
        for (unsigned i = 0; i < count && ok; ++i) ok = run_point(&rows[i], round, &options, &unstable);
        printf("# round=%u unstable_pairs=%u\n", round + 1, unstable);
    }
    if (ok && !recommend(rows, count, rounds))
        puts("# insufficient stable measurement points; recommendation withheld");
    lmmp_free(rows);
    lmmp_global_deinit();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
