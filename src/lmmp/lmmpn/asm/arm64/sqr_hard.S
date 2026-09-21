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
//   交叉乘逐行累加 -> 整体倍增 -> 对角平方折叠 (单进位链)。
//

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_sqr_hard_1_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_1_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #8]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_2_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_2_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #24]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    str     x6, [x0, #16]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_3_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_3_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #40]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #24]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    str     x6, [x0, #32]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_4_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_4_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #56]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #32]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #40]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    str     x6, [x0, #48]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_5_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_5_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #72]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #40]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #48]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #56]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    str     x6, [x0, #64]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_6_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_6_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #88]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #48]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #56]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #64]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #72]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    str     x6, [x0, #80]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_7_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_7_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #104]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #56]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #64]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #72]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #80]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #88]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    str     x6, [x0, #96]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_8_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_8_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #120]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #64]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #72]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #80]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #88]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #96]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #104]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    str     x6, [x0, #112]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_9_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_9_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #136]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #72]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #80]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #88]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #96]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #104]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #112]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #120]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    str     x6, [x0, #128]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_10_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_10_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #152]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #80]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #88]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #96]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #104]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #112]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #120]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #128]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #136]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    str     x6, [x0, #144]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_11_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_11_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #168]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #80]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #88]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #96]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #104]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #112]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #120]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #128]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #136]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #144]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #152]           // 顶列直存
    ldr     x3, [x1, #72]            // a_9
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x8, x8, x4
    str     x8, [x0, #152]
    adc     x6, x5, xzr
    str     x6, [x0, #160]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    ldr     x8, [x0, #160]
    adcs     x8, x8, x8
    str     x8, [x0, #160]
    ldr     x8, [x0, #168]
    adcs     x8, x8, x8
    str     x8, [x0, #168]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #160]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #168]
    adds    x8, x8, x6
    str     x8, [x0, #168]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_12_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_12_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #184]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #80]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #88]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #96]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #104]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #112]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #120]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #128]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #136]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #144]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #152]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #160]           // 顶列直存
    ldr     x3, [x1, #72]            // a_9
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x8, x8, x4
    str     x8, [x0, #152]
    adc     x6, x5, xzr
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #168]           // 顶列直存
    ldr     x3, [x1, #80]            // a_10
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x8, x8, x4
    str     x8, [x0, #168]
    adc     x6, x5, xzr
    str     x6, [x0, #176]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    ldr     x8, [x0, #160]
    adcs     x8, x8, x8
    str     x8, [x0, #160]
    ldr     x8, [x0, #168]
    adcs     x8, x8, x8
    str     x8, [x0, #168]
    ldr     x8, [x0, #176]
    adcs     x8, x8, x8
    str     x8, [x0, #176]
    ldr     x8, [x0, #184]
    adcs     x8, x8, x8
    str     x8, [x0, #184]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #160]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #168]
    adds    x8, x8, x6
    str     x8, [x0, #168]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #176]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #184]
    adds    x8, x8, x6
    str     x8, [x0, #184]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_13_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_13_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #200]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #80]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #88]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #96]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #104]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #112]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #120]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #128]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #136]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #144]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #152]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #160]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #168]           // 顶列直存
    ldr     x3, [x1, #72]            // a_9
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x8, x8, x4
    str     x8, [x0, #152]
    adc     x6, x5, xzr
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #176]           // 顶列直存
    ldr     x3, [x1, #80]            // a_10
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x8, x8, x4
    str     x8, [x0, #168]
    adc     x6, x5, xzr
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #184]           // 顶列直存
    ldr     x3, [x1, #88]            // a_11
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x8, x8, x4
    str     x8, [x0, #184]
    adc     x6, x5, xzr
    str     x6, [x0, #192]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    ldr     x8, [x0, #160]
    adcs     x8, x8, x8
    str     x8, [x0, #160]
    ldr     x8, [x0, #168]
    adcs     x8, x8, x8
    str     x8, [x0, #168]
    ldr     x8, [x0, #176]
    adcs     x8, x8, x8
    str     x8, [x0, #176]
    ldr     x8, [x0, #184]
    adcs     x8, x8, x8
    str     x8, [x0, #184]
    ldr     x8, [x0, #192]
    adcs     x8, x8, x8
    str     x8, [x0, #192]
    ldr     x8, [x0, #200]
    adcs     x8, x8, x8
    str     x8, [x0, #200]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #160]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #168]
    adds    x8, x8, x6
    str     x8, [x0, #168]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #176]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #184]
    adds    x8, x8, x6
    str     x8, [x0, #184]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #192]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #200]
    adds    x8, x8, x6
    str     x8, [x0, #200]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_14_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_14_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #216]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #80]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #88]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #96]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #104]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #112]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #120]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #128]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #136]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #144]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #152]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #160]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #168]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #176]           // 顶列直存
    ldr     x3, [x1, #72]            // a_9
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x8, x8, x4
    str     x8, [x0, #152]
    adc     x6, x5, xzr
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #184]           // 顶列直存
    ldr     x3, [x1, #80]            // a_10
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x8, x8, x4
    str     x8, [x0, #168]
    adc     x6, x5, xzr
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #192]           // 顶列直存
    ldr     x3, [x1, #88]            // a_11
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x8, x8, x4
    str     x8, [x0, #184]
    adc     x6, x5, xzr
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #200]           // 顶列直存
    ldr     x3, [x1, #96]            // a_12
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x8, x8, x4
    str     x8, [x0, #200]
    adc     x6, x5, xzr
    str     x6, [x0, #208]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    ldr     x8, [x0, #160]
    adcs     x8, x8, x8
    str     x8, [x0, #160]
    ldr     x8, [x0, #168]
    adcs     x8, x8, x8
    str     x8, [x0, #168]
    ldr     x8, [x0, #176]
    adcs     x8, x8, x8
    str     x8, [x0, #176]
    ldr     x8, [x0, #184]
    adcs     x8, x8, x8
    str     x8, [x0, #184]
    ldr     x8, [x0, #192]
    adcs     x8, x8, x8
    str     x8, [x0, #192]
    ldr     x8, [x0, #200]
    adcs     x8, x8, x8
    str     x8, [x0, #200]
    ldr     x8, [x0, #208]
    adcs     x8, x8, x8
    str     x8, [x0, #208]
    ldr     x8, [x0, #216]
    adcs     x8, x8, x8
    str     x8, [x0, #216]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #160]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #168]
    adds    x8, x8, x6
    str     x8, [x0, #168]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #176]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #184]
    adds    x8, x8, x6
    str     x8, [x0, #184]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #192]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #200]
    adds    x8, x8, x6
    str     x8, [x0, #200]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #208]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #216]
    adds    x8, x8, x6
    str     x8, [x0, #216]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_15_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_15_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #232]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #80]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #88]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #96]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #104]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #112]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #120]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #128]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #136]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #144]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #152]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #160]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #168]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #176]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #184]           // 顶列直存
    ldr     x3, [x1, #72]            // a_9
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x8, x8, x4
    str     x8, [x0, #152]
    adc     x6, x5, xzr
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #192]           // 顶列直存
    ldr     x3, [x1, #80]            // a_10
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x8, x8, x4
    str     x8, [x0, #168]
    adc     x6, x5, xzr
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #200]           // 顶列直存
    ldr     x3, [x1, #88]            // a_11
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x8, x8, x4
    str     x8, [x0, #184]
    adc     x6, x5, xzr
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #208]           // 顶列直存
    ldr     x3, [x1, #96]            // a_12
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x8, x8, x4
    str     x8, [x0, #200]
    adc     x6, x5, xzr
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #216]           // 顶列直存
    ldr     x3, [x1, #104]            // a_13
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x8, x8, x4
    str     x8, [x0, #216]
    adc     x6, x5, xzr
    str     x6, [x0, #224]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    ldr     x8, [x0, #160]
    adcs     x8, x8, x8
    str     x8, [x0, #160]
    ldr     x8, [x0, #168]
    adcs     x8, x8, x8
    str     x8, [x0, #168]
    ldr     x8, [x0, #176]
    adcs     x8, x8, x8
    str     x8, [x0, #176]
    ldr     x8, [x0, #184]
    adcs     x8, x8, x8
    str     x8, [x0, #184]
    ldr     x8, [x0, #192]
    adcs     x8, x8, x8
    str     x8, [x0, #192]
    ldr     x8, [x0, #200]
    adcs     x8, x8, x8
    str     x8, [x0, #200]
    ldr     x8, [x0, #208]
    adcs     x8, x8, x8
    str     x8, [x0, #208]
    ldr     x8, [x0, #216]
    adcs     x8, x8, x8
    str     x8, [x0, #216]
    ldr     x8, [x0, #224]
    adcs     x8, x8, x8
    str     x8, [x0, #224]
    ldr     x8, [x0, #232]
    adcs     x8, x8, x8
    str     x8, [x0, #232]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #160]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #168]
    adds    x8, x8, x6
    str     x8, [x0, #168]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #176]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #184]
    adds    x8, x8, x6
    str     x8, [x0, #184]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #192]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #200]
    adds    x8, x8, x6
    str     x8, [x0, #200]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #208]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #216]
    adds    x8, x8, x6
    str     x8, [x0, #216]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #224]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #232]
    adds    x8, x8, x6
    str     x8, [x0, #232]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_16_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_16_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #248]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #80]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #88]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #96]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #104]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #112]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #120]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #128]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #136]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #144]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #152]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #160]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #168]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #176]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #184]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #192]           // 顶列直存
    ldr     x3, [x1, #72]            // a_9
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x8, x8, x4
    str     x8, [x0, #152]
    adc     x6, x5, xzr
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #200]           // 顶列直存
    ldr     x3, [x1, #80]            // a_10
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x8, x8, x4
    str     x8, [x0, #168]
    adc     x6, x5, xzr
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #208]           // 顶列直存
    ldr     x3, [x1, #88]            // a_11
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x8, x8, x4
    str     x8, [x0, #184]
    adc     x6, x5, xzr
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #216]           // 顶列直存
    ldr     x3, [x1, #96]            // a_12
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x8, x8, x4
    str     x8, [x0, #200]
    adc     x6, x5, xzr
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #224]           // 顶列直存
    ldr     x3, [x1, #104]            // a_13
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x8, x8, x4
    str     x8, [x0, #216]
    adc     x6, x5, xzr
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #232]           // 顶列直存
    ldr     x3, [x1, #112]            // a_14
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x8, x8, x4
    str     x8, [x0, #232]
    adc     x6, x5, xzr
    str     x6, [x0, #240]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    ldr     x8, [x0, #160]
    adcs     x8, x8, x8
    str     x8, [x0, #160]
    ldr     x8, [x0, #168]
    adcs     x8, x8, x8
    str     x8, [x0, #168]
    ldr     x8, [x0, #176]
    adcs     x8, x8, x8
    str     x8, [x0, #176]
    ldr     x8, [x0, #184]
    adcs     x8, x8, x8
    str     x8, [x0, #184]
    ldr     x8, [x0, #192]
    adcs     x8, x8, x8
    str     x8, [x0, #192]
    ldr     x8, [x0, #200]
    adcs     x8, x8, x8
    str     x8, [x0, #200]
    ldr     x8, [x0, #208]
    adcs     x8, x8, x8
    str     x8, [x0, #208]
    ldr     x8, [x0, #216]
    adcs     x8, x8, x8
    str     x8, [x0, #216]
    ldr     x8, [x0, #224]
    adcs     x8, x8, x8
    str     x8, [x0, #224]
    ldr     x8, [x0, #232]
    adcs     x8, x8, x8
    str     x8, [x0, #232]
    ldr     x8, [x0, #240]
    adcs     x8, x8, x8
    str     x8, [x0, #240]
    ldr     x8, [x0, #248]
    adcs     x8, x8, x8
    str     x8, [x0, #248]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #160]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #168]
    adds    x8, x8, x6
    str     x8, [x0, #168]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #176]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #184]
    adds    x8, x8, x6
    str     x8, [x0, #184]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #192]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #200]
    adds    x8, x8, x6
    str     x8, [x0, #200]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #208]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #216]
    adds    x8, x8, x6
    str     x8, [x0, #216]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #224]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #232]
    adds    x8, x8, x6
    str     x8, [x0, #232]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #120]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #240]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #248]
    adds    x8, x8, x6
    str     x8, [x0, #248]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_17_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_17_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #264]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #80]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #88]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #96]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #104]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #112]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #120]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #128]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #136]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #144]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #152]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #160]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #168]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #176]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #184]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #192]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #200]           // 顶列直存
    ldr     x3, [x1, #72]            // a_9
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x8, x8, x4
    str     x8, [x0, #152]
    adc     x6, x5, xzr
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #208]           // 顶列直存
    ldr     x3, [x1, #80]            // a_10
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x8, x8, x4
    str     x8, [x0, #168]
    adc     x6, x5, xzr
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #216]           // 顶列直存
    ldr     x3, [x1, #88]            // a_11
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x8, x8, x4
    str     x8, [x0, #184]
    adc     x6, x5, xzr
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #224]           // 顶列直存
    ldr     x3, [x1, #96]            // a_12
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x8, x8, x4
    str     x8, [x0, #200]
    adc     x6, x5, xzr
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #232]           // 顶列直存
    ldr     x3, [x1, #104]            // a_13
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x8, x8, x4
    str     x8, [x0, #216]
    adc     x6, x5, xzr
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #232]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #240]           // 顶列直存
    ldr     x3, [x1, #112]            // a_14
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x8, x8, x4
    str     x8, [x0, #232]
    adc     x6, x5, xzr
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #240]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #248]           // 顶列直存
    ldr     x3, [x1, #120]            // a_15
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #248]
    adds    x8, x8, x4
    str     x8, [x0, #248]
    adc     x6, x5, xzr
    str     x6, [x0, #256]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    ldr     x8, [x0, #160]
    adcs     x8, x8, x8
    str     x8, [x0, #160]
    ldr     x8, [x0, #168]
    adcs     x8, x8, x8
    str     x8, [x0, #168]
    ldr     x8, [x0, #176]
    adcs     x8, x8, x8
    str     x8, [x0, #176]
    ldr     x8, [x0, #184]
    adcs     x8, x8, x8
    str     x8, [x0, #184]
    ldr     x8, [x0, #192]
    adcs     x8, x8, x8
    str     x8, [x0, #192]
    ldr     x8, [x0, #200]
    adcs     x8, x8, x8
    str     x8, [x0, #200]
    ldr     x8, [x0, #208]
    adcs     x8, x8, x8
    str     x8, [x0, #208]
    ldr     x8, [x0, #216]
    adcs     x8, x8, x8
    str     x8, [x0, #216]
    ldr     x8, [x0, #224]
    adcs     x8, x8, x8
    str     x8, [x0, #224]
    ldr     x8, [x0, #232]
    adcs     x8, x8, x8
    str     x8, [x0, #232]
    ldr     x8, [x0, #240]
    adcs     x8, x8, x8
    str     x8, [x0, #240]
    ldr     x8, [x0, #248]
    adcs     x8, x8, x8
    str     x8, [x0, #248]
    ldr     x8, [x0, #256]
    adcs     x8, x8, x8
    str     x8, [x0, #256]
    ldr     x8, [x0, #264]
    adcs     x8, x8, x8
    str     x8, [x0, #264]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #160]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #168]
    adds    x8, x8, x6
    str     x8, [x0, #168]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #176]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #184]
    adds    x8, x8, x6
    str     x8, [x0, #184]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #192]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #200]
    adds    x8, x8, x6
    str     x8, [x0, #200]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #208]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #216]
    adds    x8, x8, x6
    str     x8, [x0, #216]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #224]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #232]
    adds    x8, x8, x6
    str     x8, [x0, #232]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #120]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #240]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #248]
    adds    x8, x8, x6
    str     x8, [x0, #248]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #128]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #256]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #256]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #264]
    adds    x8, x8, x6
    str     x8, [x0, #264]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_18_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_18_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #280]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #80]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #88]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #96]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #104]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #112]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #120]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #128]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #136]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #144]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #152]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #160]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #168]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #176]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #184]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #192]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #200]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #208]           // 顶列直存
    ldr     x3, [x1, #72]            // a_9
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x8, x8, x4
    str     x8, [x0, #152]
    adc     x6, x5, xzr
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #216]           // 顶列直存
    ldr     x3, [x1, #80]            // a_10
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x8, x8, x4
    str     x8, [x0, #168]
    adc     x6, x5, xzr
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #224]           // 顶列直存
    ldr     x3, [x1, #88]            // a_11
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x8, x8, x4
    str     x8, [x0, #184]
    adc     x6, x5, xzr
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #232]           // 顶列直存
    ldr     x3, [x1, #96]            // a_12
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x8, x8, x4
    str     x8, [x0, #200]
    adc     x6, x5, xzr
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #232]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #240]           // 顶列直存
    ldr     x3, [x1, #104]            // a_13
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x8, x8, x4
    str     x8, [x0, #216]
    adc     x6, x5, xzr
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #232]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #240]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #248]           // 顶列直存
    ldr     x3, [x1, #112]            // a_14
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x8, x8, x4
    str     x8, [x0, #232]
    adc     x6, x5, xzr
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #240]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #248]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #248]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #256]           // 顶列直存
    ldr     x3, [x1, #120]            // a_15
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #248]
    adds    x8, x8, x4
    str     x8, [x0, #248]
    adc     x6, x5, xzr
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #256]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #256]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #264]           // 顶列直存
    ldr     x3, [x1, #128]            // a_16
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #264]
    adds    x8, x8, x4
    str     x8, [x0, #264]
    adc     x6, x5, xzr
    str     x6, [x0, #272]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    ldr     x8, [x0, #160]
    adcs     x8, x8, x8
    str     x8, [x0, #160]
    ldr     x8, [x0, #168]
    adcs     x8, x8, x8
    str     x8, [x0, #168]
    ldr     x8, [x0, #176]
    adcs     x8, x8, x8
    str     x8, [x0, #176]
    ldr     x8, [x0, #184]
    adcs     x8, x8, x8
    str     x8, [x0, #184]
    ldr     x8, [x0, #192]
    adcs     x8, x8, x8
    str     x8, [x0, #192]
    ldr     x8, [x0, #200]
    adcs     x8, x8, x8
    str     x8, [x0, #200]
    ldr     x8, [x0, #208]
    adcs     x8, x8, x8
    str     x8, [x0, #208]
    ldr     x8, [x0, #216]
    adcs     x8, x8, x8
    str     x8, [x0, #216]
    ldr     x8, [x0, #224]
    adcs     x8, x8, x8
    str     x8, [x0, #224]
    ldr     x8, [x0, #232]
    adcs     x8, x8, x8
    str     x8, [x0, #232]
    ldr     x8, [x0, #240]
    adcs     x8, x8, x8
    str     x8, [x0, #240]
    ldr     x8, [x0, #248]
    adcs     x8, x8, x8
    str     x8, [x0, #248]
    ldr     x8, [x0, #256]
    adcs     x8, x8, x8
    str     x8, [x0, #256]
    ldr     x8, [x0, #264]
    adcs     x8, x8, x8
    str     x8, [x0, #264]
    ldr     x8, [x0, #272]
    adcs     x8, x8, x8
    str     x8, [x0, #272]
    ldr     x8, [x0, #280]
    adcs     x8, x8, x8
    str     x8, [x0, #280]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #160]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #168]
    adds    x8, x8, x6
    str     x8, [x0, #168]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #176]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #184]
    adds    x8, x8, x6
    str     x8, [x0, #184]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #192]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #200]
    adds    x8, x8, x6
    str     x8, [x0, #200]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #208]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #216]
    adds    x8, x8, x6
    str     x8, [x0, #216]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #224]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #232]
    adds    x8, x8, x6
    str     x8, [x0, #232]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #120]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #240]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #248]
    adds    x8, x8, x6
    str     x8, [x0, #248]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #128]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #256]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #256]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #264]
    adds    x8, x8, x6
    str     x8, [x0, #264]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #136]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #272]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #272]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #280]
    adds    x8, x8, x6
    str     x8, [x0, #280]
    ret

.globl ASM_GSYM(lmmp_sqr_hard_19_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_19_):

    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角
    str     xzr, [x0]                // dst[0] = 0
    str     xzr, [x0, #296]           // dst[2n-1] = 0
    ldr     x3, [x1]                 // a_0
    ldr     x7, [x1, #8]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    mov     x8, x4
    str     x8, [x0, #8]
    mov     x6, x5                  // c = hi
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #16]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #24]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #32]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #40]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #48]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #56]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #64]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #72]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #80]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #88]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #96]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #104]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #112]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #120]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #128]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #136]
    adc     x6, x5, xzr            // c = hi + CF
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    adds    x8, x4, x6             // lo + c
    str     x8, [x0, #144]
    adc     x6, x5, xzr            // c = hi + CF
    str     x6, [x0, #152]           // 顶列直存
    ldr     x3, [x1, #8]            // a_1
    ldr     x7, [x1, #16]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #24]
    adds    x8, x8, x4
    str     x8, [x0, #24]
    adc     x6, x5, xzr
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #32]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #40]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #160]           // 顶列直存
    ldr     x3, [x1, #16]            // a_2
    ldr     x7, [x1, #24]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #40]
    adds    x8, x8, x4
    str     x8, [x0, #40]
    adc     x6, x5, xzr
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #48]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #56]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #168]           // 顶列直存
    ldr     x3, [x1, #24]            // a_3
    ldr     x7, [x1, #32]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #56]
    adds    x8, x8, x4
    str     x8, [x0, #56]
    adc     x6, x5, xzr
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #64]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #72]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #176]           // 顶列直存
    ldr     x3, [x1, #32]            // a_4
    ldr     x7, [x1, #40]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #72]
    adds    x8, x8, x4
    str     x8, [x0, #72]
    adc     x6, x5, xzr
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #80]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #88]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #184]           // 顶列直存
    ldr     x3, [x1, #40]            // a_5
    ldr     x7, [x1, #48]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #88]
    adds    x8, x8, x4
    str     x8, [x0, #88]
    adc     x6, x5, xzr
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #96]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #104]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #192]           // 顶列直存
    ldr     x3, [x1, #48]            // a_6
    ldr     x7, [x1, #56]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #104]
    adds    x8, x8, x4
    str     x8, [x0, #104]
    adc     x6, x5, xzr
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #112]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #120]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #200]           // 顶列直存
    ldr     x3, [x1, #56]            // a_7
    ldr     x7, [x1, #64]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #120]
    adds    x8, x8, x4
    str     x8, [x0, #120]
    adc     x6, x5, xzr
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #128]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #136]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #208]           // 顶列直存
    ldr     x3, [x1, #64]            // a_8
    ldr     x7, [x1, #72]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #136]
    adds    x8, x8, x4
    str     x8, [x0, #136]
    adc     x6, x5, xzr
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #144]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #152]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #216]           // 顶列直存
    ldr     x3, [x1, #72]            // a_9
    ldr     x7, [x1, #80]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #152]
    adds    x8, x8, x4
    str     x8, [x0, #152]
    adc     x6, x5, xzr
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #160]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #168]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #224]           // 顶列直存
    ldr     x3, [x1, #80]            // a_10
    ldr     x7, [x1, #88]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #168]
    adds    x8, x8, x4
    str     x8, [x0, #168]
    adc     x6, x5, xzr
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #176]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #184]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #232]           // 顶列直存
    ldr     x3, [x1, #88]            // a_11
    ldr     x7, [x1, #96]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #184]
    adds    x8, x8, x4
    str     x8, [x0, #184]
    adc     x6, x5, xzr
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #192]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #200]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #232]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #240]           // 顶列直存
    ldr     x3, [x1, #96]            // a_12
    ldr     x7, [x1, #104]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #200]
    adds    x8, x8, x4
    str     x8, [x0, #200]
    adc     x6, x5, xzr
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #208]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #216]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #232]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #240]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #248]           // 顶列直存
    ldr     x3, [x1, #104]            // a_13
    ldr     x7, [x1, #112]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #216]
    adds    x8, x8, x4
    str     x8, [x0, #216]
    adc     x6, x5, xzr
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #224]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #232]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #240]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #248]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #248]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #256]           // 顶列直存
    ldr     x3, [x1, #112]            // a_14
    ldr     x7, [x1, #120]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #232]
    adds    x8, x8, x4
    str     x8, [x0, #232]
    adc     x6, x5, xzr
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #240]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #248]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #248]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #256]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #256]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #264]           // 顶列直存
    ldr     x3, [x1, #120]            // a_15
    ldr     x7, [x1, #128]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #248]
    adds    x8, x8, x4
    str     x8, [x0, #248]
    adc     x6, x5, xzr
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #256]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #256]
    adc     x6, x6, xzr            // c += γ2
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #264]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #264]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #272]           // 顶列直存
    ldr     x3, [x1, #128]            // a_16
    ldr     x7, [x1, #136]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #264]
    adds    x8, x8, x4
    str     x8, [x0, #264]
    adc     x6, x5, xzr
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #272]
    adds    x7, x4, x6             // t = lo + c
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #272]
    adc     x6, x6, xzr            // c += γ2
    str     x6, [x0, #280]           // 顶列直存
    ldr     x3, [x1, #136]            // a_17
    ldr     x7, [x1, #144]
    mul     x4, x7, x3
    umulh   x5, x7, x3
    ldr     x8, [x0, #280]
    adds    x8, x8, x4
    str     x8, [x0, #280]
    adc     x6, x5, xzr
    str     x6, [x0, #288]           // 顶列直存
    # dst *= 2
    ldr     x8, [x0, #8]
    adds     x8, x8, x8
    str     x8, [x0, #8]
    ldr     x8, [x0, #16]
    adcs     x8, x8, x8
    str     x8, [x0, #16]
    ldr     x8, [x0, #24]
    adcs     x8, x8, x8
    str     x8, [x0, #24]
    ldr     x8, [x0, #32]
    adcs     x8, x8, x8
    str     x8, [x0, #32]
    ldr     x8, [x0, #40]
    adcs     x8, x8, x8
    str     x8, [x0, #40]
    ldr     x8, [x0, #48]
    adcs     x8, x8, x8
    str     x8, [x0, #48]
    ldr     x8, [x0, #56]
    adcs     x8, x8, x8
    str     x8, [x0, #56]
    ldr     x8, [x0, #64]
    adcs     x8, x8, x8
    str     x8, [x0, #64]
    ldr     x8, [x0, #72]
    adcs     x8, x8, x8
    str     x8, [x0, #72]
    ldr     x8, [x0, #80]
    adcs     x8, x8, x8
    str     x8, [x0, #80]
    ldr     x8, [x0, #88]
    adcs     x8, x8, x8
    str     x8, [x0, #88]
    ldr     x8, [x0, #96]
    adcs     x8, x8, x8
    str     x8, [x0, #96]
    ldr     x8, [x0, #104]
    adcs     x8, x8, x8
    str     x8, [x0, #104]
    ldr     x8, [x0, #112]
    adcs     x8, x8, x8
    str     x8, [x0, #112]
    ldr     x8, [x0, #120]
    adcs     x8, x8, x8
    str     x8, [x0, #120]
    ldr     x8, [x0, #128]
    adcs     x8, x8, x8
    str     x8, [x0, #128]
    ldr     x8, [x0, #136]
    adcs     x8, x8, x8
    str     x8, [x0, #136]
    ldr     x8, [x0, #144]
    adcs     x8, x8, x8
    str     x8, [x0, #144]
    ldr     x8, [x0, #152]
    adcs     x8, x8, x8
    str     x8, [x0, #152]
    ldr     x8, [x0, #160]
    adcs     x8, x8, x8
    str     x8, [x0, #160]
    ldr     x8, [x0, #168]
    adcs     x8, x8, x8
    str     x8, [x0, #168]
    ldr     x8, [x0, #176]
    adcs     x8, x8, x8
    str     x8, [x0, #176]
    ldr     x8, [x0, #184]
    adcs     x8, x8, x8
    str     x8, [x0, #184]
    ldr     x8, [x0, #192]
    adcs     x8, x8, x8
    str     x8, [x0, #192]
    ldr     x8, [x0, #200]
    adcs     x8, x8, x8
    str     x8, [x0, #200]
    ldr     x8, [x0, #208]
    adcs     x8, x8, x8
    str     x8, [x0, #208]
    ldr     x8, [x0, #216]
    adcs     x8, x8, x8
    str     x8, [x0, #216]
    ldr     x8, [x0, #224]
    adcs     x8, x8, x8
    str     x8, [x0, #224]
    ldr     x8, [x0, #232]
    adcs     x8, x8, x8
    str     x8, [x0, #232]
    ldr     x8, [x0, #240]
    adcs     x8, x8, x8
    str     x8, [x0, #240]
    ldr     x8, [x0, #248]
    adcs     x8, x8, x8
    str     x8, [x0, #248]
    ldr     x8, [x0, #256]
    adcs     x8, x8, x8
    str     x8, [x0, #256]
    ldr     x8, [x0, #264]
    adcs     x8, x8, x8
    str     x8, [x0, #264]
    ldr     x8, [x0, #272]
    adcs     x8, x8, x8
    str     x8, [x0, #272]
    ldr     x8, [x0, #280]
    adcs     x8, x8, x8
    str     x8, [x0, #280]
    ldr     x8, [x0, #288]
    adcs     x8, x8, x8
    str     x8, [x0, #288]
    ldr     x8, [x0, #296]
    adcs     x8, x8, x8
    str     x8, [x0, #296]
    # 对角平方折叠
    ldr     x7, [x1]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0]
    adds    x8, x8, x4
    str     x8, [x0]
    adc     x6, x5, xzr
    ldr     x8, [x0, #8]
    adds    x8, x8, x6
    str     x8, [x0, #8]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #8]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #16]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #16]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #24]
    adds    x8, x8, x6
    str     x8, [x0, #24]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #16]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #32]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #32]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #40]
    adds    x8, x8, x6
    str     x8, [x0, #40]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #24]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #48]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #48]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #56]
    adds    x8, x8, x6
    str     x8, [x0, #56]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #32]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #64]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #64]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #72]
    adds    x8, x8, x6
    str     x8, [x0, #72]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #40]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #80]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #80]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #88]
    adds    x8, x8, x6
    str     x8, [x0, #88]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #48]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #96]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #96]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #104]
    adds    x8, x8, x6
    str     x8, [x0, #104]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #56]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #112]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #112]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #120]
    adds    x8, x8, x6
    str     x8, [x0, #120]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #64]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #128]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #128]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #136]
    adds    x8, x8, x6
    str     x8, [x0, #136]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #72]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #144]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #144]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #152]
    adds    x8, x8, x6
    str     x8, [x0, #152]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #80]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #160]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #160]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #168]
    adds    x8, x8, x6
    str     x8, [x0, #168]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #88]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #176]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #176]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #184]
    adds    x8, x8, x6
    str     x8, [x0, #184]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #96]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #192]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #192]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #200]
    adds    x8, x8, x6
    str     x8, [x0, #200]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #104]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #208]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #208]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #216]
    adds    x8, x8, x6
    str     x8, [x0, #216]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #112]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #224]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #224]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #232]
    adds    x8, x8, x6
    str     x8, [x0, #232]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #120]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #240]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #240]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #248]
    adds    x8, x8, x6
    str     x8, [x0, #248]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #128]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #256]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #256]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #264]
    adds    x8, x8, x6
    str     x8, [x0, #264]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #136]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #272]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #272]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #280]
    adds    x8, x8, x6
    str     x8, [x0, #280]
    adc     x6, xzr, xzr          // c = CF
    ldr     x7, [x1, #144]
    mul     x4, x7, x7
    umulh   x5, x7, x7
    ldr     x8, [x0, #288]
    adds    x7, x6, x4             // t = c + lo
    adc     x6, x5, xzr            // c = hi + γ1
    adds    x8, x8, x7             // v = dst + t
    str     x8, [x0, #288]
    adc     x6, x6, xzr            // c += γ2
    ldr     x8, [x0, #296]
    adds    x8, x8, x6
    str     x8, [x0, #296]
    ret
