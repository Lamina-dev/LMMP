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

// void lmmp_sqrlo_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t n);
//
//   x0=dst, x1=numa, x2=n
//   ret: none
//   cal: dst[0..n) = (numa^2) mod B^n
//
//   三角分解 U^2 = 2*T + D (mod B^n)：
//     行 j 计算 T 的一片：dst[2j+1..n-1] += a_j * a[j+1..n-1-j]，
//     乘加链恰好终止于第 n-1 列，链尾进位落在第 n 列直接丢弃
//     （无 mullo 式 h 累加器）。行 0 为 mul_1 现写链（调用
//     lmmp_mul_1_，返回进位弃），行 1.. 为内联 addmul_1 累加链
//     （循环体取自 mullo_basecase.S 的 Lmo_am，寄存器角色经生产验证）。
//     末尾一趟融合的 2*dst + a_i^2 对角收尾。
//   行寻址：x19 = rp = dst + 2j + 1（每行 +16），x20 = up = numa + j + 1
//   （每行 +8），x21 = len = n - 2j - 1（每行 -2）。
//   对角收尾：全局 1 位 CF 链 + pending 寄存器 x10 = 对角高半 + 存位
//   ≤ B-1（每次加法的操作数均已归一化，单 CF 足够）；arm64 的
//   mul/umulh/add(无 s) 均不触标志，比 x64 版少一次 setc 保存。
//   n ≤ 3 免帧快速路径：直算各列（列内多步加法均 mod B 累加，
//   出格进位丢弃），避免行 0 的 mul_1_ 调用与寄存器保存开销。

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_sqrlo_basecase_)
.p2align 2

ASM_GSYM(lmmp_sqrlo_basecase_):
    cmp     x2, #3
    b.hi    Lsq_big                   // n >= 4：通用路径
    cmp     x2, #1
    b.hi    Lsq_n23
    ldr     x4, [x1]                  // n == 1: dst[0] = lo(a0^2)
    mul     x5, x4, x4
    str     x5, [x0]
    ret

Lsq_n23:                              // n == 2: lo(a0^2) | (2*a0*a1 + hi(a0^2)) mod B
    cmp     x2, #2
    b.hi    Lsq_n3
    ldr     x4, [x1]
    ldr     x5, [x1, #8]
    mul     x6, x4, x4                // lo(a0^2)
    umulh   x7, x4, x4                // hi(a0^2)
    mul     x8, x4, x5                // lo(a0*a1)
    str     x6, [x0]
    adds    x8, x8, x8                // 2*lo（CF 弃，mod B 累加）
    add     x8, x8, x7                // + hi（列 2 出格进位弃）
    str     x8, [x0, #8]
    ret

Lsq_n3:                               // n == 3: 列 j 多步加法均 mod B，出格弃
    ldr     x4, [x1]
    ldr     x5, [x1, #8]
    ldr     x6, [x1, #16]
    mul     x12, x4, x4               // lo(a0^2)
    umulh   x11, x4, x4               // hi(a0^2)
    mul     x9, x4, x5                // lo(a0*a1)
    umulh   x10, x4, x5               // hi(a0*a1)
    mul     x7, x4, x6                // lo(a0*a2)
    mul     x8, x5, x5                // lo(a1^2)
    str     x12, [x0]                 // dst[0]
    adds    x9, x9, x9                // 2*lo01 mod B，CF = c1
    cset    x13, cs
    adds    x9, x9, x11               // dst[1] = (2*lo01 + hi00) mod B，CF = c2
    adc     x13, x13, xzr             // 列间进位 = c1 + c2 ≤ 2
    str     x9, [x0, #8]
    adds    x7, x7, x10               // t = lo02 + hi01（CF 弃）
    adds    x7, x7, x7                // 2t（CF 弃）
    add     x7, x7, x13               // + 列间进位（CF 弃）
    add     x8, x7, x8                // + lo(a1^2)（列 3 出格进位弃）
    str     x8, [x0, #16]
    ret

Lsq_big:
    stp     x26, x25, [sp, #-80]!     // save regs
    stp     x24, x23, [sp, #16]
    stp     x22, x21, [sp, #32]
    stp     x20, x19, [sp, #48]
    stp     x29, x30, [sp, #64]
    add     x29, sp, #64
    mov     x22, x1                   // x22 = numa
    mov     x23, x0                   // x23 = dst
    mov     x24, x2                   // x24 = n
    ldr     x3, [x22]                 // a0
    add     x0, x23, #8               // dst + 1
    add     x1, x22, #8               // numa + 1
    sub     x2, x24, #1               // n - 1
    bl      ASM_GSYM(lmmp_mul_1_)     // dst[1..n-1] = a0 * a[1..n-1]，进位弃
    add     x19, x23, #24             // rp = dst + 3（行 1 窗口）
    add     x20, x22, #16             // up = numa + 2
    sub     x21, x24, #3              // len = n - 3
    cmp     x21, #1
    b.lt    Lsq_diag                  // n <= 3：无行 1..
.p2align 4

Lsq_row:                              // dst[2j+1..n-1] += a_j * a[j+1..n-1-j]
    ldr     x3, [x20, #-8]            // v = a_j
    mov     x0, x19                   // rp
    mov     x1, x20                   // up
    mov     x2, x21                   // len
    adds    x15, xzr, xzr             // carry = 0, C = 0
    tbz     x2, #0, Lsq_bx0
    ldr     x4, [x1], #8              // leading single（len 奇）
    mul     x8, x4, x3                // lo_0
    umulh   x12, x4, x3               // hi_0
    ldr     x4, [x0]
    adds    x8, x4, x8                // s_0 = rp[0] + lo_0
    cinc    x15, x12, cs              // carry = hi_0 + C
    str     x8, [x0], #8

Lsq_bx0:                              // leading pair（len ≡ 2,3 mod 4）
    tbz     x2, #1, Lsq_blk
    ldp     x4, x5, [x1], #16
    mul     x8, x4, x3                // lo_0
    umulh   x12, x4, x3               // hi_0
    mul     x9, x5, x3                // lo_1
    umulh   x13, x5, x3               // hi_1
    adds    x8, x8, x15               // s_0 = lo_0 + carry
    adcs    x9, x9, x12               // s_1 = lo_1 + hi_0 + C
    ldp     x4, x5, [x0]              // rp[0], rp[1]
    adc     x15, x13, xzr             // carry = hi_1 + C
    adds    x8, x4, x8                // s_0 += rp[0]
    adcs    x9, x5, x9                // s_1 += rp[1]
    cinc    x15, x15, cs              // carry += C
    stp     x8, x9, [x0], #16

Lsq_blk:
    lsr     x2, x2, #2                // 4-limb 块数
    cbz     x2, Lsq_row_end
    ldp     x4, x5, [x1], #32         // prefetch up[0..3]
    ldp     x6, x7, [x1, #-16]
    b       Lsq_mid
.p2align 4

Lsq_top:                              // s_j += rp[j]
    ldp     x4, x5, [x1], #32
    ldp     x6, x7, [x1, #-16]
    adds    x8, x16, x8               // s_0 += rp[0]
    adcs    x9, x17, x9               // s_1 += rp[1]
    stp     x8, x9, [x0], #32
    adcs    x10, x12, x10             // s_2 += rp[2]
    adcs    x11, x13, x11             // s_3 += rp[3]
    stp     x10, x11, [x0, #-16]
    cinc    x15, x15, cs              // carry += C

Lsq_mid:                              // products, carry chain
    sub     x2, x2, #1
    mul     x8, x4, x3                // lo_0
    umulh   x12, x4, x3               // hi_0
    mul     x9, x5, x3                // lo_1
    umulh   x13, x5, x3               // hi_1
    adds    x8, x8, x15               // s_0 = lo_0 + carry
    mul     x10, x6, x3               // lo_2
    umulh   x14, x6, x3               // hi_2
    adcs    x9, x9, x12               // s_1 = lo_1 + hi_0 + C
    mul     x11, x7, x3               // lo_3
    umulh   x15, x7, x3               // hi_3
    adcs    x10, x10, x13             // s_2 = lo_2 + hi_1 + C
    ldp     x16, x17, [x0]            // rp[0], rp[1]
    adcs    x11, x11, x14             // s_3 = lo_3 + hi_2 + C
    ldp     x12, x13, [x0, #16]       // rp[2], rp[3]
    adc     x15, x15, xzr             // carry = hi_3 + C
    cbnz    x2, Lsq_top
    adds    x8, x16, x8               // last: s_j += rp[j]
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    stp     x8, x9, [x0]
    stp     x10, x11, [x0, #16]
    // 链尾进位落第 n 列，丢弃

Lsq_row_end:
    add     x19, x19, #16             // rp += 2 limb
    add     x20, x20, #8              // up += 1 limb
    subs    x21, x21, #2              // len -= 2
    b.gt    Lsq_row

// ======================================
// 对角收尾：dst = 2*dst + sum a_i^2*B^2i，截断于 n
//   奇列仅 2d + 链 CF + pending；偶列另有对角平方 lo（CF 出链），
//   pending' = 对角 hi + 偶列倍增加位 ≤ B-1。
// ======================================
Lsq_diag:
    ldr     x4, [x22]                 // a0
    mul     x5, x4, x4                // lo(a0^2)
    umulh   x10, x4, x4               // pending = hi(a0^2) ≤ B-2
    str     x5, [x23]                 // dst[0]（T[0] 恒 0）
    add     x0, x23, #8               // &dst[1]
    add     x1, x22, #8               // &a[1]
    sub     x2, x24, #1
    lsr     x2, x2, #1                // 对数 = (n-1)/2
    adds    xzr, xzr, xzr             // CF = 0
    cbz     x2, Lsq_dtail
.p2align 4

Lsq_dpair:                            // 奇列（无对角）+ 偶列（对角 a）
    ldr     x4, [x1], #8              // 对角元 a
    ldr     x8, [x0]
    ldr     x9, [x0, #8]
    mul     x5, x4, x4                // lo(a^2)（不触标志）
    umulh   x6, x4, x4                // hi(a^2) ≤ B-2
    adcs    x8, x8, x8                // 奇列 = 2d + 链 CF，CF = c1
    adcs    x9, x9, x9                // 偶列 = 2d + c1，CF = c2
    cset    x7, cs                    // 存 c2（随后 adds 改 CF）
    adds    x8, x8, x10               // 奇列 += pending，CF = c3
    adcs    x9, x9, x5                // 偶列 += lo + c3，CF = 新链
    add     x10, x6, x7               // pending' = hi + c2（不触标志）
    stp     x8, x9, [x0], #16
    sub     x2, x2, #1                // 不触标志
    cbnz    x2, Lsq_dpair

Lsq_dtail:                            // n 偶：末尾孤奇列
    tbnz    x24, #0, Lsq_done         // n 奇：无
    ldr     x8, [x0]
    adcs    x8, x8, x8                // 2d + 链 CF
    add     x8, x8, x10               // + pending，出格进位丢弃
    str     x8, [x0]

Lsq_done:
    ldp     x29, x30, [sp, #64]       // restore regs, ret
    ldp     x20, x19, [sp, #48]
    ldp     x22, x21, [sp, #32]
    ldp     x24, x23, [sp, #16]
    ldp     x26, x25, [sp], #80
    ret
