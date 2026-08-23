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

# mp_limb_t lmmp_sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
# mp_limb_t lmmp_sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_sub_nc_)
.p2align 2

ASM_GSYM(lmmp_sub_nc_):
    cmp     xzr, x4
    b       Lsub_ent

.globl ASM_GSYM(lmmp_sub_n_)
.p2align 2

ASM_GSYM(lmmp_sub_n_):
    cmp     xzr, xzr

Lsub_ent:
    lsr     x17, x3, #2
    tbz     x3, #0, Lsub_bx0

Lsub_bx1:
    ldr     x7, [x1]
    ldr     x11, [x2]
    sbcs    x13, x7, x11
    str     x13, [x0], #8
    tbnz    x3, #1, Lsub_b11

Lsub_b01:
    cbz     x17, Lsub_ret
    ldp     x4, x5, [x1, #8]
    ldp     x8, x9, [x2, #8]
    sub     x1, x1, #8
    sub     x2, x2, #8
    b       Lsub_mid

Lsub_b11:
    ldp     x6, x7, [x1, #8]
    ldp     x10, x11, [x2, #8]
    add     x1, x1, #8
    add     x2, x2, #8
    cbz     x17, Lsub_end
    b       Lsub_top

Lsub_bx0:
    tbnz    x3, #1, Lsub_b10

Lsub_b00:
    ldp     x4, x5, [x1]
    ldp     x8, x9, [x2]
    sub     x1, x1, #16
    sub     x2, x2, #16
    b       Lsub_mid

Lsub_b10:
    ldp     x6, x7, [x1]
    ldp     x10, x11, [x2]
    cbz     x17, Lsub_end
.p2align 4

Lsub_top:
    ldp     x4, x5, [x1, #16]
    ldp     x8, x9, [x2, #16]
    sbcs    x12, x6, x10
    sbcs    x13, x7, x11
    stp     x12, x13, [x0], #16

Lsub_mid:
    ldp     x6, x7, [x1, #32]!
    ldp     x10, x11, [x2, #32]!
    sbcs    x12, x4, x8
    sbcs    x13, x5, x9
    stp     x12, x13, [x0], #16
    sub     x17, x17, #1
    cbnz    x17, Lsub_top

Lsub_end:
    sbcs    x12, x6, x10
    sbcs    x13, x7, x11
    stp     x12, x13, [x0]

Lsub_ret:
    cset    x0, cc
    ret
