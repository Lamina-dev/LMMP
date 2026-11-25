/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
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

#include "support/perf.h"
#include "lmmp/lmmpn.h"
#include "lmmp/mprand.h"
#include "lmmp/numth.h"

/*
    精确除法性能：lmmp_divexact_ 公共入口，尺寸全部落在 basecase 档
    （dn < DIVEXACT_BASECASE_THRESHOLD，对 nn 无限制）。

    输入构造 n = q*d（q、d 顶 limb 置位），确保整除且商规模确定；
    计时前用 memcmp 校验 dst == q。
*/

typedef struct {
    mp_ptr q;
    mp_ptr d;
    mp_ptr np;
    mp_ptr dst;
    mp_size_t qn;
    mp_size_t qa;  // 实际非零商 limb 数（顶 limb 恒 0）
} DivexactCtx;

static DivexactCtx make_divexact_ctx(mp_size_t nn, mp_size_t dn, mp_limb_t seed) {
    DivexactCtx c;
    c.qn = nn - dn + 1;
    c.qa = c.qn - 1;
    c.q = alloc_limbs(c.qa);
    c.d = alloc_limbs(dn);
    c.np = alloc_limbs(nn);
    c.dst = alloc_limbs(c.qn + 1);
    lmmp_seed_random_(c.q, c.qa, seed, 1);
    lmmp_seed_random_(c.d, dn, seed * 3 + 1, 1);
    c.d[0] |= 1;
    /* lmmp_mul_ 要求 na >= nb：窄窗形态商短于除数，需交换 */
    if (c.qa >= dn)
        lmmp_mul_(c.np, c.q, c.qa, c.d, dn);
    else
        lmmp_mul_(c.np, c.d, dn, c.q, c.qa);
    c.dst[c.qn] = 0x5eed5eed5eed5eedull;
    lmmp_divexact_(c.dst, c.np, nn, c.d, dn);
    if (memcmp(c.dst, c.q, (size_t)c.qa * sizeof(mp_limb_t)) != 0 || c.dst[c.qa] != 0) {
        printf("divexact bench: WRONG QUOTIENT at nn=%lld dn=%lld\n", (long long)nn,
                    (long long)dn);
    }
    return c;
}

static void free_divexact_ctx(const DivexactCtx *c) {
    lmmp_free(c->q);
    lmmp_free(c->d);
    lmmp_free(c->np);
    lmmp_free(c->dst);
}

static double bench_divexact(mp_size_t nn, mp_size_t dn, const bench_options *o) {
    DivexactCtx c = make_divexact_ctx(nn, dn, 0x9e3779b97f4a7c15ull + (mp_limb_t)nn * 31 + dn);
    double us;
    BENCH_MEASURE(us, o, lmmp_divexact_(c.dst, c.np, nn, c.d, dn); bench_sink += c.dst[0];);
    free_divexact_ctx(&c);
    return us;
}

/* 宽窗（qn >> dn）：主循环全宽步进的稳态区 */
static const mp_size_t DIVX_WIDE[][2] = {
    {16, 3}, {32, 6}, {64, 12}, {128, 24}, {256, 48}, {512, 48},
};

/* 方形窗（qn ≈ dn）：dn=min(dn,qn) 截断与收缩尾的受益区 */
static const mp_size_t DIVX_SQUARE[][2] = {
    {8, 4}, {16, 8}, {32, 16}, {64, 32}, {96, 48},
};

/* 窄窗（qn << dn）：仅低 qn limb 生效的截断区 */
static const mp_size_t DIVX_NARROW[][2] = {
    {24, 16}, {48, 32}, {64, 48}, {72, 48},
};

static void run_wide(void) {
    for (size_t i = 0; i < ARRAY_SIZE(DIVX_WIDE); ++i) {
        const mp_size_t * sz = DIVX_WIDE[i];
        report_ab("divexact wide nn,dn", sz[0], sz[1], bench_divexact(sz[0], sz[1], &bench_default), 0);
    }
}

static void run_square(void) {
    for (size_t i = 0; i < ARRAY_SIZE(DIVX_SQUARE); ++i) {
        const mp_size_t * sz = DIVX_SQUARE[i];
        report_ab("divexact square nn,dn", sz[0], sz[1], bench_divexact(sz[0], sz[1], &bench_default), 0);
    }
}

static void run_narrow(void) {
    for (size_t i = 0; i < ARRAY_SIZE(DIVX_NARROW); ++i) {
        const mp_size_t * sz = DIVX_NARROW[i];
        report_ab("divexact narrow nn,dn", sz[0], sz[1], bench_divexact(sz[0], sz[1], &bench_default), 0);
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/divexact/wide]");
    run_wide();
    puts("[numth/divexact/square]");
    run_square();
    puts("[numth/divexact/narrow]");
    run_narrow();
    lmmp_global_deinit();
    return 0;
}
