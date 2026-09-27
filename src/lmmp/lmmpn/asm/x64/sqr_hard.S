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
// void lmmp_sqr_hard_N_(mp_ptr dst, mp_srcptr numa);
//
//   平衡硬编码平方: [dst,2N] = [numa,N]^2。
//   N <= 3 : 库内 sqr_basecase.S 小规模分支的特化
//   N >= 4 : 交叉积列累加(FLINT 式, 8 列寄存器窗口 + adcx/adox 双链,
//            完成列溢写栈数组) -> 整体倍增 -> 对角平方单链折叠
//

#include "lmmp_asm.h"

.text

#ifdef LMMP_WINDOWS
    #define PUSH_RSI push rsi
    #define PUSH_RDI push rdi
    #define POP_RSI  pop rsi
    #define POP_RDI  pop rdi
#else
    #define PUSH_RSI
    #define PUSH_RDI
    #define POP_RSI
    #define POP_RDI
#endif

.globl ASM_GSYM(lmmp_sqr_hard_1_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_1_):

    mov     rax, [rx1]
    mul     rax
    mov     [rx0], rax
    mov     [rx0+8], rdx
    ret

.globl ASM_GSYM(lmmp_sqr_hard_2_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_2_):

    mov     rax, [rx1]
    mov     r11, [rx1+8]
    mov     r8, rax
    mul     rax
    mov     [rx0], rax
    mov     rax, r11
    mov     r9, rdx
    mul     rax
    mov     r10, rax
    mov     rax, r11
    mov     r11, rdx
    mul     r8
    xor     r8, r8
    add     r9, rax
    adc     r10, rdx
    adc     r11, r8
    add     r9, rax
    mov     [rx0+8], r9
    adc     r10, rdx
    mov     [rx0+16], r10
    adc     r11, r8
    mov     [rx0+24], r11
    ret

.globl ASM_GSYM(lmmp_sqr_hard_3_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_3_):

    PUSH_RSI
    PUSH_RDI
    mov     rsi, rx1
    mov     rdi, rx0
    mov     rax, [rsi]
    mov     r10, rax
    mul     rax
    mov     r11, [rsi+8]
    mov     [rdi], rax
    mov     rax, r11
    mov     [rdi+8], rdx
    mul     rax
    mov     rcx, [rsi+16]
    mov     [rdi+16], rax
    mov     rax, rcx
    mov     [rdi+24], rdx
    mul     rax
    mov     [rdi+32], rax
    mov     [rdi+40], rdx
    mov     rax, r11
    mul     r10
    mov     r8, rax
    mov     rax, rcx
    mov     r9, rdx
    mul     r10
    xor     r10, r10
    add     r9, rax
    mov     rax, r11
    mov     r11, r10
    adc     r10, rdx
    mul     rcx
    add     r10, rax
    adc     rdx, r11
    add     r8, r8
    adc     r9, r9
    adc     r10, r10
    adc     rdx, rdx
    adc     r11, r11
    add     [rdi+8], r8
    adc     [rdi+16], r9
    adc     [rdi+24], r10
    adc     [rdi+32], rdx
    adc     [rdi+40], r11
    POP_RDI
    POP_RSI
    ret

.globl ASM_GSYM(lmmp_sqr_hard_4_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_4_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbp
    push    r12
    push    r13
    push    r15
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mov     r10d, 0
    mov     r12d, 0
    mov     r13d, 0
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    adcx    r8, rax
    mulx    r11, r15, [rsi+24]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adcx    r10, r11
    adcx    r12, r11
    // --- 行 1: 乘数 a_1 ---
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+24]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 2: 乘数 a_2 ---
    mov     rdx, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adcx    r13, r11
    // --- 倍增 ---
    add     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    adc     r12, r12
    adc     r13, r13
    setc    r11b
    movzx   r11, r11b
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, rbp
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, r8
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, r9
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, r10
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, r12
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, r13
    mov     [rdi+48], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+56], r15
    pop     r15
    pop     r13
    pop     r12
    pop     rbp
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_5_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_5_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mov     r12d, 0
    mov     r13d, 0
    mov     r14d, 0
    mov     ebx, 0
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r11, r15, [rsi+32]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 1: 乘数 a_1 ---
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+32]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adox    r14, r11
    adcx    r13, r11
    adcx    r14, r11
    // --- 行 2: 乘数 a_2 ---
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+32]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 3: 乘数 a_3 ---
    mov     rdx, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adcx    rbx, r11
    // --- 倍增 ---
    add     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    adc     r12, r12
    adc     r13, r13
    adc     r14, r14
    adc     rbx, rbx
    setc    r11b
    movzx   r11, r11b
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, rbp
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, r8
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, r9
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, r10
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, r12
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, r13
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, r14
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, rbx
    mov     [rdi+64], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+72], r15
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

.globl ASM_GSYM(lmmp_sqr_hard_6_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_6_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 96               // M 交叉列数组, M[c]=[rsp+8c], c=1..10
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mov     r13d, 0
    mov     r14d, 0
    mov     ebx, 0
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    adcx    r10, rax
    mulx    r11, r15, [rsi+40]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adcx    r13, r11
    adcx    r14, r11
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     ebp, 0
    mov     r8d, 0
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+40]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+40]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adox    rbp, r11
    adcx    rbx, r11
    adcx    rbp, r11
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+40]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     rdx, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adcx    r8, r11
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    // --- 倍增 ---
    mov     rax, [rsp+8]
    mov     rcx, [rsp+16]
    mov     rdx, [rsp+24]
    mov     rbx, [rsp+32]
    add     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+40]
    mov     r8, [rsp+48]
    mov     r9, [rsp+56]
    mov     r10, [rsp+64]
    mov     [rsp+8], rax
    mov     [rsp+16], rcx
    mov     [rsp+24], rdx
    mov     [rsp+32], rbx
    adc     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mov     [rsp+40], rbp
    mov     [rsp+48], r8
    mov     [rsp+56], r9
    mov     [rsp+64], r10
    adc     rax, rax
    adc     rcx, rcx
    mov     [rsp+72], rax
    mov     [rsp+80], rcx
    setc    r11b
    movzx   r11, r11b
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+88], r15
    add     rsp, 96
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

.globl ASM_GSYM(lmmp_sqr_hard_7_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_7_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 112               // M 交叉列数组, M[c]=[rsp+8c], c=1..12
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mov     r14d, 0
    mov     ebx, 0
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r11, r15, [rsi+48]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     ebp, 0
    mov     r8d, 0
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+48]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adox    rbp, r11
    adcx    rbx, r11
    adcx    rbp, r11
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9d, 0
    mov     r10d, 0
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+48]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+48]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adox    r9, r11
    adcx    r8, r11
    adcx    r9, r11
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+48]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rdx, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adcx    r10, r11
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    // --- 倍增 ---
    mov     rax, [rsp+8]
    mov     rcx, [rsp+16]
    mov     rdx, [rsp+24]
    mov     rbx, [rsp+32]
    add     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+40]
    mov     r8, [rsp+48]
    mov     r9, [rsp+56]
    mov     r10, [rsp+64]
    mov     [rsp+8], rax
    mov     [rsp+16], rcx
    mov     [rsp+24], rdx
    mov     [rsp+32], rbx
    adc     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mov     rdx, [rsp+88]
    mov     rbx, [rsp+96]
    mov     [rsp+40], rbp
    mov     [rsp+48], r8
    mov     [rsp+56], r9
    mov     [rsp+64], r10
    adc     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     [rsp+72], rax
    mov     [rsp+80], rcx
    mov     [rsp+88], rdx
    mov     [rsp+96], rbx
    setc    r11b
    movzx   r11, r11b
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+104], r15
    add     rsp, 112
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

.globl ASM_GSYM(lmmp_sqr_hard_8_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_8_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 128               // M 交叉列数组, M[c]=[rsp+8c], c=1..14
    mov     QWORD PTR [rsp+72], 0    // M[9] 预清零
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mov     ebx, 0
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    adcx    r13, rax
    mulx    r11, r15, [rsi+56]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adcx    rbx, r11
    mov     r15, [rsp+72]
    adcx    r15, r11
    mov     [rsp+72], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8d, 0
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+56]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9d, 0
    mov     r10d, 0
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+56]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adox    r9, r11
    adcx    r8, r11
    adcx    r9, r11
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12d, 0
    mov     r13d, 0
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+56]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    r11, r15, [rsi+56]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adox    r12, r11
    adcx    r10, r11
    adcx    r12, r11
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+56]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     rdx, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adcx    r13, r11
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    // --- 倍增 ---
    mov     rax, [rsp+8]
    mov     rcx, [rsp+16]
    mov     rdx, [rsp+24]
    mov     rbx, [rsp+32]
    add     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+40]
    mov     r8, [rsp+48]
    mov     r9, [rsp+56]
    mov     r10, [rsp+64]
    mov     [rsp+8], rax
    mov     [rsp+16], rcx
    mov     [rsp+24], rdx
    mov     [rsp+32], rbx
    adc     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mov     rdx, [rsp+88]
    mov     rbx, [rsp+96]
    mov     [rsp+40], rbp
    mov     [rsp+48], r8
    mov     [rsp+56], r9
    mov     [rsp+64], r10
    adc     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+104]
    mov     r8, [rsp+112]
    mov     [rsp+72], rax
    mov     [rsp+80], rcx
    mov     [rsp+88], rdx
    mov     [rsp+96], rbx
    adc     rbp, rbp
    adc     r8, r8
    mov     [rsp+104], rbp
    mov     [rsp+112], r8
    setc    r11b
    movzx   r11, r11b
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+120], r15
    add     rsp, 128
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

.globl ASM_GSYM(lmmp_sqr_hard_9_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_9_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 144               // M 交叉列数组, M[c]=[rsp+8c], c=1..16
    xorps   xmm0, xmm0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    mov     QWORD PTR [rsp+88], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mulx    r11, r15, [rsi+64]
    mov     rdx, [rsp+72]
    adox    rdx, r11
    adcx    rbx, r15
    mov     r11d, 0
    adcx    rdx, r11
    mov     [rsp+72], rdx
    mov     r15, [rsp+80]
    adcx    r15, r11
    mov     [rsp+80], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+64]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    mov     r15, [rsp+88]
    adox    r15, r11
    adcx    r8, r11
    adcx    r15, r11
    mov     [rsp+88], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10d, 0
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+64]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12d, 0
    mov     r13d, 0
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    r11, r15, [rsi+64]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adox    r12, r11
    adcx    r10, r11
    adcx    r12, r11
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14d, 0
    mov     ebx, 0
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+64]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+64]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adox    r14, r11
    adcx    r13, r11
    adcx    r14, r11
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+64]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     rdx, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adcx    rbx, r11
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    // --- 倍增 ---
    mov     rax, [rsp+8]
    mov     rcx, [rsp+16]
    mov     rdx, [rsp+24]
    mov     rbx, [rsp+32]
    add     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+40]
    mov     r8, [rsp+48]
    mov     r9, [rsp+56]
    mov     r10, [rsp+64]
    mov     [rsp+8], rax
    mov     [rsp+16], rcx
    mov     [rsp+24], rdx
    mov     [rsp+32], rbx
    adc     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mov     rdx, [rsp+88]
    mov     rbx, [rsp+96]
    mov     [rsp+40], rbp
    mov     [rsp+48], r8
    mov     [rsp+56], r9
    mov     [rsp+64], r10
    adc     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+104]
    mov     r8, [rsp+112]
    mov     r9, [rsp+120]
    mov     r10, [rsp+128]
    mov     [rsp+72], rax
    mov     [rsp+80], rcx
    mov     [rsp+88], rdx
    mov     [rsp+96], rbx
    adc     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    mov     [rsp+104], rbp
    mov     [rsp+112], r8
    mov     [rsp+120], r9
    mov     [rsp+128], r10
    setc    r11b
    movzx   r11, r11b
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+136], r15
    add     rsp, 144
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

.globl ASM_GSYM(lmmp_sqr_hard_10_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_10_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 160               // M 交叉列数组, M[c]=[rsp+8c], c=1..18
    xorps   xmm0, xmm0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    mov     QWORD PTR [rsp+104], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    mov     rdx, [rsp+80]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+72], rax
    mov     r11d, 0
    adcx    rdx, r11
    mov     [rsp+80], rdx
    mov     r15, [rsp+88]
    adcx    r15, r11
    mov     [rsp+88], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+72]
    mov     rdx, [rsp+88]
    adox    rdx, r11
    adcx    r8, r15
    mov     r11d, 0
    mov     r15, [rsp+96]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+88], rdx
    adcx    r15, r11
    mov     [rsp+96], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    r11, r15, [rsi+72]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    mov     r15, [rsp+104]
    adox    r15, r11
    adcx    r10, r11
    adcx    r15, r11
    mov     [rsp+104], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13d, 0
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+72]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14d, 0
    mov     ebx, 0
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+72]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adox    r14, r11
    adcx    r13, r11
    adcx    r14, r11
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     ebp, 0
    mov     r8d, 0
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+72]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+72]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adox    rbp, r11
    adcx    rbx, r11
    adcx    rbp, r11
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+72]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     rdx, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adcx    r8, r11
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    // --- 倍增 ---
    mov     rax, [rsp+8]
    mov     rcx, [rsp+16]
    mov     rdx, [rsp+24]
    mov     rbx, [rsp+32]
    add     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+40]
    mov     r8, [rsp+48]
    mov     r9, [rsp+56]
    mov     r10, [rsp+64]
    mov     [rsp+8], rax
    mov     [rsp+16], rcx
    mov     [rsp+24], rdx
    mov     [rsp+32], rbx
    adc     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mov     rdx, [rsp+88]
    mov     rbx, [rsp+96]
    mov     [rsp+40], rbp
    mov     [rsp+48], r8
    mov     [rsp+56], r9
    mov     [rsp+64], r10
    adc     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+104]
    mov     r8, [rsp+112]
    mov     r9, [rsp+120]
    mov     r10, [rsp+128]
    mov     [rsp+72], rax
    mov     [rsp+80], rcx
    mov     [rsp+88], rdx
    mov     [rsp+96], rbx
    adc     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    mov     rax, [rsp+136]
    mov     rcx, [rsp+144]
    mov     [rsp+104], rbp
    mov     [rsp+112], r8
    mov     [rsp+120], r9
    mov     [rsp+128], r10
    adc     rax, rax
    adc     rcx, rcx
    mov     [rsp+136], rax
    mov     [rsp+144], rcx
    setc    r11b
    movzx   r11, r11b
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+152], r15
    add     rsp, 160
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

.globl ASM_GSYM(lmmp_sqr_hard_11_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_11_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 176               // M 交叉列数组, M[c]=[rsp+8c], c=1..20
    xorps   xmm0, xmm0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    movdqu  [rsp+104], xmm0       // M[13..14] = 0
    mov     QWORD PTR [rsp+120], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+72], rax
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+80]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+80], rcx
    mov     r11d, 0
    adcx    rax, r11
    mov     [rsp+88], rax
    mov     r15, [rsp+96]
    adcx    r15, r11
    mov     [rsp+96], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+72]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+80]
    mov     rdx, [rsp+96]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+88], rax
    mov     r11d, 0
    mov     r15, [rsp+104]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+96], rdx
    adcx    r15, r11
    mov     [rsp+104], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+80]
    mov     rdx, [rsp+104]
    adox    rdx, r11
    adcx    r10, r15
    mov     r11d, 0
    mov     r15, [rsp+112]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+104], rdx
    adcx    r15, r11
    mov     [rsp+112], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13, [rsp+112]
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+80]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    mov     r15, [rsp+120]
    adox    r15, r11
    adcx    r13, r11
    adcx    r15, r11
    mov     [rsp+120], r15
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14, [rsp+120]
    mov     ebx, 0
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+80]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     ebp, 0
    mov     r8d, 0
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+80]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adox    rbp, r11
    adcx    rbx, r11
    adcx    rbp, r11
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     r9d, 0
    mov     r10d, 0
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+80]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+80]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adox    r9, r11
    adcx    r8, r11
    adcx    r9, r11
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     rdx, [rsi+64]
    mulx    rcx, rax, [rsi+72]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+80]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 9: 乘数 a_9 ---
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    mov     rdx, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adcx    r10, r11
    mov     [rsp+152], r9
    mov     [rsp+160], r10
    // --- 倍增 ---
    mov     rax, [rsp+8]
    mov     rcx, [rsp+16]
    mov     rdx, [rsp+24]
    mov     rbx, [rsp+32]
    add     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+40]
    mov     r8, [rsp+48]
    mov     r9, [rsp+56]
    mov     r10, [rsp+64]
    mov     [rsp+8], rax
    mov     [rsp+16], rcx
    mov     [rsp+24], rdx
    mov     [rsp+32], rbx
    adc     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mov     rdx, [rsp+88]
    mov     rbx, [rsp+96]
    mov     [rsp+40], rbp
    mov     [rsp+48], r8
    mov     [rsp+56], r9
    mov     [rsp+64], r10
    adc     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     rbp, [rsp+104]
    mov     r8, [rsp+112]
    mov     r9, [rsp+120]
    mov     r10, [rsp+128]
    mov     [rsp+72], rax
    mov     [rsp+80], rcx
    mov     [rsp+88], rdx
    mov     [rsp+96], rbx
    adc     rbp, rbp
    adc     r8, r8
    adc     r9, r9
    adc     r10, r10
    mov     rax, [rsp+136]
    mov     rcx, [rsp+144]
    mov     rdx, [rsp+152]
    mov     rbx, [rsp+160]
    mov     [rsp+104], rbp
    mov     [rsp+112], r8
    mov     [rsp+120], r9
    mov     [rsp+128], r10
    adc     rax, rax
    adc     rcx, rcx
    adc     rdx, rdx
    adc     rbx, rbx
    mov     [rsp+136], rax
    mov     [rsp+144], rcx
    mov     [rsp+152], rdx
    mov     [rsp+160], rbx
    setc    r11b
    movzx   r11, r11b
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, rcx
    adc     r15, [rsp+152]
    mov     [rdi+152], r15
    mov     rdx, [rsi+80]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+160]
    mov     [rdi+160], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+168], r15
    add     rsp, 176
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

.globl ASM_GSYM(lmmp_sqr_hard_12_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_12_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 192               // M 交叉列数组, M[c]=[rsp+8c], c=1..22
    xorps   xmm0, xmm0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    movdqu  [rsp+104], xmm0       // M[13..14] = 0
    movdqu  [rsp+120], xmm0       // M[15..16] = 0
    mov     QWORD PTR [rsp+136], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+72], rax
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+80]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+80], rcx
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+88]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+88], rax
    mov     r11d, 0
    adcx    rcx, r11
    mov     [rsp+96], rcx
    mov     r15, [rsp+104]
    adcx    r15, r11
    mov     [rsp+104], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+88]
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+72]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+80]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+88]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+96], rcx
    mov     r11d, 0
    mov     r15, [rsp+112]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+104], rax
    adcx    r15, r11
    mov     [rsp+112], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+80]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+88]
    mov     rdx, [rsp+112]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+104], rax
    mov     r11d, 0
    mov     r15, [rsp+120]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+112], rdx
    adcx    r15, r11
    mov     [rsp+120], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13, [rsp+112]
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+88]
    mov     rdx, [rsp+120]
    adox    rdx, r11
    adcx    r13, r15
    mov     r11d, 0
    mov     r15, [rsp+128]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+120], rdx
    adcx    r15, r11
    mov     [rsp+128], r15
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14, [rsp+120]
    mov     rbx, [rsp+128]
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+88]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    mov     r15, [rsp+136]
    adox    r15, r11
    adcx    rbx, r11
    adcx    r15, r11
    mov     [rsp+136], r15
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rbp, [rsp+136]
    mov     r8d, 0
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+80]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+88]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     r9d, 0
    mov     r10d, 0
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+88]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adox    r9, r11
    adcx    r8, r11
    adcx    r9, r11
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     r12d, 0
    mov     r13d, 0
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+80]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+88]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     rdx, [rsi+64]
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    r11, r15, [rsi+88]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adox    r12, r11
    adcx    r10, r11
    adcx    r12, r11
    // --- 行 9: 乘数 a_9 ---
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    mov     rdx, [rsi+72]
    mulx    rcx, rax, [rsi+80]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+88]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 10: 乘数 a_10 ---
    mov     [rsp+152], r9
    mov     [rsp+160], r10
    mov     rdx, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adcx    r13, r11
    mov     [rsp+168], r12
    mov     [rsp+176], r13
    // --- 倍增 ---
    mov     r11, [rsp+176]
    shr     r11, 63
    vmovdqu ymm0, [rsp+152]
    vmovdqu ymm1, [rsp+144]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+152], ymm0
    vmovdqu ymm0, [rsp+120]
    vmovdqu ymm1, [rsp+112]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+120], ymm0
    vmovdqu ymm0, [rsp+88]
    vmovdqu ymm1, [rsp+80]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+88], ymm0
    vmovdqu ymm0, [rsp+56]
    vmovdqu ymm1, [rsp+48]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+56], ymm0
    vmovdqu ymm0, [rsp+24]
    vmovdqu ymm1, [rsp+16]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+24], ymm0
    mov     rax, [rsp+8]
    add     rax, rax
    mov     [rsp+8], rax
    mov     rax, [rsp+16]
    adc     rax, rax
    mov     [rsp+16], rax
    vzeroupper
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, rcx
    adc     r15, [rsp+152]
    mov     [rdi+152], r15
    mov     rdx, [rsi+80]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+160]
    mov     [rdi+160], r15
    mov     r15, rcx
    adc     r15, [rsp+168]
    mov     [rdi+168], r15
    mov     rdx, [rsi+88]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+176]
    mov     [rdi+176], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+184], r15
    add     rsp, 192
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

.globl ASM_GSYM(lmmp_sqr_hard_13_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_13_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 208               // M 交叉列数组, M[c]=[rsp+8c], c=1..24
    xorps   xmm0, xmm0
    mov     QWORD PTR [rsp+0], 0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    movdqu  [rsp+104], xmm0       // M[13..14] = 0
    movdqu  [rsp+120], xmm0       // M[15..16] = 0
    movdqu  [rsp+136], xmm0       // M[17..18] = 0
    mov     QWORD PTR [rsp+152], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+72], rax
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+80]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+80], rcx
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+96]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+96], rcx
    mov     r11d, 0
    adcx    rax, r11
    mov     [rsp+104], rax
    mov     r15, [rsp+112]
    adcx    r15, r11
    mov     [rsp+112], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+88]
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+72]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+80]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+88]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+96]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+104], rax
    mov     r11d, 0
    mov     r15, [rsp+120]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+112], rcx
    adcx    r15, r11
    mov     [rsp+120], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+104]
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+80]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+96]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+112], rcx
    mov     r11d, 0
    mov     r15, [rsp+128]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+120], rax
    adcx    r15, r11
    mov     [rsp+128], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13, [rsp+112]
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+88]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+96]
    mov     rdx, [rsp+128]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+120], rax
    mov     r11d, 0
    mov     r15, [rsp+136]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+128], rdx
    adcx    r15, r11
    mov     [rsp+136], r15
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14, [rsp+120]
    mov     rbx, [rsp+128]
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+96]
    mov     rdx, [rsp+136]
    adox    rdx, r11
    adcx    rbx, r15
    mov     r11d, 0
    mov     r15, [rsp+144]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+136], rdx
    adcx    r15, r11
    mov     [rsp+144], r15
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rbp, [rsp+136]
    mov     r8, [rsp+144]
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+96]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    mov     r15, [rsp+152]
    adox    r15, r11
    adcx    r8, r11
    adcx    r15, r11
    mov     [rsp+152], r15
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     r9, [rsp+152]
    mov     r10d, 0
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+88]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+96]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     r12d, 0
    mov     r13d, 0
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    r11, r15, [rsi+96]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adox    r12, r11
    adcx    r10, r11
    adcx    r12, r11
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     r14d, 0
    mov     ebx, 0
    mov     rdx, [rsi+64]
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+88]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+96]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 9: 乘数 a_9 ---
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    mov     rdx, [rsi+72]
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+96]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adox    r14, r11
    adcx    r13, r11
    adcx    r14, r11
    // --- 行 10: 乘数 a_10 ---
    mov     [rsp+152], r9
    mov     [rsp+160], r10
    mov     rdx, [rsi+80]
    mulx    rcx, rax, [rsi+88]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+96]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 11: 乘数 a_11 ---
    mov     [rsp+168], r12
    mov     [rsp+176], r13
    mov     rdx, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adcx    rbx, r11
    mov     [rsp+184], r14
    mov     [rsp+192], rbx
    // --- 倍增 ---
    mov     r11, [rsp+192]
    shr     r11, 63
    vmovdqu ymm0, [rsp+168]
    vmovdqu ymm1, [rsp+160]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+168], ymm0
    vmovdqu ymm0, [rsp+136]
    vmovdqu ymm1, [rsp+128]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+136], ymm0
    vmovdqu ymm0, [rsp+104]
    vmovdqu ymm1, [rsp+96]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+104], ymm0
    vmovdqu ymm0, [rsp+72]
    vmovdqu ymm1, [rsp+64]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+72], ymm0
    vmovdqu ymm0, [rsp+40]
    vmovdqu ymm1, [rsp+32]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+40], ymm0
    vmovdqu ymm0, [rsp+8]
    vmovdqu ymm1, [rsp+0]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+8], ymm0
    vzeroupper
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, rcx
    adc     r15, [rsp+152]
    mov     [rdi+152], r15
    mov     rdx, [rsi+80]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+160]
    mov     [rdi+160], r15
    mov     r15, rcx
    adc     r15, [rsp+168]
    mov     [rdi+168], r15
    mov     rdx, [rsi+88]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+176]
    mov     [rdi+176], r15
    mov     r15, rcx
    adc     r15, [rsp+184]
    mov     [rdi+184], r15
    mov     rdx, [rsi+96]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+192]
    mov     [rdi+192], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+200], r15
    add     rsp, 208
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

.globl ASM_GSYM(lmmp_sqr_hard_14_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_14_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 224               // M 交叉列数组, M[c]=[rsp+8c], c=1..26
    xorps   xmm0, xmm0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    movdqu  [rsp+104], xmm0       // M[13..14] = 0
    movdqu  [rsp+120], xmm0       // M[15..16] = 0
    movdqu  [rsp+136], xmm0       // M[17..18] = 0
    movdqu  [rsp+152], xmm0       // M[19..20] = 0
    mov     QWORD PTR [rsp+168], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+72], rax
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+80]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+80], rcx
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+104]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+104], rax
    mov     r11d, 0
    adcx    rcx, r11
    mov     [rsp+112], rcx
    mov     r15, [rsp+120]
    adcx    r15, r11
    mov     [rsp+120], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+88]
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+72]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+80]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+88]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+104]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+112], rcx
    mov     r11d, 0
    mov     r15, [rsp+128]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+120], rax
    adcx    r15, r11
    mov     [rsp+128], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+104]
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+80]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+104]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+120], rax
    mov     r11d, 0
    mov     r15, [rsp+136]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+128], rcx
    adcx    r15, r11
    mov     [rsp+136], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13, [rsp+112]
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+120]
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+88]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+104]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+128], rcx
    mov     r11d, 0
    mov     r15, [rsp+144]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+136], rax
    adcx    r15, r11
    mov     [rsp+144], r15
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14, [rsp+120]
    mov     rbx, [rsp+128]
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+96]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+104]
    mov     rdx, [rsp+144]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+136], rax
    mov     r11d, 0
    mov     r15, [rsp+152]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+144], rdx
    adcx    r15, r11
    mov     [rsp+152], r15
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rbp, [rsp+136]
    mov     r8, [rsp+144]
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+104]
    mov     rdx, [rsp+152]
    adox    rdx, r11
    adcx    r8, r15
    mov     r11d, 0
    mov     r15, [rsp+160]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+152], rdx
    adcx    r15, r11
    mov     [rsp+160], r15
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     r9, [rsp+152]
    mov     r10, [rsp+160]
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    r11, r15, [rsi+104]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    mov     r15, [rsp+168]
    adox    r15, r11
    adcx    r10, r11
    adcx    r15, r11
    mov     [rsp+168], r15
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     r12, [rsp+168]
    mov     r13d, 0
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+96]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+104]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     r14d, 0
    mov     ebx, 0
    mov     rdx, [rsi+64]
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+104]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adox    r14, r11
    adcx    r13, r11
    adcx    r14, r11
    // --- 行 9: 乘数 a_9 ---
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    mov     ebp, 0
    mov     r8d, 0
    mov     rdx, [rsi+72]
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+96]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+104]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 10: 乘数 a_10 ---
    mov     [rsp+152], r9
    mov     [rsp+160], r10
    mov     rdx, [rsi+80]
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+104]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adox    rbp, r11
    adcx    rbx, r11
    adcx    rbp, r11
    // --- 行 11: 乘数 a_11 ---
    mov     [rsp+168], r12
    mov     [rsp+176], r13
    mov     rdx, [rsi+88]
    mulx    rcx, rax, [rsi+96]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+104]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 12: 乘数 a_12 ---
    mov     [rsp+184], r14
    mov     [rsp+192], rbx
    mov     rdx, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adcx    r8, r11
    mov     [rsp+200], rbp
    mov     [rsp+208], r8
    // --- 倍增 ---
    mov     r11, [rsp+208]
    shr     r11, 63
    vmovdqu ymm0, [rsp+184]
    vmovdqu ymm1, [rsp+176]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+184], ymm0
    vmovdqu ymm0, [rsp+152]
    vmovdqu ymm1, [rsp+144]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+152], ymm0
    vmovdqu ymm0, [rsp+120]
    vmovdqu ymm1, [rsp+112]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+120], ymm0
    vmovdqu ymm0, [rsp+88]
    vmovdqu ymm1, [rsp+80]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+88], ymm0
    vmovdqu ymm0, [rsp+56]
    vmovdqu ymm1, [rsp+48]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+56], ymm0
    vmovdqu ymm0, [rsp+24]
    vmovdqu ymm1, [rsp+16]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+24], ymm0
    mov     rax, [rsp+8]
    add     rax, rax
    mov     [rsp+8], rax
    mov     rax, [rsp+16]
    adc     rax, rax
    mov     [rsp+16], rax
    vzeroupper
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, rcx
    adc     r15, [rsp+152]
    mov     [rdi+152], r15
    mov     rdx, [rsi+80]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+160]
    mov     [rdi+160], r15
    mov     r15, rcx
    adc     r15, [rsp+168]
    mov     [rdi+168], r15
    mov     rdx, [rsi+88]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+176]
    mov     [rdi+176], r15
    mov     r15, rcx
    adc     r15, [rsp+184]
    mov     [rdi+184], r15
    mov     rdx, [rsi+96]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+192]
    mov     [rdi+192], r15
    mov     r15, rcx
    adc     r15, [rsp+200]
    mov     [rdi+200], r15
    mov     rdx, [rsi+104]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+208]
    mov     [rdi+208], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+216], r15
    add     rsp, 224
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

.globl ASM_GSYM(lmmp_sqr_hard_15_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_15_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 240               // M 交叉列数组, M[c]=[rsp+8c], c=1..28
    xorps   xmm0, xmm0
    mov     QWORD PTR [rsp+0], 0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    movdqu  [rsp+104], xmm0       // M[13..14] = 0
    movdqu  [rsp+120], xmm0       // M[15..16] = 0
    movdqu  [rsp+136], xmm0       // M[17..18] = 0
    movdqu  [rsp+152], xmm0       // M[19..20] = 0
    movdqu  [rsp+168], xmm0       // M[21..22] = 0
    mov     QWORD PTR [rsp+184], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+72], rax
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+80]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+80], rcx
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+112]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+112], rcx
    mov     r11d, 0
    adcx    rax, r11
    mov     [rsp+120], rax
    mov     r15, [rsp+128]
    adcx    r15, r11
    mov     [rsp+128], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+88]
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+72]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+80]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+88]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+112]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+120], rax
    mov     r11d, 0
    mov     r15, [rsp+136]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+128], rcx
    adcx    r15, r11
    mov     [rsp+136], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+104]
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+80]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+112]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+128], rcx
    mov     r11d, 0
    mov     r15, [rsp+144]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+136], rax
    adcx    r15, r11
    mov     [rsp+144], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13, [rsp+112]
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+120]
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+88]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+112]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+136], rax
    mov     r11d, 0
    mov     r15, [rsp+152]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+144], rcx
    adcx    r15, r11
    mov     [rsp+152], r15
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14, [rsp+120]
    mov     rbx, [rsp+128]
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    mov     rax, [rsp+136]
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+96]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+112]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+144], rcx
    mov     r11d, 0
    mov     r15, [rsp+160]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+152], rax
    adcx    r15, r11
    mov     [rsp+160], r15
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rbp, [rsp+136]
    mov     r8, [rsp+144]
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+104]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+112]
    mov     rdx, [rsp+160]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+152], rax
    mov     r11d, 0
    mov     r15, [rsp+168]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+160], rdx
    adcx    r15, r11
    mov     [rsp+168], r15
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     r9, [rsp+152]
    mov     r10, [rsp+160]
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+112]
    mov     rdx, [rsp+168]
    adox    rdx, r11
    adcx    r10, r15
    mov     r11d, 0
    mov     r15, [rsp+176]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+168], rdx
    adcx    r15, r11
    mov     [rsp+176], r15
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     r12, [rsp+168]
    mov     r13, [rsp+176]
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+112]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    mov     r15, [rsp+184]
    adox    r15, r11
    adcx    r13, r11
    adcx    r15, r11
    mov     [rsp+184], r15
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     r14, [rsp+184]
    mov     ebx, 0
    mov     rdx, [rsi+64]
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+104]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+112]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 9: 乘数 a_9 ---
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    mov     ebp, 0
    mov     r8d, 0
    mov     rdx, [rsi+72]
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+112]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adox    rbp, r11
    adcx    rbx, r11
    adcx    rbp, r11
    // --- 行 10: 乘数 a_10 ---
    mov     [rsp+152], r9
    mov     [rsp+160], r10
    mov     r9d, 0
    mov     r10d, 0
    mov     rdx, [rsi+80]
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+104]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+112]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 11: 乘数 a_11 ---
    mov     [rsp+168], r12
    mov     [rsp+176], r13
    mov     rdx, [rsi+88]
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+112]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adox    r9, r11
    adcx    r8, r11
    adcx    r9, r11
    // --- 行 12: 乘数 a_12 ---
    mov     [rsp+184], r14
    mov     [rsp+192], rbx
    mov     rdx, [rsi+96]
    mulx    rcx, rax, [rsi+104]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+112]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 13: 乘数 a_13 ---
    mov     [rsp+200], rbp
    mov     [rsp+208], r8
    mov     rdx, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adcx    r10, r11
    mov     [rsp+216], r9
    mov     [rsp+224], r10
    // --- 倍增 ---
    mov     r11, [rsp+224]
    shr     r11, 63
    vmovdqu ymm0, [rsp+200]
    vmovdqu ymm1, [rsp+192]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+200], ymm0
    vmovdqu ymm0, [rsp+168]
    vmovdqu ymm1, [rsp+160]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+168], ymm0
    vmovdqu ymm0, [rsp+136]
    vmovdqu ymm1, [rsp+128]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+136], ymm0
    vmovdqu ymm0, [rsp+104]
    vmovdqu ymm1, [rsp+96]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+104], ymm0
    vmovdqu ymm0, [rsp+72]
    vmovdqu ymm1, [rsp+64]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+72], ymm0
    vmovdqu ymm0, [rsp+40]
    vmovdqu ymm1, [rsp+32]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+40], ymm0
    vmovdqu ymm0, [rsp+8]
    vmovdqu ymm1, [rsp+0]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+8], ymm0
    vzeroupper
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, rcx
    adc     r15, [rsp+152]
    mov     [rdi+152], r15
    mov     rdx, [rsi+80]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+160]
    mov     [rdi+160], r15
    mov     r15, rcx
    adc     r15, [rsp+168]
    mov     [rdi+168], r15
    mov     rdx, [rsi+88]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+176]
    mov     [rdi+176], r15
    mov     r15, rcx
    adc     r15, [rsp+184]
    mov     [rdi+184], r15
    mov     rdx, [rsi+96]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+192]
    mov     [rdi+192], r15
    mov     r15, rcx
    adc     r15, [rsp+200]
    mov     [rdi+200], r15
    mov     rdx, [rsi+104]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+208]
    mov     [rdi+208], r15
    mov     r15, rcx
    adc     r15, [rsp+216]
    mov     [rdi+216], r15
    mov     rdx, [rsi+112]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+224]
    mov     [rdi+224], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+232], r15
    add     rsp, 240
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

.globl ASM_GSYM(lmmp_sqr_hard_16_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_16_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 256               // M 交叉列数组, M[c]=[rsp+8c], c=1..30
    xorps   xmm0, xmm0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    movdqu  [rsp+104], xmm0       // M[13..14] = 0
    movdqu  [rsp+120], xmm0       // M[15..16] = 0
    movdqu  [rsp+136], xmm0       // M[17..18] = 0
    movdqu  [rsp+152], xmm0       // M[19..20] = 0
    movdqu  [rsp+168], xmm0       // M[21..22] = 0
    movdqu  [rsp+184], xmm0       // M[23..24] = 0
    mov     QWORD PTR [rsp+200], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+72], rax
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+80]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+80], rcx
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+120]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+120], rax
    mov     r11d, 0
    adcx    rcx, r11
    mov     [rsp+128], rcx
    mov     r15, [rsp+136]
    adcx    r15, r11
    mov     [rsp+136], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+88]
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+72]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+80]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+88]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+120]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+128], rcx
    mov     r11d, 0
    mov     r15, [rsp+144]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+136], rax
    adcx    r15, r11
    mov     [rsp+144], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+104]
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+80]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+120]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+136], rax
    mov     r11d, 0
    mov     r15, [rsp+152]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+144], rcx
    adcx    r15, r11
    mov     [rsp+152], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13, [rsp+112]
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+120]
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+88]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+120]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+144], rcx
    mov     r11d, 0
    mov     r15, [rsp+160]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+152], rax
    adcx    r15, r11
    mov     [rsp+160], r15
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14, [rsp+120]
    mov     rbx, [rsp+128]
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    mov     rax, [rsp+136]
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+96]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+120]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+152], rax
    mov     r11d, 0
    mov     r15, [rsp+168]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+160], rcx
    adcx    r15, r11
    mov     [rsp+168], r15
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rbp, [rsp+136]
    mov     r8, [rsp+144]
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+152]
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+104]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+120]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+160], rcx
    mov     r11d, 0
    mov     r15, [rsp+176]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+168], rax
    adcx    r15, r11
    mov     [rsp+176], r15
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     r9, [rsp+152]
    mov     r10, [rsp+160]
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+112]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+120]
    mov     rdx, [rsp+176]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+168], rax
    mov     r11d, 0
    mov     r15, [rsp+184]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+176], rdx
    adcx    r15, r11
    mov     [rsp+184], r15
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     r12, [rsp+168]
    mov     r13, [rsp+176]
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+112]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+120]
    mov     rdx, [rsp+184]
    adox    rdx, r11
    adcx    r13, r15
    mov     r11d, 0
    mov     r15, [rsp+192]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+184], rdx
    adcx    r15, r11
    mov     [rsp+192], r15
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     r14, [rsp+184]
    mov     rbx, [rsp+192]
    mov     rdx, [rsi+64]
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+120]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    mov     r15, [rsp+200]
    adox    r15, r11
    adcx    rbx, r11
    adcx    r15, r11
    mov     [rsp+200], r15
    // --- 行 9: 乘数 a_9 ---
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    mov     rbp, [rsp+200]
    mov     r8d, 0
    mov     rdx, [rsi+72]
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+112]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+120]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 10: 乘数 a_10 ---
    mov     [rsp+152], r9
    mov     [rsp+160], r10
    mov     r9d, 0
    mov     r10d, 0
    mov     rdx, [rsi+80]
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+120]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adox    r9, r11
    adcx    r8, r11
    adcx    r9, r11
    // --- 行 11: 乘数 a_11 ---
    mov     [rsp+168], r12
    mov     [rsp+176], r13
    mov     r12d, 0
    mov     r13d, 0
    mov     rdx, [rsi+88]
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+112]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+120]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 12: 乘数 a_12 ---
    mov     [rsp+184], r14
    mov     [rsp+192], rbx
    mov     rdx, [rsi+96]
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    r11, r15, [rsi+120]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adox    r12, r11
    adcx    r10, r11
    adcx    r12, r11
    // --- 行 13: 乘数 a_13 ---
    mov     [rsp+200], rbp
    mov     [rsp+208], r8
    mov     rdx, [rsi+104]
    mulx    rcx, rax, [rsi+112]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+120]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 14: 乘数 a_14 ---
    mov     [rsp+216], r9
    mov     [rsp+224], r10
    mov     rdx, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adcx    r13, r11
    mov     [rsp+232], r12
    mov     [rsp+240], r13
    // --- 倍增 ---
    mov     r11, [rsp+240]
    shr     r11, 63
    vmovdqu ymm0, [rsp+216]
    vmovdqu ymm1, [rsp+208]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+216], ymm0
    vmovdqu ymm0, [rsp+184]
    vmovdqu ymm1, [rsp+176]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+184], ymm0
    vmovdqu ymm0, [rsp+152]
    vmovdqu ymm1, [rsp+144]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+152], ymm0
    vmovdqu ymm0, [rsp+120]
    vmovdqu ymm1, [rsp+112]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+120], ymm0
    vmovdqu ymm0, [rsp+88]
    vmovdqu ymm1, [rsp+80]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+88], ymm0
    vmovdqu ymm0, [rsp+56]
    vmovdqu ymm1, [rsp+48]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+56], ymm0
    vmovdqu ymm0, [rsp+24]
    vmovdqu ymm1, [rsp+16]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+24], ymm0
    mov     rax, [rsp+8]
    add     rax, rax
    mov     [rsp+8], rax
    mov     rax, [rsp+16]
    adc     rax, rax
    mov     [rsp+16], rax
    vzeroupper
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, rcx
    adc     r15, [rsp+152]
    mov     [rdi+152], r15
    mov     rdx, [rsi+80]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+160]
    mov     [rdi+160], r15
    mov     r15, rcx
    adc     r15, [rsp+168]
    mov     [rdi+168], r15
    mov     rdx, [rsi+88]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+176]
    mov     [rdi+176], r15
    mov     r15, rcx
    adc     r15, [rsp+184]
    mov     [rdi+184], r15
    mov     rdx, [rsi+96]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+192]
    mov     [rdi+192], r15
    mov     r15, rcx
    adc     r15, [rsp+200]
    mov     [rdi+200], r15
    mov     rdx, [rsi+104]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+208]
    mov     [rdi+208], r15
    mov     r15, rcx
    adc     r15, [rsp+216]
    mov     [rdi+216], r15
    mov     rdx, [rsi+112]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+224]
    mov     [rdi+224], r15
    mov     r15, rcx
    adc     r15, [rsp+232]
    mov     [rdi+232], r15
    mov     rdx, [rsi+120]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+240]
    mov     [rdi+240], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+248], r15
    add     rsp, 256
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

.globl ASM_GSYM(lmmp_sqr_hard_17_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_17_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 272               // M 交叉列数组, M[c]=[rsp+8c], c=1..32
    xorps   xmm0, xmm0
    mov     QWORD PTR [rsp+0], 0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    movdqu  [rsp+104], xmm0       // M[13..14] = 0
    movdqu  [rsp+120], xmm0       // M[15..16] = 0
    movdqu  [rsp+136], xmm0       // M[17..18] = 0
    movdqu  [rsp+152], xmm0       // M[19..20] = 0
    movdqu  [rsp+168], xmm0       // M[21..22] = 0
    movdqu  [rsp+184], xmm0       // M[23..24] = 0
    movdqu  [rsp+200], xmm0       // M[25..26] = 0
    mov     QWORD PTR [rsp+216], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+72], rax
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+80]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+80], rcx
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+128]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+128], rcx
    mov     r11d, 0
    adcx    rax, r11
    mov     [rsp+136], rax
    mov     r15, [rsp+144]
    adcx    r15, r11
    mov     [rsp+144], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+88]
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+72]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+80]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+88]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+120]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+128]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+136], rax
    mov     r11d, 0
    mov     r15, [rsp+152]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+144], rcx
    adcx    r15, r11
    mov     [rsp+152], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+104]
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+80]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+128]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+144], rcx
    mov     r11d, 0
    mov     r15, [rsp+160]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+152], rax
    adcx    r15, r11
    mov     [rsp+160], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13, [rsp+112]
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+120]
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+88]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+120]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+128]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+152], rax
    mov     r11d, 0
    mov     r15, [rsp+168]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+160], rcx
    adcx    r15, r11
    mov     [rsp+168], r15
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14, [rsp+120]
    mov     rbx, [rsp+128]
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    mov     rax, [rsp+136]
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+96]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+128]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+160], rcx
    mov     r11d, 0
    mov     r15, [rsp+176]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+168], rax
    adcx    r15, r11
    mov     [rsp+176], r15
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rbp, [rsp+136]
    mov     r8, [rsp+144]
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+152]
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+104]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+120]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+160], rcx
    mov     rcx, [rsp+176]
    mulx    r11, r15, [rsi+128]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+168], rax
    mov     r11d, 0
    mov     r15, [rsp+184]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+176], rcx
    adcx    r15, r11
    mov     [rsp+184], r15
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     r9, [rsp+152]
    mov     r10, [rsp+160]
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+168]
    mov     rcx, [rsp+176]
    mulx    r11, r15, [rsi+112]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+168], rax
    mov     rax, [rsp+184]
    mulx    r11, r15, [rsi+128]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+176], rcx
    mov     r11d, 0
    mov     r15, [rsp+192]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+184], rax
    adcx    r15, r11
    mov     [rsp+192], r15
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     r12, [rsp+168]
    mov     r13, [rsp+176]
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+112]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+184]
    mulx    r11, r15, [rsi+120]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+128]
    mov     rdx, [rsp+192]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+184], rax
    mov     r11d, 0
    mov     r15, [rsp+200]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+192], rdx
    adcx    r15, r11
    mov     [rsp+200], r15
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     r14, [rsp+184]
    mov     rbx, [rsp+192]
    mov     rdx, [rsi+64]
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+120]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+128]
    mov     rdx, [rsp+200]
    adox    rdx, r11
    adcx    rbx, r15
    mov     r11d, 0
    mov     r15, [rsp+208]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+200], rdx
    adcx    r15, r11
    mov     [rsp+208], r15
    // --- 行 9: 乘数 a_9 ---
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    mov     rbp, [rsp+200]
    mov     r8, [rsp+208]
    mov     rdx, [rsi+72]
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+128]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    mov     r15, [rsp+216]
    adox    r15, r11
    adcx    r8, r11
    adcx    r15, r11
    mov     [rsp+216], r15
    // --- 行 10: 乘数 a_10 ---
    mov     [rsp+152], r9
    mov     [rsp+160], r10
    mov     r9, [rsp+216]
    mov     r10d, 0
    mov     rdx, [rsi+80]
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+120]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+128]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 11: 乘数 a_11 ---
    mov     [rsp+168], r12
    mov     [rsp+176], r13
    mov     r12d, 0
    mov     r13d, 0
    mov     rdx, [rsi+88]
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    r11, r15, [rsi+128]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adox    r12, r11
    adcx    r10, r11
    adcx    r12, r11
    // --- 行 12: 乘数 a_12 ---
    mov     [rsp+184], r14
    mov     [rsp+192], rbx
    mov     r14d, 0
    mov     ebx, 0
    mov     rdx, [rsi+96]
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+120]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+128]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 13: 乘数 a_13 ---
    mov     [rsp+200], rbp
    mov     [rsp+208], r8
    mov     rdx, [rsi+104]
    mulx    rcx, rax, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+128]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adox    r14, r11
    adcx    r13, r11
    adcx    r14, r11
    // --- 行 14: 乘数 a_14 ---
    mov     [rsp+216], r9
    mov     [rsp+224], r10
    mov     rdx, [rsi+112]
    mulx    rcx, rax, [rsi+120]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+128]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 15: 乘数 a_15 ---
    mov     [rsp+232], r12
    mov     [rsp+240], r13
    mov     rdx, [rsi+120]
    mulx    r11, r15, [rsi+128]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adcx    rbx, r11
    mov     [rsp+248], r14
    mov     [rsp+256], rbx
    // --- 倍增 ---
    mov     r11, [rsp+256]
    shr     r11, 63
    vmovdqu ymm0, [rsp+232]
    vmovdqu ymm1, [rsp+224]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+232], ymm0
    vmovdqu ymm0, [rsp+200]
    vmovdqu ymm1, [rsp+192]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+200], ymm0
    vmovdqu ymm0, [rsp+168]
    vmovdqu ymm1, [rsp+160]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+168], ymm0
    vmovdqu ymm0, [rsp+136]
    vmovdqu ymm1, [rsp+128]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+136], ymm0
    vmovdqu ymm0, [rsp+104]
    vmovdqu ymm1, [rsp+96]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+104], ymm0
    vmovdqu ymm0, [rsp+72]
    vmovdqu ymm1, [rsp+64]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+72], ymm0
    vmovdqu ymm0, [rsp+40]
    vmovdqu ymm1, [rsp+32]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+40], ymm0
    vmovdqu ymm0, [rsp+8]
    vmovdqu ymm1, [rsp+0]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+8], ymm0
    vzeroupper
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, rcx
    adc     r15, [rsp+152]
    mov     [rdi+152], r15
    mov     rdx, [rsi+80]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+160]
    mov     [rdi+160], r15
    mov     r15, rcx
    adc     r15, [rsp+168]
    mov     [rdi+168], r15
    mov     rdx, [rsi+88]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+176]
    mov     [rdi+176], r15
    mov     r15, rcx
    adc     r15, [rsp+184]
    mov     [rdi+184], r15
    mov     rdx, [rsi+96]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+192]
    mov     [rdi+192], r15
    mov     r15, rcx
    adc     r15, [rsp+200]
    mov     [rdi+200], r15
    mov     rdx, [rsi+104]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+208]
    mov     [rdi+208], r15
    mov     r15, rcx
    adc     r15, [rsp+216]
    mov     [rdi+216], r15
    mov     rdx, [rsi+112]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+224]
    mov     [rdi+224], r15
    mov     r15, rcx
    adc     r15, [rsp+232]
    mov     [rdi+232], r15
    mov     rdx, [rsi+120]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+240]
    mov     [rdi+240], r15
    mov     r15, rcx
    adc     r15, [rsp+248]
    mov     [rdi+248], r15
    mov     rdx, [rsi+128]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+256]
    mov     [rdi+256], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+264], r15
    add     rsp, 272
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

.globl ASM_GSYM(lmmp_sqr_hard_18_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_18_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 288               // M 交叉列数组, M[c]=[rsp+8c], c=1..34
    xorps   xmm0, xmm0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    movdqu  [rsp+104], xmm0       // M[13..14] = 0
    movdqu  [rsp+120], xmm0       // M[15..16] = 0
    movdqu  [rsp+136], xmm0       // M[17..18] = 0
    movdqu  [rsp+152], xmm0       // M[19..20] = 0
    movdqu  [rsp+168], xmm0       // M[21..22] = 0
    movdqu  [rsp+184], xmm0       // M[23..24] = 0
    movdqu  [rsp+200], xmm0       // M[25..26] = 0
    movdqu  [rsp+216], xmm0       // M[27..28] = 0
    mov     QWORD PTR [rsp+232], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+72], rax
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+80]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+80], rcx
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+128]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+136]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+136], rax
    mov     r11d, 0
    adcx    rcx, r11
    mov     [rsp+144], rcx
    mov     r15, [rsp+152]
    adcx    r15, r11
    mov     [rsp+152], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+88]
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+72]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+80]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+88]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+120]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+128]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+136]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+144], rcx
    mov     r11d, 0
    mov     r15, [rsp+160]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+152], rax
    adcx    r15, r11
    mov     [rsp+160], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+104]
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+80]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+128]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+136]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+152], rax
    mov     r11d, 0
    mov     r15, [rsp+168]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+160], rcx
    adcx    r15, r11
    mov     [rsp+168], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13, [rsp+112]
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+120]
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+88]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+120]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+128]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+136]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+160], rcx
    mov     r11d, 0
    mov     r15, [rsp+176]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+168], rax
    adcx    r15, r11
    mov     [rsp+176], r15
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14, [rsp+120]
    mov     rbx, [rsp+128]
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    mov     rax, [rsp+136]
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+96]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+128]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+160], rcx
    mov     rcx, [rsp+176]
    mulx    r11, r15, [rsi+136]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+168], rax
    mov     r11d, 0
    mov     r15, [rsp+184]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+176], rcx
    adcx    r15, r11
    mov     [rsp+184], r15
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rbp, [rsp+136]
    mov     r8, [rsp+144]
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+152]
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+104]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+120]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+160], rcx
    mov     rcx, [rsp+176]
    mulx    r11, r15, [rsi+128]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+168], rax
    mov     rax, [rsp+184]
    mulx    r11, r15, [rsi+136]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+176], rcx
    mov     r11d, 0
    mov     r15, [rsp+192]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+184], rax
    adcx    r15, r11
    mov     [rsp+192], r15
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     r9, [rsp+152]
    mov     r10, [rsp+160]
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+168]
    mov     rcx, [rsp+176]
    mulx    r11, r15, [rsi+112]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+168], rax
    mov     rax, [rsp+184]
    mulx    r11, r15, [rsi+128]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+176], rcx
    mov     rcx, [rsp+192]
    mulx    r11, r15, [rsi+136]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+184], rax
    mov     r11d, 0
    mov     r15, [rsp+200]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+192], rcx
    adcx    r15, r11
    mov     [rsp+200], r15
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     r12, [rsp+168]
    mov     r13, [rsp+176]
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+112]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+184]
    mov     rcx, [rsp+192]
    mulx    r11, r15, [rsi+120]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+128]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+184], rax
    mov     rax, [rsp+200]
    mulx    r11, r15, [rsi+136]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+192], rcx
    mov     r11d, 0
    mov     r15, [rsp+208]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+200], rax
    adcx    r15, r11
    mov     [rsp+208], r15
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     r14, [rsp+184]
    mov     rbx, [rsp+192]
    mov     rdx, [rsi+64]
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+120]
    adcx    r14, rax
    adox    rbx, rcx
    mov     rax, [rsp+200]
    mulx    r11, r15, [rsi+128]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+136]
    mov     rdx, [rsp+208]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+200], rax
    mov     r11d, 0
    mov     r15, [rsp+216]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+208], rdx
    adcx    r15, r11
    mov     [rsp+216], r15
    // --- 行 9: 乘数 a_9 ---
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    mov     rbp, [rsp+200]
    mov     r8, [rsp+208]
    mov     rdx, [rsi+72]
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+128]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+136]
    mov     rdx, [rsp+216]
    adox    rdx, r11
    adcx    r8, r15
    mov     r11d, 0
    mov     r15, [rsp+224]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+216], rdx
    adcx    r15, r11
    mov     [rsp+224], r15
    // --- 行 10: 乘数 a_10 ---
    mov     [rsp+152], r9
    mov     [rsp+160], r10
    mov     r9, [rsp+216]
    mov     r10, [rsp+224]
    mov     rdx, [rsi+80]
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+120]
    mulx    r11, r15, [rsi+128]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    r11, r15, [rsi+136]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    mov     r15, [rsp+232]
    adox    r15, r11
    adcx    r10, r11
    adcx    r15, r11
    mov     [rsp+232], r15
    // --- 行 11: 乘数 a_11 ---
    mov     [rsp+168], r12
    mov     [rsp+176], r13
    mov     r12, [rsp+232]
    mov     r13d, 0
    mov     rdx, [rsi+88]
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+128]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+136]
    adox    r12, r11
    adcx    r10, r15
    mov     r11d, 0
    adox    r13, r11
    adcx    r12, r11
    adcx    r13, r11
    // --- 行 12: 乘数 a_12 ---
    mov     [rsp+184], r14
    mov     [rsp+192], rbx
    mov     r14d, 0
    mov     ebx, 0
    mov     rdx, [rsi+96]
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+120]
    mulx    r11, r15, [rsi+128]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+136]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    adox    r14, r11
    adcx    r13, r11
    adcx    r14, r11
    // --- 行 13: 乘数 a_13 ---
    mov     [rsp+200], rbp
    mov     [rsp+208], r8
    mov     ebp, 0
    mov     r8d, 0
    mov     rdx, [rsi+104]
    mulx    rcx, rax, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+128]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+136]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 14: 乘数 a_14 ---
    mov     [rsp+216], r9
    mov     [rsp+224], r10
    mov     rdx, [rsi+112]
    mulx    rcx, rax, [rsi+120]
    mulx    r11, r15, [rsi+128]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+136]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adox    rbp, r11
    adcx    rbx, r11
    adcx    rbp, r11
    // --- 行 15: 乘数 a_15 ---
    mov     [rsp+232], r12
    mov     [rsp+240], r13
    mov     rdx, [rsi+120]
    mulx    rcx, rax, [rsi+128]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+136]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 16: 乘数 a_16 ---
    mov     [rsp+248], r14
    mov     [rsp+256], rbx
    mov     rdx, [rsi+128]
    mulx    r11, r15, [rsi+136]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adcx    r8, r11
    mov     [rsp+264], rbp
    mov     [rsp+272], r8
    // --- 倍增 ---
    mov     r11, [rsp+272]
    shr     r11, 63
    vmovdqu ymm0, [rsp+248]
    vmovdqu ymm1, [rsp+240]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+248], ymm0
    vmovdqu ymm0, [rsp+216]
    vmovdqu ymm1, [rsp+208]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+216], ymm0
    vmovdqu ymm0, [rsp+184]
    vmovdqu ymm1, [rsp+176]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+184], ymm0
    vmovdqu ymm0, [rsp+152]
    vmovdqu ymm1, [rsp+144]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+152], ymm0
    vmovdqu ymm0, [rsp+120]
    vmovdqu ymm1, [rsp+112]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+120], ymm0
    vmovdqu ymm0, [rsp+88]
    vmovdqu ymm1, [rsp+80]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+88], ymm0
    vmovdqu ymm0, [rsp+56]
    vmovdqu ymm1, [rsp+48]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+56], ymm0
    vmovdqu ymm0, [rsp+24]
    vmovdqu ymm1, [rsp+16]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+24], ymm0
    mov     rax, [rsp+8]
    add     rax, rax
    mov     [rsp+8], rax
    mov     rax, [rsp+16]
    adc     rax, rax
    mov     [rsp+16], rax
    vzeroupper
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, rcx
    adc     r15, [rsp+152]
    mov     [rdi+152], r15
    mov     rdx, [rsi+80]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+160]
    mov     [rdi+160], r15
    mov     r15, rcx
    adc     r15, [rsp+168]
    mov     [rdi+168], r15
    mov     rdx, [rsi+88]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+176]
    mov     [rdi+176], r15
    mov     r15, rcx
    adc     r15, [rsp+184]
    mov     [rdi+184], r15
    mov     rdx, [rsi+96]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+192]
    mov     [rdi+192], r15
    mov     r15, rcx
    adc     r15, [rsp+200]
    mov     [rdi+200], r15
    mov     rdx, [rsi+104]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+208]
    mov     [rdi+208], r15
    mov     r15, rcx
    adc     r15, [rsp+216]
    mov     [rdi+216], r15
    mov     rdx, [rsi+112]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+224]
    mov     [rdi+224], r15
    mov     r15, rcx
    adc     r15, [rsp+232]
    mov     [rdi+232], r15
    mov     rdx, [rsi+120]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+240]
    mov     [rdi+240], r15
    mov     r15, rcx
    adc     r15, [rsp+248]
    mov     [rdi+248], r15
    mov     rdx, [rsi+128]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+256]
    mov     [rdi+256], r15
    mov     r15, rcx
    adc     r15, [rsp+264]
    mov     [rdi+264], r15
    mov     rdx, [rsi+136]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+272]
    mov     [rdi+272], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+280], r15
    add     rsp, 288
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

.globl ASM_GSYM(lmmp_sqr_hard_19_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_19_):

    # 列窗口 WIN[c%8]=[rbx,rbp,r8,r9,r10,r12,r13,r14]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 304               // M 交叉列数组, M[c]=[rsp+8c], c=1..36
    xorps   xmm0, xmm0
    mov     QWORD PTR [rsp+0], 0
    movdqu  [rsp+72], xmm0       // M[9..10] = 0
    movdqu  [rsp+88], xmm0       // M[11..12] = 0
    movdqu  [rsp+104], xmm0       // M[13..14] = 0
    movdqu  [rsp+120], xmm0       // M[15..16] = 0
    movdqu  [rsp+136], xmm0       // M[17..18] = 0
    movdqu  [rsp+152], xmm0       // M[19..20] = 0
    movdqu  [rsp+168], xmm0       // M[21..22] = 0
    movdqu  [rsp+184], xmm0       // M[23..24] = 0
    movdqu  [rsp+200], xmm0       // M[25..26] = 0
    movdqu  [rsp+216], xmm0       // M[27..28] = 0
    movdqu  [rsp+232], xmm0       // M[29..30] = 0
    mov     QWORD PTR [rsp+248], 0
    // --- 行 0: 乘数 a_0 ---
    mov     rdx, [rsi+0]
    mulx    r8, rbp, [rsi+8]
    xor     r11d, r11d
    mulx    r9, rax, [rsi+16]
    mulx    r10, r15, [rsi+24]
    adcx    r8, rax
    adcx    r9, r15
    mulx    r12, rax, [rsi+32]
    mulx    r13, r15, [rsi+40]
    adcx    r10, rax
    adcx    r12, r15
    mulx    r14, rax, [rsi+48]
    mulx    rbx, r15, [rsi+56]
    adcx    r13, rax
    adcx    r14, r15
    mov     rax, [rsp+72]
    mov     rcx, [rsp+80]
    mulx    r11, r15, [rsi+64]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+72]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+72], rax
    mov     rax, [rsp+88]
    mulx    r11, r15, [rsi+80]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+80], rcx
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+128]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+136]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+144]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+144], rcx
    mov     r11d, 0
    adcx    rax, r11
    mov     [rsp+152], rax
    mov     r15, [rsp+160]
    adcx    r15, r11
    mov     [rsp+160], r15
    // --- 行 1: 乘数 a_1 ---
    mov     [rsp+8], rbp
    mov     [rsp+16], r8
    mov     rbp, [rsp+72]
    mov     r8, [rsp+80]
    mov     rdx, [rsi+8]
    mulx    rcx, rax, [rsi+16]
    mulx    r11, r15, [rsi+24]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+88]
    mov     rcx, [rsp+96]
    mulx    r11, r15, [rsi+72]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+80]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+88], rax
    mov     rax, [rsp+104]
    mulx    r11, r15, [rsi+88]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+96], rcx
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+120]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+128]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+136]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+144]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+152], rax
    mov     r11d, 0
    mov     r15, [rsp+168]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+160], rcx
    adcx    r15, r11
    mov     [rsp+168], r15
    // --- 行 2: 乘数 a_2 ---
    mov     [rsp+24], r9
    mov     [rsp+32], r10
    mov     r9, [rsp+88]
    mov     r10, [rsp+96]
    mov     rdx, [rsi+16]
    mulx    rcx, rax, [rsi+24]
    mulx    r11, r15, [rsi+32]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+104]
    mov     rcx, [rsp+112]
    mulx    r11, r15, [rsi+80]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+88]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+104], rax
    mov     rax, [rsp+120]
    mulx    r11, r15, [rsi+96]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+112], rcx
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+128]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+136]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+144]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+160], rcx
    mov     r11d, 0
    mov     r15, [rsp+176]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+168], rax
    adcx    r15, r11
    mov     [rsp+176], r15
    // --- 行 3: 乘数 a_3 ---
    mov     [rsp+40], r12
    mov     [rsp+48], r13
    mov     r12, [rsp+104]
    mov     r13, [rsp+112]
    mov     rdx, [rsi+24]
    mulx    rcx, rax, [rsi+32]
    mulx    r11, r15, [rsi+40]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+120]
    mov     rcx, [rsp+128]
    mulx    r11, r15, [rsi+88]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+96]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+120], rax
    mov     rax, [rsp+136]
    mulx    r11, r15, [rsi+104]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+128], rcx
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+120]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+128]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+136]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+160], rcx
    mov     rcx, [rsp+176]
    mulx    r11, r15, [rsi+144]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+168], rax
    mov     r11d, 0
    mov     r15, [rsp+184]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+176], rcx
    adcx    r15, r11
    mov     [rsp+184], r15
    // --- 行 4: 乘数 a_4 ---
    mov     [rsp+56], r14
    mov     [rsp+64], rbx
    mov     r14, [rsp+120]
    mov     rbx, [rsp+128]
    mov     rdx, [rsi+32]
    mulx    rcx, rax, [rsi+40]
    mulx    r11, r15, [rsi+48]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    mov     rax, [rsp+136]
    mov     rcx, [rsp+144]
    mulx    r11, r15, [rsi+96]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+104]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+136], rax
    mov     rax, [rsp+152]
    mulx    r11, r15, [rsi+112]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+144], rcx
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+128]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+160], rcx
    mov     rcx, [rsp+176]
    mulx    r11, r15, [rsi+136]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+168], rax
    mov     rax, [rsp+184]
    mulx    r11, r15, [rsi+144]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+176], rcx
    mov     r11d, 0
    mov     r15, [rsp+192]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+184], rax
    adcx    r15, r11
    mov     [rsp+192], r15
    // --- 行 5: 乘数 a_5 ---
    mov     [rsp+72], rbp
    mov     [rsp+80], r8
    mov     rbp, [rsp+136]
    mov     r8, [rsp+144]
    mov     rdx, [rsi+40]
    mulx    rcx, rax, [rsi+48]
    mulx    r11, r15, [rsi+56]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+152]
    mov     rcx, [rsp+160]
    mulx    r11, r15, [rsi+104]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+112]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+152], rax
    mov     rax, [rsp+168]
    mulx    r11, r15, [rsi+120]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+160], rcx
    mov     rcx, [rsp+176]
    mulx    r11, r15, [rsi+128]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+168], rax
    mov     rax, [rsp+184]
    mulx    r11, r15, [rsi+136]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+176], rcx
    mov     rcx, [rsp+192]
    mulx    r11, r15, [rsi+144]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+184], rax
    mov     r11d, 0
    mov     r15, [rsp+200]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+192], rcx
    adcx    r15, r11
    mov     [rsp+200], r15
    // --- 行 6: 乘数 a_6 ---
    mov     [rsp+88], r9
    mov     [rsp+96], r10
    mov     r9, [rsp+152]
    mov     r10, [rsp+160]
    mov     rdx, [rsi+48]
    mulx    rcx, rax, [rsi+56]
    mulx    r11, r15, [rsi+64]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    mov     rax, [rsp+168]
    mov     rcx, [rsp+176]
    mulx    r11, r15, [rsi+112]
    adcx    r10, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+120]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+168], rax
    mov     rax, [rsp+184]
    mulx    r11, r15, [rsi+128]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+176], rcx
    mov     rcx, [rsp+192]
    mulx    r11, r15, [rsi+136]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+184], rax
    mov     rax, [rsp+200]
    mulx    r11, r15, [rsi+144]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+192], rcx
    mov     r11d, 0
    mov     r15, [rsp+208]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+200], rax
    adcx    r15, r11
    mov     [rsp+208], r15
    // --- 行 7: 乘数 a_7 ---
    mov     [rsp+104], r12
    mov     [rsp+112], r13
    mov     r12, [rsp+168]
    mov     r13, [rsp+176]
    mov     rdx, [rsi+56]
    mulx    rcx, rax, [rsi+64]
    mulx    r11, r15, [rsi+72]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+112]
    adcx    r12, rax
    adox    r13, rcx
    mov     rax, [rsp+184]
    mov     rcx, [rsp+192]
    mulx    r11, r15, [rsi+120]
    adcx    r13, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+128]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+184], rax
    mov     rax, [rsp+200]
    mulx    r11, r15, [rsi+136]
    adcx    rcx, r15
    adox    rax, r11
    mov     [rsp+192], rcx
    mov     rcx, [rsp+208]
    mulx    r11, r15, [rsi+144]
    adox    rcx, r11
    adcx    rax, r15
    mov     [rsp+200], rax
    mov     r11d, 0
    mov     r15, [rsp+216]
    adox    r15, r11
    adcx    rcx, r11
    mov     [rsp+208], rcx
    adcx    r15, r11
    mov     [rsp+216], r15
    // --- 行 8: 乘数 a_8 ---
    mov     [rsp+120], r14
    mov     [rsp+128], rbx
    mov     r14, [rsp+184]
    mov     rbx, [rsp+192]
    mov     rdx, [rsi+64]
    mulx    rcx, rax, [rsi+72]
    mulx    r11, r15, [rsi+80]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+120]
    adcx    r14, rax
    adox    rbx, rcx
    mov     rax, [rsp+200]
    mov     rcx, [rsp+208]
    mulx    r11, r15, [rsi+128]
    adcx    rbx, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+136]
    adcx    rax, r15
    adox    rcx, r11
    mov     [rsp+200], rax
    mov     rax, [rsp+216]
    mulx    r11, r15, [rsi+144]
    adox    rax, r11
    adcx    rcx, r15
    mov     [rsp+208], rcx
    mov     r11d, 0
    mov     r15, [rsp+224]
    adox    r15, r11
    adcx    rax, r11
    mov     [rsp+216], rax
    adcx    r15, r11
    mov     [rsp+224], r15
    // --- 行 9: 乘数 a_9 ---
    mov     [rsp+136], rbp
    mov     [rsp+144], r8
    mov     rbp, [rsp+200]
    mov     r8, [rsp+208]
    mov     rdx, [rsi+72]
    mulx    rcx, rax, [rsi+80]
    mulx    r11, r15, [rsi+88]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+128]
    adcx    rbp, rax
    adox    r8, rcx
    mov     rax, [rsp+216]
    mulx    r11, r15, [rsi+136]
    adcx    r8, r15
    adox    rax, r11
    mulx    r11, r15, [rsi+144]
    mov     rdx, [rsp+224]
    adox    rdx, r11
    adcx    rax, r15
    mov     [rsp+216], rax
    mov     r11d, 0
    mov     r15, [rsp+232]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+224], rdx
    adcx    r15, r11
    mov     [rsp+232], r15
    // --- 行 10: 乘数 a_10 ---
    mov     [rsp+152], r9
    mov     [rsp+160], r10
    mov     r9, [rsp+216]
    mov     r10, [rsp+224]
    mov     rdx, [rsi+80]
    mulx    rcx, rax, [rsi+88]
    mulx    r11, r15, [rsi+96]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+120]
    mulx    r11, r15, [rsi+128]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+136]
    adcx    r9, rax
    adox    r10, rcx
    mulx    r11, r15, [rsi+144]
    mov     rdx, [rsp+232]
    adox    rdx, r11
    adcx    r10, r15
    mov     r11d, 0
    mov     r15, [rsp+240]
    adox    r15, r11
    adcx    rdx, r11
    mov     [rsp+232], rdx
    adcx    r15, r11
    mov     [rsp+240], r15
    // --- 行 11: 乘数 a_11 ---
    mov     [rsp+168], r12
    mov     [rsp+176], r13
    mov     r12, [rsp+232]
    mov     r13, [rsp+240]
    mov     rdx, [rsi+88]
    mulx    rcx, rax, [rsi+96]
    mulx    r11, r15, [rsi+104]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    rcx, rax, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+128]
    mulx    r11, r15, [rsi+136]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    r11, r15, [rsi+144]
    adox    r13, r11
    adcx    r12, r15
    mov     r11d, 0
    mov     r15, [rsp+248]
    adox    r15, r11
    adcx    r13, r11
    adcx    r15, r11
    mov     [rsp+248], r15
    // --- 行 12: 乘数 a_12 ---
    mov     [rsp+184], r14
    mov     [rsp+192], rbx
    mov     r14, [rsp+248]
    mov     ebx, 0
    mov     rdx, [rsi+96]
    mulx    rcx, rax, [rsi+104]
    mulx    r11, r15, [rsi+112]
    adcx    rbp, rax
    adox    r8, rcx
    adcx    r8, r15
    adox    r9, r11
    mulx    rcx, rax, [rsi+120]
    mulx    r11, r15, [rsi+128]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+136]
    adcx    r12, rax
    adox    r13, rcx
    mulx    r11, r15, [rsi+144]
    adox    r14, r11
    adcx    r13, r15
    mov     r11d, 0
    adox    rbx, r11
    adcx    r14, r11
    adcx    rbx, r11
    // --- 行 13: 乘数 a_13 ---
    mov     [rsp+200], rbp
    mov     [rsp+208], r8
    mov     ebp, 0
    mov     r8d, 0
    mov     rdx, [rsi+104]
    mulx    rcx, rax, [rsi+112]
    mulx    r11, r15, [rsi+120]
    adcx    r9, rax
    adox    r10, rcx
    adcx    r10, r15
    adox    r12, r11
    mulx    rcx, rax, [rsi+128]
    mulx    r11, r15, [rsi+136]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    r11, r15, [rsi+144]
    adox    rbx, r11
    adcx    r14, r15
    mov     r11d, 0
    adox    rbp, r11
    adcx    rbx, r11
    adcx    rbp, r11
    // --- 行 14: 乘数 a_14 ---
    mov     [rsp+216], r9
    mov     [rsp+224], r10
    mov     r9d, 0
    mov     r10d, 0
    mov     rdx, [rsi+112]
    mulx    rcx, rax, [rsi+120]
    mulx    r11, r15, [rsi+128]
    adcx    r12, rax
    adox    r13, rcx
    adcx    r13, r15
    adox    r14, r11
    mulx    rcx, rax, [rsi+136]
    adcx    r14, rax
    adox    rbx, rcx
    mulx    r11, r15, [rsi+144]
    adox    rbp, r11
    adcx    rbx, r15
    mov     r11d, 0
    adox    r8, r11
    adcx    rbp, r11
    adcx    r8, r11
    // --- 行 15: 乘数 a_15 ---
    mov     [rsp+232], r12
    mov     [rsp+240], r13
    mov     rdx, [rsi+120]
    mulx    rcx, rax, [rsi+128]
    mulx    r11, r15, [rsi+136]
    adcx    r14, rax
    adox    rbx, rcx
    adcx    rbx, r15
    adox    rbp, r11
    mulx    r11, r15, [rsi+144]
    adox    r8, r11
    adcx    rbp, r15
    mov     r11d, 0
    adox    r9, r11
    adcx    r8, r11
    adcx    r9, r11
    // --- 行 16: 乘数 a_16 ---
    mov     [rsp+248], r14
    mov     [rsp+256], rbx
    mov     rdx, [rsi+128]
    mulx    rcx, rax, [rsi+136]
    adcx    rbp, rax
    adox    r8, rcx
    mulx    r11, r15, [rsi+144]
    adox    r9, r11
    adcx    r8, r15
    mov     r11d, 0
    adox    r10, r11
    adcx    r9, r11
    adcx    r10, r11
    // --- 行 17: 乘数 a_17 ---
    mov     [rsp+264], rbp
    mov     [rsp+272], r8
    mov     rdx, [rsi+136]
    mulx    r11, r15, [rsi+144]
    adox    r10, r11
    adcx    r9, r15
    mov     r11d, 0
    adcx    r10, r11
    mov     [rsp+280], r9
    mov     [rsp+288], r10
    // --- 倍增 ---
    mov     r11, [rsp+288]
    shr     r11, 63
    vmovdqu ymm0, [rsp+264]
    vmovdqu ymm1, [rsp+256]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+264], ymm0
    vmovdqu ymm0, [rsp+232]
    vmovdqu ymm1, [rsp+224]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+232], ymm0
    vmovdqu ymm0, [rsp+200]
    vmovdqu ymm1, [rsp+192]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+200], ymm0
    vmovdqu ymm0, [rsp+168]
    vmovdqu ymm1, [rsp+160]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+168], ymm0
    vmovdqu ymm0, [rsp+136]
    vmovdqu ymm1, [rsp+128]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+136], ymm0
    vmovdqu ymm0, [rsp+104]
    vmovdqu ymm1, [rsp+96]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+104], ymm0
    vmovdqu ymm0, [rsp+72]
    vmovdqu ymm1, [rsp+64]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+72], ymm0
    vmovdqu ymm0, [rsp+40]
    vmovdqu ymm1, [rsp+32]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+40], ymm0
    vmovdqu ymm0, [rsp+8]
    vmovdqu ymm1, [rsp+0]
    vpsllq  ymm0, ymm0, 1
    vpsrlq  ymm1, ymm1, 63
    vpor    ymm0, ymm0, ymm1
    vmovdqu [rsp+8], ymm0
    vzeroupper
    // --- 对角折叠 ---
    mov     rdx, [rsi+0]
    mulx    rcx, rax, rdx
    mov     [rdi+0], rax
    mov     r15, rcx
    add     r15, [rsp+8]
    mov     [rdi+8], r15
    mov     rdx, [rsi+8]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+16]
    mov     [rdi+16], r15
    mov     r15, rcx
    adc     r15, [rsp+24]
    mov     [rdi+24], r15
    mov     rdx, [rsi+16]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+32]
    mov     [rdi+32], r15
    mov     r15, rcx
    adc     r15, [rsp+40]
    mov     [rdi+40], r15
    mov     rdx, [rsi+24]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+48]
    mov     [rdi+48], r15
    mov     r15, rcx
    adc     r15, [rsp+56]
    mov     [rdi+56], r15
    mov     rdx, [rsi+32]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+64]
    mov     [rdi+64], r15
    mov     r15, rcx
    adc     r15, [rsp+72]
    mov     [rdi+72], r15
    mov     rdx, [rsi+40]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+80]
    mov     [rdi+80], r15
    mov     r15, rcx
    adc     r15, [rsp+88]
    mov     [rdi+88], r15
    mov     rdx, [rsi+48]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+96]
    mov     [rdi+96], r15
    mov     r15, rcx
    adc     r15, [rsp+104]
    mov     [rdi+104], r15
    mov     rdx, [rsi+56]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+112]
    mov     [rdi+112], r15
    mov     r15, rcx
    adc     r15, [rsp+120]
    mov     [rdi+120], r15
    mov     rdx, [rsi+64]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+128]
    mov     [rdi+128], r15
    mov     r15, rcx
    adc     r15, [rsp+136]
    mov     [rdi+136], r15
    mov     rdx, [rsi+72]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+144]
    mov     [rdi+144], r15
    mov     r15, rcx
    adc     r15, [rsp+152]
    mov     [rdi+152], r15
    mov     rdx, [rsi+80]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+160]
    mov     [rdi+160], r15
    mov     r15, rcx
    adc     r15, [rsp+168]
    mov     [rdi+168], r15
    mov     rdx, [rsi+88]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+176]
    mov     [rdi+176], r15
    mov     r15, rcx
    adc     r15, [rsp+184]
    mov     [rdi+184], r15
    mov     rdx, [rsi+96]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+192]
    mov     [rdi+192], r15
    mov     r15, rcx
    adc     r15, [rsp+200]
    mov     [rdi+200], r15
    mov     rdx, [rsi+104]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+208]
    mov     [rdi+208], r15
    mov     r15, rcx
    adc     r15, [rsp+216]
    mov     [rdi+216], r15
    mov     rdx, [rsi+112]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+224]
    mov     [rdi+224], r15
    mov     r15, rcx
    adc     r15, [rsp+232]
    mov     [rdi+232], r15
    mov     rdx, [rsi+120]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+240]
    mov     [rdi+240], r15
    mov     r15, rcx
    adc     r15, [rsp+248]
    mov     [rdi+248], r15
    mov     rdx, [rsi+128]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+256]
    mov     [rdi+256], r15
    mov     r15, rcx
    adc     r15, [rsp+264]
    mov     [rdi+264], r15
    mov     rdx, [rsi+136]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+272]
    mov     [rdi+272], r15
    mov     r15, rcx
    adc     r15, [rsp+280]
    mov     [rdi+280], r15
    mov     rdx, [rsi+144]
    mulx    rcx, rax, rdx
    mov     r15, rax
    adc     r15, [rsp+288]
    mov     [rdi+288], r15
    mov     r15, r11
    adc     r15, rcx
    mov     [rdi+296], r15
    add     rsp, 304
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
