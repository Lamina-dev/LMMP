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
// mp_limb_t lmmp_add_n_sub_n_(mp_ptr dsta, mp_ptr dstb, mp_srcptr numa, mp_srcptr numb,
//                             mp_size_t n);
//
//   x0=dsta, x1=dstb, x2=numa, x3=numb, x4=n
//   ret x0 = 2*carry + borrow
//
//   cal: [dsta,n] = numa + numb, [dstb,n] = numa - numb
//
//   简单占位版本: ARM64 仅有一组 C 标志, 无法像 x64 (adcx/adox) 那样
//   双链并行, 故每肢交错计算加减, 进位/借位以寄存器传递:
//     和: s = a + b + cAdd,  cAdd' = c1 + c2   (c2 仅在 a+b=B-1 且
//         cAdd=1 时为 1, 与 c1 互斥)
//     差: d = a - b - bSub,  bSub' = b1 + b2   (b2 仅在 a=b 且
//         bSub=1 时为 1, 与 b1 互斥)
//   eqsep(dsta|dstb, numa|numb) 重叠安全: 每肢两加载先于两存储。
//   计数用 sub/cbnz (不带 s, 不动标志)。后续可改为更优的交错调度。
//

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_add_n_sub_n_)
.p2align 2

ASM_GSYM(lmmp_add_n_sub_n_):
    mov     x12, #0                // cAdd = 0
    mov     x13, #0                // bSub = 0

Lanss_loop:
    ldr     x5, [x2], #8           // x5 = a
    ldr     x6, [x3], #8           // x6 = b

    // 和链
    adds    x7, x5, x6             // C = c1
    adc     x9, xzr, xzr           // x9 = c1
    adds    x7, x7, x12            // + cAdd(prev), C = c2
    adc     x10, xzr, xzr          // x10 = c2
    add     x12, x9, x10           // cAdd' = c1 + c2 <= 1

    // 差链
    subs    x8, x5, x6             // C = !b1
    cset    x9, cc                 // x9 = b1 (a < b)
    subs    x8, x8, x13            // - bSub(prev), C = !b2
    cset    x10, cc                // x10 = b2 (a == b 且 bSub == 1)
    add     x13, x9, x10           // bSub' = b1 + b2 <= 1

    str     x7, [x0], #8
    str     x8, [x1], #8
    sub     x4, x4, #1
    cbnz    x4, Lanss_loop

    add     x0, x13, x12, lsl #1   // ret = 2*cAdd + bSub
    ret
