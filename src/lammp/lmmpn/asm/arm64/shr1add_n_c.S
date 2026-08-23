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

# mp_limb_t lmmp_shr1add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
# mp_limb_t lmmp_shr1add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_shr1add_nc_)
.p2align 2

ASM_GSYM(lmmp_shr1add_nc_):
cmp     x4, #1
b       Lrsha_ent

.globl ASM_GSYM(lmmp_shr1add_n_)
.p2align 2

ASM_GSYM(lmmp_shr1add_n_):
cmn     xzr, xzr

Lrsha_ent:
lsr     x6, x3, #2
tbz     x3, #0, Lrsha_bx0

Lrsha_bx1:
ldr     x5, [x1], #8
ldr     x9, [x2], #8
tbnz    x3, #1, Lrsha_b11

Lrsha_b01:
adcs    x13, x5, x9
and     x10, x13, #1
cbz     x6, Lrsha_1
ldp     x4, x5, [x1], #48
ldp     x8, x9, [x2], #48
adcs    x14, x4, x8
adcs    x15, x5, x9
ldp     x4, x5, [x1, #-32]
ldp     x8, x9, [x2, #-32]
extr    x17, x14, x13, #1
adcs    x12, x4, x8
adcs    x13, x5, x9
str     x17, [x0], #24
sub     x6, x6, #1
cbz     x6, Lrsha_end
b       Lrsha_top

Lrsha_1:
cset    x14, cs
extr    x17, x14, x13, #1
str     x17, [x0]
mov     x0, x10
ret

Lrsha_b11:
adcs    x15, x5, x9
and     x10, x15, #1
ldp     x4, x5, [x1], #32
ldp     x8, x9, [x2], #32
adcs    x12, x4, x8
adcs    x13, x5, x9
cbz     x6, Lrsha_3
ldp     x4, x5, [x1, #-16]
ldp     x8, x9, [x2, #-16]
extr    x17, x12, x15, #1
adcs    x14, x4, x8
adcs    x15, x5, x9
str     x17, [x0], #8
b       Lrsha_mid

Lrsha_3:
extr    x17, x12, x15, #1
str     x17, [x0], #8
b       Lrsha_2

Lrsha_bx0:
tbz     x3, #1, Lrsha_b00

Lrsha_b10:
ldp     x4, x5, [x1], #32
ldp     x8, x9, [x2], #32
adcs    x12, x4, x8
adcs    x13, x5, x9
and     x10, x12, #1
cbz     x6, Lrsha_2
ldp     x4, x5, [x1, #-16]
ldp     x8, x9, [x2, #-16]
adcs    x14, x4, x8
adcs    x15, x5, x9
b       Lrsha_mid

Lrsha_b00:
ldp     x4, x5, [x1], #48
ldp     x8, x9, [x2], #48
adcs    x14, x4, x8
adcs    x15, x5, x9
and     x10, x14, #1
ldp     x4, x5, [x1, #-32]
ldp     x8, x9, [x2, #-32]
adcs    x12, x4, x8
adcs    x13, x5, x9
add     x0, x0, #16
sub     x6, x6, #1
cbz     x6, Lrsha_end
.p2align 4

Lrsha_top:
ldp     x4, x5, [x1, #-16]
ldp     x8, x9, [x2, #-16]
extr    x16, x15, x14, #1
extr    x17, x12, x15, #1
adcs    x14, x4, x8
adcs    x15, x5, x9
stp     x16, x17, [x0, #-16]

Lrsha_mid:
ldp     x4, x5, [x1], #32
ldp     x8, x9, [x2], #32
extr    x16, x13, x12, #1
extr    x17, x14, x13, #1
adcs    x12, x4, x8
adcs    x13, x5, x9
stp     x16, x17, [x0], #32
sub     x6, x6, #1
cbnz    x6, Lrsha_top

Lrsha_end:
extr    x16, x15, x14, #1
extr    x17, x12, x15, #1
stp     x16, x17, [x0, #-16]

Lrsha_2:
cset    x14, cs
extr    x16, x13, x12, #1
extr    x17, x14, x13, #1
stp     x16, x17, [x0]

Lrsha_ret:
mov     x0, x10
ret
