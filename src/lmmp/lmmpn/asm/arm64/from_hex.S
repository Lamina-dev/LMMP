//  Copyright (C) 2026 HJimmyK (Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it
//  under the terms of the GNU Lesser General Public License as
//  published by the Free Software Foundation; either version 3 of
//  the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, see
//  <https://www.gnu.org/licenses/>.
//
//  SPDX-License-Identifier: LGPL-3.0-or-later
//

//
// mp_size_t lmmp_from_hex_(mp_ptr dst, const char* src, mp_size_t len);
//
//   x0=dst, x1=src, x2=len
//
// 输入为低位在前的十六进制串（值前导 '0' 位于串尾，先剥除）。主循环每次
// 32 字符 -> 2 limb（NEON 全字节 lane 算术解析，无查表）：
//   t = c - 48；letter = t >u 9；lower = c >=s 'a'（契约限合法输入）
//   nib = t - 7*letter - 32*lower                （'A'-'F' 补 -7、'a'-'f' 补 -39）
//   limb 字节 k = nib[2k] | nib[2k+1]<<4，由 uzp1/uzp2(nib,nib) 取偶/奇
//   字节 lane 后 shl#4 + orr 拼出（uzp 结果高半为重复填充，仅存低 8 字节；
//   替代 x64 的 maddubs 相邻对打包）。残尾（len&15 个 nibble）走标量。
//
// 寄存器约定：x9=残尾 nibble 数，x10=nfull，x11=返回值（入口即定，
// 免标量尾再计），x2=对计数；常量 v2=48/v4=9/v6=97/v24=7/v25=32 循环外
// 常驻（向量 and 无立即数形式），数据与临时用 v16-v23（调用者易失，
// 不触 d8-d15 被调保存区），零栈帧。
//

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_from_hex_)
ASM_GSYM(lmmp_from_hex_):
    // 剥值前导零（串尾的 '0'）：x9 = len-1
    mov          x9, x2
.Lstrip:
    cbz          x9, .Lret0
    sub          x9, x9, #1
    ldrb         w12, [x1, x9]
    cmp          w12, #48                    // '0'
    b.eq         .Lstrip
    add          x11, x9, #1                 // x11 = len（规范化）

    // x10 = nfull，x9 = len&15（残尾 nibble 数，须从规范化 len 取），x11 = 返回值
    lsr          x10, x11, #4
    and          x9, x11, #15
    cmp          x9, #0
    cinc         x11, x10, ne

    movi         v2.16b, #48
    movi         v4.16b, #9
    movi         v6.16b, #97
    movi         v24.16b, #7
    movi         v25.16b, #32

    lsr          x2, x10, #1                 // 对计数
    cbz          x2, .Lsingle

.p2align 4
.Lmain:                                        // 32 chars -> 2 limbs
    ldp          q16, q17, [x1], #32
    sub          v18.16b, v16.16b, v2.16b     // t
    sub          v19.16b, v17.16b, v2.16b
    cmhi         v20.16b, v18.16b, v4.16b     // letter
    cmhi         v21.16b, v19.16b, v4.16b
    cmge         v22.16b, v16.16b, v6.16b     // lower（v16/v17 原字符仍在）
    cmge         v23.16b, v17.16b, v6.16b
    and          v20.16b, v20.16b, v24.16b
    and          v21.16b, v21.16b, v24.16b
    and          v22.16b, v22.16b, v25.16b
    and          v23.16b, v23.16b, v25.16b
    sub          v18.16b, v18.16b, v20.16b
    sub          v19.16b, v19.16b, v21.16b
    sub          v18.16b, v18.16b, v22.16b    // nib1
    sub          v19.16b, v19.16b, v23.16b    // nib2
    uzp1         v20.16b, v18.16b, v18.16b    // 偶字节 nibble
    uzp2         v21.16b, v18.16b, v18.16b    // 奇字节 nibble
    shl          v21.16b, v21.16b, #4
    orr          v20.16b, v20.16b, v21.16b    // limb1（低 8 字节）
    uzp1         v22.16b, v19.16b, v19.16b
    uzp2         v23.16b, v19.16b, v19.16b
    shl          v23.16b, v23.16b, #4
    orr          v22.16b, v22.16b, v23.16b    // limb2
    stp          d20, d22, [x0], #16
    subs         x2, x2, #1
    b.ne         .Lmain

.Lsingle:                                      // 落单的整 limb（16 chars）
    tbz          x10, #0, .Lrem
    ldr          q16, [x1], #16
    sub          v17.16b, v16.16b, v2.16b
    cmhi         v18.16b, v17.16b, v4.16b
    cmge         v19.16b, v16.16b, v6.16b
    and          v18.16b, v18.16b, v24.16b
    and          v19.16b, v19.16b, v25.16b
    sub          v17.16b, v17.16b, v18.16b
    sub          v17.16b, v17.16b, v19.16b
    uzp1         v18.16b, v17.16b, v17.16b
    uzp2         v19.16b, v17.16b, v17.16b
    shl          v19.16b, v19.16b, #4
    orr          v18.16b, v18.16b, v19.16b
    str          d18, [x0], #8

.Lrem:                                         // 残尾：x9 个 nibble -> 1 limb
    cbz          x9, .Ldone
    mov          x12, #0
.Lrem_loop:
    subs         x9, x9, #1
    ldrb         w13, [x1, x9]                // 自高向低（sp[0] 为最低 nibble）
    sub          w14, w13, #48
    cmp          w14, #9
    b.ls         .Lrnb
    cmp          w13, #96
    b.le         .Lrup
    sub          w14, w14, #39                // 'a'-'f'
    b            .Lrnb
.Lrup:
    sub          w14, w14, #7                 // 'A'-'F'
.Lrnb:
    lsl          x12, x12, #4
    orr          x12, x12, x14
    cbnz         x9, .Lrem_loop
    str          x12, [x0]

.Ldone:
    mov          x0, x11
    ret

.Lret0:
    mov          x0, #0
    ret
