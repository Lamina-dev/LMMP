#
#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#   by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY. 
#
#  See <https://www.gnu.org/licenses/>.

#
# mp_limb_t lmmp_mod_1_(mp_srcptr numa, mp_size_t na, mp_limb_t x);


#include "lammp_asm.h"

.text

#ifdef LMMP_WINDOWS
#else
#endif


.text
.globl ASM_GSYM(lmmp_mod_1_)

#======================================================================
# mp_limb_t lmmp_mod_1_(mp_srcptr numa, mp_size_t na, mp_limb_t x)
#======================================================================
.p2align 4
ASM_GSYM(lmmp_mod_1_):
    push rbp
    push rbx
    push r12
#ifdef LMMP_WINDOWS
    push rsi
#endif

    # 参数统一
#ifdef LMMP_WINDOWS
    mov rsi, rcx                  # rsi = numa
    mov rbx, rdx                  # rbx = na
    # r8 已经是 x
#else
    mov rbx, rsi                  # rbx = na
    mov rsi, rdi                  # rsi = numa
    mov r8, rdx                   # r8 = x
#endif

    xor eax, eax
    bsr rcx, r8
    not rcx
    add rcx, 64
    shl r8, cl                    # 规整化 x

    mov rbp, rcx                  # 移位量
    mov r12, r8                   # 规整化除数
    mov rx0, r8                   # 参数：x

    # 栈对齐与影子空间
#ifdef LMMP_WINDOWS
    # 已 push 4 个寄存器 (32)，rsp ≡ 8，需 32+8=40
    sub rsp, 40
#else
    # 已 push 3 个寄存器，rsp 已对齐
    sub rsp, 8
#endif
    CALL(lmmp_inv_1_)
#ifdef LMMP_WINDOWS
    add rsp, 40
#else
    add rsp, 8
#endif

    mov r9, rax                   # r9 = inv
    mov r8, r12
    mov rcx, rbp

    xor eax, eax
    mov r10, [rsi+rbx*8-8]
    shld rax, r10, cl
    jmp .lab_ent

.p2align 4
.lab_top:
    mov r12, [rsi+rbx*8-8]
    shld r10, r12, cl
.lab_last:
    mul r9
    add rax, r10
    adc rdx, rbp
    mov rbp, rax
    imul rdx, r8
    sub r10, rdx
    lea rax, [r8+r10]
    cmp r10, rbp
    cmovc rax, r10
    cmp rax, r8
    jae .lab_fx
.lab_ok:
    mov r10, r12
.lab_ent:
    sub rbx, 1
    lea rbp, [rax+1]
    ja .lab_top
    jb .lab_ret

    shl r10, cl
    jmp .lab_last

.lab_fx:
    sub rax, r8
    jmp .lab_ok

.lab_ret:
    shr rax, cl                   # 余数在 rax

#ifdef LMMP_WINDOWS
    pop rsi
#endif
    pop r12
    pop rbx
    pop rbp
    ret
