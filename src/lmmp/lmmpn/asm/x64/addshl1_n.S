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
# mp_limb_t lmmp_addshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_addshl1_n_)
ASM_GSYM(lmmp_addshl1_n_):
#ifdef LMMP_WINDOWS
    push rsi
    push rdi
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov rdi, rcx
    mov rsi, rdx
    mov rdx, r8
    mov rcx, r9
    mov r8, r9
#else
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r8, rcx
#endif

    lea rsi, [rsi + r8*8]
    lea rdx, [rdx + r8*8]
    lea rdi, [rdi + r8*8]
    mov r9, r8

    xor rax, rax
    xor rbx, rbx
    and cl, -4
    neg rcx

    test r9b, 1
    jnz .lab_nx1

.lab_nx0:
    test r9b, 2
    jz .lab_top

.lab_n10:
    mov r14, [rdx + 8*rcx - 16]
    mov r15, [rdx + 8*rcx -  8]
    mov r10, [rsi + 8*rcx - 16]
    mov r11, [rsi + 8*rcx -  8]
    shl r14, 1
    rcl r15, 1
    setc bl

    add r10, r14
    adc r11, r15
    setc al

    mov [rdi + 8*rcx - 16], r10
    mov [rdi + 8*rcx -  8], r11
    test rcx, rcx
    jz .lab_ret
    jmp .lab_top

.lab_nx1:
    test r9b, 2
    jnz .lab_n11

.lab_n01:
    mov r15, [rdx + 8*rcx - 8]
    mov r11, [rsi + 8*rcx - 8]
    shl r15, 1
    setc bl

    add r11, r15
    setc al

    mov [rdi + 8*rcx - 8], r11
    test rcx, rcx
    jz .lab_ret
    jmp .lab_top

.lab_n11:
    mov r13, [rdx + 8*rcx - 24]
    mov r14, [rdx + 8*rcx - 16]
    mov r15, [rdx + 8*rcx -  8]
    mov r8,  [rsi + 8*rcx - 24]
    mov r10, [rsi + 8*rcx - 16]
    mov r11, [rsi + 8*rcx -  8]
    shl r13, 1
    rcl r14, 1
    rcl r15, 1
    setc bl

    add r8,  r13
    adc r10, r14
    adc r11, r15
    setc al

    mov [rdi + 8*rcx - 24], r8
    mov [rdi + 8*rcx - 16], r10
    mov [rdi + 8*rcx -  8], r11
    test rcx, rcx
    jz .lab_ret

.p2align 4
.lab_top:
    mov r12, [rdx + 8*rcx]
    mov r13, [rdx + 8*rcx +  8]
    neg bl
    mov r8,  [rsi + 8*rcx]
    mov r9,  [rsi + 8*rcx +  8]
    rcl r12, 1
    rcl r13, 1

    mov r14, [rdx + 8*rcx + 16]
    mov r15, [rdx + 8*rcx + 24]
    rcl r14, 1
    rcl r15, 1
    setc bl

    mov r10, [rsi + 8*rcx + 16]
    mov r11, [rsi + 8*rcx + 24]
    neg al

    adc r8,  r12
    adc r9,  r13
    mov [rdi + 8*rcx],      r8
    mov [rdi + 8*rcx +  8], r9

    adc r10, r14
    adc r11, r15
    setc al

    mov [rdi + 8*rcx + 16], r10
    mov [rdi + 8*rcx + 24], r11

    add rcx, 4
    jnz .lab_top

.lab_ret:
    add al, bl

#ifdef LMMP_WINDOWS
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rdi
    pop rsi
#else
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
#endif
    ret
