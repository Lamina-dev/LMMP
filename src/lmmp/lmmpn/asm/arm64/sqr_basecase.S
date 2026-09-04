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

// void lmmp_sqr_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
//
//   x0=dst, x1=numa, x2=na
//   ret: none
//   cal: dst[0..2na) = numa^2 = sum a_i^2 + 2*sum a_i*a_j

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_sqr_basecase_)
.p2align 2

ASM_GSYM(lmmp_sqr_basecase_):
    cmp     x2, #3
    b.ls    Lsqr_le3                 // na <= 3 small path
    ldr     x3, [x1],#8              // a0
    sub     x2, x2, #1               // remain = na - 1
    mul     x6, x3, x3               // lo(a0^2)
    umulh   x4, x3, x3               // hi(a0^2)
    str     x6, [x0],#8              // dst[0] = lo(a0^2)
    lsl     x3, x3, 1                // multiplier = 2a0
    lsl     x2, x2, #3               // x2 = byte length
    lsr     x16, x2, #5              // x16 = size >> 2
    tbnz    x2, #3, Lsqr_mbx1        // n mod 2 == 1

Lsqr_mbx0:
    adds    x11, x4, xzr             // entry carry, C = 0
    tbz     x2, #4, Lsqr_mb00

Lsqr_mb10:                           // n mod 4 == 2
    ldp     x4, x5, [x1],#16         // a1, a2
    mul     x8, x4, x3               // lo_0
    umulh   x10, x4, x3              // hi_0
    cbz     x16, Lsqr_m2e            // n == 2, tail
    ldp     x6, x7, [x1],#16         // prefetch a3, a4
    mul     x9, x5, x3               // lo_1
    b       Lsqr_mmid-8              // enter mid loop

Lsqr_mbx1:                           // leading single limb
    ldr     x7, [x1],#8
    mul     x9, x7, x3               // lo_0
    umulh   x11, x7, x3              // hi_0
    adds    x9, x9, x4               // s_0 = lo_0 + carry
    str     x9, [x0],#8
    tbnz    x2, #4, Lsqr_mb10        // n mod 4 == 2

Lsqr_mb00:                           // n mod 4 == 0
    ldp     x6, x7, [x1],#16         // a1, a2
    mul     x8, x6, x3               // lo_0
    umulh   x10, x6, x3              // hi_0
    ldp     x4, x5, [x1],#16         // prefetch a3, a4
    mul     x9, x7, x3               // lo_1
    adcs    x12, x8, x11             // s_0 = lo_0 + carry + C
    umulh   x11, x7, x3              // hi_1
    sub     x16, x16, #1
    cbz     x16, Lsqr_mend           // last pair only
.p2align 4

Lsqr_mtop:                           // main loop (upper)
    mul     x8, x4, x3               // lo_2
    ldp     x6, x7, [x1],#16         // next pair
    adcs    x13, x9, x10             // s_1 = lo_1 + hi_0 + C
    umulh   x10, x4, x3              // hi_2
    mul     x9, x5, x3               // lo_3
    stp     x12, x13, [x0],#16       // store s_0, s_1
    adcs    x12, x8, x11             // s_2 = lo_2 + hi_1 + C
    umulh   x11, x5, x3              // hi_3

Lsqr_mmid:                           // main loop (lower)
    mul     x8, x6, x3
    ldp     x4, x5, [x1],#16
    adcs    x13, x9, x10
    umulh   x10, x6, x3
    mul     x9, x7, x3
    stp     x12, x13, [x0],#16
    adcs    x12, x8, x11
    umulh   x11, x7, x3
    sub     x16, x16, #1
    cbnz    x16, Lsqr_mtop

Lsqr_mend:                           // tail: last prefetched pair
    mul     x8, x4, x3               // lo_0
    adcs    x13, x9, x10             // s_1 = lo_1 + hi_0 + C
    umulh   x10, x4, x3              // hi_0
    stp     x12, x13, [x0],#16

Lsqr_m2e:                            // 2-limb tail
    mul     x9, x5, x3               // lo_1
    adcs    x12, x8, x11             // s_0 = lo_0 + hi_prev + C
    umulh   x11, x5, x3              // hi_1
    adcs    x13, x9, x10             // s_1 = lo_1 + hi_0 + C
    stp     x12, x13, [x0],#16
    adc     x11, x11, xzr            // top carry
    str     x11, [x0],#8

Lsqr_outer:                          // outer loop: top limb a_{L-1}
    sub     x2, x2, #8               // byte length -= 8
    sub     x0, x0, x2               // x0 = dst + L
    sub     x1, x1, x2               // x1 = numa + L
    ldp     x6, x7, [x1,#-16]        // a_{L-2}, a_{L-1}
    ldr     x3, [x0,#-8]             // dst[L-1]
    and     x8, x7, x6, asr 63       // fix 2a_{L-2} overflow
    mul     x9, x7, x7               // lo(a_{L-1}^2)
    adds    x3, x3, x8               // overflow fix
    umulh   x4, x7, x7               // hi(a_{L-1}^2)
    adc     x4, x4, xzr
    adds    x3, x3, x9               // dst[L-1] += lo(a_{L-1}^2)
    str     x3, [x0,#-8]
    adc     x17, x4, xzr             // carry = hi + C
    adds    xzr, x6, x6              // C = 2a_{L-2} overflow
    adc     x3, x7, x7               // multiplier = 2a_{L-1}
    cmp     x2, #16
    beq     Lsqr_cor2                // 2 limbs left
    lsr     x16, x2, #5              // 4-limb size
    tbz     x2, #3, Lsqr_bx0

Lsqr_bx1:                            // odd remain: leading limb
    ldr     x4, [x1],#8
    mul     x8, x4, x3               // lo_0
    umulh   x4, x4, x3               // hi_0
    tbz     x2, #4, Lsqr_b01

Lsqr_b11:                            // n mod 4 == 3
    ldp     x5, x6, [x1],#16
    ldp     x12, x13, [x0]           // rp[0], rp[1]
    ldr     x14, [x0,#16]            // rp[2]
    mul     x9, x5, x3               // lo_1
    umulh   x5, x5, x3               // hi_1
    mul     x10, x6, x3              // lo_2
    umulh   x6, x6, x3               // hi_2
    adds    x8, x12, x8              // s_0 = rp[0] + lo_0
    adcs    x4, x13, x4              // s_1 = rp[1] + hi_0 + C
    adcs    x5, x14, x5              // s_2 = rp[2] + hi_1 + C
    adc     x6, x6, xzr              // tmp carry
    adds    x8, x8, x17              // s_0 += outer carry
    adcs    x4, x4, x9               // s_1 += lo_1 + C
    adcs    x5, x5, x10              // s_2 += lo_2 + C
    adc     x17, x6, xzr             // new carry
    stp     x8, x4, [x0],#16
    str     x5, [x0],#8
    cbnz    x16, Lsqr_top
    b       Lsqr_end

Lsqr_b01:                            // single leading limb
    ldr     x12, [x0]
    adds    x8, x12, x8              // s_0 = rp[0] + lo_0
    adc     x4, x4, xzr
    adds    x8, x8, x17              // s_0 += outer carry
    adc     x17, x4, xzr
    str     x8, [x0],#8
    b       Lsqr_top

Lsqr_bx0:                            // even remain
    ldp     x4, x5, [x1],#16         // prefetch a0, a1
    tbz     x2, #4, Lsqr_top+4       // enter mid loop

Lsqr_b10:                            // n mod 4 == 2
    ldp     x12, x13, [x0]           // rp[0], rp[1]
    mul     x8, x4, x3               // lo_0
    umulh   x4, x4, x3               // hi_0
    mul     x9, x5, x3               // lo_1
    umulh   x5, x5, x3               // hi_1
    adds    x8, x12, x8              // s_0 = rp[0] + lo_0
    adcs    x4, x13, x4              // s_1 = rp[1] + hi_0 + C
    adc     x5, x5, xzr              // tmp carry
    adds    x8, x8, x17              // s_0 += outer carry
    adcs    x4, x4, x9               // s_1 += lo_1 + C
    adc     x17, x5, xzr             // new carry
    stp     x8, x4, [x0],#16
.p2align 4

Lsqr_top:                            // addmul main loop: rp[j] += numa[j]*2a_i
    ldp     x4, x5, [x1],#16
    ldp     x6, x7, [x1],#16
    ldp     x12, x13, [x0]           // rp[0], rp[1]
    ldp     x14, x15, [x0,#16]       // rp[2], rp[3]
    mul     x8, x4, x3               // lo_0
    umulh   x4, x4, x3               // hi_0
    mul     x9, x5, x3               // lo_1
    umulh   x5, x5, x3               // hi_1
    mul     x10, x6, x3              // lo_2
    umulh   x6, x6, x3               // hi_2
    mul     x11, x7, x3              // lo_3
    umulh   x7, x7, x3               // hi_3
    adds    x8, x12, x8              // s_0 = rp[0] + lo_0
    adcs    x4, x13, x4              // s_1 = rp[1] + hi_0 + C
    adcs    x5, x14, x5              // s_2 = rp[2] + hi_1 + C
    adcs    x6, x15, x6              // s_3 = rp[3] + hi_2 + C
    adc     x7, x7, xzr              // tmp carry
    adds    x8, x8, x17              // s_0 += outer carry
    adcs    x4, x4, x9               // s_1 += lo_1 + C
    adcs    x5, x5, x10              // s_2 += lo_2 + C
    adcs    x6, x6, x11              // s_3 += lo_3 + C
    adc     x17, x7, xzr             // new carry
    stp     x8, x4, [x0],#16
    stp     x5, x6, [x0],#16
    sub     x16, x16, #1
    cbnz    x16, Lsqr_top

Lsqr_end:
    str     x17, [x0],#8             // carry -> dst top
    b       Lsqr_outer

Lsqr_cor2:                           // corner: last two limbs
    ldp     x10, x11, [x1]           // a0, a1
    ldp     x12, x13, [x0]           // rp[0], rp[1]
    mul     x8, x10, x3              // lo(a0*2a1)
    umulh   x4, x10, x3              // hi(a0*2a1)
    mul     x9, x11, x3              // lo(a1*2a1)
    umulh   x5, x11, x3              // hi(a1*2a1)
    adds    x8, x12, x8              // s_0 = rp[0] + lo
    adcs    x4, x13, x4              // s_1 = rp[1] + hi_0 + C
    adc     x5, x5, xzr
    adds    x8, x8, x17              // s_0 += carry
    adcs    x13, x4, x9              // s_1 += lo_1 + C
    adc     x12, x5, xzr
    str     x8, [x0]
    and     x8, x10, x7, asr 63      // fix 2a0 overflow
    mul     x9, x10, x10             // lo(a0^2)
    adds    x13, x13, x8             // dst[1] += fix
    umulh   x4, x10, x10             // hi(a0^2)
    adc     x4, x4, xzr
    adds    x13, x13, x9
    adc     x17, x4, xzr             // carry
    adds    xzr, x7, x7              // C = 2a0 overflow
    adc     x3, x10, x10             // multiplier = 2a0
    mul     x8, x11, x3              // lo(a1*2a0)
    umulh   x4, x11, x3              // hi(a1*2a0)
    adds    x8, x12, x8              // dst[2] += lo
    adc     x4, x4, xzr
    adds    x8, x8, x17              // += carry
    adc     x3, x4, xzr
    stp     x13, x8, [x0,#8]         // store dst[1], dst[2]
    and     x2, x11, x10, asr 63     // fix overflow
    mul     x5, x11, x11             // lo(a1^2)
    adds    x3, x3, x2
    umulh   x4, x11, x11             // hi(a1^2)
    adc     x4, x4, xzr
    adds    x3, x3, x5               // dst[3] += lo(a1^2)
    adc     x4, x4, xzr
    stp     x3, x4, [x0,#24]         // store dst[3], dst[4]
    ret

Lsqr_le3:                            // na == 1: dst = a0^2
    ldr     x3, [x1]
    mul     x4, x3, x3               // lo(a0^2)
    umulh   x5, x3, x3               // hi(a0^2)
    cmp     x2, #2
    b.hs    Lsqr_2o3
    stp     x4, x5, [x0]
    ret

Lsqr_2o3:                            // na == 2: a0^2 | 2a0a1 | a1^2
    ldr     x6, [x1,#8]
    mul     x7, x6, x6               // lo(a1^2)
    umulh   x8, x6, x6               // hi(a1^2)
    mul     x9, x3, x6               // lo(a0a1)
    umulh   x10, x3, x6              // hi(a0a1)
    b.hi    Lsqr_3
    adds    x5, x5, x9               // += cross terms
    adcs    x7, x7, x10
    adc     x8, x8, xzr
    adds    x5, x5, x9               // again: x2
    adcs    x7, x7, x10
    adc     x8, x8, xzr
    stp     x4, x5, [x0]
    stp     x7, x8, [x0,#16]
    ret

Lsqr_3:                              // na == 3: squares + 2*cross
    ldr     x11, [x1,#16]
    mul     x12, x11, x11            // lo(a2^2)
    umulh   x13, x11, x11            // hi(a2^2)
    mul     x14, x3, x11             // lo(a0a2)
    umulh   x15, x3, x11             // hi(a0a2)
    mul     x16, x6, x11             // lo(a1a2)
    umulh   x17, x6, x11             // hi(a1a2)
    adds    x5, x5, x9               // cross x2
    adcs    x7, x7, x10
    adcs    x8, x8, x15
    adcs    x12, x12, x17
    adc     x13, x13, xzr
    adds    x5, x5, x9
    adcs    x7, x7, x10
    adcs    x8, x8, x15
    adcs    x12, x12, x17
    adc     x13, x13, xzr
    adds    x7, x7, x14              // += a0a2, a1a2
    adcs    x8, x8, x16
    adcs    x12, x12, xzr
    adc     x13, x13, xzr
    adds    x7, x7, x14
    adcs    x8, x8, x16
    adcs    x12, x12, xzr
    adc     x13, x13, xzr
    stp     x4, x5, [x0]
    stp     x7, x8, [x0,#16]
    stp     x12, x13, [x0,#32]
    ret
