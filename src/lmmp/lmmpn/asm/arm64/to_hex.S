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
// 输出为低位在前的十六进制串（与 C 版一致）：limb 0 的字符在最前，
// 每字节的低 nibble 字符在前。主循环每次 4 limb（2×q 寄存器）出 64 字符：
//   lo = v & 0x0F（and 掩码），hi = v >>u8 4（ushr 字节 lane 移位后天然 <=15）
//   loc/hic = tbl(16B LUT, nib)          （NEON tbl 单表即查 16 字节）
//   zip1/zip2(loc, hic) 低/高半交织      （8 字节边界恰为 limb 边界，
//                                         zip1 出 limb i、zip2 出 limb i+1，
//                                         无需 x64 的 perm2i128 二次重排）
// 不足 4 的 limb 尾每 limb 走单 q 路径（ldr d 载入高 64 位清零，zip1 即
// 全 16 字符），顶 limb 的残尾（0~15 nibble）走标量查表。游走指针直接
// 推进 x1/x0，循环结束处 x1 恰指向顶 limb。
//
// 寄存器约定：x9=digits，x10=去零临时/LUT 基址，x2/x15 主/尾循环计数，
// x11-x14 顶残尾标量临时；仅用 v0-v7（d8-d15 低 64 位被调保存），零栈帧。
//

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_to_hex_)
ASM_GSYM(lmmp_to_hex_):
    // 去值前导零：x9 = na-1（最高非零 limb 下标），x10 兼作载入临时
    mov          x9, x2
.Lstrip:
    cbz          x9, .Lret0
    sub          x9, x9, #1
    ldr          x10, [x1, x9, lsl #3]
    cbz          x10, .Lstrip
    add          x2, x9, #1                 // x2 = na（规范化）

    // digits = ((na-1)*64 + bitlen(top) + 3) >> 2  -> x9（x10 = top limb）
    clz          x11, x10
    mov          w12, #64
    sub          w11, w12, w11              // bitlen(top)
    lsl          x12, x9, #6
    add          x12, x12, x11
    add          x12, x12, #3
    lsr          x9, x12, #2

    // 大小写 LUT 基址 x10（连续双表，upper 取高 16B；顶残尾标量复用）。
    // adr 无修饰符形式 GAS/clang 双兼容（@PAGE 系 Darwin 语法，GNU as 于
    // ELF 不认）；表驻 .text 同段保证 ±1MB 寻址范围（inv_1.S 同款约定）
    adr          x10, .Lhex_lut
    add          x11, x10, #16
    cmp          w3, #0
    csel         x10, x11, x10, ne

    // nfull = na-1；na==1 时仅剩顶 limb 残尾，跳过向量常量装载
    sub          x2, x2, #1
    cbz          x2, .Ltop
    ldr          q0, [x10]
    movi         v1.16b, #15
    and          x15, x2, #3                    // 尾 limb 数
    lsr          x2, x2, #2                     // 主循环计数
    cbz          x2, .Ltail

.p2align 4
.Lmain:                                        // 4 limb -> 64 chars
    ldp          q2, q3, [x1], #32
    and          v4.16b, v2.16b, v1.16b       // lo nibbles
    and          v5.16b, v3.16b, v1.16b
    ushr         v2.16b, v2.16b, #4           // hi nibbles（<=15 免掩码）
    ushr         v3.16b, v3.16b, #4
    tbl          v4.16b, {v0.16b}, v4.16b     // loc
    tbl          v5.16b, {v0.16b}, v5.16b
    tbl          v2.16b, {v0.16b}, v2.16b     // hic
    tbl          v3.16b, {v0.16b}, v3.16b
    zip1         v6.16b, v4.16b, v2.16b       // limb i   的 16 字符
    zip2         v7.16b, v4.16b, v2.16b       // limb i+1 的 16 字符
    stp          q6, q7, [x0], #32
    zip1         v6.16b, v5.16b, v3.16b       // limb i+2
    zip2         v7.16b, v5.16b, v3.16b       // limb i+3
    stp          q6, q7, [x0], #32
    subs         x2, x2, #1
    b.ne         .Lmain

.Ltail:                                        // 0~3 个整 limb，每次 1 limb
    cbz          x15, .Ltop
.Ltail_loop:
    ldr          d2, [x1], #8                 // 高 64 位清零
    and          v3.16b, v2.16b, v1.16b
    ushr         v2.16b, v2.16b, #4
    tbl          v3.16b, {v0.16b}, v3.16b
    tbl          v2.16b, {v0.16b}, v2.16b
    zip1         v4.16b, v3.16b, v2.16b       // 低半交织即全 16 字符
    str          q4, [x0], #16
    subs         x15, x15, #1
    b.ne         .Ltail_loop

.Ltop:                                         // 顶 limb 残尾（低 nibble 字符在前）
    ldr          x11, [x1]                    // x1 已推进至 &numa[nfull]
.Ltop_pair:
    lsr          x12, x11, #4
    cbz          x12, .Ltop_one               // 本字节仅剩低 nibble
    and          x13, x11, #0xF
    ldrb         w14, [x10, x13]              // 低 nibble 字符
    and          x13, x12, #0xF               // x12 = x11>>4 仍在
    ldrb         w15, [x10, x13]              // 高 nibble 字符
    strb         w14, [x0], #1                // 低在前
    strb         w15, [x0], #1
    lsr          x11, x11, #8
    b            .Ltop_pair
.Ltop_one:                                     // x11 ∈ [0,15]
    cbz          x11, .Lret
    ldrb         w14, [x10, x11]
    strb         w14, [x0]
.Lret:
    mov          x0, x9                       // digits
    ret

.Lret0:
    mov          x0, #0
    ret

.p2align 4
.Lhex_lut:                                     // 驻 .text（adr 同段范围）
    .ascii "0123456789abcdef"
    .ascii "0123456789ABCDEF"
