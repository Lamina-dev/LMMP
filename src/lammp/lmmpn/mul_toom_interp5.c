/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lammp/impl/divexact.h"
#include "../../../include/lammp/impl/toom_interp.h"


void lmmp_toom_interp5_(mp_ptr dst, mp_ptr v2, mp_ptr vm1, mp_size_t n, mp_size_t spt, int vm1_neg, mp_limb_t vinf0) {
    mp_limb_t cy, saved;
    mp_size_t dnp = 2 * n + 1;

#define r0 (dst)
#define r1 (dst + n)
#define r2 (dst + 2 * n)
#define r3 (dst + 3 * n)
#define r4 (dst + 4 * n)
#define v0 r0
#define v1 r2
#define vinf r4

    // v2=(v2-vm1)/3
    if (vm1_neg)
        lmmp_add_n_(v2, v2, vm1, dnp);
    else
        lmmp_sub_n_(v2, v2, vm1, dnp);
    lmmp_divexact_by3_(v2, v2, dnp);

    // vm1=(v1-vm1)/2
    if (vm1_neg)
        lmmp_shr1add_n_(vm1, v1, vm1, dnp);
    else
        lmmp_shr1sub_n_(vm1, v1, vm1, dnp);

    // v1=v1-v0
    v1[2 * n] -= lmmp_sub_n_(v1, v1, v0, 2 * n);

    // v2=(v2-v1)/2
    lmmp_shr1sub_n_(v2, v2, v1, dnp);

    // v1=v1-vm1
    lmmp_sub_n_(v1, v1, vm1, dnp);

    // add vm1 at correct place.
    cy = lmmp_add_n_(r1, r1, vm1, dnp);
    lmmp_inc_1(r3 + 1, cy);  // at most propagate to v1[2*n]

    saved = v1[2 * n];  // it is vinf[0]
    vinf[0] = vinf0;    // correct value of vinf

    // v2=v2-vinf*2
    cy = lmmp_shl_(vm1, vinf, spt, 1);
    cy += lmmp_sub_n_(v2, v2, vm1, spt);
    lmmp_dec_1(v2 + spt, cy);

    // vinf+=v2h, no overflow
    cy = lmmp_add_n_(vinf, vinf, v2 + n, n + 1);
    lmmp_inc_1(r3 + dnp, cy);

    // v1-=vinf, (same time vmh-=v2h)
    cy = lmmp_sub_n_(v1, v1, vinf, spt);
    vinf0 = vinf[0];
    v1[2 * n] = saved;  // correct value of v1
    lmmp_dec_1(v1 + spt, cy);

    // vml-=v2l
    cy = lmmp_sub_n_(r1, r1, v2, n);
    lmmp_dec_1(v1, cy);

    // last v2l
    cy = lmmp_add_n_(r3, r3, v2, n);
    v1[2 * n] += cy;  // no carry
    lmmp_inc_1(vinf, vinf0);
}
