//  Copyright (C) 2026 HJimmyK(Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it under
//  the terms of the GNU Lesser General Public License (LGPL) as published
//  by the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed WITHOUT ANY WARRANTY.
//
//  See <https://www.gnu.org/licenses/>.

// mp_limb_t lmmp_shr1add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
// mp_limb_t lmmp_shr1add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
//   x0=dst, x1=numa, x2=numb, x3=n, x4=c (only nc_)
//   ret x0 = t0 & 1
//   cal: t_i = numa[i] + numb[i] + C; dst[i-1] = (t_{i-1}>>1)|(t_i<<63)

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_shr1add_nc_)
.p2align 2

ASM_GSYM(lmmp_shr1add_nc_):
    cmp     x4, #1                  // C = (c != 0)
    b       Lrsha_ent

.globl ASM_GSYM(lmmp_shr1add_n_)
.p2align 2

ASM_GSYM(lmmp_shr1add_n_):
    cmn     xzr, xzr                // 0 + 0: C = 0

Lrsha_ent:
    lsr     x6, x3, #2              // x6 = n >> 2; 4 limb
    tbz     x3, #0, Lrsha_bx0       // n mod 2 == 0

Lrsha_bx1:                          // n mod 2 == 1
    ldr     x5, [x1], #8            // numa[0]
    ldr     x9, [x2], #8            // numb[0]
    tbnz    x3, #1, Lrsha_b11       // n mod 4 == 3

Lrsha_b01:                          // n mod 4 == 1
    adcs    x13, x5, x9             // t0 = numa[0] + numb[0] + C
    and     x10, x13, #1            // x10 = t0 & 1
    cbz     x6, Lrsha_1             // n == 1, finish
    ldp     x4, x5, [x1], #48
    ldp     x8, x9, [x2], #48
    adcs    x14, x4, x8             // t1
    adcs    x15, x5, x9             // t2
    ldp     x4, x5, [x1, #-32]
    ldp     x8, x9, [x2, #-32]
    extr    x17, x14, x13, #1       // dst[0] = (t0>>1)|(t1<<63)
    adcs    x12, x4, x8             // t3
    adcs    x13, x5, x9             // t4
    str     x17, [x0], #24          // store dst[0], align x0
    sub     x6, x6, #1
    cbz     x6, Lrsha_end
    b       Lrsha_top

Lrsha_1:                            // n == 1, finish
    cset    x14, cs                 // x14 = highest carry
    extr    x17, x14, x13, #1       // dst[0] = (t0>>1)|(carry<<63)
    str     x17, [x0]
    mov     x0, x10                 // ret t0 & 1
    ret

Lrsha_b11:                          // n mod 4 == 3
    adcs    x15, x5, x9             // t0 = numa[0] + numb[0] + C
    and     x10, x15, #1            // x10 = t0 & 1
    ldp     x4, x5, [x1], #32
    ldp     x8, x9, [x2], #32
    adcs    x12, x4, x8             // t1
    adcs    x13, x5, x9             // t2
    cbz     x6, Lrsha_3             // n == 3, finish
    ldp     x4, x5, [x1, #-16]
    ldp     x8, x9, [x2, #-16]
    extr    x17, x12, x15, #1       // dst[0] = (t0>>1)|(t1<<63)
    adcs    x14, x4, x8             // t3
    adcs    x15, x5, x9             // t4
    str     x17, [x0], #8
    b       Lrsha_mid

Lrsha_3:                            // n == 3, finish
    extr    x17, x12, x15, #1       // dst[0] = (t0>>1)|(t1<<63)
    str     x17, [x0], #8
    b       Lrsha_2

Lrsha_bx0:
    tbz     x3, #1, Lrsha_b00

Lrsha_b10:                          // n mod 4 == 2
    ldp     x4, x5, [x1], #32
    ldp     x8, x9, [x2], #32
    adcs    x12, x4, x8             // t0
    adcs    x13, x5, x9             // t1
    and     x10, x12, #1            // x10 = t0 & 1
    cbz     x6, Lrsha_2             // n == 2, finish
    ldp     x4, x5, [x1, #-16]
    ldp     x8, x9, [x2, #-16]
    adcs    x14, x4, x8             // t2
    adcs    x15, x5, x9             // t3
    b       Lrsha_mid

Lrsha_b00:                          // n mod 4 == 0
    ldp     x4, x5, [x1], #48
    ldp     x8, x9, [x2], #48
    adcs    x14, x4, x8             // t0
    adcs    x15, x5, x9             // t1
    and     x10, x14, #1            // x10 = t0 & 1
    ldp     x4, x5, [x1, #-32]
    ldp     x8, x9, [x2, #-32]
    adcs    x12, x4, x8             // t2
    adcs    x13, x5, x9             // t3
    add     x0, x0, #16             // align x0 for [x0,#-16]
    sub     x6, x6, #1
    cbz     x6, Lrsha_end
.p2align 4

Lrsha_top:                          // main loop (upper)
    ldp     x4, x5, [x1, #-16]
    ldp     x8, x9, [x2, #-16]
    extr    x16, x15, x14, #1       // dst = (t>>1)|(t_next<<63)
    extr    x17, x12, x15, #1
    adcs    x14, x4, x8             // new t pair
    adcs    x15, x5, x9
    stp     x16, x17, [x0, #-16]

Lrsha_mid:                          // main loop (lower)
    ldp     x4, x5, [x1], #32
    ldp     x8, x9, [x2], #32
    extr    x16, x13, x12, #1
    extr    x17, x14, x13, #1
    adcs    x12, x4, x8             // new t pair
    adcs    x13, x5, x9
    stp     x16, x17, [x0], #32
    sub     x6, x6, #1
    cbnz    x6, Lrsha_top

Lrsha_end:                          // shift out last pair
    extr    x16, x15, x14, #1
    extr    x17, x12, x15, #1
    stp     x16, x17, [x0, #-16]

Lrsha_2:                            // last t12/t13 pair
    cset    x14, cs                 // x14 = highest carry
    extr    x16, x13, x12, #1       // dst[n-2] = (t12>>1)|(t13<<63)
    extr    x17, x14, x13, #1       // dst[n-1] = (t13>>1)|(carry<<63)
    stp     x16, x17, [x0]

Lrsha_ret:
    mov     x0, x10                 // ret t0 & 1
    ret
