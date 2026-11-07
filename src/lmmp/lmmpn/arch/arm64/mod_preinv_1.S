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
// mp_limb_t lmmp_mod_preinv_1_(mp_srcptr numa, mp_size_t na, mp_limb_t x, mp_limb_t inv, mp_limb_t k);
//
//   numa=x0, na=x1, x=x2, inv=x3, k=x4
//

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_mod_preinv_1_
// ======================================
.globl ASM_GSYM(lmmp_mod_preinv_1_)
.p2align 2
ASM_GSYM(lmmp_mod_preinv_1_):
// x20=&numa[na-1], x21=na-1, x22=d, x23=k, x0=m
    stp     x29, x30, [sp, #-48]!
    stp     x20, x21, [sp, #16]
    stp     x22, x23, [sp, #32]
    mov     x23, x4
    mov     x22, x2
    mov     x21, x1
    sub     x21, x21, #1
    mov     x20, x0
    add     x20, x20, x21, lsl #3
    mov     x0, x3                  // x0 = m = inv
    cbnz    x23, Lpi_uentry
    b       Lpi_nentry

// unnormalised: d = x << k, x11 = xh = a_top>>(64-k), x1 = a_lo = a_top<<k
Lpi_uentry:
    lsl     x22, x22, x23
    ldr     x7, [x20], #-8          // top limb
    sub     x8, xzr, x23
    lsr     x11, x7, x8
    lsl     x1, x7, x23
    cbz     x21, Lpi_uend

// main loop (unnormalised): x11 = xh < d, x1 = a_lo
Lpi_utop:
    ldr     x7, [x20], #-8
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    lsr     x9, x7, x8
    orr     x1, x1, x9              // a_lo = (prev<<k)|(x7>>(64-k))
    adds    x10, x1, x10
    adc     x2, x2, x17             // x2 = q
    msub    x11, x22, x2, x1        // z = a_lo - d*q
    lsl     x1, x7, x23
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc       // borrow: z += d
    sbc     x2, x2, xzr             // borrow: q--
    cmp     x11, x22
    bcs     Lpi_ufx

Lpi_uok:                            // no quotient stored
    sub     x21, x21, #1
    cbnz    x21, Lpi_utop

// last limb, second fixup
Lpi_uend:
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    adds    x10, x1, x10
    adc     x2, x2, x17
    msub    x11, x22, x2, x1
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc
    sbc     x2, x2, xzr
    subs    x14, x11, x22           // z >= d: z -= d
    adc     x2, x2, xzr
    csel    x11, x14, x11, cs
    lsr     x0, x11, x23            // r = z >> k
    ldp     x20, x21, [sp, #16]
    ldp     x22, x23, [sp, #32]
    ldp     x29, x30, [sp], #48
    ret

Lpi_ufx:                            // z >= d: z -= d
    add     x2, x2, #1
    sub     x11, x11, x22
    b       Lpi_uok

// normalised path, k = 0
Lpi_nentry:
    ldr     x7, [x20], #-8          // top limb
    subs    x14, x7, x22
    csel    x11, x14, x7, cs        // x11 = xh
    b       Lpi_nok

// main loop (normalised)
Lpi_ntop:
    ldr     x1, [x20], #-8
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    adds    x10, x1, x10
    adc     x2, x2, x17
    msub    x11, x22, x2, x1
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc
    sbc     x2, x2, xzr
    cmp     x11, x22
    bcs     Lpi_nfx

Lpi_nok:                            // no quotient stored
    sub     x21, x21, #1
    tbz     x21, #63, Lpi_ntop
    mov     x0, x11                 // ret remainder
    ldp     x20, x21, [sp, #16]
    ldp     x22, x23, [sp, #32]
    ldp     x29, x30, [sp], #48
    ret

Lpi_nfx:                            // z >= d: z -= d
    add     x2, x2, #1
    sub     x11, x11, x22
    b       Lpi_nok
