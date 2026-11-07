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
// mp_size_t lmmp_from_hex_(mp_ptr dst, const char* src, mp_size_t len);
//
//   dst=x0, src=x1, len=x2

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_from_hex_
// ======================================
.globl ASM_GSYM(lmmp_from_hex_)
.p2align 4
ASM_GSYM(lmmp_from_hex_):
    // strip leading zeros: x9 = len-1
    mov          x9, x2
.Lstrip:
    cbz          x9, .Lret0
    sub          x9, x9, #1
    ldrb         w12, [x1, x9]
    cmp          w12, #48                     // '0'
    b.eq         .Lstrip
    add          x11, x9, #1                  // x11 = len

    // x10 = nfull, x9 = len&15, x11 = ret
    lsr          x10, x11, #4
    and          x9, x11, #15
    cmp          x9, #0
    cinc         x11, x10, ne

    movi         v2.16b, #48
    movi         v4.16b, #9
    movi         v6.16b, #97
    movi         v24.16b, #7
    movi         v25.16b, #32

    lsr          x2, x10, #1                  // pairs
    cbz          x2, .Lsingle

.p2align 4
.Lmain:                                       // 32 chars -> 2 limbs
    ldp          q16, q17, [x1], #32
    sub          v18.16b, v16.16b, v2.16b     // t
    sub          v19.16b, v17.16b, v2.16b
    cmhi         v20.16b, v18.16b, v4.16b     // letter
    cmhi         v21.16b, v19.16b, v4.16b
    cmge         v22.16b, v16.16b, v6.16b     // lower
    cmge         v23.16b, v17.16b, v6.16b
    and          v20.16b, v20.16b, v24.16b
    and          v21.16b, v21.16b, v24.16b
    and          v22.16b, v22.16b, v25.16b
    and          v23.16b, v23.16b, v25.16b
    sub          v18.16b, v18.16b, v20.16b
    sub          v19.16b, v19.16b, v21.16b
    sub          v18.16b, v18.16b, v22.16b    // nib1
    sub          v19.16b, v19.16b, v23.16b    // nib2
    uzp1         v20.16b, v18.16b, v18.16b    // even bytes
    uzp2         v21.16b, v18.16b, v18.16b    // odd bytes
    shl          v21.16b, v21.16b, #4
    orr          v20.16b, v20.16b, v21.16b    // limb1
    uzp1         v22.16b, v19.16b, v19.16b
    uzp2         v23.16b, v19.16b, v19.16b
    shl          v23.16b, v23.16b, #4
    orr          v22.16b, v22.16b, v23.16b    // limb2
    stp          d20, d22, [x0], #16
    subs         x2, x2, #1
    b.ne         .Lmain

.Lsingle:                                     // lone limb (16 chars)
    tbz          x10, #0, .Lrem
    ldr          q16, [x1], #16
    sub          v17.16b, v16.16b, v2.16b
    cmhi         v18.16b, v17.16b, v4.16b
    cmge         v19.16b, v16.16b, v6.16b
    and          v18.16b, v18.16b, v24.16b
    and          v19.16b, v19.16b, v25.16b
    sub          v17.16b, v17.16b, v18.16b
    sub          v17.16b, v17.16b, v19.16b
    uzp1         v18.16b, v17.16b, v17.16b
    uzp2         v19.16b, v17.16b, v17.16b
    shl          v19.16b, v19.16b, #4
    orr          v18.16b, v18.16b, v19.16b
    str          d18, [x0], #8

.Lrem:                                        // tail: x9 nibbles -> 1 limb
    cbz          x9, .Ldone
    mov          x12, #0
.Lrem_loop:
    subs         x9, x9, #1
    ldrb         w13, [x1, x9]                // high to low
    sub          w14, w13, #48
    cmp          w14, #9
    b.ls         .Lrnb
    cmp          w13, #96
    b.le         .Lrup
    sub          w14, w14, #39                // 'a'-'f'
    b            .Lrnb
.Lrup:
    sub          w14, w14, #7                 // 'A'-'F'
.Lrnb:
    lsl          x12, x12, #4
    orr          x12, x12, x14
    cbnz         x9, .Lrem_loop
    str          x12, [x0]

.Ldone:
    mov          x0, x11
    ret

.Lret0:
    mov          x0, #0
    ret
