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

// void lmmp_mod_2_(mp_srcptr numa, mp_size_t na, mp_ptr numb);
//
//   x0=numa, x1=na, x2=numb
//   ret [numb,2] = [numa,na] mod [numb,2]
//   cal: as div_2_, no quotient

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_mod_2_)
.p2align 2

// x19=numb, x20=numa, x21=bl, x22=bh, x24/x25=k
ASM_GSYM(lmmp_mod_2_):
    stp     x28, x27, [sp, #-96]!
    stp     x26, x25, [sp, #16]
    stp     x24, x23, [sp, #32]
    stp     x22, x21, [sp, #48]
    stp     x20, x19, [sp, #64]
    stp     x29, x30, [sp, #80]
    add     x29, sp, #80
    mov     x19, x2
    mov     x20, x0
    ldp     x21, x22, [x2]
    clz     x24, x22
    subs    x26, x1, #2
    b.ne    LBB3_3
    cbz     x24, LBB3_7
    mov     w8, #64
    sub     x23, x8, x24
    lsl     x8, x22, x24
    lsr     x9, x21, #1
    mvn     w10, w24
    lsr     x9, x9, x10
    orr     x22, x8, x9
    lsl     x21, x21, x24
    neg     x8, x24
    ldp     x11, x9, [x20]
    lsr     x20, x9, x8
    lsl     x8, x9, x24
    lsr     x9, x11, #1
    lsr     x9, x9, x10
    orr     x25, x8, x9
    lsl     x26, x11, x24
    mov     x0, x22
    mov     x1, x21
    bl      ASM_GSYM(lmmp_inv_2_1_)
    mul     x8, x0, x20
    umulh   x9, x0, x20
    adds    x8, x25, x8
    adc     x9, x20, x9
    mvn     x10, x9
    subs    x11, x26, x21
    mul     x12, x9, x21
    umulh   x9, x9, x21
    sbc     x13, x25, xzr
    subs    x11, x11, x12
    madd    x10, x22, x10, x13
    sbc     x9, x10, x9
    cmp     x9, x8
    csel    x8, xzr, x21, lo
    csel    x10, xzr, x22, lo
    adds    x8, x8, x11
    adc     x9, x10, x9
    subs    x10, x8, x21
    csetm   x11, lo
    sub     x11, x11, x22
    add     x11, x11, x9
    cmp     x8, x21
    ccmp    x9, x22, #2, lo
    csel    x10, x8, x10, ls
    csel    x11, x9, x11, ls
    cmp     x9, x22
    csel    x8, x8, x10, lo
    csel    x9, x9, x11, lo
    lsr     x8, x8, x24
    lsl     x10, x9, x23
    orr     x8, x10, x8
    str     x8, [x19]
    lsr     x8, x9, x24
    b       LBB3_24

// na>2, unnormalised: d = b << k
LBB3_3:
    mov     x23, x1
    cbz     x24, LBB3_10
    mov     w8, #64
    sub     x25, x8, x24
    lsl     x8, x22, x24
    lsr     x9, x21, #1
    mvn     w27, w24
    lsr     x9, x9, x27
    orr     x22, x8, x9
    lsl     x21, x21, x24
    mov     x0, x22
    mov     x1, x21
    bl      ASM_GSYM(lmmp_inv_2_1_)
    add     x8, x20, x23, lsl #3
    ldur    x9, [x8, #-8]
    neg     x10, x24
    lsr     x10, x9, x10
    ldr     x11, [x20, x26, lsl #3]
    lsr     x12, x11, #1
    lsr     x12, x12, x27
    lsl     x9, x9, x24
    orr     x9, x9, x12
    ldur    x8, [x8, #-24]
    lsr     x8, x8, #1
    lsr     x8, x8, x27
    lsl     x11, x11, x24
    orr     x8, x11, x8
    mul     x11, x10, x0
    umulh   x12, x10, x0
    adds    x11, x9, x11
    adc     x10, x10, x12
    mvn     x12, x10
    subs    x8, x8, x21
    mul     x13, x10, x21
    umulh   x10, x10, x21
    sbc     x9, x9, xzr
    subs    x8, x8, x13
    madd    x9, x22, x12, x9
    sbc     x9, x9, x10
    cmp     x9, x11
    csel    x10, xzr, x21, lo
    csel    x11, xzr, x22, lo
    adds    x8, x10, x8
    adc     x9, x11, x9
    subs    x10, x8, x21
    csetm   x11, lo
    sub     x11, x11, x22
    add     x11, x11, x9
    cmp     x8, x21
    ccmp    x9, x22, #2, lo
    csel    x10, x8, x10, ls
    csel    x11, x9, x11, ls
    cmp     x9, x22
    csel    x8, x8, x10, lo
    csel    x9, x9, x11, lo
    sub     x10, x23, #3
    cmp     x26, #2
    b.lo    LBB3_23
    tbnz    w10, #0, LBB3_19
    sub     x12, x23, #3
    mov     x10, x26
    cmp     x23, #4
    b.ne    LBB3_20
    b       LBB3_22

// na==2 normalised: 128-bit compare/sub
LBB3_7:
    ldp     x8, x9, [x20]
    cmp     x9, x22
    b.lo    LBB3_18
    ccmp    x8, x21, #2, eq
    b.lo    LBB3_18
    subs    x8, x8, x21
    str     x8, [x19]
    ldr     x8, [x20, #8]
    sbc     x8, x8, x22
    b       LBB3_24

// normalised path
LBB3_10:
    mov     x0, x22
    mov     x1, x21
    bl      ASM_GSYM(lmmp_inv_2_1_)
    add     x8, x20, x23, lsl #3
    ldur    x8, [x8, #-8]
    ldr     x9, [x20, x26, lsl #3]
    subs    x10, x9, x21
    sbc     x11, x8, x22
    cmp     x11, x8
    csel    x8, x10, x9, lo
    csel    x9, xzr, x22, lo
    add     x9, x9, x11
    cmp     x8, x10
    cinc    x9, x9, lo
    subs    x10, x8, x21
    csetm   x11, lo
    sub     x11, x11, x22
    add     x11, x11, x9
    cmp     x9, x22
    ccmp    x8, x21, #2, ls
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    cmp     x9, x22
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    sub     x9, x23, #3
    cmp     x26, #2
    b.lo    LBB3_17
    tbz     w9, #0, LBB3_13
    ldr     x8, [x20, x9, lsl #3]
    mul     x9, x11, x0
    umulh   x12, x11, x0
    adds    x9, x10, x9
    adc     x11, x11, x12
    mvn     x12, x11
    madd    x10, x22, x12, x10
    subs    x8, x8, x21
    cset    w12, lo
    mul     x13, x11, x21
    umulh   x11, x11, x21
    sub     x10, x10, x11
    subs    x8, x8, x13
    sbc     x10, x10, x12
    cmp     x10, x9
    csel    x9, xzr, x21, lo
    csel    x11, xzr, x22, lo
    adds    x8, x9, x8
    adc     x9, x11, x10
    subs    x10, x8, x21
    csetm   x11, lo
    sub     x11, x11, x22
    add     x11, x11, x9
    cmp     x9, x22
    ccmp    x8, x21, #2, ls
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    cmp     x9, x22
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    sub     x9, x23, #4

// normalised unrolled loop entry
LBB3_13:
    cmp     x23, #4
    b.eq    LBB3_16
    add     x8, x9, #1
    add     x9, x20, x9, lsl #3

// divide step x2: q = hi(m*r_hi)+r_hi+a_lo; z = a - q*d
LBB3_15:
    mul     x12, x11, x0
    umulh   x13, x11, x0
    adds    x12, x10, x12
    adc     x11, x11, x13
    mvn     x13, x11
    madd    x10, x22, x13, x10
    ldp     x14, x13, [x9, #-8]
    subs    x13, x13, x21
    umulh   x15, x11, x21
    sub     x10, x10, x15
    cset    w15, lo
    mul     x11, x11, x21
    subs    x11, x13, x11
    sbc     x10, x10, x15
    cmp     x10, x12
    csel    x12, xzr, x21, lo
    csel    x13, xzr, x22, lo
    adds    x11, x12, x11
    adc     x10, x13, x10
    subs    x12, x11, x21
    csetm   x13, lo
    sub     x13, x13, x22
    add     x13, x13, x10
    cmp     x10, x22
    ccmp    x11, x21, #2, ls
    csel    x12, x11, x12, lo
    csel    x13, x10, x13, lo
    cmp     x10, x22
    csel    x11, x11, x12, lo
    csel    x10, x10, x13, lo
    mul     x12, x10, x0
    umulh   x13, x10, x0
    adds    x12, x11, x12
    adc     x10, x10, x13
    mvn     x13, x10
    madd    x11, x22, x13, x11
    subs    x13, x14, x21
    cset    w14, lo
    mul     x15, x10, x21
    umulh   x10, x10, x21
    sub     x10, x11, x10
    subs    x11, x13, x15
    sbc     x10, x10, x14
    cmp     x10, x12
    csel    x12, xzr, x21, lo
    csel    x13, xzr, x22, lo
    adds    x11, x12, x11
    adc     x12, x13, x10
    sub     x9, x9, #16
    subs    x10, x11, x21
    csetm   x13, lo
    sub     x13, x13, x22
    add     x13, x13, x12
    cmp     x12, x22
    ccmp    x11, x21, #2, ls
    csel    x10, x11, x10, lo
    csel    x13, x12, x13, lo
    cmp     x12, x22
    csel    x10, x11, x10, lo
    csel    x11, x12, x13, lo
    sub     x8, x8, #2
    cmp     x8, #1
    b.hi    LBB3_15

LBB3_16:
    mov     x9, #0

// normalised: last step
LBB3_17:
    ldr     x8, [x20, x9, lsl #3]
    mul     x9, x0, x11
    umulh   x12, x0, x11
    adds    x9, x10, x9
    adc     x11, x11, x12
    mvn     x12, x11
    madd    x10, x22, x12, x10
    subs    x8, x8, x21
    cset    w12, lo
    mul     x13, x11, x21
    umulh   x11, x11, x21
    sub     x10, x10, x11
    subs    x8, x8, x13
    sbc     x10, x10, x12
    cmp     x10, x9
    csel    x9, xzr, x21, lo
    csel    x11, xzr, x22, lo
    adds    x8, x9, x8
    adc     x9, x11, x10
    subs    x10, x8, x21
    csetm   x11, lo
    sub     x11, x11, x22
    add     x11, x11, x9
    cmp     x9, x22
    ccmp    x8, x21, #2, ls
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    cmp     x9, x22
    csel    x10, x8, x10, lo
    csel    x8, x9, x11, lo
    str     x10, [x19]
    b       LBB3_24

// a < b: r = a
LBB3_18:
    str     x8, [x19]
    ldr     x8, [x20, #8]
    b       LBB3_24

// first step, odd adjust; a_lo = lo<<k|hi>>(64-k)
LBB3_19:
    ldr     x11, [x20, x10, lsl #3]
    lsl     x11, x11, x24
    add     x12, x20, x26, lsl #3
    ldur    x12, [x12, #-16]
    lsr     x12, x12, x25
    orr     x11, x12, x11
    mul     x12, x9, x0
    umulh   x13, x9, x0
    adds    x12, x8, x12
    adc     x9, x9, x13
    mvn     x13, x9
    madd    x8, x22, x13, x8
    subs    x11, x11, x21
    cset    w13, lo
    mul     x14, x9, x21
    umulh   x9, x9, x21
    sub     x8, x8, x9
    subs    x9, x11, x14
    sbc     x8, x8, x13
    cmp     x8, x12
    csel    x11, xzr, x21, lo
    csel    x12, xzr, x22, lo
    adds    x9, x11, x9
    adc     x11, x12, x8
    subs    x8, x9, x21
    csetm   x12, lo
    sub     x12, x12, x22
    add     x12, x12, x11
    cmp     x9, x21
    ccmp    x11, x22, #2, lo
    csel    x8, x9, x8, ls
    csel    x12, x11, x12, ls
    cmp     x11, x22
    csel    x8, x9, x8, lo
    csel    x9, x11, x12, lo
    sub     x12, x23, #4
    cmp     x23, #4
    b.eq    LBB3_22

// unnormalised 2x unrolled loop
LBB3_20:
    add     x11, x20, x12, lsl #3
    add     x12, x12, #1

// divide step x2, shifted a_lo
LBB3_21:
    ldr     x13, [x11]
    lsl     x13, x13, x24
    add     x10, x20, x10, lsl #3
    ldur    x10, [x10, #-16]
    lsr     x10, x10, x25
    orr     x10, x10, x13
    mul     x13, x9, x0
    umulh   x14, x9, x0
    adds    x13, x8, x13
    adc     x9, x9, x14
    mvn     x14, x9
    madd    x8, x22, x14, x8
    subs    x10, x10, x21
    cset    w14, lo
    mul     x15, x9, x21
    umulh   x9, x9, x21
    sub     x8, x8, x9
    subs    x9, x10, x15
    sbc     x8, x8, x14
    cmp     x8, x13
    csel    x10, xzr, x21, lo
    csel    x13, xzr, x22, lo
    adds    x9, x10, x9
    adc     x8, x13, x8
    subs    x10, x9, x21
    csetm   x13, lo
    sub     x13, x13, x22
    add     x13, x13, x8
    cmp     x9, x21
    ccmp    x8, x22, #2, lo
    csel    x10, x9, x10, ls
    csel    x13, x8, x13, ls
    cmp     x8, x22
    csel    x9, x9, x10, lo
    csel    x8, x8, x13, lo
    ldp     x10, x13, [x11, #-16]!
    sub     x12, x12, #2
    lsl     x13, x13, x24
    lsr     x10, x10, x25
    orr     x10, x10, x13
    mul     x13, x8, x0
    umulh   x14, x8, x0
    adds    x13, x9, x13
    adc     x8, x8, x14
    mvn     x14, x8
    madd    x9, x22, x14, x9
    subs    x10, x10, x21
    cset    w14, lo
    mul     x15, x8, x21
    umulh   x8, x8, x21
    sub     x8, x9, x8
    subs    x9, x10, x15
    sbc     x8, x8, x14
    cmp     x8, x13
    csel    x10, xzr, x21, lo
    csel    x13, xzr, x22, lo
    adds    x9, x10, x9
    adc     x10, x13, x8
    subs    x8, x9, x21
    csetm   x13, lo
    sub     x13, x13, x22
    add     x13, x13, x10
    cmp     x9, x21
    ccmp    x10, x22, #2, lo
    csel    x8, x9, x8, ls
    csel    x13, x10, x13, ls
    cmp     x10, x22
    csel    x8, x9, x8, lo
    csel    x9, x10, x13, lo
    mov     x10, x12
    cmp     x12, #1
    b.hi    LBB3_21

LBB3_22:
    mov     x10, #0

// unnormalised: last step + r >> k
LBB3_23:
    ldr     x10, [x20, x10, lsl #3]
    lsl     x10, x10, x24
    mul     x11, x9, x0
    umulh   x12, x9, x0
    adds    x11, x8, x11
    adc     x9, x9, x12
    mvn     x12, x9
    madd    x8, x22, x12, x8
    subs    x10, x10, x21
    cset    w12, lo
    mul     x13, x9, x21
    umulh   x9, x9, x21
    sub     x8, x8, x9
    subs    x9, x10, x13
    sbc     x8, x8, x12
    cmp     x8, x11
    csel    x10, xzr, x21, lo
    csel    x11, xzr, x22, lo
    adds    x9, x10, x9
    adc     x8, x11, x8
    subs    x10, x9, x21
    csetm   x11, lo
    sub     x11, x11, x22
    add     x11, x11, x8
    cmp     x9, x21
    ccmp    x8, x22, #2, lo
    csel    x10, x9, x10, ls
    csel    x11, x8, x11, ls
    cmp     x8, x22
    csel    x9, x9, x10, lo
    csel    x8, x8, x11, lo
    lsr     x9, x9, x24
    lsl     x10, x8, x25
    orr     x9, x10, x9
    str     x9, [x19]
    lsr     x8, x8, x24

// remainder hi limb, restore and return
LBB3_24:
    str     x8, [x19, #8]
    ldp     x29, x30, [sp, #80]
    ldp     x20, x19, [sp, #64]
    ldp     x22, x21, [sp, #48]
    ldp     x24, x23, [sp, #32]
    ldp     x26, x25, [sp, #16]
    ldp     x28, x27, [sp], #96
    ret
