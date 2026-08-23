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

# mp_limb_t lmmp_shr1sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
# mp_limb_t lmmp_shr1sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_shr1sub_nc_)
.p2align 2

ASM_GSYM(lmmp_shr1sub_nc_):
cmp     xzr, x4
b       Lrshs_ent

.globl ASM_GSYM(lmmp_shr1sub_n_)
.p2align 2

ASM_GSYM(lmmp_shr1sub_n_):
cmp     xzr, xzr

Lrshs_ent:
lsr     x6, x3, #2
tbz     x3, #0, Lrshs_bx0

Lrshs_bx1:
ldr     x5, [x1], #8
ldr     x9, [x2], #8
tbnz    x3, #1, Lrshs_b11

Lrshs_b01:
sbcs    x13, x5, x9
and     x10, x13, #1
cbz     x6, Lrshs_1
ldp     x4, x5, [x1], #48
ldp     x8, x9, [x2], #48
sbcs    x14, x4, x8
sbcs    x15, x5, x9
ldp     x4, x5, [x1, #-32]
ldp     x8, x9, [x2, #-32]
extr    x17, x14, x13, #1
sbcs    x12, x4, x8
sbcs    x13, x5, x9
str     x17, [x0], #24
sub     x6, x6, #1
cbz     x6, Lrshs_end
b       Lrshs_top

Lrshs_1:
cset    x14, cc
extr    x17, x14, x13, #1
str     x17, [x0]
mov     x0, x10
ret

Lrshs_b11:
sbcs    x15, x5, x9
and     x10, x15, #1
ldp     x4, x5, [x1], #32
ldp     x8, x9, [x2], #32
sbcs    x12, x4, x8
sbcs    x13, x5, x9
cbz     x6, Lrshs_3
ldp     x4, x5, [x1, #-16]
ldp     x8, x9, [x2, #-16]
extr    x17, x12, x15, #1
sbcs    x14, x4, x8
sbcs    x15, x5, x9
str     x17, [x0], #8
b       Lrshs_mid

Lrshs_3:
extr    x17, x12, x15, #1
str     x17, [x0], #8
b       Lrshs_2

Lrshs_bx0:
tbz     x3, #1, Lrshs_b00

Lrshs_b10:
ldp     x4, x5, [x1], #32
ldp     x8, x9, [x2], #32
sbcs    x12, x4, x8
sbcs    x13, x5, x9
and     x10, x12, #1
cbz     x6, Lrshs_2
ldp     x4, x5, [x1, #-16]
ldp     x8, x9, [x2, #-16]
sbcs    x14, x4, x8
sbcs    x15, x5, x9
b       Lrshs_mid

Lrshs_b00:
ldp     x4, x5, [x1], #48
ldp     x8, x9, [x2], #48
sbcs    x14, x4, x8
sbcs    x15, x5, x9
and     x10, x14, #1
ldp     x4, x5, [x1, #-32]
ldp     x8, x9, [x2, #-32]
sbcs    x12, x4, x8
sbcs    x13, x5, x9
add     x0, x0, #16
sub     x6, x6, #1
cbz     x6, Lrshs_end
.p2align 4

Lrshs_top:
ldp     x4, x5, [x1, #-16]
ldp     x8, x9, [x2, #-16]
extr    x16, x15, x14, #1
extr    x17, x12, x15, #1
sbcs    x14, x4, x8
sbcs    x15, x5, x9
stp     x16, x17, [x0, #-16]

Lrshs_mid:
ldp     x4, x5, [x1], #32
ldp     x8, x9, [x2], #32
extr    x16, x13, x12, #1
extr    x17, x14, x13, #1
sbcs    x12, x4, x8
sbcs    x13, x5, x9
stp     x16, x17, [x0], #32
sub     x6, x6, #1
cbnz    x6, Lrshs_top

Lrshs_end:
extr    x16, x15, x14, #1
extr    x17, x12, x15, #1
stp     x16, x17, [x0, #-16]

Lrshs_2:
cset    x14, cc
extr    x16, x13, x12, #1
extr    x17, x14, x13, #1
stp     x16, x17, [x0]

Lrshs_ret:
mov     x0, x10
ret
