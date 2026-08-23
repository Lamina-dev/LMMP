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

# void lmmp_not_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_not_)
.p2align 2

ASM_GSYM(lmmp_not_):
    lsr     x17, x2, #3
    cbz     x17, Lnot_tail
.p2align 4

Lnot_loop:
    ld1     {v0.16b, v1.16b, v2.16b, v3.16b}, [x1], #64
    mvn     v0.16b, v0.16b
    mvn     v1.16b, v1.16b
    mvn     v2.16b, v2.16b
    mvn     v3.16b, v3.16b
    st1     {v0.16b, v1.16b, v2.16b, v3.16b}, [x0], #64
    sub     x17, x17, #1
    cbnz    x17, Lnot_loop

Lnot_tail:
    ands    x17, x2, #7
    b.eq    Lnot_done

Lnot_tail_loop:
    ldr     x4, [x1], #8
    mvn     x4, x4
    str     x4, [x0], #8
    sub     x17, x17, #1
    cbnz    x17, Lnot_tail_loop

Lnot_done:
    ret
