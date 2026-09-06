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

#include "lmmp/lmmpn.h"
#include "lmmp/mprand.h"
#include "lmmp/numth.h"
#include "lmmp_bench.hpp"

using namespace lmmp_bench;

namespace {

const int POOL = 1024;

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void fill_random(mp_ptr p, mp_size_t n, mp_limb_t seed) {
    lmmp_seed_random_(p, n, seed, 1);
    if (n > 0) p[n - 1] |= (mp_limb_t)1 << 63;
}

}  // namespace

/* cbrt_3 / cbrt_6 估计路径（log2/exp2 种子）与除法快速路径的性能基线 */

BENCH_CASE("numth/cbrt", cbrt_3) {
    mp_ptr a = alloc_limbs(3 * POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + 3 * i, 3, 0xC0FFEEull + i, 1);
        if (a[3 * i + 1] == 0 && a[3 * i + 2] == 0) a[3 * i + 1] = 1;
    }

    size_t idx = 0;
    auto m = measure([&] {
        mp_limb_t* p = a + 3 * (idx++ & (POOL - 1));
        lmmp_cbrt_3_(p[0], p[1], p[2]);
    });
    report("lmmp_cbrt_3_(3 limbs)", m);

    lmmp_free(a);
}

BENCH_CASE("numth/cbrt", cbrt_6_est_n4) {
    mp_ptr a = alloc_limbs(6 * POOL);
    mp_ptr r = alloc_limbs(2);
    for (int i = 0; i < POOL; i++) fill_random(a + 6 * i, 4, 0x1234578ull + i);

    size_t idx = 0;
    auto m = measure([&] { lmmp_cbrt_6_(r, a + 6 * (idx++ & (POOL - 1)), 4); });
    report("lmmp_cbrt_6_(n=4 est)", m);

    lmmp_free(a);
    lmmp_free(r);
}

BENCH_CASE("numth/cbrt", cbrt_6_est_n5) {
    mp_ptr a = alloc_limbs(6 * POOL);
    mp_ptr r = alloc_limbs(2);
    for (int i = 0; i < POOL; i++) fill_random(a + 6 * i, 5, 0x9ABCDEF1ull + i);

    size_t idx = 0;
    auto m = measure([&] { lmmp_cbrt_6_(r, a + 6 * (idx++ & (POOL - 1)), 5); });
    report("lmmp_cbrt_6_(n=5 est)", m);

    lmmp_free(a);
    lmmp_free(r);
}

// n=6 且最高 limb < CBRT_DIVIDE_MIN，强制走 log2/exp2 估计路径
BENCH_CASE("numth/cbrt", cbrt_6_est_n6) {
    mp_ptr a = alloc_limbs(6 * POOL);
    mp_ptr r = alloc_limbs(2);
    for (int i = 0; i < POOL; i++) {
        fill_random(a + 6 * i, 6, 0xFEDCBA98ull + i);
        mp_limb_t* p = a + 6 * i;
        p[5] &= 0x5FFFFFFFFFFFFFFFull;
        if (p[5] == 0) p[5] = 1;
    }

    size_t idx = 0;
    auto m = measure([&] { lmmp_cbrt_6_(r, a + 6 * (idx++ & (POOL - 1)), 6); });
    report("lmmp_cbrt_6_(n=6 est)", m);

    lmmp_free(a);
    lmmp_free(r);
}

// n=6 且最高 limb >= CBRT_DIVIDE_MIN，走 cbrt_3 种子 + 除法路径（对照）
BENCH_CASE("numth/cbrt", cbrt_6_fast_n6) {
    mp_ptr a = alloc_limbs(6 * POOL);
    mp_ptr r = alloc_limbs(2);
    for (int i = 0; i < POOL; i++) fill_random(a + 6 * i, 6, 0x13579BDFull + i);

    size_t idx = 0;
    auto m = measure([&] { lmmp_cbrt_6_(r, a + 6 * (idx++ & (POOL - 1)), 6); });
    report("lmmp_cbrt_6_(n=6 fast)", m);

    lmmp_free(a);
    lmmp_free(r);
}
