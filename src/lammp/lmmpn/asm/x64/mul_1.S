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
# mp_limb_t lmmp_mul_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x);



#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_mul_1_)

ASM_GSYM(lmmp_mul_1_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    push    rbx
    # Windows: rcx=dst, rdx=numa, r8=na, r9=x
    mov     rdi, rcx
    mov     rsi, rdx
    # r8 = na (unchanged)
    mov     rdx, r9                 # x -> rdx for MULX
#else
    push    rbx
    # Linux: rdi=dst, rsi=numa, rdx=na, rcx=x
    mov     r8, rdx                 # na -> r8
    mov     rdx, rcx                # x -> rdx
#endif

    lea     rsi, [rsi + r8*8]       # src end
    lea     rdi, [rdi + r8*8]       # dst end
    mov     r9, r8
    neg     r9                      # r9 = -na (negative index)
    xor     r10d, r10d              # carry = 0

    # Determine remainder (na % 4)
    mov     eax, r8d
    and     eax, 3
    jz      .check_post
    cmp     eax, 2
    je      .do_rem2
    jb      .do_rem1                # eax == 1
    # eax == 3

.do_rem3:
    mulx    r11, rax, [rsi + r9*8]
    mulx    r8,  rbx, [rsi + r9*8 + 8]
    add     rax, r10
    mov     [rdi + r9*8], rax
    adc     rbx, r11
    mov     [rdi + r9*8 + 8], rbx
    adc     r8, 0
    mov     r10, r8
    # third limb
    mulx    r11, rax, [rsi + r9*8 + 16]
    add     rax, r10
    mov     [rdi + r9*8 + 16], rax
    adc     r11, 0
    mov     r10, r11
    add     r9, 3
    jmp     .check_post

.do_rem2:
    mulx    r11, rax, [rsi + r9*8]
    mulx    r8,  rbx, [rsi + r9*8 + 8]
    add     rax, r10
    mov     [rdi + r9*8], rax
    adc     rbx, r11
    mov     [rdi + r9*8 + 8], rbx
    adc     r8, 0
    mov     r10, r8
    add     r9, 2
    jmp     .check_post

.do_rem1:
    mulx    r11, rax, [rsi + r9*8]
    add     rax, r10
    mov     [rdi + r9*8], rax
    adc     r11, 0
    mov     r10, r11
    add     r9, 1
    # fall through

.check_post:
    test    r9, r9
    jz      .done

.p2align 4
.loop4:
    mulx    r11, rax, [rsi + r9*8]          # p0 = a0*x
    mulx    r8,  rbx, [rsi + r9*8 + 8]      # p1 = a1*x
    add     rax, r10                        # sum0 = lo0 + carry, sets CF
    mov     [rdi + r9*8], rax
    adc     rbx, r11                        # sum1 = lo1 + hi0 + CF, sets CF
    mov     [rdi + r9*8 + 8], rbx
    mulx    r11, rax, [rsi + r9*8 + 16]     # p2 = a2*x
    adc     rax, r8                         # sum2 = lo2 + hi1 + CF, sets CF
    mov     [rdi + r9*8 + 16], rax
    mulx    r8,  rbx, [rsi + r9*8 + 24]     # p3 = a3*x
    adc     rbx, r11                        # sum3 = lo3 + hi2 + CF, sets CF
    mov     [rdi + r9*8 + 24], rbx
    adc     r8, 0                           # hi3 + CF
    mov     r10, r8                         # new carry
    add     r9, 4
    jnz     .loop4

.done:
    mov     rax, r10                        # return carry
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret
