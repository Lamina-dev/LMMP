#
#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#  by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY.
#
#  See <https://www.gnu.org/licenses/>.

#
# mp_limb_t lmmp_addshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_addshl1_n_)
ASM_GSYM(lmmp_addshl1_n_):

#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    mov     rdi, rcx                # dst
    mov     rsi, rdx                # numa
    mov     rdx, r8                 # numb
    mov     r8, r9                  # n
#else
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    mov     r8, rcx                 # n
#endif

    xor     r14d, r14d              # prev = 0（移位进位入）
    xor     r15d, r15d              # 加法进位暂存 = 0
    mov     rcx, r8                 # rcx = n
    and     rcx, 3                  # 余数 limb 数
    jz      .Lgroups

.p2align 4
.Lscalar:
    mov     r10, [rdx]              # t = numb[i]
    shld    r10, r14, 1             # shifted = (numb[i]<<1) | (prev>>63)
    mov     r14, [rdx]              # prev = numb[i]（原始值）
    add     r15, r15                # 恢复加法进位 CF
    mov     r9, [rsi]               # a = numa[i]
    adc     r9, r10                 # a + shifted + CF
    sbb     r15, r15                # 捕获 -进位
    mov     [rdi], r9
    lea     rdi, [rdi+8]
    lea     rsi, [rsi+8]
    lea     rdx, [rdx+8]
    dec     rcx
    jnz     .Lscalar

.Lgroups:
    mov     rcx, r8
    shr     rcx, 2                  # 4 limb 一组
    jz      .Ldone

.p2align 4
.Lloop:
    mov     r10, [rdx]              # t0 = numb[i]
    mov     r11, [rdx+8]            # t1
    mov     r12, [rdx+16]           # t2
    mov     r13, [rdx+24]           # t3
    shld    r13, r12, 1             # sh3 = (t3<<1) | (t2>>63)
    shld    r12, r11, 1             # sh2 = (t2<<1) | (t1>>63)
    shld    r11, r10, 1             # sh1 = (t1<<1) | (t0>>63)
    shld    r10, r14, 1             # sh0 = (t0<<1) | (prev>>63)
    mov     r14, [rdx+24]           # prev = numb[i+3]（原始值）
    add     r15, r15                # 恢复加法进位 CF
    mov     r9,  [rsi]              # a0
    mov     rbx, [rsi+8]            # a1
    mov     rbp, [rsi+16]           # a2
    mov     rax, [rsi+24]           # a3
    adc     r9,  r10
    adc     rbx, r11
    adc     rbp, r12
    adc     rax, r13                # CF = 进位出
    sbb     r15, r15                # 捕获 -进位
    mov     [rdi],    r9
    mov     [rdi+8],  rbx
    mov     [rdi+16], rbp
    mov     [rdi+24], rax
    lea     rdi, [rdi+32]
    lea     rsi, [rsi+32]
    lea     rdx, [rdx+32]
    dec     rcx
    jnz     .Lloop

.Ldone:
    mov     rax, r14
    shr     rax, 63                 # numb[n-1] >> 63
    sub     rax, r15                # + 加法进位（r15 = 0 或 -1）

#ifdef LMMP_WINDOWS
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
    pop     rdi
    pop     rsi
#else
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#endif
    ret
