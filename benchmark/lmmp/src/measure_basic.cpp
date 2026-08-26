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
#include "lmmp_measure.hpp"


using namespace lmmp_measure;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void fill_random(mp_ptr p, mp_size_t n, mp_limb_t seed) {
    lmmp_seed_random_(p, n, seed, 1);
}

}  // namespace

MEASURE_CASE("lmmpn/add", add_n) {
    const mp_size_t n = 2000;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n);
    fill_random(a, n, 0x123456789ull);
    fill_random(b, n, 0x987654321ull);
    File f("add_n");
    for (mp_size_t i = 2; i <= n; i += 2) {
        auto m = measure([&] { lmmp_add_n_(d, a, b, i); }, i, 12);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

MEASURE_CASE("lmmpn/mul", mul_n) {
    const mp_size_t n = 2000;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(2 * n);
    fill_random(a, n, 0x120e3489bceull);
    fill_random(b, n, 0x92cab543221ull);
    File f("mul_n");
    for (mp_size_t i = 10; i <= n; i += 2) {
        auto m = measure([&] { lmmp_mul_n_(d, a, b, i); }, i, 8);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

MEASURE_CASE("lmmpn/mul", mul_mersenne) {
    const mp_size_t n = 2000;
    mp_size_t rn = lmmp_fft_next_size_(n);
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(rn);
    fill_random(a, n, 0x120e3489bceull);
    fill_random(b, n, 0x92cab543221ull);
    File f("mul_mersenne");
    for (mp_size_t i = 10; i <= n; i += 2) {
        rn = lmmp_fft_next_size_(i);
        auto m = measure([&] { lmmp_mul_mersenne_(d, rn, a, i, b, i); }, i, 8);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

MEASURE_CASE("lmmpn/mul", mul_toom22) {
    const mp_size_t n = 200;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(2 * n);
    fill_random(a, n, 0x120e3489bceull);
    fill_random(b, n, 0x92cab543221ull);
    File f("mul_toom22");
    for (mp_size_t i = 10; i <= n; i += 2) {
        auto m = measure([&] { lmmp_mul_toom22_(d, a, i, b, i); }, i, 12);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

MEASURE_CASE("lmmpn/mul", mul_toom33) {
    const mp_size_t n = 3500;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(2 * n);
    fill_random(a, n, 0x120e3489bceull);
    fill_random(b, n, 0x92cab543221ull);
    File f("mul_toom33");
    for (mp_size_t i = 20; i <= n; i += 2) {
        auto m = measure([&] { lmmp_mul_toom33_(d, a, i, b, i); }, i, 8);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

MEASURE_CASE("lmmpn/mul", mul_toom44) {
    const mp_size_t n = 3500;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(2 * n);
    fill_random(a, n, 0x120e3489bceull);
    fill_random(b, n, 0x92cab543221ull);
    File f("mul_toom44");
    for (mp_size_t i = 20; i <= n; i += 5) {
        auto m = measure([&] { lmmp_mul_toom44_(d, a, i, b, i); }, i, 8);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

MEASURE_CASE("lmmpn/mul", mul_fft) {
    const mp_size_t n = 5000;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(2 * n);
    fill_random(a, n, 0x120e3489bceull);
    fill_random(b, n, 0x92cab543221ull);
    File f("mul_fft");
    for (mp_size_t i = 50; i <= n; i += 5) {
        auto m = measure([&] { lmmp_mul_fft_(d, a, i, b, i); }, i, 6);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

MEASURE_CASE("lmmpn/mullo", mullo_basecase) {
    const mp_size_t n = 100;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n);
    fill_random(a, n, 0x120e3489bceull);
    fill_random(b, n, 0x92cab543221ull);
    File f("mullo_basecase");
    for (mp_size_t i = 20; i <= n; i += 1) {
        auto m = measure([&] { lmmp_mullo_basecase_(d, a, b, i); }, i, 12);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

MEASURE_CASE("lmmpn/mullo", mullo_dc) {
    const mp_size_t n = 5000;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr c = alloc_limbs(2 * n);
    mp_ptr d = alloc_limbs(n);
    fill_random(a, n, 0x120e3489bceull);
    fill_random(b, n, 0x92cab543221ull);
    File f("mullo_dc");
    for (mp_size_t i = 50; i <= n; i += 5) {
        auto m = measure([&] { lmmp_mullo_dc_(d, a, b, c, i); }, i, 6);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(c); lmmp_free(d);
}

MEASURE_CASE("lmmpn/mullo", mullo_fft) {
    const mp_size_t n = 5000;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr c = alloc_limbs(2 * n);
    mp_ptr d = alloc_limbs(n);
    fill_random(a, n, 0x120e3489bceull);
    fill_random(b, n, 0x92cab543221ull);
    File f("mullo_fft");
    for (mp_size_t i = 50; i <= n; i += 5) {
        auto m = measure([&] { lmmp_mullo_fft_(d, a, b, i, c); }, i, 6);
        write(f, m);
        progress_bar(i, n, 40, "    Measuring");
    }
    printf("    Done\n");
    lmmp_free(a); lmmp_free(b); lmmp_free(c); lmmp_free(d);
}