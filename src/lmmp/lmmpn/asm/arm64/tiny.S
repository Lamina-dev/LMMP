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

// int lmmp_limb_bits_(mp_limb_t x);
// int lmmp_limb_popcnt_(mp_limb_t x);
// int lmmp_leading_zeros_(mp_limb_t x);
// int lmmp_tailing_zeros_(mp_limb_t x);
// mp_limb_t lmmp_mulh_(mp_limb_t a, mp_limb_t b);
// void lmmp_mullh_(mp_limb_t a, mp_limb_t b, mp_ptr dst);
// ulong lmmp_mulmod_ulong_(ulong a, ulong b, ulong mod, ulongp q);
//
//   x0=a, x1=b, x2=mod/dst, x3=q

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_limb_bits_)
.p2align 2

// ret bit length of x, 0 if x == 0
ASM_GSYM(lmmp_limb_bits_):
    clz     x8, x0                  // clz(x), 64 if 0
    mov     w9, #64
    sub     w8, w9, w8              // 64 - clz
    cmp     x0, #0
    csel    w0, wzr, w8, eq         // x == 0 -> 0
    ret

.globl ASM_GSYM(lmmp_limb_popcnt_)
.p2align 2

// ret popcount(x), via NEON
ASM_GSYM(lmmp_limb_popcnt_):
    fmov    d0, x0                  // x to vector
    cnt     v0.8b, v0.8b            // per-byte popcount
    addv    b0, v0.8b               // sum 8 bytes
    fmov    w0, s0
    ret

.globl ASM_GSYM(lmmp_leading_zeros_)
.p2align 2

// ret clz(x), 64 if x == 0
ASM_GSYM(lmmp_leading_zeros_):
    clz     x0, x0
    ret

.globl ASM_GSYM(lmmp_tailing_zeros_)
.p2align 2

// ret ctz(x), 64 if x == 0
ASM_GSYM(lmmp_tailing_zeros_):
    rbit    x8, x0                  // clz of reversed = ctz
    clz     x0, x8
    ret

.globl ASM_GSYM(lmmp_mulh_)
.p2align 2

// ret hi 64 of a*b
ASM_GSYM(lmmp_mulh_):
    umulh   x0, x1, x0
    ret

.globl ASM_GSYM(lmmp_mullh_)
.p2align 2

// dst[0] = lo(a*b), dst[1] = hi(a*b)
ASM_GSYM(lmmp_mullh_):
    mul     x8, x1, x0              // lo 64
    umulh   x9, x1, x0              // hi 64
    stp     x8, x9, [x2]
    ret

.globl ASM_GSYM(lmmp_mulmod_ulong_)
.p2align 2

// r = (a*b) mod mod, *q = quotient
// cal: s = clz(mod), M = (a*b) << s
//      q0 = (M*v) >> 128, msub fixups
//      r = r' >> s
ASM_GSYM(lmmp_mulmod_ulong_):
    stp     x24, x23, [sp, #-64]!   // save regs
    stp     x22, x21, [sp, #16]
    stp     x20, x19, [sp, #32]
    stp     x29, x30, [sp, #48]
    add     x29, sp, #48
    mov     x19, x3                 // x19 = q
    mov     x20, x1                 // x20 = b
    mov     x21, x0                 // x21 = a
    clz     x23, x2                 // x23 = s = clz(mod)
    lsl     x22, x2, x23            // x22 = mod' = mod << s
    mov     x0, x22
    bl      ASM_GSYM(lmmp_inv_1_)   // x0 = v
    mul     x8, x20, x21            // N = a*b: lo 64
    umulh   x9, x20, x21            // N: hi 64
    lsl     x10, x8, x23            // M = N << s: lo 64
    mvn     w11, w23                // w11 = 63 - s
    lsr     x8, x8, #1
    lsr     x8, x8, x11             // N_lo >> (64 - s)
    lsl     x9, x9, x23
    orr     x8, x9, x8              // M: hi 64
    mul     x9, x0, x8              // q0 ~= M_hi * v
    umulh   x11, x0, x8
    adds    x9, x10, x9             // rounding for >> 128
    add     x8, x8, #1
    adc     x8, x8, x11             // x8 = q0
    msub    x10, x8, x22, x10       // r' = M_lo - q0*mod'
    cmp     x10, x9
    csel    x9, x22, xzr, hi        // fix: r' += mod'
    cset    w11, hi
    sub     x8, x8, x11             // q--
    add     x9, x9, x10
    cmp     x9, x22                 // second fix: r' >= mod' ?
    cinc    x8, x8, hs              // q++
    csel    x10, x22, xzr, hs
    sub     x9, x9, x10             // r' -= mod'
    str     x8, [x19]               // *q = q
    lsr     x0, x9, x23             // r = r' >> s
    ldp     x29, x30, [sp, #48]     // restore regs
    ldp     x20, x19, [sp, #32]
    ldp     x22, x21, [sp, #16]
    ldp     x24, x23, [sp], #64
    ret
