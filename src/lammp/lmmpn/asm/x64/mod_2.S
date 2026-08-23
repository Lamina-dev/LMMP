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
# void lmmp_mod_2_(mp_srcptr numa, mp_size_t na, mp_ptr numb);

#include "lmmp_asm.h"

.text
.globl ASM_GSYM(lmmp_mod_2_)

#======================================================================
# void lmmp_mod_2_(mp_srcptr numa, mp_size_t na, mp_ptr numb)
# 余数存入 [numb]
#======================================================================
.p2align 4
ASM_GSYM(lmmp_mod_2_):
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15                     # 用 r15 保存 numb 指针

    # Windows 下保存非易失寄存器 rdi / rsi
win push rdi
win push rsi

    # 统一参数：r15 = numb, rsi = numa, rbx = na
#ifdef LMMP_WINDOWS
    mov r15, r8                  # numb
    mov rsi, rcx                 # numa
    mov rbx, rdx                 # na
#else
    mov r15, rdx                 # numb
    mov rbx, rsi                 # na
    mov rsi, rdi                 # numa
#endif
    lea rbx, [rbx-1]             # rbx = na - 1

    mov r8, [r15]                # 除数低
    mov r9, [r15+8]              # 除数高

    # ---- 被除数与除数比较 ----
    mov r11, [rsi+rbx*8-8]
    mov r12, [rsi+rbx*8]
    xor r13, r13
    cmp r12, r9
    ja .lab_2
    setb dl
    cmp r11, r8
    setb al
    or dl, al
    jz .lab_2
    # qh = 0
    dec rbx
    mov r13, r12
    mov r12, r11
    jz .lab_retnsh
    mov r11, [rsi+rbx*8-8]

.lab_2:
    # ---- 除数规整化 ----
    bsr rcx, r9
    not rcx
    add rcx, 64
    shld r9, r8, cl
    shl r8, cl

    push rcx                     # 移位量
    push r8
    push r9
    mov rx0, r9                  # 参数：除数高位

    # 栈对齐与影子空间
    # 当前已 push 8+3=11 个寄存器，rsp ≡ 0 mod 16
win sub rsp, 32
    CALL(lmmp_inv_1_)
win add rsp, 32

    pop r9
    pop r8
    mov rbp, rax                 # rbp = inv - 2^64

    # ---- 修正逆元 ----
    mov r10, r9
    imul r10, rax
    xor rcx, rcx
    mul r8
    add r10, r8
    adc rcx, -1
    add r10, rdx
    adc rcx, 0
    js .lab_2f
.lab_1b:
    dec rbp
    sub rax, r8
    sbb r10, r9
    sbb rcx, 0
    jns .lab_1b
.lab_2f:
    pop rcx                      # 恢复移位量
    # rbp = (2^192-1) div b - 2^64
    shld r13, r12, cl
    shld r12, r11, cl
    jmp .lab_ent

.p2align 4
.lab_top:
    mov r10, [rsi+rbx*8-8]
    shld r11, r10, cl
.lab_last:                       # r13:r12 = ah < b
    mov rax, rbp
    mul r13
    mov r14, r12
    add r14, rax
    adc rdx, r13
    mov r13, r14                 # ql
    mov r14, rdx                 # qh
    imul rdx, r9
    xchg r13, r12
    mov rax, r8
    sub r13, rdx
    xchg r12, r11                # r11 = ql
    mul r14
    sub r12, r8
    sbb r13, r9
    sub r12, rax
    sbb r13, rdx
    xor rax, rax
    xor rdx, rdx
    cmp r13, r11
    cmovnc rax, r8
    cmovnc rdx, r9

    add r12, rax
    adc r13, rdx
    cmp r13, r9
    jae .lab_fx
.lab_ok:
    mov r11, r10
.lab_ent:
    sub rbx, 1
    ja .lab_top
    jb .lab_ret
    shl r11, cl
    jmp .lab_last

.lab_fx:
    seta dl
    cmp r12, r8
    setae al
    or al, dl
    jz .lab_ok

    sub r12, r8
    sbb r13, r9
    jmp .lab_ok

.lab_ret:
    shrd r12, r13, cl
    shr r13, cl
.lab_retnsh:
    # ---- 写入余数 ----
    mov [r15], r12
    mov [r15+8], r13

    # 恢复寄存器
win pop rsi
win pop rdi
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
