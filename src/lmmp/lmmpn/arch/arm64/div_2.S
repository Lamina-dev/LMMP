//  Copyright (C) 2026 HJimmyK (Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it
//  under the terms of the GNU Lesser General Public License as
//  published by the Free Software Foundation; either version 3 of
//  the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, see
//  <https://www.gnu.org/licenses/>.
//
//  SPDX-License-Identifier: LGPL-3.0-or-later

//
// void lmmp_div_2_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_ptr numb);
//
//   dstq=x0, numa=x1, na=x2, numb=x3

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_div_2_
// ======================================
// x19=&dstq[i], x20=&numa[i], x21=cnt, x22=bl, x23=bh
// x24=m, x25=r_hi, x26=r_lo, x27=k, x28=64-k, x8=lo<<k
.globl ASM_GSYM(lmmp_div_2_)
.p2align 2
ASM_GSYM(lmmp_div_2_):
    cbz     x0, Ldiv2_to_mod        // dstq==NULL -> mod_2_
    stp     x29, x30, [sp, #-112]!
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    stp     x25, x26, [sp, #64]
    stp     x27, x28, [sp, #80]
    str     x3, [sp, #96]           // numb
    add     x19, x0, x2, lsl #3
    sub     x19, x19, #16           // &dstq[na-2]
    mov     x20, x1
    add     x20, x20, x2, lsl #3
    sub     x20, x20, #8            // &numa[na-1]
    ldp     x22, x23, [x3]          // b0, b1
    mov     x21, x2                 // na
    clz     x27, x23                // k
    cbz     x27, Ldiv2_n            // MSB(bh)=1 -> normalised

// unnormalised: b<<k, m = inv_2_1_(bh,bl)
Ldiv2_u:
    neg     x28, x27                // 64-k
    lsl     x0, x23, x27
    lsr     x1, x22, x28
    orr     x23, x0, x1             // bh'
    lsl     x22, x22, x27           // bl'
    mov     x0, x23
    mov     x1, x22
    bl      ASM_GSYM(lmmp_inv_2_1_)
    mov     x24, x0                 // m
    ldr     x7, [x20], #-8          // top limb
    lsr     x25, x7, x28            // r_hi = top>>(64-k)
    ldr     x9, [x20], #-8          // second limb
    lsl     x1, x7, x27
    lsr     x10, x9, x28
    orr     x26, x1, x10            // r_lo = top<<k|second>>(64-k)
    lsl     x8, x9, x27             // lo<<k of second
    subs    x21, x21, #2
    b.eq    Ldiv2_u2                // na==2, single step
    b       Ldiv2_utop

Ldiv2_u2:
    b       Ldiv2_ulast             // a0 = lo<<k, x21==0

// normalised: first pair < 2b -> one conditional subtract
Ldiv2_n:
    ldr     x26, [x20], #-8         // top limb
    ldr     x9, [x20], #-8          // second limb
    subs    x2, x9, x22
    sbcs    x1, x26, x23
    cset    x13, cs                 // q = (a_top:a_2nd >= b)
    csel    x25, x1, x26, cs        // r_hi
    csel    x26, x2, x9, cs         // r_lo
    str     x13, [x19], #-8         // dstq[na-2]
    subs    x21, x21, #2
    b.eq    Ldiv2_fin_n             // na==2 done, no inv needed
    mov     x0, x23
    mov     x1, x22
    bl      ASM_GSYM(lmmp_inv_2_1_)
    mov     x24, x0                 // m
    b       Ldiv2_ntop

.p2align 4
// main loop (normalised): x9 = next limb
Ldiv2_ntop:
    ldr     x9, [x20], #-8
Ldiv2_ncore:
    mul     x10, x24, x25           // lo(m*r_hi)
    umulh   x11, x24, x25           // hi(m*r_hi)
    adds    x12, x26, x10
    adc     x13, x25, x11           // q = r_hi+hi+r_lo>>64
    mul     x14, x22, x13           // lo(bl*q)
    umulh   x15, x22, x13           // hi(bl*q)
    mul     x16, x23, x13           // bh*q
    subs    x17, x9, x14            // z_lo = a0-lo
    sbc     x26, x26, x15           // z_hi = r_lo-hi-borrow
    sub     x26, x26, x16           //      - bh*q
    subs    x17, x17, x22           // z_lo -= bl
    sbc     x25, x26, x23           // z_hi -= bh-borrow
    cmp     x25, x12                // z_hi vs q_lo
    csel    x14, xzr, x22, lo       // z_hi<q_lo: +0
    csel    x15, xzr, x23, lo       // else: +b
    cinc    x13, x13, lo            // q += borrow
    adds    x26, x17, x14           // r_lo
    adc     x25, x25, x15           // r_hi
    cmp     x25, x23                // r >= b?
    b.lo    Ldiv2_nok
    ccmp    x26, x22, #2, eq        // r_hi==bh: cmp lo; >bh: C=1
    b.lo    Ldiv2_nok
    add     x13, x13, #1
    subs    x26, x26, x22
    sbc     x25, x25, x23           // r -= b, q++
Ldiv2_nok:
    str     x13, [x19], #-8
    subs    x21, x21, #1
    b.hi    Ldiv2_ntop
    b       Ldiv2_fin_n

.p2align 4
// main loop (unnormalised): a0 = (prev<<k)|(next>>(64-k))
Ldiv2_utop:
    ldr     x9, [x20], #-8
    lsr     x10, x9, x28
    lsl     x11, x9, x27
    orr     x9, x8, x10
    mov     x8, x11
Ldiv2_ucore:
    mul     x10, x24, x25           // lo(m*r_hi)
    umulh   x11, x24, x25           // hi(m*r_hi)
    adds    x12, x26, x10
    adc     x13, x25, x11           // q = r_hi+hi+r_lo>>64
    mul     x14, x22, x13           // lo(bl*q)
    umulh   x15, x22, x13           // hi(bl*q)
    mul     x16, x23, x13           // bh*q
    subs    x17, x9, x14            // z_lo = a0-lo
    sbc     x26, x26, x15           // z_hi = r_lo-hi-borrow
    sub     x26, x26, x16           //      - bh*q
    subs    x17, x17, x22           // z_lo -= bl
    sbc     x25, x26, x23           // z_hi -= bh-borrow
    cmp     x25, x12                // z_hi vs q_lo
    csel    x14, xzr, x22, lo       // z_hi<q_lo: +0
    csel    x15, xzr, x23, lo       // else: +b
    cinc    x13, x13, lo            // q += borrow
    adds    x26, x17, x14           // r_lo
    adc     x25, x25, x15           // r_hi
    cmp     x25, x23                // r >= b?
    b.lo    Ldiv2_uok
    ccmp    x26, x22, #2, eq        // r_hi==bh: cmp lo; >bh: C=1
    b.lo    Ldiv2_uok
    add     x13, x13, #1
    subs    x26, x26, x22
    sbc     x25, x25, x23           // r -= b, q++
Ldiv2_uok:
    str     x13, [x19], #-8
    subs    x21, x21, #1
    b.hi    Ldiv2_utop
    b.lo    Ldiv2_fin_u
Ldiv2_ulast:                        // last step: a0 = prev<<k
    mov     x9, x8
    b       Ldiv2_ucore

// r >>= k, store [numb,2]
Ldiv2_fin_u:
    lsr     x0, x26, x27
    lsl     x1, x25, x28
    orr     x0, x0, x1
    lsr     x1, x25, x27
    ldr     x2, [sp, #96]
    stp     x0, x1, [x2]
    b       Ldiv2_epi
Ldiv2_fin_n:
    ldr     x2, [sp, #96]
    stp     x26, x25, [x2]
Ldiv2_epi:
    ldp     x27, x28, [sp, #80]
    ldp     x25, x26, [sp, #64]
    ldp     x23, x24, [sp, #48]
    ldp     x21, x22, [sp, #32]
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #112
    ret

Ldiv2_to_mod:                       // remap: mod_2_(numa, na, numb)
    mov     x0, x1
    mov     x1, x2
    mov     x2, x3
    b       ASM_GSYM(lmmp_mod_2_)


// ======================================
// lmmp_div_2_s_
// ======================================
// x19=&dstq[i], x20=&numa[i], x21=cnt, x22=bl, x23=bh
// x24=m, x25=r_hi, x26=r_lo, x27=qh
.globl ASM_GSYM(lmmp_div_2_s_)
.p2align 2
ASM_GSYM(lmmp_div_2_s_):
    stp     x29, x30, [sp, #-96]!
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    stp     x25, x26, [sp, #64]
    stp     x27, x28, [sp, #80]
    add     x19, x0, x2, lsl #3
    sub     x19, x19, #24           // &dstq[na-3]
    mov     x20, x1
    add     x20, x20, x2, lsl #3
    sub     x20, x20, #8            // &numa[na-1]
    mov     x21, x2                 // na
    ldp     x22, x23, [x3]          // b0, b1 (MSB(bh)=1)
    mov     x0, x23
    mov     x1, x22
    bl      ASM_GSYM(lmmp_inv_2_1_)
    mov     x24, x0                 // m
    ldr     x26, [x20], #-8         // top limb
    ldr     x9, [x20], #-8          // second limb
    subs    x2, x9, x22
    sbcs    x1, x26, x23
    cset    x27, cs                 // qh = (a_top:a_2nd >= b)
    csel    x25, x1, x26, cs        // r_hi
    csel    x26, x2, x9, cs         // r_lo
    sub     x21, x21, #2
    b       Ldiv2s_top

.p2align 4
// main loop (normalised): x9 = next limb
Ldiv2s_top:
    ldr     x9, [x20], #-8
Ldiv2s_core:
    mul     x10, x24, x25           // lo(m*r_hi)
    umulh   x11, x24, x25           // hi(m*r_hi)
    adds    x12, x26, x10
    adc     x13, x25, x11           // q = r_hi+hi+r_lo>>64
    mul     x14, x22, x13           // lo(bl*q)
    umulh   x15, x22, x13           // hi(bl*q)
    mul     x16, x23, x13           // bh*q
    subs    x17, x9, x14            // z_lo = a0-lo
    sbc     x26, x26, x15           // z_hi = r_lo-hi-borrow
    sub     x26, x26, x16           //      - bh*q
    subs    x17, x17, x22           // z_lo -= bl
    sbc     x25, x26, x23           // z_hi -= bh-borrow
    cmp     x25, x12                // z_hi vs q_lo
    csel    x14, xzr, x22, lo       // z_hi<q_lo: +0
    csel    x15, xzr, x23, lo       // else: +b
    cinc    x13, x13, lo            // q += borrow
    adds    x26, x17, x14           // r_lo
    adc     x25, x25, x15           // r_hi
    cmp     x25, x23                // r >= b?
    b.lo    Ldiv2s_ok
    ccmp    x26, x22, #2, eq        // r_hi==bh: cmp lo; >bh: C=1
    b.lo    Ldiv2s_ok
    add     x13, x13, #1
    subs    x26, x26, x22
    sbc     x25, x25, x23           // r -= b, q++
Ldiv2s_ok:
    str     x13, [x19], #-8
    subs    x21, x21, #1
    b.hi    Ldiv2s_top
    stp     x26, x25, [x20, #8]     // [numa,2] = remainder
    mov     x0, x27                 // ret qh
    ldp     x27, x28, [sp, #80]
    ldp     x25, x26, [sp, #64]
    ldp     x23, x24, [sp, #48]
    ldp     x21, x22, [sp, #32]
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #96
    ret
