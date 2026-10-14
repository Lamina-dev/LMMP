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
//   Windows: dst=rcx, numa=rdx, na=r8, upper=r9b
//   SysV:    dst=rdi, numa=rsi, na=rdx, upper=cl
//
// 输出为低位在前的十六进制串（与 C 版一致）：limb 0 的字符在最前，
// 每字节的低 nibble 字符在前。主循环每次 4 limb（ymm=32B）出 64 字符：
//   lo/hi = v & 0x0F / (v >>u16 4) & 0x0F   （16 位 lane 移位使各字节取高 nibble）
//   loc/hic = pshufb(16B LUT, nib)          （每 128 lane 一张表，vinserti128 复制）
//   unpacklo/unpackhi 交织后，128 lane 序为 {i, i+2}/{i+1, i+3}，
//   perm2i128(0x20/0x31) 交叉重排为 {i, i+1} 与 {i+2, i+3}
// 不足 4 的 limb 尾走 xmm（每 limb 16 字符），顶 limb 的残尾（0~15 nibble）
// 走标量。游走指针直接推进 rx1，循环结束处 rx1 恰指向顶 limb。
//
// 寄存器约定（两 ABI 通用部分）：r10=LUT 基址，r11/digits 与 tail 计数
// 经 push/pop 中转；rcx/rdx/r8/r9 仅在其参数职责耗尽后作暂存。
//

#include "lmmp_asm.h"

#ifdef LMMP_WINDOWS
#define rb2 r9b
#else
#define rb2 cl
#endif

.text

.p2align 4

.globl ASM_GSYM(lmmp_to_hex_)
ASM_GSYM(lmmp_to_hex_):
    // 大小写 LUT 基址（r10），标量尾复用；顺手装载向量常量
    lea          r10, [rip + .Lhex_lower]
    lea          r11, [rip + .Lhex_upper]
    test         rb2, rb2
    cmovne       r10, r11
    vmovdqa      xmm0, [r10]
    vinserti128  ymm0, ymm0, xmm0, 1         // ymm0 = 16B LUT 复制到两个 128 lane
    mov          eax, 0x0F
    vmovd        xmm1, eax
    vpbroadcastb ymm1, xmm1                  // ymm1 = 0x0F

    // 去值前导零：rax = na-1（最高非零 limb 下标）
    mov          rax, rx2
.Lstrip:
    test         rax, rax
    jz           .Lret0
    dec          rax
    cmp          qword ptr [rx1 + rax * 8], 0
    je           .Lstrip
    lea          rx2, [rax + 1]              // rx2 = na（规范化）

    // digits = ((na-1)*64 + bitlen(top) + 3) >> 2   -> 压栈保存
    mov          r11, [rx1 + rax * 8]        // top limb
    lzcnt        r11, r11
    neg          r11
    add          r11, 64                     // bitlen
    shl          rax, 6
    add          rax, r11
    add          rax, 3
    shr          rax, 2
    push         rax                         // [digits]

    // nfull = na-1：q = nfull>>2（r11 计数），tail = nfull&3（压栈）
    mov          rax, rx2
    dec          rax
    mov          r11, rax
    and          rax, 3
    push         rax                         // [digits][tail]
    shr          r11, 2
    jz           .Ltail_limbs

.p2align 4
.Lmain:                                        // 4 limb -> 64 chars
    vmovdqu      ymm2, [rx1]                 // {i, i+1, i+2, i+3}
    vpand        ymm3, ymm2, ymm1            // lo nibbles
    vpsrlw       ymm2, ymm2, 4
    vpand        ymm2, ymm2, ymm1            // hi nibbles
    vpshufb      ymm3, ymm0, ymm3            // loc
    vpshufb      ymm2, ymm0, ymm2            // hic
    vpunpcklbw   ymm4, ymm3, ymm2            // {limb i,   limb i+2}
    vpunpckhbw   ymm5, ymm3, ymm2            // {limb i+1, limb i+3}
    vperm2i128   ymm6, ymm4, ymm5, 0x20      // {i,   i+1}
    vmovdqu      [rx0], ymm6
    vperm2i128   ymm6, ymm4, ymm5, 0x31      // {i+2, i+3}
    vmovdqu      [rx0 + 32], ymm6
    add          rx1, 32
    add          rx0, 64
    dec          r11
    jnz          .Lmain

.Ltail_limbs:                                  // 1~3 个整 limb（xmm，每次 1 limb）
    pop          rax                         // tail 计数
    test         rax, rax
    jz           .Ltop
.Ltail_loop:
    vmovq        xmm2, [rx1]
    vpand        xmm3, xmm2, xmm1
    vpsrlw       xmm2, xmm2, 4
    vpand        xmm2, xmm2, xmm1
    vpshufb      xmm3, xmm0, xmm3
    vpshufb      xmm2, xmm0, xmm2
    vpunpcklbw   xmm4, xmm3, xmm2
    vmovdqu      [rx0], xmm4                 // 16 chars
    add          rx1, 8
    add          rx0, 16
    dec          rax
    jnz          .Ltail_loop

.Ltop:                                         // 顶 limb 残尾（低 nibble 字符在前）
    pop          r11                         // digits
    mov          rax, [rx1]                  // top limb（rx1 已推进至 &numa[nfull]）
    // 此后 rdx/r8/r9 的参数职责均已耗尽，可作暂存
.Ltop_pair:
    mov          rdx, rax
    shr          rdx, 4
    jz           .Ltop_one                   // 本字节仅剩低 nibble
    mov          r8, rax
    and          r8, 0x0F
    movzx        edx, byte ptr [r10 + r8]    // 低 nibble 字符
    mov          r8, rax
    shr          r8, 4
    and          r8, 0x0F
    movzx        r8d, byte ptr [r10 + r8]    // 高 nibble 字符
    shl          r8, 8
    or           rdx, r8
    mov          [rx0], dx                   // 低在前
    add          rx0, 2
    shr          rax, 8
    jmp          .Ltop_pair
.Ltop_one:                                     // 此处 rax ∈ [0,15]
    test         rax, rax
    jz           .Lret
    movzx        edx, byte ptr [r10 + rax]
    mov          [rx0], dl
.Lret:
    vzeroupper
    mov          rax, r11                    // digits
    ret

.Lret0:
    xor          eax, eax
    ret

.section .rodata
.p2align 4
.Lhex_lower:
    .ascii "0123456789abcdef"
.Lhex_upper:
    .ascii "0123456789ABCDEF"
