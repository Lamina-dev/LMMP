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

// mp_limb_t lmmp_addshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
//   dst=x0 numa=x1 numb=x2 n=x3
//   ret x0 = numa + (numb << 1) 's carry
//   cal: dst[i] = numa[i] + (numb[i]<<1 | numb[i-1]>>63) + carry

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_addshl1_n_)
.p2align 2

ASM_GSYM(lmmp_addshl1_n_):
    lsr     x6, x3, #2              // 4-limbs size
    tbz     x3, #0, Lashl_bx0       // n&3 choose path

Lashl_bx1:
    ldr     x5, [x1]                // a0 = numa[0]
    tbnz    x3, #1, Lashl_b11

Lashl_b01:                          // n % 4 == 1
    ldr     x11, [x2]               // b0 = numb[0]
    cbz     x6, Lashl_1
    ldp     x8, x9, [x2, #8]
    lsl     x13, x11, #1            // t = b0 << 1
    adds    x15, x13, x5            // dst[0] = a0 + t, CF
    str     x15, [x0], #8
    sub     x1, x1, #24
    sub     x2, x2, #8
    b       Lashl_mid

Lashl_1:                            // n == 1
    lsl     x13, x11, #1
    adds    x15, x13, x5
    str     x15, [x0]
    lsr     x0, x11, #63            // ret = (b0>>63) + carry
    adc     x0, x0, xzr
    ret

Lashl_b11:                          // n % 4 == 3
    ldr     x9, [x2]
    ldp     x10, x11, [x2, #8]!
    lsl     x13, x9, #1
    adds    x17, x13, x5            // dst[0] = a0 + (b0<<1), CF
    str     x17, [x0], #8
    sub     x1, x1, #8
    cbz     x6, Lashl_end
    b       Lashl_top

Lashl_bx0:
    tbnz    x3, #1, Lashl_b10

Lashl_b00:                          // n % 4 == 0
    adds    x11, xzr, xzr           // CF = 0
    ldp     x8, x9, [x2], #-16
    sub     x1, x1, #32
    b       Lashl_mid

Lashl_b10:                          // n % 4 == 2
    adds    x9, xzr, xzr            // CF = 0
    ldp     x10, x11, [x2]
    sub     x1, x1, #16
    cbz     x6, Lashl_end
.p2align 4

Lashl_top:                          // main loop (upper): extr + adcs
    ldp     x4, x5, [x1, #16]
    extr    x12, x10, x9, #63       // t = (b<<1) | (b_prev>>63)
    ldp     x8, x9, [x2, #16]
    extr    x13, x11, x10, #63
    adcs    x14, x12, x4            // dst = a + t + CF
    adcs    x15, x13, x5
    stp     x14, x15, [x0], #16

Lashl_mid:                          // main loop (lower)
    ldp     x4, x5, [x1, #32]!
    extr    x12, x8, x11, #63
    ldp     x10, x11, [x2, #32]!
    extr    x13, x9, x8, #63
    adcs    x16, x12, x4
    adcs    x17, x13, x5
    stp     x16, x17, [x0], #16
    sub     x6, x6, #1
    cbnz    x6, Lashl_top

Lashl_end:                          // last 2 limb, ret = (b[n-1]>>63) + carry
    ldp     x4, x5, [x1, #16]
    extr    x12, x10, x9, #63
    extr    x13, x11, x10, #63
    adcs    x14, x12, x4
    adcs    x15, x13, x5
    stp     x14, x15, [x0]
    lsr     x0, x11, #63
    adc     x0, x0, xzr
    ret
