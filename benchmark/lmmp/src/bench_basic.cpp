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

#include <cstring>

using namespace lmmp_bench;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void fill_random(mp_ptr p, mp_size_t n, mp_limb_t seed) {
    lmmp_seed_random_(p, n, seed, 1);
    if (n > 0) p[n - 1] |= (mp_limb_t)1 << 63;
}

}  // namespace

BENCH_CASE("lmmpn/add", add_n_1000) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n);
    fill_random(a, n, 0x123456789ull);
    fill_random(b, n, 0x987654321ull);

    auto m = measure([&] { lmmp_add_n_(d, a, b, n); });
    report("lmmp_add_n_(1000 limbs)", m, n * 8.0);

    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("lmmpn/sub", sub_n_1000) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n);
    fill_random(a, n, 0x123456789ull);
    fill_random(b, n, 0x987654321ull);

    auto m = measure([&] { lmmp_sub_n_(d, a, b, n); });
    report("lmmp_sub_n_(1000 limbs)", m, n * 8.0);

    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("lmmpn/mul", mul_1000x1000) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(2 * n + 2);
    fill_random(a, n, 0x1111222233334444ull);
    fill_random(b, n, 0x5555666677778888ull);

    auto m = measure([&] { lmmp_mul_(d, a, n, b, n); });
    report("lmmp_mul_(1000x1000)", m, n * n * 8.0);

    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("lmmpn/sqr", sqr_1000) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n);
    mp_ptr d = alloc_limbs(2 * n + 2);
    fill_random(a, n, 0x0badf00dcafefaceull);

    auto m = measure([&] { lmmp_sqr_(d, a, n); });
    report("lmmp_sqr_(1000)", m, n * n * 8.0);

    lmmp_free(a); lmmp_free(d);
}

BENCH_CASE("lmmpn/div", div_2000x1000) {
    const mp_size_t na = 2000;
    const mp_size_t nb = 1000;
    mp_ptr n = alloc_limbs(na);
    mp_ptr d = alloc_limbs(nb);
    mp_ptr q = alloc_limbs(na - nb + 1);
    mp_ptr r = alloc_limbs(nb);
    fill_random(n, na, 0x0123456789abcdeful);
    fill_random(d, nb, 0xfedcba9876543210ull);

    auto m = measure([&] { lmmp_div_(q, r, n, na, d, nb); });
    report("lmmp_div_(2000/1000)", m);

    lmmp_free(n); lmmp_free(d); lmmp_free(q); lmmp_free(r);
}

BENCH_CASE("numth/gcd", gcd_lehmer_200) {
    const mp_size_t n = 200;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n);
    fill_random(a, n, 0x0a0a0a0a0b0b0b0bull);
    fill_random(b, n, 0x0c0c0c0c0d0d0d0dull);

    auto m = measure([&] { lmmp_gcd_lehmer_(d, a, n, b, n); });
    report("lmmp_gcd_lehmer_(200 limbs)", m);

    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("numth/factorial", factorial_1000) {
    mp_bitcnt_t bits = 0;
    mp_size_t need = lmmp_factorial_size_(1000, &bits);
    mp_ptr d = alloc_limbs(need + 2);

    auto m = measure([&] { lmmp_factorial_(d, bits, need, 1000); });
    report("lmmp_factorial_(1000)", m);

    lmmp_free(d);
}
