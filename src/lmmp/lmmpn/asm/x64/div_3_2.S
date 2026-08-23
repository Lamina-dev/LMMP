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
# mp_limb_t lmmp_div_3_2_(mp_ptr numa, mp_srcptr numb, mp_limb_t inv21);

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_div_3_2_)
ASM_GSYM(lmmp_div_3_2_):
    # Linux  : RDI=numa,  RSI=numb,  RDX=inv21
    # Windows: RCX=numa,  RDX=numb,  R8=inv21

    push rbx
    push rsi
    push rdi
    push r12
    push r13
    push r15

#ifdef LMMP_WINDOWS
    # Win RBX, RSI, RDI, R12-R15
    mov r15, rcx        # R15 = numa
    mov rsi, rdx        # RSI = numb
    mov rax, r8         # RAX = inv21
#else
    # Linux
    mov r15, rdi        # R15 = numa
   #mov rsi, rsi        # RSI = numb
    mov rax, rdx        # RAX = inv21
#endif

    mov r8,  [rsi]      # numb[0]
    mov r9,  [rsi+8]    # numb[1]
    mov r11, [r15]      # numa[0]
    mov r12, [r15+8]    # numa[1]
    mov r13, [r15+16]   # numa[2]

    mul r13             #rdx:rax=a2*(inv-2^64)
    mov r10, r12
    add r10, rax
    adc rdx, r13        #rdx:r10=a2*inv+a1=qh:ql
    mov r13, r10        #r13=ql
    mov r10, rdx        #r10=qh
    imul rdx, r9        #rdx=qh*bh m64
    xchg r13, r12       #r12=ql, r13=a1
    mov rax, r8         #rax=bh
    sub r13, rdx        #r13=a1-qh*bh m64
    xchg r12, r11       #r11=ql, r12=a0
    mul r10             #rdx:rax=qh*bl
    sub r12, r8
    sbb r13, r9         #r13:r12=a-qh*bh*2^64-b m128
    sub r12, rax
    sbb r13, rdx        #r13:r12=a-(qh+1)*bh*2^64-b m128 = z m128

    xor rax, rax
    xor rdx, rdx

    cmp r13, r11
    cmovnc rax, r8      #if(z m128<ql,d=0,-1)
    cmovnc rdx, r9
    adc r10, 0          #++qh, r13:r12=a-qh*b

    add r12, rax        #else(d=0,1)
    adc r13, rdx        #r13:r12+=b

    cmp r13, r9         #if(r13:r12>=b)
    jae .lab_fx         #r13:r12-=b, ++qh

.lab_ok:
    mov [r15],   r12
    mov [r15+8], r13
    mov rax, r10        #return qh

    pop r15
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx
    ret

.lab_fx:
    seta dl
    cmp r12, r8
    setae al
    or al, dl
    jz .lab_ok

    inc r10
    sub r12, r8
    sbb r13, r9
    jmp .lab_ok
