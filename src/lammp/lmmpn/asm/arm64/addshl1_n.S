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

# mp_limb_t lmmp_addshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_addshl1_n_)
.p2align 2

ASM_GSYM(lmmp_addshl1_n_):
lsr     x6, x3, #2
tbz     x3, #0, Lashl_bx0

Lashl_bx1:
ldr     x5, [x1]
tbnz    x3, #1, Lashl_b11

Lashl_b01:
ldr     x11, [x2]
cbz     x6, Lashl_1
ldp     x8, x9, [x2, #8]
lsl     x13, x11, #1
adds    x15, x13, x5
str     x15, [x0], #8
sub     x1, x1, #24
sub     x2, x2, #8
b       Lashl_mid

Lashl_1:
lsl     x13, x11, #1
adds    x15, x13, x5
str     x15, [x0]
lsr     x0, x11, #63
adc     x0, x0, xzr
ret

Lashl_b11:
ldr     x9, [x2]
ldp     x10, x11, [x2, #8]!
lsl     x13, x9, #1
adds    x17, x13, x5
str     x17, [x0], #8
sub     x1, x1, #8
cbz     x6, Lashl_end
b       Lashl_top

Lashl_bx0:
tbnz    x3, #1, Lashl_b10

Lashl_b00:
adds    x11, xzr, xzr
ldp     x8, x9, [x2], #-16
sub     x1, x1, #32
b       Lashl_mid

Lashl_b10:
adds    x9, xzr, xzr
ldp     x10, x11, [x2]
sub     x1, x1, #16
cbz     x6, Lashl_end
.p2align 4

Lashl_top:
ldp     x4, x5, [x1, #16]
extr    x12, x10, x9, #63
ldp     x8, x9, [x2, #16]
extr    x13, x11, x10, #63
adcs    x14, x12, x4
adcs    x15, x13, x5
stp     x14, x15, [x0], #16

Lashl_mid:
ldp     x4, x5, [x1, #32]!
extr    x12, x8, x11, #63
ldp     x10, x11, [x2, #32]!
extr    x13, x9, x8, #63
adcs    x16, x12, x4
adcs    x17, x13, x5
stp     x16, x17, [x0], #16
sub     x6, x6, #1
cbnz    x6, Lashl_top

Lashl_end:
ldp     x4, x5, [x1, #16]
extr    x12, x10, x9, #63
extr    x13, x11, x10, #63
adcs    x14, x12, x4
adcs    x15, x13, x5
stp     x14, x15, [x0]
lsr     x0, x11, #63
adc     x0, x0, xzr
ret
