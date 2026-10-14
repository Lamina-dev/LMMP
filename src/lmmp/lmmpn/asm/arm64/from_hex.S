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
// mp_size_t lmmp_from_hex_(mp_ptr dst, const char* src, mp_size_t len);
//
//   x0=dst, x1=src, x2=len
//
// 简单标量实现（后续在 macOS 上以 NEON 优化）：输入低位在前，值前导
// '0' 位于串尾先剥除；每 16 字符组装 1 limb（自高向低移位或），残尾
// 同法。nibble 取值（双大小写，与 C 版表一致；契约限合法十六进制输入）：
//   t = c - '0'；t > 9 时 (c >= 'a') ? t - 39 : t - 7
//

#include "lmmp_asm.h"

.text

.p2align 2

.globl ASM_GSYM(lmmp_from_hex_)
ASM_GSYM(lmmp_from_hex_):
    // 剥值前导零：x9 = len-1
    mov          x9, x2
.Lstrip:
    cbz          x9, .Lret0
    sub          x9, x9, #1
    ldrb         w10, [x1, x9]
    cmp          w10, #48                    // '0'
    b.eq         .Lstrip
    add          x11, x9, #1                 // x11 = len（规范化）

    lsr          x2, x11, #4                 // x2 = nfull
    mov          x9, #0                      // i
.Lmain:                                        // 每 16 字符 -> 1 limb
    cmp          x9, x2
    b.hs         .Lrem
    add          x15, x1, x9, lsl #4         // base = src + i*16
    mov          x12, #0                     // l
    mov          x13, #15                    // j（自高向低，sp[0] 最终成为最低 nibble）
.Linner:
    ldrb         w14, [x15, x13]
    sub          w16, w14, #48
    cmp          w16, #9
    b.ls         .Lnb
    cmp          w14, #96                    // 'a'-1
    b.le         .Lup
    sub          w16, w16, #39               // 'a'-'f'
    b            .Lnb
.Lup:
    sub          w16, w16, #7                // 'A'-'F'
.Lnb:
    lsl          x12, x12, #4
    orr          x12, x12, x16
    subs         x13, x13, #1
    b.ge         .Linner
    str          x12, [x0, x9, lsl #3]       // dst[i] = l
    add          x9, x9, #1
    b            .Lmain

.Lrem:                                         // 残尾：len&15 个 nibble -> 1 limb
    and          x13, x11, #15
    cbz          x13, .Ldone
    add          x15, x1, x2, lsl #4         // base = src + nfull*16
    mov          x12, #0
.Lrem_loop:
    subs         x13, x13, #1
    ldrb         w14, [x15, x13]
    sub          w16, w14, #48
    cmp          w16, #9
    b.ls         .Lrnb
    cmp          w14, #96
    b.le         .Lrup
    sub          w16, w16, #39
    b            .Lrnb
.Lrup:
    sub          w16, w16, #7
.Lrnb:
    lsl          x12, x12, #4
    orr          x12, x12, x16
    cbnz         x13, .Lrem_loop              // subs 已减完 x13（解析段的 cmp 不影响 cbnz）
    str          x12, [x0, x2, lsl #3]        // dst[nfull] = l

.Ldone:
    mov          x0, x2
    ands         x10, x11, #15
    cinc         x0, x0, ne
    ret

.Lret0:
    mov          x0, #0
    ret
