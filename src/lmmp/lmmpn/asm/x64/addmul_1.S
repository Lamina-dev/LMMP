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
# mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_addmul_1_)

ASM_GSYM(lmmp_addmul_1_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    # Windows: rcx=rp, rdx=up, r8=n, r9=vl
    mov     rdi, rcx                # rp
    mov     rsi, rdx                # up
    mov     rdx, r9                 # vl -> rdx（mulx 隐式乘数）
    # r8 = n
#else
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    # Linux: rdi=rp, rsi=up, rdx=n, rcx=vl
    mov     r8, rdx                 # n -> r8
    mov     rdx, rcx                # vl -> rdx
#endif

    xor     r9d, r9d                # carry = 0
    xor     r14d, r14d              # 零寄存器
    test    r8, r8
    jz      .Lret

    mov     rcx, r8
    and     r8d, 3                  # leftover = n & 3
    shr     rcx, 2                  # groups = n >> 2

    # ---- 处理余数 limb（标量，1~3 个）----
    test    r8, r8
    jz      .Lmain
    xor     eax, eax                # CF = 0, OF = 0

.Lscalar:
    mulx    r12, rax, [rsi]
    adcx    rax, [rdi]              # rax = rp[i] + lo; CF = o1
    adox    rax, r9                 # rax += carry; OF = o2
    mov     [rdi], rax
    adcx    r12, r14                # hi += o1
    adox    r12, r14                # hi += o2
    mov     r9, r12                 # carry = hi
    lea     rdi, [rdi+8]
    lea     rsi, [rsi+8]
    dec     r8
    jnz     .Lscalar

.Lmain:
    test    rcx, rcx
    jz      .Lret
    xor     eax, eax                # CF = 0, OF = 0

.p2align 4
.Lloop4:
    mulx    r12, rax, [rsi]         # hi0=r12, lo0=rax
    mulx    r13, rbx, [rsi+8]       # hi1=r13, lo1=rbx
    mulx    r15, rbp, [rsi+16]      # hi2=r15, lo2=rbp
    mulx    r11, r8,  [rsi+24]      # hi3=r11, lo3=r8

    adcx    rax, r9                 # lo0 += carry; CF = t0
    adox    r12, r14                # hi0 += 0; OF = 0
    adcx    rbx, r12                # lo1 += hi0 + t0; CF = t2
    adox    r13, r14                # hi1 += 0; OF = 0
    adcx    rbp, r13                # lo2 += hi1 + t2; CF = t4
    adox    r15, r14                # hi2 += 0; OF = 0
    adcx    r8, r15                 # lo3 += hi2 + t4; CF = t6
    adox    r11, r14                # hi3 += 0; OF = 0

    adcx    r11, r14                # hi3 += t6; CF = 0

    add     rax, [rdi]              # rax += rp[0]; CF = z0
    mov     [rdi], rax
    adcx    rbx, [rdi+8]            # CF = z1
    mov     [rdi+8], rbx
    adcx    rbp, [rdi+16]           # CF = z2
    mov     [rdi+16], rbp
    adcx    r8, [rdi+24]            # CF = z3
    mov     [rdi+24], r8

    adcx    r11, r14                # carry = hi3 + z3
    mov     r9, r11

    lea     rdi, [rdi+32]
    lea     rsi, [rsi+32]
    dec     rcx
    jnz     .Lloop4

.Lret:
    mov     rax, r9
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
