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

// void lmmp_dec_to_chars_(mp_byte_t* dst, mp_size_t n);
// void lmmp_b36_to_chars_(mp_byte_t* dst, mp_size_t n, bool upper);
//
//   x0=dst, x1=n, w2=upper
//   ret: none
//   cal: dst[i] = '0' + dst[i] (+ adj*(dst[i]>9)),
//           adj = 39(lowercase)/7(capitals),
//

#include "lmmp_asm.h"

.text

// ---------- lmmp_dec_to_chars_: [0,9] -> ['0','9'] ----------

.globl ASM_GSYM(lmmp_dec_to_chars_)
.p2align 2

ASM_GSYM(lmmp_dec_to_chars_):
    movi    v0.16b, #48               // v0 = '0'
    lsr     x2, x1, #6                // x2 = n >> 6
    cbz     x2, Ldec_tail

.p2align 4
Ldec_loop:                            // main loop: 64 bytes
    ld1     {v1.16b, v2.16b, v3.16b, v4.16b}, [x0]
    add     v1.16b, v1.16b, v0.16b    // x + '0'
    add     v2.16b, v2.16b, v0.16b
    add     v3.16b, v3.16b, v0.16b
    add     v4.16b, v4.16b, v0.16b
    st1     {v1.16b, v2.16b, v3.16b, v4.16b}, [x0], #64
    subs    x2, x2, #1
    b.ne    Ldec_loop

Ldec_tail:
    ands    x2, x1, #63               // remainder bytes
    b.eq    Ldec_done

Ldec_tail_loop:                       // remainder bytes
    ldrb    w3, [x0]
    add     w3, w3, #48
    strb    w3, [x0], #1
    subs    x2, x2, #1
    b.ne    Ldec_tail_loop

Ldec_done:
    ret

// ---------- lmmp_b36_to_chars_: [0,35] -> ['0'-'9',a-z/A-Z] ----------

.globl ASM_GSYM(lmmp_b36_to_chars_)
.p2align 2

ASM_GSYM(lmmp_b36_to_chars_):
    mov     w3, #39                   // adj: lowercase 39
    mov     w4, #7                    // adj: capitals 7
    cmp     w2, #0
    csel    w3, w3, w4, eq            // upper==0 -> 39 else 7

    dup     v5.16b, w3                // v5 = adj
    movi    v0.16b, #48               // v0 = '0'
    movi    v6.16b, #9                // v6 = 9
    lsr     x2, x1, #6                // x2 = n >> 6
    cbz     x2, Lb36_tail

.p2align 4
Lb36_loop:                            // main loop: 64 bytes

    ld1     {v1.16b, v2.16b, v3.16b, v4.16b}, [x0]
    cmgt    v16.16b, v1.16b, v6.16b   // mask = x > 9 (signed cmp, x>=0)
    cmgt    v17.16b, v2.16b, v6.16b
    cmgt    v18.16b, v3.16b, v6.16b
    cmgt    v19.16b, v4.16b, v6.16b
    and     v16.16b, v16.16b, v5.16b  // mask & adj
    and     v17.16b, v17.16b, v5.16b
    and     v18.16b, v18.16b, v5.16b
    and     v19.16b, v19.16b, v5.16b
    add     v1.16b, v1.16b, v0.16b    // x + '0'
    add     v2.16b, v2.16b, v0.16b
    add     v3.16b, v3.16b, v0.16b
    add     v4.16b, v4.16b, v0.16b
    add     v1.16b, v1.16b, v16.16b   // + adj*(x>9)
    add     v2.16b, v2.16b, v17.16b
    add     v3.16b, v3.16b, v18.16b
    add     v4.16b, v4.16b, v19.16b
    st1     {v1.16b, v2.16b, v3.16b, v4.16b}, [x0], #64
    subs    x2, x2, #1
    b.ne    Lb36_loop

Lb36_tail:
    ands    x2, x1, #63               // remainder bytes
    b.eq    Lb36_done

Lb36_tail_loop:                       // remainder bytes
    ldrb    w4, [x0]
    add     w5, w4, #48
    cmp     w4, #9
    b.ls    Lb36_st
    add     w5, w5, w3                // + adj
Lb36_st:
    strb    w5, [x0], #1
    subs    x2, x2, #1
    b.ne    Lb36_tail_loop

Lb36_done:
    ret
