/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  This library is free software: you can redistribute it and/or modify it
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
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "numth/tune/gcd_tune.h"

enum { GCD, GCDEXT, ENTRY_COUNT };
enum { BALANCED, NEAR_BALANCED, UNEVEN, SHAPE_COUNT };
enum { MIN_N = 24, MAX_N = 768 };

static const char program[] = "tune_numth_gcd_dispatch";
static const char parameter[] = "GCD_HGCD_THRESHOLD";
static const char* const entry_name[] = {"gcd", "gcdext"};
static const char* const shape_name[] = {"equal", "5:4", "2:1"};

typedef struct {
    mp_ptr g, s, t;
    mp_size_t gn;
    mp_ssize_t sn, tn;
} gcd_result;

typedef struct {
    mp_srcptr u, v;
    mp_size_t un, vn;
    gcd_result result;
    int entry;
    bool hgcd;
} gcd_bench;

typedef struct {
    int n, entry, shape;
    double ratios[GCD_TUNE_MAX_ROUNDS];
} point;

static void run_gcd(gcd_bench* ctx) {
    gcd_result* r = &ctx->result;
    if (ctx->entry == GCD) {
        r->gn = ctx->hgcd ? lmmp_gcd_hgcd_(r->g, ctx->u, ctx->un, ctx->v, ctx->vn)
                         : lmmp_gcd_lehmer_(r->g, ctx->u, ctx->un, ctx->v, ctx->vn);
    } else {
        r->gn = ctx->hgcd ? lmmp_gcdext_hgcd_(r->g, r->s, &r->sn, r->t, &r->tn, ctx->u, ctx->un, ctx->v, ctx->vn)
                         : lmmp_gcdext_lehmer_(r->g, r->s, &r->sn, r->t, &r->tn, ctx->u, ctx->un, ctx->v, ctx->vn);
    }
}

static mp_limb_t call_gcd(void* opaque) {
    gcd_bench* ctx = opaque;
    run_gcd(ctx);
    const gcd_result* r = &ctx->result;
    return ctx->entry == GCD ? r->gn ^ r->g[0] : r->gn ^ r->g[0] ^ (mp_limb_t)r->sn ^ (mp_limb_t)r->tn;
}

static mp_size_t magnitude(mp_ssize_t n) {
    return n < 0 ? (mp_size_t)-n : (mp_size_t)n;
}

static bool same_result(const gcd_result* a, const gcd_result* b, int entry) {
    return a->gn == b->gn && a->gn > 0 && lmmp_cmp_(a->g, b->g, a->gn) == 0 &&
           (entry == GCD ||
            (a->sn == b->sn && a->tn == b->tn &&
             (!a->sn || lmmp_cmp_(a->s, b->s, magnitude(a->sn)) == 0) &&
             (!a->tn || lmmp_cmp_(a->t, b->t, magnitude(a->tn)) == 0)));
}

static bool check_paths(gcd_bench* low, gcd_bench* high, gcd_result* public_result) {
    run_gcd(low);
    run_gcd(high);
    if (!same_result(&low->result, &high->result, low->entry))
        return false;

    if (low->entry == GCD) {
        public_result->gn = lmmp_gcd_(public_result->g, low->u, low->un, low->v, low->vn);
    } else {
        public_result->gn = lmmp_gcdext_(public_result->g, public_result->s, &public_result->sn, public_result->t,
                                         &public_result->tn, low->u, low->un, low->v, low->vn);
    }
    return same_result(&low->result, public_result, low->entry);
}

static bool run_point(point* row, unsigned round, bool measure, const lmmp_tune_options* options) {
    mp_size_t un = row->n, vn = row->n;
    if (row->entry == GCD)
        vn = row->shape == UNEVEN ? un / 2 : row->shape == NEAR_BALANCED ? 4 * un / 5 : un;
    else
        un = row->shape == UNEVEN ? 2 * vn : row->shape == NEAR_BALANCED ? (5 * vn + 3) / 4 : vn;

    mp_ptr u = alloc_limbs(4 * un + 7 * vn);
    mp_ptr v = u + un, output = v + vn;
    mp_size_t stride = un + 2 * vn;
    mp_limb_t seed = options->seed + ((mp_limb_t)round << 32) + (mp_limb_t)row->n * 16 + 2 * row->shape;
    fill_random(u, un, seed);
    fill_random(v, vn, seed + 1);
    gcd_bench low = {.u = u, .v = v, .un = un, .vn = vn, .entry = row->entry,
                     .result = {.g = output, .s = output + vn, .t = output + 2 * vn}};
    gcd_bench high = low;
    high.hgcd = true;
    high.result = (gcd_result){.g = output + stride, .s = output + stride + vn, .t = output + stride + 2 * vn};
    gcd_result actual = {.g = output + 2 * stride, .s = output + 2 * stride + vn, .t = output + 2 * stride + 2 * vn};
    bool ok = check_paths(&low, &high, &actual);
    if (ok && measure) {
        gcd_tune_pair pair;
        ok = gcd_tune_measure(call_gcd, &low, &high, options, &pair, &row->ratios[round]);
        if (ok)
            printf("%u %s %s %d %llu %llu %.6f %.6f %.6f %.4f %.4f %u %d\n", round + 1, entry_name[row->entry],
                   shape_name[row->shape], row->n, (unsigned long long)un, (unsigned long long)vn,
                   pair.low_us, pair.high_us, row->ratios[round], pair.low_mad, pair.high_mad, pair.retries, pair.stable);
    }
    if (!ok)
        fprintf(stderr, "%s check or measurement failed at %llu x %llu limb\n", entry_name[row->entry],
                (unsigned long long)un, (unsigned long long)vn);
    lmmp_free(u);
    return ok;
}

static void recommend(const point* rows, unsigned count, unsigned rounds, int entry, int shape) {
    double loss[MAX_N + 2] = {0};
    unsigned points = 0;
    for (unsigned i = 0; i < count; ++i) {
        const point* row = &rows[i];
        if ((entry != ENTRY_COUNT && row->entry != entry) || (shape != SHAPE_COUNT && row->shape != shape))
            continue;
        double ratio = gcd_tune_median(row->ratios, rounds);
        if (ratio <= 0)
            continue;
        double fastest = LMMP_MIN(1., ratio);
        for (int candidate = MIN_N; candidate <= MAX_N + 1; ++candidate)
            loss[candidate] += (row->n >= candidate ? ratio : 1.) / fastest - 1.;
        ++points;
    }

    const char* entry_label = entry == ENTRY_COUNT ? "joint" : entry_name[entry];
    const char* shape_label = shape == SHAPE_COUNT ? "all" : shape_name[shape];
    if (!points) {
        printf("# entry=%s shape=%s no stable points; recommendation withheld\n", entry_label, shape_label);
        return;
    }

    int best = MIN_N;
    for (int candidate = MIN_N; candidate <= MAX_N + 1; ++candidate) {
        loss[candidate] *= 100. / points;
        if (loss[candidate] < loss[best])
            best = candidate;
    }
    int first = best, last = best;
    while (first > MIN_N && loss[first - 1] <= loss[best] + 0.1) --first;
    while (last < MAX_N + 1 && loss[last + 1] <= loss[best] + 0.1) ++last;
    printf("# entry=%s shape=%s points=%u best=%d loss_pct=%.5f within_0.1pp=%d..%d current=%d current_loss_pct=%.5f\n",
           entry_label, shape_label, points, best, loss[best], first, last, GCD_HGCD_THRESHOLD, loss[GCD_HGCD_THRESHOLD]);
    if (entry == ENTRY_COUNT)
        printf("recommended %s = %d\n", parameter, best);
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    options.min_ms = 0.25;
    options.max_retry = 1;
    unsigned rounds = 5;
    if (!lmmp_tune_parse_ex(argc, argv, &options, gcd_tune_parse_rounds, &rounds))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage(program);
        puts("  --rounds N        odd scan count, default 5; 0 checks inputs and results only");
        puts("  Program defaults: --min-ms 0.25 --max-retry 1");
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current(parameter, GCD_HGCD_THRESHOLD, GCD_HGCD_THRESHOLD);
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_INFO) {
        printf("program=%s\nparameter=%s\npaths=lehmer,hgcd\n", program, parameter);
        puts("dispatch_size=gcd:max(un,vn),gcdext:min(un,vn); shapes=equal,5:4,2:1");
        puts("input_limb_grid=24..44/4,48..144/1,148..192/4,200..256/8,272..512/16,544..768/32"
             " plus current boundary +/- 1");
        puts("candidate_limb_grid=24..769/1; joint scoring gives equal weight to each stable point");
        lmmp_tune_print_current(parameter, GCD_HGCD_THRESHOLD, GCD_HGCD_THRESHOLD);
        lmmp_tune_print_env();
        return EXIT_SUCCESS;
    }
    if (GCD_HGCD_THRESHOLD < MIN_N || GCD_HGCD_THRESHOLD > MAX_N + 1) {
        fputs("GCD_HGCD_THRESHOLD is outside the candidate grid\n", stderr);
        return EXIT_FAILURE;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    point* rows = lmmp_alloc((MAX_N - MIN_N + 1) * SHAPE_COUNT * ENTRY_COUNT * sizeof(*rows));
    unsigned count = 0;
    for (int n = MIN_N; n <= MAX_N; ++n) {
        int step = n < 48 ? 4 : n <= 144 ? 1 : n <= 192 ? 4 : n <= 256 ? 8 : n <= 512 ? 16 : 32;
        if (n % step != 0 && abs(n - GCD_HGCD_THRESHOLD) > 1)
            continue;
        for (int entry = GCD; entry < ENTRY_COUNT; ++entry)
            for (int shape = BALANCED; shape < SHAPE_COUNT; ++shape)
                rows[count++] = (point){.n = n, .entry = entry, .shape = shape};
    }
    printf("# program=%s points=%u rounds=%u samples=%u min_ms=%.3f seed=%llu\n",
           program, count, rounds, options.samples, options.min_ms, options.seed);
    lmmp_tune_print_env_comment();
    puts("# round entry shape n un vn lehmer_us hgcd_us hgcd_over_lehmer lehmer_mad hgcd_mad retries stable");

    bool ok = true;
    for (unsigned round = 0; round < LMMP_MAX(1U, rounds) && ok; ++round)
        for (unsigned i = 0; i < count && ok; ++i)
            ok = run_point(&rows[i], round, rounds != 0, &options);
    if (ok) {
        printf("# checks_passed=%u\n", count * LMMP_MAX(1U, rounds));
        if (rounds) {
            for (int entry = GCD; entry < ENTRY_COUNT; ++entry)
                for (int shape = BALANCED; shape <= SHAPE_COUNT; ++shape)
                    recommend(rows, count, rounds, entry, shape);
            recommend(rows, count, rounds, ENTRY_COUNT, SHAPE_COUNT);
        }
    }
    lmmp_free(rows);
    lmmp_global_deinit();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
