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

// mp_limb_t lmmp_mod_1_(mp_srcptr numa, mp_size_t na, mp_limb_t x);
//
//   x0=numa, x1=na, x2=x
//   ret x0 = [numa,na] mod x
//   cal: as div_1_, no quotient stored

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_mod_1_)
.p2align 2

ASM_GSYM(lmmp_mod_1_):
// x20=&numa[na-1], x21=na-1, x22=d, x23=k, x0=m
    stp     x29, x30, [sp, #-80]!
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    mov     x22, x2
    mov     x21, x1
    sub     x21, x21, #1
    mov     x20, x0
    add     x20, x20, x21, lsl #3
    tbnz    x22, #63, Lmod_normalised

// unnormalised: d = x << k, m = inv(d) - 2^64
Lmod_unnorm:
    clz     x23, x22
    lsl     x0, x22, x23
    bl      ASM_GSYM(lmmp_inv_1_)

// x22 = d; x11 = xh = a_top>>(64-k), x1 = a_lo = a_top<<k
Lmod_uentry:
    lsl     x22, x22, x23
    ldr     x7, [x20], #-8          // top limb
    sub     x8, xzr, x23
    lsr     x11, x7, x8
    lsl     x1, x7, x23
    cbz     x21, Lmod_uend

// main loop (unnormalised): x11 = xh < d, x1 = a_lo
Lmod_utop:
    ldr     x7, [x20], #-8
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    lsr     x9, x7, x8
    orr     x1, x1, x9              // a_lo = (prev<<k)|(x7>>(64-k))
    adds    x10, x1, x10
    adc     x2, x2, x17             // x2 = q
    msub    x11, x22, x2, x1        // z = a_lo - d*q
    lsl     x1, x7, x23
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc       // borrow: z += d
    sbc     x2, x2, xzr             // borrow: q--
    cmp     x11, x22
    bcs     Lmod_ufx

Lmod_uok:                           // no quotient stored
    sub     x21, x21, #1
    cbnz    x21, Lmod_utop

// last limb, second fixup
Lmod_uend:
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    adds    x10, x1, x10
    adc     x2, x2, x17
    msub    x11, x22, x2, x1
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc
    sbc     x2, x2, xzr
    subs    x14, x11, x22           // z >= d: z -= d
    adc     x2, x2, xzr
    csel    x11, x14, x11, cs
    lsr     x0, x11, x23            // r = z >> k
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

Lmod_ufx:                           // z >= d: z -= d
    add     x2, x2, #1
    sub     x11, x11, x22
    b       Lmod_uok

// normalised path, k = 0
Lmod_normalised:
    mov     x0, x22
    bl      ASM_GSYM(lmmp_inv_1_)

Lmod_nentry:
    ldr     x7, [x20], #-8          // top limb
    subs    x14, x7, x22
    csel    x11, x14, x7, cs        // x11 = xh
    b       Lmod_nok

// main loop (normalised)
Lmod_ntop:
    ldr     x1, [x20], #-8
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    adds    x10, x1, x10
    adc     x2, x2, x17
    msub    x11, x22, x2, x1
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc
    sbc     x2, x2, xzr
    cmp     x11, x22
    bcs     Lmod_nfx

Lmod_nok:                           // no quotient stored
    sub     x21, x21, #1
    tbz     x21, #63, Lmod_ntop
    mov     x0, x11                 // ret remainder
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

Lmod_nfx:                           // z >= d: z -= d
    add     x2, x2, #1
    sub     x11, x11, x22
    b       Lmod_nok
