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

# mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);

#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_addmul_1_)
.p2align 2

ASM_GSYM(lmmp_addmul_1_):
adds    x15, xzr, xzr
tbz     x2, #0, Lam1_1
ldr     x4, [x1], #8
mul     x8, x4, x3
umulh   x12, x4, x3
ldr     x4, [x0]
adds    x8, x4, x8
cinc    x15, x12, cs
str     x8, [x0], #8

Lam1_1:
tbz     x2, #1, Lam1_2
ldp     x4, x5, [x1], #16
mul     x8, x4, x3
umulh   x12, x4, x3
mul     x9, x5, x3
umulh   x13, x5, x3
adds    x8, x8, x15
adcs    x9, x9, x12
ldp     x4, x5, [x0]
adc     x15, x13, xzr
adds    x8, x4, x8
adcs    x9, x5, x9
cinc    x15, x15, cs
stp     x8, x9, [x0], #16

Lam1_2:
lsr     x2, x2, #2
cbz     x2, Lam1_le3
ldp     x4, x5, [x1], #32
ldp     x6, x7, [x1, #-16]
b       Lam1_mid

Lam1_le3:
mov     x0, x15
ret
.p2align 4

Lam1_top:
ldp     x4, x5, [x1], #32
ldp     x6, x7, [x1, #-16]
adds    x8, x16, x8
adcs    x9, x17, x9
stp     x8, x9, [x0], #32
adcs    x10, x12, x10
adcs    x11, x13, x11
stp     x10, x11, [x0, #-16]
cinc    x15, x15, cs

Lam1_mid:
sub     x2, x2, #1
mul     x8, x4, x3
umulh   x12, x4, x3
mul     x9, x5, x3
umulh   x13, x5, x3
adds    x8, x8, x15
mul     x10, x6, x3
umulh   x14, x6, x3
adcs    x9, x9, x12
mul     x11, x7, x3
umulh   x15, x7, x3
adcs    x10, x10, x13
ldp     x16, x17, [x0]
adcs    x11, x11, x14
ldp     x12, x13, [x0, #16]
adc     x15, x15, xzr
cbnz    x2, Lam1_top
adds    x8, x16, x8
adcs    x9, x17, x9
adcs    x10, x12, x10
adcs    x11, x13, x11
stp     x8, x9, [x0]
stp     x10, x11, [x0, #16]
cinc    x0, x15, cs
ret
