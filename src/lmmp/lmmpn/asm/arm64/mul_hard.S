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
// void lmmp_mul_hard_N_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb);
//
//   平衡硬编码乘法: [dst,2N] = [numa,N] * [numb,N]。
//   种子 mul_1 + 展开 addmul_1, 双链结构:
//     链1: s_k = lo_k + hi_{k-1} + C (adcs 链跨整行)
//     链2: dst += s, 溢出并入 carry -> 顶列直存
//

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_mul_hard_1_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_1_):

    ldr     x3, [x1]
    ldr     x4, [x2]
    mul     x5, x3, x4
    umulh   x6, x3, x4
    stp     x5, x6, [x0]
    ret

.globl ASM_GSYM(lmmp_mul_hard_2_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_2_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    stp     x8, x9, [x0, #0]
    str     x15, [x0, #16]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #8]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    str     x15, [x0, #24]
    ret

.globl ASM_GSYM(lmmp_mul_hard_3_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_3_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    stp     x8, x9, [x0, #0]
    str     x10, [x0, #16]
    str     x15, [x0, #24]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #8]
    ldr     x12, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    str     x10, [x0, #24]
    str     x15, [x0, #32]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x6, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    adcs    x10, x10, x13
    adc     x15, x14, xzr
    ldp     x16, x17, [x0, #16]
    ldr     x12, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    str     x10, [x0, #32]
    str     x15, [x0, #40]
    ret

.globl ASM_GSYM(lmmp_mul_hard_4_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_4_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
    str     x15, [x0, #32]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    str     x15, [x0, #40]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
    str     x15, [x0, #48]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ret

.globl ASM_GSYM(lmmp_mul_hard_5_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_5_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
    ldr     x4, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    str     x8, [x0, #32]
    str     x15, [x0, #40]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldr     x4, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #40]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #40]
    str     x15, [x0, #48]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
    ldr     x4, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #48]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #48]
    str     x15, [x0, #56]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x4, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #56]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #56]
    str     x15, [x0, #64]
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
    ldr     x4, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #64]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #64]
    str     x15, [x0, #72]
    ret

.globl ASM_GSYM(lmmp_mul_hard_6_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_6_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    stp     x8, x9, [x0, #32]
    str     x15, [x0, #48]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
    ldp     x4, x5, [x1, #32]
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
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    str     x15, [x0, #64]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
    ldp     x4, x5, [x1, #32]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    str     x15, [x0, #80]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x4, x5, [x1, #32]
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
    ret

.globl ASM_GSYM(lmmp_mul_hard_7_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_7_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    str     x10, [x0, #48]
    str     x15, [x0, #56]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldr     x12, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    str     x10, [x0, #56]
    str     x15, [x0, #64]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldr     x12, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    str     x10, [x0, #64]
    str     x15, [x0, #72]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldr     x12, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    str     x10, [x0, #80]
    str     x15, [x0, #88]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldr     x12, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    str     x10, [x0, #88]
    str     x15, [x0, #96]
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldr     x12, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    str     x10, [x0, #96]
    str     x15, [x0, #104]
    ret

.globl ASM_GSYM(lmmp_mul_hard_8_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_8_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
    str     x15, [x0, #64]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
    str     x15, [x0, #80]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
    str     x15, [x0, #96]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
    str     x15, [x0, #112]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ret

.globl ASM_GSYM(lmmp_mul_hard_9_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_9_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    str     x8, [x0, #64]
    str     x15, [x0, #72]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #72]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #72]
    str     x15, [x0, #80]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #80]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #80]
    str     x15, [x0, #88]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #96]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #96]
    str     x15, [x0, #104]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #104]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #104]
    str     x15, [x0, #112]
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #112]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #112]
    str     x15, [x0, #120]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #120]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #120]
    str     x15, [x0, #128]
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
    ldr     x4, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #128]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #128]
    str     x15, [x0, #136]
    ret

.globl ASM_GSYM(lmmp_mul_hard_10_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_10_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    stp     x8, x9, [x0, #64]
    str     x15, [x0, #80]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
    ldp     x4, x5, [x1, #64]
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
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    str     x15, [x0, #96]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    str     x15, [x0, #112]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #64]
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
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    str     x15, [x0, #128]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
    ldp     x4, x5, [x1, #64]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    str     x15, [x0, #144]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #64]
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
    ret

.globl ASM_GSYM(lmmp_mul_hard_11_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_11_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    stp     x8, x9, [x0, #64]
    str     x10, [x0, #80]
    str     x15, [x0, #88]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
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
    ldp     x16, x17, [x0, #72]
    ldr     x12, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    str     x10, [x0, #88]
    str     x15, [x0, #96]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldr     x12, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    str     x10, [x0, #96]
    str     x15, [x0, #104]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldr     x12, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    str     x10, [x0, #112]
    str     x15, [x0, #120]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldr     x12, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    str     x10, [x0, #120]
    str     x15, [x0, #128]
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldr     x12, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    str     x10, [x0, #128]
    str     x15, [x0, #136]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldr     x12, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    str     x10, [x0, #144]
    str     x15, [x0, #152]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldr     x12, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x10, [x0, #152]
    str     x15, [x0, #160]
    ldr     x3, [x2, #80]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldr     x12, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    str     x10, [x0, #160]
    str     x15, [x0, #168]
    ret

.globl ASM_GSYM(lmmp_mul_hard_12_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_12_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
    str     x15, [x0, #96]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
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
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
    str     x15, [x0, #112]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
    str     x15, [x0, #128]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
    str     x15, [x0, #144]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
    str     x15, [x0, #160]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldr     x3, [x2, #80]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
    str     x15, [x0, #176]
    ldr     x3, [x2, #88]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
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
    ret

.globl ASM_GSYM(lmmp_mul_hard_13_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_13_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    str     x8, [x0, #96]
    str     x15, [x0, #104]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #104]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #104]
    str     x15, [x0, #112]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #112]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #112]
    str     x15, [x0, #120]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #128]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #128]
    str     x15, [x0, #136]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #136]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #136]
    str     x15, [x0, #144]
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #144]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #144]
    str     x15, [x0, #152]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #160]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #160]
    str     x15, [x0, #168]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #168]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #168]
    str     x15, [x0, #176]
    ldr     x3, [x2, #80]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #176]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #176]
    str     x15, [x0, #184]
    ldr     x3, [x2, #88]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #184]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #184]
    str     x15, [x0, #192]
    ldr     x3, [x2, #96]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
    ldr     x4, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #192]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #192]
    str     x15, [x0, #200]
    ret

.globl ASM_GSYM(lmmp_mul_hard_14_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_14_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    stp     x8, x9, [x0, #96]
    str     x15, [x0, #112]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
    ldp     x4, x5, [x1, #96]
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
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    str     x15, [x0, #128]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    str     x15, [x0, #144]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #96]
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
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    str     x15, [x0, #160]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    str     x15, [x0, #176]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #96]
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
    ldr     x3, [x2, #80]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    str     x15, [x0, #192]
    ldr     x3, [x2, #88]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldr     x3, [x2, #96]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
    ldp     x4, x5, [x1, #96]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    str     x15, [x0, #208]
    ldr     x3, [x2, #104]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    ldp     x4, x5, [x1, #96]
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
    ret

.globl ASM_GSYM(lmmp_mul_hard_15_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_15_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    stp     x8, x9, [x0, #96]
    str     x10, [x0, #112]
    str     x15, [x0, #120]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldr     x12, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    str     x10, [x0, #120]
    str     x15, [x0, #128]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldr     x12, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    str     x10, [x0, #128]
    str     x15, [x0, #136]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldr     x12, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    str     x10, [x0, #144]
    str     x15, [x0, #152]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldr     x12, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x10, [x0, #152]
    str     x15, [x0, #160]
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldr     x12, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    str     x10, [x0, #160]
    str     x15, [x0, #168]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldr     x12, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    str     x10, [x0, #176]
    str     x15, [x0, #184]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldr     x12, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    str     x10, [x0, #184]
    str     x15, [x0, #192]
    ldr     x3, [x2, #80]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldr     x12, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    str     x10, [x0, #192]
    str     x15, [x0, #200]
    ldr     x3, [x2, #88]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldr     x3, [x2, #96]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
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
    ldp     x16, x17, [x0, #192]
    ldr     x12, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    str     x10, [x0, #208]
    str     x15, [x0, #216]
    ldr     x3, [x2, #104]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
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
    ldp     x16, x17, [x0, #200]
    ldr     x12, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    str     x10, [x0, #216]
    str     x15, [x0, #224]
    ldr     x3, [x2, #112]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
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
    ldp     x16, x17, [x0, #208]
    ldr     x12, [x0, #224]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #208]
    str     x10, [x0, #224]
    str     x15, [x0, #232]
    ret

.globl ASM_GSYM(lmmp_mul_hard_16_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_16_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
    str     x15, [x0, #128]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
    str     x15, [x0, #144]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
    str     x15, [x0, #160]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
    str     x15, [x0, #176]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
    str     x15, [x0, #192]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldr     x3, [x2, #80]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
    str     x15, [x0, #208]
    ldr     x3, [x2, #88]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldr     x3, [x2, #96]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
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
    ldp     x16, x17, [x0, #192]
    ldp     x12, x13, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    stp     x10, x11, [x0, #208]
    str     x15, [x0, #224]
    ldr     x3, [x2, #104]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
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
    ldr     x3, [x2, #112]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
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
    ldp     x16, x17, [x0, #208]
    ldp     x12, x13, [x0, #224]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #208]
    stp     x10, x11, [x0, #224]
    str     x15, [x0, #240]
    ldr     x3, [x2, #120]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
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
    ret

.globl ASM_GSYM(lmmp_mul_hard_17_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_17_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    str     x8, [x0, #128]
    str     x15, [x0, #136]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #136]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #136]
    str     x15, [x0, #144]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #144]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #144]
    str     x15, [x0, #152]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #160]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #160]
    str     x15, [x0, #168]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #168]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #168]
    str     x15, [x0, #176]
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #176]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #176]
    str     x15, [x0, #184]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #192]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #192]
    str     x15, [x0, #200]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #200]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #200]
    str     x15, [x0, #208]
    ldr     x3, [x2, #80]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #208]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #208]
    str     x15, [x0, #216]
    ldr     x3, [x2, #88]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldr     x3, [x2, #96]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
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
    ldp     x16, x17, [x0, #192]
    ldp     x12, x13, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    stp     x10, x11, [x0, #208]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #224]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #224]
    str     x15, [x0, #232]
    ldr     x3, [x2, #104]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
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
    ldp     x16, x17, [x0, #200]
    ldp     x12, x13, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    stp     x10, x11, [x0, #216]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #232]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #232]
    str     x15, [x0, #240]
    ldr     x3, [x2, #112]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
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
    ldp     x16, x17, [x0, #208]
    ldp     x12, x13, [x0, #224]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #208]
    stp     x10, x11, [x0, #224]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #240]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #240]
    str     x15, [x0, #248]
    ldr     x3, [x2, #120]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
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
    ldp     x16, x17, [x0, #216]
    ldp     x12, x13, [x0, #232]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #216]
    stp     x10, x11, [x0, #232]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #248]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #248]
    str     x15, [x0, #256]
    ldr     x3, [x2, #128]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
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
    ldp     x16, x17, [x0, #192]
    ldp     x12, x13, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    stp     x10, x11, [x0, #208]
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
    ldp     x16, x17, [x0, #224]
    ldp     x12, x13, [x0, #240]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #224]
    stp     x10, x11, [x0, #240]
    ldr     x4, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    adds    x8, x8, x15
    adc     x15, x12, xzr
    ldr     x16, [x0, #256]
    adds    x8, x16, x8
    adc     x15, x15, xzr
    str     x8, [x0, #256]
    str     x15, [x0, #264]
    ret

.globl ASM_GSYM(lmmp_mul_hard_18_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_18_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    stp     x8, x9, [x0, #128]
    str     x15, [x0, #144]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
    ldp     x4, x5, [x1, #128]
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
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    str     x15, [x0, #160]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    str     x15, [x0, #176]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
    ldp     x4, x5, [x1, #128]
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
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    str     x15, [x0, #192]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    str     x15, [x0, #208]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
    ldp     x4, x5, [x1, #128]
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
    ldr     x3, [x2, #80]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #208]
    str     x15, [x0, #224]
    ldr     x3, [x2, #88]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldr     x3, [x2, #96]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
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
    ldp     x16, x17, [x0, #192]
    ldp     x12, x13, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    stp     x10, x11, [x0, #208]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #224]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #224]
    str     x15, [x0, #240]
    ldr     x3, [x2, #104]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
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
    ldp     x16, x17, [x0, #200]
    ldp     x12, x13, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    stp     x10, x11, [x0, #216]
    ldp     x4, x5, [x1, #128]
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
    ldr     x3, [x2, #112]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
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
    ldp     x16, x17, [x0, #208]
    ldp     x12, x13, [x0, #224]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #208]
    stp     x10, x11, [x0, #224]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #240]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #240]
    str     x15, [x0, #256]
    ldr     x3, [x2, #120]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
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
    ldp     x16, x17, [x0, #216]
    ldp     x12, x13, [x0, #232]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #216]
    stp     x10, x11, [x0, #232]
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
    ldr     x3, [x2, #128]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
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
    ldp     x16, x17, [x0, #192]
    ldp     x12, x13, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    stp     x10, x11, [x0, #208]
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
    ldp     x16, x17, [x0, #224]
    ldp     x12, x13, [x0, #240]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #224]
    stp     x10, x11, [x0, #240]
    ldp     x4, x5, [x1, #128]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    adc     x15, x13, xzr
    ldp     x16, x17, [x0, #256]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #256]
    str     x15, [x0, #272]
    ldr     x3, [x2, #136]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
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
    ldp     x16, x17, [x0, #200]
    ldp     x12, x13, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    stp     x10, x11, [x0, #216]
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
    ldp     x16, x17, [x0, #232]
    ldp     x12, x13, [x0, #248]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #232]
    stp     x10, x11, [x0, #248]
    ldp     x4, x5, [x1, #128]
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
    ret

.globl ASM_GSYM(lmmp_mul_hard_19_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_19_):

    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1
    ldr     x3, [x2]
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, xzr
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    adcs    x11, x11, x14
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #0]
    stp     x10, x11, [x0, #16]
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
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    stp     x8, x9, [x0, #128]
    str     x10, [x0, #144]
    str     x15, [x0, #152]
    ldr     x3, [x2, #8]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #8]
    ldp     x12, x13, [x0, #24]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #8]
    stp     x10, x11, [x0, #24]
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
    ldp     x16, x17, [x0, #40]
    ldp     x12, x13, [x0, #56]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #40]
    stp     x10, x11, [x0, #56]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldr     x12, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    str     x10, [x0, #152]
    str     x15, [x0, #160]
    ldr     x3, [x2, #16]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #16]
    ldp     x12, x13, [x0, #32]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #16]
    stp     x10, x11, [x0, #32]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldr     x12, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    str     x10, [x0, #160]
    str     x15, [x0, #168]
    ldr     x3, [x2, #24]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldr     x3, [x2, #32]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #32]
    ldp     x12, x13, [x0, #48]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #32]
    stp     x10, x11, [x0, #48]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldr     x12, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    str     x10, [x0, #176]
    str     x15, [x0, #184]
    ldr     x3, [x2, #40]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #72]
    ldp     x12, x13, [x0, #88]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #72]
    stp     x10, x11, [x0, #88]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldr     x12, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    str     x10, [x0, #184]
    str     x15, [x0, #192]
    ldr     x3, [x2, #48]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #48]
    ldp     x12, x13, [x0, #64]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #48]
    stp     x10, x11, [x0, #64]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldr     x12, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    str     x10, [x0, #192]
    str     x15, [x0, #200]
    ldr     x3, [x2, #56]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #88]
    ldp     x12, x13, [x0, #104]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #88]
    stp     x10, x11, [x0, #104]
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
    ldr     x3, [x2, #64]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #64]
    ldp     x12, x13, [x0, #80]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #64]
    stp     x10, x11, [x0, #80]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
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
    ldp     x16, x17, [x0, #192]
    ldr     x12, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    str     x10, [x0, #208]
    str     x15, [x0, #216]
    ldr     x3, [x2, #72]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #104]
    ldp     x12, x13, [x0, #120]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #104]
    stp     x10, x11, [x0, #120]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
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
    ldp     x16, x17, [x0, #200]
    ldr     x12, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    str     x10, [x0, #216]
    str     x15, [x0, #224]
    ldr     x3, [x2, #80]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #80]
    ldp     x12, x13, [x0, #96]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #80]
    stp     x10, x11, [x0, #96]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
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
    ldp     x16, x17, [x0, #208]
    ldr     x12, [x0, #224]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #208]
    str     x10, [x0, #224]
    str     x15, [x0, #232]
    ldr     x3, [x2, #88]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #120]
    ldp     x12, x13, [x0, #136]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #120]
    stp     x10, x11, [x0, #136]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldr     x3, [x2, #96]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #96]
    ldp     x12, x13, [x0, #112]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #96]
    stp     x10, x11, [x0, #112]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
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
    ldp     x16, x17, [x0, #192]
    ldp     x12, x13, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    stp     x10, x11, [x0, #208]
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
    ldp     x16, x17, [x0, #224]
    ldr     x12, [x0, #240]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #224]
    str     x10, [x0, #240]
    str     x15, [x0, #248]
    ldr     x3, [x2, #104]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #136]
    ldp     x12, x13, [x0, #152]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #136]
    stp     x10, x11, [x0, #152]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
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
    ldp     x16, x17, [x0, #200]
    ldp     x12, x13, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    stp     x10, x11, [x0, #216]
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
    ldp     x16, x17, [x0, #232]
    ldr     x12, [x0, #248]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #232]
    str     x10, [x0, #248]
    str     x15, [x0, #256]
    ldr     x3, [x2, #112]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #112]
    ldp     x12, x13, [x0, #128]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #112]
    stp     x10, x11, [x0, #128]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
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
    ldp     x16, x17, [x0, #208]
    ldp     x12, x13, [x0, #224]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #208]
    stp     x10, x11, [x0, #224]
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
    ldp     x16, x17, [x0, #240]
    ldr     x12, [x0, #256]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #240]
    str     x10, [x0, #256]
    str     x15, [x0, #264]
    ldr     x3, [x2, #120]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #152]
    ldp     x12, x13, [x0, #168]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #152]
    stp     x10, x11, [x0, #168]
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
    ldp     x16, x17, [x0, #184]
    ldp     x12, x13, [x0, #200]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #184]
    stp     x10, x11, [x0, #200]
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
    ldp     x16, x17, [x0, #216]
    ldp     x12, x13, [x0, #232]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #216]
    stp     x10, x11, [x0, #232]
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
    ldr     x3, [x2, #128]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #128]
    ldp     x12, x13, [x0, #144]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #128]
    stp     x10, x11, [x0, #144]
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
    ldp     x16, x17, [x0, #160]
    ldp     x12, x13, [x0, #176]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #160]
    stp     x10, x11, [x0, #176]
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
    ldp     x16, x17, [x0, #192]
    ldp     x12, x13, [x0, #208]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #192]
    stp     x10, x11, [x0, #208]
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
    ldp     x16, x17, [x0, #224]
    ldp     x12, x13, [x0, #240]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #224]
    stp     x10, x11, [x0, #240]
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
    ldp     x16, x17, [x0, #256]
    ldr     x12, [x0, #272]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #256]
    str     x10, [x0, #272]
    str     x15, [x0, #280]
    ldr     x3, [x2, #136]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #168]
    ldp     x12, x13, [x0, #184]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #168]
    stp     x10, x11, [x0, #184]
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
    ldp     x16, x17, [x0, #200]
    ldp     x12, x13, [x0, #216]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #200]
    stp     x10, x11, [x0, #216]
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
    ldp     x16, x17, [x0, #232]
    ldp     x12, x13, [x0, #248]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #232]
    stp     x10, x11, [x0, #248]
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
    ldp     x16, x17, [x0, #264]
    ldr     x12, [x0, #280]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #264]
    str     x10, [x0, #280]
    str     x15, [x0, #288]
    ldr     x3, [x2, #144]
    mov     x15, xzr
    ldp     x4, x5, [x1, #0]
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldp     x6, x7, [x1, #16]
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
    ldp     x16, x17, [x0, #144]
    ldp     x12, x13, [x0, #160]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #144]
    stp     x10, x11, [x0, #160]
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
    ldp     x16, x17, [x0, #176]
    ldp     x12, x13, [x0, #192]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #176]
    stp     x10, x11, [x0, #192]
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
    ldp     x16, x17, [x0, #208]
    ldp     x12, x13, [x0, #224]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #208]
    stp     x10, x11, [x0, #224]
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
    ldp     x16, x17, [x0, #240]
    ldp     x12, x13, [x0, #256]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #240]
    stp     x10, x11, [x0, #256]
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
    ldp     x16, x17, [x0, #272]
    ldr     x12, [x0, #288]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adc     x15, x15, xzr
    stp     x8, x9, [x0, #272]
    str     x10, [x0, #288]
    str     x15, [x0, #296]
    ret
