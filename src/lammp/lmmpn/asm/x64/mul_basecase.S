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
# [dst, na+nb] = [numa, na] * [numb, nb]
# void lmmp_mul_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

#include "lmmp_asm.h"

.text

#ifdef LMMP_WINDOWS
    # Windows: dst=rcx, numa=rdx, na=r8, numb=r9, nb=[rsp+0x48]
    #define P_DST rcx
    #define P_NUMA rdx
    #define P_NA r8
    #define P_NUMB r9
    #define P_NB QWORD PTR [rsp+0x48]
#else
    # Linux: dst=rdi, numa=rsi, na=rdx, numb=rcx, nb=r8
    #define P_DST rdi
    #define P_NUMA rsi
    #define P_NA rdx
    #define P_NUMB rcx
    #define P_NB r8
#endif

.macro PUSH_ALL
#ifdef LMMP_WINDOWS
    push rsi
    push rdi
#endif
    push rbx
    push rbp
.endm

.macro POP_ALL
    pop rbp
    pop rbx
#ifdef LMMP_WINDOWS
    pop rdi
    pop rsi
#endif
.endm

.macro PUSH_EXTRA
    push r12
    push r13
    push r14
.endm

.macro POP_EXTRA
    pop r14
    pop r13
    pop r12
.endm

.macro PUSH_FULL
    push r15
    push r8
.endm

.macro POP_FULL
    pop rax
    pop r15
.endm

# ====================== .code ======================

.text
.globl ASM_GSYM(lmmp_mul_basecase_)
.p2align 4

ASM_GSYM(lmmp_mul_basecase_):
    # 保存保留寄存器
    PUSH_ALL

    # 初始化寄存器
#ifdef LMMP_WINDOWS
    mov rdi, P_DST           # dst -> rdi
    mov rsi, P_NUMA          # numa -> rsi
    mov rdx, P_NA            # na -> rdx
    mov rcx, P_NUMB          # numb -> rcx
    mov r8, P_NB             # nb -> r8
#else

#endif

    mov rbx, rdx             # rbx = na
    neg rbx                  # rbx = -na

    mov rax, [rsi]           # rax = numa[0]
    lea rsi, [rsi + rdx*8]   # rsi = numa + na
    lea rdi, [rdi + rdx*8]   # rdi = dst + na

    mov r9, [rcx]            # r9 = numb[0]
    mul r9                   # 乘法：rdx:rax = numa[0] * numb[0]

    test r8b, 1              # 检查nb奇偶性
    jz lab_do_mul_2

# ==================== 分支1：nb为奇数 ====================
ASM_GSYM(lab_do_mul_1):
    test bl, 1
    jnz lab_m1x1

ASM_GSYM(lab_m1x0):
    mov r10, rax
    mov r11, rdx
    mov rax, [rsi + rbx*8 + 8]
    test bl, 2
    jnz lab_m110

ASM_GSYM(lab_m100):
    lea rbp, [rbx + 2]
    jmp lab_m1l0

ASM_GSYM(lab_m110):
    lea rbp, [rbx]
    jmp lab_m1l2

ASM_GSYM(lab_m1x1):
    mov r11, rax
    mov r10, rdx
    test bl, 2
    jz lab_m111

ASM_GSYM(lab_m101):
    lea rbp, [rbx + 3]
    test rbp, rbp
    js lab_m1l1
    mov [rdi - 8], rax
    mov [rdi], rdx
    POP_ALL
    ret

ASM_GSYM(lab_m111):
    lea rbp, [rbx + 1]
    mov rax, [rsi + rbx*8 + 8]
    jmp lab_m1l3

.p2align 4
ASM_GSYM(lab_m1tp):
    mov r10, rdx
    add r11, rax
ASM_GSYM(lab_m1l1):
    mov rax, [rsi + rbp*8 - 16]
    adc r10, 0
    mul r9
    add r10, rax
    mov [rdi + rbp*8 - 24], r11
    mov rax, [rsi + rbp*8 - 8]
    mov r11, rdx
    adc r11, 0
ASM_GSYM(lab_m1l0):
    mul r9
    mov [rdi + rbp*8 - 16], r10
    add r11, rax
    mov r10, rdx
    mov rax, [rsi + rbp*8]
    adc r10, 0
ASM_GSYM(lab_m1l3):
    mul r9
    mov [rdi + rbp*8 - 8], r11
    mov r11, rdx
    add r10, rax
    mov rax, [rsi + rbp*8 + 8]
    adc r11, 0
ASM_GSYM(lab_m1l2):
    mul r9
    mov [rdi + rbp*8], r10
    add rbp, 4
    jnc lab_m1tp

ASM_GSYM(lab_m1ed):
    add r11, rax
    adc rdx, 0
    mov [rdi - 8], r11
    mov [rdi], rdx

    dec r8                  # nb--
    jz lab_ret2

    lea rcx, [rcx + 8]      # numb++
    lea rdi, [rdi + 8]      # dst++
    PUSH_EXTRA
    jmp lab_do_addmul

# ==================== 分支2：nb为偶数 ====================
ASM_GSYM(lab_do_mul_2):
    PUSH_EXTRA
    mov r14, [rcx + 8]      # r14 = numb[1]

    test bl, 1
    jnz lab_m2b1

ASM_GSYM(lab_m2b0):
    lea rbp, [rbx]
    xor r10, r10
    mov r12, rax
    mov r11, rdx
    jmp lab_m2l0

ASM_GSYM(lab_m2b1):
    lea rbp, [rbx + 1]
    xor r11, r11
    xor r12, r12
    mov r10, rax
    mov r13, rdx
    jmp lab_m2l1

.p2align 4
ASM_GSYM(lab_m2tp):
    mul r9
    add r10, rax
    mov r13, rdx
    adc r13, 0
ASM_GSYM(lab_m2l1):
    mov rax, [rsi + rbp*8 - 8]
    mul r14
    add r10, r11
    adc r13, 0
    add r12, rax
    mov [rdi + rbp*8 - 8], r10
    mov r10, rdx
    adc r10, 0
    mov rax, [rsi + rbp*8]
    mul r9
    add r12, rax
    mov r11, rdx
    adc r11, 0
    add r12, r13
ASM_GSYM(lab_m2l0):
    mov rax, [rsi + rbp*8]
    adc r11, 0
    mul r14
    mov [rdi + rbp*8], r12
    add r10, rax
    mov r12, rdx
    mov rax, [rsi + rbp*8 + 8]
    adc r12, 0
    add rbp, 2
    jnc lab_m2tp

ASM_GSYM(lab_m2ed):
    mul r9
    add r10, rax
    mov r13, rdx
    adc r13, 0
    mov rax, [rsi - 8]
    mul r14
    add r10, r11
    adc r13, 0
    add r12, rax
    mov [rdi - 8], r10
    adc rdx, 0
    add r12, r13
    mov [rdi], r12
    adc rdx, 0
    mov [rdi + 8], rdx

    sub r8, 2               # nb -= 2
    jz lab_ret5
    lea rcx, [rcx + 16]     # numb += 2
    lea rdi, [rdi + 16]     # dst += 2

# ==================== 累加乘法核心循环 ====================
ASM_GSYM(lab_do_addmul):
    PUSH_FULL

ASM_GSYM(lab_outer):
    mov r9, [rcx]
    mov r8, [rcx + 8]
    mov rax, [rsi + rbx*8]
    mul r9
    test bl, 1
    jnz lab_a1x1

ASM_GSYM(lab_a1x0):
    mov r14, [rdi + rbx*8]
    xor r10, r10
    mov r11, rdx
    test bl, 2
    jnz lab_a110

ASM_GSYM(lab_a100):
    lea rbp, [rbx + 2]
    add r14, rax
    adc r11, 0
    mov rax, [rsi + rbx*8]
    mul r8
    mov r15, [rdi + rbx*8 + 8]
    jmp lab_lo0

ASM_GSYM(lab_a110):
    lea rbp, [rbx]
    xor r13, r13
    jmp lab_lo2

ASM_GSYM(lab_a1x1):
    mov r15, [rdi + rbx*8]
    xor r12, r12
    xor r11, r11
    test bl, 2
    jz lab_a111

ASM_GSYM(lab_a101):
    lea rbp, [rbx + 3]
    mov r13, rdx
    add r15, rax
    mov rax, [rsi + rbx*8]
    adc r13, 0
    jmp lab_top

ASM_GSYM(lab_a111):
    lea rbp, [rbx + 1]
    jmp lab_lo3

.p2align 4
ASM_GSYM(lab_top):
    mov r14, [rdi + rbp*8 - 16]
    mul r8
    mov r10, rdx
    add r14, rax
    adc r10, 0
    add r15, r11
    adc r13, 0
    add r14, r12
    adc r10, 0
    mov rax, [rsi + rbp*8 - 16]
    mul r9
    add r14, rax
    mov r11, rdx
    adc r11, 0
    mov rax, [rsi + rbp*8 - 16]
    mul r8
    mov [rdi + rbp*8 - 24], r15
    mov r15, [rdi + rbp*8 - 8]
    add r14, r13
    adc r11, 0
ASM_GSYM(lab_lo0):
    mov r12, rdx
    mov [rdi + rbp*8 - 16], r14
    add r15, rax
    adc r12, 0
    mov rax, [rsi + rbp*8 - 8]
    add r15, r10
    adc r12, 0
    mul r9
ASM_GSYM(lab_lo3):
    add r15, rax
    mov r13, rdx
    adc r13, 0
    mov rax, [rsi + rbp*8 - 8]
    mul r8
    add r15, r11
    mov r14, [rdi + rbp*8]
    adc r13, 0
    mov r10, rdx
    add r14, rax
    adc r10, 0
    mov rax, [rsi + rbp*8]
    mul r9
    add r14, r12
    mov [rdi + rbp*8 - 8], r15
    mov r11, rdx
    adc r10, 0
ASM_GSYM(lab_lo2):
    add r14, rax
    adc r11, 0
    mov rax, [rsi + rbp*8]
    add r14, r13
    adc r11, 0
    mul r8
    mov r15, [rdi + rbp*8 + 8]
    add r15, rax
    mov r12, rdx
    adc r12, 0
    mov rax, [rsi + rbp*8 + 8]
    mov [rdi + rbp*8], r14
    mul r9
    add r15, r10
    mov r13, rdx
    adc r12, 0
    add r15, rax
    mov rax, [rsi + rbp*8 + 8]
    adc r13, 0
    add rbp, 4
    jnc lab_top

ASM_GSYM(lab_end):
    mul r8
    add r15, r11
    adc r13, 0
    add rax, r12
    adc rdx, 0
    mov [rdi - 8], r15
    add rax, r13
    adc rdx, 0
    mov [rdi], rax
    mov [rdi + 8], rdx

    sub QWORD PTR [rsp], 2
    lea rcx, [rcx + 16]
    lea rdi, [rdi + 16]
    jnz lab_outer

    POP_FULL
ASM_GSYM(lab_ret5):
    POP_EXTRA
ASM_GSYM(lab_ret2):
    POP_ALL
    ret
