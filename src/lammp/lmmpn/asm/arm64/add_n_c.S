#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#  by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY.
#
#  See <https://www.gnu.org/licenses/>.

# mp_limb_t lmmp_add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
# mp_limb_t lmmp_add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_add_nc_)
.p2align 2

ASM_GSYM(lmmp_add_nc_):
    cmp     x4, #1
    b       Ladd_ent

.globl ASM_GSYM(lmmp_add_n_)
.p2align 2

ASM_GSYM(lmmp_add_n_):
    cmn     xzr, xzr

Ladd_ent:
    lsr     x17, x3, #2
    tbz     x3, #0, Ladd_bx0

Ladd_bx1:
    ldr     x7, [x1]
    ldr     x11, [x2]
    adcs    x13, x7, x11
    str     x13, [x0], #8
    tbnz    x3, #1, Ladd_b11

Ladd_b01:
    cbz     x17, Ladd_ret
    ldp     x4, x5, [x1, #8]
    ldp     x8, x9, [x2, #8]
    sub     x1, x1, #8
    sub     x2, x2, #8
    b       Ladd_mid

Ladd_b11:
    ldp     x6, x7, [x1, #8]
    ldp     x10, x11, [x2, #8]
    add     x1, x1, #8
    add     x2, x2, #8
    cbz     x17, Ladd_end
    b       Ladd_top

Ladd_bx0:
    tbnz    x3, #1, Ladd_b10

Ladd_b00:
    ldp     x4, x5, [x1]
    ldp     x8, x9, [x2]
    sub     x1, x1, #16
    sub     x2, x2, #16
    b       Ladd_mid

Ladd_b10:
    ldp     x6, x7, [x1]
    ldp     x10, x11, [x2]
    cbz     x17, Ladd_end
.p2align 4

Ladd_top:
    ldp     x4, x5, [x1, #16]
    ldp     x8, x9, [x2, #16]
    adcs    x12, x6, x10
    adcs    x13, x7, x11
    stp     x12, x13, [x0], #16

Ladd_mid:
    ldp     x6, x7, [x1, #32]!
    ldp     x10, x11, [x2, #32]!
    adcs    x12, x4, x8
    adcs    x13, x5, x9
    stp     x12, x13, [x0], #16
    sub     x17, x17, #1
    cbnz    x17, Ladd_top

Ladd_end:
    adcs    x12, x6, x10
    adcs    x13, x7, x11
    stp     x12, x13, [x0]

Ladd_ret:
    cset    x0, cs
    ret
