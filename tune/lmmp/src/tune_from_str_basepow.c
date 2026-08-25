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

/* 阈值调优：FROM_STR_BASEPOW_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_FROM_STR_BASEPOW_THRESHOLD; }
static void set_threshold(uint64_t v) { lmmp_tune_FROM_STR_BASEPOW_THRESHOLD = v; }

typedef struct {
    mp_ptr dst;
    mp_byte_t* buf;
    mp_size_t len;
} fromstr_ctx;

static mp_size_t digits_for_limb_target(uint64_t limbs) {
    /* 每个 10^19 块对应 19 位十进制数；这里近似并让 from_str_len 做最终换算。 */
    return (mp_size_t)((limbs * 1927 + 50) / 100);
}

static uint64_t actual_limb_size(uint64_t raw_size) {
    const mp_size_t len = digits_for_limb_target(raw_size);
    return (uint64_t)lmmp_from_str_len_(NULL, len, 10);
}

static void fromstr_ctx_init(fromstr_ctx* c, uint64_t raw_size) {
    uint64_t state = UINT64_C(0x6124c90a4f06a12b);
    c->len = digits_for_limb_target(raw_size);
    c->buf = (mp_byte_t*)lmmp_alloc((size_t)c->len + 1);
    for (mp_size_t i = 0; i < c->len; ++i) {
        state += UINT64_C(0x9e3779b97f4a7c15);
        c->buf[i] = (mp_byte_t)('1' + (state % 9));
    }
    c->buf[c->len] = 0;
    c->dst = (mp_ptr)lmmp_alloc((size_t)(lmmp_from_str_len_(c->buf, c->len, 10) + 2)
                                * sizeof(mp_limb_t));
}

static double bench_fromstr(void* v) {
    fromstr_ctx* c = (fromstr_ctx*)v;
    (void)lmmp_from_str_(c->dst, c->buf, c->len, 10);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    fromstr_ctx* c = (fromstr_ctx*)lmmp_alloc(sizeof(fromstr_ctx));
    if (c != NULL)
        fromstr_ctx_init(c, size);
    return c;
}

static void free_ctx(void* v) {
    fromstr_ctx* c = (fromstr_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->dst);
        lmmp_free(c->buf);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    (void)size;
    if (use_high) {
        lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = 8;
        set_threshold(8);
    } else {
        lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = 8;
        set_threshold(UINT64_C(1) << 20);
    }
}

int tune_run_from_str_basepow(void) {
    tune_1d_spec_t spec;
    const uint64_t saved_divide = lmmp_tune_FROM_STR_DIVIDE_THRESHOLD;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "FROM_STR_BASEPOW_THRESHOLD";
    spec.low_name = "from_str_basecase";
    spec.high_name = "from_str_basepow_divide";
    spec.lo = 8;
    spec.hi = 512;
    spec.sample_lo = 8;
    spec.sample_hi = 512;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_fromstr;
    spec.map_size = actual_limb_size;
    {
        const int rc = tune_run_1d(&spec);
        lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = saved_divide;
        return rc;
    }
}
