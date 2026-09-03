//  Copyright (C) 2026 HJimmyK(Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it under
//  the terms of the GNU Lesser General Public License (LGPL) as published
//  by the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed WITHOUT ANY WARRANTY.
//
//  See <https://www.gnu.org/licenses/>.

// mp_limb_t lmmp_div_3_2_(mp_ptr numa, mp_srcptr numb, mp_limb_t inv21);
//
//   x0=numa, x1=numb, x2=inv21
//   ret x0 = quotient q
//   cal: q = a2*inv21 + a1; z = a - q*b; fixups

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_div_3_2_)
.p2align 2

ASM_GSYM(lmmp_div_3_2_):
    ldp     x8, x9, [x0]           // x8 = a0, x9 = a1
    ldr     x10, [x0, #16]         // x10 = a2
    mul     x11, x10, x2           // lo(a2*m)
    umulh   x12, x10, x2           // hi(a2*m)
    adds    x11, x9, x11
    adc     x10, x10, x12          // x10:x11 = q = a2*inv + a1
    ldp     x13, x12, [x1]         // x13 = bl, x12 = bh
    mvn     x14, x10               // ~qh
    subs    x8, x8, x13            // a0 - bl
    mul     x15, x10, x13          // lo(qh*bl)
    umulh   x16, x10, x13          // hi(qh*bl)
    sbc     x9, x9, xzr            // borrow to a1
    subs    x8, x8, x15
    madd    x9, x12, x14, x9       // a1 -= (qh+1)*bh
    sbc     x9, x9, x16            // x9:x8 = z = a - q*b
    cmp     x9, x11
    csel    x11, x13, xzr, hs      // no borrow: z += b
    cset    w14, hs
    csel    x15, x12, xzr, hs
    sub     x10, x10, x14          // borrow: qh--
    add     x14, x10, #1           // qh+1
    adds    x8, x11, x8
    adc     x9, x15, x9
    add     x11, x14, #1           // qh+2
    subs    x14, x8, x13           // z - b
    csetm   x15, lo
    sub     x15, x15, x12
    add     x15, x15, x9
    cmp     x8, x13
    ccmp    x9, x12, #2, lo        // z >= b ?
    csinc   x11, x11, x10, hi
    csel    x13, x8, x14, ls
    csel    x14, x9, x15, ls
    cmp     x9, x12
    csel    x12, x8, x13, lo       // final r lo
    csinc   x8, x11, x10, hs       // final q
    csel    x9, x9, x14, lo        // final r hi
    stp     x12, x9, [x0]          // r -> [numa,2]
    mov     x0, x8
    ret
