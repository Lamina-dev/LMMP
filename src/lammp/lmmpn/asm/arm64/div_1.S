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

# mp_limb_t lmmp_div_1_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_limb_t x);
# mp_limb_t lmmp_div_1_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_limb_t x);

#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_div_1_)
.p2align 2

ASM_GSYM(lmmp_div_1_):
cbz     x0, Ldiv1_to_mod
mov     x4, x3
mov     x3, x2
mov     x2, x1
mov     x1, #0
cbz     x3, Ldiv_fz
stp     x29, x30, [sp, #-80]!
mov     x29, sp
stp     x19, x20, [sp, #16]
stp     x21, x22, [sp, #32]
stp     x23, x24, [sp, #48]
sub     x21, x3, #1
add     x7, x21, x1
add     x20, x2, x21, lsl #3
add     x19, x0, x7, lsl #3
mov     x24, x1
mov     x22, x4
tbnz    x4, #63, Ldiv_normalised

Ldiv_unnorm:
clz     x23, x22
lsl     x0, x22, x23
bl      ASM_GSYM(lmmp_inv_1_)

Ldiv_uentry:
lsl     x22, x22, x23
ldr     x7, [x20], #-8
sub     x8, xzr, x23
lsr     x11, x7, x8
lsl     x1, x7, x23
cbz     x21, Ldiv_uend

Ldiv_utop:
ldr     x7, [x20], #-8
add     x2, x11, #1
mul     x10, x11, x0
umulh   x17, x11, x0
lsr     x9, x7, x8
orr     x1, x1, x9
adds    x10, x1, x10
adc     x2, x2, x17
msub    x11, x22, x2, x1
lsl     x1, x7, x23
cmp     x10, x11
add     x14, x11, x22
csel    x11, x14, x11, cc
sbc     x2, x2, xzr
cmp     x11, x22
bcs     Ldiv_ufx

Ldiv_uok:
str     x2, [x19], #-8
sub     x21, x21, #1
cbnz    x21, Ldiv_utop

Ldiv_uend:
add     x2, x11, #1
mul     x10, x11, x0
umulh   x17, x11, x0
adds    x10, x1, x10
adc     x2, x2, x17
msub    x11, x22, x2, x1
cmp     x10, x11
add     x14, x11, x22
csel    x11, x14, x11, cc
sbc     x2, x2, xzr
subs    x14, x11, x22
adc     x2, x2, xzr
csel    x11, x14, x11, cs
str     x2, [x19], #-8
cbnz    x24, Ldiv_ftop
lsr     x0, x11, x23
ldp     x19, x20, [sp, #16]
ldp     x21, x22, [sp, #32]
ldp     x23, x24, [sp, #48]
ldp     x29, x30, [sp], #80
ret

Ldiv_ufx:
add     x2, x2, #1
sub     x11, x11, x22
b       Ldiv_uok

Ldiv_normalised:
mov     x0, x22
bl      ASM_GSYM(lmmp_inv_1_)

Ldiv_nentry:
ldr     x7, [x20], #-8
subs    x14, x7, x22
adc     x2, xzr, xzr
csel    x11, x14, x7, cs
b       Ldiv_nok

Ldiv_ntop:
ldr     x1, [x20], #-8
add     x2, x11, #1
mul     x10, x11, x0
umulh   x17, x11, x0
adds    x10, x1, x10
adc     x2, x2, x17
msub    x11, x22, x2, x1
cmp     x10, x11
add     x14, x11, x22
csel    x11, x14, x11, cc
sbc     x2, x2, xzr
cmp     x11, x22
bcs     Ldiv_nfx

Ldiv_nok:
str     x2, [x19], #-8
sub     x21, x21, #1
tbz     x21, #63, Ldiv_ntop

Ldiv_nend:
cbnz    x24, Ldiv_frac
mov     x0, x11
ldp     x19, x20, [sp, #16]
ldp     x21, x22, [sp, #32]
ldp     x23, x24, [sp, #48]
ldp     x29, x30, [sp], #80
ret

Ldiv_nfx:
add     x2, x2, #1
sub     x11, x11, x22
b       Ldiv_nok

Ldiv_frac:
mov     x23, #0

Ldiv_ftop:
add     x2, x11, #1
mul     x10, x11, x0
umulh   x17, x11, x0
add     x2, x2, x17
msub    x11, x22, x2, xzr
cmp     x10, x11
add     x14, x11, x22
csel    x11, x14, x11, cc
sbc     x2, x2, xzr
str     x2, [x19], #-8
sub     x24, x24, #1
cbnz    x24, Ldiv_ftop
lsr     x0, x11, x23
ldp     x19, x20, [sp, #16]
ldp     x21, x22, [sp, #32]
ldp     x23, x24, [sp, #48]
ldp     x29, x30, [sp], #80
ret

Ldiv_fz:
cbz     x1, Ldiv_zend

Ldiv_ztop:
str     xzr, [x0], #8
sub     x1, x1, #1
cbnz    x1, Ldiv_ztop

Ldiv_zend:
mov     x0, #0
ret

Ldiv1_to_mod:
mov     x0, x1
mov     x1, x2
mov     x2, x3
b       ASM_GSYM(lmmp_mod_1_)

.globl ASM_GSYM(lmmp_div_1_s_)
.p2align 2

ASM_GSYM(lmmp_div_1_s_):
stp     x29, x30, [sp, #-80]!
stp     x19, x20, [sp, #16]
stp     x21, x22, [sp, #32]
stp     x23, x24, [sp, #48]
mov     x22, x3
mov     x21, x2
mov     x20, x1
add     x20, x20, x21, lsl #3
add     x19, x0, x21, lsl #3
sub     x19, x19, #8
mov     x0, x22
bl      ASM_GSYM(lmmp_inv_1_)
mov     x24, x0
ldr     x7, [x20, #-8]!
subs    x14, x7, x22
cset    x23, cs
csel    x11, x14, x7, cs
sub     x21, x21, #1
.p2align 4

Ldivs_top:
ldr     x1, [x20, #-8]!
add     x2, x11, #1
mul     x10, x11, x24
umulh   x17, x11, x24
adds    x10, x1, x10
adc     x2, x2, x17
msub    x11, x22, x2, x1
cmp     x10, x11
add     x14, x11, x22
csel    x11, x14, x11, cc
sbc     x2, x2, xzr
cmp     x11, x22
bcs     Ldivs_fx

Ldivs_ok:
str     x2, [x19, #-8]!
sub     x21, x21, #1
cbnz    x21, Ldivs_top
str     x11, [x20]
mov     x0, x23
ldp     x19, x20, [sp, #16]
ldp     x21, x22, [sp, #32]
ldp     x23, x24, [sp, #48]
ldp     x29, x30, [sp], #80
ret

Ldivs_fx:
add     x2, x2, #1
sub     x11, x11, x22
b       Ldivs_ok
