//
//  Copyright (C) 2026 HJimmyK(Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it under
//  the terms of the GNU Lesser General Public License (LGPL) as published
//  by the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed WITHOUT ANY WARRANTY.
//
//  See <https://www.gnu.org/licenses/>.
//
//  本文件由 gen_hard.py 生成, 请勿手工修改; 重新生成: python gen_hard.py


//
// void lmmp_mul_hard_N_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb);
//
//   平衡硬编码乘法: [dst,2N] = [numa,N] * [numb,N], N 为编译期常量。
//   N <= 8 : FLINT 式寄存器整行累加(mulx+adcx/adox 双进位链)
//   N >= 9 : 种子 mul_1 + 展开 addmul_2(双乘数交错列)
//

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_mul_hard_1_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_1_):

    # SysV: rdi=dst rsi=numa rdx=numb | Win: rcx=dst rdx=numa r8=numb
#ifdef LMMP_WINDOWS
    mov     r10, [rdx]              // a0
    mov     rdx, [r8]               // b0 -> rdx (mulx 隐式乘数)
    mulx    r11, rax, r10           // a0*b0: r11=hi rax=lo
    mov     [rcx], rax
    mov     [rcx+8], r11
#else
    mov     rdx, [rdx]              // b0 -> rdx
    mulx    r11, rax, [rsi]         // a0*b0
    mov     [rdi], rax
    mov     [rdi+8], r11
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_2_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_2_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数
    # ring=[rax,r8,r9] scr=r10 zero=r11
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    r12
    push    r13
    push    r14
    push    r15
    xor     r11d, r11d                  // 清 CF/OF
    mov     rdx, [rcx+0]
    mulx    rax, r10, [rsi+0]
    mov     [rdi+0], r10
    mulx    r8, r10, [rsi+8]
    adcx    rax, r10
    adcx    r8, r11
    mov     rdx, [rcx+8]
    mulx    r9, r10, [rsi+0]
    adcx    rax, r10
    mov     [rdi+8], rax
    mulx    r10, rax, [rsi+8]
    adcx    r8, r9
    adox    r8, rax
    adcx    r10, r11
    adox    r10, r11
    mov     [rdi+16], r8
    mov     [rdi+24], r10
    pop     r15
    pop     r14
    pop     r13
    pop     r12
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_3_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_3_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数
    # ring=[rax,r8,r9,r10] scr=r11 zero=r12
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    r12
    push    r13
    push    r14
    push    r15
    xor     r12d, r12d                  // 清 CF/OF
    mov     rdx, [rcx+0]
    mulx    rax, r11, [rsi+0]
    mov     [rdi+0], r11
    mulx    r8, r11, [rsi+8]
    adcx    rax, r11
    mulx    r9, r11, [rsi+16]
    adcx    r8, r11
    adcx    r9, r12
    mov     rdx, [rcx+8]
    mulx    r10, r11, [rsi+0]
    adcx    rax, r11
    mov     [rdi+8], rax
    mulx    r11, rax, [rsi+8]
    adcx    r8, r10
    adox    r8, rax
    mulx    r10, rax, [rsi+16]
    adcx    r9, r11
    adox    r9, rax
    adcx    r10, r12
    adox    r10, r12
    mov     rdx, [rcx+16]
    mulx    rax, r11, [rsi+0]
    adcx    r8, r11
    mov     [rdi+16], r8
    mulx    r11, r8, [rsi+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rsi+16]
    adcx    r10, r11
    adox    r10, r8
    adcx    rax, r12
    adox    rax, r12
    mov     [rdi+24], r9
    mov     [rdi+32], r10
    mov     [rdi+40], rax
    pop     r15
    pop     r14
    pop     r13
    pop     r12
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_4_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_4_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数
    # ring=[rax,r8,r9,r10,r11] scr=r12 zero=r13
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    r12
    push    r13
    push    r14
    push    r15
    xor     r13d, r13d                  // 清 CF/OF
    mov     rdx, [rcx+0]
    mulx    rax, r12, [rsi+0]
    mov     [rdi+0], r12
    mulx    r8, r12, [rsi+8]
    adcx    rax, r12
    mulx    r9, r12, [rsi+16]
    adcx    r8, r12
    mulx    r10, r12, [rsi+24]
    adcx    r9, r12
    adcx    r10, r13
    mov     rdx, [rcx+8]
    mulx    r11, r12, [rsi+0]
    adcx    rax, r12
    mov     [rdi+8], rax
    mulx    r12, rax, [rsi+8]
    adcx    r8, r11
    adox    r8, rax
    mulx    r11, rax, [rsi+16]
    adcx    r9, r12
    adox    r9, rax
    mulx    r12, rax, [rsi+24]
    adcx    r10, r11
    adox    r10, rax
    adcx    r12, r13
    adox    r12, r13
    mov     rdx, [rcx+16]
    mulx    rax, r11, [rsi+0]
    adcx    r8, r11
    mov     [rdi+16], r8
    mulx    r11, r8, [rsi+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rsi+16]
    adcx    r10, r11
    adox    r10, r8
    mulx    r11, r8, [rsi+24]
    adcx    r12, rax
    adox    r12, r8
    adcx    r11, r13
    adox    r11, r13
    mov     rdx, [rcx+24]
    mulx    r8, rax, [rsi+0]
    adcx    r9, rax
    mov     [rdi+24], r9
    mulx    rax, r9, [rsi+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rsi+16]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rsi+24]
    adcx    r11, r8
    adox    r11, r9
    adcx    rax, r13
    adox    rax, r13
    mov     [rdi+32], r10
    mov     [rdi+40], r12
    mov     [rdi+48], r11
    mov     [rdi+56], rax
    pop     r15
    pop     r14
    pop     r13
    pop     r12
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_5_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_5_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数
    # ring=[rax,r8,r9,r10,r11,r12] scr=r13 zero=r14
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    r12
    push    r13
    push    r14
    push    r15
    xor     r14d, r14d                  // 清 CF/OF
    mov     rdx, [rcx+0]
    mulx    rax, r13, [rsi+0]
    mov     [rdi+0], r13
    mulx    r8, r13, [rsi+8]
    adcx    rax, r13
    mulx    r9, r13, [rsi+16]
    adcx    r8, r13
    mulx    r10, r13, [rsi+24]
    adcx    r9, r13
    mulx    r11, r13, [rsi+32]
    adcx    r10, r13
    adcx    r11, r14
    mov     rdx, [rcx+8]
    mulx    r12, r13, [rsi+0]
    adcx    rax, r13
    mov     [rdi+8], rax
    mulx    r13, rax, [rsi+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rsi+16]
    adcx    r9, r13
    adox    r9, rax
    mulx    r13, rax, [rsi+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rsi+32]
    adcx    r11, r13
    adox    r11, rax
    adcx    r12, r14
    adox    r12, r14
    mov     rdx, [rcx+16]
    mulx    rax, r13, [rsi+0]
    adcx    r8, r13
    mov     [rdi+16], r8
    mulx    r13, r8, [rsi+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rsi+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rsi+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rsi+32]
    adcx    r12, r13
    adox    r12, r8
    adcx    rax, r14
    adox    rax, r14
    mov     rdx, [rcx+24]
    mulx    r8, r13, [rsi+0]
    adcx    r9, r13
    mov     [rdi+24], r9
    mulx    r13, r9, [rsi+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rsi+16]
    adcx    r11, r13
    adox    r11, r9
    mulx    r13, r9, [rsi+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rsi+32]
    adcx    rax, r13
    adox    rax, r9
    adcx    r8, r14
    adox    r8, r14
    mov     rdx, [rcx+32]
    mulx    r9, r13, [rsi+0]
    adcx    r10, r13
    mov     [rdi+32], r10
    mulx    r13, r10, [rsi+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rsi+16]
    adcx    r12, r13
    adox    r12, r10
    mulx    r13, r10, [rsi+24]
    adcx    rax, r9
    adox    rax, r10
    mulx    r9, r10, [rsi+32]
    adcx    r8, r13
    adox    r8, r10
    adcx    r9, r14
    adox    r9, r14
    mov     [rdi+40], r11
    mov     [rdi+48], r12
    mov     [rdi+56], rax
    mov     [rdi+64], r8
    mov     [rdi+72], r9
    pop     r15
    pop     r14
    pop     r13
    pop     r12
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_6_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_6_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数
    # ring=[rax,r8,r9,r10,r11,r12,r13] scr=r14 zero=r15
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    r12
    push    r13
    push    r14
    push    r15
    xor     r15d, r15d                  // 清 CF/OF
    mov     rdx, [rcx+0]
    mulx    rax, r14, [rsi+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rsi+8]
    adcx    rax, r14
    mulx    r9, r14, [rsi+16]
    adcx    r8, r14
    mulx    r10, r14, [rsi+24]
    adcx    r9, r14
    mulx    r11, r14, [rsi+32]
    adcx    r10, r14
    mulx    r12, r14, [rsi+40]
    adcx    r11, r14
    adcx    r12, r15
    mov     rdx, [rcx+8]
    mulx    r13, r14, [rsi+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rsi+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rsi+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rsi+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rsi+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rsi+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, r15
    adox    r14, r15
    mov     rdx, [rcx+16]
    mulx    rax, r13, [rsi+0]
    adcx    r8, r13
    mov     [rdi+16], r8
    mulx    r13, r8, [rsi+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rsi+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rsi+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rsi+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rsi+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, r15
    adox    r13, r15
    mov     rdx, [rcx+24]
    mulx    r8, rax, [rsi+0]
    adcx    r9, rax
    mov     [rdi+24], r9
    mulx    rax, r9, [rsi+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rsi+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rsi+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rsi+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rsi+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, r15
    adox    rax, r15
    mov     rdx, [rcx+32]
    mulx    r9, r8, [rsi+0]
    adcx    r10, r8
    mov     [rdi+32], r10
    mulx    r8, r10, [rsi+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rsi+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rsi+24]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rsi+32]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rsi+40]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, r15
    adox    r8, r15
    mov     rdx, [rcx+40]
    mulx    r10, r9, [rsi+0]
    adcx    r11, r9
    mov     [rdi+40], r11
    mulx    r9, r11, [rsi+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rsi+16]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rsi+24]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rsi+32]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rsi+40]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, r15
    adox    r9, r15
    mov     [rdi+48], r12
    mov     [rdi+56], r14
    mov     [rdi+64], r13
    mov     [rdi+72], rax
    mov     [rdi+80], r8
    mov     [rdi+88], r9
    pop     r15
    pop     r14
    pop     r13
    pop     r12
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_7_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_7_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数
    # ring=[rax,r8,r9,r10,r11,r12,r13,r14] scr=r15 zero=rbx
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    mov     rdx, [rcx+0]
    mulx    rax, r15, [rsi+0]
    mov     [rdi+0], r15
    mulx    r8, r15, [rsi+8]
    adcx    rax, r15
    mulx    r9, r15, [rsi+16]
    adcx    r8, r15
    mulx    r10, r15, [rsi+24]
    adcx    r9, r15
    mulx    r11, r15, [rsi+32]
    adcx    r10, r15
    mulx    r12, r15, [rsi+40]
    adcx    r11, r15
    mulx    r13, r15, [rsi+48]
    adcx    r12, r15
    adcx    r13, rbx
    mov     rdx, [rcx+8]
    mulx    r14, r15, [rsi+0]
    adcx    rax, r15
    mov     [rdi+8], rax
    mulx    r15, rax, [rsi+8]
    adcx    r8, r14
    adox    r8, rax
    mulx    r14, rax, [rsi+16]
    adcx    r9, r15
    adox    r9, rax
    mulx    r15, rax, [rsi+24]
    adcx    r10, r14
    adox    r10, rax
    mulx    r14, rax, [rsi+32]
    adcx    r11, r15
    adox    r11, rax
    mulx    r15, rax, [rsi+40]
    adcx    r12, r14
    adox    r12, rax
    mulx    r14, rax, [rsi+48]
    adcx    r13, r15
    adox    r13, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rcx+16]
    mulx    rax, r15, [rsi+0]
    adcx    r8, r15
    mov     [rdi+16], r8
    mulx    r15, r8, [rsi+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rsi+16]
    adcx    r10, r15
    adox    r10, r8
    mulx    r15, r8, [rsi+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rsi+32]
    adcx    r12, r15
    adox    r12, r8
    mulx    r15, r8, [rsi+40]
    adcx    r13, rax
    adox    r13, r8
    mulx    rax, r8, [rsi+48]
    adcx    r14, r15
    adox    r14, r8
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rcx+24]
    mulx    r8, r15, [rsi+0]
    adcx    r9, r15
    mov     [rdi+24], r9
    mulx    r15, r9, [rsi+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rsi+16]
    adcx    r11, r15
    adox    r11, r9
    mulx    r15, r9, [rsi+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rsi+32]
    adcx    r13, r15
    adox    r13, r9
    mulx    r15, r9, [rsi+40]
    adcx    r14, r8
    adox    r14, r9
    mulx    r8, r9, [rsi+48]
    adcx    rax, r15
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rcx+32]
    mulx    r9, r15, [rsi+0]
    adcx    r10, r15
    mov     [rdi+32], r10
    mulx    r15, r10, [rsi+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rsi+16]
    adcx    r12, r15
    adox    r12, r10
    mulx    r15, r10, [rsi+24]
    adcx    r13, r9
    adox    r13, r10
    mulx    r9, r10, [rsi+32]
    adcx    r14, r15
    adox    r14, r10
    mulx    r15, r10, [rsi+40]
    adcx    rax, r9
    adox    rax, r10
    mulx    r9, r10, [rsi+48]
    adcx    r8, r15
    adox    r8, r10
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rcx+40]
    mulx    r10, r15, [rsi+0]
    adcx    r11, r15
    mov     [rdi+40], r11
    mulx    r15, r11, [rsi+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rsi+16]
    adcx    r13, r15
    adox    r13, r11
    mulx    r15, r11, [rsi+24]
    adcx    r14, r10
    adox    r14, r11
    mulx    r10, r11, [rsi+32]
    adcx    rax, r15
    adox    rax, r11
    mulx    r15, r11, [rsi+40]
    adcx    r8, r10
    adox    r8, r11
    mulx    r10, r11, [rsi+48]
    adcx    r9, r15
    adox    r9, r11
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rcx+48]
    mulx    r11, r15, [rsi+0]
    adcx    r12, r15
    mov     [rdi+48], r12
    mulx    r15, r12, [rsi+8]
    adcx    r13, r11
    adox    r13, r12
    mulx    r11, r12, [rsi+16]
    adcx    r14, r15
    adox    r14, r12
    mulx    r15, r12, [rsi+24]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rsi+32]
    adcx    r8, r15
    adox    r8, r12
    mulx    r15, r12, [rsi+40]
    adcx    r9, r11
    adox    r9, r12
    mulx    r11, r12, [rsi+48]
    adcx    r10, r15
    adox    r10, r12
    adcx    r11, rbx
    adox    r11, rbx
    mov     [rdi+56], r13
    mov     [rdi+64], r14
    mov     [rdi+72], rax
    mov     [rdi+80], r8
    mov     [rdi+88], r9
    mov     [rdi+96], r10
    mov     [rdi+104], r11
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_8_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_8_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数
    # ring=[rax,r8,r9,r10,r11,r12,r13,r14,r15] scr=rbx zero=rbp
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebp, ebp                  // 清 CF/OF
    mov     rdx, [rcx+0]
    mulx    rax, rbx, [rsi+0]
    mov     [rdi+0], rbx
    mulx    r8, rbx, [rsi+8]
    adcx    rax, rbx
    mulx    r9, rbx, [rsi+16]
    adcx    r8, rbx
    mulx    r10, rbx, [rsi+24]
    adcx    r9, rbx
    mulx    r11, rbx, [rsi+32]
    adcx    r10, rbx
    mulx    r12, rbx, [rsi+40]
    adcx    r11, rbx
    mulx    r13, rbx, [rsi+48]
    adcx    r12, rbx
    mulx    r14, rbx, [rsi+56]
    adcx    r13, rbx
    adcx    r14, rbp
    mov     rdx, [rcx+8]
    mulx    r15, rbx, [rsi+0]
    adcx    rax, rbx
    mov     [rdi+8], rax
    mulx    rbx, rax, [rsi+8]
    adcx    r8, r15
    adox    r8, rax
    mulx    r15, rax, [rsi+16]
    adcx    r9, rbx
    adox    r9, rax
    mulx    rbx, rax, [rsi+24]
    adcx    r10, r15
    adox    r10, rax
    mulx    r15, rax, [rsi+32]
    adcx    r11, rbx
    adox    r11, rax
    mulx    rbx, rax, [rsi+40]
    adcx    r12, r15
    adox    r12, rax
    mulx    r15, rax, [rsi+48]
    adcx    r13, rbx
    adox    r13, rax
    mulx    rbx, rax, [rsi+56]
    adcx    r14, r15
    adox    r14, rax
    adcx    rbx, rbp
    adox    rbx, rbp
    mov     rdx, [rcx+16]
    mulx    rax, r15, [rsi+0]
    adcx    r8, r15
    mov     [rdi+16], r8
    mulx    r15, r8, [rsi+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rsi+16]
    adcx    r10, r15
    adox    r10, r8
    mulx    r15, r8, [rsi+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rsi+32]
    adcx    r12, r15
    adox    r12, r8
    mulx    r15, r8, [rsi+40]
    adcx    r13, rax
    adox    r13, r8
    mulx    rax, r8, [rsi+48]
    adcx    r14, r15
    adox    r14, r8
    mulx    r15, r8, [rsi+56]
    adcx    rbx, rax
    adox    rbx, r8
    adcx    r15, rbp
    adox    r15, rbp
    mov     rdx, [rcx+24]
    mulx    r8, rax, [rsi+0]
    adcx    r9, rax
    mov     [rdi+24], r9
    mulx    rax, r9, [rsi+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rsi+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rsi+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rsi+32]
    adcx    r13, rax
    adox    r13, r9
    mulx    rax, r9, [rsi+40]
    adcx    r14, r8
    adox    r14, r9
    mulx    r8, r9, [rsi+48]
    adcx    rbx, rax
    adox    rbx, r9
    mulx    rax, r9, [rsi+56]
    adcx    r15, r8
    adox    r15, r9
    adcx    rax, rbp
    adox    rax, rbp
    mov     rdx, [rcx+32]
    mulx    r9, r8, [rsi+0]
    adcx    r10, r8
    mov     [rdi+32], r10
    mulx    r8, r10, [rsi+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rsi+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rsi+24]
    adcx    r13, r9
    adox    r13, r10
    mulx    r9, r10, [rsi+32]
    adcx    r14, r8
    adox    r14, r10
    mulx    r8, r10, [rsi+40]
    adcx    rbx, r9
    adox    rbx, r10
    mulx    r9, r10, [rsi+48]
    adcx    r15, r8
    adox    r15, r10
    mulx    r8, r10, [rsi+56]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbp
    adox    r8, rbp
    mov     rdx, [rcx+40]
    mulx    r10, r9, [rsi+0]
    adcx    r11, r9
    mov     [rdi+40], r11
    mulx    r9, r11, [rsi+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rsi+16]
    adcx    r13, r9
    adox    r13, r11
    mulx    r9, r11, [rsi+24]
    adcx    r14, r10
    adox    r14, r11
    mulx    r10, r11, [rsi+32]
    adcx    rbx, r9
    adox    rbx, r11
    mulx    r9, r11, [rsi+40]
    adcx    r15, r10
    adox    r15, r11
    mulx    r10, r11, [rsi+48]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rsi+56]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbp
    adox    r9, rbp
    mov     rdx, [rcx+48]
    mulx    r11, r10, [rsi+0]
    adcx    r12, r10
    mov     [rdi+48], r12
    mulx    r10, r12, [rsi+8]
    adcx    r13, r11
    adox    r13, r12
    mulx    r11, r12, [rsi+16]
    adcx    r14, r10
    adox    r14, r12
    mulx    r10, r12, [rsi+24]
    adcx    rbx, r11
    adox    rbx, r12
    mulx    r11, r12, [rsi+32]
    adcx    r15, r10
    adox    r15, r12
    mulx    r10, r12, [rsi+40]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rsi+48]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rsi+56]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbp
    adox    r10, rbp
    mov     rdx, [rcx+56]
    mulx    r12, r11, [rsi+0]
    adcx    r13, r11
    mov     [rdi+56], r13
    mulx    r11, r13, [rsi+8]
    adcx    r14, r12
    adox    r14, r13
    mulx    r12, r13, [rsi+16]
    adcx    rbx, r11
    adox    rbx, r13
    mulx    r11, r13, [rsi+24]
    adcx    r15, r12
    adox    r15, r13
    mulx    r12, r13, [rsi+32]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rsi+40]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rsi+48]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rsi+56]
    adcx    r10, r12
    adox    r10, r13
    adcx    r11, rbp
    adox    r11, rbp
    mov     [rdi+64], r14
    mov     [rdi+72], rbx
    mov     [rdi+80], r15
    mov     [rdi+88], rax
    mov     [rdi+96], r8
    mov     [rdi+104], r9
    mov     [rdi+112], r10
    mov     [rdi+120], r11
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_9_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_9_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..5) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+16], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+24], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+32], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+40], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r14, [rcx+0]
    adcx    r12, r14
    mov     [rdi+48], r12
    mulx    r14, r12, [rcx+8]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+16]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+24]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+32]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+0]
    adcx    r13, r14
    mov     [rdi+56], r13
    mulx    r14, r13, [rcx+8]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+16]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+24]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+32]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+64], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     [rdi+72], r8
    mov     [rdi+80], r9
    mov     [rdi+88], r10
    mov     [rdi+96], r11
    mov     [rdi+104], r12
    # === pass 1: numb[5..9) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]
    mulx    r8, r14, [rcx+40]
    mov     rdx, [rdi+40]
    add     rdx, r14
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r9, r14, [rcx+48]
    adcx    r8, r14
    mulx    r10, r14, [rcx+56]
    adcx    r9, r14
    mulx    r11, r14, [rcx+64]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+40]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+48]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+48], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, r8, [rcx+48]
    adcx    r9, r12
    adox    r9, r8
    mulx    r12, r8, [rcx+56]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+64]
    adcx    r11, r12
    adox    r11, r8
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    r8, r12, [rcx+40]
    adcx    r9, r12
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r12, r9, [rcx+48]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+56]
    adcx    r11, r12
    adox    r11, r9
    mulx    r12, r9, [rcx+64]
    adcx    r14, r8
    adox    r14, r9
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+24]
    mulx    r9, r8, [rcx+40]
    adcx    r10, r8
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r8, r10, [rcx+48]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+56]
    adcx    r14, r8
    adox    r14, r10
    mulx    r8, r10, [rcx+64]
    adcx    r12, r9
    adox    r12, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+32]
    mulx    r10, r9, [rcx+40]
    adcx    r11, r9
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r9, r11, [rcx+48]
    adcx    r14, r10
    adox    r14, r11
    mulx    r10, r11, [rcx+56]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+64]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+40]
    mulx    r11, r10, [rcx+40]
    adcx    r14, r10
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r10, r14, [rcx+48]
    adcx    r12, r11
    adox    r12, r14
    mulx    r11, r14, [rcx+56]
    adcx    r8, r10
    adox    r8, r14
    mulx    r10, r14, [rcx+64]
    adcx    r9, r11
    adox    r9, r14
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+48]
    mulx    r14, r11, [rcx+40]
    adcx    r12, r11
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r11, r12, [rcx+48]
    adcx    r8, r14
    adox    r8, r12
    mulx    r14, r12, [rcx+56]
    adcx    r9, r11
    adox    r9, r12
    mulx    r11, r12, [rcx+64]
    adcx    r10, r14
    adox    r10, r12
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+56]
    mulx    r12, r14, [rcx+40]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r8, [rcx+48]
    adcx    r9, r12
    adox    r9, r8
    mulx    r12, r8, [rcx+56]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+64]
    adcx    r11, r12
    adox    r11, r8
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+64]
    mulx    r8, r12, [rcx+40]
    adcx    r9, r12
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r12, r9, [rcx+48]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+56]
    adcx    r11, r12
    adox    r11, r9
    mulx    r12, r9, [rcx+64]
    adcx    r14, r8
    adox    r14, r9
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, r10
    add     rdx, rbp
    mov     [rdi+112], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+120], rdx
    mov     rdx, r14
    adc     rdx, 0
    mov     [rdi+128], rdx
    mov     rdx, r12
    adc     rdx, 0
    mov     [rdi+136], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_10_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_10_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..5) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+16], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+24], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+32], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+40], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r14, [rcx+0]
    adcx    r12, r14
    mov     [rdi+48], r12
    mulx    r14, r12, [rcx+8]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+16]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+24]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+32]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+0]
    adcx    r13, r14
    mov     [rdi+56], r13
    mulx    r14, r13, [rcx+8]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+16]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+24]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+32]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+64], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+72], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     [rdi+80], r9
    mov     [rdi+88], r10
    mov     [rdi+96], r11
    mov     [rdi+104], r12
    mov     [rdi+112], r13
    # === pass 1: numb[5..10) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    r9, r14, [rcx+40]
    mov     rdx, [rdi+40]
    add     rdx, r14
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r10, r14, [rcx+48]
    adcx    r9, r14
    mulx    r11, r14, [rcx+56]
    adcx    r10, r14
    mulx    r12, r14, [rcx+64]
    adcx    r11, r14
    mulx    r13, r14, [rcx+72]
    adcx    r12, r14
    adcx    r13, rbx
    mov     rdx, [rsi+8]
    mulx    rax, r14, [rcx+40]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+48]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+48], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, r9, [rcx+48]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+56]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+64]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+72]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+16]
    mulx    r8, r14, [rcx+40]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r14, r10, [rcx+48]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+56]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+64]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+72]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+24]
    mulx    r9, r14, [rcx+40]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r14, r11, [rcx+48]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+56]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+64]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+72]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+32]
    mulx    r10, r14, [rcx+40]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+56]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+64]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+72]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+40]
    mulx    r11, r14, [rcx+40]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r14, r13, [rcx+48]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+56]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+64]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+72]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+48]
    mulx    r12, r14, [rcx+40]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r14, rax, [rcx+48]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+56]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+64]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+72]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+56]
    mulx    r13, r14, [rcx+40]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r8, [rcx+48]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+56]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+64]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+72]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+64]
    mulx    rax, r14, [rcx+40]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r14, r9, [rcx+48]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+56]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+64]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+72]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+72]
    mulx    r8, r14, [rcx+40]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r14, r10, [rcx+48]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+56]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+64]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+72]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, r11
    add     rdx, rbp
    mov     [rdi+120], rdx
    mov     rdx, r12
    adc     rdx, 0
    mov     [rdi+128], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+136], rdx
    mov     rdx, rax
    adc     rdx, 0
    mov     [rdi+144], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+152], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_11_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_11_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..6) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    mulx    r12, r14, [rcx+40]
    adcx    r11, r14
    adcx    r12, rbx
    mov     rdx, [rsi+8]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+16], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    r8, rax, [rcx+0]
    adcx    r9, rax
    mov     [rdi+24], r9
    mulx    rax, r9, [rcx+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r9, r8, [rcx+0]
    adcx    r10, r8
    mov     [rdi+32], r10
    mulx    r8, r10, [rcx+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+24]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+32]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+40]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r10, r9, [rcx+0]
    adcx    r11, r9
    mov     [rdi+40], r11
    mulx    r9, r11, [rcx+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+16]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+24]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+32]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+40]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r11, r10, [rcx+0]
    adcx    r12, r10
    mov     [rdi+48], r12
    mulx    r10, r12, [rcx+8]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+16]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+24]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+32]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+40]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r12, r11, [rcx+0]
    adcx    r14, r11
    mov     [rdi+56], r14
    mulx    r11, r14, [rcx+8]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+16]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+24]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+32]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+40]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r14, r12, [rcx+0]
    adcx    r13, r12
    mov     [rdi+64], r13
    mulx    r12, r13, [rcx+8]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+16]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+24]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+32]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+40]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+72], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+80], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     [rdi+88], r9
    mov     [rdi+96], r10
    mov     [rdi+104], r11
    mov     [rdi+112], r12
    mov     [rdi+120], r14
    mov     [rdi+128], r13
    # === pass 1: numb[6..11) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    r9, rax, [rcx+48]
    mov     rdx, [rdi+48]
    add     rdx, rax
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r10, rax, [rcx+56]
    adcx    r9, rax
    mulx    r11, rax, [rcx+64]
    adcx    r10, rax
    mulx    r12, rax, [rcx+72]
    adcx    r11, rax
    mulx    r14, rax, [rcx+80]
    adcx    r12, rax
    adcx    r14, rbx
    mov     rdx, [rsi+8]
    mulx    r13, rax, [rcx+48]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    rax, r9, [rcx+56]
    adcx    r10, r13
    adox    r10, r9
    mulx    r13, r9, [rcx+64]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+72]
    adcx    r12, r13
    adox    r12, r9
    mulx    r13, r9, [rcx+80]
    adcx    r14, rax
    adox    r14, r9
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+16]
    mulx    r8, rax, [rcx+48]
    adcx    r10, rax
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    rax, r10, [rcx+56]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+64]
    adcx    r12, rax
    adox    r12, r10
    mulx    rax, r10, [rcx+72]
    adcx    r14, r8
    adox    r14, r10
    mulx    r8, r10, [rcx+80]
    adcx    r13, rax
    adox    r13, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+24]
    mulx    r9, rax, [rcx+48]
    adcx    r11, rax
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    rax, r11, [rcx+56]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+64]
    adcx    r14, rax
    adox    r14, r11
    mulx    rax, r11, [rcx+72]
    adcx    r13, r9
    adox    r13, r11
    mulx    r9, r11, [rcx+80]
    adcx    r8, rax
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+32]
    mulx    r10, rax, [rcx+48]
    adcx    r12, rax
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    rax, r12, [rcx+56]
    adcx    r14, r10
    adox    r14, r12
    mulx    r10, r12, [rcx+64]
    adcx    r13, rax
    adox    r13, r12
    mulx    rax, r12, [rcx+72]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+80]
    adcx    r9, rax
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+40]
    mulx    r11, rax, [rcx+48]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    rax, r14, [rcx+56]
    adcx    r13, r11
    adox    r13, r14
    mulx    r11, r14, [rcx+64]
    adcx    r8, rax
    adox    r8, r14
    mulx    rax, r14, [rcx+72]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+80]
    adcx    r10, rax
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+48]
    mulx    r12, rax, [rcx+48]
    adcx    r13, rax
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    rax, r13, [rcx+56]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+64]
    adcx    r9, rax
    adox    r9, r13
    mulx    rax, r13, [rcx+72]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+80]
    adcx    r11, rax
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+56]
    mulx    r14, rax, [rcx+48]
    adcx    r8, rax
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    rax, r8, [rcx+56]
    adcx    r9, r14
    adox    r9, r8
    mulx    r14, r8, [rcx+64]
    adcx    r10, rax
    adox    r10, r8
    mulx    rax, r8, [rcx+72]
    adcx    r11, r14
    adox    r11, r8
    mulx    r14, r8, [rcx+80]
    adcx    r12, rax
    adox    r12, r8
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+64]
    mulx    r13, rax, [rcx+48]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    rax, r9, [rcx+56]
    adcx    r10, r13
    adox    r10, r9
    mulx    r13, r9, [rcx+64]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+72]
    adcx    r12, r13
    adox    r12, r9
    mulx    r13, r9, [rcx+80]
    adcx    r14, rax
    adox    r14, r9
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+72]
    mulx    r8, rax, [rcx+48]
    adcx    r10, rax
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    rax, r10, [rcx+56]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+64]
    adcx    r12, rax
    adox    r12, r10
    mulx    rax, r10, [rcx+72]
    adcx    r14, r8
    adox    r14, r10
    mulx    r8, r10, [rcx+80]
    adcx    r13, rax
    adox    r13, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+80]
    mulx    r9, rax, [rcx+48]
    adcx    r11, rax
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    rax, r11, [rcx+56]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+64]
    adcx    r14, rax
    adox    r14, r11
    mulx    rax, r11, [rcx+72]
    adcx    r13, r9
    adox    r13, r11
    mulx    r9, r11, [rcx+80]
    adcx    r8, rax
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, r12
    add     rdx, rbp
    mov     [rdi+136], rdx
    mov     rdx, r14
    adc     rdx, 0
    mov     [rdi+144], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+152], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+160], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+168], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_12_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_12_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..6) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    mulx    r12, r14, [rcx+40]
    adcx    r11, r14
    adcx    r12, rbx
    mov     rdx, [rsi+8]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+16], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    r8, rax, [rcx+0]
    adcx    r9, rax
    mov     [rdi+24], r9
    mulx    rax, r9, [rcx+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r9, r8, [rcx+0]
    adcx    r10, r8
    mov     [rdi+32], r10
    mulx    r8, r10, [rcx+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+24]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+32]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+40]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r10, r9, [rcx+0]
    adcx    r11, r9
    mov     [rdi+40], r11
    mulx    r9, r11, [rcx+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+16]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+24]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+32]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+40]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r11, r10, [rcx+0]
    adcx    r12, r10
    mov     [rdi+48], r12
    mulx    r10, r12, [rcx+8]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+16]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+24]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+32]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+40]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r12, r11, [rcx+0]
    adcx    r14, r11
    mov     [rdi+56], r14
    mulx    r11, r14, [rcx+8]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+16]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+24]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+32]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+40]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r14, r12, [rcx+0]
    adcx    r13, r12
    mov     [rdi+64], r13
    mulx    r12, r13, [rcx+8]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+16]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+24]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+32]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+40]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+72], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+80], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    r8, rax, [rcx+0]
    adcx    r9, rax
    mov     [rdi+88], r9
    mulx    rax, r9, [rcx+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     [rdi+96], r10
    mov     [rdi+104], r11
    mov     [rdi+112], r12
    mov     [rdi+120], r14
    mov     [rdi+128], r13
    mov     [rdi+136], rax
    # === pass 1: numb[6..12) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]
    mulx    r10, r8, [rcx+48]
    mov     rdx, [rdi+48]
    add     rdx, r8
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r11, r8, [rcx+56]
    adcx    r10, r8
    mulx    r12, r8, [rcx+64]
    adcx    r11, r8
    mulx    r14, r8, [rcx+72]
    adcx    r12, r8
    mulx    r13, r8, [rcx+80]
    adcx    r14, r8
    mulx    rax, r8, [rcx+88]
    adcx    r13, r8
    adcx    rax, rbx
    mov     rdx, [rsi+8]
    mulx    r9, r8, [rcx+48]
    adcx    r10, r8
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r8, r10, [rcx+56]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+64]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+72]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+80]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+88]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+16]
    mulx    r10, r9, [rcx+48]
    adcx    r11, r9
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r9, r11, [rcx+56]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+64]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+72]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+80]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+88]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+24]
    mulx    r11, r10, [rcx+48]
    adcx    r12, r10
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r10, r12, [rcx+56]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+64]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+72]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+80]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+88]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+32]
    mulx    r12, r11, [rcx+48]
    adcx    r14, r11
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r11, r14, [rcx+56]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+64]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+72]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+80]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+88]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+40]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r12, r13, [rcx+56]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+64]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+72]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+80]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+88]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+48]
    mulx    r13, r14, [rcx+48]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r14, rax, [rcx+56]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+64]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+72]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+80]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+88]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+56]
    mulx    rax, r13, [rcx+48]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r13, r8, [rcx+56]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+64]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+72]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+80]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+88]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+64]
    mulx    r8, rax, [rcx+48]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    rax, r9, [rcx+56]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+64]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+72]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+80]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+88]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+72]
    mulx    r9, r8, [rcx+48]
    adcx    r10, r8
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r8, r10, [rcx+56]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+64]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+72]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+80]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+88]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+80]
    mulx    r10, r9, [rcx+48]
    adcx    r11, r9
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r9, r11, [rcx+56]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+64]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+72]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+80]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+88]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+88]
    mulx    r11, r10, [rcx+48]
    adcx    r12, r10
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r10, r12, [rcx+56]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+64]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+72]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+80]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+88]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, r14
    add     rdx, rbp
    mov     [rdi+144], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+152], rdx
    mov     rdx, rax
    adc     rdx, 0
    mov     [rdi+160], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+168], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+176], rdx
    mov     rdx, r10
    adc     rdx, 0
    mov     [rdi+184], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_13_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_13_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..5) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+16], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+24], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+32], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+40], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r14, [rcx+0]
    adcx    r12, r14
    mov     [rdi+48], r12
    mulx    r14, r12, [rcx+8]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+16]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+24]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+32]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+0]
    adcx    r13, r14
    mov     [rdi+56], r13
    mulx    r14, r13, [rcx+8]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+16]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+24]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+32]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+64], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+72], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+80], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+88]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+88], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+96], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     [rdi+104], r12
    mov     [rdi+112], r13
    mov     [rdi+120], rax
    mov     [rdi+128], r8
    mov     [rdi+136], r9
    # === pass 1: numb[5..9) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]
    mulx    r12, r14, [rcx+40]
    mov     rdx, [rdi+40]
    add     rdx, r14
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r13, r14, [rcx+48]
    adcx    r12, r14
    mulx    rax, r14, [rcx+56]
    adcx    r13, r14
    mulx    r8, r14, [rcx+64]
    adcx    rax, r14
    adcx    r8, rbx
    mov     rdx, [rsi+8]
    mulx    r9, r14, [rcx+40]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+48]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+48], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+56]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+64]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    r12, r9, [rcx+40]
    adcx    r13, r9
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r9, r13, [rcx+48]
    adcx    rax, r12
    adox    rax, r13
    mulx    r12, r13, [rcx+56]
    adcx    r8, r9
    adox    r8, r13
    mulx    r9, r13, [rcx+64]
    adcx    r14, r12
    adox    r14, r13
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+24]
    mulx    r13, r12, [rcx+40]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r12, rax, [rcx+48]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+56]
    adcx    r14, r12
    adox    r14, rax
    mulx    r12, rax, [rcx+64]
    adcx    r9, r13
    adox    r9, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+32]
    mulx    rax, r13, [rcx+40]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r13, r8, [rcx+48]
    adcx    r14, rax
    adox    r14, r8
    mulx    rax, r8, [rcx+56]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+64]
    adcx    r12, rax
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+40]
    mulx    r8, rax, [rcx+40]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    rax, r14, [rcx+48]
    adcx    r9, r8
    adox    r9, r14
    mulx    r8, r14, [rcx+56]
    adcx    r12, rax
    adox    r12, r14
    mulx    rax, r14, [rcx+64]
    adcx    r13, r8
    adox    r13, r14
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+48]
    mulx    r14, r8, [rcx+40]
    adcx    r9, r8
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r8, r9, [rcx+48]
    adcx    r12, r14
    adox    r12, r9
    mulx    r14, r9, [rcx+56]
    adcx    r13, r8
    adox    r13, r9
    mulx    r8, r9, [rcx+64]
    adcx    rax, r14
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+56]
    mulx    r9, r14, [rcx+40]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+56]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+64]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r9, [rcx+40]
    adcx    r13, r9
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r9, r13, [rcx+48]
    adcx    rax, r12
    adox    rax, r13
    mulx    r12, r13, [rcx+56]
    adcx    r8, r9
    adox    r8, r13
    mulx    r9, r13, [rcx+64]
    adcx    r14, r12
    adox    r14, r13
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r12, [rcx+40]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r12, rax, [rcx+48]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+56]
    adcx    r14, r12
    adox    r14, rax
    mulx    r12, rax, [rcx+64]
    adcx    r9, r13
    adox    r9, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+40]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r13, r8, [rcx+48]
    adcx    r14, rax
    adox    r14, r8
    mulx    rax, r8, [rcx+56]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+64]
    adcx    r12, rax
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    r8, rax, [rcx+40]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    rax, r14, [rcx+48]
    adcx    r9, r8
    adox    r9, r14
    mulx    r8, r14, [rcx+56]
    adcx    r12, rax
    adox    r12, r14
    mulx    rax, r14, [rcx+64]
    adcx    r13, r8
    adox    r13, r14
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r14, r8, [rcx+40]
    adcx    r9, r8
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r8, r9, [rcx+48]
    adcx    r12, r14
    adox    r12, r9
    mulx    r14, r9, [rcx+56]
    adcx    r13, r8
    adox    r13, r9
    mulx    r8, r9, [rcx+64]
    adcx    rax, r14
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, r12
    add     rdx, rbp
    mov     [rdi+144], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+152], rdx
    mov     rdx, rax
    adc     rdx, 0
    mov     [rdi+160], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+168], rdx
    # === pass 2: numb[9..13) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]
    mulx    r12, r14, [rcx+72]
    mov     rdx, [rdi+72]
    add     rdx, r14
    mov     [rdi+72], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r13, r14, [rcx+80]
    adcx    r12, r14
    mulx    rax, r14, [rcx+88]
    adcx    r13, r14
    mulx    r8, r14, [rcx+96]
    adcx    rax, r14
    adcx    r8, rbx
    mov     rdx, [rsi+8]
    mulx    r9, r14, [rcx+72]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, r12, [rcx+80]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+88]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+96]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    r12, r9, [rcx+72]
    adcx    r13, r9
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r9, r13, [rcx+80]
    adcx    rax, r12
    adox    rax, r13
    mulx    r12, r13, [rcx+88]
    adcx    r8, r9
    adox    r8, r13
    mulx    r9, r13, [rcx+96]
    adcx    r14, r12
    adox    r14, r13
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+24]
    mulx    r13, r12, [rcx+72]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r12, rax, [rcx+80]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+88]
    adcx    r14, r12
    adox    r14, rax
    mulx    r12, rax, [rcx+96]
    adcx    r9, r13
    adox    r9, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+32]
    mulx    rax, r13, [rcx+72]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r13, r8, [rcx+80]
    adcx    r14, rax
    adox    r14, r8
    mulx    rax, r8, [rcx+88]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+96]
    adcx    r12, rax
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+40]
    mulx    r8, rax, [rcx+72]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    rax, r14, [rcx+80]
    adcx    r9, r8
    adox    r9, r14
    mulx    r8, r14, [rcx+88]
    adcx    r12, rax
    adox    r12, r14
    mulx    rax, r14, [rcx+96]
    adcx    r13, r8
    adox    r13, r14
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+48]
    mulx    r14, r8, [rcx+72]
    adcx    r9, r8
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r8, r9, [rcx+80]
    adcx    r12, r14
    adox    r12, r9
    mulx    r14, r9, [rcx+88]
    adcx    r13, r8
    adox    r13, r9
    mulx    r8, r9, [rcx+96]
    adcx    rax, r14
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+56]
    mulx    r9, r14, [rcx+72]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r12, [rcx+80]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+88]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+96]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r9, [rcx+72]
    adcx    r13, r9
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r9, r13, [rcx+80]
    adcx    rax, r12
    adox    rax, r13
    mulx    r12, r13, [rcx+88]
    adcx    r8, r9
    adox    r8, r13
    mulx    r9, r13, [rcx+96]
    adcx    r14, r12
    adox    r14, r13
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r12, [rcx+72]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r12, rax, [rcx+80]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+88]
    adcx    r14, r12
    adox    r14, rax
    mulx    r12, rax, [rcx+96]
    adcx    r9, r13
    adox    r9, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+72]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r13, r8, [rcx+80]
    adcx    r14, rax
    adox    r14, r8
    mulx    rax, r8, [rcx+88]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+96]
    adcx    r12, rax
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    r8, rax, [rcx+72]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    rax, r14, [rcx+80]
    adcx    r9, r8
    adox    r9, r14
    mulx    r8, r14, [rcx+88]
    adcx    r12, rax
    adox    r12, r14
    mulx    rax, r14, [rcx+96]
    adcx    r13, r8
    adox    r13, r14
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r14, r8, [rcx+72]
    adcx    r9, r8
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r8, r9, [rcx+80]
    adcx    r12, r14
    adox    r12, r9
    mulx    r14, r9, [rcx+88]
    adcx    r13, r8
    adox    r13, r9
    mulx    r8, r9, [rcx+96]
    adcx    rax, r14
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, r12
    add     rdx, rbp
    mov     [rdi+176], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+184], rdx
    mov     rdx, rax
    adc     rdx, 0
    mov     [rdi+192], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+200], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_14_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_14_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..5) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+16], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+24], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+32], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+40], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r14, [rcx+0]
    adcx    r12, r14
    mov     [rdi+48], r12
    mulx    r14, r12, [rcx+8]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+16]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+24]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+32]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+0]
    adcx    r13, r14
    mov     [rdi+56], r13
    mulx    r14, r13, [rcx+8]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+16]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+24]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+32]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+64], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+72], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+80], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+88]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+88], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+96], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r14, [rcx+0]
    adcx    r12, r14
    mov     [rdi+104], r12
    mulx    r14, r12, [rcx+8]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+16]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+24]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+32]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     [rdi+112], r13
    mov     [rdi+120], rax
    mov     [rdi+128], r8
    mov     [rdi+136], r9
    mov     [rdi+144], r10
    # === pass 1: numb[5..10) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    r13, r14, [rcx+40]
    mov     rdx, [rdi+40]
    add     rdx, r14
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+48]
    adcx    r13, r14
    mulx    r8, r14, [rcx+56]
    adcx    rax, r14
    mulx    r9, r14, [rcx+64]
    adcx    r8, r14
    mulx    r10, r14, [rcx+72]
    adcx    r9, r14
    adcx    r10, rbx
    mov     rdx, [rsi+8]
    mulx    r11, r14, [rcx+40]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+48]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+48], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, r13, [rcx+48]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+56]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+64]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+72]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+16]
    mulx    r12, r14, [rcx+40]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r14, rax, [rcx+48]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+56]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+64]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+72]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+24]
    mulx    r13, r14, [rcx+40]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r14, r8, [rcx+48]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+56]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+64]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+72]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+32]
    mulx    rax, r14, [rcx+40]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r14, r9, [rcx+48]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+56]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+64]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+72]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+40]
    mulx    r8, r14, [rcx+40]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r14, r10, [rcx+48]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+56]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+64]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+72]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+48]
    mulx    r9, r14, [rcx+40]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r14, r11, [rcx+48]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+56]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+64]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+72]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+56]
    mulx    r10, r14, [rcx+40]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+56]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+64]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+72]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+64]
    mulx    r11, r14, [rcx+40]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r14, r13, [rcx+48]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+56]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+64]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+72]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+72]
    mulx    r12, r14, [rcx+40]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r14, rax, [rcx+48]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+56]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+64]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+72]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+80]
    mulx    r13, r14, [rcx+40]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r14, r8, [rcx+48]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+56]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+64]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+72]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    rax, r14, [rcx+40]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r14, r9, [rcx+48]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+56]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+64]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+72]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r8, r14, [rcx+40]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r14, r10, [rcx+48]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+56]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+64]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+72]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+104]
    mulx    r9, r14, [rcx+40]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r14, r11, [rcx+48]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+56]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+64]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+72]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, r12
    add     rdx, rbp
    mov     [rdi+152], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+160], rdx
    mov     rdx, rax
    adc     rdx, 0
    mov     [rdi+168], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+176], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+184], rdx
    # === pass 2: numb[10..14) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]
    mulx    r12, r14, [rcx+80]
    mov     rdx, [rdi+80]
    add     rdx, r14
    mov     [rdi+80], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r13, r14, [rcx+88]
    adcx    r12, r14
    mulx    rax, r14, [rcx+96]
    adcx    r13, r14
    mulx    r8, r14, [rcx+104]
    adcx    rax, r14
    adcx    r8, rbx
    mov     rdx, [rsi+8]
    mulx    r9, r14, [rcx+80]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, r12, [rcx+88]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+96]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+104]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    r12, r9, [rcx+80]
    adcx    r13, r9
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r9, r13, [rcx+88]
    adcx    rax, r12
    adox    rax, r13
    mulx    r12, r13, [rcx+96]
    adcx    r8, r9
    adox    r8, r13
    mulx    r9, r13, [rcx+104]
    adcx    r14, r12
    adox    r14, r13
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+24]
    mulx    r13, r12, [rcx+80]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r12, rax, [rcx+88]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+96]
    adcx    r14, r12
    adox    r14, rax
    mulx    r12, rax, [rcx+104]
    adcx    r9, r13
    adox    r9, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+32]
    mulx    rax, r13, [rcx+80]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r13, r8, [rcx+88]
    adcx    r14, rax
    adox    r14, r8
    mulx    rax, r8, [rcx+96]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+104]
    adcx    r12, rax
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+40]
    mulx    r8, rax, [rcx+80]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    rax, r14, [rcx+88]
    adcx    r9, r8
    adox    r9, r14
    mulx    r8, r14, [rcx+96]
    adcx    r12, rax
    adox    r12, r14
    mulx    rax, r14, [rcx+104]
    adcx    r13, r8
    adox    r13, r14
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+48]
    mulx    r14, r8, [rcx+80]
    adcx    r9, r8
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r8, r9, [rcx+88]
    adcx    r12, r14
    adox    r12, r9
    mulx    r14, r9, [rcx+96]
    adcx    r13, r8
    adox    r13, r9
    mulx    r8, r9, [rcx+104]
    adcx    rax, r14
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+56]
    mulx    r9, r14, [rcx+80]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r12, [rcx+88]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+96]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+104]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r9, [rcx+80]
    adcx    r13, r9
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r9, r13, [rcx+88]
    adcx    rax, r12
    adox    rax, r13
    mulx    r12, r13, [rcx+96]
    adcx    r8, r9
    adox    r8, r13
    mulx    r9, r13, [rcx+104]
    adcx    r14, r12
    adox    r14, r13
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r12, [rcx+80]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r12, rax, [rcx+88]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+96]
    adcx    r14, r12
    adox    r14, rax
    mulx    r12, rax, [rcx+104]
    adcx    r9, r13
    adox    r9, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+80]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r13, r8, [rcx+88]
    adcx    r14, rax
    adox    r14, r8
    mulx    rax, r8, [rcx+96]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+104]
    adcx    r12, rax
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    r8, rax, [rcx+80]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    rax, r14, [rcx+88]
    adcx    r9, r8
    adox    r9, r14
    mulx    r8, r14, [rcx+96]
    adcx    r12, rax
    adox    r12, r14
    mulx    rax, r14, [rcx+104]
    adcx    r13, r8
    adox    r13, r14
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r14, r8, [rcx+80]
    adcx    r9, r8
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r8, r9, [rcx+88]
    adcx    r12, r14
    adox    r12, r9
    mulx    r14, r9, [rcx+96]
    adcx    r13, r8
    adox    r13, r9
    mulx    r8, r9, [rcx+104]
    adcx    rax, r14
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+104]
    mulx    r9, r14, [rcx+80]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+184]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+184], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r14, r12, [rcx+88]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+96]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+104]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, r13
    add     rdx, rbp
    mov     [rdi+192], rdx
    mov     rdx, rax
    adc     rdx, 0
    mov     [rdi+200], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+208], rdx
    mov     rdx, r14
    adc     rdx, 0
    mov     [rdi+216], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_15_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_15_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..5) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+16], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+24], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+32], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+40], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r14, [rcx+0]
    adcx    r12, r14
    mov     [rdi+48], r12
    mulx    r14, r12, [rcx+8]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+16]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+24]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+32]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+0]
    adcx    r13, r14
    mov     [rdi+56], r13
    mulx    r14, r13, [rcx+8]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+16]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+24]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+32]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+64], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+72], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+80], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+88]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+88], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+96], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r14, [rcx+0]
    adcx    r12, r14
    mov     [rdi+104], r12
    mulx    r14, r12, [rcx+8]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+16]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+24]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+32]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r14, [rcx+0]
    adcx    r13, r14
    mov     [rdi+112], r13
    mulx    r14, r13, [rcx+8]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+16]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+24]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+32]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     [rdi+120], rax
    mov     [rdi+128], r8
    mov     [rdi+136], r9
    mov     [rdi+144], r10
    mov     [rdi+152], r11
    # === pass 1: numb[5..10) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+40]
    mov     rdx, [rdi+40]
    add     rdx, r14
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r8, r14, [rcx+48]
    adcx    rax, r14
    mulx    r9, r14, [rcx+56]
    adcx    r8, r14
    mulx    r10, r14, [rcx+64]
    adcx    r9, r14
    mulx    r11, r14, [rcx+72]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+40]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+48]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+48], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, rax, [rcx+48]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+56]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+64]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+72]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r14, [rcx+40]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r14, r8, [rcx+48]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+56]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+64]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+72]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r14, [rcx+40]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r14, r9, [rcx+48]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+56]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+64]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+72]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r14, [rcx+40]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r14, r10, [rcx+48]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+56]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+64]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+72]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r14, [rcx+40]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r14, r11, [rcx+48]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+56]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+64]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+72]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r14, [rcx+40]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+56]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+64]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+72]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+40]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r13, [rcx+48]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+56]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+64]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+72]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r14, [rcx+40]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r14, rax, [rcx+48]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+56]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+64]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+72]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+40]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r14, r8, [rcx+48]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+56]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+64]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+72]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r14, [rcx+40]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r14, r9, [rcx+48]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+56]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+64]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+72]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+88]
    mulx    r8, r14, [rcx+40]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r14, r10, [rcx+48]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+56]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+64]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+72]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r14, [rcx+40]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r14, r11, [rcx+48]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+56]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+64]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+72]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r14, [rcx+40]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+56]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+64]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+72]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r14, [rcx+40]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r14, r13, [rcx+48]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+56]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+64]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+72]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, rax
    add     rdx, rbp
    mov     [rdi+160], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+168], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+176], rdx
    mov     rdx, r10
    adc     rdx, 0
    mov     [rdi+184], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+192], rdx
    # === pass 2: numb[10..15) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+80]
    mov     rdx, [rdi+80]
    add     rdx, r14
    mov     [rdi+80], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r8, r14, [rcx+88]
    adcx    rax, r14
    mulx    r9, r14, [rcx+96]
    adcx    r8, r14
    mulx    r10, r14, [rcx+104]
    adcx    r9, r14
    mulx    r11, r14, [rcx+112]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+80]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, rax, [rcx+88]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+96]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+104]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+112]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r14, [rcx+80]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r14, r8, [rcx+88]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+96]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+104]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+112]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r14, [rcx+80]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r14, r9, [rcx+88]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+96]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+104]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+112]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r14, [rcx+80]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r14, r10, [rcx+88]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+96]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+104]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+112]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r14, [rcx+80]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r14, r11, [rcx+88]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+96]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+104]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+112]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r14, [rcx+80]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r14, r12, [rcx+88]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+96]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+104]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+112]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+80]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r13, [rcx+88]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+96]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+104]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+112]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r14, [rcx+80]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r14, rax, [rcx+88]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+96]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+104]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+112]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+80]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r14, r8, [rcx+88]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+96]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+104]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+112]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r14, [rcx+80]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r14, r9, [rcx+88]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+96]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+104]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+112]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+88]
    mulx    r8, r14, [rcx+80]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r14, r10, [rcx+88]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+96]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+104]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+112]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r14, [rcx+80]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r14, r11, [rcx+88]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+96]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+104]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+112]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r14, [rcx+80]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+184]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+184], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r14, r12, [rcx+88]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+96]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+104]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+112]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r14, [rcx+80]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+192]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+192], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r14, r13, [rcx+88]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+96]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+104]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+112]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, rax
    add     rdx, rbp
    mov     [rdi+200], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+208], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+216], rdx
    mov     rdx, r10
    adc     rdx, 0
    mov     [rdi+224], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+232], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_16_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_16_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..6) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    mulx    r12, r14, [rcx+40]
    adcx    r11, r14
    adcx    r12, rbx
    mov     rdx, [rsi+8]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+16], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    r8, rax, [rcx+0]
    adcx    r9, rax
    mov     [rdi+24], r9
    mulx    rax, r9, [rcx+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r9, r8, [rcx+0]
    adcx    r10, r8
    mov     [rdi+32], r10
    mulx    r8, r10, [rcx+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+24]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+32]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+40]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r10, r9, [rcx+0]
    adcx    r11, r9
    mov     [rdi+40], r11
    mulx    r9, r11, [rcx+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+16]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+24]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+32]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+40]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r11, r10, [rcx+0]
    adcx    r12, r10
    mov     [rdi+48], r12
    mulx    r10, r12, [rcx+8]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+16]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+24]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+32]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+40]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r12, r11, [rcx+0]
    adcx    r14, r11
    mov     [rdi+56], r14
    mulx    r11, r14, [rcx+8]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+16]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+24]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+32]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+40]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r14, r12, [rcx+0]
    adcx    r13, r12
    mov     [rdi+64], r13
    mulx    r12, r13, [rcx+8]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+16]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+24]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+32]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+40]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+72], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+80], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    r8, rax, [rcx+0]
    adcx    r9, rax
    mov     [rdi+88], r9
    mulx    rax, r9, [rcx+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r8, [rcx+0]
    adcx    r10, r8
    mov     [rdi+96], r10
    mulx    r8, r10, [rcx+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+24]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+32]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+40]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r9, [rcx+0]
    adcx    r11, r9
    mov     [rdi+104], r11
    mulx    r9, r11, [rcx+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+16]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+24]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+32]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+40]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r10, [rcx+0]
    adcx    r12, r10
    mov     [rdi+112], r12
    mulx    r10, r12, [rcx+8]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+16]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+24]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+32]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+40]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+120]
    mulx    r12, r11, [rcx+0]
    adcx    r14, r11
    mov     [rdi+120], r14
    mulx    r11, r14, [rcx+8]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+16]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+24]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+32]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+40]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     [rdi+128], r13
    mov     [rdi+136], rax
    mov     [rdi+144], r8
    mov     [rdi+152], r9
    mov     [rdi+160], r10
    mov     [rdi+168], r11
    # === pass 1: numb[6..11) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    r13, r12, [rcx+48]
    mov     rdx, [rdi+48]
    add     rdx, r12
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    rax, r12, [rcx+56]
    adcx    r13, r12
    mulx    r8, r12, [rcx+64]
    adcx    rax, r12
    mulx    r9, r12, [rcx+72]
    adcx    r8, r12
    mulx    r10, r12, [rcx+80]
    adcx    r9, r12
    adcx    r10, rbx
    mov     rdx, [rsi+8]
    mulx    r11, r12, [rcx+48]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r12, r13, [rcx+56]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+64]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+72]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+80]
    adcx    r10, r12
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+16]
    mulx    r14, r12, [rcx+48]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r12, rax, [rcx+56]
    adcx    r8, r14
    adox    r8, rax
    mulx    r14, rax, [rcx+64]
    adcx    r9, r12
    adox    r9, rax
    mulx    r12, rax, [rcx+72]
    adcx    r10, r14
    adox    r10, rax
    mulx    r14, rax, [rcx+80]
    adcx    r11, r12
    adox    r11, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+24]
    mulx    r13, r12, [rcx+48]
    adcx    r8, r12
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r12, r8, [rcx+56]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+64]
    adcx    r10, r12
    adox    r10, r8
    mulx    r12, r8, [rcx+72]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+80]
    adcx    r14, r12
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+32]
    mulx    rax, r12, [rcx+48]
    adcx    r9, r12
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r12, r9, [rcx+56]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+64]
    adcx    r11, r12
    adox    r11, r9
    mulx    r12, r9, [rcx+72]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+80]
    adcx    r13, r12
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+40]
    mulx    r8, r12, [rcx+48]
    adcx    r10, r12
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r12, r10, [rcx+56]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+64]
    adcx    r14, r12
    adox    r14, r10
    mulx    r12, r10, [rcx+72]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+80]
    adcx    rax, r12
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+48]
    mulx    r9, r12, [rcx+48]
    adcx    r11, r12
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r12, r11, [rcx+56]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+64]
    adcx    r13, r12
    adox    r13, r11
    mulx    r12, r11, [rcx+72]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+80]
    adcx    r8, r12
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+56]
    mulx    r10, r12, [rcx+48]
    adcx    r14, r12
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r12, r14, [rcx+56]
    adcx    r13, r10
    adox    r13, r14
    mulx    r10, r14, [rcx+64]
    adcx    rax, r12
    adox    rax, r14
    mulx    r12, r14, [rcx+72]
    adcx    r8, r10
    adox    r8, r14
    mulx    r10, r14, [rcx+80]
    adcx    r9, r12
    adox    r9, r14
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+64]
    mulx    r11, r12, [rcx+48]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r12, r13, [rcx+56]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+64]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+72]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+80]
    adcx    r10, r12
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+72]
    mulx    r14, r12, [rcx+48]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r12, rax, [rcx+56]
    adcx    r8, r14
    adox    r8, rax
    mulx    r14, rax, [rcx+64]
    adcx    r9, r12
    adox    r9, rax
    mulx    r12, rax, [rcx+72]
    adcx    r10, r14
    adox    r10, rax
    mulx    r14, rax, [rcx+80]
    adcx    r11, r12
    adox    r11, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+80]
    mulx    r13, r12, [rcx+48]
    adcx    r8, r12
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r12, r8, [rcx+56]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+64]
    adcx    r10, r12
    adox    r10, r8
    mulx    r12, r8, [rcx+72]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+80]
    adcx    r14, r12
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    rax, r12, [rcx+48]
    adcx    r9, r12
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r12, r9, [rcx+56]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+64]
    adcx    r11, r12
    adox    r11, r9
    mulx    r12, r9, [rcx+72]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+80]
    adcx    r13, r12
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r8, r12, [rcx+48]
    adcx    r10, r12
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r12, r10, [rcx+56]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+64]
    adcx    r14, r12
    adox    r14, r10
    mulx    r12, r10, [rcx+72]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+80]
    adcx    rax, r12
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+104]
    mulx    r9, r12, [rcx+48]
    adcx    r11, r12
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r12, r11, [rcx+56]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+64]
    adcx    r13, r12
    adox    r13, r11
    mulx    r12, r11, [rcx+72]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+80]
    adcx    r8, r12
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+112]
    mulx    r10, r12, [rcx+48]
    adcx    r14, r12
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r12, r14, [rcx+56]
    adcx    r13, r10
    adox    r13, r14
    mulx    r10, r14, [rcx+64]
    adcx    rax, r12
    adox    rax, r14
    mulx    r12, r14, [rcx+72]
    adcx    r8, r10
    adox    r8, r14
    mulx    r10, r14, [rcx+80]
    adcx    r9, r12
    adox    r9, r14
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+120]
    mulx    r11, r12, [rcx+48]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+120]
    mulx    r12, r13, [rcx+56]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+64]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+72]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+80]
    adcx    r10, r12
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, rax
    add     rdx, rbp
    mov     [rdi+176], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+184], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+192], rdx
    mov     rdx, r10
    adc     rdx, 0
    mov     [rdi+200], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+208], rdx
    # === pass 2: numb[11..16) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r12, [rcx+88]
    mov     rdx, [rdi+88]
    add     rdx, r12
    mov     [rdi+88], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r8, r12, [rcx+96]
    adcx    rax, r12
    mulx    r9, r12, [rcx+104]
    adcx    r8, r12
    mulx    r10, r12, [rcx+112]
    adcx    r9, r12
    mulx    r11, r12, [rcx+120]
    adcx    r10, r12
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r14, r12, [rcx+88]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r12, rax, [rcx+96]
    adcx    r8, r14
    adox    r8, rax
    mulx    r14, rax, [rcx+104]
    adcx    r9, r12
    adox    r9, rax
    mulx    r12, rax, [rcx+112]
    adcx    r10, r14
    adox    r10, rax
    mulx    r14, rax, [rcx+120]
    adcx    r11, r12
    adox    r11, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r12, [rcx+88]
    adcx    r8, r12
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r12, r8, [rcx+96]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+104]
    adcx    r10, r12
    adox    r10, r8
    mulx    r12, r8, [rcx+112]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+120]
    adcx    r14, r12
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r12, [rcx+88]
    adcx    r9, r12
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r12, r9, [rcx+96]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+104]
    adcx    r11, r12
    adox    r11, r9
    mulx    r12, r9, [rcx+112]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+120]
    adcx    r13, r12
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r12, [rcx+88]
    adcx    r10, r12
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r12, r10, [rcx+96]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+104]
    adcx    r14, r12
    adox    r14, r10
    mulx    r12, r10, [rcx+112]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+120]
    adcx    rax, r12
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r12, [rcx+88]
    adcx    r11, r12
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r12, r11, [rcx+96]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+104]
    adcx    r13, r12
    adox    r13, r11
    mulx    r12, r11, [rcx+112]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+120]
    adcx    r8, r12
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r12, [rcx+88]
    adcx    r14, r12
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r12, r14, [rcx+96]
    adcx    r13, r10
    adox    r13, r14
    mulx    r10, r14, [rcx+104]
    adcx    rax, r12
    adox    rax, r14
    mulx    r12, r14, [rcx+112]
    adcx    r8, r10
    adox    r8, r14
    mulx    r10, r14, [rcx+120]
    adcx    r9, r12
    adox    r9, r14
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r12, [rcx+88]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r12, r13, [rcx+96]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+104]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+112]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+120]
    adcx    r10, r12
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r14, r12, [rcx+88]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r12, rax, [rcx+96]
    adcx    r8, r14
    adox    r8, rax
    mulx    r14, rax, [rcx+104]
    adcx    r9, r12
    adox    r9, rax
    mulx    r12, rax, [rcx+112]
    adcx    r10, r14
    adox    r10, rax
    mulx    r14, rax, [rcx+120]
    adcx    r11, r12
    adox    r11, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r12, [rcx+88]
    adcx    r8, r12
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r12, r8, [rcx+96]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+104]
    adcx    r10, r12
    adox    r10, r8
    mulx    r12, r8, [rcx+112]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+120]
    adcx    r14, r12
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r12, [rcx+88]
    adcx    r9, r12
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r12, r9, [rcx+96]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+104]
    adcx    r11, r12
    adox    r11, r9
    mulx    r12, r9, [rcx+112]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+120]
    adcx    r13, r12
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+88]
    mulx    r8, r12, [rcx+88]
    adcx    r10, r12
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r12, r10, [rcx+96]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+104]
    adcx    r14, r12
    adox    r14, r10
    mulx    r12, r10, [rcx+112]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+120]
    adcx    rax, r12
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r12, [rcx+88]
    adcx    r11, r12
    setc    r15b
    mov     rdx, [rdi+184]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+184], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r12, r11, [rcx+96]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+104]
    adcx    r13, r12
    adox    r13, r11
    mulx    r12, r11, [rcx+112]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+120]
    adcx    r8, r12
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r12, [rcx+88]
    adcx    r14, r12
    setc    r15b
    mov     rdx, [rdi+192]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+192], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r12, r14, [rcx+96]
    adcx    r13, r10
    adox    r13, r14
    mulx    r10, r14, [rcx+104]
    adcx    rax, r12
    adox    rax, r14
    mulx    r12, r14, [rcx+112]
    adcx    r8, r10
    adox    r8, r14
    mulx    r10, r14, [rcx+120]
    adcx    r9, r12
    adox    r9, r14
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r12, [rcx+88]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+200]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+200], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r12, r13, [rcx+96]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+104]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+112]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+120]
    adcx    r10, r12
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+120]
    mulx    r14, r12, [rcx+88]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+208]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+208], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+120]
    mulx    r12, rax, [rcx+96]
    adcx    r8, r14
    adox    r8, rax
    mulx    r14, rax, [rcx+104]
    adcx    r9, r12
    adox    r9, rax
    mulx    r12, rax, [rcx+112]
    adcx    r10, r14
    adox    r10, rax
    mulx    r14, rax, [rcx+120]
    adcx    r11, r12
    adox    r11, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, r8
    add     rdx, rbp
    mov     [rdi+216], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+224], rdx
    mov     rdx, r10
    adc     rdx, 0
    mov     [rdi+232], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+240], rdx
    mov     rdx, r14
    adc     rdx, 0
    mov     [rdi+248], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_17_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_17_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..6) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    mulx    r12, r14, [rcx+40]
    adcx    r11, r14
    adcx    r12, rbx
    mov     rdx, [rsi+8]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+16], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    r8, rax, [rcx+0]
    adcx    r9, rax
    mov     [rdi+24], r9
    mulx    rax, r9, [rcx+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r9, r8, [rcx+0]
    adcx    r10, r8
    mov     [rdi+32], r10
    mulx    r8, r10, [rcx+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+24]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+32]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+40]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r10, r9, [rcx+0]
    adcx    r11, r9
    mov     [rdi+40], r11
    mulx    r9, r11, [rcx+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+16]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+24]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+32]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+40]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r11, r10, [rcx+0]
    adcx    r12, r10
    mov     [rdi+48], r12
    mulx    r10, r12, [rcx+8]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+16]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+24]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+32]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+40]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r12, r11, [rcx+0]
    adcx    r14, r11
    mov     [rdi+56], r14
    mulx    r11, r14, [rcx+8]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+16]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+24]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+32]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+40]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r14, r12, [rcx+0]
    adcx    r13, r12
    mov     [rdi+64], r13
    mulx    r12, r13, [rcx+8]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+16]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+24]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+32]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+40]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+72], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+80], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    r8, rax, [rcx+0]
    adcx    r9, rax
    mov     [rdi+88], r9
    mulx    rax, r9, [rcx+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r8, [rcx+0]
    adcx    r10, r8
    mov     [rdi+96], r10
    mulx    r8, r10, [rcx+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+24]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+32]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+40]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r9, [rcx+0]
    adcx    r11, r9
    mov     [rdi+104], r11
    mulx    r9, r11, [rcx+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+16]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+24]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+32]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+40]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r10, [rcx+0]
    adcx    r12, r10
    mov     [rdi+112], r12
    mulx    r10, r12, [rcx+8]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+16]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+24]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+32]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+40]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+120]
    mulx    r12, r11, [rcx+0]
    adcx    r14, r11
    mov     [rdi+120], r14
    mulx    r11, r14, [rcx+8]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+16]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+24]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+32]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+40]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+128]
    mulx    r14, r12, [rcx+0]
    adcx    r13, r12
    mov     [rdi+128], r13
    mulx    r12, r13, [rcx+8]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+16]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+24]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+32]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+40]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     [rdi+136], rax
    mov     [rdi+144], r8
    mov     [rdi+152], r9
    mov     [rdi+160], r10
    mov     [rdi+168], r11
    mov     [rdi+176], r12
    # === pass 1: numb[6..12) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+48]
    mov     rdx, [rdi+48]
    add     rdx, r14
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r8, r14, [rcx+56]
    adcx    rax, r14
    mulx    r9, r14, [rcx+64]
    adcx    r8, r14
    mulx    r10, r14, [rcx+72]
    adcx    r9, r14
    mulx    r11, r14, [rcx+80]
    adcx    r10, r14
    mulx    r12, r14, [rcx+88]
    adcx    r11, r14
    adcx    r12, rbx
    mov     rdx, [rsi+8]
    mulx    r13, r14, [rcx+48]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, rax, [rcx+56]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+64]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+72]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+80]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+88]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    rax, r13, [rcx+48]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r13, r8, [rcx+56]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+64]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+72]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+80]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+88]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    r8, rax, [rcx+48]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    rax, r9, [rcx+56]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+64]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+72]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+80]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+88]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r9, r8, [rcx+48]
    adcx    r10, r8
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r8, r10, [rcx+56]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+64]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+72]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+80]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+88]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r10, r9, [rcx+48]
    adcx    r11, r9
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r9, r11, [rcx+56]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+64]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+72]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+80]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+88]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r11, r10, [rcx+48]
    adcx    r12, r10
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r10, r12, [rcx+56]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+64]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+72]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+80]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+88]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r12, r11, [rcx+48]
    adcx    r14, r11
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+56]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+64]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+72]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+80]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+88]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r12, r13, [rcx+56]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+64]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+72]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+80]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+88]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+48]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r14, rax, [rcx+56]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+64]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+72]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+80]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+88]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+48]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r13, r8, [rcx+56]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+64]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+72]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+80]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+88]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    r8, rax, [rcx+48]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    rax, r9, [rcx+56]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+64]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+72]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+80]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+88]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r8, [rcx+48]
    adcx    r10, r8
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r8, r10, [rcx+56]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+64]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+72]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+80]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+88]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r9, [rcx+48]
    adcx    r11, r9
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r9, r11, [rcx+56]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+64]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+72]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+80]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+88]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r10, [rcx+48]
    adcx    r12, r10
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r10, r12, [rcx+56]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+64]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+72]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+80]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+88]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+120]
    mulx    r12, r11, [rcx+48]
    adcx    r14, r11
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+120]
    mulx    r11, r14, [rcx+56]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+64]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+72]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+80]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+88]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+128]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+128]
    mulx    r12, r13, [rcx+56]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+64]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+72]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+80]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+88]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, rax
    add     rdx, rbp
    mov     [rdi+184], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+192], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+200], rdx
    mov     rdx, r10
    adc     rdx, 0
    mov     [rdi+208], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+216], rdx
    mov     rdx, r12
    adc     rdx, 0
    mov     [rdi+224], rdx
    # === pass 2: numb[12..17) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+96]
    mov     rdx, [rdi+96]
    add     rdx, r14
    mov     [rdi+96], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r8, r14, [rcx+104]
    adcx    rax, r14
    mulx    r9, r14, [rcx+112]
    adcx    r8, r14
    mulx    r10, r14, [rcx+120]
    adcx    r9, r14
    mulx    r11, r14, [rcx+128]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+96]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, rax, [rcx+104]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+112]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+120]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+128]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r14, [rcx+96]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r14, r8, [rcx+104]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+112]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+120]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+128]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r14, [rcx+96]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r14, r9, [rcx+104]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+112]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+120]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+128]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r14, [rcx+96]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r14, r10, [rcx+104]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+112]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+120]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+128]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r14, [rcx+96]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r14, r11, [rcx+104]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+112]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+120]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+128]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r14, [rcx+96]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r14, r12, [rcx+104]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+112]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+120]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+128]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+96]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r13, [rcx+104]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+112]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+120]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+128]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r14, [rcx+96]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r14, rax, [rcx+104]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+112]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+120]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+128]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+96]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r14, r8, [rcx+104]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+112]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+120]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+128]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r14, [rcx+96]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r14, r9, [rcx+104]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+112]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+120]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+128]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+88]
    mulx    r8, r14, [rcx+96]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+184]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+184], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r14, r10, [rcx+104]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+112]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+120]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+128]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r14, [rcx+96]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+192]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+192], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r14, r11, [rcx+104]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+112]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+120]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+128]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r14, [rcx+96]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+200]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+200], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r14, r12, [rcx+104]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+112]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+120]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+128]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r14, [rcx+96]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+208]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+208], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r14, r13, [rcx+104]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+112]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+120]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+128]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+120]
    mulx    r12, r14, [rcx+96]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+216]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+216], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+120]
    mulx    r14, rax, [rcx+104]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+112]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+120]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+128]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+128]
    mulx    r13, r14, [rcx+96]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+224]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+224], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+128]
    mulx    r14, r8, [rcx+104]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+112]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+120]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+128]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, r9
    add     rdx, rbp
    mov     [rdi+232], rdx
    mov     rdx, r10
    adc     rdx, 0
    mov     [rdi+240], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+248], rdx
    mov     rdx, r12
    adc     rdx, 0
    mov     [rdi+256], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+264], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_18_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_18_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..6) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    mulx    r12, r14, [rcx+40]
    adcx    r11, r14
    adcx    r12, rbx
    mov     rdx, [rsi+8]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+16], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    r8, rax, [rcx+0]
    adcx    r9, rax
    mov     [rdi+24], r9
    mulx    rax, r9, [rcx+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r9, r8, [rcx+0]
    adcx    r10, r8
    mov     [rdi+32], r10
    mulx    r8, r10, [rcx+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+24]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+32]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+40]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r10, r9, [rcx+0]
    adcx    r11, r9
    mov     [rdi+40], r11
    mulx    r9, r11, [rcx+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+16]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+24]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+32]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+40]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r11, r10, [rcx+0]
    adcx    r12, r10
    mov     [rdi+48], r12
    mulx    r10, r12, [rcx+8]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+16]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+24]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+32]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+40]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r12, r11, [rcx+0]
    adcx    r14, r11
    mov     [rdi+56], r14
    mulx    r11, r14, [rcx+8]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+16]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+24]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+32]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+40]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r14, r12, [rcx+0]
    adcx    r13, r12
    mov     [rdi+64], r13
    mulx    r12, r13, [rcx+8]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+16]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+24]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+32]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+40]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+72], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+0]
    adcx    r8, r13
    mov     [rdi+80], r8
    mulx    r13, r8, [rcx+8]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+16]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+24]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+32]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+40]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    r8, rax, [rcx+0]
    adcx    r9, rax
    mov     [rdi+88], r9
    mulx    rax, r9, [rcx+8]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+16]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+24]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+32]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+40]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r8, [rcx+0]
    adcx    r10, r8
    mov     [rdi+96], r10
    mulx    r8, r10, [rcx+8]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+16]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+24]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+32]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+40]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r9, [rcx+0]
    adcx    r11, r9
    mov     [rdi+104], r11
    mulx    r9, r11, [rcx+8]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+16]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+24]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+32]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+40]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r10, [rcx+0]
    adcx    r12, r10
    mov     [rdi+112], r12
    mulx    r10, r12, [rcx+8]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+16]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+24]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+32]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+40]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+120]
    mulx    r12, r11, [rcx+0]
    adcx    r14, r11
    mov     [rdi+120], r14
    mulx    r11, r14, [rcx+8]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+16]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+24]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+32]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+40]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+128]
    mulx    r14, r12, [rcx+0]
    adcx    r13, r12
    mov     [rdi+128], r13
    mulx    r12, r13, [rcx+8]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+16]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+24]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+32]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+40]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+136]
    mulx    r13, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+136], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+40]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     [rdi+144], r8
    mov     [rdi+152], r9
    mov     [rdi+160], r10
    mov     [rdi+168], r11
    mov     [rdi+176], r12
    mov     [rdi+184], r14
    # === pass 1: numb[6..12) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]
    mulx    r8, r13, [rcx+48]
    mov     rdx, [rdi+48]
    add     rdx, r13
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r9, r13, [rcx+56]
    adcx    r8, r13
    mulx    r10, r13, [rcx+64]
    adcx    r9, r13
    mulx    r11, r13, [rcx+72]
    adcx    r10, r13
    mulx    r12, r13, [rcx+80]
    adcx    r11, r13
    mulx    r14, r13, [rcx+88]
    adcx    r12, r13
    adcx    r14, rbx
    mov     rdx, [rsi+8]
    mulx    rax, r13, [rcx+48]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r13, r8, [rcx+56]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+64]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+72]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+80]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+88]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+16]
    mulx    r8, rax, [rcx+48]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    rax, r9, [rcx+56]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+64]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+72]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+80]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+88]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+24]
    mulx    r9, r8, [rcx+48]
    adcx    r10, r8
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r8, r10, [rcx+56]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+64]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+72]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+80]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+88]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+32]
    mulx    r10, r9, [rcx+48]
    adcx    r11, r9
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r9, r11, [rcx+56]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+64]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+72]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+80]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+88]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+40]
    mulx    r11, r10, [rcx+48]
    adcx    r12, r10
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r10, r12, [rcx+56]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+64]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+72]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+80]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+88]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+48]
    mulx    r12, r11, [rcx+48]
    adcx    r14, r11
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r11, r14, [rcx+56]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+64]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+72]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+80]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+88]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+56]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r12, r13, [rcx+56]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+64]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+72]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+80]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+88]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+64]
    mulx    r13, r14, [rcx+48]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r14, rax, [rcx+56]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+64]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+72]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+80]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+88]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+72]
    mulx    rax, r13, [rcx+48]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r13, r8, [rcx+56]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+64]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+72]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+80]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+88]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+80]
    mulx    r8, rax, [rcx+48]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    rax, r9, [rcx+56]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+64]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+72]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+80]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+88]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+88]
    mulx    r9, r8, [rcx+48]
    adcx    r10, r8
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r8, r10, [rcx+56]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+64]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+72]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+80]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+88]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+96]
    mulx    r10, r9, [rcx+48]
    adcx    r11, r9
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r9, r11, [rcx+56]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+64]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+72]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+80]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+88]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+104]
    mulx    r11, r10, [rcx+48]
    adcx    r12, r10
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r10, r12, [rcx+56]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+64]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+72]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+80]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+88]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+112]
    mulx    r12, r11, [rcx+48]
    adcx    r14, r11
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r11, r14, [rcx+56]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+64]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+72]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+80]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+88]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+120]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+120]
    mulx    r12, r13, [rcx+56]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+64]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+72]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+80]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+88]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+128]
    mulx    r13, r14, [rcx+48]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+128]
    mulx    r14, rax, [rcx+56]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+64]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+72]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+80]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+88]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+136]
    mulx    rax, r13, [rcx+48]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+184]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+184], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+136]
    mulx    r13, r8, [rcx+56]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+64]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+72]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+80]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+88]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, r9
    add     rdx, rbp
    mov     [rdi+192], rdx
    mov     rdx, r10
    adc     rdx, 0
    mov     [rdi+200], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+208], rdx
    mov     rdx, r12
    adc     rdx, 0
    mov     [rdi+216], rdx
    mov     rdx, r14
    adc     rdx, 0
    mov     [rdi+224], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+232], rdx
    # === pass 2: numb[12..18) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]
    mulx    r9, rax, [rcx+96]
    mov     rdx, [rdi+96]
    add     rdx, rax
    mov     [rdi+96], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r10, rax, [rcx+104]
    adcx    r9, rax
    mulx    r11, rax, [rcx+112]
    adcx    r10, rax
    mulx    r12, rax, [rcx+120]
    adcx    r11, rax
    mulx    r14, rax, [rcx+128]
    adcx    r12, rax
    mulx    r13, rax, [rcx+136]
    adcx    r14, rax
    adcx    r13, rbx
    mov     rdx, [rsi+8]
    mulx    r8, rax, [rcx+96]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    rax, r9, [rcx+104]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+112]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+120]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+128]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+136]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+16]
    mulx    r9, r8, [rcx+96]
    adcx    r10, r8
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r8, r10, [rcx+104]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+112]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+120]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+128]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+136]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+24]
    mulx    r10, r9, [rcx+96]
    adcx    r11, r9
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r9, r11, [rcx+104]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+112]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+120]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+128]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+136]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+32]
    mulx    r11, r10, [rcx+96]
    adcx    r12, r10
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r10, r12, [rcx+104]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+112]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+120]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+128]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+136]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+40]
    mulx    r12, r11, [rcx+96]
    adcx    r14, r11
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r11, r14, [rcx+104]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+112]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+120]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+128]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+136]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+48]
    mulx    r14, r12, [rcx+96]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r12, r13, [rcx+104]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+112]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+120]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+128]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+136]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+56]
    mulx    r13, r14, [rcx+96]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, rax, [rcx+104]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+112]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+120]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+128]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+136]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+64]
    mulx    rax, r13, [rcx+96]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r13, r8, [rcx+104]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+112]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+120]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+128]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+136]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+72]
    mulx    r8, rax, [rcx+96]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    rax, r9, [rcx+104]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+112]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+120]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+128]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+136]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+80]
    mulx    r9, r8, [rcx+96]
    adcx    r10, r8
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r8, r10, [rcx+104]
    adcx    r11, r9
    adox    r11, r10
    mulx    r9, r10, [rcx+112]
    adcx    r12, r8
    adox    r12, r10
    mulx    r8, r10, [rcx+120]
    adcx    r14, r9
    adox    r14, r10
    mulx    r9, r10, [rcx+128]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+136]
    adcx    rax, r9
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+88]
    mulx    r10, r9, [rcx+96]
    adcx    r11, r9
    setc    r15b
    mov     rdx, [rdi+184]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+184], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r9, r11, [rcx+104]
    adcx    r12, r10
    adox    r12, r11
    mulx    r10, r11, [rcx+112]
    adcx    r14, r9
    adox    r14, r11
    mulx    r9, r11, [rcx+120]
    adcx    r13, r10
    adox    r13, r11
    mulx    r10, r11, [rcx+128]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+136]
    adcx    r8, r10
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+96]
    mulx    r11, r10, [rcx+96]
    adcx    r12, r10
    setc    r15b
    mov     rdx, [rdi+192]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+192], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r10, r12, [rcx+104]
    adcx    r14, r11
    adox    r14, r12
    mulx    r11, r12, [rcx+112]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+120]
    adcx    rax, r11
    adox    rax, r12
    mulx    r11, r12, [rcx+128]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+136]
    adcx    r9, r11
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+104]
    mulx    r12, r11, [rcx+96]
    adcx    r14, r11
    setc    r15b
    mov     rdx, [rdi+200]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+200], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r11, r14, [rcx+104]
    adcx    r13, r12
    adox    r13, r14
    mulx    r12, r14, [rcx+112]
    adcx    rax, r11
    adox    rax, r14
    mulx    r11, r14, [rcx+120]
    adcx    r8, r12
    adox    r8, r14
    mulx    r12, r14, [rcx+128]
    adcx    r9, r11
    adox    r9, r14
    mulx    r11, r14, [rcx+136]
    adcx    r10, r12
    adox    r10, r14
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+112]
    mulx    r14, r12, [rcx+96]
    adcx    r13, r12
    setc    r15b
    mov     rdx, [rdi+208]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+208], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r12, r13, [rcx+104]
    adcx    rax, r14
    adox    rax, r13
    mulx    r14, r13, [rcx+112]
    adcx    r8, r12
    adox    r8, r13
    mulx    r12, r13, [rcx+120]
    adcx    r9, r14
    adox    r9, r13
    mulx    r14, r13, [rcx+128]
    adcx    r10, r12
    adox    r10, r13
    mulx    r12, r13, [rcx+136]
    adcx    r11, r14
    adox    r11, r13
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+120]
    mulx    r13, r14, [rcx+96]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+216]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+216], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+120]
    mulx    r14, rax, [rcx+104]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+112]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+120]
    adcx    r10, r13
    adox    r10, rax
    mulx    r13, rax, [rcx+128]
    adcx    r11, r14
    adox    r11, rax
    mulx    r14, rax, [rcx+136]
    adcx    r12, r13
    adox    r12, rax
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+128]
    mulx    rax, r13, [rcx+96]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+224]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+224], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+128]
    mulx    r13, r8, [rcx+104]
    adcx    r9, rax
    adox    r9, r8
    mulx    rax, r8, [rcx+112]
    adcx    r10, r13
    adox    r10, r8
    mulx    r13, r8, [rcx+120]
    adcx    r11, rax
    adox    r11, r8
    mulx    rax, r8, [rcx+128]
    adcx    r12, r13
    adox    r12, r8
    mulx    r13, r8, [rcx+136]
    adcx    r14, rax
    adox    r14, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+136]
    mulx    r8, rax, [rcx+96]
    adcx    r9, rax
    setc    r15b
    mov     rdx, [rdi+232]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+232], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+136]
    mulx    rax, r9, [rcx+104]
    adcx    r10, r8
    adox    r10, r9
    mulx    r8, r9, [rcx+112]
    adcx    r11, rax
    adox    r11, r9
    mulx    rax, r9, [rcx+120]
    adcx    r12, r8
    adox    r12, r9
    mulx    r8, r9, [rcx+128]
    adcx    r14, rax
    adox    r14, r9
    mulx    rax, r9, [rcx+136]
    adcx    r13, r8
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, r10
    add     rdx, rbp
    mov     [rdi+240], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+248], rdx
    mov     rdx, r12
    adc     rdx, 0
    mov     [rdi+256], rdx
    mov     rdx, r14
    adc     rdx, 0
    mov     [rdi+264], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+272], rdx
    mov     rdx, rax
    adc     rdx, 0
    mov     [rdi+280], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_mul_hard_19_)
.p2align 4

ASM_GSYM(lmmp_mul_hard_19_):

    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
    mov     rcx, r8                 // numb
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    xor     ebx, ebx                  // 清 CF/OF
    # === pass 0: numb[0..5) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    rax, r14, [rcx+0]
    mov     [rdi+0], r14
    mulx    r8, r14, [rcx+8]
    adcx    rax, r14
    mulx    r9, r14, [rcx+16]
    adcx    r8, r14
    mulx    r10, r14, [rcx+24]
    adcx    r9, r14
    mulx    r11, r14, [rcx+32]
    adcx    r10, r14
    adcx    r11, rbx
    mov     rdx, [rsi+8]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+8], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+16]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+16], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+24]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+24], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+32]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+32], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+40]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+40], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+48]
    mulx    r10, r14, [rcx+0]
    adcx    r12, r14
    mov     [rdi+48], r12
    mulx    r14, r12, [rcx+8]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+16]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+24]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+32]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+56]
    mulx    r11, r14, [rcx+0]
    adcx    r13, r14
    mov     [rdi+56], r13
    mulx    r14, r13, [rcx+8]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+16]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+24]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+32]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+64], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+72], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+80], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+88]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+88], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+96]
    mulx    r9, r14, [rcx+0]
    adcx    r11, r14
    mov     [rdi+96], r11
    mulx    r14, r11, [rcx+8]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+16]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+24]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+32]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+104]
    mulx    r10, r14, [rcx+0]
    adcx    r12, r14
    mov     [rdi+104], r12
    mulx    r14, r12, [rcx+8]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+16]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+24]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+32]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+112]
    mulx    r11, r14, [rcx+0]
    adcx    r13, r14
    mov     [rdi+112], r13
    mulx    r14, r13, [rcx+8]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+16]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+24]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+32]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+120]
    mulx    r12, r14, [rcx+0]
    adcx    rax, r14
    mov     [rdi+120], rax
    mulx    r14, rax, [rcx+8]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+16]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+24]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+32]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+128]
    mulx    r13, r14, [rcx+0]
    adcx    r8, r14
    mov     [rdi+128], r8
    mulx    r14, r8, [rcx+8]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+16]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+24]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+32]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+136]
    mulx    rax, r14, [rcx+0]
    adcx    r9, r14
    mov     [rdi+136], r9
    mulx    r14, r9, [rcx+8]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+16]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+24]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+32]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+144]
    mulx    r8, r14, [rcx+0]
    adcx    r10, r14
    mov     [rdi+144], r10
    mulx    r14, r10, [rcx+8]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+16]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+24]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+32]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     [rdi+152], r11
    mov     [rdi+160], r12
    mov     [rdi+168], r13
    mov     [rdi+176], rax
    mov     [rdi+184], r8
    # === pass 1: numb[5..10) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    r11, r14, [rcx+40]
    mov     rdx, [rdi+40]
    add     rdx, r14
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r12, r14, [rcx+48]
    adcx    r11, r14
    mulx    r13, r14, [rcx+56]
    adcx    r12, r14
    mulx    rax, r14, [rcx+64]
    adcx    r13, r14
    mulx    r8, r14, [rcx+72]
    adcx    rax, r14
    adcx    r8, rbx
    mov     rdx, [rsi+8]
    mulx    r9, r14, [rcx+40]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+48]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+48], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, r11, [rcx+48]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+56]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+64]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+72]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+16]
    mulx    r10, r14, [rcx+40]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+56]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+56], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+56]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+64]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+72]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+24]
    mulx    r11, r14, [rcx+40]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+64]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+64], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r14, r13, [rcx+48]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+56]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+64]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+72]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+32]
    mulx    r12, r14, [rcx+40]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+72]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+72], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r14, rax, [rcx+48]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+56]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+64]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+72]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+40]
    mulx    r13, r14, [rcx+40]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+80]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+80], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r14, r8, [rcx+48]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+56]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+64]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+72]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+48]
    mulx    rax, r14, [rcx+40]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r14, r9, [rcx+48]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+56]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+64]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+72]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+56]
    mulx    r8, r14, [rcx+40]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r10, [rcx+48]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+56]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+64]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+72]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+64]
    mulx    r9, r14, [rcx+40]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r14, r11, [rcx+48]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+56]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+64]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+72]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+72]
    mulx    r10, r14, [rcx+40]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+56]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+64]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+72]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+80]
    mulx    r11, r14, [rcx+40]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r14, r13, [rcx+48]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+56]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+64]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+72]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+88]
    mulx    r12, r14, [rcx+40]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r14, rax, [rcx+48]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+56]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+64]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+72]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+96]
    mulx    r13, r14, [rcx+40]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r14, r8, [rcx+48]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+56]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+64]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+72]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+104]
    mulx    rax, r14, [rcx+40]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r14, r9, [rcx+48]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+56]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+64]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+72]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+112]
    mulx    r8, r14, [rcx+40]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r14, r10, [rcx+48]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+56]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+64]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+72]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+120]
    mulx    r9, r14, [rcx+40]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+120]
    mulx    r14, r11, [rcx+48]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+56]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+64]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+72]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+128]
    mulx    r10, r14, [rcx+40]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+128]
    mulx    r14, r12, [rcx+48]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+56]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+64]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+72]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+136]
    mulx    r11, r14, [rcx+40]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+136]
    mulx    r14, r13, [rcx+48]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+56]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+64]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+72]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+144]
    mulx    r12, r14, [rcx+40]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+184]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+184], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+144]
    mulx    r14, rax, [rcx+48]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+56]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+64]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+72]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, r8
    add     rdx, rbp
    mov     [rdi+192], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+200], rdx
    mov     rdx, r10
    adc     rdx, 0
    mov     [rdi+208], rdx
    mov     rdx, r11
    adc     rdx, 0
    mov     [rdi+216], rdx
    mov     rdx, r12
    adc     rdx, 0
    mov     [rdi+224], rdx
    # === pass 2: numb[10..15) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]
    mulx    r8, r14, [rcx+80]
    mov     rdx, [rdi+80]
    add     rdx, r14
    mov     [rdi+80], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r9, r14, [rcx+88]
    adcx    r8, r14
    mulx    r10, r14, [rcx+96]
    adcx    r9, r14
    mulx    r11, r14, [rcx+104]
    adcx    r10, r14
    mulx    r12, r14, [rcx+112]
    adcx    r11, r14
    adcx    r12, rbx
    mov     rdx, [rsi+8]
    mulx    r13, r14, [rcx+80]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+88]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+88], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, r8, [rcx+88]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+96]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+104]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+112]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+16]
    mulx    rax, r14, [rcx+80]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+96]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+96], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r14, r9, [rcx+88]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+96]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+104]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+112]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+24]
    mulx    r8, r14, [rcx+80]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+104]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+104], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r14, r10, [rcx+88]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+96]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+104]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+112]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+32]
    mulx    r9, r14, [rcx+80]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+112]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+112], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r14, r11, [rcx+88]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+96]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+104]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+112]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+40]
    mulx    r10, r14, [rcx+80]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+120]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+120], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    r14, r12, [rcx+88]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+96]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+104]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+112]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+48]
    mulx    r11, r14, [rcx+80]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r14, r13, [rcx+88]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+96]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+104]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+112]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+56]
    mulx    r12, r14, [rcx+80]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, rax, [rcx+88]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+96]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+104]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+112]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+64]
    mulx    r13, r14, [rcx+80]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r14, r8, [rcx+88]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+96]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+104]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+112]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+72]
    mulx    rax, r14, [rcx+80]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r14, r9, [rcx+88]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+96]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+104]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+112]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+80]
    mulx    r8, r14, [rcx+80]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r14, r10, [rcx+88]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+96]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+104]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+112]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+88]
    mulx    r9, r14, [rcx+80]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    r14, r11, [rcx+88]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+96]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+104]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+112]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+96]
    mulx    r10, r14, [rcx+80]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r14, r12, [rcx+88]
    adcx    r13, r10
    adox    r13, r12
    mulx    r10, r12, [rcx+96]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+104]
    adcx    r8, r10
    adox    r8, r12
    mulx    r10, r12, [rcx+112]
    adcx    r9, r14
    adox    r9, r12
    adcx    r10, rbx
    adox    r10, rbx
    mov     rdx, [rsi+104]
    mulx    r11, r14, [rcx+80]
    adcx    r13, r14
    setc    r15b
    mov     rdx, [rdi+184]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+184], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r14, r13, [rcx+88]
    adcx    rax, r11
    adox    rax, r13
    mulx    r11, r13, [rcx+96]
    adcx    r8, r14
    adox    r8, r13
    mulx    r14, r13, [rcx+104]
    adcx    r9, r11
    adox    r9, r13
    mulx    r11, r13, [rcx+112]
    adcx    r10, r14
    adox    r10, r13
    adcx    r11, rbx
    adox    r11, rbx
    mov     rdx, [rsi+112]
    mulx    r12, r14, [rcx+80]
    adcx    rax, r14
    setc    r15b
    mov     rdx, [rdi+192]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+192], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r14, rax, [rcx+88]
    adcx    r8, r12
    adox    r8, rax
    mulx    r12, rax, [rcx+96]
    adcx    r9, r14
    adox    r9, rax
    mulx    r14, rax, [rcx+104]
    adcx    r10, r12
    adox    r10, rax
    mulx    r12, rax, [rcx+112]
    adcx    r11, r14
    adox    r11, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+120]
    mulx    r13, r14, [rcx+80]
    adcx    r8, r14
    setc    r15b
    mov     rdx, [rdi+200]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+200], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+120]
    mulx    r14, r8, [rcx+88]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+96]
    adcx    r10, r14
    adox    r10, r8
    mulx    r14, r8, [rcx+104]
    adcx    r11, r13
    adox    r11, r8
    mulx    r13, r8, [rcx+112]
    adcx    r12, r14
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+128]
    mulx    rax, r14, [rcx+80]
    adcx    r9, r14
    setc    r15b
    mov     rdx, [rdi+208]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+208], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+128]
    mulx    r14, r9, [rcx+88]
    adcx    r10, rax
    adox    r10, r9
    mulx    rax, r9, [rcx+96]
    adcx    r11, r14
    adox    r11, r9
    mulx    r14, r9, [rcx+104]
    adcx    r12, rax
    adox    r12, r9
    mulx    rax, r9, [rcx+112]
    adcx    r13, r14
    adox    r13, r9
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+136]
    mulx    r8, r14, [rcx+80]
    adcx    r10, r14
    setc    r15b
    mov     rdx, [rdi+216]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r10
    mov     [rdi+216], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+136]
    mulx    r14, r10, [rcx+88]
    adcx    r11, r8
    adox    r11, r10
    mulx    r8, r10, [rcx+96]
    adcx    r12, r14
    adox    r12, r10
    mulx    r14, r10, [rcx+104]
    adcx    r13, r8
    adox    r13, r10
    mulx    r8, r10, [rcx+112]
    adcx    rax, r14
    adox    rax, r10
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+144]
    mulx    r9, r14, [rcx+80]
    adcx    r11, r14
    setc    r15b
    mov     rdx, [rdi+224]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r11
    mov     [rdi+224], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+144]
    mulx    r14, r11, [rcx+88]
    adcx    r12, r9
    adox    r12, r11
    mulx    r9, r11, [rcx+96]
    adcx    r13, r14
    adox    r13, r11
    mulx    r14, r11, [rcx+104]
    adcx    rax, r9
    adox    rax, r11
    mulx    r9, r11, [rcx+112]
    adcx    r8, r14
    adox    r8, r11
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, r12
    add     rdx, rbp
    mov     [rdi+232], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+240], rdx
    mov     rdx, rax
    adc     rdx, 0
    mov     [rdi+248], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+256], rdx
    mov     rdx, r9
    adc     rdx, 0
    mov     [rdi+264], rdx
    # === pass 3: numb[15..19) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]
    mulx    r12, r14, [rcx+120]
    mov     rdx, [rdi+120]
    add     rdx, r14
    mov     [rdi+120], rdx
    setc    bpl
    movzx   rbp, bpl
    xor     r15d, r15d
    mov     rdx, [rsi+0]
    mulx    r13, r14, [rcx+128]
    adcx    r12, r14
    mulx    rax, r14, [rcx+136]
    adcx    r13, r14
    mulx    r8, r14, [rcx+144]
    adcx    rax, r14
    adcx    r8, rbx
    mov     rdx, [rsi+8]
    mulx    r9, r14, [rcx+120]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+128]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+128], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+8]
    mulx    r14, r12, [rcx+128]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+136]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+144]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+16]
    mulx    r12, r9, [rcx+120]
    adcx    r13, r9
    setc    r15b
    mov     rdx, [rdi+136]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+136], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+16]
    mulx    r9, r13, [rcx+128]
    adcx    rax, r12
    adox    rax, r13
    mulx    r12, r13, [rcx+136]
    adcx    r8, r9
    adox    r8, r13
    mulx    r9, r13, [rcx+144]
    adcx    r14, r12
    adox    r14, r13
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+24]
    mulx    r13, r12, [rcx+120]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+144]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+144], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+24]
    mulx    r12, rax, [rcx+128]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+136]
    adcx    r14, r12
    adox    r14, rax
    mulx    r12, rax, [rcx+144]
    adcx    r9, r13
    adox    r9, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+32]
    mulx    rax, r13, [rcx+120]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+152]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+152], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+32]
    mulx    r13, r8, [rcx+128]
    adcx    r14, rax
    adox    r14, r8
    mulx    rax, r8, [rcx+136]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+144]
    adcx    r12, rax
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+40]
    mulx    r8, rax, [rcx+120]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+160]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+160], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+40]
    mulx    rax, r14, [rcx+128]
    adcx    r9, r8
    adox    r9, r14
    mulx    r8, r14, [rcx+136]
    adcx    r12, rax
    adox    r12, r14
    mulx    rax, r14, [rcx+144]
    adcx    r13, r8
    adox    r13, r14
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+48]
    mulx    r14, r8, [rcx+120]
    adcx    r9, r8
    setc    r15b
    mov     rdx, [rdi+168]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+168], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+48]
    mulx    r8, r9, [rcx+128]
    adcx    r12, r14
    adox    r12, r9
    mulx    r14, r9, [rcx+136]
    adcx    r13, r8
    adox    r13, r9
    mulx    r8, r9, [rcx+144]
    adcx    rax, r14
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+56]
    mulx    r9, r14, [rcx+120]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+176]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+176], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+56]
    mulx    r14, r12, [rcx+128]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+136]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+144]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+64]
    mulx    r12, r9, [rcx+120]
    adcx    r13, r9
    setc    r15b
    mov     rdx, [rdi+184]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+184], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+64]
    mulx    r9, r13, [rcx+128]
    adcx    rax, r12
    adox    rax, r13
    mulx    r12, r13, [rcx+136]
    adcx    r8, r9
    adox    r8, r13
    mulx    r9, r13, [rcx+144]
    adcx    r14, r12
    adox    r14, r13
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+72]
    mulx    r13, r12, [rcx+120]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+192]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+192], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+72]
    mulx    r12, rax, [rcx+128]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+136]
    adcx    r14, r12
    adox    r14, rax
    mulx    r12, rax, [rcx+144]
    adcx    r9, r13
    adox    r9, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+80]
    mulx    rax, r13, [rcx+120]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+200]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+200], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+80]
    mulx    r13, r8, [rcx+128]
    adcx    r14, rax
    adox    r14, r8
    mulx    rax, r8, [rcx+136]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+144]
    adcx    r12, rax
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+88]
    mulx    r8, rax, [rcx+120]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+208]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+208], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+88]
    mulx    rax, r14, [rcx+128]
    adcx    r9, r8
    adox    r9, r14
    mulx    r8, r14, [rcx+136]
    adcx    r12, rax
    adox    r12, r14
    mulx    rax, r14, [rcx+144]
    adcx    r13, r8
    adox    r13, r14
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+96]
    mulx    r14, r8, [rcx+120]
    adcx    r9, r8
    setc    r15b
    mov     rdx, [rdi+216]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+216], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+96]
    mulx    r8, r9, [rcx+128]
    adcx    r12, r14
    adox    r12, r9
    mulx    r14, r9, [rcx+136]
    adcx    r13, r8
    adox    r13, r9
    mulx    r8, r9, [rcx+144]
    adcx    rax, r14
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, [rsi+104]
    mulx    r9, r14, [rcx+120]
    adcx    r12, r14
    setc    r15b
    mov     rdx, [rdi+224]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r12
    mov     [rdi+224], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+104]
    mulx    r14, r12, [rcx+128]
    adcx    r13, r9
    adox    r13, r12
    mulx    r9, r12, [rcx+136]
    adcx    rax, r14
    adox    rax, r12
    mulx    r14, r12, [rcx+144]
    adcx    r8, r9
    adox    r8, r12
    adcx    r14, rbx
    adox    r14, rbx
    mov     rdx, [rsi+112]
    mulx    r12, r9, [rcx+120]
    adcx    r13, r9
    setc    r15b
    mov     rdx, [rdi+232]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r13
    mov     [rdi+232], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+112]
    mulx    r9, r13, [rcx+128]
    adcx    rax, r12
    adox    rax, r13
    mulx    r12, r13, [rcx+136]
    adcx    r8, r9
    adox    r8, r13
    mulx    r9, r13, [rcx+144]
    adcx    r14, r12
    adox    r14, r13
    adcx    r9, rbx
    adox    r9, rbx
    mov     rdx, [rsi+120]
    mulx    r13, r12, [rcx+120]
    adcx    rax, r12
    setc    r15b
    mov     rdx, [rdi+240]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, rax
    mov     [rdi+240], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+120]
    mulx    r12, rax, [rcx+128]
    adcx    r8, r13
    adox    r8, rax
    mulx    r13, rax, [rcx+136]
    adcx    r14, r12
    adox    r14, rax
    mulx    r12, rax, [rcx+144]
    adcx    r9, r13
    adox    r9, rax
    adcx    r12, rbx
    adox    r12, rbx
    mov     rdx, [rsi+128]
    mulx    rax, r13, [rcx+120]
    adcx    r8, r13
    setc    r15b
    mov     rdx, [rdi+248]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r8
    mov     [rdi+248], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+128]
    mulx    r13, r8, [rcx+128]
    adcx    r14, rax
    adox    r14, r8
    mulx    rax, r8, [rcx+136]
    adcx    r9, r13
    adox    r9, r8
    mulx    r13, r8, [rcx+144]
    adcx    r12, rax
    adox    r12, r8
    adcx    r13, rbx
    adox    r13, rbx
    mov     rdx, [rsi+136]
    mulx    r8, rax, [rcx+120]
    adcx    r14, rax
    setc    r15b
    mov     rdx, [rdi+256]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r14
    mov     [rdi+256], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+136]
    mulx    rax, r14, [rcx+128]
    adcx    r9, r8
    adox    r9, r14
    mulx    r8, r14, [rcx+136]
    adcx    r12, rax
    adox    r12, r14
    mulx    rax, r14, [rcx+144]
    adcx    r13, r8
    adox    r13, r14
    adcx    rax, rbx
    adox    rax, rbx
    mov     rdx, [rsi+144]
    mulx    r14, r8, [rcx+120]
    adcx    r9, r8
    setc    r15b
    mov     rdx, [rdi+264]
    add     rdx, rbp
    setc    bpl
    movzx   rbp, bpl
    add     rdx, r9
    mov     [rdi+264], rdx
    adc     rbp, 0
    add     rbp, r15
    xor     r15d, r15d
    mov     rdx, [rsi+144]
    mulx    r8, r9, [rcx+128]
    adcx    r12, r14
    adox    r12, r9
    mulx    r14, r9, [rcx+136]
    adcx    r13, r8
    adox    r13, r9
    mulx    r8, r9, [rcx+144]
    adcx    rax, r14
    adox    rax, r9
    adcx    r8, rbx
    adox    r8, rbx
    mov     rdx, r12
    add     rdx, rbp
    mov     [rdi+272], rdx
    mov     rdx, r13
    adc     rdx, 0
    mov     [rdi+280], rdx
    mov     rdx, rax
    adc     rdx, 0
    mov     [rdi+288], rdx
    mov     rdx, r8
    adc     rdx, 0
    mov     [rdi+296], rdx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret
