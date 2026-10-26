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
//   Windows: dst=rcx, src=rdx, len=r8
//   SysV:    dst=rdi, src=rsi, len=rdx
//
// 输入为低位在前的十六进制串（值前导 '0' 位于串尾，先剥除）。主循环每次
// 16 字符 -> 1 limb（xmm，全 VEX 编码不染上半 ymm 状态）：
//   n0 = c - '0'；gt9 = n0 >s 9（字母）；islo = c >s '`'
//   letter = blendv(c - ('A'-10), c - ('a'-10), islo)   （双大小写）
//   nib = blendv(letter, n0, ~gt9) & 0x0F
//   byte = maddubs(nib, {1,16,...})                     （相邻对：hi*16+lo）
// 残尾（len&15 个 nibble）走标量。契约同 C 版：输入全为合法十六进制字符。
//
// 寄存器约定：r11=len（规范化），r10=迭代计数，r9=残尾数；r8/rdx 仅在
// 其参数职责（len/src 游走）耗尽后作暂存；xmm0-5 双 ABI 均为易失。
//

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_from_hex_)
ASM_GSYM(lmmp_from_hex_):
    // 剥值前导零（串尾的 '0'）：rax = len-1
    mov          rax, rx2
.Lstrip:
    test         rax, rax
    jz           .Lret0
    dec          rax
    cmp          byte ptr [rx1 + rax], 0x30
    je           .Lstrip
    lea          r11, [rax + 1]              // r11 = len（规范化）

    shr          r11, 4                      // r11 = nfull（迭代数）
    // 残尾数 r9 = len & 15
    lea          r9, [rax + 1]               // len
    and          r9, 15
    mov          r10, r11
    test         r10, r10
    jz           .Lrem

.p2align 4
.Lmain:                                        // 16 chars -> 1 limb
    vmovdqu      xmm0, [rx1]
    vpsubb       xmm1, xmm0, [rip + .Lc48]    // n0 = c - '0'
    vpcmpgtb     xmm2, xmm1, [rip + .Lc9]     // gt9（字母路径）
    vpcmpgtb     xmm3, xmm0, [rip + .Lc60]    // c > '`'（小写）
    vpsubb       xmm4, xmm0, [rip + .Lc55]    // c - ('A'-10)
    vpsubb       xmm5, xmm0, [rip + .Lc87]    // c - ('a'-10)
    vpblendvb    xmm4, xmm4, xmm5, xmm3       // letter（大小写择一）
    vpxor        xmm2, xmm2, [rip + .Lones]   // isdig = ~gt9
    vpblendvb    xmm1, xmm4, xmm1, xmm2       // nib
    vpand        xmm1, xmm1, [rip + .Lc0f]
    vpmaddubsw   xmm1, xmm1, [rip + .Lpat]    // 相邻对打包为字节
    vpackuswb    xmm1, xmm1, xmm1
    vmovq        [rx0], xmm1                  // limb
    add          rx1, 16
    add          rx0, 8
    dec          r10
    jnz          .Lmain

.Lrem:                                         // 残尾：r9 个 nibble -> 1 limb
    test         r9, r9
    jz           .Ldone
    mov          r8, r9                       // r8 = 剩余计数（亦作索引）
    xor          r10, r10                     // l（r10 计数职责已尽）
.Lrem_loop:
    dec          r8
    movzx        eax, byte ptr [rx1 + r8]
    shl          r10, 4
    sub          eax, 48                      // t = c - '0'
    cmp          eax, 9
    jbe          .Lrem_nb
    cmp          eax, 22                      // 大写 t∈[17,22]，小写 t∈[49,54]
    jg           .Lrem_low
    sub          eax, 7                       // 'A'-'F'
    jmp          .Lrem_nb
.Lrem_low:
    sub          eax, 39                      // 'a'-'f'
.Lrem_nb:
    or           r10, rax
    test         r8, r8
    jnz          .Lrem_loop
    mov          [rx0], r10                   // dst[nfull] = l

.Ldone:
    mov          rax, r11
    test         r9, r9
    jz           .Lfin
    inc          rax
.Lfin:
    ret

.Lret0:
    xor          eax, eax
    ret

.section .rodata
.p2align 4
.Lc48:  .byte 48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48
.Lc9:   .byte 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9
.Lc60:  .byte 96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96
.Lc55:  .byte 55,55,55,55,55,55,55,55,55,55,55,55,55,55,55,55
.Lc87:  .byte 87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87
.Lones: .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
.Lc0f:  .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
.Lpat:  .byte 1,16,1,16,1,16,1,16,1,16,1,16,1,16,1,16
