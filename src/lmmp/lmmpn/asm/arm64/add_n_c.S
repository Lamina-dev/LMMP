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

// mp_limb_t lmmp_add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
// mp_limb_t lmmp_add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
//   x0=dst, x1=numa, x2=numb, x3=n, x4=c (only nc_)
//   ret x0 = highest limb carry
//   cal: dst[i] = numa[i] + numb[i] + C, adcs chain

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_add_nc_)
.p2align 2

ASM_GSYM(lmmp_add_nc_):
    cmp     x4, #1                  // C = (c != 0)
    b       Ladd_ent

.globl ASM_GSYM(lmmp_add_n_)
.p2align 2

ASM_GSYM(lmmp_add_n_):
    cmn     xzr, xzr                // 0 + 0: C = 0, carry = 0

Ladd_ent:
    lsr     x17, x3, #2             // x17 = n >> 2; 4 limb 
    tbz     x3, #0, Ladd_bx0        // n mod 2 == 0

Ladd_bx1:                           // n mod 2 == 1
    ldr     x7, [x1]
    ldr     x11, [x2]
    adcs    x13, x7, x11            // t0 = numa[0] + numb[0] + C
    str     x13, [x0], #8           // dst[0] = t0
    tbnz    x3, #1, Ladd_b11        // n mod 4 == 3

Ladd_b01:                           // n mod 4 == 1
    cbz     x17, Ladd_ret           // n == 1, takle and return
    ldp     x4, x5, [x1, #8]
    ldp     x8, x9, [x2, #8]
    sub     x1, x1, #8
    sub     x2, x2, #8
    b       Ladd_mid

Ladd_b11:                           // n mod 4 == 3
    ldp     x6, x7, [x1, #8]
    ldp     x10, x11, [x2, #8]
    add     x1, x1, #8
    add     x2, x2, #8
    cbz     x17, Ladd_end           // n == 3, return
    b       Ladd_top

Ladd_bx0:
    tbnz    x3, #1, Ladd_b10        // n mod 4 == 2

Ladd_b00:                           // n mod 4 == 0
    ldp     x4, x5, [x1]
    ldp     x8, x9, [x2]
    sub     x1, x1, #16
    sub     x2, x2, #16
    b       Ladd_mid

Ladd_b10:                           // n mod 4 == 2
    ldp     x6, x7, [x1]
    ldp     x10, x11, [x2]
    cbz     x17, Ladd_end           // n == 2, return
.p2align 4

Ladd_top:                           // takle x6/x7 pair, ldp x4/x5 pair
    ldp     x4, x5, [x1, #16]
    ldp     x8, x9, [x2, #16]
    adcs    x12, x6, x10            // t_i   = a + b + C
    adcs    x13, x7, x11            // t_i+1
    stp     x12, x13, [x0], #16

Ladd_mid:                           // takle x4/x5 pair
    ldp     x6, x7, [x1, #32]!
    ldp     x10, x11, [x2, #32]!
    adcs    x12, x4, x8             // t_i+2 = a + b + C
    adcs    x13, x5, x9             // t_i+3: C = carry in pair
    stp     x12, x13, [x0], #16
    sub     x17, x17, #1
    cbnz    x17, Ladd_top

Ladd_end:                           // return
    adcs    x12, x6, x10
    adcs    x13, x7, x11
    stp     x12, x13, [x0]

Ladd_ret:
    cset    x0, cs                  // ret highest limb carry
    ret
