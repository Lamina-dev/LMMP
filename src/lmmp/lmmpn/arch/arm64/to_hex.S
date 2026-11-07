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
// mp_size_t lmmp_to_hex_(char* dst, mp_srcptr numa, mp_size_t na, bool upper);
//
//   dst=x0, numa=x1, na=x2, upper=w3

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_to_hex_
// ======================================
.globl ASM_GSYM(lmmp_to_hex_)
.p2align 4
ASM_GSYM(lmmp_to_hex_):
    // strip leading zeros: x9 = na-1
    mov          x9, x2
.Lstrip:
    cbz          x9, .Lret0
    sub          x9, x9, #1
    ldr          x10, [x1, x9, lsl #3]
    cbz          x10, .Lstrip
    add          x2, x9, #1                 // x2 = na

    // digits = ((na-1)*64 + bitlen(top) + 3) >> 2 -> x9
    clz          x11, x10
    mov          w12, #64
    sub          w11, w12, w11              // bitlen(top)
    lsl          x12, x9, #6
    add          x12, x12, x11
    add          x12, x12, #3
    lsr          x9, x12, #2

    // LUT base x10 (two 16B tables, upper = high 16B); plain adr form is
    // GAS/clang portable; tables in .text keep +-1MB range
    adr          x10, .Lhex_lut
    add          x11, x10, #16
    cmp          w3, #0
    csel         x10, x11, x10, ne

    // nfull = na-1; na==1: only top limb tail
    sub          x2, x2, #1
    cbz          x2, .Ltop
    ldr          q0, [x10]
    movi         v1.16b, #15
    and          x15, x2, #3                  // tail limbs
    lsr          x2, x2, #2                   // main loop count
    cbz          x2, .Ltail

.p2align 4
.Lmain:                                       // 4 limb -> 64 chars
    ldp          q2, q3, [x1], #32
    and          v4.16b, v2.16b, v1.16b       // lo nibbles
    and          v5.16b, v3.16b, v1.16b
    ushr         v2.16b, v2.16b, #4           // hi nibbles
    ushr         v3.16b, v3.16b, #4
    tbl          v4.16b, {v0.16b}, v4.16b     // loc
    tbl          v5.16b, {v0.16b}, v5.16b
    tbl          v2.16b, {v0.16b}, v2.16b     // hic
    tbl          v3.16b, {v0.16b}, v3.16b
    zip1         v6.16b, v4.16b, v2.16b       // limb i chars
    zip2         v7.16b, v4.16b, v2.16b       // limb i+1 chars
    stp          q6, q7, [x0], #32
    zip1         v6.16b, v5.16b, v3.16b       // limb i+2
    zip2         v7.16b, v5.16b, v3.16b       // limb i+3
    stp          q6, q7, [x0], #32
    subs         x2, x2, #1
    b.ne         .Lmain

.Ltail:                                       // 0..3 limbs, 1 per iter
    cbz          x15, .Ltop
.Ltail_loop:
    ldr          d2, [x1], #8                 // high 64 bits zero
    and          v3.16b, v2.16b, v1.16b
    ushr         v2.16b, v2.16b, #4
    tbl          v3.16b, {v0.16b}, v3.16b
    tbl          v2.16b, {v0.16b}, v2.16b
    zip1         v4.16b, v3.16b, v2.16b       // all 16 chars
    str          q4, [x0], #16
    subs         x15, x15, #1
    b.ne         .Ltail_loop

.Ltop:                                        // top limb tail
    ldr          x11, [x1]                    // x1 = &numa[nfull]
.Ltop_pair:
    lsr          x12, x11, #4
    cbz          x12, .Ltop_one               // low nibble only
    and          x13, x11, #0xF
    ldrb         w14, [x10, x13]              // lo nibble char
    and          x13, x12, #0xF               // x12 = x11>>4
    ldrb         w15, [x10, x13]              // hi nibble char
    strb         w14, [x0], #1                // lo first
    strb         w15, [x0], #1
    lsr          x11, x11, #8
    b            .Ltop_pair
.Ltop_one:                                    // x11 in [0,15]
    cbz          x11, .Lret
    ldrb         w14, [x10, x11]
    strb         w14, [x0]
.Lret:
    mov          x0, x9                       // digits
    ret

.Lret0:
    mov          x0, #0
    ret

.p2align 4
.Lhex_lut:
    .ascii "0123456789abcdef"
    .ascii "0123456789ABCDEF"
