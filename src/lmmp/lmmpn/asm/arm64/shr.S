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

// mp_limb_t lmmp_shr_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c);
// mp_limb_t lmmp_shr_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr);
//
//   x0=dst, x1=numa, x2=na, x3=shr, x4=c (only c_)
//   ret x0 = numa[0] << (64 - shr)
//   cal: dst[i] = (numa[i] >> shr) | (numa[i+1] << (64-shr))

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_shr_c_)
.p2align 2

ASM_GSYM(lmmp_shr_c_):

Lshr_ent:
    cbz     x3, Lshr_copy            // shr == 0, copy
    mov     x5, #64
    sub     x5, x5, x3               // x5 = s = 64 - shr
    neg     x7, x3                   // -shr
    dup     v0.2d, x7                // 右移量 -k
    dup     v1.2d, x5                // 左移量 +s
    ldr     x6, [x1]                 // a[0]
    lsl     x6, x6, x5               // ret = a[0] << s（左对齐，常驻 x6）
    tbz     x2, #0, Lshr_even        // n 偶

    // n 奇：剥顶孤 limb —— 只算不写（维持升序写序，dst<numa 部分
    // 重叠安全）：fin = a[n-1]<<s，存值 a[n-1]>>k|c 延迟到全部对之后
    add     x10, x1, x2, lsl #3
    ldr     x11, [x10, #-8]          // a[n-1]
    lsl     x7, x11, x5
    lsr     x11, x11, x3
    orr     x11, x11, x4             // 延迟存值（x11 常驻至收尾）
    dup     v2.2d, x7                // fin = a[n-1] << s
    b       Lshr_pro

Lshr_even:
    dup     v2.2d, x4                // fin = c（or 进末对高位 lane）

Lshr_pro:
    lsr     x8, x2, #1               // P = 对数
    cbz     x8, Lshr_done            // n == 1：剥顶已覆盖
    ldr     q5, [x1], #16            // p0
    ushl    v4.2d, v5.2d, v1.2d      // LA
    ushl    v3.2d, v5.2d, v0.2d      // RA
    sub     x9, x8, #1
    lsr     x9, x9, #1               // full = (P-1)/2 完整迭代数
    cbz     x9, Lshr_left
.p2align 4

Lshr_v4:                             // 主循环：载 B/C 两对，发 outA/outB
    ldr     q5, [x1], #16            // B
    ushl    v6.2d, v5.2d, v1.2d      // LB
    ushl    v7.2d, v5.2d, v0.2d      // RB
    ext     v5.16b, v4.16b, v6.16b, #8
    orr     v5.16b, v3.16b, v5.16b   // outA = RA | mix
    str     q5, [x0], #16
    ldr     q5, [x1], #16            // C（前视）
    ushl    v4.2d, v5.2d, v1.2d      // LC（新 LA）
    ushl    v3.2d, v5.2d, v0.2d      // RC（新 RA）
    ext     v5.16b, v6.16b, v4.16b, #8 // [LB[1], LC[0]]
    orr     v5.16b, v7.16b, v5.16b   // outB = RB | mix
    str     q5, [x0], #16
    subs    x9, x9, #1
    b.ne    Lshr_v4

Lshr_left:
    tbnz    x8, #0, Lshr_fin         // P 奇：A 已是末对
    // P 偶：余一对，先发射（用其余对 L），再以末对状态进 fin
    ldr     q7, [x1]                 // 末对 raw
    ushl    v6.2d, v7.2d, v1.2d      // L_last
    ext     v5.16b, v4.16b, v6.16b, #8
    orr     v5.16b, v3.16b, v5.16b   // out = RA | [LA[1], L_last[0]]
    str     q5, [x0], #16
    ushl    v3.2d, v7.2d, v0.2d      // R_last
    mov     v4.16b, v6.16b           // LA = L_last

Lshr_fin:                            // 末对：out = RA | [LA[1], fin[0]]
    ext     v5.16b, v4.16b, v2.16b, #8
    orr     v5.16b, v3.16b, v5.16b
    str     q5, [x0], #16

Lshr_done:                           // 奇 n：x0 恰指向 dst[n-1]（2P = n-1）
    tbz     x2, #0, Lshr_ret
    str     x11, [x0]                // 延迟的顶 limb 存储
Lshr_ret:
    mov     x0, x6
    ret

.globl ASM_GSYM(lmmp_shr_)
.p2align 2

ASM_GSYM(lmmp_shr_):
    mov     x4, xzr                  // c = 0
    b       Lshr_ent

Lshr_copy:                           // shr == 0: copy, ret 0
    cmp     x0, x1
    b.eq    Lshr_copy_done
    lsr     x7, x2, #2
    cbz     x7, Lshr_copy_tail
.p2align 4

Lshr_copy_loop:
    ldr     q3, [x1], #16
    str     q3, [x0], #16
    ldr     q4, [x1], #16
    str     q4, [x0], #16
    subs    x7, x7, #1
    b.ne    Lshr_copy_loop

Lshr_copy_tail:
    ands    x7, x2, #3
    b.eq    Lshr_copy_done

Lshr_copy_tail_loop:
    ldr     x6, [x1], #8
    str     x6, [x0], #8
    subs    x7, x7, #1
    b.ne    Lshr_copy_tail_loop

Lshr_copy_done:
    mov     x0, xzr
    ret
