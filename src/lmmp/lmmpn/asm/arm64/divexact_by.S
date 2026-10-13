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

//
// void lmmp_divexact_by3_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
// void lmmp_divexact_by9_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
// void lmmp_divexact_by15_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
//
//   cal: Hensel 精确除法, 每 limb:
//            l = s - c,  q = l * INV_k mod B,  c' = borrow(l) + floor(k*q/B)
//        三个函数的核心循环结构不同 (进位计算恒等式各异), 勿强行共享。
//        恒等式 (B = 2^64), 均以单次 cmp 收尾, 避开 umulh 的乘法管线串行:
//            floor(3q/B)  = [q>=2^63] + [l<q]
//                证: h=0 时 l=3q>=q; h=1 时 l=3q-B, [l<q] <=> 2q<B,
//                    与 [q>=2^63] 恰好互补各贡献 1; h=2 时 l=3q-2B<q 恒真
//            floor(9q/B)  = (q>>61) + [l<q]
//                证: t+q=9q≡l (mod B), carry(t+q) ⟺ t=(l-q)mod B 回绕 ⟺ q>l
//            floor(15q/B) = (q>>60) - [(q<<4)<q]   (16q-q 的借位)
//            floor(15q/B) 另一等价实现: w = l+q (=16q mod B),
//                c' = borrow + (q>>60) - [w<q], 经 cinc+sbc 折叠 (同 clang)
//   regs: x0=dst, x1=numa, x2=na, x8=INV_k, x9/x3/x4=s, x10=c,
//         x12=l, x14=q, x15=移位结果, x16=by15 的 w, x17=余数段计数器
//   note: c 值域 by3<=3, by9<=9, by15<=15; 借位由 subs 后首个条件指令
//         cinc(...,lo) 直接吸收 (标志位经 mul/str/lsr 存活), 其后的 cmp
//         才切换标志位供末条 cinc/sbc 使用, 三者顺序不可调换。
//

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_divexact_by3_
// ======================================
.p2align 2
.globl ASM_GSYM(lmmp_divexact_by3_)
ASM_GSYM(lmmp_divexact_by3_):
    movz    x8, #0xAAAA, lsl #48
    movk    x8, #0xAAAA, lsl #32        // INV3 = 3^(-1) mod B
    movk    x8, #0xAAAA, lsl #16
    movk    x8, #0xAAAB
    mov     x10, #0                     // c = 0
    ands    x17, x2, #3                 // 余数 limb 先行
    b.eq    Lby3_groups

Lby3_scalar:
    ldr     x9, [x1], #8                // s
    subs    x12, x9, x10                // l = s - c, C = !borrow
    mul     x14, x12, x8                // q = l * INV3
    str     x14, [x0], #8
    lsr     x15, x14, #63               // u = [q >= 2^63]
    cinc    x15, x15, lo                // u + borrow (标志位来自 subs)
    cmp     x12, x14                    // lo = [l < q]
    cinc    x10, x15, lo                // c' = borrow + u + [l < q]
    subs    x17, x17, #1
    b.ne    Lby3_scalar

Lby3_groups:
    lsr     x2, x2, #2                  // 4 limbs per group
    cbz     x2, Lby3_ret

.p2align 2
Lby3_loop:
    ldp     x3, x4, [x1]                // s0, s1
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0]
    lsr     x15, x14, #63
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #8]
    lsr     x15, x14, #63
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    ldp     x3, x4, [x1, #16]           // s2, s3
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0, #16]
    lsr     x15, x14, #63
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #24]
    lsr     x15, x14, #63
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    add     x1, x1, #32
    add     x0, x0, #32
    subs    x2, x2, #1
    b.ne    Lby3_loop

Lby3_ret:
    ret

// ======================================
// lmmp_divexact_by9_
// ======================================
.p2align 2
.globl ASM_GSYM(lmmp_divexact_by9_)
ASM_GSYM(lmmp_divexact_by9_):
    movz    x8, #0x8E38, lsl #48
    movk    x8, #0xE38E, lsl #32        // INV9 = 9^(-1) mod B
    movk    x8, #0x38E3, lsl #16
    movk    x8, #0x8E39
    mov     x10, #0
    ands    x17, x2, #3
    b.eq    Lby9_groups

Lby9_scalar:
    ldr     x9, [x1], #8
    subs    x12, x9, x10
    mul     x14, x12, x8
    str     x14, [x0], #8
    lsr     x15, x14, #61               // hi = q >> 61
    cinc    x15, x15, lo                // hi + borrow
    cmp     x12, x14                    // lo = [l < q]
    cinc    x10, x15, lo
    subs    x17, x17, #1
    b.ne    Lby9_scalar

Lby9_groups:
    lsr     x2, x2, #2
    cbz     x2, Lby9_ret

.p2align 2
Lby9_loop:
    ldp     x3, x4, [x1]
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0]
    lsr     x15, x14, #61
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #8]
    lsr     x15, x14, #61
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    ldp     x3, x4, [x1, #16]
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0, #16]
    lsr     x15, x14, #61
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #24]
    lsr     x15, x14, #61
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    add     x1, x1, #32
    add     x0, x0, #32
    subs    x2, x2, #1
    b.ne    Lby9_loop

Lby9_ret:
    ret

// ======================================
// lmmp_divexact_by15_
// ======================================
.p2align 2
.globl ASM_GSYM(lmmp_divexact_by15_)
ASM_GSYM(lmmp_divexact_by15_):
    movz    x8, #0xEEEE, lsl #48
    movk    x8, #0xEEEE, lsl #32        // INV15 = 15^(-1) mod B
    movk    x8, #0xEEEE, lsl #16
    movk    x8, #0xEEEF
    mov     x10, #0
    ands    x17, x2, #3
    b.eq    Lby15_groups

Lby15_scalar:
    ldr     x9, [x1], #8
    subs    x12, x9, x10                // l = s - c, C = !borrow
    mul     x14, x12, x8                // q = l * INV15
    str     x14, [x0], #8
    add     x16, x12, x14               // w = l + q = 16q mod B
    lsr     x15, x14, #60               // hi = q >> 60
    cinc    x15, x15, lo                // hi + borrow (标志位来自 subs)
    cmp     x16, x14                    // C = [w >= q]
    sbc     x10, x15, xzr               // c' = hi + borrow - [w < q]
    subs    x17, x17, #1
    b.ne    Lby15_scalar

Lby15_groups:
    lsr     x2, x2, #2
    cbz     x2, Lby15_ret

.p2align 2
Lby15_loop:
    ldp     x3, x4, [x1]
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0]
    add     x16, x12, x14               // w = l + q
    lsr     x15, x14, #60
    cinc    x15, x15, lo
    cmp     x16, x14
    sbc     x10, x15, xzr

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #8]
    add     x16, x12, x14               // w = l + q
    lsr     x15, x14, #60
    cinc    x15, x15, lo
    cmp     x16, x14
    sbc     x10, x15, xzr

    ldp     x3, x4, [x1, #16]
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0, #16]
    add     x16, x12, x14               // w = l + q
    lsr     x15, x14, #60
    cinc    x15, x15, lo
    cmp     x16, x14
    sbc     x10, x15, xzr

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #24]
    add     x16, x12, x14               // w = l + q
    lsr     x15, x14, #60
    cinc    x15, x15, lo
    cmp     x16, x14
    sbc     x10, x15, xzr

    add     x1, x1, #32
    add     x0, x0, #32
    subs    x2, x2, #1
    b.ne    Lby15_loop

Lby15_ret:
    ret
