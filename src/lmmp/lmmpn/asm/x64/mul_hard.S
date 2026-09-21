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
    mov     rdx, [rcx+0]            // 行乘数 0
    mulx    rax, r10, [rsi+0]       // hi0->rax lo0->r10
    mov     [rdi+0], r10           // dst[0] = lo0
    mulx    r8, r10, [rsi+8]     // hi1->r8 lo1->r10
    adcx    rax, r10                // col1 = hi0 + lo1
    adcx    r8, r11                  // col2 = hi1 + CF
    mov     rdx, [rcx+8]            // 行乘数 1
    mulx    r9, r10, [rsi+0]       // hi0->r9 lo0->r10
    adcx    rax, r10                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r10, rax, [rsi+8]     // hi1->r10 lo1->ring0
    adcx    r8, r9               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    adcx    r10, r11                 // 顶列 = hi1 + 进位
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
    mov     rdx, [rcx+0]            // 行乘数 0
    mulx    rax, r11, [rsi+0]       // hi0->rax lo0->r11
    mov     [rdi+0], r11           // dst[0] = lo0
    mulx    r8, r11, [rsi+8]     // hi1->r8 lo1->r11
    adcx    rax, r11                // col1 = hi0 + lo1
    mulx    r9, r11, [rsi+16]     // hi2->r9 lo2->r11
    adcx    r8, r11                // col2 = hi1 + lo2
    adcx    r9, r12                  // col3 = hi2 + CF
    mov     rdx, [rcx+8]            // 行乘数 1
    mulx    r10, r11, [rsi+0]       // hi0->r10 lo0->r11
    adcx    rax, r11                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r11, rax, [rsi+8]     // hi1->r11 lo1->ring0
    adcx    r8, r10               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r10, rax, [rsi+16]     // hi2->r10 lo2->ring0
    adcx    r9, r11               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    adcx    r10, r12                 // 顶列 = hi2 + 进位
    adox    r10, r12
    mov     rdx, [rcx+16]            // 行乘数 2
    mulx    rax, r11, [rsi+0]       // hi0->rax lo0->r11
    adcx    r8, r11                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r11, r8, [rsi+8]     // hi1->r11 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rsi+16]     // hi2->rax lo2->ring0
    adcx    r10, r11               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    adcx    rax, r12                 // 顶列 = hi2 + 进位
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
    mov     rdx, [rcx+0]            // 行乘数 0
    mulx    rax, r12, [rsi+0]       // hi0->rax lo0->r12
    mov     [rdi+0], r12           // dst[0] = lo0
    mulx    r8, r12, [rsi+8]     // hi1->r8 lo1->r12
    adcx    rax, r12                // col1 = hi0 + lo1
    mulx    r9, r12, [rsi+16]     // hi2->r9 lo2->r12
    adcx    r8, r12                // col2 = hi1 + lo2
    mulx    r10, r12, [rsi+24]     // hi3->r10 lo3->r12
    adcx    r9, r12                // col3 = hi2 + lo3
    adcx    r10, r13                  // col4 = hi3 + CF
    mov     rdx, [rcx+8]            // 行乘数 1
    mulx    r11, r12, [rsi+0]       // hi0->r11 lo0->r12
    adcx    rax, r12                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r12, rax, [rsi+8]     // hi1->r12 lo1->ring0
    adcx    r8, r11               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r11, rax, [rsi+16]     // hi2->r11 lo2->ring0
    adcx    r9, r12               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r12, rax, [rsi+24]     // hi3->r12 lo3->ring0
    adcx    r10, r11               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    adcx    r12, r13                 // 顶列 = hi3 + 进位
    adox    r12, r13
    mov     rdx, [rcx+16]            // 行乘数 2
    mulx    rax, r11, [rsi+0]       // hi0->rax lo0->r11
    adcx    r8, r11                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r11, r8, [rsi+8]     // hi1->r11 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rsi+16]     // hi2->rax lo2->ring0
    adcx    r10, r11               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r11, r8, [rsi+24]     // hi3->r11 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r11, r13                 // 顶列 = hi3 + 进位
    adox    r11, r13
    mov     rdx, [rcx+24]            // 行乘数 3
    mulx    r8, rax, [rsi+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    rax, r9, [rsi+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rsi+16]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r9               // OF 链: += lo2
    mulx    rax, r9, [rsi+24]     // hi3->rax lo3->ring0
    adcx    r11, r8               // CF 链: += hi2
    adox    r11, r9               // OF 链: += lo3
    adcx    rax, r13                 // 顶列 = hi3 + 进位
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
    mov     rdx, [rcx+0]            // 行乘数 0
    mulx    rax, r13, [rsi+0]       // hi0->rax lo0->r13
    mov     [rdi+0], r13           // dst[0] = lo0
    mulx    r8, r13, [rsi+8]     // hi1->r8 lo1->r13
    adcx    rax, r13                // col1 = hi0 + lo1
    mulx    r9, r13, [rsi+16]     // hi2->r9 lo2->r13
    adcx    r8, r13                // col2 = hi1 + lo2
    mulx    r10, r13, [rsi+24]     // hi3->r10 lo3->r13
    adcx    r9, r13                // col3 = hi2 + lo3
    mulx    r11, r13, [rsi+32]     // hi4->r11 lo4->r13
    adcx    r10, r13                // col4 = hi3 + lo4
    adcx    r11, r14                  // col5 = hi4 + CF
    mov     rdx, [rcx+8]            // 行乘数 1
    mulx    r12, r13, [rsi+0]       // hi0->r12 lo0->r13
    adcx    rax, r13                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r13, rax, [rsi+8]     // hi1->r13 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rsi+16]     // hi2->r12 lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r13, rax, [rsi+24]     // hi3->r13 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rsi+32]     // hi4->r12 lo4->ring0
    adcx    r11, r13               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, r14                 // 顶列 = hi4 + 进位
    adox    r12, r14
    mov     rdx, [rcx+16]            // 行乘数 2
    mulx    rax, r13, [rsi+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r13, r8, [rsi+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rsi+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rsi+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rsi+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    rax, r14                 // 顶列 = hi4 + 进位
    adox    rax, r14
    mov     rdx, [rcx+24]            // 行乘数 3
    mulx    r8, r13, [rsi+0]       // hi0->r8 lo0->r13
    adcx    r9, r13                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    r13, r9, [rsi+8]     // hi1->r13 lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rsi+16]     // hi2->r8 lo2->ring0
    adcx    r11, r13               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r13, r9, [rsi+24]     // hi3->r13 lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rsi+32]     // hi4->r8 lo4->ring0
    adcx    rax, r13               // CF 链: += hi3
    adox    rax, r9               // OF 链: += lo4
    adcx    r8, r14                 // 顶列 = hi4 + 进位
    adox    r8, r14
    mov     rdx, [rcx+32]            // 行乘数 4
    mulx    r9, r13, [rsi+0]       // hi0->r9 lo0->r13
    adcx    r10, r13                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r13, r10, [rsi+8]     // hi1->r13 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rsi+16]     // hi2->r9 lo2->ring0
    adcx    r12, r13               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r13, r10, [rsi+24]     // hi3->r13 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r10               // OF 链: += lo3
    mulx    r9, r10, [rsi+32]     // hi4->r9 lo4->ring0
    adcx    r8, r13               // CF 链: += hi3
    adox    r8, r10               // OF 链: += lo4
    adcx    r9, r14                 // 顶列 = hi4 + 进位
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
    mov     rdx, [rcx+0]            // 行乘数 0
    mulx    rax, r14, [rsi+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rsi+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rsi+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rsi+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rsi+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    mulx    r12, r14, [rsi+40]     // hi5->r12 lo5->r14
    adcx    r11, r14                // col5 = hi4 + lo5
    adcx    r12, r15                  // col6 = hi5 + CF
    mov     rdx, [rcx+8]            // 行乘数 1
    mulx    r13, r14, [rsi+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rsi+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rsi+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rsi+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rsi+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rsi+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, r15                 // 顶列 = hi5 + 进位
    adox    r14, r15
    mov     rdx, [rcx+16]            // 行乘数 2
    mulx    rax, r13, [rsi+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r13, r8, [rsi+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rsi+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rsi+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rsi+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rsi+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, r15                 // 顶列 = hi5 + 进位
    adox    r13, r15
    mov     rdx, [rcx+24]            // 行乘数 3
    mulx    r8, rax, [rsi+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    rax, r9, [rsi+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rsi+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rsi+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rsi+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rsi+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, r15                 // 顶列 = hi5 + 进位
    adox    rax, r15
    mov     rdx, [rcx+32]            // 行乘数 4
    mulx    r9, r8, [rsi+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r8, r10, [rsi+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rsi+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rsi+24]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rsi+32]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rsi+40]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, r15                 // 顶列 = hi5 + 进位
    adox    r8, r15
    mov     rdx, [rcx+40]            // 行乘数 5
    mulx    r10, r9, [rsi+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r9, r11, [rsi+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rsi+16]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rsi+24]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rsi+32]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rsi+40]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, r15                 // 顶列 = hi5 + 进位
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
    mov     rdx, [rcx+0]            // 行乘数 0
    mulx    rax, r15, [rsi+0]       // hi0->rax lo0->r15
    mov     [rdi+0], r15           // dst[0] = lo0
    mulx    r8, r15, [rsi+8]     // hi1->r8 lo1->r15
    adcx    rax, r15                // col1 = hi0 + lo1
    mulx    r9, r15, [rsi+16]     // hi2->r9 lo2->r15
    adcx    r8, r15                // col2 = hi1 + lo2
    mulx    r10, r15, [rsi+24]     // hi3->r10 lo3->r15
    adcx    r9, r15                // col3 = hi2 + lo3
    mulx    r11, r15, [rsi+32]     // hi4->r11 lo4->r15
    adcx    r10, r15                // col4 = hi3 + lo4
    mulx    r12, r15, [rsi+40]     // hi5->r12 lo5->r15
    adcx    r11, r15                // col5 = hi4 + lo5
    mulx    r13, r15, [rsi+48]     // hi6->r13 lo6->r15
    adcx    r12, r15                // col6 = hi5 + lo6
    adcx    r13, rbx                  // col7 = hi6 + CF
    mov     rdx, [rcx+8]            // 行乘数 1
    mulx    r14, r15, [rsi+0]       // hi0->r14 lo0->r15
    adcx    rax, r15                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r15, rax, [rsi+8]     // hi1->r15 lo1->ring0
    adcx    r8, r14               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r14, rax, [rsi+16]     // hi2->r14 lo2->ring0
    adcx    r9, r15               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r15, rax, [rsi+24]     // hi3->r15 lo3->ring0
    adcx    r10, r14               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r14, rax, [rsi+32]     // hi4->r14 lo4->ring0
    adcx    r11, r15               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r15, rax, [rsi+40]     // hi5->r15 lo5->ring0
    adcx    r12, r14               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    mulx    r14, rax, [rsi+48]     // hi6->r14 lo6->ring0
    adcx    r13, r15               // CF 链: += hi5
    adox    r13, rax               // OF 链: += lo6
    adcx    r14, rbx                 // 顶列 = hi6 + 进位
    adox    r14, rbx
    mov     rdx, [rcx+16]            // 行乘数 2
    mulx    rax, r15, [rsi+0]       // hi0->rax lo0->r15
    adcx    r8, r15                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r15, r8, [rsi+8]     // hi1->r15 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rsi+16]     // hi2->rax lo2->ring0
    adcx    r10, r15               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r15, r8, [rsi+24]     // hi3->r15 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rsi+32]     // hi4->rax lo4->ring0
    adcx    r12, r15               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r15, r8, [rsi+40]     // hi5->r15 lo5->ring0
    adcx    r13, rax               // CF 链: += hi4
    adox    r13, r8               // OF 链: += lo5
    mulx    rax, r8, [rsi+48]     // hi6->rax lo6->ring0
    adcx    r14, r15               // CF 链: += hi5
    adox    r14, r8               // OF 链: += lo6
    adcx    rax, rbx                 // 顶列 = hi6 + 进位
    adox    rax, rbx
    mov     rdx, [rcx+24]            // 行乘数 3
    mulx    r8, r15, [rsi+0]       // hi0->r8 lo0->r15
    adcx    r9, r15                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    r15, r9, [rsi+8]     // hi1->r15 lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rsi+16]     // hi2->r8 lo2->ring0
    adcx    r11, r15               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r15, r9, [rsi+24]     // hi3->r15 lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rsi+32]     // hi4->r8 lo4->ring0
    adcx    r13, r15               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    mulx    r15, r9, [rsi+40]     // hi5->r15 lo5->ring0
    adcx    r14, r8               // CF 链: += hi4
    adox    r14, r9               // OF 链: += lo5
    mulx    r8, r9, [rsi+48]     // hi6->r8 lo6->ring0
    adcx    rax, r15               // CF 链: += hi5
    adox    rax, r9               // OF 链: += lo6
    adcx    r8, rbx                 // 顶列 = hi6 + 进位
    adox    r8, rbx
    mov     rdx, [rcx+32]            // 行乘数 4
    mulx    r9, r15, [rsi+0]       // hi0->r9 lo0->r15
    adcx    r10, r15                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r15, r10, [rsi+8]     // hi1->r15 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rsi+16]     // hi2->r9 lo2->ring0
    adcx    r12, r15               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r15, r10, [rsi+24]     // hi3->r15 lo3->ring0
    adcx    r13, r9               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r9, r10, [rsi+32]     // hi4->r9 lo4->ring0
    adcx    r14, r15               // CF 链: += hi3
    adox    r14, r10               // OF 链: += lo4
    mulx    r15, r10, [rsi+40]     // hi5->r15 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    mulx    r9, r10, [rsi+48]     // hi6->r9 lo6->ring0
    adcx    r8, r15               // CF 链: += hi5
    adox    r8, r10               // OF 链: += lo6
    adcx    r9, rbx                 // 顶列 = hi6 + 进位
    adox    r9, rbx
    mov     rdx, [rcx+40]            // 行乘数 5
    mulx    r10, r15, [rsi+0]       // hi0->r10 lo0->r15
    adcx    r11, r15                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r15, r11, [rsi+8]     // hi1->r15 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rsi+16]     // hi2->r10 lo2->ring0
    adcx    r13, r15               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r15, r11, [rsi+24]     // hi3->r15 lo3->ring0
    adcx    r14, r10               // CF 链: += hi2
    adox    r14, r11               // OF 链: += lo3
    mulx    r10, r11, [rsi+32]     // hi4->r10 lo4->ring0
    adcx    rax, r15               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r15, r11, [rsi+40]     // hi5->r15 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    mulx    r10, r11, [rsi+48]     // hi6->r10 lo6->ring0
    adcx    r9, r15               // CF 链: += hi5
    adox    r9, r11               // OF 链: += lo6
    adcx    r10, rbx                 // 顶列 = hi6 + 进位
    adox    r10, rbx
    mov     rdx, [rcx+48]            // 行乘数 6
    mulx    r11, r15, [rsi+0]       // hi0->r11 lo0->r15
    adcx    r12, r15                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r15, r12, [rsi+8]     // hi1->r15 lo1->ring0
    adcx    r13, r11               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r11, r12, [rsi+16]     // hi2->r11 lo2->ring0
    adcx    r14, r15               // CF 链: += hi1
    adox    r14, r12               // OF 链: += lo2
    mulx    r15, r12, [rsi+24]     // hi3->r15 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rsi+32]     // hi4->r11 lo4->ring0
    adcx    r8, r15               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r15, r12, [rsi+40]     // hi5->r15 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    mulx    r11, r12, [rsi+48]     // hi6->r11 lo6->ring0
    adcx    r10, r15               // CF 链: += hi5
    adox    r10, r12               // OF 链: += lo6
    adcx    r11, rbx                 // 顶列 = hi6 + 进位
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
    mov     rdx, [rcx+0]            // 行乘数 0
    mulx    rax, rbx, [rsi+0]       // hi0->rax lo0->rbx
    mov     [rdi+0], rbx           // dst[0] = lo0
    mulx    r8, rbx, [rsi+8]     // hi1->r8 lo1->rbx
    adcx    rax, rbx                // col1 = hi0 + lo1
    mulx    r9, rbx, [rsi+16]     // hi2->r9 lo2->rbx
    adcx    r8, rbx                // col2 = hi1 + lo2
    mulx    r10, rbx, [rsi+24]     // hi3->r10 lo3->rbx
    adcx    r9, rbx                // col3 = hi2 + lo3
    mulx    r11, rbx, [rsi+32]     // hi4->r11 lo4->rbx
    adcx    r10, rbx                // col4 = hi3 + lo4
    mulx    r12, rbx, [rsi+40]     // hi5->r12 lo5->rbx
    adcx    r11, rbx                // col5 = hi4 + lo5
    mulx    r13, rbx, [rsi+48]     // hi6->r13 lo6->rbx
    adcx    r12, rbx                // col6 = hi5 + lo6
    mulx    r14, rbx, [rsi+56]     // hi7->r14 lo7->rbx
    adcx    r13, rbx                // col7 = hi6 + lo7
    adcx    r14, rbp                  // col8 = hi7 + CF
    mov     rdx, [rcx+8]            // 行乘数 1
    mulx    r15, rbx, [rsi+0]       // hi0->r15 lo0->rbx
    adcx    rax, rbx                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    rbx, rax, [rsi+8]     // hi1->rbx lo1->ring0
    adcx    r8, r15               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r15, rax, [rsi+16]     // hi2->r15 lo2->ring0
    adcx    r9, rbx               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    rbx, rax, [rsi+24]     // hi3->rbx lo3->ring0
    adcx    r10, r15               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r15, rax, [rsi+32]     // hi4->r15 lo4->ring0
    adcx    r11, rbx               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    rbx, rax, [rsi+40]     // hi5->rbx lo5->ring0
    adcx    r12, r15               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    mulx    r15, rax, [rsi+48]     // hi6->r15 lo6->ring0
    adcx    r13, rbx               // CF 链: += hi5
    adox    r13, rax               // OF 链: += lo6
    mulx    rbx, rax, [rsi+56]     // hi7->rbx lo7->ring0
    adcx    r14, r15               // CF 链: += hi6
    adox    r14, rax               // OF 链: += lo7
    adcx    rbx, rbp                 // 顶列 = hi7 + 进位
    adox    rbx, rbp
    mov     rdx, [rcx+16]            // 行乘数 2
    mulx    rax, r15, [rsi+0]       // hi0->rax lo0->r15
    adcx    r8, r15                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r15, r8, [rsi+8]     // hi1->r15 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rsi+16]     // hi2->rax lo2->ring0
    adcx    r10, r15               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r15, r8, [rsi+24]     // hi3->r15 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rsi+32]     // hi4->rax lo4->ring0
    adcx    r12, r15               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r15, r8, [rsi+40]     // hi5->r15 lo5->ring0
    adcx    r13, rax               // CF 链: += hi4
    adox    r13, r8               // OF 链: += lo5
    mulx    rax, r8, [rsi+48]     // hi6->rax lo6->ring0
    adcx    r14, r15               // CF 链: += hi5
    adox    r14, r8               // OF 链: += lo6
    mulx    r15, r8, [rsi+56]     // hi7->r15 lo7->ring0
    adcx    rbx, rax               // CF 链: += hi6
    adox    rbx, r8               // OF 链: += lo7
    adcx    r15, rbp                 // 顶列 = hi7 + 进位
    adox    r15, rbp
    mov     rdx, [rcx+24]            // 行乘数 3
    mulx    r8, rax, [rsi+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    rax, r9, [rsi+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rsi+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rsi+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rsi+32]     // hi4->r8 lo4->ring0
    adcx    r13, rax               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    mulx    rax, r9, [rsi+40]     // hi5->rax lo5->ring0
    adcx    r14, r8               // CF 链: += hi4
    adox    r14, r9               // OF 链: += lo5
    mulx    r8, r9, [rsi+48]     // hi6->r8 lo6->ring0
    adcx    rbx, rax               // CF 链: += hi5
    adox    rbx, r9               // OF 链: += lo6
    mulx    rax, r9, [rsi+56]     // hi7->rax lo7->ring0
    adcx    r15, r8               // CF 链: += hi6
    adox    r15, r9               // OF 链: += lo7
    adcx    rax, rbp                 // 顶列 = hi7 + 进位
    adox    rax, rbp
    mov     rdx, [rcx+32]            // 行乘数 4
    mulx    r9, r8, [rsi+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r8, r10, [rsi+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rsi+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rsi+24]     // hi3->r8 lo3->ring0
    adcx    r13, r9               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r9, r10, [rsi+32]     // hi4->r9 lo4->ring0
    adcx    r14, r8               // CF 链: += hi3
    adox    r14, r10               // OF 链: += lo4
    mulx    r8, r10, [rsi+40]     // hi5->r8 lo5->ring0
    adcx    rbx, r9               // CF 链: += hi4
    adox    rbx, r10               // OF 链: += lo5
    mulx    r9, r10, [rsi+48]     // hi6->r9 lo6->ring0
    adcx    r15, r8               // CF 链: += hi5
    adox    r15, r10               // OF 链: += lo6
    mulx    r8, r10, [rsi+56]     // hi7->r8 lo7->ring0
    adcx    rax, r9               // CF 链: += hi6
    adox    rax, r10               // OF 链: += lo7
    adcx    r8, rbp                 // 顶列 = hi7 + 进位
    adox    r8, rbp
    mov     rdx, [rcx+40]            // 行乘数 5
    mulx    r10, r9, [rsi+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r9, r11, [rsi+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rsi+16]     // hi2->r10 lo2->ring0
    adcx    r13, r9               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r9, r11, [rsi+24]     // hi3->r9 lo3->ring0
    adcx    r14, r10               // CF 链: += hi2
    adox    r14, r11               // OF 链: += lo3
    mulx    r10, r11, [rsi+32]     // hi4->r10 lo4->ring0
    adcx    rbx, r9               // CF 链: += hi3
    adox    rbx, r11               // OF 链: += lo4
    mulx    r9, r11, [rsi+40]     // hi5->r9 lo5->ring0
    adcx    r15, r10               // CF 链: += hi4
    adox    r15, r11               // OF 链: += lo5
    mulx    r10, r11, [rsi+48]     // hi6->r10 lo6->ring0
    adcx    rax, r9               // CF 链: += hi5
    adox    rax, r11               // OF 链: += lo6
    mulx    r9, r11, [rsi+56]     // hi7->r9 lo7->ring0
    adcx    r8, r10               // CF 链: += hi6
    adox    r8, r11               // OF 链: += lo7
    adcx    r9, rbp                 // 顶列 = hi7 + 进位
    adox    r9, rbp
    mov     rdx, [rcx+48]            // 行乘数 6
    mulx    r11, r10, [rsi+0]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r10, r12, [rsi+8]     // hi1->r10 lo1->ring0
    adcx    r13, r11               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r11, r12, [rsi+16]     // hi2->r11 lo2->ring0
    adcx    r14, r10               // CF 链: += hi1
    adox    r14, r12               // OF 链: += lo2
    mulx    r10, r12, [rsi+24]     // hi3->r10 lo3->ring0
    adcx    rbx, r11               // CF 链: += hi2
    adox    rbx, r12               // OF 链: += lo3
    mulx    r11, r12, [rsi+32]     // hi4->r11 lo4->ring0
    adcx    r15, r10               // CF 链: += hi3
    adox    r15, r12               // OF 链: += lo4
    mulx    r10, r12, [rsi+40]     // hi5->r10 lo5->ring0
    adcx    rax, r11               // CF 链: += hi4
    adox    rax, r12               // OF 链: += lo5
    mulx    r11, r12, [rsi+48]     // hi6->r11 lo6->ring0
    adcx    r8, r10               // CF 链: += hi5
    adox    r8, r12               // OF 链: += lo6
    mulx    r10, r12, [rsi+56]     // hi7->r10 lo7->ring0
    adcx    r9, r11               // CF 链: += hi6
    adox    r9, r12               // OF 链: += lo7
    adcx    r10, rbp                 // 顶列 = hi7 + 进位
    adox    r10, rbp
    mov     rdx, [rcx+56]            // 行乘数 7
    mulx    r12, r11, [rsi+0]       // hi0->r12 lo0->r11
    adcx    r13, r11                // col7 完成, CF=cf1
    mov     [rdi+56], r13           // dst[7]
    mulx    r11, r13, [rsi+8]     // hi1->r11 lo1->ring0
    adcx    r14, r12               // CF 链: += hi0
    adox    r14, r13               // OF 链: += lo1
    mulx    r12, r13, [rsi+16]     // hi2->r12 lo2->ring0
    adcx    rbx, r11               // CF 链: += hi1
    adox    rbx, r13               // OF 链: += lo2
    mulx    r11, r13, [rsi+24]     // hi3->r11 lo3->ring0
    adcx    r15, r12               // CF 链: += hi2
    adox    r15, r13               // OF 链: += lo3
    mulx    r12, r13, [rsi+32]     // hi4->r12 lo4->ring0
    adcx    rax, r11               // CF 链: += hi3
    adox    rax, r13               // OF 链: += lo4
    mulx    r11, r13, [rsi+40]     // hi5->r11 lo5->ring0
    adcx    r8, r12               // CF 链: += hi4
    adox    r8, r13               // OF 链: += lo5
    mulx    r12, r13, [rsi+48]     // hi6->r12 lo6->ring0
    adcx    r9, r11               // CF 链: += hi5
    adox    r9, r13               // OF 链: += lo6
    mulx    r11, r13, [rsi+56]     // hi7->r11 lo7->ring0
    adcx    r10, r12               // CF 链: += hi6
    adox    r10, r13               // OF 链: += lo7
    adcx    r11, rbp                 // 顶列 = hi7 + 进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    adcx    r11, rbx                  // col5 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r14, [rcx+0]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r14, r12, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r14, [rcx+0]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col7 完成, CF=cf1
    mov     [rdi+56], r13           // dst[7]
    mulx    r14, r13, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col8 完成, CF=cf1
    mov     [rdi+64], rax           // dst[8]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     [rdi+72], r8
    mov     [rdi+80], r9
    mov     [rdi+88], r10
    mov     [rdi+96], r11
    mov     [rdi+104], r12
    # === pass 1: numb[5..9) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r8, r14, [rcx+40]       // hi0->r8 lo0->r14
    mov     rdx, [rdi+40]
    add     rdx, r14               // dst + lo0
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r9, r14, [rcx+48]     // hi1->r9 lo1->r14
    adcx    r8, r14                // col6 = hi0 + lo1
    mulx    r10, r14, [rcx+56]     // hi2->r10 lo2->r14
    adcx    r9, r14                // col7 = hi1 + lo2
    mulx    r11, r14, [rcx+64]     // hi3->r11 lo3->r14
    adcx    r10, r14                // col8 = hi2 + lo3
    adcx    r11, rbx                  // col9 = hi3 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    r8, r14                // col6 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+48]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+48], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, r8, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r9, r12               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r12, r8, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r11, r12               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r8, r12, [rcx+40]       // hi0->r8 lo0->r12
    adcx    r9, r12                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r12, r9, [rcx+48]     // hi1->r12 lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r11, r12               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r12, r9, [rcx+64]     // hi3->r12 lo3->ring0
    adcx    r14, r8               // CF 链: += hi2
    adox    r14, r9               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r9, r8, [rcx+40]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r8, r10, [rcx+48]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    r14, r8               // CF 链: += hi1
    adox    r14, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+64]     // hi3->r8 lo3->ring0
    adcx    r12, r9               // CF 链: += hi2
    adox    r12, r10               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r10, r9, [rcx+40]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r9, r11, [rcx+48]     // hi1->r9 lo1->ring0
    adcx    r14, r10               // CF 链: += hi0
    adox    r14, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+56]     // hi2->r10 lo2->ring0
    adcx    r12, r9               // CF 链: += hi1
    adox    r12, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+64]     // hi3->r9 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r11               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r11, r10, [rcx+40]       // hi0->r11 lo0->r10
    adcx    r14, r10                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r10, r14, [rcx+48]     // hi1->r10 lo1->ring0
    adcx    r12, r11               // CF 链: += hi0
    adox    r12, r14               // OF 链: += lo1
    mulx    r11, r14, [rcx+56]     // hi2->r11 lo2->ring0
    adcx    r8, r10               // CF 链: += hi1
    adox    r8, r14               // OF 链: += lo2
    mulx    r10, r14, [rcx+64]     // hi3->r10 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r14               // OF 链: += lo3
    adcx    r10, rbx                 // 顶列 = hi3 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r14, r11, [rcx+40]       // hi0->r14 lo0->r11
    adcx    r12, r11                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r11, r12, [rcx+48]     // hi1->r11 lo1->ring0
    adcx    r8, r14               // CF 链: += hi0
    adox    r8, r12               // OF 链: += lo1
    mulx    r14, r12, [rcx+56]     // hi2->r14 lo2->ring0
    adcx    r9, r11               // CF 链: += hi1
    adox    r9, r12               // OF 链: += lo2
    mulx    r11, r12, [rcx+64]     // hi3->r11 lo3->ring0
    adcx    r10, r14               // CF 链: += hi2
    adox    r10, r12               // OF 链: += lo3
    adcx    r11, rbx                 // 顶列 = hi3 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    r8, r14                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r8, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r9, r12               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r12, r8, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r11, r12               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r8, r12, [rcx+40]       // hi0->r8 lo0->r12
    adcx    r9, r12                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r12, r9, [rcx+48]     // hi1->r12 lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r11, r12               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r12, r9, [rcx+64]     // hi3->r12 lo3->ring0
    adcx    r14, r8               // CF 链: += hi2
    adox    r14, r9               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, r10
    add     rdx, rbp                // += pc
    mov     [rdi+112], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+120], rdx
    mov     rdx, r14
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+128], rdx
    mov     rdx, r12
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    adcx    r11, rbx                  // col5 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r14, [rcx+0]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r14, r12, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r14, [rcx+0]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col7 完成, CF=cf1
    mov     [rdi+56], r13           // dst[7]
    mulx    r14, r13, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col8 完成, CF=cf1
    mov     [rdi+64], rax           // dst[8]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col9 完成, CF=cf1
    mov     [rdi+72], r8           // dst[9]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     [rdi+80], r9
    mov     [rdi+88], r10
    mov     [rdi+96], r11
    mov     [rdi+104], r12
    mov     [rdi+112], r13
    # === pass 1: numb[5..10) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    mov     rdx, [rdi+40]
    add     rdx, r14               // dst + lo0
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r10, r14, [rcx+48]     // hi1->r10 lo1->r14
    adcx    r9, r14                // col6 = hi0 + lo1
    mulx    r11, r14, [rcx+56]     // hi2->r11 lo2->r14
    adcx    r10, r14                // col7 = hi1 + lo2
    mulx    r12, r14, [rcx+64]     // hi3->r12 lo3->r14
    adcx    r11, r14                // col8 = hi2 + lo3
    mulx    r13, r14, [rcx+72]     // hi4->r13 lo4->r14
    adcx    r12, r14                // col9 = hi3 + lo4
    adcx    r13, rbx                  // col10 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    rax, r14, [rcx+40]       // hi0->rax lo0->r14
    adcx    r9, r14                // col6 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+48]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+48], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, r9, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+72]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r8, r14, [rcx+40]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r14, r10, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+72]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r14, r11, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+72]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r10, r14, [rcx+40]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r14, r12, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+56]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+72]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r11, r14, [rcx+40]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r14, r13, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+56]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+72]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r14, rax, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+72]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r13, r14, [rcx+40]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r8, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+56]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+72]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    rax, r14, [rcx+40]       // hi0->rax lo0->r14
    adcx    r9, r14                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r14, r9, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+72]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r8, r14, [rcx+40]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r14, r10, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+72]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, r11
    add     rdx, rbp                // += pc
    mov     [rdi+120], rdx
    mov     rdx, r12
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+128], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+136], rdx
    mov     rdx, rax
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+144], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    mulx    r12, r14, [rcx+40]     // hi5->r12 lo5->r14
    adcx    r11, r14                // col5 = hi4 + lo5
    adcx    r12, rbx                  // col6 = hi5 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r8, rax, [rcx+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    rax, r9, [rcx+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r9, r8, [rcx+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r8, r10, [rcx+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+24]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+40]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r10, r9, [rcx+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r9, r11, [rcx+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+24]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+40]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r11, r10, [rcx+0]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r10, r12, [rcx+8]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+24]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+40]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r12, r11, [rcx+0]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col7 完成, CF=cf1
    mov     [rdi+56], r14           // dst[7]
    mulx    r11, r14, [rcx+8]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+24]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+40]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r14, r12, [rcx+0]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col8 完成, CF=cf1
    mov     [rdi+64], r13           // dst[8]
    mulx    r12, r13, [rcx+8]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+16]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+24]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+32]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+40]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col9 完成, CF=cf1
    mov     [rdi+72], rax           // dst[9]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col10 完成, CF=cf1
    mov     [rdi+80], r8           // dst[10]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     [rdi+88], r9
    mov     [rdi+96], r10
    mov     [rdi+104], r11
    mov     [rdi+112], r12
    mov     [rdi+120], r14
    mov     [rdi+128], r13
    # === pass 1: numb[6..11) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r9, rax, [rcx+48]       // hi0->r9 lo0->rax
    mov     rdx, [rdi+48]
    add     rdx, rax               // dst + lo0
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r10, rax, [rcx+56]     // hi1->r10 lo1->rax
    adcx    r9, rax                // col7 = hi0 + lo1
    mulx    r11, rax, [rcx+64]     // hi2->r11 lo2->rax
    adcx    r10, rax                // col8 = hi1 + lo2
    mulx    r12, rax, [rcx+72]     // hi3->r12 lo3->rax
    adcx    r11, rax                // col9 = hi2 + lo3
    mulx    r14, rax, [rcx+80]     // hi4->r14 lo4->rax
    adcx    r12, rax                // col10 = hi3 + lo4
    adcx    r14, rbx                  // col11 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r13, rax, [rcx+48]       // hi0->r13 lo0->rax
    adcx    r9, rax                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    rax, r9, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r10, r13               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r13, r9, [rcx+64]     // hi2->r13 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r12, r13               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r13, r9, [rcx+80]     // hi4->r13 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r8, rax, [rcx+48]       // hi0->r8 lo0->rax
    adcx    r10, rax                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    rax, r10, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+64]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    rax, r10, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r14, r8               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+80]     // hi4->r8 lo4->ring0
    adcx    r13, rax               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r9, rax, [rcx+48]       // hi0->r9 lo0->rax
    adcx    r11, rax                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    rax, r11, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r14, rax               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    rax, r11, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r13, r9               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r8, rax               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r10, rax, [rcx+48]       // hi0->r10 lo0->rax
    adcx    r12, rax                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    rax, r12, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r14, r10               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+64]     // hi2->r10 lo2->ring0
    adcx    r13, rax               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    rax, r12, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+80]     // hi4->r10 lo4->ring0
    adcx    r9, rax               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r11, rax, [rcx+48]       // hi0->r11 lo0->rax
    adcx    r14, rax                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    rax, r14, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r13, r11               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r11, r14, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r8, rax               // CF 链: += hi1
    adox    r8, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r14               // OF 链: += lo3
    mulx    r11, r14, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r10, rax               // CF 链: += hi3
    adox    r10, r14               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r12, rax, [rcx+48]       // hi0->r12 lo0->rax
    adcx    r13, rax                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    rax, r13, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+64]     // hi2->r12 lo2->ring0
    adcx    r9, rax               // CF 链: += hi1
    adox    r9, r13               // OF 链: += lo2
    mulx    rax, r13, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, r13               // OF 链: += lo3
    mulx    r12, r13, [rcx+80]     // hi4->r12 lo4->ring0
    adcx    r11, rax               // CF 链: += hi3
    adox    r11, r13               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r14, rax, [rcx+48]       // hi0->r14 lo0->rax
    adcx    r8, rax                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    rax, r8, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r9, r14               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r14, r8, [rcx+64]     // hi2->r14 lo2->ring0
    adcx    r10, rax               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    rax, r8, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r11, r14               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r14, r8, [rcx+80]     // hi4->r14 lo4->ring0
    adcx    r12, rax               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r14, rbx                 // 顶列 = hi4 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r13, rax, [rcx+48]       // hi0->r13 lo0->rax
    adcx    r9, rax                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    rax, r9, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r10, r13               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r13, r9, [rcx+64]     // hi2->r13 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r12, r13               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r13, r9, [rcx+80]     // hi4->r13 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r8, rax, [rcx+48]       // hi0->r8 lo0->rax
    adcx    r10, rax                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    rax, r10, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+64]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    rax, r10, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r14, r8               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+80]     // hi4->r8 lo4->ring0
    adcx    r13, rax               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    r9, rax, [rcx+48]       // hi0->r9 lo0->rax
    adcx    r11, rax                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    rax, r11, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r14, rax               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    rax, r11, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r13, r9               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r8, rax               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, r12
    add     rdx, rbp                // += pc
    mov     [rdi+136], rdx
    mov     rdx, r14
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+144], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+152], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+160], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    mulx    r12, r14, [rcx+40]     // hi5->r12 lo5->r14
    adcx    r11, r14                // col5 = hi4 + lo5
    adcx    r12, rbx                  // col6 = hi5 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r8, rax, [rcx+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    rax, r9, [rcx+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r9, r8, [rcx+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r8, r10, [rcx+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+24]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+40]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r10, r9, [rcx+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r9, r11, [rcx+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+24]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+40]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r11, r10, [rcx+0]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r10, r12, [rcx+8]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+24]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+40]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r12, r11, [rcx+0]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col7 完成, CF=cf1
    mov     [rdi+56], r14           // dst[7]
    mulx    r11, r14, [rcx+8]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+24]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+40]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r14, r12, [rcx+0]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col8 完成, CF=cf1
    mov     [rdi+64], r13           // dst[8]
    mulx    r12, r13, [rcx+8]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+16]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+24]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+32]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+40]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col9 完成, CF=cf1
    mov     [rdi+72], rax           // dst[9]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col10 完成, CF=cf1
    mov     [rdi+80], r8           // dst[10]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, rax, [rcx+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col11 完成, CF=cf1
    mov     [rdi+88], r9           // dst[11]
    mulx    rax, r9, [rcx+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     [rdi+96], r10
    mov     [rdi+104], r11
    mov     [rdi+112], r12
    mov     [rdi+120], r14
    mov     [rdi+128], r13
    mov     [rdi+136], rax
    # === pass 1: numb[6..12) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r10, r8, [rcx+48]       // hi0->r10 lo0->r8
    mov     rdx, [rdi+48]
    add     rdx, r8               // dst + lo0
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r11, r8, [rcx+56]     // hi1->r11 lo1->r8
    adcx    r10, r8                // col7 = hi0 + lo1
    mulx    r12, r8, [rcx+64]     // hi2->r12 lo2->r8
    adcx    r11, r8                // col8 = hi1 + lo2
    mulx    r14, r8, [rcx+72]     // hi3->r14 lo3->r8
    adcx    r12, r8                // col9 = hi2 + lo3
    mulx    r13, r8, [rcx+80]     // hi4->r13 lo4->r8
    adcx    r14, r8                // col10 = hi3 + lo4
    mulx    rax, r8, [rcx+88]     // hi5->rax lo5->r8
    adcx    r13, r8                // col11 = hi4 + lo5
    adcx    rax, rbx                  // col12 = hi5 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r9, r8, [rcx+48]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r8, r10, [rcx+56]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+72]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+88]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r10, r9, [rcx+48]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r9, r11, [rcx+56]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+64]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+72]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+80]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+88]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r11, r10, [rcx+48]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r10, r12, [rcx+56]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+72]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+88]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r12, r11, [rcx+48]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r11, r14, [rcx+56]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+64]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+72]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+80]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+88]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r14, r12, [rcx+48]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r12, r13, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+64]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+80]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+88]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r13, r14, [rcx+48]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r14, rax, [rcx+56]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+64]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+72]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+80]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+88]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    rax, r13, [rcx+48]       // hi0->rax lo0->r13
    adcx    r8, r13                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r13, r8, [rcx+56]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+64]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+72]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+80]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+88]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r8, rax, [rcx+48]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    rax, r9, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+64]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+80]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+88]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r9, r8, [rcx+48]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r8, r10, [rcx+56]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+72]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+88]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    r10, r9, [rcx+48]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r9, r11, [rcx+56]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+64]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+72]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+80]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+88]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r11, r10, [rcx+48]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r10, r12, [rcx+56]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+72]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+88]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, r14
    add     rdx, rbp                // += pc
    mov     [rdi+144], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+152], rdx
    mov     rdx, rax
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+160], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+168], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+176], rdx
    mov     rdx, r10
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    adcx    r11, rbx                  // col5 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r14, [rcx+0]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r14, r12, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r14, [rcx+0]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col7 完成, CF=cf1
    mov     [rdi+56], r13           // dst[7]
    mulx    r14, r13, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col8 完成, CF=cf1
    mov     [rdi+64], rax           // dst[8]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col9 完成, CF=cf1
    mov     [rdi+72], r8           // dst[9]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col10 完成, CF=cf1
    mov     [rdi+80], r9           // dst[10]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col11 完成, CF=cf1
    mov     [rdi+88], r10           // dst[11]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col12 完成, CF=cf1
    mov     [rdi+96], r11           // dst[12]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     [rdi+104], r12
    mov     [rdi+112], r13
    mov     [rdi+120], rax
    mov     [rdi+128], r8
    mov     [rdi+136], r9
    # === pass 1: numb[5..9) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    mov     rdx, [rdi+40]
    add     rdx, r14               // dst + lo0
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r13, r14, [rcx+48]     // hi1->r13 lo1->r14
    adcx    r12, r14                // col6 = hi0 + lo1
    mulx    rax, r14, [rcx+56]     // hi2->rax lo2->r14
    adcx    r13, r14                // col7 = hi1 + lo2
    mulx    r8, r14, [rcx+64]     // hi3->r8 lo3->r14
    adcx    rax, r14                // col8 = hi2 + lo3
    adcx    r8, rbx                  // col9 = hi3 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col6 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+48]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+48], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, r12, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r12, r9, [rcx+40]       // hi0->r12 lo0->r9
    adcx    r13, r9                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r9, r13, [rcx+48]     // hi1->r9 lo1->ring0
    adcx    rax, r12               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r8, r9               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r9, r13, [rcx+64]     // hi3->r9 lo3->ring0
    adcx    r14, r12               // CF 链: += hi2
    adox    r14, r13               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r13, r12, [rcx+40]       // hi0->r13 lo0->r12
    adcx    rax, r12                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r12, rax, [rcx+48]     // hi1->r12 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+56]     // hi2->r13 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+64]     // hi3->r12 lo3->ring0
    adcx    r9, r13               // CF 链: += hi2
    adox    r9, rax               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    rax, r13, [rcx+40]       // hi0->rax lo0->r13
    adcx    r8, r13                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r13, r8, [rcx+48]     // hi1->r13 lo1->ring0
    adcx    r14, rax               // CF 链: += hi0
    adox    r14, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+64]     // hi3->r13 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r13, rbx                 // 顶列 = hi3 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r8, rax, [rcx+40]       // hi0->r8 lo0->rax
    adcx    r14, rax                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    rax, r14, [rcx+48]     // hi1->rax lo1->ring0
    adcx    r9, r8               // CF 链: += hi0
    adox    r9, r14               // OF 链: += lo1
    mulx    r8, r14, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+64]     // hi3->rax lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r14               // OF 链: += lo3
    adcx    rax, rbx                 // 顶列 = hi3 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r14, r8, [rcx+40]       // hi0->r14 lo0->r8
    adcx    r9, r8                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r8, r9, [rcx+48]     // hi1->r8 lo1->ring0
    adcx    r12, r14               // CF 链: += hi0
    adox    r12, r9               // OF 链: += lo1
    mulx    r14, r9, [rcx+56]     // hi2->r14 lo2->ring0
    adcx    r13, r8               // CF 链: += hi1
    adox    r13, r9               // OF 链: += lo2
    mulx    r8, r9, [rcx+64]     // hi3->r8 lo3->ring0
    adcx    rax, r14               // CF 链: += hi2
    adox    rax, r9               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r12, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r9, [rcx+40]       // hi0->r12 lo0->r9
    adcx    r13, r9                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r9, r13, [rcx+48]     // hi1->r9 lo1->ring0
    adcx    rax, r12               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r8, r9               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r9, r13, [rcx+64]     // hi3->r9 lo3->ring0
    adcx    r14, r12               // CF 链: += hi2
    adox    r14, r13               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r12, [rcx+40]       // hi0->r13 lo0->r12
    adcx    rax, r12                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r12, rax, [rcx+48]     // hi1->r12 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+56]     // hi2->r13 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+64]     // hi3->r12 lo3->ring0
    adcx    r9, r13               // CF 链: += hi2
    adox    r9, rax               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+40]       // hi0->rax lo0->r13
    adcx    r8, r13                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r13, r8, [rcx+48]     // hi1->r13 lo1->ring0
    adcx    r14, rax               // CF 链: += hi0
    adox    r14, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+64]     // hi3->r13 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r13, rbx                 // 顶列 = hi3 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, rax, [rcx+40]       // hi0->r8 lo0->rax
    adcx    r14, rax                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    rax, r14, [rcx+48]     // hi1->rax lo1->ring0
    adcx    r9, r8               // CF 链: += hi0
    adox    r9, r14               // OF 链: += lo1
    mulx    r8, r14, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+64]     // hi3->rax lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r14               // OF 链: += lo3
    adcx    rax, rbx                 // 顶列 = hi3 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r14, r8, [rcx+40]       // hi0->r14 lo0->r8
    adcx    r9, r8                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r8, r9, [rcx+48]     // hi1->r8 lo1->ring0
    adcx    r12, r14               // CF 链: += hi0
    adox    r12, r9               // OF 链: += lo1
    mulx    r14, r9, [rcx+56]     // hi2->r14 lo2->ring0
    adcx    r13, r8               // CF 链: += hi1
    adox    r13, r9               // OF 链: += lo2
    mulx    r8, r9, [rcx+64]     // hi3->r8 lo3->ring0
    adcx    rax, r14               // CF 链: += hi2
    adox    rax, r9               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, r12
    add     rdx, rbp                // += pc
    mov     [rdi+144], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+152], rdx
    mov     rdx, rax
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+160], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+168], rdx
    # === pass 2: numb[9..13) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r12, r14, [rcx+72]       // hi0->r12 lo0->r14
    mov     rdx, [rdi+72]
    add     rdx, r14               // dst + lo0
    mov     [rdi+72], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r13, r14, [rcx+80]     // hi1->r13 lo1->r14
    adcx    r12, r14                // col10 = hi0 + lo1
    mulx    rax, r14, [rcx+88]     // hi2->rax lo2->r14
    adcx    r13, r14                // col11 = hi1 + lo2
    mulx    r8, r14, [rcx+96]     // hi3->r8 lo3->r14
    adcx    rax, r14                // col12 = hi2 + lo3
    adcx    r8, rbx                  // col13 = hi3 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r9, r14, [rcx+72]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, r12, [rcx+80]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+88]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+96]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r12, r9, [rcx+72]       // hi0->r12 lo0->r9
    adcx    r13, r9                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r9, r13, [rcx+80]     // hi1->r9 lo1->ring0
    adcx    rax, r12               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+88]     // hi2->r12 lo2->ring0
    adcx    r8, r9               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r9, r13, [rcx+96]     // hi3->r9 lo3->ring0
    adcx    r14, r12               // CF 链: += hi2
    adox    r14, r13               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r13, r12, [rcx+72]       // hi0->r13 lo0->r12
    adcx    rax, r12                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r12, rax, [rcx+80]     // hi1->r12 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+88]     // hi2->r13 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+96]     // hi3->r12 lo3->ring0
    adcx    r9, r13               // CF 链: += hi2
    adox    r9, rax               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    rax, r13, [rcx+72]       // hi0->rax lo0->r13
    adcx    r8, r13                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r13, r8, [rcx+80]     // hi1->r13 lo1->ring0
    adcx    r14, rax               // CF 链: += hi0
    adox    r14, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+88]     // hi2->rax lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+96]     // hi3->r13 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r13, rbx                 // 顶列 = hi3 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r8, rax, [rcx+72]       // hi0->r8 lo0->rax
    adcx    r14, rax                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    rax, r14, [rcx+80]     // hi1->rax lo1->ring0
    adcx    r9, r8               // CF 链: += hi0
    adox    r9, r14               // OF 链: += lo1
    mulx    r8, r14, [rcx+88]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+96]     // hi3->rax lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r14               // OF 链: += lo3
    adcx    rax, rbx                 // 顶列 = hi3 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r14, r8, [rcx+72]       // hi0->r14 lo0->r8
    adcx    r9, r8                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r8, r9, [rcx+80]     // hi1->r8 lo1->ring0
    adcx    r12, r14               // CF 链: += hi0
    adox    r12, r9               // OF 链: += lo1
    mulx    r14, r9, [rcx+88]     // hi2->r14 lo2->ring0
    adcx    r13, r8               // CF 链: += hi1
    adox    r13, r9               // OF 链: += lo2
    mulx    r8, r9, [rcx+96]     // hi3->r8 lo3->ring0
    adcx    rax, r14               // CF 链: += hi2
    adox    rax, r9               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r9, r14, [rcx+72]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r12, [rcx+80]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+88]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+96]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r9, [rcx+72]       // hi0->r12 lo0->r9
    adcx    r13, r9                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r9, r13, [rcx+80]     // hi1->r9 lo1->ring0
    adcx    rax, r12               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+88]     // hi2->r12 lo2->ring0
    adcx    r8, r9               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r9, r13, [rcx+96]     // hi3->r9 lo3->ring0
    adcx    r14, r12               // CF 链: += hi2
    adox    r14, r13               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r12, [rcx+72]       // hi0->r13 lo0->r12
    adcx    rax, r12                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r12, rax, [rcx+80]     // hi1->r12 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+88]     // hi2->r13 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+96]     // hi3->r12 lo3->ring0
    adcx    r9, r13               // CF 链: += hi2
    adox    r9, rax               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+72]       // hi0->rax lo0->r13
    adcx    r8, r13                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r13, r8, [rcx+80]     // hi1->r13 lo1->ring0
    adcx    r14, rax               // CF 链: += hi0
    adox    r14, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+88]     // hi2->rax lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+96]     // hi3->r13 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r13, rbx                 // 顶列 = hi3 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, rax, [rcx+72]       // hi0->r8 lo0->rax
    adcx    r14, rax                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    rax, r14, [rcx+80]     // hi1->rax lo1->ring0
    adcx    r9, r8               // CF 链: += hi0
    adox    r9, r14               // OF 链: += lo1
    mulx    r8, r14, [rcx+88]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+96]     // hi3->rax lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r14               // OF 链: += lo3
    adcx    rax, rbx                 // 顶列 = hi3 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r14, r8, [rcx+72]       // hi0->r14 lo0->r8
    adcx    r9, r8                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r8, r9, [rcx+80]     // hi1->r8 lo1->ring0
    adcx    r12, r14               // CF 链: += hi0
    adox    r12, r9               // OF 链: += lo1
    mulx    r14, r9, [rcx+88]     // hi2->r14 lo2->ring0
    adcx    r13, r8               // CF 链: += hi1
    adox    r13, r9               // OF 链: += lo2
    mulx    r8, r9, [rcx+96]     // hi3->r8 lo3->ring0
    adcx    rax, r14               // CF 链: += hi2
    adox    rax, r9               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, r12
    add     rdx, rbp                // += pc
    mov     [rdi+176], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+184], rdx
    mov     rdx, rax
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+192], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    adcx    r11, rbx                  // col5 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r14, [rcx+0]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r14, r12, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r14, [rcx+0]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col7 完成, CF=cf1
    mov     [rdi+56], r13           // dst[7]
    mulx    r14, r13, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col8 完成, CF=cf1
    mov     [rdi+64], rax           // dst[8]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col9 完成, CF=cf1
    mov     [rdi+72], r8           // dst[9]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col10 完成, CF=cf1
    mov     [rdi+80], r9           // dst[10]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col11 完成, CF=cf1
    mov     [rdi+88], r10           // dst[11]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col12 完成, CF=cf1
    mov     [rdi+96], r11           // dst[12]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r14, [rcx+0]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col13 完成, CF=cf1
    mov     [rdi+104], r12           // dst[13]
    mulx    r14, r12, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     [rdi+112], r13
    mov     [rdi+120], rax
    mov     [rdi+128], r8
    mov     [rdi+136], r9
    mov     [rdi+144], r10
    # === pass 1: numb[5..10) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r13, r14, [rcx+40]       // hi0->r13 lo0->r14
    mov     rdx, [rdi+40]
    add     rdx, r14               // dst + lo0
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    rax, r14, [rcx+48]     // hi1->rax lo1->r14
    adcx    r13, r14                // col6 = hi0 + lo1
    mulx    r8, r14, [rcx+56]     // hi2->r8 lo2->r14
    adcx    rax, r14                // col7 = hi1 + lo2
    mulx    r9, r14, [rcx+64]     // hi3->r9 lo3->r14
    adcx    r8, r14                // col8 = hi2 + lo3
    mulx    r10, r14, [rcx+72]     // hi4->r10 lo4->r14
    adcx    r9, r14                // col9 = hi3 + lo4
    adcx    r10, rbx                  // col10 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r11, r14, [rcx+40]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col6 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+48]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+48], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, r13, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+56]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+72]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r14, rax, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+72]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r13, r14, [rcx+40]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r14, r8, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+56]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+72]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    rax, r14, [rcx+40]       // hi0->rax lo0->r14
    adcx    r9, r14                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r14, r9, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+72]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r8, r14, [rcx+40]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r14, r10, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+72]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r14, r11, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+72]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r10, r14, [rcx+40]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r12, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+56]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+72]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r11, r14, [rcx+40]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r14, r13, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+56]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+72]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r14, rax, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+72]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    r13, r14, [rcx+40]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r14, r8, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+56]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+72]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    rax, r14, [rcx+40]       // hi0->rax lo0->r14
    adcx    r9, r14                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r14, r9, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+72]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r8, r14, [rcx+40]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r14, r10, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+72]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r14, r11, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+72]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, r12
    add     rdx, rbp                // += pc
    mov     [rdi+152], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+160], rdx
    mov     rdx, rax
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+168], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+176], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+184], rdx
    # === pass 2: numb[10..14) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r12, r14, [rcx+80]       // hi0->r12 lo0->r14
    mov     rdx, [rdi+80]
    add     rdx, r14               // dst + lo0
    mov     [rdi+80], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r13, r14, [rcx+88]     // hi1->r13 lo1->r14
    adcx    r12, r14                // col11 = hi0 + lo1
    mulx    rax, r14, [rcx+96]     // hi2->rax lo2->r14
    adcx    r13, r14                // col12 = hi1 + lo2
    mulx    r8, r14, [rcx+104]     // hi3->r8 lo3->r14
    adcx    rax, r14                // col13 = hi2 + lo3
    adcx    r8, rbx                  // col14 = hi3 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r9, r14, [rcx+80]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, r12, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+96]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r12, r9, [rcx+80]       // hi0->r12 lo0->r9
    adcx    r13, r9                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r9, r13, [rcx+88]     // hi1->r9 lo1->ring0
    adcx    rax, r12               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+96]     // hi2->r12 lo2->ring0
    adcx    r8, r9               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r9, r13, [rcx+104]     // hi3->r9 lo3->ring0
    adcx    r14, r12               // CF 链: += hi2
    adox    r14, r13               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r13, r12, [rcx+80]       // hi0->r13 lo0->r12
    adcx    rax, r12                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r12, rax, [rcx+88]     // hi1->r12 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+96]     // hi2->r13 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+104]     // hi3->r12 lo3->ring0
    adcx    r9, r13               // CF 链: += hi2
    adox    r9, rax               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    rax, r13, [rcx+80]       // hi0->rax lo0->r13
    adcx    r8, r13                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r13, r8, [rcx+88]     // hi1->r13 lo1->ring0
    adcx    r14, rax               // CF 链: += hi0
    adox    r14, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+96]     // hi2->rax lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+104]     // hi3->r13 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r13, rbx                 // 顶列 = hi3 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r8, rax, [rcx+80]       // hi0->r8 lo0->rax
    adcx    r14, rax                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    rax, r14, [rcx+88]     // hi1->rax lo1->ring0
    adcx    r9, r8               // CF 链: += hi0
    adox    r9, r14               // OF 链: += lo1
    mulx    r8, r14, [rcx+96]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+104]     // hi3->rax lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r14               // OF 链: += lo3
    adcx    rax, rbx                 // 顶列 = hi3 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r14, r8, [rcx+80]       // hi0->r14 lo0->r8
    adcx    r9, r8                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r8, r9, [rcx+88]     // hi1->r8 lo1->ring0
    adcx    r12, r14               // CF 链: += hi0
    adox    r12, r9               // OF 链: += lo1
    mulx    r14, r9, [rcx+96]     // hi2->r14 lo2->ring0
    adcx    r13, r8               // CF 链: += hi1
    adox    r13, r9               // OF 链: += lo2
    mulx    r8, r9, [rcx+104]     // hi3->r8 lo3->ring0
    adcx    rax, r14               // CF 链: += hi2
    adox    rax, r9               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r9, r14, [rcx+80]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r12, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+96]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r9, [rcx+80]       // hi0->r12 lo0->r9
    adcx    r13, r9                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r9, r13, [rcx+88]     // hi1->r9 lo1->ring0
    adcx    rax, r12               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+96]     // hi2->r12 lo2->ring0
    adcx    r8, r9               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r9, r13, [rcx+104]     // hi3->r9 lo3->ring0
    adcx    r14, r12               // CF 链: += hi2
    adox    r14, r13               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r12, [rcx+80]       // hi0->r13 lo0->r12
    adcx    rax, r12                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r12, rax, [rcx+88]     // hi1->r12 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+96]     // hi2->r13 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+104]     // hi3->r12 lo3->ring0
    adcx    r9, r13               // CF 链: += hi2
    adox    r9, rax               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+80]       // hi0->rax lo0->r13
    adcx    r8, r13                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r13, r8, [rcx+88]     // hi1->r13 lo1->ring0
    adcx    r14, rax               // CF 链: += hi0
    adox    r14, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+96]     // hi2->rax lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+104]     // hi3->r13 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r13, rbx                 // 顶列 = hi3 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, rax, [rcx+80]       // hi0->r8 lo0->rax
    adcx    r14, rax                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    rax, r14, [rcx+88]     // hi1->rax lo1->ring0
    adcx    r9, r8               // CF 链: += hi0
    adox    r9, r14               // OF 链: += lo1
    mulx    r8, r14, [rcx+96]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+104]     // hi3->rax lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r14               // OF 链: += lo3
    adcx    rax, rbx                 // 顶列 = hi3 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r14, r8, [rcx+80]       // hi0->r14 lo0->r8
    adcx    r9, r8                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r8, r9, [rcx+88]     // hi1->r8 lo1->ring0
    adcx    r12, r14               // CF 链: += hi0
    adox    r12, r9               // OF 链: += lo1
    mulx    r14, r9, [rcx+96]     // hi2->r14 lo2->ring0
    adcx    r13, r8               // CF 链: += hi1
    adox    r13, r9               // OF 链: += lo2
    mulx    r8, r9, [rcx+104]     // hi3->r8 lo3->ring0
    adcx    rax, r14               // CF 链: += hi2
    adox    rax, r9               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r9, r14, [rcx+80]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col23 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+184]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+184], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r14, r12, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+96]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, r13
    add     rdx, rbp                // += pc
    mov     [rdi+192], rdx
    mov     rdx, rax
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+200], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+208], rdx
    mov     rdx, r14
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    adcx    r11, rbx                  // col5 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r14, [rcx+0]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r14, r12, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r14, [rcx+0]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col7 完成, CF=cf1
    mov     [rdi+56], r13           // dst[7]
    mulx    r14, r13, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col8 完成, CF=cf1
    mov     [rdi+64], rax           // dst[8]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col9 完成, CF=cf1
    mov     [rdi+72], r8           // dst[9]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col10 完成, CF=cf1
    mov     [rdi+80], r9           // dst[10]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col11 完成, CF=cf1
    mov     [rdi+88], r10           // dst[11]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col12 完成, CF=cf1
    mov     [rdi+96], r11           // dst[12]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r14, [rcx+0]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col13 完成, CF=cf1
    mov     [rdi+104], r12           // dst[13]
    mulx    r14, r12, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r14, [rcx+0]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col14 完成, CF=cf1
    mov     [rdi+112], r13           // dst[14]
    mulx    r14, r13, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     [rdi+120], rax
    mov     [rdi+128], r8
    mov     [rdi+136], r9
    mov     [rdi+144], r10
    mov     [rdi+152], r11
    # === pass 1: numb[5..10) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+40]       // hi0->rax lo0->r14
    mov     rdx, [rdi+40]
    add     rdx, r14               // dst + lo0
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r8, r14, [rcx+48]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col6 = hi0 + lo1
    mulx    r9, r14, [rcx+56]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col7 = hi1 + lo2
    mulx    r10, r14, [rcx+64]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col8 = hi2 + lo3
    mulx    r11, r14, [rcx+72]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col9 = hi3 + lo4
    adcx    r11, rbx                  // col10 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col6 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+48]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+48], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, rax, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+72]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r14, [rcx+40]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r14, r8, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+56]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+72]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r14, [rcx+40]       // hi0->rax lo0->r14
    adcx    r9, r14                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r14, r9, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+72]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r14, [rcx+40]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r14, r10, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+72]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r14, r11, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+72]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r14, [rcx+40]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r14, r12, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+56]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+72]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r14, [rcx+40]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r13, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+56]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+72]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r14, rax, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+72]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+40]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r14, r8, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+56]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+72]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r14, [rcx+40]       // hi0->rax lo0->r14
    adcx    r9, r14                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r14, r9, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+72]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, r14, [rcx+40]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r14, r10, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+72]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r14, r11, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+72]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r14, [rcx+40]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r14, r12, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+56]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+72]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r14, [rcx+40]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r14, r13, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+56]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+72]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, rax
    add     rdx, rbp                // += pc
    mov     [rdi+160], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+168], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+176], rdx
    mov     rdx, r10
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+184], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+192], rdx
    # === pass 2: numb[10..15) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+80]       // hi0->rax lo0->r14
    mov     rdx, [rdi+80]
    add     rdx, r14               // dst + lo0
    mov     [rdi+80], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r8, r14, [rcx+88]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col11 = hi0 + lo1
    mulx    r9, r14, [rcx+96]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col12 = hi1 + lo2
    mulx    r10, r14, [rcx+104]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col13 = hi2 + lo3
    mulx    r11, r14, [rcx+112]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col14 = hi3 + lo4
    adcx    r11, rbx                  // col15 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+80]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, rax, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+96]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+112]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r14, [rcx+80]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r14, r8, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+96]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+112]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r14, [rcx+80]       // hi0->rax lo0->r14
    adcx    r9, r14                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r14, r9, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+96]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+112]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r14, [rcx+80]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r14, r10, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+96]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+112]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r14, [rcx+80]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r14, r11, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+96]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+112]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r14, [rcx+80]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r14, r12, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+96]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+112]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r14, [rcx+80]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r13, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+96]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+112]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r14, [rcx+80]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r14, rax, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+96]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+112]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+80]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r14, r8, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+96]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+112]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r14, [rcx+80]       // hi0->rax lo0->r14
    adcx    r9, r14                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r14, r9, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+96]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+112]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, r14, [rcx+80]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r14, r10, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+96]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+112]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r14, [rcx+80]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r14, r11, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+96]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+112]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r14, [rcx+80]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col23 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+184]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+184], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r14, r12, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+96]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+112]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r14, [rcx+80]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col24 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+192]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+192], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r14, r13, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+96]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+112]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, rax
    add     rdx, rbp                // += pc
    mov     [rdi+200], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+208], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+216], rdx
    mov     rdx, r10
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+224], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    mulx    r12, r14, [rcx+40]     // hi5->r12 lo5->r14
    adcx    r11, r14                // col5 = hi4 + lo5
    adcx    r12, rbx                  // col6 = hi5 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r8, rax, [rcx+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    rax, r9, [rcx+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r9, r8, [rcx+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r8, r10, [rcx+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+24]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+40]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r10, r9, [rcx+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r9, r11, [rcx+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+24]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+40]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r11, r10, [rcx+0]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r10, r12, [rcx+8]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+24]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+40]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r12, r11, [rcx+0]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col7 完成, CF=cf1
    mov     [rdi+56], r14           // dst[7]
    mulx    r11, r14, [rcx+8]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+24]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+40]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r14, r12, [rcx+0]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col8 完成, CF=cf1
    mov     [rdi+64], r13           // dst[8]
    mulx    r12, r13, [rcx+8]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+16]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+24]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+32]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+40]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col9 完成, CF=cf1
    mov     [rdi+72], rax           // dst[9]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col10 完成, CF=cf1
    mov     [rdi+80], r8           // dst[10]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, rax, [rcx+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col11 完成, CF=cf1
    mov     [rdi+88], r9           // dst[11]
    mulx    rax, r9, [rcx+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r8, [rcx+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col12 完成, CF=cf1
    mov     [rdi+96], r10           // dst[12]
    mulx    r8, r10, [rcx+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+24]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+40]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r9, [rcx+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col13 完成, CF=cf1
    mov     [rdi+104], r11           // dst[13]
    mulx    r9, r11, [rcx+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+24]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+40]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r10, [rcx+0]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col14 完成, CF=cf1
    mov     [rdi+112], r12           // dst[14]
    mulx    r10, r12, [rcx+8]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+24]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+40]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r12, r11, [rcx+0]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col15 完成, CF=cf1
    mov     [rdi+120], r14           // dst[15]
    mulx    r11, r14, [rcx+8]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+24]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+40]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     [rdi+128], r13
    mov     [rdi+136], rax
    mov     [rdi+144], r8
    mov     [rdi+152], r9
    mov     [rdi+160], r10
    mov     [rdi+168], r11
    # === pass 1: numb[6..11) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r13, r12, [rcx+48]       // hi0->r13 lo0->r12
    mov     rdx, [rdi+48]
    add     rdx, r12               // dst + lo0
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    rax, r12, [rcx+56]     // hi1->rax lo1->r12
    adcx    r13, r12                // col7 = hi0 + lo1
    mulx    r8, r12, [rcx+64]     // hi2->r8 lo2->r12
    adcx    rax, r12                // col8 = hi1 + lo2
    mulx    r9, r12, [rcx+72]     // hi3->r9 lo3->r12
    adcx    r8, r12                // col9 = hi2 + lo3
    mulx    r10, r12, [rcx+80]     // hi4->r10 lo4->r12
    adcx    r9, r12                // col10 = hi3 + lo4
    adcx    r10, rbx                  // col11 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r11, r12, [rcx+48]       // hi0->r11 lo0->r12
    adcx    r13, r12                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r12, r13, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r14, r12, [rcx+48]       // hi0->r14 lo0->r12
    adcx    rax, r12                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r12, rax, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r8, r14               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r14, rax, [rcx+64]     // hi2->r14 lo2->ring0
    adcx    r9, r12               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r10, r14               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r14, rax, [rcx+80]     // hi4->r14 lo4->ring0
    adcx    r11, r12               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r14, rbx                 // 顶列 = hi4 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r13, r12, [rcx+48]       // hi0->r13 lo0->r12
    adcx    r8, r12                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r12, r8, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+64]     // hi2->r13 lo2->ring0
    adcx    r10, r12               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r12, r8, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+80]     // hi4->r13 lo4->ring0
    adcx    r14, r12               // CF 链: += hi3
    adox    r14, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    rax, r12, [rcx+48]       // hi0->rax lo0->r12
    adcx    r9, r12                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r12, r9, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+64]     // hi2->rax lo2->ring0
    adcx    r11, r12               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r12, r9, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r14, rax               // CF 链: += hi2
    adox    r14, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+80]     // hi4->rax lo4->ring0
    adcx    r13, r12               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r8, r12, [rcx+48]       // hi0->r8 lo0->r12
    adcx    r10, r12                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r12, r10, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+64]     // hi2->r8 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, r10               // OF 链: += lo2
    mulx    r12, r10, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+80]     // hi4->r8 lo4->ring0
    adcx    rax, r12               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r9, r12, [rcx+48]       // hi0->r9 lo0->r12
    adcx    r11, r12                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r12, r11, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r14, r9               // CF 链: += hi0
    adox    r14, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r13, r12               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r12, r11, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r8, r12               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r10, r12, [rcx+48]       // hi0->r10 lo0->r12
    adcx    r14, r12                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r12, r14, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r10, r14, [rcx+64]     // hi2->r10 lo2->ring0
    adcx    rax, r12               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r12, r14, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r10, r14, [rcx+80]     // hi4->r10 lo4->ring0
    adcx    r9, r12               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r11, r12, [rcx+48]       // hi0->r11 lo0->r12
    adcx    r13, r12                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r12, r13, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r14, r12, [rcx+48]       // hi0->r14 lo0->r12
    adcx    rax, r12                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r12, rax, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r8, r14               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r14, rax, [rcx+64]     // hi2->r14 lo2->ring0
    adcx    r9, r12               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r10, r14               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r14, rax, [rcx+80]     // hi4->r14 lo4->ring0
    adcx    r11, r12               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r14, rbx                 // 顶列 = hi4 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    r13, r12, [rcx+48]       // hi0->r13 lo0->r12
    adcx    r8, r12                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r12, r8, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+64]     // hi2->r13 lo2->ring0
    adcx    r10, r12               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r12, r8, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+80]     // hi4->r13 lo4->ring0
    adcx    r14, r12               // CF 链: += hi3
    adox    r14, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    rax, r12, [rcx+48]       // hi0->rax lo0->r12
    adcx    r9, r12                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r12, r9, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+64]     // hi2->rax lo2->ring0
    adcx    r11, r12               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r12, r9, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r14, rax               // CF 链: += hi2
    adox    r14, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+80]     // hi4->rax lo4->ring0
    adcx    r13, r12               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r8, r12, [rcx+48]       // hi0->r8 lo0->r12
    adcx    r10, r12                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r12, r10, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+64]     // hi2->r8 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, r10               // OF 链: += lo2
    mulx    r12, r10, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+80]     // hi4->r8 lo4->ring0
    adcx    rax, r12               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r9, r12, [rcx+48]       // hi0->r9 lo0->r12
    adcx    r11, r12                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r12, r11, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r14, r9               // CF 链: += hi0
    adox    r14, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r13, r12               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r12, r11, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r8, r12               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r10, r12, [rcx+48]       // hi0->r10 lo0->r12
    adcx    r14, r12                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r12, r14, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r10, r14, [rcx+64]     // hi2->r10 lo2->ring0
    adcx    rax, r12               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r12, r14, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r10, r14, [rcx+80]     // hi4->r10 lo4->ring0
    adcx    r9, r12               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r11, r12, [rcx+48]       // hi0->r11 lo0->r12
    adcx    r13, r12                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+120]            // 重载行乘数
    mulx    r12, r13, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, rax
    add     rdx, rbp                // += pc
    mov     [rdi+176], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+184], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+192], rdx
    mov     rdx, r10
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+200], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+208], rdx
    # === pass 2: numb[11..16) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r12, [rcx+88]       // hi0->rax lo0->r12
    mov     rdx, [rdi+88]
    add     rdx, r12               // dst + lo0
    mov     [rdi+88], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r8, r12, [rcx+96]     // hi1->r8 lo1->r12
    adcx    rax, r12                // col12 = hi0 + lo1
    mulx    r9, r12, [rcx+104]     // hi2->r9 lo2->r12
    adcx    r8, r12                // col13 = hi1 + lo2
    mulx    r10, r12, [rcx+112]     // hi3->r10 lo3->r12
    adcx    r9, r12                // col14 = hi2 + lo3
    mulx    r11, r12, [rcx+120]     // hi4->r11 lo4->r12
    adcx    r10, r12                // col15 = hi3 + lo4
    adcx    r11, rbx                  // col16 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r14, r12, [rcx+88]       // hi0->r14 lo0->r12
    adcx    rax, r12                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r12, rax, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r8, r14               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r14, rax, [rcx+104]     // hi2->r14 lo2->ring0
    adcx    r9, r12               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r10, r14               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r14, rax, [rcx+120]     // hi4->r14 lo4->ring0
    adcx    r11, r12               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r14, rbx                 // 顶列 = hi4 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r12, [rcx+88]       // hi0->r13 lo0->r12
    adcx    r8, r12                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r12, r8, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+104]     // hi2->r13 lo2->ring0
    adcx    r10, r12               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r12, r8, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+120]     // hi4->r13 lo4->ring0
    adcx    r14, r12               // CF 链: += hi3
    adox    r14, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r12, [rcx+88]       // hi0->rax lo0->r12
    adcx    r9, r12                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r12, r9, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+104]     // hi2->rax lo2->ring0
    adcx    r11, r12               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r12, r9, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r14, rax               // CF 链: += hi2
    adox    r14, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+120]     // hi4->rax lo4->ring0
    adcx    r13, r12               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r12, [rcx+88]       // hi0->r8 lo0->r12
    adcx    r10, r12                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r12, r10, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+104]     // hi2->r8 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, r10               // OF 链: += lo2
    mulx    r12, r10, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+120]     // hi4->r8 lo4->ring0
    adcx    rax, r12               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r12, [rcx+88]       // hi0->r9 lo0->r12
    adcx    r11, r12                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r12, r11, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r14, r9               // CF 链: += hi0
    adox    r14, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+104]     // hi2->r9 lo2->ring0
    adcx    r13, r12               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r12, r11, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+120]     // hi4->r9 lo4->ring0
    adcx    r8, r12               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r12, [rcx+88]       // hi0->r10 lo0->r12
    adcx    r14, r12                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r12, r14, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r10, r14, [rcx+104]     // hi2->r10 lo2->ring0
    adcx    rax, r12               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r12, r14, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r10, r14, [rcx+120]     // hi4->r10 lo4->ring0
    adcx    r9, r12               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r12, [rcx+88]       // hi0->r11 lo0->r12
    adcx    r13, r12                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r12, r13, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+104]     // hi2->r11 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+120]     // hi4->r11 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r14, r12, [rcx+88]       // hi0->r14 lo0->r12
    adcx    rax, r12                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r12, rax, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r8, r14               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r14, rax, [rcx+104]     // hi2->r14 lo2->ring0
    adcx    r9, r12               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r10, r14               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r14, rax, [rcx+120]     // hi4->r14 lo4->ring0
    adcx    r11, r12               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r14, rbx                 // 顶列 = hi4 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r12, [rcx+88]       // hi0->r13 lo0->r12
    adcx    r8, r12                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r12, r8, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+104]     // hi2->r13 lo2->ring0
    adcx    r10, r12               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r12, r8, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+120]     // hi4->r13 lo4->ring0
    adcx    r14, r12               // CF 链: += hi3
    adox    r14, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r12, [rcx+88]       // hi0->rax lo0->r12
    adcx    r9, r12                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r12, r9, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+104]     // hi2->rax lo2->ring0
    adcx    r11, r12               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r12, r9, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r14, rax               // CF 链: += hi2
    adox    r14, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+120]     // hi4->rax lo4->ring0
    adcx    r13, r12               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, r12, [rcx+88]       // hi0->r8 lo0->r12
    adcx    r10, r12                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r12, r10, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+104]     // hi2->r8 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, r10               // OF 链: += lo2
    mulx    r12, r10, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+120]     // hi4->r8 lo4->ring0
    adcx    rax, r12               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r12, [rcx+88]       // hi0->r9 lo0->r12
    adcx    r11, r12                // col23 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+184]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+184], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r12, r11, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r14, r9               // CF 链: += hi0
    adox    r14, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+104]     // hi2->r9 lo2->ring0
    adcx    r13, r12               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r12, r11, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+120]     // hi4->r9 lo4->ring0
    adcx    r8, r12               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r12, [rcx+88]       // hi0->r10 lo0->r12
    adcx    r14, r12                // col24 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+192]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+192], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r12, r14, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r10, r14, [rcx+104]     // hi2->r10 lo2->ring0
    adcx    rax, r12               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r12, r14, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r10, r14, [rcx+120]     // hi4->r10 lo4->ring0
    adcx    r9, r12               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r12, [rcx+88]       // hi0->r11 lo0->r12
    adcx    r13, r12                // col25 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+200]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+200], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r12, r13, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+104]     // hi2->r11 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+120]     // hi4->r11 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r14, r12, [rcx+88]       // hi0->r14 lo0->r12
    adcx    rax, r12                // col26 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+208]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+208], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+120]            // 重载行乘数
    mulx    r12, rax, [rcx+96]     // hi1->r12 lo1->ring0
    adcx    r8, r14               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r14, rax, [rcx+104]     // hi2->r14 lo2->ring0
    adcx    r9, r12               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+112]     // hi3->r12 lo3->ring0
    adcx    r10, r14               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r14, rax, [rcx+120]     // hi4->r14 lo4->ring0
    adcx    r11, r12               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r14, rbx                 // 顶列 = hi4 + 进位
    adox    r14, rbx
    mov     rdx, r8
    add     rdx, rbp                // += pc
    mov     [rdi+216], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+224], rdx
    mov     rdx, r10
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+232], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+240], rdx
    mov     rdx, r14
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    mulx    r12, r14, [rcx+40]     // hi5->r12 lo5->r14
    adcx    r11, r14                // col5 = hi4 + lo5
    adcx    r12, rbx                  // col6 = hi5 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r8, rax, [rcx+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    rax, r9, [rcx+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r9, r8, [rcx+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r8, r10, [rcx+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+24]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+40]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r10, r9, [rcx+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r9, r11, [rcx+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+24]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+40]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r11, r10, [rcx+0]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r10, r12, [rcx+8]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+24]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+40]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r12, r11, [rcx+0]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col7 完成, CF=cf1
    mov     [rdi+56], r14           // dst[7]
    mulx    r11, r14, [rcx+8]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+24]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+40]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r14, r12, [rcx+0]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col8 完成, CF=cf1
    mov     [rdi+64], r13           // dst[8]
    mulx    r12, r13, [rcx+8]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+16]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+24]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+32]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+40]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col9 完成, CF=cf1
    mov     [rdi+72], rax           // dst[9]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col10 完成, CF=cf1
    mov     [rdi+80], r8           // dst[10]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, rax, [rcx+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col11 完成, CF=cf1
    mov     [rdi+88], r9           // dst[11]
    mulx    rax, r9, [rcx+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r8, [rcx+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col12 完成, CF=cf1
    mov     [rdi+96], r10           // dst[12]
    mulx    r8, r10, [rcx+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+24]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+40]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r9, [rcx+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col13 完成, CF=cf1
    mov     [rdi+104], r11           // dst[13]
    mulx    r9, r11, [rcx+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+24]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+40]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r10, [rcx+0]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col14 完成, CF=cf1
    mov     [rdi+112], r12           // dst[14]
    mulx    r10, r12, [rcx+8]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+24]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+40]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r12, r11, [rcx+0]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col15 完成, CF=cf1
    mov     [rdi+120], r14           // dst[15]
    mulx    r11, r14, [rcx+8]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+24]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+40]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    r14, r12, [rcx+0]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col16 完成, CF=cf1
    mov     [rdi+128], r13           // dst[16]
    mulx    r12, r13, [rcx+8]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+16]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+24]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+32]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+40]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     [rdi+136], rax
    mov     [rdi+144], r8
    mov     [rdi+152], r9
    mov     [rdi+160], r10
    mov     [rdi+168], r11
    mov     [rdi+176], r12
    # === pass 1: numb[6..12) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+48]       // hi0->rax lo0->r14
    mov     rdx, [rdi+48]
    add     rdx, r14               // dst + lo0
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r8, r14, [rcx+56]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col7 = hi0 + lo1
    mulx    r9, r14, [rcx+64]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col8 = hi1 + lo2
    mulx    r10, r14, [rcx+72]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col9 = hi2 + lo3
    mulx    r11, r14, [rcx+80]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col10 = hi3 + lo4
    mulx    r12, r14, [rcx+88]     // hi5->r12 lo5->r14
    adcx    r11, r14                // col11 = hi4 + lo5
    adcx    r12, rbx                  // col12 = hi5 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r13, r14, [rcx+48]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, rax, [rcx+56]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+64]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+72]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+80]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+88]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    rax, r13, [rcx+48]       // hi0->rax lo0->r13
    adcx    r8, r13                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r13, r8, [rcx+56]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+64]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+72]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+80]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+88]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r8, rax, [rcx+48]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    rax, r9, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+64]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+80]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+88]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r9, r8, [rcx+48]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r8, r10, [rcx+56]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+72]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+88]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r10, r9, [rcx+48]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r9, r11, [rcx+56]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+64]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+72]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+80]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+88]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r11, r10, [rcx+48]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r10, r12, [rcx+56]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+72]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+88]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r12, r11, [rcx+48]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r11, r14, [rcx+56]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+64]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+72]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+80]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+88]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r14, r12, [rcx+48]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r12, r13, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+64]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+80]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+88]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+48]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r14, rax, [rcx+56]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+64]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+72]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+80]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+88]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+48]       // hi0->rax lo0->r13
    adcx    r8, r13                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r13, r8, [rcx+56]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+64]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+72]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+80]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+88]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, rax, [rcx+48]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    rax, r9, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+64]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+80]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+88]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r8, [rcx+48]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r8, r10, [rcx+56]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+72]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+88]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r9, [rcx+48]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r9, r11, [rcx+56]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+64]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+72]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+80]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+88]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r10, [rcx+48]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r10, r12, [rcx+56]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+72]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+88]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r12, r11, [rcx+48]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+120]            // 重载行乘数
    mulx    r11, r14, [rcx+56]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+64]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+72]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+80]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+88]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    r14, r12, [rcx+48]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+128]            // 重载行乘数
    mulx    r12, r13, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+64]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+80]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+88]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, rax
    add     rdx, rbp                // += pc
    mov     [rdi+184], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+192], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+200], rdx
    mov     rdx, r10
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+208], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+216], rdx
    mov     rdx, r12
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+224], rdx
    # === pass 2: numb[12..17) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+96]       // hi0->rax lo0->r14
    mov     rdx, [rdi+96]
    add     rdx, r14               // dst + lo0
    mov     [rdi+96], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r8, r14, [rcx+104]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col13 = hi0 + lo1
    mulx    r9, r14, [rcx+112]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col14 = hi1 + lo2
    mulx    r10, r14, [rcx+120]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col15 = hi2 + lo3
    mulx    r11, r14, [rcx+128]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col16 = hi3 + lo4
    adcx    r11, rbx                  // col17 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+96]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, rax, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+112]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+128]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r14, [rcx+96]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r14, r8, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+112]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+128]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r14, [rcx+96]       // hi0->rax lo0->r14
    adcx    r9, r14                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r14, r9, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+112]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+128]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r14, [rcx+96]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r14, r10, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+112]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+128]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r14, [rcx+96]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r14, r11, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+112]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+128]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r14, [rcx+96]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r14, r12, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+112]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+128]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r14, [rcx+96]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r13, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+112]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+128]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r14, [rcx+96]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r14, rax, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+112]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+128]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+96]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r14, r8, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+112]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+128]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r14, [rcx+96]       // hi0->rax lo0->r14
    adcx    r9, r14                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r14, r9, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+112]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+128]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, r14, [rcx+96]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col23 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+184]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+184], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r14, r10, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+112]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+128]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r14, [rcx+96]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col24 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+192]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+192], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r14, r11, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+112]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+128]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r14, [rcx+96]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col25 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+200]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+200], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r14, r12, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+112]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+128]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r14, [rcx+96]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col26 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+208]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+208], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r14, r13, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+112]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+128]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r12, r14, [rcx+96]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col27 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+216]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+216], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+120]            // 重载行乘数
    mulx    r14, rax, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+112]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+128]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    r13, r14, [rcx+96]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col28 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+224]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+224], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+128]            // 重载行乘数
    mulx    r14, r8, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+112]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+128]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, r9
    add     rdx, rbp                // += pc
    mov     [rdi+232], rdx
    mov     rdx, r10
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+240], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+248], rdx
    mov     rdx, r12
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+256], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    mulx    r12, r14, [rcx+40]     // hi5->r12 lo5->r14
    adcx    r11, r14                // col5 = hi4 + lo5
    adcx    r12, rbx                  // col6 = hi5 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r8, rax, [rcx+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    rax, r9, [rcx+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r9, r8, [rcx+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r8, r10, [rcx+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+24]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+40]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r10, r9, [rcx+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r9, r11, [rcx+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+24]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+40]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r11, r10, [rcx+0]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r10, r12, [rcx+8]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+24]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+40]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r12, r11, [rcx+0]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col7 完成, CF=cf1
    mov     [rdi+56], r14           // dst[7]
    mulx    r11, r14, [rcx+8]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+24]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+40]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r14, r12, [rcx+0]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col8 完成, CF=cf1
    mov     [rdi+64], r13           // dst[8]
    mulx    r12, r13, [rcx+8]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+16]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+24]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+32]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+40]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col9 完成, CF=cf1
    mov     [rdi+72], rax           // dst[9]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+0]       // hi0->rax lo0->r13
    adcx    r8, r13                // col10 完成, CF=cf1
    mov     [rdi+80], r8           // dst[10]
    mulx    r13, r8, [rcx+8]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+24]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+40]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, rax, [rcx+0]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col11 完成, CF=cf1
    mov     [rdi+88], r9           // dst[11]
    mulx    rax, r9, [rcx+8]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+24]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+40]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r8, [rcx+0]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col12 完成, CF=cf1
    mov     [rdi+96], r10           // dst[12]
    mulx    r8, r10, [rcx+8]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+24]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+40]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r9, [rcx+0]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col13 完成, CF=cf1
    mov     [rdi+104], r11           // dst[13]
    mulx    r9, r11, [rcx+8]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+24]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+40]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r10, [rcx+0]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col14 完成, CF=cf1
    mov     [rdi+112], r12           // dst[14]
    mulx    r10, r12, [rcx+8]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+24]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+40]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r12, r11, [rcx+0]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col15 完成, CF=cf1
    mov     [rdi+120], r14           // dst[15]
    mulx    r11, r14, [rcx+8]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+24]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+40]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    r14, r12, [rcx+0]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col16 完成, CF=cf1
    mov     [rdi+128], r13           // dst[16]
    mulx    r12, r13, [rcx+8]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+16]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+24]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+32]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+40]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+136]            // 行乘数 17
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col17 完成, CF=cf1
    mov     [rdi+136], rax           // dst[17]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+40]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     [rdi+144], r8
    mov     [rdi+152], r9
    mov     [rdi+160], r10
    mov     [rdi+168], r11
    mov     [rdi+176], r12
    mov     [rdi+184], r14
    # === pass 1: numb[6..12) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r8, r13, [rcx+48]       // hi0->r8 lo0->r13
    mov     rdx, [rdi+48]
    add     rdx, r13               // dst + lo0
    mov     [rdi+48], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r9, r13, [rcx+56]     // hi1->r9 lo1->r13
    adcx    r8, r13                // col7 = hi0 + lo1
    mulx    r10, r13, [rcx+64]     // hi2->r10 lo2->r13
    adcx    r9, r13                // col8 = hi1 + lo2
    mulx    r11, r13, [rcx+72]     // hi3->r11 lo3->r13
    adcx    r10, r13                // col9 = hi2 + lo3
    mulx    r12, r13, [rcx+80]     // hi4->r12 lo4->r13
    adcx    r11, r13                // col10 = hi3 + lo4
    mulx    r14, r13, [rcx+88]     // hi5->r14 lo5->r13
    adcx    r12, r13                // col11 = hi4 + lo5
    adcx    r14, rbx                  // col12 = hi5 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    rax, r13, [rcx+48]       // hi0->rax lo0->r13
    adcx    r8, r13                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r13, r8, [rcx+56]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+64]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+72]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+80]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+88]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r8, rax, [rcx+48]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    rax, r9, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+64]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+80]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+88]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r9, r8, [rcx+48]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r8, r10, [rcx+56]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+72]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+88]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r10, r9, [rcx+48]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r9, r11, [rcx+56]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+64]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+72]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+80]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+88]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r11, r10, [rcx+48]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r10, r12, [rcx+56]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+72]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+88]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r12, r11, [rcx+48]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r11, r14, [rcx+56]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+64]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+72]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+80]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+88]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r14, r12, [rcx+48]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r12, r13, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+64]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+80]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+88]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r13, r14, [rcx+48]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r14, rax, [rcx+56]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+64]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+72]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+80]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+88]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    rax, r13, [rcx+48]       // hi0->rax lo0->r13
    adcx    r8, r13                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r13, r8, [rcx+56]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+64]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+72]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+80]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+88]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    r8, rax, [rcx+48]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    rax, r9, [rcx+56]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+64]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+72]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+80]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+88]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r9, r8, [rcx+48]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r8, r10, [rcx+56]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+64]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+72]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+80]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+88]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r10, r9, [rcx+48]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r9, r11, [rcx+56]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+64]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+72]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+80]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+88]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r11, r10, [rcx+48]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r10, r12, [rcx+56]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+64]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+72]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+80]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+88]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r12, r11, [rcx+48]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r11, r14, [rcx+56]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+64]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+72]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+80]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+88]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r14, r12, [rcx+48]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+120]            // 重载行乘数
    mulx    r12, r13, [rcx+56]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+64]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+72]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+80]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+88]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    r13, r14, [rcx+48]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+128]            // 重载行乘数
    mulx    r14, rax, [rcx+56]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+64]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+72]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+80]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+88]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+136]            // 行乘数 17
    mulx    rax, r13, [rcx+48]       // hi0->rax lo0->r13
    adcx    r8, r13                // col23 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+184]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+184], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+136]            // 重载行乘数
    mulx    r13, r8, [rcx+56]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+64]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+72]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+80]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+88]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, r9
    add     rdx, rbp                // += pc
    mov     [rdi+192], rdx
    mov     rdx, r10
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+200], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+208], rdx
    mov     rdx, r12
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+216], rdx
    mov     rdx, r14
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+224], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+232], rdx
    # === pass 2: numb[12..18) x numa, 环宽 6 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r9, rax, [rcx+96]       // hi0->r9 lo0->rax
    mov     rdx, [rdi+96]
    add     rdx, rax               // dst + lo0
    mov     [rdi+96], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r10, rax, [rcx+104]     // hi1->r10 lo1->rax
    adcx    r9, rax                // col13 = hi0 + lo1
    mulx    r11, rax, [rcx+112]     // hi2->r11 lo2->rax
    adcx    r10, rax                // col14 = hi1 + lo2
    mulx    r12, rax, [rcx+120]     // hi3->r12 lo3->rax
    adcx    r11, rax                // col15 = hi2 + lo3
    mulx    r14, rax, [rcx+128]     // hi4->r14 lo4->rax
    adcx    r12, rax                // col16 = hi3 + lo4
    mulx    r13, rax, [rcx+136]     // hi5->r13 lo5->rax
    adcx    r14, rax                // col17 = hi4 + lo5
    adcx    r13, rbx                  // col18 = hi5 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r8, rax, [rcx+96]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    rax, r9, [rcx+104]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+112]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+120]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+128]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+136]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r9, r8, [rcx+96]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r8, r10, [rcx+104]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+112]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+120]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+128]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+136]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r10, r9, [rcx+96]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r9, r11, [rcx+104]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+112]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+120]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+128]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+136]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r11, r10, [rcx+96]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r10, r12, [rcx+104]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+112]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+120]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+128]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+136]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r12, r11, [rcx+96]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r11, r14, [rcx+104]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+112]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+120]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+128]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+136]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r14, r12, [rcx+96]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r12, r13, [rcx+104]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+112]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+120]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+128]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+136]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r13, r14, [rcx+96]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, rax, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+112]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+128]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+136]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    rax, r13, [rcx+96]       // hi0->rax lo0->r13
    adcx    r8, r13                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r13, r8, [rcx+104]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+112]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+120]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+128]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+136]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r8, rax, [rcx+96]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    rax, r9, [rcx+104]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+112]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+120]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+128]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+136]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    r9, r8, [rcx+96]       // hi0->r9 lo0->r8
    adcx    r10, r8                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r8, r10, [rcx+104]     // hi1->r8 lo1->ring0
    adcx    r11, r9               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r9, r10, [rcx+112]     // hi2->r9 lo2->ring0
    adcx    r12, r8               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r8, r10, [rcx+120]     // hi3->r8 lo3->ring0
    adcx    r14, r9               // CF 链: += hi2
    adox    r14, r10               // OF 链: += lo3
    mulx    r9, r10, [rcx+128]     // hi4->r9 lo4->ring0
    adcx    r13, r8               // CF 链: += hi3
    adox    r13, r10               // OF 链: += lo4
    mulx    r8, r10, [rcx+136]     // hi5->r8 lo5->ring0
    adcx    rax, r9               // CF 链: += hi4
    adox    rax, r10               // OF 链: += lo5
    adcx    r8, rbx                 // 顶列 = hi5 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r10, r9, [rcx+96]       // hi0->r10 lo0->r9
    adcx    r11, r9                // col23 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+184]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+184], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r9, r11, [rcx+104]     // hi1->r9 lo1->ring0
    adcx    r12, r10               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r10, r11, [rcx+112]     // hi2->r10 lo2->ring0
    adcx    r14, r9               // CF 链: += hi1
    adox    r14, r11               // OF 链: += lo2
    mulx    r9, r11, [rcx+120]     // hi3->r9 lo3->ring0
    adcx    r13, r10               // CF 链: += hi2
    adox    r13, r11               // OF 链: += lo3
    mulx    r10, r11, [rcx+128]     // hi4->r10 lo4->ring0
    adcx    rax, r9               // CF 链: += hi3
    adox    rax, r11               // OF 链: += lo4
    mulx    r9, r11, [rcx+136]     // hi5->r9 lo5->ring0
    adcx    r8, r10               // CF 链: += hi4
    adox    r8, r11               // OF 链: += lo5
    adcx    r9, rbx                 // 顶列 = hi5 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r11, r10, [rcx+96]       // hi0->r11 lo0->r10
    adcx    r12, r10                // col24 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+192]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+192], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r10, r12, [rcx+104]     // hi1->r10 lo1->ring0
    adcx    r14, r11               // CF 链: += hi0
    adox    r14, r12               // OF 链: += lo1
    mulx    r11, r12, [rcx+112]     // hi2->r11 lo2->ring0
    adcx    r13, r10               // CF 链: += hi1
    adox    r13, r12               // OF 链: += lo2
    mulx    r10, r12, [rcx+120]     // hi3->r10 lo3->ring0
    adcx    rax, r11               // CF 链: += hi2
    adox    rax, r12               // OF 链: += lo3
    mulx    r11, r12, [rcx+128]     // hi4->r11 lo4->ring0
    adcx    r8, r10               // CF 链: += hi3
    adox    r8, r12               // OF 链: += lo4
    mulx    r10, r12, [rcx+136]     // hi5->r10 lo5->ring0
    adcx    r9, r11               // CF 链: += hi4
    adox    r9, r12               // OF 链: += lo5
    adcx    r10, rbx                 // 顶列 = hi5 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r12, r11, [rcx+96]       // hi0->r12 lo0->r11
    adcx    r14, r11                // col25 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+200]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+200], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r11, r14, [rcx+104]     // hi1->r11 lo1->ring0
    adcx    r13, r12               // CF 链: += hi0
    adox    r13, r14               // OF 链: += lo1
    mulx    r12, r14, [rcx+112]     // hi2->r12 lo2->ring0
    adcx    rax, r11               // CF 链: += hi1
    adox    rax, r14               // OF 链: += lo2
    mulx    r11, r14, [rcx+120]     // hi3->r11 lo3->ring0
    adcx    r8, r12               // CF 链: += hi2
    adox    r8, r14               // OF 链: += lo3
    mulx    r12, r14, [rcx+128]     // hi4->r12 lo4->ring0
    adcx    r9, r11               // CF 链: += hi3
    adox    r9, r14               // OF 链: += lo4
    mulx    r11, r14, [rcx+136]     // hi5->r11 lo5->ring0
    adcx    r10, r12               // CF 链: += hi4
    adox    r10, r14               // OF 链: += lo5
    adcx    r11, rbx                 // 顶列 = hi5 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r14, r12, [rcx+96]       // hi0->r14 lo0->r12
    adcx    r13, r12                // col26 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+208]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+208], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r12, r13, [rcx+104]     // hi1->r12 lo1->ring0
    adcx    rax, r14               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r14, r13, [rcx+112]     // hi2->r14 lo2->ring0
    adcx    r8, r12               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r12, r13, [rcx+120]     // hi3->r12 lo3->ring0
    adcx    r9, r14               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r14, r13, [rcx+128]     // hi4->r14 lo4->ring0
    adcx    r10, r12               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    mulx    r12, r13, [rcx+136]     // hi5->r12 lo5->ring0
    adcx    r11, r14               // CF 链: += hi4
    adox    r11, r13               // OF 链: += lo5
    adcx    r12, rbx                 // 顶列 = hi5 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r13, r14, [rcx+96]       // hi0->r13 lo0->r14
    adcx    rax, r14                // col27 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+216]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+216], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+120]            // 重载行乘数
    mulx    r14, rax, [rcx+104]     // hi1->r14 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+112]     // hi2->r13 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+120]     // hi3->r14 lo3->ring0
    adcx    r10, r13               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r13, rax, [rcx+128]     // hi4->r13 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    mulx    r14, rax, [rcx+136]     // hi5->r14 lo5->ring0
    adcx    r12, r13               // CF 链: += hi4
    adox    r12, rax               // OF 链: += lo5
    adcx    r14, rbx                 // 顶列 = hi5 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    rax, r13, [rcx+96]       // hi0->rax lo0->r13
    adcx    r8, r13                // col28 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+224]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+224], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+128]            // 重载行乘数
    mulx    r13, r8, [rcx+104]     // hi1->r13 lo1->ring0
    adcx    r9, rax               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+112]     // hi2->rax lo2->ring0
    adcx    r10, r13               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+120]     // hi3->r13 lo3->ring0
    adcx    r11, rax               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    rax, r8, [rcx+128]     // hi4->rax lo4->ring0
    adcx    r12, r13               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    mulx    r13, r8, [rcx+136]     // hi5->r13 lo5->ring0
    adcx    r14, rax               // CF 链: += hi4
    adox    r14, r8               // OF 链: += lo5
    adcx    r13, rbx                 // 顶列 = hi5 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+136]            // 行乘数 17
    mulx    r8, rax, [rcx+96]       // hi0->r8 lo0->rax
    adcx    r9, rax                // col29 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+232]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+232], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+136]            // 重载行乘数
    mulx    rax, r9, [rcx+104]     // hi1->rax lo1->ring0
    adcx    r10, r8               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    r8, r9, [rcx+112]     // hi2->r8 lo2->ring0
    adcx    r11, rax               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    rax, r9, [rcx+120]     // hi3->rax lo3->ring0
    adcx    r12, r8               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    r8, r9, [rcx+128]     // hi4->r8 lo4->ring0
    adcx    r14, rax               // CF 链: += hi3
    adox    r14, r9               // OF 链: += lo4
    mulx    rax, r9, [rcx+136]     // hi5->rax lo5->ring0
    adcx    r13, r8               // CF 链: += hi4
    adox    r13, r9               // OF 链: += lo5
    adcx    rax, rbx                 // 顶列 = hi5 + 进位
    adox    rax, rbx
    mov     rdx, r10
    add     rdx, rbp                // += pc
    mov     [rdi+240], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+248], rdx
    mov     rdx, r12
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+256], rdx
    mov     rdx, r14
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+264], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+272], rdx
    mov     rdx, rax
    adc     rdx, 0               // 链式消化进位
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
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    mov     [rdi+0], r14           // dst[0] = lo0
    mulx    r8, r14, [rcx+8]     // hi1->r8 lo1->r14
    adcx    rax, r14                // col1 = hi0 + lo1
    mulx    r9, r14, [rcx+16]     // hi2->r9 lo2->r14
    adcx    r8, r14                // col2 = hi1 + lo2
    mulx    r10, r14, [rcx+24]     // hi3->r10 lo3->r14
    adcx    r9, r14                // col3 = hi2 + lo3
    mulx    r11, r14, [rcx+32]     // hi4->r11 lo4->r14
    adcx    r10, r14                // col4 = hi3 + lo4
    adcx    r11, rbx                  // col5 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col1 完成, CF=cf1
    mov     [rdi+8], rax           // dst[1]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col2 完成, CF=cf1
    mov     [rdi+16], r8           // dst[2]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col3 完成, CF=cf1
    mov     [rdi+24], r9           // dst[3]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col4 完成, CF=cf1
    mov     [rdi+32], r10           // dst[4]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col5 完成, CF=cf1
    mov     [rdi+40], r11           // dst[5]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r10, r14, [rcx+0]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col6 完成, CF=cf1
    mov     [rdi+48], r12           // dst[6]
    mulx    r14, r12, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r11, r14, [rcx+0]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col7 完成, CF=cf1
    mov     [rdi+56], r13           // dst[7]
    mulx    r14, r13, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col8 完成, CF=cf1
    mov     [rdi+64], rax           // dst[8]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col9 完成, CF=cf1
    mov     [rdi+72], r8           // dst[9]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col10 完成, CF=cf1
    mov     [rdi+80], r9           // dst[10]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col11 完成, CF=cf1
    mov     [rdi+88], r10           // dst[11]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r9, r14, [rcx+0]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col12 完成, CF=cf1
    mov     [rdi+96], r11           // dst[12]
    mulx    r14, r11, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+16]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+32]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r10, r14, [rcx+0]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col13 完成, CF=cf1
    mov     [rdi+104], r12           // dst[13]
    mulx    r14, r12, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+16]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+32]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r11, r14, [rcx+0]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col14 完成, CF=cf1
    mov     [rdi+112], r13           // dst[14]
    mulx    r14, r13, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+16]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+32]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r12, r14, [rcx+0]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col15 完成, CF=cf1
    mov     [rdi+120], rax           // dst[15]
    mulx    r14, rax, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+16]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+32]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    r13, r14, [rcx+0]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col16 完成, CF=cf1
    mov     [rdi+128], r8           // dst[16]
    mulx    r14, r8, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+16]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+32]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+136]            // 行乘数 17
    mulx    rax, r14, [rcx+0]       // hi0->rax lo0->r14
    adcx    r9, r14                // col17 完成, CF=cf1
    mov     [rdi+136], r9           // dst[17]
    mulx    r14, r9, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+16]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+32]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+144]            // 行乘数 18
    mulx    r8, r14, [rcx+0]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col18 完成, CF=cf1
    mov     [rdi+144], r10           // dst[18]
    mulx    r14, r10, [rcx+8]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+16]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+24]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+32]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     [rdi+152], r11
    mov     [rdi+160], r12
    mov     [rdi+168], r13
    mov     [rdi+176], rax
    mov     [rdi+184], r8
    # === pass 1: numb[5..10) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r11, r14, [rcx+40]       // hi0->r11 lo0->r14
    mov     rdx, [rdi+40]
    add     rdx, r14               // dst + lo0
    mov     [rdi+40], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r12, r14, [rcx+48]     // hi1->r12 lo1->r14
    adcx    r11, r14                // col6 = hi0 + lo1
    mulx    r13, r14, [rcx+56]     // hi2->r13 lo2->r14
    adcx    r12, r14                // col7 = hi1 + lo2
    mulx    rax, r14, [rcx+64]     // hi3->rax lo3->r14
    adcx    r13, r14                // col8 = hi2 + lo3
    mulx    r8, r14, [rcx+72]     // hi4->r8 lo4->r14
    adcx    rax, r14                // col9 = hi3 + lo4
    adcx    r8, rbx                  // col10 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col6 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+48]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+48], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, r11, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+72]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r10, r14, [rcx+40]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col7 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+56]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+56], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r14, r12, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+56]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+72]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r11, r14, [rcx+40]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col8 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+64]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+64], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r14, r13, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+56]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+72]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col9 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+72]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+72], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r14, rax, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+72]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r13, r14, [rcx+40]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col10 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+80]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+80], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r14, r8, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+56]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+72]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    rax, r14, [rcx+40]       // hi0->rax lo0->r14
    adcx    r9, r14                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r14, r9, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+72]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r8, r14, [rcx+40]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r10, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+72]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r14, r11, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+72]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r10, r14, [rcx+40]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r14, r12, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+56]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+72]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    r11, r14, [rcx+40]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r14, r13, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+56]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+72]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r14, rax, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+72]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r13, r14, [rcx+40]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r14, r8, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+56]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+72]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    rax, r14, [rcx+40]       // hi0->rax lo0->r14
    adcx    r9, r14                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r14, r9, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+56]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+72]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r8, r14, [rcx+40]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r14, r10, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+56]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+72]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r9, r14, [rcx+40]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+120]            // 重载行乘数
    mulx    r14, r11, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+56]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+72]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    r10, r14, [rcx+40]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+128]            // 重载行乘数
    mulx    r14, r12, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+56]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+72]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+136]            // 行乘数 17
    mulx    r11, r14, [rcx+40]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+136]            // 重载行乘数
    mulx    r14, r13, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+56]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+72]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+144]            // 行乘数 18
    mulx    r12, r14, [rcx+40]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col23 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+184]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+184], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+144]            // 重载行乘数
    mulx    r14, rax, [rcx+48]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+56]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+64]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+72]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, r8
    add     rdx, rbp                // += pc
    mov     [rdi+192], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+200], rdx
    mov     rdx, r10
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+208], rdx
    mov     rdx, r11
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+216], rdx
    mov     rdx, r12
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+224], rdx
    # === pass 2: numb[10..15) x numa, 环宽 5 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r8, r14, [rcx+80]       // hi0->r8 lo0->r14
    mov     rdx, [rdi+80]
    add     rdx, r14               // dst + lo0
    mov     [rdi+80], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r9, r14, [rcx+88]     // hi1->r9 lo1->r14
    adcx    r8, r14                // col11 = hi0 + lo1
    mulx    r10, r14, [rcx+96]     // hi2->r10 lo2->r14
    adcx    r9, r14                // col12 = hi1 + lo2
    mulx    r11, r14, [rcx+104]     // hi3->r11 lo3->r14
    adcx    r10, r14                // col13 = hi2 + lo3
    mulx    r12, r14, [rcx+112]     // hi4->r12 lo4->r14
    adcx    r11, r14                // col14 = hi3 + lo4
    adcx    r12, rbx                  // col15 = hi4 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r13, r14, [rcx+80]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col11 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+88]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+88], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, r8, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+96]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+112]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    rax, r14, [rcx+80]       // hi0->rax lo0->r14
    adcx    r9, r14                // col12 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+96]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+96], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r14, r9, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+96]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+112]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r8, r14, [rcx+80]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col13 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+104]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+104], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r14, r10, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+96]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+112]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    r9, r14, [rcx+80]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col14 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+112]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+112], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r14, r11, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+96]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+112]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r10, r14, [rcx+80]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col15 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+120]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+120], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    r14, r12, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+96]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+112]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r11, r14, [rcx+80]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r14, r13, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+96]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+112]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r12, r14, [rcx+80]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, rax, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+96]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+112]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r13, r14, [rcx+80]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r14, r8, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+96]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+112]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    rax, r14, [rcx+80]       // hi0->rax lo0->r14
    adcx    r9, r14                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r14, r9, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+96]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+112]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    r8, r14, [rcx+80]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r14, r10, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+96]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+112]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r9, r14, [rcx+80]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    r14, r11, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+96]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+112]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r10, r14, [rcx+80]       // hi0->r10 lo0->r14
    adcx    r12, r14                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r14, r12, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r13, r10               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r10, r12, [rcx+96]     // hi2->r10 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r8, r10               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    mulx    r10, r12, [rcx+112]     // hi4->r10 lo4->ring0
    adcx    r9, r14               // CF 链: += hi3
    adox    r9, r12               // OF 链: += lo4
    adcx    r10, rbx                 // 顶列 = hi4 + 进位
    adox    r10, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r11, r14, [rcx+80]       // hi0->r11 lo0->r14
    adcx    r13, r14                // col23 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+184]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+184], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r14, r13, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    rax, r11               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r11, r13, [rcx+96]     // hi2->r11 lo2->ring0
    adcx    r8, r14               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r14, r13, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r9, r11               // CF 链: += hi2
    adox    r9, r13               // OF 链: += lo3
    mulx    r11, r13, [rcx+112]     // hi4->r11 lo4->ring0
    adcx    r10, r14               // CF 链: += hi3
    adox    r10, r13               // OF 链: += lo4
    adcx    r11, rbx                 // 顶列 = hi4 + 进位
    adox    r11, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r12, r14, [rcx+80]       // hi0->r12 lo0->r14
    adcx    rax, r14                // col24 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+192]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+192], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r14, rax, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r8, r12               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r12, rax, [rcx+96]     // hi2->r12 lo2->ring0
    adcx    r9, r14               // CF 链: += hi1
    adox    r9, rax               // OF 链: += lo2
    mulx    r14, rax, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r10, r12               // CF 链: += hi2
    adox    r10, rax               // OF 链: += lo3
    mulx    r12, rax, [rcx+112]     // hi4->r12 lo4->ring0
    adcx    r11, r14               // CF 链: += hi3
    adox    r11, rax               // OF 链: += lo4
    adcx    r12, rbx                 // 顶列 = hi4 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r13, r14, [rcx+80]       // hi0->r13 lo0->r14
    adcx    r8, r14                // col25 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+200]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+200], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+120]            // 重载行乘数
    mulx    r14, r8, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r9, r13               // CF 链: += hi0
    adox    r9, r8               // OF 链: += lo1
    mulx    r13, r8, [rcx+96]     // hi2->r13 lo2->ring0
    adcx    r10, r14               // CF 链: += hi1
    adox    r10, r8               // OF 链: += lo2
    mulx    r14, r8, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r11, r13               // CF 链: += hi2
    adox    r11, r8               // OF 链: += lo3
    mulx    r13, r8, [rcx+112]     // hi4->r13 lo4->ring0
    adcx    r12, r14               // CF 链: += hi3
    adox    r12, r8               // OF 链: += lo4
    adcx    r13, rbx                 // 顶列 = hi4 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    rax, r14, [rcx+80]       // hi0->rax lo0->r14
    adcx    r9, r14                // col26 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+208]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+208], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+128]            // 重载行乘数
    mulx    r14, r9, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r10, rax               // CF 链: += hi0
    adox    r10, r9               // OF 链: += lo1
    mulx    rax, r9, [rcx+96]     // hi2->rax lo2->ring0
    adcx    r11, r14               // CF 链: += hi1
    adox    r11, r9               // OF 链: += lo2
    mulx    r14, r9, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r9               // OF 链: += lo3
    mulx    rax, r9, [rcx+112]     // hi4->rax lo4->ring0
    adcx    r13, r14               // CF 链: += hi3
    adox    r13, r9               // OF 链: += lo4
    adcx    rax, rbx                 // 顶列 = hi4 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+136]            // 行乘数 17
    mulx    r8, r14, [rcx+80]       // hi0->r8 lo0->r14
    adcx    r10, r14                // col27 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+216]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r10              // += ring0
    mov     [rdi+216], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+136]            // 重载行乘数
    mulx    r14, r10, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r11, r8               // CF 链: += hi0
    adox    r11, r10               // OF 链: += lo1
    mulx    r8, r10, [rcx+96]     // hi2->r8 lo2->ring0
    adcx    r12, r14               // CF 链: += hi1
    adox    r12, r10               // OF 链: += lo2
    mulx    r14, r10, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r10               // OF 链: += lo3
    mulx    r8, r10, [rcx+112]     // hi4->r8 lo4->ring0
    adcx    rax, r14               // CF 链: += hi3
    adox    rax, r10               // OF 链: += lo4
    adcx    r8, rbx                 // 顶列 = hi4 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+144]            // 行乘数 18
    mulx    r9, r14, [rcx+80]       // hi0->r9 lo0->r14
    adcx    r11, r14                // col28 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+224]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r11              // += ring0
    mov     [rdi+224], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+144]            // 重载行乘数
    mulx    r14, r11, [rcx+88]     // hi1->r14 lo1->ring0
    adcx    r12, r9               // CF 链: += hi0
    adox    r12, r11               // OF 链: += lo1
    mulx    r9, r11, [rcx+96]     // hi2->r9 lo2->ring0
    adcx    r13, r14               // CF 链: += hi1
    adox    r13, r11               // OF 链: += lo2
    mulx    r14, r11, [rcx+104]     // hi3->r14 lo3->ring0
    adcx    rax, r9               // CF 链: += hi2
    adox    rax, r11               // OF 链: += lo3
    mulx    r9, r11, [rcx+112]     // hi4->r9 lo4->ring0
    adcx    r8, r14               // CF 链: += hi3
    adox    r8, r11               // OF 链: += lo4
    adcx    r9, rbx                 // 顶列 = hi4 + 进位
    adox    r9, rbx
    mov     rdx, r12
    add     rdx, rbp                // += pc
    mov     [rdi+232], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+240], rdx
    mov     rdx, rax
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+248], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+256], rdx
    mov     rdx, r9
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+264], rdx
    # === pass 3: numb[15..19) x numa, 环宽 4 ===
    mov     rdx, [rsi+0]            // 行乘数 0
    mulx    r12, r14, [rcx+120]       // hi0->r12 lo0->r14
    mov     rdx, [rdi+120]
    add     rdx, r14               // dst + lo0
    mov     [rdi+120], rdx
    setc    bpl
    movzx   rbp, bpl              // pc = CF
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+0]            // 重载行乘数
    mulx    r13, r14, [rcx+128]     // hi1->r13 lo1->r14
    adcx    r12, r14                // col16 = hi0 + lo1
    mulx    rax, r14, [rcx+136]     // hi2->rax lo2->r14
    adcx    r13, r14                // col17 = hi1 + lo2
    mulx    r8, r14, [rcx+144]     // hi3->r8 lo3->r14
    adcx    rax, r14                // col18 = hi2 + lo3
    adcx    r8, rbx                  // col19 = hi3 + CF
    mov     rdx, [rsi+8]            // 行乘数 1
    mulx    r9, r14, [rcx+120]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col16 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+128]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+128], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+8]            // 重载行乘数
    mulx    r14, r12, [rcx+128]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+136]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+144]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+16]            // 行乘数 2
    mulx    r12, r9, [rcx+120]       // hi0->r12 lo0->r9
    adcx    r13, r9                // col17 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+136]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+136], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+16]            // 重载行乘数
    mulx    r9, r13, [rcx+128]     // hi1->r9 lo1->ring0
    adcx    rax, r12               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+136]     // hi2->r12 lo2->ring0
    adcx    r8, r9               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r9, r13, [rcx+144]     // hi3->r9 lo3->ring0
    adcx    r14, r12               // CF 链: += hi2
    adox    r14, r13               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+24]            // 行乘数 3
    mulx    r13, r12, [rcx+120]       // hi0->r13 lo0->r12
    adcx    rax, r12                // col18 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+144]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+144], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+24]            // 重载行乘数
    mulx    r12, rax, [rcx+128]     // hi1->r12 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+136]     // hi2->r13 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+144]     // hi3->r12 lo3->ring0
    adcx    r9, r13               // CF 链: += hi2
    adox    r9, rax               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+32]            // 行乘数 4
    mulx    rax, r13, [rcx+120]       // hi0->rax lo0->r13
    adcx    r8, r13                // col19 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+152]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+152], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+32]            // 重载行乘数
    mulx    r13, r8, [rcx+128]     // hi1->r13 lo1->ring0
    adcx    r14, rax               // CF 链: += hi0
    adox    r14, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+136]     // hi2->rax lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+144]     // hi3->r13 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r13, rbx                 // 顶列 = hi3 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+40]            // 行乘数 5
    mulx    r8, rax, [rcx+120]       // hi0->r8 lo0->rax
    adcx    r14, rax                // col20 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+160]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+160], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+40]            // 重载行乘数
    mulx    rax, r14, [rcx+128]     // hi1->rax lo1->ring0
    adcx    r9, r8               // CF 链: += hi0
    adox    r9, r14               // OF 链: += lo1
    mulx    r8, r14, [rcx+136]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+144]     // hi3->rax lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r14               // OF 链: += lo3
    adcx    rax, rbx                 // 顶列 = hi3 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+48]            // 行乘数 6
    mulx    r14, r8, [rcx+120]       // hi0->r14 lo0->r8
    adcx    r9, r8                // col21 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+168]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+168], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+48]            // 重载行乘数
    mulx    r8, r9, [rcx+128]     // hi1->r8 lo1->ring0
    adcx    r12, r14               // CF 链: += hi0
    adox    r12, r9               // OF 链: += lo1
    mulx    r14, r9, [rcx+136]     // hi2->r14 lo2->ring0
    adcx    r13, r8               // CF 链: += hi1
    adox    r13, r9               // OF 链: += lo2
    mulx    r8, r9, [rcx+144]     // hi3->r8 lo3->ring0
    adcx    rax, r14               // CF 链: += hi2
    adox    rax, r9               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+56]            // 行乘数 7
    mulx    r9, r14, [rcx+120]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col22 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+176]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+176], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+56]            // 重载行乘数
    mulx    r14, r12, [rcx+128]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+136]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+144]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+64]            // 行乘数 8
    mulx    r12, r9, [rcx+120]       // hi0->r12 lo0->r9
    adcx    r13, r9                // col23 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+184]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+184], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+64]            // 重载行乘数
    mulx    r9, r13, [rcx+128]     // hi1->r9 lo1->ring0
    adcx    rax, r12               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+136]     // hi2->r12 lo2->ring0
    adcx    r8, r9               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r9, r13, [rcx+144]     // hi3->r9 lo3->ring0
    adcx    r14, r12               // CF 链: += hi2
    adox    r14, r13               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+72]            // 行乘数 9
    mulx    r13, r12, [rcx+120]       // hi0->r13 lo0->r12
    adcx    rax, r12                // col24 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+192]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+192], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+72]            // 重载行乘数
    mulx    r12, rax, [rcx+128]     // hi1->r12 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+136]     // hi2->r13 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+144]     // hi3->r12 lo3->ring0
    adcx    r9, r13               // CF 链: += hi2
    adox    r9, rax               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+80]            // 行乘数 10
    mulx    rax, r13, [rcx+120]       // hi0->rax lo0->r13
    adcx    r8, r13                // col25 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+200]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+200], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+80]            // 重载行乘数
    mulx    r13, r8, [rcx+128]     // hi1->r13 lo1->ring0
    adcx    r14, rax               // CF 链: += hi0
    adox    r14, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+136]     // hi2->rax lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+144]     // hi3->r13 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r13, rbx                 // 顶列 = hi3 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+88]            // 行乘数 11
    mulx    r8, rax, [rcx+120]       // hi0->r8 lo0->rax
    adcx    r14, rax                // col26 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+208]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+208], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+88]            // 重载行乘数
    mulx    rax, r14, [rcx+128]     // hi1->rax lo1->ring0
    adcx    r9, r8               // CF 链: += hi0
    adox    r9, r14               // OF 链: += lo1
    mulx    r8, r14, [rcx+136]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+144]     // hi3->rax lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r14               // OF 链: += lo3
    adcx    rax, rbx                 // 顶列 = hi3 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+96]            // 行乘数 12
    mulx    r14, r8, [rcx+120]       // hi0->r14 lo0->r8
    adcx    r9, r8                // col27 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+216]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+216], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+96]            // 重载行乘数
    mulx    r8, r9, [rcx+128]     // hi1->r8 lo1->ring0
    adcx    r12, r14               // CF 链: += hi0
    adox    r12, r9               // OF 链: += lo1
    mulx    r14, r9, [rcx+136]     // hi2->r14 lo2->ring0
    adcx    r13, r8               // CF 链: += hi1
    adox    r13, r9               // OF 链: += lo2
    mulx    r8, r9, [rcx+144]     // hi3->r8 lo3->ring0
    adcx    rax, r14               // CF 链: += hi2
    adox    rax, r9               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, [rsi+104]            // 行乘数 13
    mulx    r9, r14, [rcx+120]       // hi0->r9 lo0->r14
    adcx    r12, r14                // col28 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+224]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r12              // += ring0
    mov     [rdi+224], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+104]            // 重载行乘数
    mulx    r14, r12, [rcx+128]     // hi1->r14 lo1->ring0
    adcx    r13, r9               // CF 链: += hi0
    adox    r13, r12               // OF 链: += lo1
    mulx    r9, r12, [rcx+136]     // hi2->r9 lo2->ring0
    adcx    rax, r14               // CF 链: += hi1
    adox    rax, r12               // OF 链: += lo2
    mulx    r14, r12, [rcx+144]     // hi3->r14 lo3->ring0
    adcx    r8, r9               // CF 链: += hi2
    adox    r8, r12               // OF 链: += lo3
    adcx    r14, rbx                 // 顶列 = hi3 + 进位
    adox    r14, rbx
    mov     rdx, [rsi+112]            // 行乘数 14
    mulx    r12, r9, [rcx+120]       // hi0->r12 lo0->r9
    adcx    r13, r9                // col29 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+232]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r13              // += ring0
    mov     [rdi+232], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+112]            // 重载行乘数
    mulx    r9, r13, [rcx+128]     // hi1->r9 lo1->ring0
    adcx    rax, r12               // CF 链: += hi0
    adox    rax, r13               // OF 链: += lo1
    mulx    r12, r13, [rcx+136]     // hi2->r12 lo2->ring0
    adcx    r8, r9               // CF 链: += hi1
    adox    r8, r13               // OF 链: += lo2
    mulx    r9, r13, [rcx+144]     // hi3->r9 lo3->ring0
    adcx    r14, r12               // CF 链: += hi2
    adox    r14, r13               // OF 链: += lo3
    adcx    r9, rbx                 // 顶列 = hi3 + 进位
    adox    r9, rbx
    mov     rdx, [rsi+120]            // 行乘数 15
    mulx    r13, r12, [rcx+120]       // hi0->r13 lo0->r12
    adcx    rax, r12                // col30 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+240]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, rax              // += ring0
    mov     [rdi+240], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+120]            // 重载行乘数
    mulx    r12, rax, [rcx+128]     // hi1->r12 lo1->ring0
    adcx    r8, r13               // CF 链: += hi0
    adox    r8, rax               // OF 链: += lo1
    mulx    r13, rax, [rcx+136]     // hi2->r13 lo2->ring0
    adcx    r14, r12               // CF 链: += hi1
    adox    r14, rax               // OF 链: += lo2
    mulx    r12, rax, [rcx+144]     // hi3->r12 lo3->ring0
    adcx    r9, r13               // CF 链: += hi2
    adox    r9, rax               // OF 链: += lo3
    adcx    r12, rbx                 // 顶列 = hi3 + 进位
    adox    r12, rbx
    mov     rdx, [rsi+128]            // 行乘数 16
    mulx    rax, r13, [rcx+120]       // hi0->rax lo0->r13
    adcx    r8, r13                // col31 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+248]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r8              // += ring0
    mov     [rdi+248], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+128]            // 重载行乘数
    mulx    r13, r8, [rcx+128]     // hi1->r13 lo1->ring0
    adcx    r14, rax               // CF 链: += hi0
    adox    r14, r8               // OF 链: += lo1
    mulx    rax, r8, [rcx+136]     // hi2->rax lo2->ring0
    adcx    r9, r13               // CF 链: += hi1
    adox    r9, r8               // OF 链: += lo2
    mulx    r13, r8, [rcx+144]     // hi3->r13 lo3->ring0
    adcx    r12, rax               // CF 链: += hi2
    adox    r12, r8               // OF 链: += lo3
    adcx    r13, rbx                 // 顶列 = hi3 + 进位
    adox    r13, rbx
    mov     rdx, [rsi+136]            // 行乘数 17
    mulx    r8, rax, [rcx+120]       // hi0->r8 lo0->rax
    adcx    r14, rax                // col32 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+256]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r14              // += ring0
    mov     [rdi+256], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+136]            // 重载行乘数
    mulx    rax, r14, [rcx+128]     // hi1->rax lo1->ring0
    adcx    r9, r8               // CF 链: += hi0
    adox    r9, r14               // OF 链: += lo1
    mulx    r8, r14, [rcx+136]     // hi2->r8 lo2->ring0
    adcx    r12, rax               // CF 链: += hi1
    adox    r12, r14               // OF 链: += lo2
    mulx    rax, r14, [rcx+144]     // hi3->rax lo3->ring0
    adcx    r13, r8               // CF 链: += hi2
    adox    r13, r14               // OF 链: += lo3
    adcx    rax, rbx                 // 顶列 = hi3 + 进位
    adox    rax, rbx
    mov     rdx, [rsi+144]            // 行乘数 18
    mulx    r14, r8, [rcx+120]       // hi0->r14 lo0->r8
    adcx    r9, r8                // col33 完成, CF=cf1
    setc    r15b                  // R = cf1
    mov     rdx, [rdi+264]
    add     rdx, rbp              // += pc
    setc    bpl
    movzx   rbp, bpl              // pc = gamma_a
    add     rdx, r9              // += ring0
    mov     [rdi+264], rdx
    adc     rbp, 0                // pc += gamma_b
    add     rbp, r15               // pc += cf1
    xor     r15d, r15d              // R = 0
    mov     rdx, [rsi+144]            // 重载行乘数
    mulx    r8, r9, [rcx+128]     // hi1->r8 lo1->ring0
    adcx    r12, r14               // CF 链: += hi0
    adox    r12, r9               // OF 链: += lo1
    mulx    r14, r9, [rcx+136]     // hi2->r14 lo2->ring0
    adcx    r13, r8               // CF 链: += hi1
    adox    r13, r9               // OF 链: += lo2
    mulx    r8, r9, [rcx+144]     // hi3->r8 lo3->ring0
    adcx    rax, r14               // CF 链: += hi2
    adox    rax, r9               // OF 链: += lo3
    adcx    r8, rbx                 // 顶列 = hi3 + 进位
    adox    r8, rbx
    mov     rdx, r12
    add     rdx, rbp                // += pc
    mov     [rdi+272], rdx
    mov     rdx, r13
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+280], rdx
    mov     rdx, rax
    adc     rdx, 0               // 链式消化进位
    mov     [rdi+288], rdx
    mov     rdx, r8
    adc     rdx, 0               // 链式消化进位
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
