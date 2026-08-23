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
# mp_limb_t lmmp_shr1add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
#
# mp_limb_t lmmp_shr1add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);



#include "lammp_asm.h"

.text

#ifdef LMMP_WINDOWS
.macro WIN_PROLOG
    push rsi
    push rdi
    push rbx
    mov rsi, rdx        # numa
    mov rdi, rcx        # dst
    mov rdx, r8         # numb
    mov rcx, r9         # n
.endm

.macro WIN_EPILOG
    pop rbx
    pop rdi
    pop rsi
    ret
.endm

.macro WIN_LOAD_C
    mov r8, [rsp + 64]
.endm
#else
.macro WIN_PROLOG
    push rbx
    mov rdi, rdi        # dst
    mov rsi, rsi        # numa
    mov rdx, rdx        # numb
    mov rcx, rcx        # n
.endm

.macro WIN_EPILOG
    pop rbx
    ret
.endm

.macro WIN_LOAD_C
    mov r8, r8          # c
.endm
#endif

#define ADCSBB adc

.text
.p2align 4

# ---------------------------------------------------------
# lmmp_shr1add_nc_
# ---------------------------------------------------------
.globl ASM_GSYM(lmmp_shr1add_nc_)
ASM_GSYM(lmmp_shr1add_nc_):
    WIN_PROLOG
    WIN_LOAD_C
    xor rax, rax
    neg r8
    mov rbx, [rsi]
    adc rbx, [rdx]
    jmp entry

.p2align 4

# ---------------------------------------------------------
# lmmp_shr1add_n_
# ---------------------------------------------------------
.globl ASM_GSYM(lmmp_shr1add_n_)
ASM_GSYM(lmmp_shr1add_n_):
    WIN_PROLOG
    xor rax, rax
    mov rbx, [rsi]
    add rbx, [rdx]

ASM_GSYM(entry):
    rcr rbx, 1
    adc rax, rax
    mov r11, rcx
    and r11, 3
    cmp r11, 1
    je do_quad

ASM_GSYM(n1):
    cmp r11, 2
    jne n2
    add rbx, rbx
    mov r10, [rsi+8]
    adc r10, [rdx+8]
    lea rsi, [rsi+8]
    lea rdx, [rdx+8]
    lea rdi, [rdi+8]
    rcr r10, 1
    rcr rbx, 1
    mov [rdi-8], rbx
    jmp cj1

ASM_GSYM(n2):
    cmp r11, 3
    jne n3
    add rbx, rbx
    mov r9, [rsi+8]
    mov r10, [rsi+16]
    adc r9, [rdx+8]
    adc r10, [rdx+16]
    lea rsi, [rsi+16]
    lea rdx, [rdx+16]
    lea rdi, [rdi+16]
    rcr r10, 1
    rcr r9, 1
    rcr rbx, 1
    mov [rdi-16], rbx
    jmp cj2

ASM_GSYM(n3):
    dec rcx
    add rbx, rbx
    mov r8, [rsi+8]
    mov r9, [rsi+16]
    adc r8, [rdx+8]
    adc r9, [rdx+16]
    mov r10, [rsi+24]
    adc r10, [rdx+24]
    lea rsi, [rsi+24]
    lea rdx, [rdx+24]
    lea rdi, [rdi+24]
    rcr r10, 1
    rcr r9, 1
    rcr r8, 1
    rcr rbx, 1
    mov [rdi-24], rbx
    mov [rdi-16], r8

ASM_GSYM(cj2):
    mov [rdi-8], r9
ASM_GSYM(cj1):
    mov rbx, r10

ASM_GSYM(do_quad):
    shr rcx, 2
    je end_loop

.p2align 4
ASM_GSYM(loop_top):
    add rbx, rbx
    mov r8, [rsi+8]
    mov r9, [rsi+16]
    adc r8, [rdx+8]
    adc r9, [rdx+16]
    mov r10, [rsi+24]
    mov r11, [rsi+32]
    adc r10, [rdx+24]
    adc r11, [rdx+32]
    lea rsi, [rsi+32]
    lea rdx, [rdx+32]
    rcr r11, 1
    rcr r10, 1
    rcr r9, 1
    rcr r8, 1
    rcr rbx, 1
    mov [rdi],    rbx
    mov [rdi+8],  r8
    mov [rdi+16], r9
    mov [rdi+24], r10
    mov rbx, r11
    lea rdi, [rdi+32]
    dec rcx
    jne loop_top

ASM_GSYM(end_loop):
    mov [rdi], rbx
    WIN_EPILOG
