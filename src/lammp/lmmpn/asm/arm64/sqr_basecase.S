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

# void lmmp_sqr_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_sqr_basecase_)
.p2align 2

ASM_GSYM(lmmp_sqr_basecase_):
cmp     x2, #3
b.ls    Lsqr_le3
ldr     x3, [x1],#8
sub     x2, x2, #1
mul     x6, x3, x3
umulh   x4, x3, x3
str     x6, [x0],#8
lsl     x3, x3, 1
lsl     x2, x2, #3
lsr     x16, x2, #5
tbnz    x2, #3, Lsqr_mbx1

Lsqr_mbx0:
adds    x11, x4, xzr
tbz     x2, #4, Lsqr_mb00

Lsqr_mb10:
ldp     x4, x5, [x1],#16
mul     x8, x4, x3
umulh   x10, x4, x3
cbz     x16, Lsqr_m2e
ldp     x6, x7, [x1],#16
mul     x9, x5, x3
b       Lsqr_mmid-8

Lsqr_mbx1:
ldr     x7, [x1],#8
mul     x9, x7, x3
umulh   x11, x7, x3
adds    x9, x9, x4
str     x9, [x0],#8
tbnz    x2, #4, Lsqr_mb10

Lsqr_mb00:
ldp     x6, x7, [x1],#16
mul     x8, x6, x3
umulh   x10, x6, x3
ldp     x4, x5, [x1],#16
mul     x9, x7, x3
adcs    x12, x8, x11
umulh   x11, x7, x3
sub     x16, x16, #1
cbz     x16, Lsqr_mend
.p2align 4

Lsqr_mtop:
mul     x8, x4, x3
ldp     x6, x7, [x1],#16
adcs    x13, x9, x10
umulh   x10, x4, x3
mul     x9, x5, x3
stp     x12, x13, [x0],#16
adcs    x12, x8, x11
umulh   x11, x5, x3

Lsqr_mmid:
mul     x8, x6, x3
ldp     x4, x5, [x1],#16
adcs    x13, x9, x10
umulh   x10, x6, x3
mul     x9, x7, x3
stp     x12, x13, [x0],#16
adcs    x12, x8, x11
umulh   x11, x7, x3
sub     x16, x16, #1
cbnz    x16, Lsqr_mtop

Lsqr_mend:
mul     x8, x4, x3
adcs    x13, x9, x10
umulh   x10, x4, x3
stp     x12, x13, [x0],#16

Lsqr_m2e:
mul     x9, x5, x3
adcs    x12, x8, x11
umulh   x11, x5, x3
adcs    x13, x9, x10
stp     x12, x13, [x0],#16
adc     x11, x11, xzr
str     x11, [x0],#8

Lsqr_outer:
sub     x2, x2, #8
sub     x0, x0, x2
sub     x1, x1, x2
ldp     x6, x7, [x1,#-16]
ldr     x3, [x0,#-8]
and     x8, x7, x6, asr 63
mul     x9, x7, x7
adds    x3, x3, x8
umulh   x4, x7, x7
adc     x4, x4, xzr
adds    x3, x3, x9
str     x3, [x0,#-8]
adc     x17, x4, xzr
adds    xzr, x6, x6
adc     x3, x7, x7
cmp     x2, #16
beq     Lsqr_cor2
lsr     x16, x2, #5
tbz     x2, #3, Lsqr_bx0

Lsqr_bx1:
ldr     x4, [x1],#8
mul     x8, x4, x3
umulh   x4, x4, x3
tbz     x2, #4, Lsqr_b01

Lsqr_b11:
ldp     x5, x6, [x1],#16
ldp     x12, x13, [x0]
ldr     x14, [x0,#16]
mul     x9, x5, x3
umulh   x5, x5, x3
mul     x10, x6, x3
umulh   x6, x6, x3
adds    x8, x12, x8
adcs    x4, x13, x4
adcs    x5, x14, x5
adc     x6, x6, xzr
adds    x8, x8, x17
adcs    x4, x4, x9
adcs    x5, x5, x10
adc     x17, x6, xzr
stp     x8, x4, [x0],#16
str     x5, [x0],#8
cbnz    x16, Lsqr_top
b       Lsqr_end

Lsqr_b01:
ldr     x12, [x0]
adds    x8, x12, x8
adc     x4, x4, xzr
adds    x8, x8, x17
adc     x17, x4, xzr
str     x8, [x0],#8
b       Lsqr_top

Lsqr_bx0:
ldp     x4, x5, [x1],#16
tbz     x2, #4, Lsqr_top+4

Lsqr_b10:
ldp     x12, x13, [x0]
mul     x8, x4, x3
umulh   x4, x4, x3
mul     x9, x5, x3
umulh   x5, x5, x3
adds    x8, x12, x8
adcs    x4, x13, x4
adc     x5, x5, xzr
adds    x8, x8, x17
adcs    x4, x4, x9
adc     x17, x5, xzr
stp     x8, x4, [x0],#16
.p2align 4

Lsqr_top:
ldp     x4, x5, [x1],#16
ldp     x6, x7, [x1],#16
ldp     x12, x13, [x0]
ldp     x14, x15, [x0,#16]
mul     x8, x4, x3
umulh   x4, x4, x3
mul     x9, x5, x3
umulh   x5, x5, x3
mul     x10, x6, x3
umulh   x6, x6, x3
mul     x11, x7, x3
umulh   x7, x7, x3
adds    x8, x12, x8
adcs    x4, x13, x4
adcs    x5, x14, x5
adcs    x6, x15, x6
adc     x7, x7, xzr
adds    x8, x8, x17
adcs    x4, x4, x9
adcs    x5, x5, x10
adcs    x6, x6, x11
adc     x17, x7, xzr
stp     x8, x4, [x0],#16
stp     x5, x6, [x0],#16
sub     x16, x16, #1
cbnz    x16, Lsqr_top

Lsqr_end:
str     x17, [x0],#8
b       Lsqr_outer

Lsqr_cor2:
ldp     x10, x11, [x1]
ldp     x12, x13, [x0]
mul     x8, x10, x3
umulh   x4, x10, x3
mul     x9, x11, x3
umulh   x5, x11, x3
adds    x8, x12, x8
adcs    x4, x13, x4
adc     x5, x5, xzr
adds    x8, x8, x17
adcs    x13, x4, x9
adc     x12, x5, xzr
str     x8, [x0]
and     x8, x10, x7, asr 63
mul     x9, x10, x10
adds    x13, x13, x8
umulh   x4, x10, x10
adc     x4, x4, xzr
adds    x13, x13, x9
adc     x17, x4, xzr
adds    xzr, x7, x7
adc     x3, x10, x10
mul     x8, x11, x3
umulh   x4, x11, x3
adds    x8, x12, x8
adc     x4, x4, xzr
adds    x8, x8, x17
adc     x3, x4, xzr
stp     x13, x8, [x0,#8]
and     x2, x11, x10, asr 63
mul     x5, x11, x11
adds    x3, x3, x2
umulh   x4, x11, x11
adc     x4, x4, xzr
adds    x3, x3, x5
adc     x4, x4, xzr
stp     x3, x4, [x0,#24]
ret

Lsqr_le3:
ldr     x3, [x1]
mul     x4, x3, x3
umulh   x5, x3, x3
cmp     x2, #2
b.hs    Lsqr_2o3
stp     x4, x5, [x0]
ret

Lsqr_2o3:
ldr     x6, [x1,#8]
mul     x7, x6, x6
umulh   x8, x6, x6
mul     x9, x3, x6
umulh   x10, x3, x6
b.hi    Lsqr_3
adds    x5, x5, x9
adcs    x7, x7, x10
adc     x8, x8, xzr
adds    x5, x5, x9
adcs    x7, x7, x10
adc     x8, x8, xzr
stp     x4, x5, [x0]
stp     x7, x8, [x0,#16]
ret

Lsqr_3:
ldr     x11, [x1,#16]
mul     x12, x11, x11
umulh   x13, x11, x11
mul     x14, x3, x11
umulh   x15, x3, x11
mul     x16, x6, x11
umulh   x17, x6, x11
adds    x5, x5, x9
adcs    x7, x7, x10
adcs    x8, x8, x15
adcs    x12, x12, x17
adc     x13, x13, xzr
adds    x5, x5, x9
adcs    x7, x7, x10
adcs    x8, x8, x15
adcs    x12, x12, x17
adc     x13, x13, xzr
adds    x7, x7, x14
adcs    x8, x8, x16
adcs    x12, x12, xzr
adc     x13, x13, xzr
adds    x7, x7, x14
adcs    x8, x8, x16
adcs    x12, x12, xzr
adc     x13, x13, xzr
stp     x4, x5, [x0]
stp     x7, x8, [x0,#16]
stp     x12, x13, [x0,#32]
ret
