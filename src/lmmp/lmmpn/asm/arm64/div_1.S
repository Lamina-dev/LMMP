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

// mp_limb_t lmmp_div_1_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_limb_t x);
//   ret x0 = [numa,na] mod x; [dstq,na] = [numa,na] div x if dstq != NULL
// mp_limb_t lmmp_div_1_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_limb_t x);
//   qh:[dstq,na-1] = [numa,na] div x, [numa,1] = remainder
//
//   x0=dstq, x1=numa, x2=na, x3=x
//   ret x0 = remainder (div_1_), qh (div_1_s_)
//   cal: q = (xh+1) + hi(xh*m); z = a_lo - d*q; fixups

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_div_1_)
.p2align 2

ASM_GSYM(lmmp_div_1_):
// x1=extra(0), x2=numa, x3=na, x4=x
    cbz     x0, Ldiv1_to_mod        // dstq==NULL -> mod_1_
    mov     x4, x3
    mov     x3, x2
    mov     x2, x1
    mov     x1, #0
    cbz     x3, Ldiv_fz             // na==0, zero quotient
    stp     x29, x30, [sp, #-80]!
    mov     x29, sp
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    sub     x21, x3, #1             // x21 = na-1
    add     x7, x21, x1
    add     x20, x2, x21, lsl #3    // x20 = &numa[na-1]
    add     x19, x0, x7, lsl #3     // x19 = &dstq[na-1+extra]
    mov     x24, x1                 // x24 = extra, 0 here
    mov     x22, x4                 // x22 = divisor x
    tbnz    x4, #63, Ldiv_normalised // MSB(x)=1 -> normalised

// unnormalised: d = x << k, m = inv(d) - 2^64
Ldiv_unnorm:
    clz     x23, x22                // x23 = k = clz(x)
    lsl     x0, x22, x23
    bl      ASM_GSYM(lmmp_inv_1_)   // x0 = m = inv(d) - 2^64

Ldiv_uentry:
    lsl     x22, x22, x23           // x22 = d = x << k
    ldr     x7, [x20], #-8          // top limb
    sub     x8, xzr, x23            // x8 = -k, shift 64-k
    lsr     x11, x7, x8             // x11 = xh = a_top >> (64-k)
    lsl     x1, x7, x23             // x1 = a_lo = a_top << k
    cbz     x21, Ldiv_uend          // na==1, finish

// main loop (unnormalised): x11 = xh < d, x1 = a_lo
Ldiv_utop:
    ldr     x7, [x20], #-8          // next limb
    add     x2, x11, #1             // x2 = xh+1
    mul     x10, x11, x0            // x10 = lo(xh*m)
    umulh   x17, x11, x0            // x17 = hi(xh*m)
    lsr     x9, x7, x8              // top k bits
    orr     x1, x1, x9              // a_lo = (prev<<k)|(x7>>(64-k))
    adds    x10, x1, x10
    adc     x2, x2, x17             // x2 = q = xh+1+hi+carry
    msub    x11, x22, x2, x1        // z = a_lo - d*q
    lsl     x1, x7, x23             // a_lo for next round
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc       // borrow: z += d
    sbc     x2, x2, xzr             // borrow: q--
    cmp     x11, x22
    bcs     Ldiv_ufx                // z >= d

Ldiv_uok:
    str     x2, [x19], #-8          // store quotient
    sub     x21, x21, #1
    cbnz    x21, Ldiv_utop

// last limb, q <= 2 fixups
Ldiv_uend:
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    adds    x10, x1, x10
    adc     x2, x2, x17             // x2 = q
    msub    x11, x22, x2, x1        // z = a_lo - d*q
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc       // borrow: z += d
    sbc     x2, x2, xzr
    subs    x14, x11, x22           // z >= d: z -= d
    adc     x2, x2, xzr             // q++
    csel    x11, x14, x11, cs
    str     x2, [x19], #-8          // store lowest quotient
    cbnz    x24, Ldiv_ftop          // extra > 0
    lsr     x0, x11, x23            // r = z >> k
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

Ldiv_ufx:
    add     x2, x2, #1              // q++
    sub     x11, x11, x22           // z -= d
    b       Ldiv_uok

// normalised path, k = 0
Ldiv_normalised:
    mov     x0, x22
    bl      ASM_GSYM(lmmp_inv_1_)   // x0 = m = inv(x) - 2^64

Ldiv_nentry:
    ldr     x7, [x20], #-8          // top limb
    subs    x14, x7, x22
    adc     x2, xzr, xzr            // x2 = (a_top >= d)
    csel    x11, x14, x7, cs        // x11 = xh = a_top mod d
    b       Ldiv_nok

// main loop (normalised)
Ldiv_ntop:
    ldr     x1, [x20], #-8          // a_lo = next limb
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    adds    x10, x1, x10
    adc     x2, x2, x17             // x2 = q
    msub    x11, x22, x2, x1        // z = a_lo - d*q
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc       // borrow fix
    sbc     x2, x2, xzr
    cmp     x11, x22
    bcs     Ldiv_nfx

Ldiv_nok:
    str     x2, [x19], #-8          // store quotient
    sub     x21, x21, #1
    tbz     x21, #63, Ldiv_ntop

Ldiv_nend:
    cbnz    x24, Ldiv_frac
    mov     x0, x11                 // ret remainder
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

Ldiv_nfx:
    add     x2, x2, #1
    sub     x11, x11, x22
    b       Ldiv_nok

Ldiv_frac:
    mov     x23, #0                 // fractional: k = 0

// fractional loop: dividend 0:0, extra quotient limbs
Ldiv_ftop:
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    add     x2, x2, x17
    msub    x11, x22, x2, xzr       // z = -d*q (mod 2^64)
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc
    sbc     x2, x2, xzr
    str     x2, [x19], #-8
    sub     x24, x24, #1
    cbnz    x24, Ldiv_ftop
    lsr     x0, x11, x23            // restore remainder
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

Ldiv_fz:                            // na==0: zero quotient
    cbz     x1, Ldiv_zend

Ldiv_ztop:
    str     xzr, [x0], #8
    sub     x1, x1, #1
    cbnz    x1, Ldiv_ztop

Ldiv_zend:
    mov     x0, #0
    ret

Ldiv1_to_mod:                       // remap: mod_1_(numa, na, x)
    mov     x0, x1
    mov     x1, x2
    mov     x2, x3
    b       ASM_GSYM(lmmp_mod_1_)

// ----------------------------------------------------------------------
// mp_limb_t lmmp_div_1_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_limb_t x);
//   qh:[dstq,na-1] = [numa,na] div x, [numa,1] = remainder
//
//   x0=dstq, x1=numa, x2=na, x3=x
//   ret x0 = qh
//   cal: q = (xh+1) + hi(xh*m); z = a_lo - d*q; fixups
// ----------------------------------------------------------------------
.globl ASM_GSYM(lmmp_div_1_s_)
.p2align 2

ASM_GSYM(lmmp_div_1_s_):
    stp     x29, x30, [sp, #-80]!
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    mov     x22, x3                 // x22 = d = x
    mov     x21, x2                 // x21 = na
    mov     x20, x1                 // x20 = numa
    add     x20, x20, x21, lsl #3
    add     x19, x0, x21, lsl #3
    sub     x19, x19, #8            // x19 = &dstq[na-1]
    mov     x0, x22
    bl      ASM_GSYM(lmmp_inv_1_)   // x0 = m = inv(d) - 2^64
    mov     x24, x0
    ldr     x7, [x20, #-8]!         // top limb
    subs    x14, x7, x22
    cset    x23, cs                 // x23 = qh = (a_top >= d)
    csel    x11, x14, x7, cs        // x11 = xh
    sub     x21, x21, #1
.p2align 4

// main loop: q = (xh+1) + hi(xh*m); z = a_lo - d*q; fixups
Ldivs_top:
    ldr     x1, [x20, #-8]!         // a_lo
    add     x2, x11, #1
    mul     x10, x11, x24
    umulh   x17, x11, x24
    adds    x10, x1, x10
    adc     x2, x2, x17             // x2 = q
    msub    x11, x22, x2, x1        // z = a_lo - d*q
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc       // borrow fix
    sbc     x2, x2, xzr
    cmp     x11, x22
    bcs     Ldivs_fx

Ldivs_ok:
    str     x2, [x19, #-8]!         // store quotient
    sub     x21, x21, #1
    cbnz    x21, Ldivs_top
    str     x11, [x20]              // [numa] = remainder
    mov     x0, x23                 // ret qh
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

Ldivs_fx:
    add     x2, x2, #1
    sub     x11, x11, x22
    b       Ldivs_ok
