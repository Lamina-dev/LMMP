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
// mp_limb_t lmmp_addmul_2_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_srcptr numb2);
//
//   rp=x0, up=x1, n=x2, vp=x3

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_addmul_2_
// ======================================
// x4=v0, x5=v1, x12=cy0, x13=cy1, x6/x7/x16/x17=u window,
// x8/x9/x10/x11=lo0/hi0/lo1/hi1 (+ x14 second-pair hi1), x14/x15=rp
.globl ASM_GSYM(lmmp_addmul_2_)
.p2align 2
ASM_GSYM(lmmp_addmul_2_):
    ldp     x4, x5, [x3]                // v0, v1
    mov     x12, xzr                    // cy0 = 0
    mov     x13, xzr                    // cy1 = 0
    tbz     x2, #0, Lam2_1              // n mod 2 == 0

    ldr     x6, [x1], #8                // u0
    mul     x8, x6, x4                  // lo0
    umulh   x9, x6, x4                  // hi0
    mul     x10, x6, x5                 // lo1
    umulh   x11, x6, x5                 // hi1
    ldr     x14, [x0]
    adds    x8, x8, x14                 // t = lo0 + rp
    adcs    x9, x9, x10                 // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr               // h2 = hi1 + c2
    adds    x8, x8, x12                 // s = t + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13                // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr               // cy1 = h2 + c5

Lam2_1:
    tbz     x2, #1, Lam2_2              // n mod 4 < 2

    ldp     x6, x7, [x1], #16           // u0, u1
    ldr     x14, [x0]
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x8, x8, x14                 // t = lo0 + rp
    adcs    x9, x9, x10                 // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr               // h2 = hi1 + c2
    adds    x8, x8, x12                 // s = t + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13                // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr               // cy1 = h2 + c5

    ldr     x14, [x0]
    mul     x8, x7, x4
    umulh   x9, x7, x4
    mul     x10, x7, x5
    umulh   x11, x7, x5
    adds    x8, x8, x14                 // t = lo0 + rp
    adcs    x9, x9, x10                 // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr               // h2 = hi1 + c2
    adds    x8, x8, x12                 // s = t + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13                // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr               // cy1 = h2 + c5

Lam2_2:
    lsr     x2, x2, #2                  // 4-limb groups left
    cbz     x2, Lam2_end
.p2align 4

Lam2_top:
    ldp     x6, x7, [x1], #32           // u0, u1
    ldp     x16, x17, [x1, #-16]        // u2, u3
    ldp     x14, x15, [x0]              // rp0, rp1

    mul     x8, x6, x4                  // lo0
    umulh   x9, x6, x4                  // hi0
    mul     x10, x6, x5                 // lo1
    umulh   x11, x6, x5                 // hi1
    adds    x8, x8, x14                 // t = lo0 + rp
    adcs    x9, x9, x10                 // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr               // h2 = hi1 + c2
    adds    x8, x8, x12                 // s = t + cy0
    adcs    x12, x9, x13                // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr               // cy1 = h2 + c5

    mul     x9, x7, x4                  // lo0'
    umulh   x10, x7, x4                 // hi0'
    mul     x11, x7, x5                 // lo1'
    umulh   x14, x7, x5                 // hi1'
    adds    x9, x9, x15                 // t' = lo0' + rp
    adcs    x10, x10, x11               // h' = hi0' + lo1' + c1
    adcs    x14, x14, xzr               // h2' = hi1' + c2
    adds    x9, x9, x12                 // s' = t' + cy0
    adcs    x12, x10, x13               // cy0 = h' + cy1 + c4
    adcs    x13, x14, xzr               // cy1 = h2' + c5
    stp     x8, x9, [x0], #16

    ldp     x14, x15, [x0]              // rp2, rp3
    mul     x8, x16, x4                 // lo0
    umulh   x9, x16, x4                 // hi0
    mul     x10, x16, x5                // lo1
    umulh   x11, x16, x5                // hi1
    adds    x8, x8, x14                 // t = lo0 + rp
    adcs    x9, x9, x10                 // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr               // h2 = hi1 + c2
    adds    x8, x8, x12                 // s = t + cy0
    adcs    x12, x9, x13                // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr               // cy1 = h2 + c5

    mul     x9, x17, x4                 // lo0'
    umulh   x10, x17, x4                // hi0'
    mul     x11, x17, x5                // lo1'
    umulh   x14, x17, x5                // hi1'
    adds    x9, x9, x15                 // t' = lo0' + rp
    adcs    x10, x10, x11               // h' = hi0' + lo1' + c1
    adcs    x14, x14, xzr               // h2' = hi1' + c2
    adds    x9, x9, x12                 // s' = t' + cy0
    adcs    x12, x10, x13               // cy0 = h' + cy1 + c4
    adcs    x13, x14, xzr               // cy1 = h2' + c5
    stp     x8, x9, [x0], #16

    subs    x2, x2, #1
    b.ne    Lam2_top

Lam2_end:
    str     x12, [x0]                   // rp[n] = cy0
    mov     x0, x13                     // ret = cy1
    ret
