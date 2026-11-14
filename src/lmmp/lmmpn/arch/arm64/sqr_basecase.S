//  Copyright (C) 2026 HJimmyK (Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it
//  under the terms of the GNU Lesser General Public License as
//  published by the Free Software Foundation, either version 3 of
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
// void lmmp_sqr_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
//
//   dst=x0, numa=x1, na=x2

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_sqr_basecase_
// ======================================
// x19=dst, x20=numa, x21=na, x22=row len
// kernel temps: x4=v0, x5=v1, x12=cy0, x13=cy1, x6/x7/x16/x17=u,
// x8/x9/x10/x11/x14/x15=prod+sums, x0/x1/x2=rp/up/len walkers
.globl ASM_GSYM(lmmp_sqr_basecase_)
.p2align 2
ASM_GSYM(lmmp_sqr_basecase_):
    cmp     x2, #3
    b.ls    Lsqr_le3                 // na <= 3 small path

    stp     x29, x30, [sp, #-64]!    // save regs
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    mov     x19, x0                  // dst
    mov     x20, x1                  // numa
    mov     x21, x2                  // na

    // seed pair (a[0], a[1]): corner at col 1, mul over [numa+2, n-2]
    ldp     x4, x5, [x20]            // v0, v1
    mul     x6, x4, x5               // corner lo
    umulh   x7, x4, x5               // corner hi (<= B-2)
    str     x6, [x19, #8]            // dst[1] = corner lo (fresh)
    mov     x12, x7                  // cy0 = corner hi
    mov     x13, xzr                 // cy1 = 0
    add     x0, x19, #16             // rp = dst + 2
    add     x1, x20, #16             // up = a + 2
    sub     x2, x21, #2              // len = n - 2
    tbz     x2, #0, Lsq_s1
    ldr     x6, [x1], #8             // u0
    mul     x8, x6, x4               // lo0
    umulh   x9, x6, x4               // hi0
    mul     x10, x6, x5              // lo1
    umulh   x11, x6, x5              // hi1
    adds    x9, x9, x10              // h = hi0 + lo1  (c1 = 0)
    adcs    x11, x11, xzr            // h2 = hi1 + c2
    adds    x8, x8, x12              // s = lo0 + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1

Lsq_s1:
    tbz     x2, #1, Lsq_s2
    ldp     x6, x7, [x1], #16        // u0, u1
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s0 = lo0 + cy0
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    str     x8, [x0], #8
    mul     x8, x7, x4
    umulh   x9, x7, x4
    mul     x10, x7, x5
    umulh   x11, x7, x5
    adds    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s1
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    str     x8, [x0], #8

Lsq_s2:
    lsr     x2, x2, #2
    cbz     x2, Lsq_send
.p2align 4

Lsq_stop:
    ldp     x6, x7, [x1], #32        // u0, u1
    ldp     x16, x17, [x1, #-16]     // u2, u3
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s0 = lo0 + cy0
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    mul     x9, x7, x4
    umulh   x10, x7, x4
    mul     x11, x7, x5
    umulh   x14, x7, x5
    adds    x10, x10, x11            // h'
    adcs    x14, x14, xzr            // h2'
    adds    x9, x9, x12              // s1
    adcs    x12, x10, x13            // cy0
    adcs    x13, x14, xzr            // cy1
    stp     x8, x9, [x0], #16
    mul     x8, x16, x4
    umulh   x9, x16, x4
    mul     x10, x16, x5
    umulh   x11, x16, x5
    adds    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s2
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    mul     x9, x17, x4
    umulh   x10, x17, x4
    mul     x11, x17, x5
    umulh   x14, x17, x5
    adds    x10, x10, x11            // h'
    adcs    x14, x14, xzr            // h2'
    adds    x9, x9, x12              // s3
    adcs    x12, x10, x13            // cy0
    adcs    x13, x14, xzr            // cy1
    stp     x8, x9, [x0], #16
    subs    x2, x2, #1
    b.ne    Lsq_stop

Lsq_send:
    str     x12, [x0]                // col n = cy0
    str     x13, [x0, #8]            // col n+1 = cy1
    sub     x22, x21, #2             // seed row len

    // pair rows m >= 1: v = (a[2m], a[2m+1]), corner at col 4m+1,
    // addmul_2 over [numa+2m+2, n-2m-2] at rp = dst+4m+2
Lsq_trans:                           // row end ptrs + len done -> next row
    sub     x0, x0, x22, lsl #3      // rp += 32 (next row)
    add     x0, x0, #32
    sub     x1, x1, x22, lsl #3      // up += 16 (next row)
    add     x1, x1, #16
    subs    x22, x22, #2             // len = n - 2m - 4
    b.le    Lsq_last                 // len <= 0 done (len 1 = 1-limb row)
    ldp     x4, x5, [x1, #-16]       // v = (a[2m], a[2m+1])
    mul     x6, x4, x5               // corner lo
    umulh   x7, x4, x5               // corner hi
    ldr     x8, [x0, #-8]            // anchor dst[4m+1]
    adds    x8, x8, x6
    str     x8, [x0, #-8]
    cinc    x12, x7, cs              // cy0 = corner hi + c (<= B-1)
    mov     x13, xzr                 // cy1 = 0
    mov     x2, x22                  // len
    tbz     x22, #0, Lsq_p1
    ldr     x6, [x1], #8
    mul     x8, x6, x4               // lo0
    umulh   x9, x6, x4               // hi0
    mul     x10, x6, x5              // lo1
    umulh   x11, x6, x5              // hi1
    ldr     x14, [x0]
    adds    x8, x8, x14              // t = lo0 + rp
    adcs    x9, x9, x10              // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr            // h2 = hi1 + c2
    adds    x8, x8, x12              // s = t + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13             // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr            // cy1 = h2 + c5

Lsq_p1:
    tbz     x22, #1, Lsq_p2
    ldp     x6, x7, [x1], #16        // u0, u1
    ldr     x14, [x0]
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x8, x8, x14              // t = lo0 + rp
    adcs    x9, x9, x10              // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr            // h2 = hi1 + c2
    adds    x8, x8, x12              // s = t + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    ldr     x14, [x0]
    mul     x8, x7, x4
    umulh   x9, x7, x4
    mul     x10, x7, x5
    umulh   x11, x7, x5
    adds    x8, x8, x14              // t
    adcs    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s
    str     x8, [x0], #8
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1

Lsq_p2:
    lsr     x2, x2, #2
    cbz     x2, Lsq_pend
.p2align 4

Lsq_ptop:
    ldp     x6, x7, [x1], #32        // u0, u1
    ldp     x16, x17, [x1, #-16]     // u2, u3
    ldp     x14, x15, [x0]           // rp0, rp1
    mul     x8, x6, x4               // lo0
    umulh   x9, x6, x4               // hi0
    mul     x10, x6, x5              // lo1
    umulh   x11, x6, x5              // hi1
    adds    x8, x8, x14              // t = lo0 + rp
    adcs    x9, x9, x10              // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr            // h2 = hi1 + c2
    adds    x8, x8, x12              // s = t + cy0
    adcs    x12, x9, x13             // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr            // cy1 = h2 + c5
    mul     x9, x7, x4               // lo0'
    umulh   x10, x7, x4              // hi0'
    mul     x11, x7, x5              // lo1'
    umulh   x14, x7, x5              // hi1'
    adds    x9, x9, x15              // t' = lo0' + rp
    adcs    x10, x10, x11            // h' = hi0' + lo1' + c1
    adcs    x14, x14, xzr            // h2' = hi1' + c2
    adds    x9, x9, x12              // s' = t' + cy0
    adcs    x12, x10, x13            // cy0 = h' + cy1 + c4
    adcs    x13, x14, xzr            // cy1 = h2' + c5
    stp     x8, x9, [x0], #16
    ldp     x14, x15, [x0]           // rp2, rp3
    mul     x8, x16, x4              // lo0
    umulh   x9, x16, x4              // hi0
    mul     x10, x16, x5             // lo1
    umulh   x11, x16, x5             // hi1
    adds    x8, x8, x14              // t
    adcs    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    mul     x9, x17, x4              // lo0'
    umulh   x10, x17, x4             // hi0'
    mul     x11, x17, x5             // lo1'
    umulh   x14, x17, x5             // hi1'
    adds    x9, x9, x15              // t'
    adcs    x10, x10, x11            // h'
    adcs    x14, x14, xzr            // h2'
    adds    x9, x9, x12              // s'
    adcs    x12, x10, x13            // cy0
    adcs    x13, x14, xzr            // cy1
    stp     x8, x9, [x0], #16
    subs    x2, x2, #1
    b.ne    Lsq_ptop

Lsq_pend:
    str     x12, [x0]                // col n+2m = cy0
    str     x13, [x0, #8]            // col n+2m+1 = cy1
    b       Lsq_trans

Lsq_last:
    tbz     x21, #0, Lsq_ceven       // n even: final corner; n odd: zero top
    lsl     x8, x21, #4
    sub     x8, x8, #8               // &dst[2n-1]
    str     xzr, [x19, x8]
    b       Lsq_diag

Lsq_ceven:                           // corner a[n-2]*a[n-1] at col 2n-3
    sub     x8, x21, #1
    ldr     x5, [x20, x8, lsl #3]    // a[n-1]
    sub     x8, x8, #1
    ldr     x4, [x20, x8, lsl #3]    // a[n-2]
    mul     x6, x4, x5
    umulh   x7, x4, x5
    lsl     x8, x21, #4
    sub     x8, x8, #24              // &dst[2n-3]
    ldr     x9, [x19, x8]
    adds    x9, x9, x6               // dst[2n-3] += lo
    str     x9, [x19, x8]
    cinc    x9, x7, cs               // v = hi + c (<= B-1, no further carry)
    add     x8, x8, #8
    str     x9, [x19, x8]            // dst[2n-2] = v (store: sole content)
    add     x8, x8, #8
    str     xzr, [x19, x8]           // dst[2n-1] = 0

    // fused tail: dst = 2*T + diag, extr doubling (each limb <= B-1)
    // plus a_i^2 lo/hi on a single adcs chain (col sums <= 2B-1)
Lsq_diag:
    ldr     x6, [x20]                // a[0]
    mul     x10, x6, x6              // lo(a[0]^2)
    umulh   x11, x6, x6              // hi(a[0]^2)
    str     x10, [x19]               // dst[0] = lo (T[0] = 0)
    ldr     x9, [x19, #8]            // T[1]
    extr    x13, x9, xzr, #63        // 2*T[1]
    mov     x5, x9                   // prev
    adds    x13, x13, x11            // + hi(a[0]^2), chain C starts
    str     x13, [x19, #8]
    add     x0, x19, #16
    add     x1, x20, #8
    sub     x2, x21, #1              // pairs left = n - 1
    cbz     x2, Lsq_dend
.p2align 4

Lsq_dtop:
    ldr     x6, [x1], #8             // a[i]
    ldp     x8, x9, [x0]             // T[2i], T[2i+1]
    extr    x12, x8, x5, #63         // 2*T[2i] | T[2i-1] MSB
    extr    x13, x9, x8, #63         // 2*T[2i+1] | T[2i] MSB
    mov     x5, x9                   // prev = T[2i+1]
    mul     x10, x6, x6              // lo(a[i]^2)
    umulh   x11, x6, x6              // hi(a[i]^2)
    adcs    x12, x12, x10            // + lo (chain from prev pair)
    adcs    x13, x13, x11            // + hi
    stp     x12, x13, [x0], #16
    sub     x2, x2, #1               // no flags: keep the chain carry
    cbnz    x2, Lsq_dtop

Lsq_dend:
    ldp     x21, x22, [sp, #32]      // restore regs, ret
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #64
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
