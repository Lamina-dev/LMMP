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
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

/* 阈值调优：DIVEXACT_MERSENNE_THRESHOLD（mu 主积：朴素直乘 vs 梅森分发不缓存）
 * 以 lmmp_divexact_ nn=4dn+1 黑盒测量；钉死 DIVEXACT_MERSENNE 使缓存不介入 */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD = v; }

typedef struct {
    mp_ptr q;
    mp_ptr dp;
    mp_ptr np;
    mp_ptr dst;
    mp_size_t nn;
    mp_size_t dn;
} divexact_ctx;

static void divexact_ctx_init(divexact_ctx* c, mp_size_t dn) {
    const mp_size_t nn = 4 * dn + 1;
    const mp_size_t qn = nn - dn + 1;
    c->nn = nn;
    c->dn = dn;
    c->q = (mp_ptr)lmmp_alloc((size_t)qn * sizeof(mp_limb_t));
    c->dp = (mp_ptr)lmmp_alloc((size_t)dn * sizeof(mp_limb_t));
    c->np = (mp_ptr)lmmp_alloc((size_t)(nn + 1) * sizeof(mp_limb_t));
    c->dst = (mp_ptr)lmmp_alloc((size_t)(nn + 1) * sizeof(mp_limb_t));
    tune_fill_limbs(c->q, qn, UINT64_C(0x8f1bbcdcbfa97e39));
    tune_fill_limbs(c->dp, dn, UINT64_C(0x9b05688c2b3e6c1f));
    c->q[qn - 1] |= LIMB_B_2;
    c->dp[0] |= 1u;
    c->dp[dn - 1] |= LIMB_B_2;
    lmmp_mul_(c->np, c->q, qn, c->dp, dn);
}

static double bench_divexact(void* v) {
    divexact_ctx* c = (divexact_ctx*)v;
    lmmp_divexact_(c->dst, c->np, c->nn, c->dp, c->dn);
    return 0.0;
}

static void free_ctx(void* v) {
    divexact_ctx* c = (divexact_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->q);
        lmmp_free(c->dp);
        lmmp_free(c->np);
        lmmp_free(c->dst);
        lmmp_free(c);
    }
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    divexact_ctx* c = (divexact_ctx*)lmmp_alloc(sizeof(divexact_ctx));
    if (c != NULL)
        divexact_ctx_init(c, (mp_size_t)size);
    return c;
}

/* 真实选择：dn >= 阈值 → 梅森 cache。测量点 size=dn：
 * 低路（toom）设 size+1，高路（梅森）设 size。 */
static void apply_path(uint64_t size, int use_high) {
    lmmp_tune_DIVEXACT_MERSENNE_CACHE_THRESHOLD = UINT64_C(1) << 20; /* 缓存不介入 */
    set_threshold(use_high ? size : size + 1);
}

int tune_run_divexact_mersenne_fold(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "DIVEXACT_MERSENNE_THRESHOLD";
    spec.low_name = "divexact_mu_toom";
    spec.high_name = "divexact_mu_mersenne";
    spec.lo = 32;
    spec.hi = 4096;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_divexact;
    return tune_run_1d(&spec);
}
