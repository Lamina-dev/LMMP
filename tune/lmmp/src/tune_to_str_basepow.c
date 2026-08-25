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

/* 阈值调优：TO_STR_BASEPOW_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_TO_STR_BASEPOW_THRESHOLD; }
static void set_threshold(uint64_t v) { lmmp_tune_TO_STR_BASEPOW_THRESHOLD = v; }

typedef struct {
    mp_ptr num;
    mp_byte_t* buf;
    mp_size_t n;
} tostr_ctx;

static void tostr_ctx_init(tostr_ctx* c, mp_size_t n) {
    c->n = n;
    c->num = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    tune_fill_limbs(c->num, n, UINT64_C(0x0c1bbcd2f3a12d79));
    c->num[n - 1] |= LIMB_B_2;
    c->buf = (mp_byte_t*)lmmp_alloc((size_t)lmmp_to_str_len_(c->num, n, 10) + 2);
}

static double bench_tostr(void* v) {
    tostr_ctx* c = (tostr_ctx*)v;
    (void)lmmp_to_str_(c->buf, c->num, c->n, 10);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    tostr_ctx* c = (tostr_ctx*)lmmp_alloc(sizeof(tostr_ctx));
    if (c != NULL)
        tostr_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    tostr_ctx* c = (tostr_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->num);
        lmmp_free(c->buf);
        lmmp_free(c);
    }
}

/*
 * low = 整段 basecase；high = 构造基数幂后递归 divide。
 * 为避免调优模式下 basecase 的堆缓冲区尺寸小于输入，low 时同步抬高
 * BASEPOW，high 时把 DIVIDE/BASEPOW 都压到安全下界。
 */
static void apply_path(uint64_t size, int use_high) {
    if (use_high) {
        lmmp_tune_TO_STR_DIVIDE_THRESHOLD = 3;
        set_threshold(3);
    } else {
        lmmp_tune_TO_STR_DIVIDE_THRESHOLD = 3;
        set_threshold(size + 1);
    }
}

int tune_run_to_str_basepow(void) {
    tune_1d_spec_t spec;
    const uint64_t saved_divide = lmmp_tune_TO_STR_DIVIDE_THRESHOLD;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "TO_STR_BASEPOW_THRESHOLD";
    spec.low_name = "to_str_basecase";
    spec.high_name = "to_str_basepow_divide";
    spec.lo = 3;
    spec.hi = 256;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_tostr;
    {
        const int rc = tune_run_1d(&spec);
        lmmp_tune_TO_STR_DIVIDE_THRESHOLD = saved_divide;
        return rc;
    }
}
