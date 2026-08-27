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

#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/lmmpn.h"


mp_limb_t lmmp_add_n_sub_n_(mp_ptr dsta, mp_ptr dstb, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
#ifdef LMMP_ASM
    mp_limb_t acyo = 0, scyo = 0;
    mp_size_t off, this_n;

    if (dsta != numa && dsta != numb) {
        for (off = 0; off < n; off += PART_SIZE) {
            this_n = LMMP_MIN(n - off, PART_SIZE);
            acyo = lmmp_add_nc_(dsta + off, numa + off, numb + off, this_n, acyo);
            scyo = lmmp_sub_nc_(dstb + off, numa + off, numb + off, this_n, scyo);
        }
    } else if (dstb != numa && dstb != numb) {
        for (off = 0; off < n; off += PART_SIZE) {
            this_n = LMMP_MIN(n - off, PART_SIZE);
            scyo = lmmp_sub_nc_(dstb + off, numa + off, numb + off, this_n, scyo);
            acyo = lmmp_add_nc_(dsta + off, numa + off, numb + off, this_n, acyo);
        }
    } else {
        mp_limb_t tp[PART_SIZE];
        for (off = 0; off < n; off += PART_SIZE) {
            this_n = LMMP_MIN(n - off, PART_SIZE);
            acyo = lmmp_add_nc_(tp, numa + off, numb + off, this_n, acyo);
            scyo = lmmp_sub_nc_(dstb + off, numa + off, numb + off, this_n, scyo);
            lmmp_copy(dsta + off, tp, this_n);
        }
    }
    return 2 * acyo + scyo;
#else
    mp_size_t i;
    mp_limb_t acyo, scyo;

    for (i = 0, acyo = 0, scyo = 0; i < n; i++) {
        mp_limb_t a, b, r;
        a = numa[i];
        b = numb[i];
        r = a + acyo;
        acyo = (r < acyo);
        r += b;
        acyo += (r < b);
        dsta[i] = r;

        b += scyo;
        scyo = (b < scyo);
        scyo += (a < b);
        dstb[i] = a - b;
    }
    return 2 * acyo + scyo;
#endif
}
