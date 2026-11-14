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
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "lmmp/lmmp.h"
#include "lmmp/lmmpn.h"
#include "lmmp/mprand.h"
#include "lmmp/numth.h"
#include "lmmp_bench.hpp"
#include "lmmp_bench_lib.hpp"

namespace lmmp_bench {

namespace {

/*
    精确除法性能：lmmp_divexact_ 公共入口，尺寸全部落在 basecase 档
    （nn < DIVEXACT_NN_THRESHOLD 且 dn < DIVEXACT_BASECASE_THRESHOLD）。

    输入构造 n = q*d（q、d 顶 limb 置位），确保整除且商规模确定；
    计时前用 memcmp 校验 dst == q。
*/

struct DivexactCtx {
    mp_ptr q;
    mp_ptr d;
    mp_ptr np;
    mp_ptr dst;
    mp_size_t qn;
    mp_size_t qa;  // 实际非零商 limb 数（顶 limb 恒 0）
};

DivexactCtx make_divexact_ctx(mp_size_t nn, mp_size_t dn, mp_limb_t seed) {
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
        std::printf("divexact bench: WRONG QUOTIENT at nn=%lld dn=%lld\n", (long long)nn,
                    (long long)dn);
    }
    return c;
}

void free_divexact_ctx(DivexactCtx& c) {
    lmmp_free(c.q);
    lmmp_free(c.d);
    lmmp_free(c.np);
    lmmp_free(c.dst);
}

}  // namespace

double bench_divexact(mp_size_t nn, mp_size_t dn, const BenchOpts& o) {
    DivexactCtx c = make_divexact_ctx(nn, dn, 0x9e3779b97f4a7c15ull + (mp_limb_t)nn * 31 + dn);
    double us = measure([&] { lmmp_divexact_(c.dst, c.np, nn, c.d, dn); sink += c.dst[0]; }, o);
    free_divexact_ctx(c);
    return us;
}

}  // namespace lmmp_bench

using namespace lmmp_bench;

namespace {

/* 宽窗（qn >> dn）：主循环全宽步进的稳态区 */
const mp_size_t DIVX_WIDE[][2] = {
    {16, 3}, {32, 6}, {64, 12}, {128, 24}, {256, 48}, {512, 48},
};

/* 方形窗（qn ≈ dn）：dn=min(dn,qn) 截断与收缩尾的受益区 */
const mp_size_t DIVX_SQUARE[][2] = {
    {8, 4}, {16, 8}, {32, 16}, {64, 32}, {96, 48},
};

/* 窄窗（qn << dn）：仅低 qn limb 生效的截断区 */
const mp_size_t DIVX_NARROW[][2] = {
    {24, 16}, {48, 32}, {64, 48}, {72, 48},
};

}  // namespace

BENCH_CASE("numth/divexact", wide) {
    for (const auto& sz : DIVX_WIDE) report_ab("divexact wide nn,dn", sz[0], sz[1], bench_divexact(sz[0], sz[1], {}));
}

BENCH_CASE("numth/divexact", square) {
    for (const auto& sz : DIVX_SQUARE) report_ab("divexact square nn,dn", sz[0], sz[1], bench_divexact(sz[0], sz[1], {}));
}

BENCH_CASE("numth/divexact", narrow) {
    for (const auto& sz : DIVX_NARROW) report_ab("divexact narrow nn,dn", sz[0], sz[1], bench_divexact(sz[0], sz[1], {}));
}
