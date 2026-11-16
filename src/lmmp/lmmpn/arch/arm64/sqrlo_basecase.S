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
// void lmmp_sqrlo_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t n);
//
//   dst=x0, numa=x1, n=x2

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_sqrlo_basecase_
// ======================================
// x19=dst, x20=numa, x21=n, x22=row len
// kernel temps: x4=v0, x5=v1, x12=cy0, x13=cy1, x6/x7/x16/x17=u,
// x8/x9/x10/x11/x14/x15=prod+sums, x0/x1/x2=rp/up/len walkers
.globl ASM_GSYM(lmmp_sqrlo_basecase_)
.p2align 2
ASM_GSYM(lmmp_sqrlo_basecase_):
    cmp     x2, #3
    b.hi    Lsq_big                   // n >= 4
    cmp     x2, #1
    b.hi    Lsq_n23
    ldr     x4, [x1]                  // n == 1: dst[0] = lo(a0^2)
    mul     x5, x4, x4
    str     x5, [x0]
    ret

Lsq_n23:                              // n == 2: lo(a0^2) | (2*a0*a1 + hi(a0^2)) mod B
    cmp     x2, #2
    b.hi    Lsq_n3
    ldr     x4, [x1]
    ldr     x5, [x1, #8]
    mul     x6, x4, x4                // lo(a0^2)
    umulh   x7, x4, x4                // hi(a0^2)
    mul     x8, x4, x5                // lo(a0*a1)
    str     x6, [x0]
    adds    x8, x8, x8                // 2*lo mod B
    add     x8, x8, x7                // + hi
    str     x8, [x0, #8]
    ret

Lsq_n3:                               // n == 3: mod B accumulation
    ldr     x4, [x1]
    ldr     x5, [x1, #8]
    ldr     x6, [x1, #16]
    mul     x12, x4, x4               // lo(a0^2)
    umulh   x11, x4, x4               // hi(a0^2)
    mul     x9, x4, x5                // lo(a0*a1)
    umulh   x10, x4, x5               // hi(a0*a1)
    mul     x7, x4, x6                // lo(a0*a2)
    mul     x8, x5, x5                // lo(a1^2)
    str     x12, [x0]                 // dst[0]
    adds    x9, x9, x9                // 2*lo01 mod B, C = c1
    cset    x13, cs
    adds    x9, x9, x11               // dst[1] mod B, C = c2
    adc     x13, x13, xzr             // inter-col carry = c1 + c2
    str     x9, [x0, #8]
    adds    x7, x7, x10               // t = lo02 + hi01
    adds    x7, x7, x7                // 2t
    add     x7, x7, x13               // + inter-col carry
    add     x8, x7, x8                // + lo(a1^2)
    str     x8, [x0, #16]
    ret

Lsq_big:
    stp     x29, x30, [sp, #-64]!     // save regs
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    mov     x19, x0                   // dst
    mov     x20, x1                   // numa
    mov     x21, x2                   // n

    // seed pair (a[0], a[1]): corner at col 1, fresh mul over [numa+2, n-3],
    // tail a[0]*a[n-1] into dst[n-1] (col n carry discarded)
    ldp     x4, x5, [x20]             // v0, v1
    mul     x6, x4, x5                // corner lo
    umulh   x7, x4, x5                // corner hi (<= B-2)
    str     x6, [x19, #8]             // dst[1] = corner lo (fresh)
    mov     x12, x7                   // cy0 = corner hi
    mov     x13, xzr                  // cy1 = 0
    add     x0, x19, #16              // rp = dst + 2
    add     x1, x20, #16              // up = a + 2
    sub     x2, x21, #3               // len = n - 3
    mov     x22, x2
    tbz     x2, #0, Lsq_s1
    ldr     x6, [x1], #8              // u0
    mul     x8, x6, x4                // lo0
    umulh   x9, x6, x4                // hi0
    mul     x10, x6, x5               // lo1
    umulh   x11, x6, x5               // hi1
    adds    x9, x9, x10               // h = hi0 + lo1  (c1 = 0)
    adcs    x11, x11, xzr             // h2 = hi1 + c2
    adds    x8, x8, x12               // s = lo0 + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13              // cy0
    adcs    x13, x11, xzr             // cy1

Lsq_s1:
    tbz     x2, #1, Lsq_s2
    ldp     x6, x7, [x1], #16         // u0, u1
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x9, x9, x10               // h
    adcs    x11, x11, xzr             // h2
    adds    x8, x8, x12               // s0 = lo0 + cy0
    adcs    x12, x9, x13              // cy0
    adcs    x13, x11, xzr             // cy1
    str     x8, [x0], #8
    mul     x8, x7, x4
    umulh   x9, x7, x4
    mul     x10, x7, x5
    umulh   x11, x7, x5
    adds    x9, x9, x10               // h
    adcs    x11, x11, xzr             // h2
    adds    x8, x8, x12               // s1
    adcs    x12, x9, x13              // cy0
    adcs    x13, x11, xzr             // cy1
    str     x8, [x0], #8

Lsq_s2:
    lsr     x2, x2, #2
    cbz     x2, Lsq_send
.p2align 4

Lsq_stop:
    ldp     x6, x7, [x1], #32         // u0, u1
    ldp     x16, x17, [x1, #-16]      // u2, u3
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x9, x9, x10               // h
    adcs    x11, x11, xzr             // h2
    adds    x8, x8, x12               // s0 = lo0 + cy0
    adcs    x12, x9, x13              // cy0
    adcs    x13, x11, xzr             // cy1
    mul     x9, x7, x4
    umulh   x10, x7, x4
    mul     x11, x7, x5
    umulh   x14, x7, x5
    adds    x10, x10, x11             // h'
    adcs    x14, x14, xzr             // h2'
    adds    x9, x9, x12               // s1
    adcs    x12, x10, x13             // cy0
    adcs    x13, x14, xzr             // cy1
    stp     x8, x9, [x0], #16
    mul     x8, x16, x4
    umulh   x9, x16, x4
    mul     x10, x16, x5
    umulh   x11, x16, x5
    adds    x9, x9, x10               // h
    adcs    x11, x11, xzr             // h2
    adds    x8, x8, x12               // s2
    adcs    x12, x9, x13              // cy0
    adcs    x13, x11, xzr             // cy1
    mul     x9, x17, x4
    umulh   x10, x17, x4
    mul     x11, x17, x5
    umulh   x14, x17, x5
    adds    x10, x10, x11             // h'
    adcs    x14, x14, xzr             // h2'
    adds    x9, x9, x12               // s3
    adcs    x12, x10, x13             // cy0
    adcs    x13, x14, xzr             // cy1
    stp     x8, x9, [x0], #16
    subs    x2, x2, #1
    b.ne    Lsq_stop

Lsq_send:                             // x0 = dst+n-1, x1 = &a[n-1]
    ldr     x6, [x1]                  // a[n-1]
    mul     x8, x6, x4                // lo(a[0] * a[n-1])
    adds    x8, x8, x12               // + cy0, col n carry discarded
    str     x8, [x0]                  // dst[n-1] (fresh)

    // pair rows m >= 1: v = (a[2m], a[2m+1]), corner at col 4m+1,
    // addmul_2 over [numa+2m+2, n-4m-3] at rp = dst+4m+2 (rows shrink by 4),
    // tail a[2m] * a[n-1-2m] into dst[n-1]; cy1 at col n discarded
Lsq_trans:                            // row end ptrs + len done -> next row
    sub     x0, x0, x22, lsl #3       // rp += 32 (next row)
    add     x0, x0, #32
    sub     x1, x1, x22, lsl #3       // up += 16 (next row)
    add     x1, x1, #16
    subs    x22, x22, #4              // len -= 4
    b.lt    Lsq_last                  // len < 0: len -1 = lone corner, done below
    ldp     x4, x5, [x1, #-16]        // v = (a[2m], a[2m+1])
    mul     x6, x4, x5                // corner lo
    umulh   x7, x4, x5                // corner hi
    ldr     x8, [x0, #-8]             // anchor dst[4m+1]
    adds    x8, x8, x6
    str     x8, [x0, #-8]
    cinc    x12, x7, cs               // cy0 = corner hi + c (<= B-1)
    mov     x13, xzr                  // cy1 = 0
    mov     x2, x22                   // len (len == 0 skips kernel below)
    tbz     x22, #0, Lsq_p1
    ldr     x6, [x1], #8
    mul     x8, x6, x4                // lo0
    umulh   x9, x6, x4                // hi0
    mul     x10, x6, x5               // lo1
    umulh   x11, x6, x5               // hi1
    ldr     x14, [x0]
    adds    x8, x8, x14               // t = lo0 + rp
    adcs    x9, x9, x10               // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr             // h2 = hi1 + c2
    adds    x8, x8, x12               // s = t + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13              // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr             // cy1 = h2 + c5

Lsq_p1:
    tbz     x22, #1, Lsq_p2
    ldp     x6, x7, [x1], #16         // u0, u1
    ldr     x14, [x0]
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x8, x8, x14               // t = lo0 + rp
    adcs    x9, x9, x10               // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr             // h2 = hi1 + c2
    adds    x8, x8, x12               // s = t + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13              // cy0
    adcs    x13, x11, xzr             // cy1
    ldr     x14, [x0]
    mul     x8, x7, x4
    umulh   x9, x7, x4
    mul     x10, x7, x5
    umulh   x11, x7, x5
    adds    x8, x8, x14               // t
    adcs    x9, x9, x10               // h
    adcs    x11, x11, xzr             // h2
    adds    x8, x8, x12               // s
    str     x8, [x0], #8
    adcs    x12, x9, x13              // cy0
    adcs    x13, x11, xzr             // cy1

Lsq_p2:
    lsr     x2, x2, #2
    cbz     x2, Lsq_pend
.p2align 4

Lsq_ptop:
    ldp     x6, x7, [x1], #32         // u0, u1
    ldp     x16, x17, [x1, #-16]      // u2, u3
    ldp     x14, x15, [x0]            // rp0, rp1
    mul     x8, x6, x4                // lo0
    umulh   x9, x6, x4                // hi0
    mul     x10, x6, x5               // lo1
    umulh   x11, x6, x5               // hi1
    adds    x8, x8, x14               // t = lo0 + rp
    adcs    x9, x9, x10               // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr             // h2 = hi1 + c2
    adds    x8, x8, x12               // s = t + cy0
    adcs    x12, x9, x13              // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr             // cy1 = h2 + c5
    mul     x9, x7, x4                // lo0'
    umulh   x10, x7, x4               // hi0'
    mul     x11, x7, x5               // lo1'
    umulh   x14, x7, x5               // hi1'
    adds    x9, x9, x15               // t' = lo0' + rp
    adcs    x10, x10, x11             // h' = hi0' + lo1' + c1
    adcs    x14, x14, xzr             // h2' = hi1' + c2
    adds    x9, x9, x12               // s' = t' + cy0
    adcs    x12, x10, x13             // cy0 = h' + cy1 + c4
    adcs    x13, x14, xzr             // cy1 = h2' + c5
    stp     x8, x9, [x0], #16
    ldp     x14, x15, [x0]            // rp2, rp3
    mul     x8, x16, x4               // lo0
    umulh   x9, x16, x4               // hi0
    mul     x10, x16, x5              // lo1
    umulh   x11, x16, x5              // hi1
    adds    x8, x8, x14               // t
    adcs    x9, x9, x10               // h
    adcs    x11, x11, xzr             // h2
    adds    x8, x8, x12               // s
    adcs    x12, x9, x13              // cy0
    adcs    x13, x11, xzr             // cy1
    mul     x9, x17, x4               // lo0'
    umulh   x10, x17, x4              // hi0'
    mul     x11, x17, x5              // lo1'
    umulh   x14, x17, x5              // hi1'
    adds    x9, x9, x15               // t'
    adcs    x10, x10, x11             // h'
    adcs    x14, x14, xzr             // h2'
    adds    x9, x9, x12               // s'
    adcs    x12, x10, x13             // cy0
    adcs    x13, x14, xzr             // cy1
    stp     x8, x9, [x0], #16
    subs    x2, x2, #1
    b.ne    Lsq_ptop

Lsq_pend:                             // x0 = dst+n-1, x1 = &a[n-1-2m]
    ldr     x6, [x1]                  // a[n-1-2m]
    mul     x8, x6, x4                // lo(a[2m] * a[n-1-2m])
    ldr     x9, [x0]
    add     x8, x9, x8                // + dst[n-1]
    add     x8, x8, x12               // + cy0; both col n carries discarded
    str     x8, [x0]
    b       Lsq_trans

Lsq_last:                             // len == -1: corner a[2m]*a[2m+1] at dst[n-1]
    cmp     x22, #-1
    b.ne    Lsq_diag                  // len <= -2: rows done
    ldr     x4, [x1, #-16]            // a[2m]
    ldr     x5, [x1, #-8]             // a[2m+1]
    mul     x6, x4, x5
    ldr     x8, [x0, #-8]             // dst[n-1]
    adds    x8, x8, x6                // + lo(corner), hi + C discarded
    str     x8, [x0, #-8]

    // fused tail: dst = 2*T + diag mod B^n, extr doubling (each limb <= B-1)
    // plus a_i^2 lo/hi on a single adcs chain (col sums <= 2B-1)
Lsq_diag:
    ldr     x6, [x20]                 // a[0]
    mul     x10, x6, x6               // lo(a[0]^2)
    umulh   x11, x6, x6               // hi(a[0]^2)
    str     x10, [x19]                // dst[0] = lo (T[0] = 0)
    ldr     x9, [x19, #8]             // T[1]
    extr    x13, x9, xzr, #63         // 2*T[1]
    mov     x5, x9                    // prev
    adds    x13, x13, x11             // + hi(a[0]^2), chain C starts
    str     x13, [x19, #8]
    add     x0, x19, #16
    add     x1, x20, #8
    sub     x2, x21, #2
    lsr     x2, x2, #1                // pairs = (n-2)/2 (>= 1 for n >= 4)
.p2align 4

Lsq_dtop:
    ldr     x6, [x1], #8              // a[i]
    ldp     x8, x9, [x0]              // T[2i], T[2i+1]
    extr    x12, x8, x5, #63          // 2*T[2i] | T[2i-1] MSB
    extr    x13, x9, x8, #63          // 2*T[2i+1] | T[2i] MSB
    mov     x5, x9                    // prev = T[2i+1]
    mul     x10, x6, x6               // lo(a[i]^2)
    umulh   x11, x6, x6               // hi(a[i]^2)
    adcs    x12, x12, x10             // + lo (chain from prev pair)
    adcs    x13, x13, x11             // + hi
    stp     x12, x13, [x0], #16
    sub     x2, x2, #1                // no flags: keep the chain carry
    cbnz    x2, Lsq_dtop

    tbnz    x21, #0, Lsq_dtail        // n odd: lone tail col n-1
    b       Lsq_done

Lsq_dtail:                            // 2*T[n-1] | T[n-2] MSB + lo(a[(n-1)/2]^2)
    ldr     x6, [x1]                  // a[(n-1)/2]
    mul     x10, x6, x6               // lo
    ldr     x8, [x0]                  // T[n-1]
    extr    x12, x8, x5, #63          // 2*T[n-1] | T[n-2] MSB
    adcs    x12, x12, x10             // + lo + C, out-of-range carry dropped
    str     x12, [x0]

Lsq_done:
    ldp     x21, x22, [sp, #32]       // restore regs, ret
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #64
    ret
