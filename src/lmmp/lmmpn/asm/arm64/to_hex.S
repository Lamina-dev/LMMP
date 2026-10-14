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
// mp_size_t lmmp_to_hex_(char* dst, mp_srcptr numa, mp_size_t na, bool upper);
//
//   x0=dst, x1=numa, x2=na, w3=upper
//
// 简单标量实现（后续在 macOS 上以 NEON 优化）：输出低位在前，每字节
// 低 nibble 字符在前，与 C 版一致。字符映射用算术（免常量表）：
//   char = '0' + nib + (nib > 9 ? adj : 0), adj = 39(lower)/7(upper)
//

#include "lmmp_asm.h"

.text

.p2align 2

.globl ASM_GSYM(lmmp_to_hex_)
ASM_GSYM(lmmp_to_hex_):
    // 去值前导零：x9 = na-1（最高非零 limb 下标）
    mov          x9, x2
.Lstrip:
    cbz          x9, .Lret0
    sub          x9, x9, #1
    ldr          x10, [x1, x9, lsl #3]
    cbz          x10, .Lstrip
    add          x2, x9, #1                 // x2 = na（规范化）

    // digits = ((na-1)*64 + bitlen(top) + 3) >> 2  -> x9
    ldr          x10, [x1, x9, lsl #3]      // top
    clz          x11, x10
    mov          w12, #64
    sub          w11, w12, w11              // bitlen
    lsl          x9, x9, #6
    add          x9, x9, x11
    add          x9, x9, #3
    lsr          x9, x9, #2

    // adj = upper ? 7 : 39 -> w13
    cmp          w3, #0
    mov          w12, #39
    mov          w13, #7
    csel         w13, w13, w12, ne

    // 全 limb 段：x14 = i（0..nfull-1），x2 = nfull
    sub          x2, x2, #1
    cbz          x2, .Ltop
    mov          x14, #0
.Llimb:
    ldr          x10, [x1, x14, lsl #3]
    mov          w15, #8
.Lbyte:                                        // 每字节出 2 字符：低 nibble 在前
    and          w16, w10, #0xF
    add          w17, w16, #48
    cmp          w16, #9
    b.ls         .Lb1
    add          w17, w17, w13
.Lb1:
    strb         w17, [x0], #1
    lsr          x10, x10, #4
    and          w16, w10, #0xF
    add          w17, w16, #48
    cmp          w16, #9
    b.ls         .Lb2
    add          w17, w17, w13
.Lb2:
    strb         w17, [x0], #1
    lsr          x10, x10, #4
    subs         w15, w15, #1
    b.ne         .Lbyte
    add          x14, x14, #1
    cmp          x14, x2
    b.lo         .Llimb

.Ltop:                                         // 顶 limb 残尾（x10 重载）
    ldr          x10, [x1, x2, lsl #3]
.Ltop_pair:
    lsr          x11, x10, #4
    cbz          x11, .Ltop_one
    and          w16, w10, #0xF
    add          w17, w16, #48
    cmp          w16, #9
    b.ls         .Lt1
    add          w17, w17, w13
.Lt1:
    strb         w17, [x0], #1
    lsr          x11, x10, #4
    and          w16, w11, #0xF
    add          w17, w16, #48
    cmp          w16, #9
    b.ls         .Lt2
    add          w17, w17, w13
.Lt2:
    strb         w17, [x0], #1
    lsr          x10, x10, #8
    b            .Ltop_pair
.Ltop_one:                                     // x10 ∈ [0,15]
    cbz          x10, .Lret
    and          w16, w10, #0xF
    add          w17, w16, #48
    cmp          w16, #9
    b.ls         .Lt3
    add          w17, w17, w13
.Lt3:
    strb         w17, [x0], #1
.Lret:
    mov          x0, x9
    ret

.Lret0:
    mov          x0, #0
    ret
