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
// mp_limb_t lmmp_shl_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl);
//
// mp_limb_t lmmp_shl_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl, mp_limb_t c);
//
//   Windows: dst=rcx, numa=rdx, na=r8, shl=r9, c=[rsp+40] (only c_)
//   SysV:    dst=rdi, numa=rsi, na=rdx, shl=rcx, c=r8 (only c_)
//   ret rax = numa[na-1] >> (64 - shl)
//   cal: dst[i] = (numa[i] << shl) | (numa[i-1] >> (64-shl))

#include "lmmp_asm.h"

.text

.p2align 4

// lmmp_shl_c_
.globl ASM_GSYM(lmmp_shl_c_)
ASM_GSYM(lmmp_shl_c_):
#ifdef LMMP_WINDOWS
    mov     rax, [rsp+40]           // rax = c
    xchg    rcx, r9                 // dst <-> shl
#else
    mov     rax, r8                 // rax = c
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif
    jmp     .Lentry

// lmmp_shl_
.p2align 4
.globl ASM_GSYM(lmmp_shl_)
ASM_GSYM(lmmp_shl_):
    xor     rax, rax                // c = 0
#ifdef LMMP_WINDOWS
    xchg    rcx, r9                 // dst <-> shl
#else
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif

.Lentry:
    and     cl, 63                  // shl &= 63
    push    r12
    push    r13
    push    r14
    jz      .Lcopy                  // shl == 0: 降序 copy（dst > numa 安全）

    mov     r10d, 64
    sub     r10d, ecx               // r10 = s = 64 - shl
    vmovq   xmm4, rcx               // 向量左移计数 k
    vmovq   xmm5, r10               // 向量右移计数 s

    mov     r11, [rdx + r8*8 - 8]   // a[n-1]
    xor     r12d, r12d
    shld    r12, r11, cl            // r12 = ret = a[n-1] >> s（暂存他寄存器）

    lea     rdx, [rdx + r8*8 - 8]   // rdx -> a[n-1]
    lea     r9, [r9 + r8*8 - 8]     // r9  -> dst[n-1]

    cmp     r8, 512
    jae     .Lpeel
    lea     r10, [r8 - 1]           // n-1
    xor     r14d, r14d              // P = 0：小 n 免剥底（省 ~14 条入口指令，
    jmp     .Lcommon                //   FFT 高频温热小调用无拆分存储罚可免）

.Lpeel:                             // n >= 512：按 dst 相位剥 P 个 limb 使组存储 32B 对齐
    mov     r10, r9
    shr     r10, 3
    neg     r10
    lea     r11, [r10 + 3]
    and     r11d, 3                 // P = (3 - (dst>>3)) & 3 <= 3 < n-1，免 clamp
    mov     r14, r11                // P 暂存（底链计数）
    lea     r10, [r8 - 1]
    sub     r10, r11                // M = n-1-P = 向量+顶尾 limb 数

.Lcommon:
    mov     r13, r10
    and     r13d, 3                 // T = 顶部余 limb 数
    shr     r10, 2                  // F = 向量组数
    mov     r8, r10
    test    r13, r13
    jz      .Loff

.Ltail:                             // 顶部 T 个 limb，降序标量 shld 链
    mov     r10, [rdx]
    mov     r11, [rdx - 8]
    shld    r10, r11, cl
    mov     [r9], r10
    sub     rdx, 8
    sub     r9, 8
    dec     r13
    jnz     .Ltail

.Loff:                              // 循环不变量对齐：rdx = &a[P+4F+1]
    add     rdx, 8
    add     r9, 8

.Lvec:
    test    r8, 1
    jz      .Lpairs
    vmovdqu ymm0, [rdx - 32]        // A = a[i+1..i+4]
    vmovdqu ymm1, [rdx - 40]        // A' = a[i..i+3]
    vpsllq  ymm2, ymm0, xmm4        // A << k
    vpsrlq  ymm3, ymm1, xmm5        // A' >> s
    vpor    ymm2, ymm2, ymm3
    vmovdqu [r9 - 32], ymm2
    sub     rdx, 32
    sub     r9, 32
    dec     r8

.Lpairs:
    shr     r8, 1                   // 组对数
    jz      .Lbottom

.p2align 4
.Lloop:                             // 主循环：每组对 8 limb，降序
    vmovdqu ymm0, [rdx - 32]        // 高组 A
    vmovdqu ymm1, [rdx - 40]        // 高组 A'
    vpsllq  ymm2, ymm0, xmm4
    vpsrlq  ymm3, ymm1, xmm5
    vmovdqu ymm0, [rdx - 64]        // 低组 A（载入提前发射）
    vmovdqu ymm1, [rdx - 72]        // 低组 A'
    vpor    ymm2, ymm2, ymm3
    vmovdqu [r9 - 32], ymm2
    vpsllq  ymm2, ymm0, xmm4
    vpsrlq  ymm3, ymm1, xmm5
    vpor    ymm2, ymm2, ymm3
    vmovdqu [r9 - 64], ymm2
    sub     rdx, 64
    sub     r9, 64
    dec     r8
    jnz     .Lloop

.Lbottom:                           // rdx = &a[P+1], r9 = &dst[P+1]
    test    r14, r14
    jz      .Lend

.Lbottom_chain:                     // 剥出的 P 个 limb，降序标量 shld 链
    mov     r10, [rdx - 8]          // a[i]
    mov     r11, [rdx - 16]         // a[i-1]
    shld    r10, r11, cl
    mov     [r9 - 8], r10
    sub     rdx, 8
    sub     r9, 8
    dec     r14
    jnz     .Lbottom_chain

.Lend:                              // rdx = &a[1], r9 = &dst[1]
    mov     r10, [rdx - 8]
    shl     r10, cl
    or      r10, rax                // (a[0] << shl) | c
    mov     [r9 - 8], r10
    mov     rax, r12                // ret
    vzeroupper
    pop     r14
    pop     r13
    pop     r12
    ret

.Lcopy:                             // shl == 0: 降序 copy, ret 0
    xor     eax, eax
    cmp     r9, rdx
    je      .Lcopy_out
    lea     rdx, [rdx + r8*8]
    lea     r9, [r9 + r8*8]
    mov     r13, r8
    and     r13d, 3                 // 尾 limb 数
    shr     r8, 2                   // 4-limb 组数
    jz      .Lcopy_tail

.p2align 4
.Lcopy_loop:
    vmovdqu ymm0, [rdx - 32]
    vmovdqu [r9 - 32], ymm0
    sub     rdx, 32
    sub     r9, 32
    dec     r8
    jnz     .Lcopy_loop

.Lcopy_tail:
    test    r13, r13
    jz      .Lcopy_out

.Lcopy_tail_loop:
    mov     r10, [rdx - 8]
    mov     [r9 - 8], r10
    sub     rdx, 8
    sub     r9, 8
    dec     r13
    jnz     .Lcopy_tail_loop

.Lcopy_out:
    vzeroupper
    pop     r14
    pop     r13
    pop     r12
    ret
