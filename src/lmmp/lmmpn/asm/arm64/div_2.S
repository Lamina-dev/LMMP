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

// void lmmp_div_2_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_ptr numb);
// mp_limb_t lmmp_div_2_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_srcptr numb);
//
//   x0=dstq, x1=numa, x2=na, x3=numb
//   ret x0 = qh (div_2_s_)
//   cal: d = b << k, m = inv_2_1_(d)
//      q = hi(m*r_hi) + r_hi + a_lo
//      z = a - q*d, two fixups, r >>= k

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_div_2_)
.p2align 2

// x19=numb, x20=numa, x21=dstq, x22=bl, x23=bh
// na==2 -> single step; else LBB4_5
ASM_GSYM(lmmp_div_2_):
    stp     x28, x27, [sp, #-96]!
    stp     x26, x25, [sp, #16]
    stp     x24, x23, [sp, #32]
    stp     x22, x21, [sp, #48]
    stp     x20, x19, [sp, #64]
    stp     x29, x30, [sp, #80]
    add     x29, sp, #80
    mov     x19, x3
    mov     x20, x1
    mov     x21, x0
    ldp     x22, x23, [x3]
    subs    x27, x2, #2
    b.ne    LBB4_5
    clz     x24, x23
    cbz     x24, LBB4_10
    mov     w8, #64
    sub     x25, x8, x24
    lsl     x8, x23, x24
    lsr     x9, x22, #1
    mvn     w10, w24
    lsr     x9, x9, x10
    orr     x23, x8, x9
    lsl     x22, x22, x24
    neg     x8, x24
    ldp     x11, x9, [x20]
    lsr     x20, x9, x8
    lsl     x8, x9, x24
    lsr     x9, x11, #1
    lsr     x9, x9, x10
    orr     x26, x8, x9
    lsl     x27, x11, x24
    mov     x0, x23
    mov     x1, x22
    bl      ASM_GSYM(lmmp_inv_2_1_)
    mul     x8, x0, x20
    umulh   x9, x0, x20
    adds    x8, x26, x8
    adc     x9, x20, x9
    mvn     x10, x9
    subs    x11, x27, x22
    mul     x12, x9, x22
    umulh   x13, x9, x22
    sbc     x14, x26, xzr
    subs    x11, x11, x12
    madd    x10, x23, x10, x14
    sbc     x10, x10, x13
    cmp     x10, x8
    csel    x8, x22, xzr, hs
    cset    w12, hs
    csel    x13, x23, xzr, hs
    sub     x12, x9, x12
    add     x9, x12, #1
    adds    x8, x8, x11
    adc     x11, x13, x10
    add     x9, x9, #1
    subs    x10, x8, x22
    csetm   x13, lo
    sub     x13, x13, x23
    add     x13, x13, x11
    cmp     x8, x22
    ccmp    x11, x23, #2, lo
    csinc   x14, x9, x12, hi
    csel    x9, x8, x10, ls
    csel    x13, x11, x13, ls
    cmp     x11, x23
    csel    x9, x8, x9, lo
    csinc   x10, x14, x12, hs
    csel    x8, x11, x13, lo
    cbz     x21, LBB4_4
    str     x10, [x21]

// unshift: r >>= k, store [numb,2]
LBB4_4:
    lsr     x9, x9, x24
    lsl     x10, x8, x25
    orr     x9, x10, x9
    lsr     x8, x8, x24
    str     x9, [x19]
    b       LBB4_47

// na>2: split by dstq, k
LBB4_5:
    mov     x24, x2
    clz     x25, x23
    cbz     x21, LBB4_14
    cbz     x25, LBB4_20
    mov     w8, #64
    sub     x26, x8, x25
    lsl     x8, x23, x25
    lsr     x9, x22, #1
    mvn     w28, w25
    lsr     x9, x9, x28
    orr     x23, x8, x9
    lsl     x22, x22, x25
    mov     x0, x23
    mov     x1, x22
    bl      ASM_GSYM(lmmp_inv_2_1_)
    add     x8, x20, x24, lsl #3
    ldur    x9, [x8, #-8]
    neg     x10, x25
    ldr     x11, [x20, x27, lsl #3]
    lsr     x10, x9, x10
    lsr     x12, x11, #1
    lsr     x12, x12, x28
    lsl     x9, x9, x25
    orr     x9, x9, x12
    ldur    x8, [x8, #-24]
    lsr     x8, x8, #1
    lsr     x8, x8, x28
    lsl     x11, x11, x25
    orr     x8, x11, x8
    mul     x11, x10, x0
    umulh   x12, x10, x0
    adds    x11, x9, x11
    adc     x10, x10, x12
    mvn     x12, x10
    subs    x8, x8, x22
    mul     x13, x10, x22
    umulh   x14, x10, x22
    sbc     x9, x9, xzr
    subs    x8, x8, x13
    madd    x9, x23, x12, x9
    sbc     x9, x9, x14
    cmp     x9, x11
    csel    x11, x22, xzr, hs
    cset    w12, hs
    csel    x13, x23, xzr, hs
    sub     x10, x10, x12
    add     x12, x10, #1
    adds    x8, x11, x8
    adc     x9, x13, x9
    add     x11, x12, #1
    subs    x12, x8, x22
    csetm   x13, lo
    sub     x13, x13, x23
    add     x13, x13, x9
    cmp     x8, x22
    ccmp    x9, x23, #2, lo
    csinc   x11, x11, x10, hi
    csel    x12, x8, x12, ls
    csel    x13, x9, x13, ls
    cmp     x9, x23
    csel    x8, x8, x12, lo
    csinc   x10, x11, x10, hs
    csel    x13, x9, x13, lo
    str     x10, [x21, x27, lsl #3]
    sub     x12, x24, #3
    cmp     x27, #2
    b.lo    LBB4_40
    tbnz    w12, #0, LBB4_36
    sub     x10, x24, #3
    mov     x12, x27
    cmp     x24, #4
    b.ne    LBB4_37
    b       LBB4_39

// na==2 normalised: 128-bit compare/sub
LBB4_10:
    ldp     x8, x9, [x20]
    cmp     x9, x23
    b.lo    LBB4_18
    ccmp    x8, x22, #2, eq
    b.lo    LBB4_18
    subs    x8, x8, x22
    str     x8, [x19]
    ldr     x8, [x20, #8]
    sbc     x8, x8, x23
    str     x8, [x19, #8]
    cbz     x21, LBB4_48
    mov     w8, #1
    str     x8, [x21]
    b       LBB4_48

// dstq==NULL, unnormalised path
LBB4_14:
    cbz     x25, LBB4_28
    mov     w8, #64
    sub     x26, x8, x25
    lsl     x8, x23, x25
    lsr     x9, x22, #1
    mvn     w23, w25
    lsr     x9, x9, x23
    orr     x21, x8, x9
    lsl     x22, x22, x25
    mov     x0, x21
    mov     x1, x22
    bl      ASM_GSYM(lmmp_inv_2_1_)
    add     x8, x20, x24, lsl #3
    ldur    x9, [x8, #-8]
    neg     x10, x25
    lsr     x10, x9, x10
    ldr     x11, [x20, x27, lsl #3]
    lsr     x12, x11, #1
    lsr     x12, x12, x23
    lsl     x9, x9, x25
    orr     x9, x9, x12
    ldur    x8, [x8, #-24]
    lsr     x8, x8, #1
    lsr     x8, x8, x23
    lsl     x11, x11, x25
    orr     x8, x11, x8
    mul     x11, x10, x0
    umulh   x12, x10, x0
    adds    x11, x9, x11
    adc     x10, x10, x12
    mvn     x12, x10
    subs    x8, x8, x22
    mul     x13, x10, x22
    umulh   x10, x10, x22
    sbc     x9, x9, xzr
    subs    x8, x8, x13
    madd    x9, x21, x12, x9
    sbc     x9, x9, x10
    cmp     x9, x11
    csel    x10, xzr, x22, lo
    csel    x11, xzr, x21, lo
    adds    x8, x10, x8
    adc     x9, x11, x9
    subs    x10, x8, x22
    csetm   x11, lo
    sub     x11, x11, x21
    add     x11, x11, x9
    cmp     x8, x22
    ccmp    x9, x21, #2, lo
    csel    x10, x8, x10, ls
    csel    x11, x9, x11, ls
    cmp     x9, x21
    csel    x8, x8, x10, lo
    csel    x9, x9, x11, lo
    sub     x10, x24, #3
    cmp     x27, #2
    b.lo    LBB4_46
    tbnz    w10, #0, LBB4_41
    sub     x12, x24, #3
    mov     x10, x27
    b       LBB4_42

// a < b: q = 0, r = a
LBB4_18:
    str     x8, [x19]
    ldr     x8, [x20, #8]
    str     x8, [x19, #8]
    cbz     x21, LBB4_48
    str     xzr, [x21]
    b       LBB4_48

// normalised + quotient path
LBB4_20:
    mov     x0, x23
    mov     x1, x22
    bl      ASM_GSYM(lmmp_inv_2_1_)
    add     x8, x20, x24, lsl #3
    ldr     x9, [x20, x27, lsl #3]
    ldur    x8, [x8, #-8]
    subs    x10, x9, x22
    sbc     x11, x8, x23
    cmp     x11, x8
    csel    x9, x10, x9, lo
    cset    w12, lo
    csel    x13, xzr, x23, lo
    add     x13, x13, x11
    cmp     x9, x10
    cinc    x10, x13, lo
    cmp     x11, x8
    mov     w8, #1
    cinc    x8, x8, lo
    subs    x11, x9, x22
    csetm   x13, lo
    sub     x13, x13, x23
    add     x13, x13, x10
    cmp     x10, x23
    ccmp    x9, x22, #2, ls
    csel    x8, x12, x8, lo
    csel    x11, x9, x11, lo
    csel    x13, x10, x13, lo
    cmp     x10, x23
    csel    x8, x12, x8, lo
    csel    x9, x9, x11, lo
    csel    x12, x10, x13, lo
    str     x8, [x21, x27, lsl #3]
    sub     x10, x24, #3
    cmp     x27, #2
    b.lo    LBB4_27
    tbz     w10, #0, LBB4_23
    ldr     x8, [x20, x10, lsl #3]
    mul     x11, x12, x0
    umulh   x13, x12, x0
    adds    x11, x9, x11
    adc     x12, x12, x13
    mvn     x13, x12
    madd    x9, x23, x13, x9
    subs    x8, x8, x22
    cset    w13, lo
    mul     x14, x12, x22
    umulh   x15, x12, x22
    sub     x9, x9, x15
    subs    x8, x8, x14
    sbc     x9, x9, x13
    cmp     x9, x11
    csel    x11, x22, xzr, hs
    cset    w13, hs
    csel    x14, x23, xzr, hs
    sub     x12, x12, x13
    add     x13, x12, #1
    adds    x8, x11, x8
    adc     x11, x14, x9
    add     x9, x13, #1
    subs    x13, x8, x22
    csetm   x14, lo
    sub     x14, x14, x23
    add     x14, x14, x11
    cmp     x11, x23
    ccmp    x8, x22, #2, ls
    csinc   x15, x9, x12, hs
    csel    x9, x8, x13, lo
    csel    x13, x11, x14, lo
    cmp     x11, x23
    csel    x9, x8, x9, lo
    csinc   x8, x15, x12, hs
    csel    x12, x11, x13, lo
    str     x8, [x21, x10, lsl #3]
    sub     x10, x24, #4

// normalised unrolled loop entry
LBB4_23:
    cmp     x24, #4
    b.eq    LBB4_26
    add     x8, x10, #1
    lsl     x11, x10, #3
    add     x10, x21, x11
    add     x13, x20, x11
    mov     x11, x13

// normalised main loop 2x unrolled:
// q = hi(m*r_hi)+r_hi+a_lo; z = a - q*d; two fixups
LBB4_25:
    ldr     x14, [x11], #-16
    mul     x15, x12, x0
    umulh   x16, x12, x0
    adds    x15, x9, x15
    adc     x12, x12, x16            // q = r_hi + hi(m*r_hi) + carry
    mvn     x16, x12
    madd    x9, x23, x16, x9         // r_lo -= (q+1)*bl via ~q
    subs    x14, x14, x22
    cset    w16, lo
    mul     x17, x12, x22
    umulh   x1, x12, x22
    sub     x9, x9, x1
    subs    x14, x14, x17
    sbc     x9, x9, x16
    cmp     x9, x15
    csel    x15, x22, xzr, hs
    cset    w16, hs
    csel    x17, x23, xzr, hs
    sub     x12, x12, x16
    add     x16, x12, #1
    adds    x14, x15, x14
    adc     x9, x17, x9
    add     x15, x16, #1
    subs    x16, x14, x22
    csetm   x17, lo
    sub     x17, x17, x23
    add     x17, x17, x9
    cmp     x9, x23
    ccmp    x14, x22, #2, ls
    csinc   x15, x15, x12, hs
    csel    x16, x14, x16, lo
    csel    x17, x9, x17, lo
    cmp     x9, x23
    csel    x14, x14, x16, lo
    csinc   x12, x15, x12, hs
    csel    x9, x9, x17, lo
    str     x12, [x10]
    ldur    x12, [x13, #-8]
    mul     x13, x9, x0
    umulh   x15, x9, x0
    adds    x13, x14, x13
    adc     x9, x9, x15
    mvn     x15, x9
    madd    x14, x23, x15, x14
    subs    x12, x12, x22
    cset    w15, lo
    mul     x16, x9, x22
    umulh   x17, x9, x22
    sub     x14, x14, x17
    subs    x12, x12, x16
    sbc     x14, x14, x15
    cmp     x14, x13
    csel    x13, x22, xzr, hs
    cset    w15, hs
    csel    x16, x23, xzr, hs
    sub     x15, x9, x15
    add     x9, x15, #1
    adds    x12, x13, x12
    adc     x13, x16, x14
    add     x9, x9, #1
    subs    x14, x12, x22
    csetm   x16, lo
    sub     x16, x16, x23
    add     x16, x16, x13
    cmp     x13, x23
    ccmp    x12, x22, #2, ls
    csinc   x17, x9, x15, hs
    csel    x9, x12, x14, lo
    csel    x14, x13, x16, lo
    cmp     x13, x23
    csel    x9, x12, x9, lo
    csinc   x15, x17, x15, hs
    csel    x12, x13, x14, lo
    stur    x15, [x10, #-8]
    sub     x8, x8, #2
    sub     x10, x10, #16
    mov     x13, x11
    cmp     x8, #1
    b.hi    LBB4_25

LBB4_26:
    mov     x10, #0

// normalised: last limb, remainder to [numb,2]
LBB4_27:
    ldr     x8, [x20, x10, lsl #3]
    mul     x10, x0, x12
    umulh   x11, x0, x12
    adds    x10, x9, x10
    adc     x11, x12, x11
    mvn     x12, x11
    madd    x9, x23, x12, x9
    subs    x8, x8, x22
    cset    w12, lo
    mul     x13, x11, x22
    umulh   x14, x11, x22
    sub     x9, x9, x14
    subs    x8, x8, x13
    sbc     x9, x9, x12
    cmp     x9, x10
    csel    x10, x22, xzr, hs
    cset    w12, hs
    csel    x13, x23, xzr, hs
    sub     x11, x11, x12
    add     x12, x11, #1
    adds    x8, x10, x8
    adc     x9, x13, x9
    add     x10, x12, #1
    subs    x12, x8, x22
    csetm   x13, lo
    sub     x13, x13, x23
    add     x13, x13, x9
    cmp     x9, x23
    ccmp    x8, x22, #2, ls
    csinc   x10, x10, x11, hs
    csel    x12, x8, x12, lo
    csel    x13, x9, x13, lo
    cmp     x9, x23
    csel    x8, x8, x12, lo
    csinc   x10, x10, x11, hs
    str     x10, [x21]
    csel    x9, x9, x13, lo
    stp     x8, x9, [x19]
    b       LBB4_48

// dstq==NULL, normalised (mod path)
LBB4_28:
    mov     x0, x23
    mov     x1, x22
    bl      ASM_GSYM(lmmp_inv_2_1_)
    add     x8, x20, x24, lsl #3
    ldur    x8, [x8, #-8]
    ldr     x9, [x20, x27, lsl #3]
    subs    x10, x9, x22
    sbc     x11, x8, x23
    cmp     x11, x8
    csel    x8, x10, x9, lo
    csel    x9, xzr, x23, lo
    add     x9, x9, x11
    cmp     x8, x10
    cinc    x9, x9, lo
    subs    x10, x8, x22
    csetm   x11, lo
    sub     x11, x11, x23
    add     x11, x11, x9
    cmp     x9, x23
    ccmp    x8, x22, #2, ls
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    cmp     x9, x23
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    sub     x9, x24, #3
    cmp     x27, #2
    b.lo    LBB4_35
    tbz     w9, #0, LBB4_31
    ldr     x8, [x20, x9, lsl #3]
    mul     x9, x11, x0
    umulh   x12, x11, x0
    adds    x9, x10, x9
    adc     x11, x11, x12
    mvn     x12, x11
    madd    x10, x23, x12, x10
    subs    x8, x8, x22
    cset    w12, lo
    mul     x13, x11, x22
    umulh   x11, x11, x22
    sub     x10, x10, x11
    subs    x8, x8, x13
    sbc     x10, x10, x12
    cmp     x10, x9
    csel    x9, xzr, x22, lo
    csel    x11, xzr, x23, lo
    adds    x8, x9, x8
    adc     x9, x11, x10
    subs    x10, x8, x22
    csetm   x11, lo
    sub     x11, x11, x23
    add     x11, x11, x9
    cmp     x9, x23
    ccmp    x8, x22, #2, ls
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    cmp     x9, x23
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    sub     x9, x24, #4

// normalised mod loop 2x unrolled, no quotient
LBB4_31:
    cmp     x24, #4
    b.eq    LBB4_34
    add     x8, x9, #1
    add     x9, x20, x9, lsl #3

// divide step x2, remainder only
LBB4_33:
    mul     x12, x11, x0
    umulh   x13, x11, x0
    adds    x12, x10, x12
    adc     x11, x11, x13
    mvn     x13, x11
    madd    x10, x23, x13, x10
    ldp     x14, x13, [x9, #-8]
    subs    x13, x13, x22
    umulh   x15, x11, x22
    sub     x10, x10, x15
    cset    w15, lo
    mul     x11, x11, x22
    subs    x11, x13, x11
    sbc     x10, x10, x15
    cmp     x10, x12
    csel    x12, xzr, x22, lo
    csel    x13, xzr, x23, lo
    adds    x11, x12, x11
    adc     x10, x13, x10
    subs    x12, x11, x22
    csetm   x13, lo
    sub     x13, x13, x23
    add     x13, x13, x10
    cmp     x10, x23
    ccmp    x11, x22, #2, ls
    csel    x12, x11, x12, lo
    csel    x13, x10, x13, lo
    cmp     x10, x23
    csel    x11, x11, x12, lo
    csel    x10, x10, x13, lo
    mul     x12, x10, x0
    umulh   x13, x10, x0
    adds    x12, x11, x12
    adc     x10, x10, x13
    mvn     x13, x10
    madd    x11, x23, x13, x11
    subs    x13, x14, x22
    cset    w14, lo
    mul     x15, x10, x22
    umulh   x10, x10, x22
    sub     x10, x11, x10
    subs    x11, x13, x15
    sbc     x10, x10, x14
    cmp     x10, x12
    csel    x12, xzr, x22, lo
    csel    x13, xzr, x23, lo
    adds    x11, x12, x11
    adc     x12, x13, x10
    sub     x9, x9, #16
    subs    x10, x11, x22
    csetm   x13, lo
    sub     x13, x13, x23
    add     x13, x13, x12
    cmp     x12, x23
    ccmp    x11, x22, #2, ls
    csel    x10, x11, x10, lo
    csel    x13, x12, x13, lo
    cmp     x12, x23
    csel    x10, x11, x10, lo
    csel    x11, x12, x13, lo
    sub     x8, x8, #2
    cmp     x8, #1
    b.hi    LBB4_33

LBB4_34:
    mov     x9, #0

// normalised mod: last step
LBB4_35:
    ldr     x8, [x20, x9, lsl #3]
    mul     x9, x0, x11
    umulh   x12, x0, x11
    adds    x9, x10, x9
    adc     x11, x11, x12
    mvn     x12, x11
    madd    x10, x23, x12, x10
    subs    x8, x8, x22
    cset    w12, lo
    mul     x13, x11, x22
    umulh   x11, x11, x22
    sub     x10, x10, x11
    subs    x8, x8, x13
    sbc     x10, x10, x12
    cmp     x10, x9
    csel    x9, xzr, x22, lo
    csel    x11, xzr, x23, lo
    adds    x8, x9, x8
    adc     x9, x11, x10
    subs    x10, x8, x22
    csetm   x11, lo
    sub     x11, x11, x23
    add     x11, x11, x9
    cmp     x9, x23
    ccmp    x8, x22, #2, ls
    csel    x10, x8, x10, lo
    csel    x11, x9, x11, lo
    cmp     x9, x23
    csel    x10, x8, x10, lo
    csel    x8, x9, x11, lo
    str     x10, [x19]
    b       LBB4_47

// first step, odd adjust; a_lo = lo<<k|hi>>(64-k)
LBB4_36:
    ldr     x9, [x20, x12, lsl #3]
    lsl     x9, x9, x25
    add     x10, x20, x27, lsl #3
    ldur    x10, [x10, #-16]
    lsr     x10, x10, x26
    orr     x9, x10, x9
    mul     x10, x13, x0
    umulh   x11, x13, x0
    adds    x10, x8, x10
    adc     x11, x13, x11
    mvn     x13, x11
    madd    x8, x23, x13, x8
    subs    x9, x9, x22
    cset    w13, lo
    mul     x14, x11, x22
    umulh   x15, x11, x22
    sub     x8, x8, x15
    subs    x9, x9, x14
    sbc     x8, x8, x13
    cmp     x8, x10
    csel    x10, x22, xzr, hs
    cset    w13, hs
    csel    x14, x23, xzr, hs
    sub     x11, x11, x13
    add     x13, x11, #1
    adds    x9, x10, x9
    adc     x10, x14, x8
    add     x8, x13, #1
    subs    x13, x9, x22
    csetm   x14, lo
    sub     x14, x14, x23
    add     x14, x14, x10
    cmp     x9, x22
    ccmp    x10, x23, #2, lo
    csinc   x15, x8, x11, hi
    csel    x8, x9, x13, ls
    csel    x13, x10, x14, ls
    cmp     x10, x23
    csel    x8, x9, x8, lo
    csinc   x9, x15, x11, hs
    csel    x13, x10, x13, lo
    str     x9, [x21, x12, lsl #3]
    sub     x10, x24, #4
    cmp     x24, #4
    b.eq    LBB4_39

// unnormalised 2x unrolled loop
LBB4_37:
    lsl     x11, x10, #3
    add     x9, x20, x11
    add     x10, x10, #1
    add     x11, x21, x11

// divide step x2, shifted a_lo
LBB4_38:
    ldr     x14, [x9]
    lsl     x14, x14, x25
    add     x12, x20, x12, lsl #3
    ldur    x12, [x12, #-16]
    lsr     x12, x12, x26
    orr     x12, x12, x14
    mul     x14, x13, x0
    umulh   x15, x13, x0
    adds    x14, x8, x14
    adc     x13, x13, x15
    mvn     x15, x13
    madd    x8, x23, x15, x8
    subs    x12, x12, x22
    cset    w15, lo
    mul     x16, x13, x22
    umulh   x17, x13, x22
    sub     x8, x8, x17
    subs    x12, x12, x16
    sbc     x8, x8, x15
    cmp     x8, x14
    csel    x14, x22, xzr, hs
    cset    w15, hs
    csel    x16, x23, xzr, hs
    sub     x13, x13, x15
    add     x15, x13, #1
    adds    x12, x14, x12
    adc     x8, x16, x8
    add     x14, x15, #1
    subs    x15, x12, x22
    csetm   x16, lo
    sub     x16, x16, x23
    add     x16, x16, x8
    cmp     x12, x22
    ccmp    x8, x23, #2, lo
    csinc   x14, x14, x13, hi
    csel    x15, x12, x15, ls
    csel    x16, x8, x16, ls
    cmp     x8, x23
    csel    x12, x12, x15, lo
    csinc   x13, x14, x13, hs
    csel    x8, x8, x16, lo
    str     x13, [x11]
    sub     x10, x10, #2
    ldp     x13, x14, [x9, #-16]!
    lsl     x14, x14, x25
    lsr     x13, x13, x26
    orr     x13, x13, x14
    mul     x14, x8, x0
    umulh   x15, x8, x0
    adds    x14, x12, x14
    adc     x8, x8, x15
    mvn     x15, x8
    madd    x12, x23, x15, x12
    subs    x13, x13, x22
    cset    w15, lo
    mul     x16, x8, x22
    umulh   x17, x8, x22
    sub     x12, x12, x17
    subs    x13, x13, x16
    sbc     x12, x12, x15
    cmp     x12, x14
    csel    x14, x22, xzr, hs
    cset    w15, hs
    csel    x16, x23, xzr, hs
    sub     x15, x8, x15
    add     x8, x15, #1
    adds    x13, x14, x13
    adc     x12, x16, x12
    add     x8, x8, #1
    subs    x14, x13, x22
    csetm   x16, lo
    sub     x16, x16, x23
    add     x16, x16, x12
    cmp     x13, x22
    ccmp    x12, x23, #2, lo
    csinc   x17, x8, x15, hi
    csel    x8, x13, x14, ls
    csel    x14, x12, x16, ls
    cmp     x12, x23
    csel    x8, x13, x8, lo
    csinc   x15, x17, x15, hs
    csel    x13, x12, x14, lo
    stur    x15, [x11, #-8]
    sub     x11, x11, #16
    mov     x12, x10
    cmp     x10, #1
    b.hi    LBB4_38

LBB4_39:
    mov     x12, #0

// unnormalised: last limb + r >> k
LBB4_40:
    ldr     x9, [x20, x12, lsl #3]
    lsl     x9, x9, x25
    mul     x10, x13, x0
    umulh   x11, x13, x0
    adds    x10, x8, x10
    adc     x11, x13, x11
    mvn     x12, x11
    madd    x8, x23, x12, x8
    subs    x9, x9, x22
    cset    w12, lo
    mul     x13, x11, x22
    umulh   x14, x11, x22
    sub     x8, x8, x14
    subs    x9, x9, x13
    sbc     x8, x8, x12
    cmp     x8, x10
    csel    x10, x22, xzr, hs
    cset    w12, hs
    csel    x13, x23, xzr, hs
    sub     x11, x11, x12
    add     x12, x11, #1
    adds    x9, x10, x9
    adc     x8, x13, x8
    add     x10, x12, #1
    subs    x12, x9, x22
    csetm   x13, lo
    sub     x13, x13, x23
    add     x13, x13, x8
    cmp     x9, x22
    ccmp    x8, x23, #2, lo
    csinc   x10, x10, x11, hi
    csel    x12, x9, x12, ls
    csel    x13, x8, x13, ls
    cmp     x8, x23
    csel    x9, x9, x12, lo
    csinc   x10, x10, x11, hs
    csel    x8, x8, x13, lo
    str     x10, [x21]
    lsr     x9, x9, x25
    lsl     x10, x8, x26
    orr     x9, x10, x9
    lsr     x8, x8, x25
    str     x9, [x19]
    b       LBB4_47

// unnormalised mod: first step, odd adjust
LBB4_41:
    ldr     x11, [x20, x10, lsl #3]
    lsl     x11, x11, x25
    add     x12, x20, x27, lsl #3
    ldur    x12, [x12, #-16]
    lsr     x12, x12, x26
    orr     x11, x12, x11
    mul     x12, x9, x0
    umulh   x13, x9, x0
    adds    x12, x8, x12
    adc     x9, x9, x13
    mvn     x13, x9
    madd    x8, x21, x13, x8
    subs    x11, x11, x22
    cset    w13, lo
    mul     x14, x9, x22
    umulh   x9, x9, x22
    sub     x8, x8, x9
    subs    x9, x11, x14
    sbc     x8, x8, x13
    cmp     x8, x12
    csel    x11, xzr, x22, lo
    csel    x12, xzr, x21, lo
    adds    x9, x11, x9
    adc     x11, x12, x8
    subs    x8, x9, x22
    csetm   x12, lo
    sub     x12, x12, x21
    add     x12, x12, x11
    cmp     x9, x22
    ccmp    x11, x21, #2, lo
    csel    x8, x9, x8, ls
    csel    x12, x11, x12, ls
    cmp     x11, x21
    csel    x8, x9, x8, lo
    csel    x9, x11, x12, lo
    sub     x12, x24, #4

// unnormalised mod 2x unrolled loop
LBB4_42:
    cmp     x24, #4
    b.eq    LBB4_45
    add     x11, x20, x12, lsl #3
    add     x12, x12, #1

// divide step x2, shifted a_lo
LBB4_44:
    ldr     x13, [x11]
    lsl     x13, x13, x25
    add     x10, x20, x10, lsl #3
    ldur    x10, [x10, #-16]
    lsr     x10, x10, x26
    orr     x10, x10, x13
    mul     x13, x9, x0
    umulh   x14, x9, x0
    adds    x13, x8, x13
    adc     x9, x9, x14
    mvn     x14, x9
    madd    x8, x21, x14, x8
    subs    x10, x10, x22
    cset    w14, lo
    mul     x15, x9, x22
    umulh   x9, x9, x22
    sub     x8, x8, x9
    subs    x9, x10, x15
    sbc     x8, x8, x14
    cmp     x8, x13
    csel    x10, xzr, x22, lo
    csel    x13, xzr, x21, lo
    adds    x9, x10, x9
    adc     x8, x13, x8
    subs    x10, x9, x22
    csetm   x13, lo
    sub     x13, x13, x21
    add     x13, x13, x8
    cmp     x9, x22
    ccmp    x8, x21, #2, lo
    csel    x10, x9, x10, ls
    csel    x13, x8, x13, ls
    cmp     x8, x21
    csel    x9, x9, x10, lo
    csel    x8, x8, x13, lo
    ldp     x10, x13, [x11, #-16]!
    sub     x12, x12, #2
    lsl     x13, x13, x25
    lsr     x10, x10, x26
    orr     x10, x10, x13
    mul     x13, x8, x0
    umulh   x14, x8, x0
    adds    x13, x9, x13
    adc     x8, x8, x14
    mvn     x14, x8
    madd    x9, x21, x14, x9
    subs    x10, x10, x22
    cset    w14, lo
    mul     x15, x8, x22
    umulh   x8, x8, x22
    sub     x8, x9, x8
    subs    x9, x10, x15
    sbc     x8, x8, x14
    cmp     x8, x13
    csel    x10, xzr, x22, lo
    csel    x13, xzr, x21, lo
    adds    x9, x10, x9
    adc     x10, x13, x8
    subs    x8, x9, x22
    csetm   x13, lo
    sub     x13, x13, x21
    add     x13, x13, x10
    cmp     x9, x22
    ccmp    x10, x21, #2, lo
    csel    x8, x9, x8, ls
    csel    x13, x10, x13, ls
    cmp     x10, x21
    csel    x8, x9, x8, lo
    csel    x9, x10, x13, lo
    mov     x10, x12
    cmp     x12, #1
    b.hi    LBB4_44

LBB4_45:
    mov     x10, #0

// unnormalised mod: last step + r >> k
LBB4_46:
    ldr     x10, [x20, x10, lsl #3]
    lsl     x10, x10, x25
    mul     x11, x9, x0
    umulh   x12, x9, x0
    adds    x11, x8, x11
    adc     x9, x9, x12
    mvn     x12, x9
    madd    x8, x21, x12, x8
    subs    x10, x10, x22
    cset    w12, lo
    mul     x13, x9, x22
    umulh   x9, x9, x22
    sub     x8, x8, x9
    subs    x9, x10, x13
    sbc     x8, x8, x12
    cmp     x8, x11
    csel    x10, xzr, x22, lo
    csel    x11, xzr, x21, lo
    adds    x9, x10, x9
    adc     x8, x11, x8
    subs    x10, x9, x22
    csetm   x11, lo
    sub     x11, x11, x21
    add     x11, x11, x8
    cmp     x9, x22
    ccmp    x8, x21, #2, lo
    csel    x10, x9, x10, ls
    csel    x11, x8, x11, ls
    cmp     x8, x21
    csel    x9, x9, x10, lo
    csel    x8, x8, x11, lo
    lsr     x9, x9, x25
    lsl     x10, x8, x26
    orr     x9, x10, x9
    str     x9, [x19]
    lsr     x8, x8, x25

// remainder hi limb -> [numb+8]
LBB4_47:
    str     x8, [x19, #8]

// restore and return
LBB4_48:
    ldp     x29, x30, [sp, #80]
    ldp     x20, x19, [sp, #64]
    ldp     x22, x21, [sp, #48]
    ldp     x24, x23, [sp, #32]
    ldp     x26, x25, [sp, #16]
    ldp     x28, x27, [sp], #96
    ret

.globl ASM_GSYM(lmmp_div_2_s_)
.p2align 2

// x19=numa, x20=dstq, x21=bh, x22=bl
// k=0: pre-divide top limbs, 2x unrolled
ASM_GSYM(lmmp_div_2_s_):
    stp     x24, x23, [sp, #-64]!
    stp     x22, x21, [sp, #16]
    stp     x20, x19, [sp, #32]
    stp     x29, x30, [sp, #48]
    add     x29, sp, #48
    mov     x23, x2
    mov     x19, x1
    mov     x20, x0
    ldp     x22, x21, [x3]
    mov     x0, x21
    mov     x1, x22
    bl      ASM_GSYM(lmmp_inv_2_1_)
    mov     x8, x0
    add     x9, x19, x23, lsl #3
    ldur    x9, [x9, #-8]
    sub     x12, x23, #2
    ldr     x10, [x19, x12, lsl #3]
    subs    x11, x10, x22
    sbc     x13, x9, x21
    cmp     x13, x9
    csel    x10, x11, x10, lo
    cset    w14, lo
    csel    x15, xzr, x21, lo
    add     x15, x15, x13
    cmp     x10, x11
    cinc    x11, x15, lo
    cmp     x13, x9
    mov     w9, #1
    cinc    x9, x9, lo
    subs    x13, x10, x22
    csetm   x15, lo
    sub     x15, x15, x21
    add     x15, x15, x11
    cmp     x10, x22
    ccmp    x11, x21, #2, lo
    csel    x9, x14, x9, ls
    csel    x13, x10, x13, ls
    csel    x15, x11, x15, ls
    cmp     x11, x21
    csel    x0, x14, x9, lo
    csel    x10, x10, x13, lo
    csel    x13, x11, x15, lo
    sub     x11, x23, #3
    cmp     x12, #2
    b.lo    LBB6_7
    tbz     w11, #0, LBB6_3
    ldr     x9, [x19, x11, lsl #3]
    mul     x12, x13, x8
    umulh   x14, x13, x8
    adds    x12, x10, x12
    adc     x13, x13, x14
    mvn     x14, x13
    madd    x10, x21, x14, x10
    subs    x9, x9, x22
    cset    w14, lo
    mul     x15, x13, x22
    umulh   x16, x13, x22
    sub     x10, x10, x16
    subs    x9, x9, x15
    sbc     x10, x10, x14
    cmp     x10, x12
    csel    x12, x22, xzr, hs
    cset    w14, hs
    csel    x15, x21, xzr, hs
    sub     x13, x13, x14
    add     x14, x13, #1
    adds    x9, x12, x9
    adc     x12, x15, x10
    add     x10, x14, #1
    subs    x14, x9, x22
    csetm   x15, lo
    sub     x15, x15, x21
    add     x15, x15, x12
    cmp     x9, x22
    ccmp    x12, x21, #2, lo
    csinc   x16, x10, x13, hi
    csel    x10, x9, x14, ls
    csel    x14, x12, x15, ls
    cmp     x12, x21
    csel    x10, x9, x10, lo
    csinc   x9, x16, x13, hs
    csel    x13, x12, x14, lo
    str     x9, [x20, x11, lsl #3]
    sub     x11, x23, #4

// unrolled loop entry
LBB6_3:
    cmp     x23, #4
    b.eq    LBB6_6
    add     x9, x11, #1
    lsl     x12, x11, #3
    add     x11, x20, x12
    add     x14, x19, x12
    mov     x12, x14

// divide step x2, as div_2_ LBB4_25
LBB6_5:
    ldr     x15, [x12], #-16
    mul     x16, x13, x8
    umulh   x17, x13, x8
    adds    x16, x10, x16
    adc     x13, x13, x17
    mvn     x17, x13
    madd    x10, x21, x17, x10
    subs    x15, x15, x22
    cset    w17, lo
    mul     x1, x13, x22
    umulh   x2, x13, x22
    sub     x10, x10, x2
    subs    x15, x15, x1
    sbc     x10, x10, x17
    cmp     x10, x16
    csel    x16, x22, xzr, hs
    cset    w17, hs
    csel    x1, x21, xzr, hs
    sub     x13, x13, x17
    add     x17, x13, #1
    adds    x15, x16, x15
    adc     x10, x1, x10
    add     x16, x17, #1
    subs    x17, x15, x22
    csetm   x1, lo
    sub     x1, x1, x21
    add     x1, x1, x10
    cmp     x15, x22
    ccmp    x10, x21, #2, lo
    csinc   x16, x16, x13, hi
    csel    x17, x15, x17, ls
    csel    x1, x10, x1, ls
    cmp     x10, x21
    csel    x15, x15, x17, lo
    csinc   x16, x16, x13, hs
    csel    x10, x10, x1, lo
    ldur    x13, [x14, #-8]
    mul     x14, x10, x8
    umulh   x17, x10, x8
    adds    x14, x15, x14
    adc     x10, x10, x17
    mvn     x17, x10
    madd    x15, x21, x17, x15
    subs    x13, x13, x22
    cset    w17, lo
    mul     x1, x10, x22
    umulh   x2, x10, x22
    sub     x15, x15, x2
    subs    x13, x13, x1
    sbc     x15, x15, x17
    cmp     x15, x14
    csel    x14, x22, xzr, hs
    cset    w17, hs
    csel    x1, x21, xzr, hs
    sub     x17, x10, x17
    add     x10, x17, #1
    adds    x13, x14, x13
    adc     x14, x1, x15
    add     x10, x10, #1
    subs    x15, x13, x22
    csetm   x1, lo
    sub     x1, x1, x21
    add     x1, x1, x14
    cmp     x13, x22
    ccmp    x14, x21, #2, lo
    csinc   x2, x10, x17, hi
    csel    x10, x13, x15, ls
    csel    x15, x14, x1, ls
    cmp     x14, x21
    csel    x10, x13, x10, lo
    csinc   x17, x2, x17, hs
    csel    x13, x14, x15, lo
    stp     x17, x16, [x11, #-8]
    sub     x9, x9, #2
    sub     x11, x11, #16
    mov     x14, x12
    cmp     x9, #1
    b.hi    LBB6_5

LBB6_6:
    mov     x11, #0

// last limb: store quotient and remainder, ret qh
LBB6_7:
    ldr     x9, [x19, x11, lsl #3]
    mul     x11, x8, x13
    umulh   x8, x8, x13
    adds    x11, x10, x11
    adc     x8, x13, x8
    mvn     x12, x8
    madd    x10, x21, x12, x10
    subs    x9, x9, x22
    cset    w12, lo
    mul     x13, x8, x22
    umulh   x14, x8, x22
    sub     x10, x10, x14
    subs    x9, x9, x13
    sbc     x10, x10, x12
    cmp     x10, x11
    csel    x11, x22, xzr, hs
    cset    w12, hs
    csel    x13, x21, xzr, hs
    sub     x8, x8, x12
    add     x12, x8, #1
    adds    x9, x11, x9
    adc     x10, x13, x10
    add     x11, x12, #1
    subs    x12, x9, x22
    csetm   x13, lo
    sub     x13, x13, x21
    add     x13, x13, x10
    cmp     x9, x22
    ccmp    x10, x21, #2, lo
    csinc   x11, x11, x8, hi
    csel    x12, x9, x12, ls
    csel    x13, x10, x13, ls
    cmp     x10, x21
    csel    x9, x9, x12, lo
    csinc   x8, x11, x8, hs
    str     x8, [x20]
    csel    x8, x10, x13, lo
    stp     x9, x8, [x19]
    ldp     x29, x30, [sp, #48]
    ldp     x20, x19, [sp, #32]
    ldp     x22, x21, [sp, #16]
    ldp     x24, x23, [sp], #64
    ret
