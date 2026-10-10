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

// mp_limb_t lmmp_shl_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl, mp_limb_t c);
// mp_limb_t lmmp_shl_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl);
//
//   x0=dst, x1=numa, x2=na, x3=shl, x4=c (only c_)
//   ret x0 = numa[na-1] >> (64 - shl)
//   cal: dst[i] = (numa[i] << shl) | (numa[i-1] >> (64-shl))

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_shl_c_)
.p2align 2

ASM_GSYM(lmmp_shl_c_):

Lshl_ent:
    cbz     x3, Lshl_copy           // shl == 0, copy
    sub     x7, x3, #64              // k - 64（负 = 右移 s）
    neg     x5, x7                   // x5 = s = 64 - shl
    dup     v0.2d, x3                // 左移量 k
    dup     v1.2d, x7                // 右移量 -(64-k)
    dup     v2.2d, x4                // lo_prev = [c, c]
    lsr     x8, x2, #2               // 4-limb 块数
    and     x9, x2, #3               // 余 limb 数
    cbz     x8, Lshl_pair
.p2align 4

Lshl_v4:                             // 主循环：4 limb/迭代
    ldr     q3, [x1], #16            // A = [a_i, a_{i+1}]
    ldr     q4, [x1], #16            // B = [a_{i+2}, a_{i+3}]
    ushl    v5.2d, v3.2d, v0.2d      // hiA = A << k
    ushl    v6.2d, v3.2d, v1.2d      // loA = A >> s
    ext     v3.16b, v2.16b, v6.16b, #8 // [lo_prev[1], loA[0]]
    ushl    v7.2d, v4.2d, v0.2d      // hiB
    ushl    v4.2d, v4.2d, v1.2d      // loB（B 原值已耗尽，原地）
    orr     v5.16b, v5.16b, v3.16b   // outA
    ext     v3.16b, v6.16b, v4.16b, #8 // [loA[1], loB[0]]
    str     q5, [x0], #16
    orr     v7.16b, v7.16b, v3.16b   // outB
    str     q7, [x0], #16
    orr     v2.16b, v4.16b, v4.16b   // lo_prev = loB
    subs    x8, x8, #1
    b.ne    Lshl_v4

Lshl_pair:                           // 余一对（2 limb）
    tbz     x9, #1, Lshl_single
    ldr     q3, [x1], #16
    ushl    v5.2d, v3.2d, v0.2d      // hi
    ushl    v6.2d, v3.2d, v1.2d      // lo
    ext     v3.16b, v2.16b, v6.16b, #8
    orr     v5.16b, v5.16b, v3.16b
    str     q5, [x0], #16
    orr     v2.16b, v6.16b, v6.16b   // lo_prev = lo

Lshl_single:                         // 余一 limb
    tbz     x9, #0, Lshl_ret
    ldr     x6, [x1]                 // a[n-1]
    umov    x7, v2.d[1]              // a[n-2] >> s（n==1 时为 c）
    lsr     x10, x6, x5              // ret = a[n-1] >> s（暂存 x10）
    lsl     x6, x6, x3
    orr     x6, x6, x7
    str     x6, [x0]
    mov     x0, x10
    ret

Lshl_ret:
    umov    x0, v2.d[1]              // ret = a[n-1] >> s = lo_prev[1]
    ret

.globl ASM_GSYM(lmmp_shl_)
.p2align 2

ASM_GSYM(lmmp_shl_):
    mov     x4, xzr                  // c = 0
    b       Lshl_ent

Lshl_copy:                           // shl == 0: copy, ret 0
    cmp     x0, x1
    b.eq    Lshl_copy_done
    lsr     x7, x2, #2
    cbz     x7, Lshl_copy_tail
.p2align 4

Lshl_copy_loop:
    ldr     q3, [x1], #16
    str     q3, [x0], #16
    ldr     q4, [x1], #16
    str     q4, [x0], #16
    subs    x7, x7, #1
    b.ne    Lshl_copy_loop

Lshl_copy_tail:
    ands    x7, x2, #3
    b.eq    Lshl_copy_done

Lshl_copy_tail_loop:
    ldr     x6, [x1], #8
    str     x6, [x0], #8
    subs    x7, x7, #1
    b.ne    Lshl_copy_tail_loop

Lshl_copy_done:
    mov     x0, xzr
    ret
