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

# mp_limb_t lmmp_shlnot_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl);

#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_shlnot_)
.p2align 2

ASM_GSYM(lmmp_shlnot_):
cbz     x3, Lsln_copy0
neg     x5, x3
ldr     x6, [x1], #8
lsl     x7, x6, x3
mvn     x7, x7
str     x7, [x0], #8
subs    x8, x2, #1
b.eq    Lsln_done_last
lsr     x9, x8, #2
and     x10, x8, #3
cbz     x9, Lsln_tail
.p2align 4

Lsln_loop:
ldp     x11, x12, [x1], #16
ldp     x13, x14, [x1], #16
lsl     x15, x11, x3
lsr     x16, x6, x5
eon     x15, x16, x15
lsl     x16, x12, x3
lsr     x17, x11, x5
eon     x16, x17, x16
stp     x15, x16, [x0], #16
lsl     x15, x13, x3
lsr     x16, x12, x5
eon     x15, x16, x15
lsl     x16, x14, x3
lsr     x17, x13, x5
eon     x16, x17, x16
stp     x15, x16, [x0], #16
mov     x6, x14
sub     x9, x9, #1
cbnz    x9, Lsln_loop

Lsln_tail:
cbz     x10, Lsln_done

Lsln_tail_loop:
ldr     x11, [x1], #8
lsl     x12, x11, x3
lsr     x13, x6, x5
eon     x12, x13, x12
str     x12, [x0], #8
mov     x6, x11
sub     x10, x10, #1
cbnz    x10, Lsln_tail_loop

Lsln_done:

Lsln_done_last:
lsr     x0, x6, x5
ret

Lsln_copy0:
mov     x6, x2
lsr     x7, x2, #2
cbz     x7, Lsln_copy0_tail
.p2align 4

Lsln_copy0_loop:
ldp     x8, x9, [x1], #16
ldp     x10, x11, [x1], #16
mvn     x8, x8
mvn     x9, x9
mvn     x10, x10
mvn     x11, x11
stp     x8, x9, [x0], #16
stp     x10, x11, [x0], #16
sub     x7, x7, #1
cbnz    x7, Lsln_copy0_loop

Lsln_copy0_tail:
ands    x6, x6, #3
b.eq    Lsln_copy0_done

Lsln_copy0_tail_loop:
ldr     x8, [x1], #8
mvn     x8, x8
str     x8, [x0], #8
sub     x6, x6, #1
cbnz    x6, Lsln_copy0_tail_loop

Lsln_copy0_done:
mov     x0, xzr
ret
