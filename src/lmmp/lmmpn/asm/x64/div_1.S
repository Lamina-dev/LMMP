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
# mp_limb_t lmmp_div_1_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_limb_t x);
#
# mp_limb_t lmmp_div_1_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_limb_t x);

#include "lmmp_asm.h"

.text

# ----------------------------------------------------------------------
# mp_limb_t lmmp_div_1_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_limb_t x)
# ----------------------------------------------------------------------
.globl ASM_GSYM(lmmp_div_1_)
.p2align 4
ASM_GSYM(lmmp_div_1_):
    test    rx0, rx0
    jz      .Lmod                   # dstq == NULL → 仅计算余数

    # ---- 正常除法（存储商） ----
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
#endif
    push    rbp
    push    rbx
    push    r12

    mov     rbx, rx2                # rbx = na
#ifdef LMMP_WINDOWS
    mov     rdi, rcx                # rdi = dstq
    mov     rsi, rdx                # rsi = numa
#endif
    mov     r8, rx3                 # r8 = x

    xor     eax, eax
    bsr     rcx, r8
    not     rcx
    add     rcx, 64
    shl     r8, cl                  # 规整化除数

#ifndef LMMP_WINDOWS
    mov     r10, rdi                # 保存 rdi (dstq) 到 r10，call 会破坏它
#endif
    mov     r11, rax                # r11 = 0
    mov     rbp, rcx                # rbp = 移位量
    mov     r12, r8                 # r12 = 规整化除数
    mov     rx0, r8                 # 参数：x → rcx/rdi

    # 栈对齐与影子空间（尽管并未使用）
#ifdef LMMP_WINDOWS
    sub     rsp, 32
#endif
    CALL(lmmp_inv_1_)
#ifdef LMMP_WINDOWS
    add     rsp, 32
#endif

#ifndef LMMP_WINDOWS
    mov     rdi, r10                # 恢复 rdi
#endif
    mov     r9, rax                 # r9 = 逆
    mov     r8, r12
    mov     rcx, rbp
    mov     rax, r11                # rax = 0

    mov     r10, [rsi+rbx*8-8]
    shld    rax, r10, cl
    jmp     .Lent

.p2align 4
.Ltop:
    mov     r12, [rsi+rbx*8-8]
    shld    r10, r12, cl
.Llast:                             # rax = xh < r8, rbp = xh+1
    mul     r9                      # rdx:rax = xh * (inv-2^64)
    add     rax, r10
    adc     rdx, rbp
    mov     rbp, rax                # rbp = ql
    mov     r11, rdx                # r11 = qh
    imul    rdx, r8
    sub     r10, rdx                # r10 = z
    lea     rax, [r8+r10]
    cmp     r10, rbp
    cmovc   rax, r10
    adc     r11, -1
    cmp     rax, r8
    jae     .Lfix

.Lok:
    mov     [rdi+rbx*8], r11         # 存储商
    mov     r10, r12

.Lent:
    sub     rbx, 1
    lea     rbp, [rax+1]
    ja      .Ltop
    jb      .Lret

    shl     r10, cl
    jmp     .Llast

.Lfix:
    sub     rax, r8
    inc     r11
    jmp     .Lok

.Lret:
    shr     rax, cl                 # 余数在 rax
    pop     r12
    pop     rbx
    pop     rbp
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

# ------------------------- dstq == NULL：仅计算余数 -------------------------
.Lmod:
#ifdef LMMP_WINDOWS
    # 参数重映射：lmmp_mod_1_(numa,na,x) → rcx=numa, rdx=na, r8=x
    mov     rcx, rdx
    mov     rdx, r8
    mov     r8, r9
    # 入口 rsp 仅含返回地址（8 字节，未对齐），需 40 字节调整
    sub     rsp, 40
    CALL(lmmp_mod_1_)
    add     rsp, 40
#else
    # Linux 入口 rsp 未对齐，映射参数并调整栈
    mov     rdi, rsi
    mov     rsi, rdx
    mov     rdx, rcx
    sub     rsp, 8
    CALL(lmmp_mod_1_)
    add     rsp, 8
#endif
    ret

# ----------------------------------------------------------------------
# mp_limb_t lmmp_div_1_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_limb_t x)
# ----------------------------------------------------------------------
.globl ASM_GSYM(lmmp_div_1_s_)
.p2align 4
ASM_GSYM(lmmp_div_1_s_):
    push    rbp
    push    rbx
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
#endif

    mov     rbx, rx2
#ifdef LMMP_WINDOWS
    mov     rdi, rcx
    mov     rsi, rdx
#else
    mov     r10, rdi
#endif
    mov     rbp, rx3
    mov     rx0, rx3                # 参数：x → rcx/rdi

    # 栈对齐及影子空间
#ifdef LMMP_WINDOWS
    # 已 push 4 个寄存器 (32 字节)，rsp ≡ 8，需 32+8=40
    sub     rsp, 40
#else
    # 已 push 2 个寄存器，rsp ≡ 8，对齐需 8
    sub     rsp, 8
#endif
    CALL(lmmp_inv_1_)
#ifdef LMMP_WINDOWS
    add     rsp, 40
#else
    add     rsp, 8
#endif

#ifndef LMMP_WINDOWS
    mov     rdi, r10
#endif
    mov     r8, rbp
    mov     r9, rax
    xor     rcx, rcx
    mov     r10, [rsi+rbx*8-8]
    mov     rax, r10
    sub     rax, r8
    setnc   cl
    cmovc   rax, r10
    dec     rbx

.p2align 4
.Lstop:
    lea     rbp, [rax+1]
    mov     r10, [rsi+rbx*8-8]
    mul     r9
    add     rax, r10
    adc     rdx, rbp
    mov     rbp, rax
    mov     r11, rdx
    imul    rdx, r8
    sub     r10, rdx
    lea     rax, [r8+r10]
    cmp     r10, rbp
    cmovc   rax, r10
    adc     r11, -1
    cmp     rax, r8
    jae     .Lsfx

.Lsok:
    mov     [rdi+rbx*8-8], r11
    dec     rbx
    jnz     .Lstop
    mov     [rsi], rax
    mov     rax, rcx

#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    pop     rbx
    pop     rbp
    ret

.Lsfx:
    sub     rax, r8
    inc     r11
    jmp     .Lsok
