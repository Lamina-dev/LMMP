#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#  by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY.
#
#  See <https://www.gnu.org/licenses/>.

# int lmmp_limb_bits_(mp_limb_t x);
# int lmmp_limb_popcnt_(mp_limb_t x);
# int lmmp_leading_zeros_(mp_limb_t x);
# int lmmp_tailing_zeros_(mp_limb_t x);
# mp_limb_t lmmp_mulh_(mp_limb_t a, mp_limb_t b);
# void lmmp_mullh_(mp_limb_t a, mp_limb_t b, mp_ptr dst);
# ulong lmmp_mulmod_ulong_(ulong a, ulong b, ulong mod, ulongp q);

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_limb_bits_)
.p2align 2

ASM_GSYM(lmmp_limb_bits_):
    clz     x8, x0
    mov     w9, #64
    sub     w8, w9, w8
    cmp     x0, #0
    csel    w0, wzr, w8, eq
    ret

.globl ASM_GSYM(lmmp_limb_popcnt_)
.p2align 2

ASM_GSYM(lmmp_limb_popcnt_):
    fmov    d0, x0
    cnt.8b  v0, v0
    addv.8b b0, v0
    fmov    w0, s0
    ret

.globl ASM_GSYM(lmmp_leading_zeros_)
.p2align 2

ASM_GSYM(lmmp_leading_zeros_):
    clz     x0, x0
    ret

.globl ASM_GSYM(lmmp_tailing_zeros_)
.p2align 2

ASM_GSYM(lmmp_tailing_zeros_):
    rbit    x8, x0
    clz     x0, x8
    ret

.globl ASM_GSYM(lmmp_mulh_)
.p2align 2

ASM_GSYM(lmmp_mulh_):
    umulh   x0, x1, x0
    ret

.globl ASM_GSYM(lmmp_mullh_)
.p2align 2

ASM_GSYM(lmmp_mullh_):
    mul     x8, x1, x0
    umulh   x9, x1, x0
    stp     x8, x9, [x2]
    ret

.globl ASM_GSYM(lmmp_mulmod_ulong_)
.p2align 2

ASM_GSYM(lmmp_mulmod_ulong_):
    stp     x24, x23, [sp, #-64]!
    stp     x22, x21, [sp, #16]
    stp     x20, x19, [sp, #32]
    stp     x29, x30, [sp, #48]
    add     x29, sp, #48
    mov     x19, x3
    mov     x20, x1
    mov     x21, x0
    clz     x23, x2
    lsl     x22, x2, x23
    mov     x0, x22
    bl      ASM_GSYM(lmmp_inv_1_)
    mul     x8, x20, x21
    umulh   x9, x20, x21
    lsl     x10, x8, x23
    mvn     w11, w23
    lsr     x8, x8, #1
    lsr     x8, x8, x11
    lsl     x9, x9, x23
    orr     x8, x9, x8
    mul     x9, x0, x8
    umulh   x11, x0, x8
    adds    x9, x10, x9
    add     x8, x8, #1
    adc     x8, x8, x11
    msub    x10, x8, x22, x10
    cmp     x10, x9
    csel    x9, x22, xzr, hi
    cset    w11, hi
    sub     x8, x8, x11
    add     x9, x9, x10
    cmp     x9, x22
    cinc    x8, x8, hs
    csel    x10, x22, xzr, hs
    sub     x9, x9, x10
    str     x8, [x19]
    lsr     x0, x9, x23
    ldp     x29, x30, [sp, #48]
    ldp     x20, x19, [sp, #32]
    ldp     x22, x21, [sp, #16]
    ldp     x24, x23, [sp], #64
    ret
