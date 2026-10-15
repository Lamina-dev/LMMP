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
// mp_limb_t lmmp_shr_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr);
//
// mp_limb_t lmmp_shr_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c);
//
//   Windows: dst=rcx, numa=rdx, na=r8, shr=r9, c=[rsp+40] (only c_)
//   SysV:    dst=rdi, numa=rsi, na=rdx, shr=rcx, c=r8 (only c_)
//   ret rax = numa[0] << (64 - shr)
//   cal: dst[i] = (numa[i] >> shr) | (numa[i+1] << (64-shr))

#include "lmmp_asm.h"

.text

.p2align 4

// lmmp_shr_c_
.globl ASM_GSYM(lmmp_shr_c_)
ASM_GSYM(lmmp_shr_c_):
#ifdef LMMP_WINDOWS
    mov     rax, [rsp+40]           // rax = c
    xchg    rcx, r9                 // dst <-> shr
#else
    mov     rax, r8                 // rax = c
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif
    jmp     .Lentry

// lmmp_shr_
.p2align 4
.globl ASM_GSYM(lmmp_shr_)
ASM_GSYM(lmmp_shr_):
    xor     rax, rax                // c = 0
#ifdef LMMP_WINDOWS
    xchg    rcx, r9
#else
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif

.Lentry:
    and     cl, 63                  // shr &= 63
    push    r12
    push    r13
    push    r14
    jz      .Lcopy                  // shr == 0: 升序 copy（dst < numa 安全）

    mov     r10d, 64
    sub     r10d, ecx               // r10 = s = 64 - shr
    vmovq   xmm4, rcx               // 向量右移计数 k
    vmovq   xmm5, r10               // 向量左移计数 s

    mov     r11, [rdx]              // a[0]
    xor     r12d, r12d
    shrd    r12, r11, cl            // r12 = ret = a[0] << s（暂存他寄存器）

    mov     r10, [rdx + r8*8 - 8]   // a[n-1]
    shr     r10, cl
    or      r10, rax                // | c
    mov     rax, r10                // rax = 顶 limb 值（延迟存储，勿提前写）

    cmp     r8, 512
    jae     .Lpeel
    lea     r11, [r8 - 1]           // n-1
    xor     r14d, r14d              // P = 0：小 n 免剥底（省 ~14 条入口指令，
    jmp     .Lcommon                //   FFT 高频温热小调用无拆分存储罚可免）

.Lpeel:                             // n >= 512：按 dst 相位剥 P 个 limb 使组存储 32B 对齐
    mov     r10, r9
    shr     r10, 3
    neg     r10
    and     r10d, 3                 // P = (-(dst>>3)) & 3 <= 3 < n-1，免 clamp
    mov     r14, r10                // 头链计数
    lea     r11, [r8 - 1]
    sub     r11, r10                // M = n-1-P = 向量+顶尾 limb 数

.Lcommon:
    mov     r8, r11
    shr     r8, 2                   // F = 向量组数
    mov     r13, r11
    and     r13d, 3                 // T = 顶部标量尾 limb 数

    test    r14, r14
    jz      .Lvec

.Lhead:                             // 底部 P 个 limb，升序标量 shrd 链
    mov     r10, [rdx]              // a[i]
    mov     r11, [rdx + 8]          // a[i+1]
    shrd    r10, r11, cl
    mov     [r9], r10
    add     rdx, 8
    add     r9, 8
    dec     r14
    jnz     .Lhead

.Lvec:
    test    r8, 1
    jz      .Lpairs
    vmovdqu ymm0, [rdx]             // A = a[i..i+3]
    vmovdqu ymm1, [rdx + 8]         // A' = a[i+1..i+4]
    vpsrlq  ymm2, ymm0, xmm4        // A >> k
    vpsllq  ymm3, ymm1, xmm5        // A' << s
    vpor    ymm2, ymm2, ymm3
    vmovdqu [r9], ymm2
    add     rdx, 32
    add     r9, 32
    dec     r8

.Lpairs:
    shr     r8, 1                   // 组对数
    jz      .Ltail

.p2align 4
.Lloop:                             // 主循环：每组对 8 limb，升序
    vmovdqu ymm0, [rdx]             // 低组 A
    vmovdqu ymm1, [rdx + 8]         // 低组 A'
    vpsrlq  ymm2, ymm0, xmm4
    vpsllq  ymm3, ymm1, xmm5
    vmovdqu ymm0, [rdx + 32]        // 高组 A（载入提前发射）
    vmovdqu ymm1, [rdx + 40]        // 高组 A'
    vpor    ymm2, ymm2, ymm3
    vmovdqu [r9], ymm2
    vpsrlq  ymm2, ymm0, xmm4
    vpsllq  ymm3, ymm1, xmm5
    vpor    ymm2, ymm2, ymm3
    vmovdqu [r9 + 32], ymm2
    add     rdx, 64
    add     r9, 64
    dec     r8
    jnz     .Lloop

.Ltail:                             // 顶部 T 个 limb（至 dst[n-2]），升序标量链
    test    r13, r13
    jz      .Ltop
    mov     r10, [rdx]              // a[i]
    mov     r11, [rdx + 8]          // a[i+1]
    shrd    r10, r11, cl
    mov     [r9], r10
    add     rdx, 8
    add     r9, 8
    dec     r13
    jnz     .Ltail

.Ltop:                              // r9 恰为 dst + 8(n-1)
    mov     [r9], rax               // 延迟的顶 limb 存储
    mov     rax, r12                // ret
    vzeroupper
    pop     r14
    pop     r13
    pop     r12
    ret

.Lcopy:                             // shr == 0: 升序 copy, ret 0
    xor     eax, eax
    cmp     r9, rdx
    je      .Lcopy_out
    mov     r13, r8
    and     r13d, 3                 // 尾 limb 数
    shr     r8, 2                   // 4-limb 组数
    jz      .Lcopy_tail

.p2align 4
.Lcopy_loop:
    vmovdqu ymm0, [rdx]
    vmovdqu [r9], ymm0
    add     rdx, 32
    add     r9, 32
    dec     r8
    jnz     .Lcopy_loop

.Lcopy_tail:
    test    r13, r13
    jz      .Lcopy_out

.Lcopy_tail_loop:
    mov     r10, [rdx]
    mov     [r9], r10
    add     rdx, 8
    add     r9, 8
    dec     r13
    jnz     .Lcopy_tail_loop

.Lcopy_out:
    vzeroupper
    pop     r14
    pop     r13
    pop     r12
    ret
