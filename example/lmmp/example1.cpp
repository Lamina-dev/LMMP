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
#include <cstring>
#include <algorithm>

#include "lmmp/lmmpn.h"

static mp_size_t read_decimal(const char* s, mp_ptr dst) {
    mp_size_t len = (mp_size_t)std::strlen(s);
    mp_byte_t* digits = (mp_byte_t*)lmmp_alloc(len * sizeof(mp_byte_t));
    for (mp_size_t i = 0; i < len; ++i) digits[len - 1 - i] = (mp_byte_t)(s[i] - '0');
    mp_size_t n = lmmp_from_str_(dst, digits, len, 10);
    lmmp_free(digits);
    return n;
}

static void print_decimal(const mp_byte_t* digits, mp_size_t len) {
    for (mp_size_t i = len; i-- > 0;) std::putchar((int)('0' + digits[i]));
    std::printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: %s <a> <b>\n", argv[0]);
        return 1;
    }

    lmmp_global_init();

    const char* as = argv[1];
    const char* bs = argv[2];
    mp_size_t al = (mp_size_t)std::strlen(as);
    mp_size_t bl = (mp_size_t)std::strlen(bs);
    if (al < bl) {
        std::swap(al, bl);
        std::swap(as, bs);
    }

    mp_ptr a = (mp_ptr)lmmp_alloc(al * sizeof(mp_limb_t));
    mp_ptr b = (mp_ptr)lmmp_alloc(al * sizeof(mp_limb_t));
    mp_ptr r = (mp_ptr)lmmp_alloc((al + 1) * sizeof(mp_limb_t));

    mp_size_t an = read_decimal(as, a);
    mp_size_t bn = read_decimal(bs, b);

    mp_limb_t carry = lmmp_add_(r, a, an, b, bn);
    mp_size_t rn = an + carry;
    mp_size_t slen = lmmp_to_str_len_(r, rn, 10);
    mp_byte_t* str = (mp_byte_t*)lmmp_alloc(slen * sizeof(mp_byte_t));
    slen = lmmp_to_str_(str, r, rn, 10);
    std::printf("a + b = ");
    print_decimal(str, slen);

    mp_limb_t borrow = lmmp_sub_(r, a, an, b, bn);
    if (borrow) {
        std::printf("a - b is negative; this demo only prints unsigned results.\n");
    } else {
        rn = an;
        while (rn > 1 && r[rn - 1] == 0) --rn;
        slen = lmmp_to_str_len_(r, rn, 10);
        str = (mp_byte_t*)lmmp_realloc(str, slen * sizeof(mp_byte_t));
        slen = lmmp_to_str_(str, r, rn, 10);
        std::printf("a - b = ");
        print_decimal(str, slen);
    }

    lmmp_free(a); lmmp_free(b); lmmp_free(r); lmmp_free(str);
    lmmp_global_deinit();
    return 0;
}
