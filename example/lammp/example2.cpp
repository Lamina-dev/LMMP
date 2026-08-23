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

#include <cstdio>
#include <cstdlib>

#include "lammp/numth.h"

static bool is_number(const char* s) {
    if (s == nullptr || *s == '\0') return false;
    while (*s != '\0') {
        if (*s < '0' || *s > '9') return false;
        ++s;
    }
    return true;
}

static void factorial(unsigned n) {
    mp_bitcnt_t bits = 0;
    mp_size_t need = lmmp_factorial_size_(n, &bits);
    mp_ptr dst = (mp_ptr)lmmp_alloc(need * sizeof(mp_limb_t));
    mp_size_t len = lmmp_factorial_(dst, bits, need, n);

    std::printf("%u! has %llu bits (about %.0f decimal digits), limb count = %lld\n",
                n,
                (unsigned long long)bits,
                (double)bits * 0.301029995663981195,
                (long long)len);
    std::printf("high limb = 0x%llx, low limb = 0x%llx\n",
                (unsigned long long)dst[len - 1],
                (unsigned long long)dst[0]);

    lmmp_free(dst);
}

int main(int argc, char** argv) {
    lmmp_global_init();

    unsigned n = 0;
    if (argc >= 2) {
        if (!is_number(argv[1])) {
            std::printf("Invalid number: %s\n", argv[1]);
            lmmp_global_deinit();
            return 1;
        }
        n = (unsigned)std::strtoul(argv[1], nullptr, 10);
    } else {
        std::printf("Please input a non-negative integer: ");
        if (std::scanf("%u", &n) != 1) {
            std::printf("Input error.\n");
            lmmp_global_deinit();
            return 1;
        }
    }

    factorial(n);
    lmmp_global_deinit();
    return 0;
}
