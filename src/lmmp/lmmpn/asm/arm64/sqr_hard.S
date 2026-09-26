//
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
//
//  本文件由 gen_hard.py 生成, 请勿手工修改; 重新生成: python gen_hard.py


//
// void lmmp_sqr_hard_N_(mp_ptr dst, mp_srcptr numa);
//
//   平衡硬编码平方: [dst,2N] = [numa,N]^2。
//   N <= 3 : 库内 sqr_basecase.S 小规模分支的特化
//   N >= 4 : 交叉乘行累加(双链) -> 倍增(连续 adcs 长链) -> 对角折叠(单加数链)。
//

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_sqr_hard_1_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_1_):

    ldr     x3, [x1]
    mul     x4, x3, x3
    umulh   x5, x3, x3
    stp     x4, x5, [x0]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_2_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_2_):

    ldr     x3, [x1]
    ldr     x6, [x1,#8]
    mul     x4, x3, x3
    umulh   x5, x3, x3
    mul     x7, x6, x6
    umulh   x8, x6, x6
    mul     x9, x3, x6
    umulh   x10, x3, x6
    adds    x5, x5, x9
    adcs    x7, x7, x10
    adc     x8, x8, xzr
    adds    x5, x5, x9
    adcs    x7, x7, x10
    adc     x8, x8, xzr
    stp     x4, x5, [x0]
    stp     x7, x8, [x0,#16]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_3_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_3_):

    ldr     x3, [x1]
    ldr     x6, [x1,#8]
    mul     x4, x3, x3
    umulh   x5, x3, x3
    mul     x7, x6, x6
    umulh   x8, x6, x6
    mul     x9, x3, x6
    umulh   x10, x3, x6
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

.globl ASM_GSYM(lmmp_sqr_hard_4_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_4_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    stp     x8, x9, [x0, #8]
    str     x10, [x0, #24]
    str     x15, [x0, #32]
    ldr     x7, [x1, #0]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    str     x15, [x0, #40]
    ldr     x7, [x1, #8]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #40]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #40]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x14, x12, xzr
    str     x14, [x0, #56]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_5_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_5_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    str     x15, [x0, #40]
    ldr     x7, [x1, #0]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #24]
    ldr     x12, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    str     x10, [x0, #40]
    str     x15, [x0, #48]
    ldr     x7, [x1, #8]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    str     x15, [x0, #56]
    ldr     x7, [x1, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #56]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #56]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x14, x12, xzr
    str     x14, [x0, #72]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_6_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_6_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldr     x4, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    str     x8, [x0, #40]
    str     x15, [x0, #48]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    str     x15, [x0, #56]
    ldr     x7, [x1, #8]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #40]
    ldr     x12, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    str     x10, [x0, #56]
    str     x15, [x0, #64]
    ldr     x7, [x1, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    str     x15, [x0, #72]
    ldr     x7, [x1, #24]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #72]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #72]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x14, x12, xzr
    str     x14, [x0, #88]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_7_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_7_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    stp     x8, x9, [x0, #40]
    str     x15, [x0, #56]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldr     x4, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #56]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #56]
    str     x15, [x0, #64]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    str     x15, [x0, #72]
    ldr     x7, [x1, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #56]
    ldr     x12, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    str     x10, [x0, #72]
    str     x15, [x0, #80]
    ldr     x7, [x1, #24]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    str     x15, [x0, #88]
    ldr     x7, [x1, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #88]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #88]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x14, x12, xzr
    str     x14, [x0, #104]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_8_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_8_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    stp     x8, x9, [x0, #40]
    str     x10, [x0, #56]
    str     x15, [x0, #64]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    str     x15, [x0, #72]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldr     x4, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #72]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #72]
    str     x15, [x0, #80]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    str     x15, [x0, #88]
    ldr     x7, [x1, #24]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #72]
    ldr     x12, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    str     x10, [x0, #88]
    str     x15, [x0, #96]
    ldr     x7, [x1, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    str     x15, [x0, #104]
    ldr     x7, [x1, #40]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #104]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #104]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x14, x12, xzr
    str     x14, [x0, #120]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_9_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_9_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    str     x15, [x0, #72]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #56]
    ldr     x12, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    str     x10, [x0, #72]
    str     x15, [x0, #80]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    str     x15, [x0, #88]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #88]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #88]
    str     x15, [x0, #96]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    str     x15, [x0, #104]
    ldr     x7, [x1, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #88]
    ldr     x12, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    str     x10, [x0, #104]
    str     x15, [x0, #112]
    ldr     x7, [x1, #40]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    str     x15, [x0, #120]
    ldr     x7, [x1, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #120]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #120]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x14, x12, xzr
    str     x14, [x0, #136]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_10_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_10_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldr     x4, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    str     x8, [x0, #72]
    str     x15, [x0, #80]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    str     x15, [x0, #88]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #72]
    ldr     x12, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    str     x10, [x0, #88]
    str     x15, [x0, #96]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    str     x15, [x0, #104]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldr     x4, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #104]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #104]
    str     x15, [x0, #112]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    str     x15, [x0, #120]
    ldr     x7, [x1, #40]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #104]
    ldr     x12, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    str     x10, [x0, #120]
    str     x15, [x0, #128]
    ldr     x7, [x1, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    str     x15, [x0, #136]
    ldr     x7, [x1, #56]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #136]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #136]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x14, x12, xzr
    str     x14, [x0, #152]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_11_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_11_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    stp     x8, x9, [x0, #72]
    str     x15, [x0, #88]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldr     x4, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #88]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #88]
    str     x15, [x0, #96]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    str     x15, [x0, #104]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #88]
    ldr     x12, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    str     x10, [x0, #104]
    str     x15, [x0, #112]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    str     x15, [x0, #120]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldr     x4, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #120]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #120]
    str     x15, [x0, #128]
    ldr     x7, [x1, #40]
    ldr     x4, [x1, #48]
    ldr     x16, [x0, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    str     x15, [x0, #136]
    ldr     x7, [x1, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #120]
    ldr     x12, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    str     x10, [x0, #136]
    str     x15, [x0, #144]
    ldr     x7, [x1, #56]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x15, [x0, #152]
    ldr     x7, [x1, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #152]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #152]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #160]
    adc     x14, x12, xzr
    str     x14, [x0, #168]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_12_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_12_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    stp     x8, x9, [x0, #72]
    str     x10, [x0, #88]
    str     x15, [x0, #96]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    str     x15, [x0, #104]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldr     x4, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #104]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #104]
    str     x15, [x0, #112]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    str     x15, [x0, #120]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #104]
    ldr     x12, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    str     x10, [x0, #120]
    str     x15, [x0, #128]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    str     x15, [x0, #136]
    ldr     x7, [x1, #40]
    ldr     x4, [x1, #48]
    ldr     x16, [x0, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldr     x4, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #136]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #136]
    str     x15, [x0, #144]
    ldr     x7, [x1, #48]
    ldr     x4, [x1, #56]
    ldr     x16, [x0, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    str     x15, [x0, #152]
    ldr     x7, [x1, #56]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #136]
    ldr     x12, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x10, [x0, #152]
    str     x15, [x0, #160]
    ldr     x7, [x1, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    str     x15, [x0, #168]
    ldr     x7, [x1, #72]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #160]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #168]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #168]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #176]
    adc     x14, x12, xzr
    str     x14, [x0, #184]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_13_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_13_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    str     x15, [x0, #104]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #88]
    ldr     x12, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    str     x10, [x0, #104]
    str     x15, [x0, #112]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    str     x15, [x0, #120]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #120]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #120]
    str     x15, [x0, #128]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    str     x15, [x0, #136]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #120]
    ldr     x12, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    str     x10, [x0, #136]
    str     x15, [x0, #144]
    ldr     x7, [x1, #40]
    ldr     x4, [x1, #48]
    ldr     x16, [x0, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x15, [x0, #152]
    ldr     x7, [x1, #48]
    ldr     x4, [x1, #56]
    ldr     x16, [x0, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #152]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #152]
    str     x15, [x0, #160]
    ldr     x7, [x1, #56]
    ldr     x4, [x1, #64]
    ldr     x16, [x0, #128]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    str     x15, [x0, #168]
    ldr     x7, [x1, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #152]
    ldr     x12, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    str     x10, [x0, #168]
    str     x15, [x0, #176]
    ldr     x7, [x1, #72]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #160]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    str     x15, [x0, #184]
    ldr     x7, [x1, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #176]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #184]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #184]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #192]
    adc     x14, x12, xzr
    str     x14, [x0, #200]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_14_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_14_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldr     x4, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    str     x8, [x0, #104]
    str     x15, [x0, #112]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    str     x15, [x0, #120]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #104]
    ldr     x12, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    str     x10, [x0, #120]
    str     x15, [x0, #128]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    str     x15, [x0, #136]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldr     x4, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #136]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #136]
    str     x15, [x0, #144]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    str     x15, [x0, #152]
    ldr     x7, [x1, #40]
    ldr     x4, [x1, #48]
    ldr     x16, [x0, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #136]
    ldr     x12, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x10, [x0, #152]
    str     x15, [x0, #160]
    ldr     x7, [x1, #48]
    ldr     x4, [x1, #56]
    ldr     x16, [x0, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    str     x15, [x0, #168]
    ldr     x7, [x1, #56]
    ldr     x4, [x1, #64]
    ldr     x16, [x0, #128]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldr     x4, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #168]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #168]
    str     x15, [x0, #176]
    ldr     x7, [x1, #64]
    ldr     x4, [x1, #72]
    ldr     x16, [x0, #144]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    str     x15, [x0, #184]
    ldr     x7, [x1, #72]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #160]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #168]
    ldr     x12, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    str     x10, [x0, #184]
    str     x15, [x0, #192]
    ldr     x7, [x1, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #176]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    str     x15, [x0, #200]
    ldr     x7, [x1, #88]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #192]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #200]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #200]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #208]
    adc     x14, x12, xzr
    str     x14, [x0, #216]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_15_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_15_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    stp     x8, x9, [x0, #104]
    str     x15, [x0, #120]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldr     x4, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #120]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #120]
    str     x15, [x0, #128]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    str     x15, [x0, #136]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #120]
    ldr     x12, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    str     x10, [x0, #136]
    str     x15, [x0, #144]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x15, [x0, #152]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldr     x4, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #152]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #152]
    str     x15, [x0, #160]
    ldr     x7, [x1, #40]
    ldr     x4, [x1, #48]
    ldr     x16, [x0, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    str     x15, [x0, #168]
    ldr     x7, [x1, #48]
    ldr     x4, [x1, #56]
    ldr     x16, [x0, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #152]
    ldr     x12, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    str     x10, [x0, #168]
    str     x15, [x0, #176]
    ldr     x7, [x1, #56]
    ldr     x4, [x1, #64]
    ldr     x16, [x0, #128]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    str     x15, [x0, #184]
    ldr     x7, [x1, #64]
    ldr     x4, [x1, #72]
    ldr     x16, [x0, #144]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldr     x4, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #184]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #184]
    str     x15, [x0, #192]
    ldr     x7, [x1, #72]
    ldr     x4, [x1, #80]
    ldr     x16, [x0, #160]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #160]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    str     x15, [x0, #200]
    ldr     x7, [x1, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #176]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #184]
    ldr     x12, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    str     x10, [x0, #200]
    str     x15, [x0, #208]
    ldr     x7, [x1, #88]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #192]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    str     x15, [x0, #216]
    ldr     x7, [x1, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #208]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #216]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #216]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #224]
    adc     x14, x12, xzr
    str     x14, [x0, #232]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_16_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_16_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    stp     x8, x9, [x0, #104]
    str     x10, [x0, #120]
    str     x15, [x0, #128]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    str     x15, [x0, #136]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldr     x4, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #136]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #136]
    str     x15, [x0, #144]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    str     x15, [x0, #152]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #136]
    ldr     x12, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x10, [x0, #152]
    str     x15, [x0, #160]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    str     x15, [x0, #168]
    ldr     x7, [x1, #40]
    ldr     x4, [x1, #48]
    ldr     x16, [x0, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldr     x4, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #168]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #168]
    str     x15, [x0, #176]
    ldr     x7, [x1, #48]
    ldr     x4, [x1, #56]
    ldr     x16, [x0, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    str     x15, [x0, #184]
    ldr     x7, [x1, #56]
    ldr     x4, [x1, #64]
    ldr     x16, [x0, #128]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #168]
    ldr     x12, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    str     x10, [x0, #184]
    str     x15, [x0, #192]
    ldr     x7, [x1, #64]
    ldr     x4, [x1, #72]
    ldr     x16, [x0, #144]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    str     x15, [x0, #200]
    ldr     x7, [x1, #72]
    ldr     x4, [x1, #80]
    ldr     x16, [x0, #160]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #160]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    ldr     x4, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #200]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #200]
    str     x15, [x0, #208]
    ldr     x7, [x1, #80]
    ldr     x4, [x1, #88]
    ldr     x16, [x0, #176]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #176]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
    str     x15, [x0, #216]
    ldr     x7, [x1, #88]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #192]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #200]
    ldr     x12, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    str     x10, [x0, #216]
    str     x15, [x0, #224]
    ldr     x7, [x1, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #208]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #216]
    str     x15, [x0, #232]
    ldr     x7, [x1, #104]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #224]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #232]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #232]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #240]
    adc     x14, x12, xzr
    str     x14, [x0, #248]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_17_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_17_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    str     x15, [x0, #136]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #120]
    ldr     x12, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    str     x10, [x0, #136]
    str     x15, [x0, #144]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x15, [x0, #152]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #152]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #152]
    str     x15, [x0, #160]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    str     x15, [x0, #168]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #152]
    ldr     x12, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    str     x10, [x0, #168]
    str     x15, [x0, #176]
    ldr     x7, [x1, #40]
    ldr     x4, [x1, #48]
    ldr     x16, [x0, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    str     x15, [x0, #184]
    ldr     x7, [x1, #48]
    ldr     x4, [x1, #56]
    ldr     x16, [x0, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #184]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #184]
    str     x15, [x0, #192]
    ldr     x7, [x1, #56]
    ldr     x4, [x1, #64]
    ldr     x16, [x0, #128]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    str     x15, [x0, #200]
    ldr     x7, [x1, #64]
    ldr     x4, [x1, #72]
    ldr     x16, [x0, #144]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #184]
    ldr     x12, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    str     x10, [x0, #200]
    str     x15, [x0, #208]
    ldr     x7, [x1, #72]
    ldr     x4, [x1, #80]
    ldr     x16, [x0, #160]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #160]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    str     x15, [x0, #216]
    ldr     x7, [x1, #80]
    ldr     x4, [x1, #88]
    ldr     x16, [x0, #176]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #176]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #216]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #216]
    str     x15, [x0, #224]
    ldr     x7, [x1, #88]
    ldr     x4, [x1, #96]
    ldr     x16, [x0, #192]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #192]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #200]
    ldp     x12, x13, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    stp     x10, x11, [x0, #216]
    str     x15, [x0, #232]
    ldr     x7, [x1, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #208]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #216]
    ldr     x12, [x0, #232]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #216]
    str     x10, [x0, #232]
    str     x15, [x0, #240]
    ldr     x7, [x1, #104]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #224]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #232]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #232]
    str     x15, [x0, #248]
    ldr     x7, [x1, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #240]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #248]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #248]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #256]
    adc     x14, x12, xzr
    str     x14, [x0, #264]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_18_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_18_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldr     x4, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    str     x8, [x0, #136]
    str     x15, [x0, #144]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    str     x15, [x0, #152]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #136]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #136]
    ldr     x12, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x10, [x0, #152]
    str     x15, [x0, #160]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    str     x15, [x0, #168]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldr     x4, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #168]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #168]
    str     x15, [x0, #176]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    str     x15, [x0, #184]
    ldr     x7, [x1, #40]
    ldr     x4, [x1, #48]
    ldr     x16, [x0, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #136]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #168]
    ldr     x12, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    str     x10, [x0, #184]
    str     x15, [x0, #192]
    ldr     x7, [x1, #48]
    ldr     x4, [x1, #56]
    ldr     x16, [x0, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    str     x15, [x0, #200]
    ldr     x7, [x1, #56]
    ldr     x4, [x1, #64]
    ldr     x16, [x0, #128]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    ldr     x4, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #200]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #200]
    str     x15, [x0, #208]
    ldr     x7, [x1, #64]
    ldr     x4, [x1, #72]
    ldr     x16, [x0, #144]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
    str     x15, [x0, #216]
    ldr     x7, [x1, #72]
    ldr     x4, [x1, #80]
    ldr     x16, [x0, #160]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #160]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #136]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #200]
    ldr     x12, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    str     x10, [x0, #216]
    str     x15, [x0, #224]
    ldr     x7, [x1, #80]
    ldr     x4, [x1, #88]
    ldr     x16, [x0, #176]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #176]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #216]
    str     x15, [x0, #232]
    ldr     x7, [x1, #88]
    ldr     x4, [x1, #96]
    ldr     x16, [x0, #192]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #192]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #200]
    ldp     x12, x13, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    stp     x10, x11, [x0, #216]
    ldr     x4, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #232]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #232]
    str     x15, [x0, #240]
    ldr     x7, [x1, #96]
    ldr     x4, [x1, #104]
    ldr     x16, [x0, #208]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #208]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #216]
    ldp     x12, x13, [x0, #232]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #216]
    stp     x10, x11, [x0, #232]
    str     x15, [x0, #248]
    ldr     x7, [x1, #104]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #224]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #136]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #232]
    ldr     x12, [x0, #248]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #232]
    str     x10, [x0, #248]
    str     x15, [x0, #256]
    ldr     x7, [x1, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #240]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #248]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #248]
    str     x15, [x0, #264]
    ldr     x7, [x1, #120]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #256]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #264]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #264]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #272]
    adc     x14, x12, xzr
    str     x14, [x0, #280]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_19_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_19_):

    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)
    ldr     x4, [x1]
    mul     x9, x4, x4
    umulh   x17, x4, x4
    str     x9, [x0]
    adds    x3, x4, x4
    ldp     x4, x5, [x1, #8]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #24]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x17
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    stp     x8, x9, [x0, #136]
    str     x15, [x0, #152]
    ldr     x7, [x1, #0]
    ldr     x4, [x1, #8]
    ldr     x16, [x0, #16]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #16]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #16]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #32]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #24]
    ldp     x12, x13, [x0, #40]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #24]
    stp     x10, x11, [x0, #40]
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldr     x4, [x1, #144]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #152]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #152]
    str     x15, [x0, #160]
    ldr     x7, [x1, #8]
    ldr     x4, [x1, #16]
    ldr     x16, [x0, #32]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #32]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #24]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #40]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #136]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    str     x15, [x0, #168]
    ldr     x7, [x1, #16]
    ldr     x4, [x1, #24]
    ldr     x16, [x0, #48]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #48]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #48]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #56]
    ldp     x12, x13, [x0, #72]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #56]
    stp     x10, x11, [x0, #72]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #144]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #152]
    ldr     x12, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    str     x10, [x0, #168]
    str     x15, [x0, #176]
    ldr     x7, [x1, #24]
    ldr     x4, [x1, #32]
    ldr     x16, [x0, #64]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #64]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #40]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #56]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    str     x15, [x0, #184]
    ldr     x7, [x1, #32]
    ldr     x4, [x1, #40]
    ldr     x16, [x0, #80]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #80]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #48]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #64]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldr     x4, [x1, #144]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #184]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #184]
    str     x15, [x0, #192]
    ldr     x7, [x1, #40]
    ldr     x4, [x1, #48]
    ldr     x16, [x0, #96]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #96]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #56]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #72]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #136]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    str     x15, [x0, #200]
    ldr     x7, [x1, #48]
    ldr     x4, [x1, #56]
    ldr     x16, [x0, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #112]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #80]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #144]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #184]
    ldr     x12, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    str     x10, [x0, #200]
    str     x15, [x0, #208]
    ldr     x7, [x1, #56]
    ldr     x4, [x1, #64]
    ldr     x16, [x0, #128]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #128]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #72]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #88]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    ldp     x4, x5, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    str     x15, [x0, #216]
    ldr     x7, [x1, #64]
    ldr     x4, [x1, #72]
    ldr     x16, [x0, #144]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #144]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #80]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #96]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
    ldr     x4, [x1, #144]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #216]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #216]
    str     x15, [x0, #224]
    ldr     x7, [x1, #72]
    ldr     x4, [x1, #80]
    ldr     x16, [x0, #160]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #160]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #88]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #104]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #136]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #200]
    ldp     x12, x13, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    stp     x10, x11, [x0, #216]
    str     x15, [x0, #232]
    ldr     x7, [x1, #80]
    ldr     x4, [x1, #88]
    ldr     x16, [x0, #176]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #176]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #112]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #144]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #216]
    ldr     x12, [x0, #232]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #216]
    str     x10, [x0, #232]
    str     x15, [x0, #240]
    ldr     x7, [x1, #88]
    ldr     x4, [x1, #96]
    ldr     x16, [x0, #192]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #192]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #104]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #120]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #200]
    ldp     x12, x13, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    stp     x10, x11, [x0, #216]
    ldp     x4, x5, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #232]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #232]
    str     x15, [x0, #248]
    ldr     x7, [x1, #96]
    ldr     x4, [x1, #104]
    ldr     x16, [x0, #208]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #208]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #112]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #128]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #216]
    ldp     x12, x13, [x0, #232]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #216]
    stp     x10, x11, [x0, #232]
    ldr     x4, [x1, #144]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #248]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #248]
    str     x15, [x0, #256]
    ldr     x7, [x1, #104]
    ldr     x4, [x1, #112]
    ldr     x16, [x0, #224]
    and     x17, x4, x7, asr 63
    adds    x16, x16, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #224]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #120]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #136]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    ldp     x16, x17, [x0, #232]
    ldp     x12, x13, [x0, #248]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #232]
    stp     x10, x11, [x0, #248]
    str     x15, [x0, #264]
    ldr     x7, [x1, #112]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #240]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #144]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #248]
    ldr     x12, [x0, #264]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #248]
    str     x10, [x0, #264]
    str     x15, [x0, #272]
    ldr     x7, [x1, #120]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #256]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    ldp     x4, x5, [x1, #136]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #264]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #264]
    str     x15, [x0, #280]
    ldr     x7, [x1, #128]
    and     x17, x4, x7, asr 63
    adds    x16, x9, x17
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #272]
    adc     x15, x12, xzr
    adds    xzr, x7, x7
    adc     x3, x4, x4
    mov     x6, x4
    ldr     x4, [x1, #144]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #280]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #280]
    and     x17, x4, x6, asr 63
    mul     x9, x4, x4
    umulh   x12, x4, x4
    adds    x16, x15, x17
    adc     x12, x12, xzr
    adds    x16, x16, x9
    str     x16, [x0, #288]
    adc     x14, x12, xzr
    str     x14, [x0, #296]
    ret
