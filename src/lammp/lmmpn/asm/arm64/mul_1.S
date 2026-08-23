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

# mp_limb_t lmmp_mul_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x);

#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_mul_1_)
.p2align 2

ASM_GSYM(lmmp_mul_1_):
adds    x4, xzr, xzr

Lm1_com:
lsr     x17, x2, #2
tbnz    x2, #0, Lm1_bx1

Lm1_bx0:
mov     x11, x4
tbz     x2, #1, Lm1_b00

Lm1_b10:
ldp     x4, x5, [x1]
mul     x8, x4, x3
umulh   x10, x4, x3
cbz     x17, Lm1_2
ldp     x6, x7, [x1, #16]!
mul     x9, x5, x3
b       Lm1_mid-8

Lm1_2:
mul     x9, x5, x3
b       Lm1_2e

Lm1_bx1:
ldr     x7, [x1], #8
mul     x9, x7, x3
umulh   x11, x7, x3
adds    x9, x9, x4
str     x9, [x0], #8
tbnz    x2, #1, Lm1_b10

Lm1_b01:
cbz     x17, Lm1_1

Lm1_b00:
ldp     x6, x7, [x1]
mul     x8, x6, x3
umulh   x10, x6, x3
ldp     x4, x5, [x1, #16]
mul     x9, x7, x3
adcs    x12, x8, x11
umulh   x11, x7, x3
add     x0, x0, #16
sub     x17, x17, #1
cbz     x17, Lm1_end
.p2align 4

Lm1_top:
mul     x8, x4, x3
ldp     x6, x7, [x1, #32]!
adcs    x13, x9, x10
umulh   x10, x4, x3
mul     x9, x5, x3
stp     x12, x13, [x0, #-16]
adcs    x12, x8, x11
umulh   x11, x5, x3

Lm1_mid:
mul     x8, x6, x3
ldp     x4, x5, [x1, #16]
adcs    x13, x9, x10
umulh   x10, x6, x3
mul     x9, x7, x3
stp     x12, x13, [x0], #32
adcs    x12, x8, x11
umulh   x11, x7, x3
sub     x17, x17, #1
cbnz    x17, Lm1_top

Lm1_end:
mul     x8, x4, x3
adcs    x13, x9, x10
umulh   x10, x4, x3
mul     x9, x5, x3
stp     x12, x13, [x0, #-16]

Lm1_2e:
adcs    x12, x8, x11
umulh   x11, x5, x3
adcs    x13, x9, x10
stp     x12, x13, [x0]

Lm1_1:
adc     x0, x11, xzr
ret
