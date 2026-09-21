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
//   N >= 4 : 交叉乘逐行累加 -> 整体倍增 -> 对角平方折叠
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

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+56], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+32], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     rax, [rsi+24]          // a3*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+40], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+48], r10
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+56], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_5_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_5_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+72], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+40], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+32]          // a4*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+48], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+56], r10
    # --- 单行: a_3 ---
    mov     r8, [rsi+24]            // u = a_3
    xor     r14d, r14d             // c = 0
    mov     r13, [rdi+56]
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     r14, rdx               // c = hi
    adc     r14, 0
    add     r13, rax               // D + lo'
    adc     r14, 0
    mov     [rdi+56], r13
    mov     [rdi+64], r14          // 顶列直存
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+72], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_6_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_6_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+88], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+48], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+40]          // a5*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+56], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+64], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     rax, [rsi+40]          // a5*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+72], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+80], r10
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+88], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_7_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_7_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+104], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+56], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+48]          // a6*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+64], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+72], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+48]          // a6*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+80], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+88], r10
    # --- 单行: a_5 ---
    mov     r8, [rsi+40]            // u = a_5
    xor     r14d, r14d             // c = 0
    mov     r13, [rdi+88]
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     r14, rdx               // c = hi
    adc     r14, 0
    add     r13, rax               // D + lo'
    adc     r14, 0
    mov     [rdi+88], r13
    mov     [rdi+96], r14          // 顶列直存
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+104], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_8_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_8_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+120], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+64], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+56]          // a7*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+72], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+80], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+56]          // a7*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+88], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+96], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     rax, [rsi+56]          // a7*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+104], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+112], r10
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+120], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_9_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_9_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+136], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+72], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+64]          // a8*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+80], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+88], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+64]          // a8*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+96], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+104], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+64]          // a8*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+112], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+120], r10
    # --- 单行: a_7 ---
    mov     r8, [rsi+56]            // u = a_7
    xor     r14d, r14d             // c = 0
    mov     r13, [rdi+120]
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     r14, rdx               // c = hi
    adc     r14, 0
    add     r13, rax               // D + lo'
    adc     r14, 0
    mov     [rdi+120], r13
    mov     [rdi+128], r14          // 顶列直存
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+136], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_10_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_10_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+152], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+80], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+72]          // a9*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+88], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+96], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+72]          // a9*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+104], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+112], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+72]          // a9*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+120], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+128], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     rax, [rsi+72]          // a9*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+136], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+144], r10
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+152], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_11_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_11_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+168], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+80], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+88], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+80]          // a10*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+96], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+104], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+80]          // a10*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+112], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+120], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+80]          // a10*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+128], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+136], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+80]          // a10*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+144], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+152], r10
    # --- 单行: a_9 ---
    mov     r8, [rsi+72]            // u = a_9
    xor     r14d, r14d             // c = 0
    mov     r13, [rdi+152]
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     r14, rdx               // c = hi
    adc     r14, 0
    add     r13, rax               // D + lo'
    adc     r14, 0
    mov     [rdi+152], r13
    mov     [rdi+160], r14          // 顶列直存
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    mov     rax, [rdi+160]
    adc     rax, rax
    mov     [rdi+160], rax
    mov     rax, [rdi+168]
    adc     rax, rax
    mov     [rdi+168], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8
    mov     [rdi+152], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+80]
    mov     rax, rdx
    mulx    r11, r10, rax          // a10^2
    mov     r9, [rdi+160]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+160], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+168]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+168], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_12_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_12_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+184], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+80], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+88]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+88], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+96], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+88]          // a11*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+104], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+112], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+88]          // a11*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+120], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+128], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+88]          // a11*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+136], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+144], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+88]          // a11*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+152], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+160], r10
    # --- 行对: a_9 与 a_10 ---
    mov     r8, [rsi+72]            // u = a_9
    mov     r9, [rsi+80]            // v = a_10
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+80]          // a10
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+152], r13
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+160], r13
    mov     rax, [rsi+88]          // a11*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+168], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+176], r10
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    mov     rax, [rdi+160]
    adc     rax, rax
    mov     [rdi+160], rax
    mov     rax, [rdi+168]
    adc     rax, rax
    mov     [rdi+168], rax
    mov     rax, [rdi+176]
    adc     rax, rax
    mov     [rdi+176], rax
    mov     rax, [rdi+184]
    adc     rax, rax
    mov     [rdi+184], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8
    mov     [rdi+152], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+80]
    mov     rax, rdx
    mulx    r11, r10, rax          // a10^2
    mov     r9, [rdi+160]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+160], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+168]
    adcx    r9, r8
    mov     [rdi+168], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+88]
    mov     rax, rdx
    mulx    r11, r10, rax          // a11^2
    mov     r9, [rdi+176]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+176], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+184]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+184], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_13_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_13_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+200], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+80], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+88]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+88], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+96]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+96], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+104], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+96]          // a12*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+112], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+120], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+96]          // a12*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+128], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+136], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+96]          // a12*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+144], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+152], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+96]          // a12*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+160], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+168], r10
    # --- 行对: a_9 与 a_10 ---
    mov     r8, [rsi+72]            // u = a_9
    mov     r9, [rsi+80]            // v = a_10
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+80]          // a10
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+152], r13
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+160], r13
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+96]          // a12*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+176], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+184], r10
    # --- 单行: a_11 ---
    mov     r8, [rsi+88]            // u = a_11
    xor     r14d, r14d             // c = 0
    mov     r13, [rdi+184]
    mov     rax, [rsi+96]
    mul     r8
    add     rax, r14               // lo + c
    mov     r14, rdx               // c = hi
    adc     r14, 0
    add     r13, rax               // D + lo'
    adc     r14, 0
    mov     [rdi+184], r13
    mov     [rdi+192], r14          // 顶列直存
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    mov     rax, [rdi+160]
    adc     rax, rax
    mov     [rdi+160], rax
    mov     rax, [rdi+168]
    adc     rax, rax
    mov     [rdi+168], rax
    mov     rax, [rdi+176]
    adc     rax, rax
    mov     [rdi+176], rax
    mov     rax, [rdi+184]
    adc     rax, rax
    mov     [rdi+184], rax
    mov     rax, [rdi+192]
    adc     rax, rax
    mov     [rdi+192], rax
    mov     rax, [rdi+200]
    adc     rax, rax
    mov     [rdi+200], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8
    mov     [rdi+152], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+80]
    mov     rax, rdx
    mulx    r11, r10, rax          // a10^2
    mov     r9, [rdi+160]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+160], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+168]
    adcx    r9, r8
    mov     [rdi+168], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+88]
    mov     rax, rdx
    mulx    r11, r10, rax          // a11^2
    mov     r9, [rdi+176]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+176], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+184]
    adcx    r9, r8
    mov     [rdi+184], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+96]
    mov     rax, rdx
    mulx    r11, r10, rax          // a12^2
    mov     r9, [rdi+192]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+192], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+200]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+200], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_14_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_14_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+216], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+80], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+88]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+88], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+96]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+96], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+104]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+104], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+112], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+104]          // a13*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+120], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+128], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+104]          // a13*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+136], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+144], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+104]          // a13*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+152], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+160], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+104]          // a13*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+168], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+176], r10
    # --- 行对: a_9 与 a_10 ---
    mov     r8, [rsi+72]            // u = a_9
    mov     r9, [rsi+80]            // v = a_10
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+80]          // a10
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+152], r13
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+160], r13
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+104]          // a13*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+184], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+192], r10
    # --- 行对: a_11 与 a_12 ---
    mov     r8, [rsi+88]            // u = a_11
    mov     r9, [rsi+96]            // v = a_12
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+96]          // a12
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+184], r13
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+192], r13
    mov     rax, [rsi+104]          // a13*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+200], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+208], r10
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    mov     rax, [rdi+160]
    adc     rax, rax
    mov     [rdi+160], rax
    mov     rax, [rdi+168]
    adc     rax, rax
    mov     [rdi+168], rax
    mov     rax, [rdi+176]
    adc     rax, rax
    mov     [rdi+176], rax
    mov     rax, [rdi+184]
    adc     rax, rax
    mov     [rdi+184], rax
    mov     rax, [rdi+192]
    adc     rax, rax
    mov     [rdi+192], rax
    mov     rax, [rdi+200]
    adc     rax, rax
    mov     [rdi+200], rax
    mov     rax, [rdi+208]
    adc     rax, rax
    mov     [rdi+208], rax
    mov     rax, [rdi+216]
    adc     rax, rax
    mov     [rdi+216], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8
    mov     [rdi+152], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+80]
    mov     rax, rdx
    mulx    r11, r10, rax          // a10^2
    mov     r9, [rdi+160]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+160], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+168]
    adcx    r9, r8
    mov     [rdi+168], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+88]
    mov     rax, rdx
    mulx    r11, r10, rax          // a11^2
    mov     r9, [rdi+176]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+176], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+184]
    adcx    r9, r8
    mov     [rdi+184], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+96]
    mov     rax, rdx
    mulx    r11, r10, rax          // a12^2
    mov     r9, [rdi+192]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+192], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+200]
    adcx    r9, r8
    mov     [rdi+200], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+104]
    mov     rax, rdx
    mulx    r11, r10, rax          // a13^2
    mov     r9, [rdi+208]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+208], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+216]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+216], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_15_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_15_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+232], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+80], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+88]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+88], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+96]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+96], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+104]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+104], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+112]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+112], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+120], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+112]          // a14*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+128], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+136], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+112]          // a14*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+144], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+152], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+112]          // a14*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+160], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+168], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+112]          // a14*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+176], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+184], r10
    # --- 行对: a_9 与 a_10 ---
    mov     r8, [rsi+72]            // u = a_9
    mov     r9, [rsi+80]            // v = a_10
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+80]          // a10
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+152], r13
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+160], r13
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+184], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+112]          // a14*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+192], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+200], r10
    # --- 行对: a_11 与 a_12 ---
    mov     r8, [rsi+88]            // u = a_11
    mov     r9, [rsi+96]            // v = a_12
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+96]          // a12
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+184], r13
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+192], r13
    mov     r13, [rdi+200]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+200], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+112]          // a14*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+208], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+216], r10
    # --- 单行: a_13 ---
    mov     r8, [rsi+104]            // u = a_13
    xor     r14d, r14d             // c = 0
    mov     r13, [rdi+216]
    mov     rax, [rsi+112]
    mul     r8
    add     rax, r14               // lo + c
    mov     r14, rdx               // c = hi
    adc     r14, 0
    add     r13, rax               // D + lo'
    adc     r14, 0
    mov     [rdi+216], r13
    mov     [rdi+224], r14          // 顶列直存
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    mov     rax, [rdi+160]
    adc     rax, rax
    mov     [rdi+160], rax
    mov     rax, [rdi+168]
    adc     rax, rax
    mov     [rdi+168], rax
    mov     rax, [rdi+176]
    adc     rax, rax
    mov     [rdi+176], rax
    mov     rax, [rdi+184]
    adc     rax, rax
    mov     [rdi+184], rax
    mov     rax, [rdi+192]
    adc     rax, rax
    mov     [rdi+192], rax
    mov     rax, [rdi+200]
    adc     rax, rax
    mov     [rdi+200], rax
    mov     rax, [rdi+208]
    adc     rax, rax
    mov     [rdi+208], rax
    mov     rax, [rdi+216]
    adc     rax, rax
    mov     [rdi+216], rax
    mov     rax, [rdi+224]
    adc     rax, rax
    mov     [rdi+224], rax
    mov     rax, [rdi+232]
    adc     rax, rax
    mov     [rdi+232], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8
    mov     [rdi+152], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+80]
    mov     rax, rdx
    mulx    r11, r10, rax          // a10^2
    mov     r9, [rdi+160]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+160], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+168]
    adcx    r9, r8
    mov     [rdi+168], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+88]
    mov     rax, rdx
    mulx    r11, r10, rax          // a11^2
    mov     r9, [rdi+176]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+176], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+184]
    adcx    r9, r8
    mov     [rdi+184], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+96]
    mov     rax, rdx
    mulx    r11, r10, rax          // a12^2
    mov     r9, [rdi+192]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+192], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+200]
    adcx    r9, r8
    mov     [rdi+200], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+104]
    mov     rax, rdx
    mulx    r11, r10, rax          // a13^2
    mov     r9, [rdi+208]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+208], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+216]
    adcx    r9, r8
    mov     [rdi+216], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+112]
    mov     rax, rdx
    mulx    r11, r10, rax          // a14^2
    mov     r9, [rdi+224]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+224], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+232]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+232], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_16_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_16_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+248], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+80], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+88]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+88], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+96]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+96], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+104]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+104], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+112]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+112], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+120]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+120], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+128], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+120]          // a15*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+136], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+144], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+120]          // a15*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+152], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+160], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+120]          // a15*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+168], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+176], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+120]          // a15*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+184], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+192], r10
    # --- 行对: a_9 与 a_10 ---
    mov     r8, [rsi+72]            // u = a_9
    mov     r9, [rsi+80]            // v = a_10
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+80]          // a10
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+152], r13
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+160], r13
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+184], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+192], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+120]          // a15*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+200], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+208], r10
    # --- 行对: a_11 与 a_12 ---
    mov     r8, [rsi+88]            // u = a_11
    mov     r9, [rsi+96]            // v = a_12
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+96]          // a12
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+184], r13
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+192], r13
    mov     r13, [rdi+200]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+200], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+208]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+208], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+120]          // a15*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+216], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+224], r10
    # --- 行对: a_13 与 a_14 ---
    mov     r8, [rsi+104]            // u = a_13
    mov     r9, [rsi+112]            // v = a_14
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+216]          // D 预载
    mov     rax, [rsi+112]          // a14
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+216], r13
    mov     r13, [rdi+224]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+224], r13
    mov     rax, [rsi+120]          // a15*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+232], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+240], r10
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    mov     rax, [rdi+160]
    adc     rax, rax
    mov     [rdi+160], rax
    mov     rax, [rdi+168]
    adc     rax, rax
    mov     [rdi+168], rax
    mov     rax, [rdi+176]
    adc     rax, rax
    mov     [rdi+176], rax
    mov     rax, [rdi+184]
    adc     rax, rax
    mov     [rdi+184], rax
    mov     rax, [rdi+192]
    adc     rax, rax
    mov     [rdi+192], rax
    mov     rax, [rdi+200]
    adc     rax, rax
    mov     [rdi+200], rax
    mov     rax, [rdi+208]
    adc     rax, rax
    mov     [rdi+208], rax
    mov     rax, [rdi+216]
    adc     rax, rax
    mov     [rdi+216], rax
    mov     rax, [rdi+224]
    adc     rax, rax
    mov     [rdi+224], rax
    mov     rax, [rdi+232]
    adc     rax, rax
    mov     [rdi+232], rax
    mov     rax, [rdi+240]
    adc     rax, rax
    mov     [rdi+240], rax
    mov     rax, [rdi+248]
    adc     rax, rax
    mov     [rdi+248], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8
    mov     [rdi+152], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+80]
    mov     rax, rdx
    mulx    r11, r10, rax          // a10^2
    mov     r9, [rdi+160]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+160], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+168]
    adcx    r9, r8
    mov     [rdi+168], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+88]
    mov     rax, rdx
    mulx    r11, r10, rax          // a11^2
    mov     r9, [rdi+176]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+176], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+184]
    adcx    r9, r8
    mov     [rdi+184], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+96]
    mov     rax, rdx
    mulx    r11, r10, rax          // a12^2
    mov     r9, [rdi+192]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+192], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+200]
    adcx    r9, r8
    mov     [rdi+200], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+104]
    mov     rax, rdx
    mulx    r11, r10, rax          // a13^2
    mov     r9, [rdi+208]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+208], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+216]
    adcx    r9, r8
    mov     [rdi+216], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+112]
    mov     rax, rdx
    mulx    r11, r10, rax          // a14^2
    mov     r9, [rdi+224]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+224], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+232]
    adcx    r9, r8
    mov     [rdi+232], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+120]
    mov     rax, rdx
    mulx    r11, r10, rax          // a15^2
    mov     r9, [rdi+240]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+240], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+248]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+248], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_17_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_17_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+264], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+80], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+88]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+88], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+96]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+96], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+104]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+104], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+112]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+112], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+120]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+120], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+128]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+128], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+136], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+128]          // a16*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+144], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+152], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+128]          // a16*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+160], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+168], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+128]          // a16*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+176], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+184], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+184], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+128]          // a16*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+192], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+200], r10
    # --- 行对: a_9 与 a_10 ---
    mov     r8, [rsi+72]            // u = a_9
    mov     r9, [rsi+80]            // v = a_10
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+80]          // a10
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+152], r13
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+160], r13
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+184], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+192], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+200]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+200], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+128]          // a16*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+208], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+216], r10
    # --- 行对: a_11 与 a_12 ---
    mov     r8, [rsi+88]            // u = a_11
    mov     r9, [rsi+96]            // v = a_12
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+96]          // a12
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+184], r13
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+192], r13
    mov     r13, [rdi+200]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+200], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+208]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+208], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+216]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+216], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+128]          // a16*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+224], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+232], r10
    # --- 行对: a_13 与 a_14 ---
    mov     r8, [rsi+104]            // u = a_13
    mov     r9, [rsi+112]            // v = a_14
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+216]          // D 预载
    mov     rax, [rsi+112]          // a14
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+216], r13
    mov     r13, [rdi+224]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+224], r13
    mov     r13, [rdi+232]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+232], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+128]          // a16*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+240], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+248], r10
    # --- 单行: a_15 ---
    mov     r8, [rsi+120]            // u = a_15
    xor     r14d, r14d             // c = 0
    mov     r13, [rdi+248]
    mov     rax, [rsi+128]
    mul     r8
    add     rax, r14               // lo + c
    mov     r14, rdx               // c = hi
    adc     r14, 0
    add     r13, rax               // D + lo'
    adc     r14, 0
    mov     [rdi+248], r13
    mov     [rdi+256], r14          // 顶列直存
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    mov     rax, [rdi+160]
    adc     rax, rax
    mov     [rdi+160], rax
    mov     rax, [rdi+168]
    adc     rax, rax
    mov     [rdi+168], rax
    mov     rax, [rdi+176]
    adc     rax, rax
    mov     [rdi+176], rax
    mov     rax, [rdi+184]
    adc     rax, rax
    mov     [rdi+184], rax
    mov     rax, [rdi+192]
    adc     rax, rax
    mov     [rdi+192], rax
    mov     rax, [rdi+200]
    adc     rax, rax
    mov     [rdi+200], rax
    mov     rax, [rdi+208]
    adc     rax, rax
    mov     [rdi+208], rax
    mov     rax, [rdi+216]
    adc     rax, rax
    mov     [rdi+216], rax
    mov     rax, [rdi+224]
    adc     rax, rax
    mov     [rdi+224], rax
    mov     rax, [rdi+232]
    adc     rax, rax
    mov     [rdi+232], rax
    mov     rax, [rdi+240]
    adc     rax, rax
    mov     [rdi+240], rax
    mov     rax, [rdi+248]
    adc     rax, rax
    mov     [rdi+248], rax
    mov     rax, [rdi+256]
    adc     rax, rax
    mov     [rdi+256], rax
    mov     rax, [rdi+264]
    adc     rax, rax
    mov     [rdi+264], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8
    mov     [rdi+152], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+80]
    mov     rax, rdx
    mulx    r11, r10, rax          // a10^2
    mov     r9, [rdi+160]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+160], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+168]
    adcx    r9, r8
    mov     [rdi+168], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+88]
    mov     rax, rdx
    mulx    r11, r10, rax          // a11^2
    mov     r9, [rdi+176]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+176], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+184]
    adcx    r9, r8
    mov     [rdi+184], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+96]
    mov     rax, rdx
    mulx    r11, r10, rax          // a12^2
    mov     r9, [rdi+192]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+192], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+200]
    adcx    r9, r8
    mov     [rdi+200], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+104]
    mov     rax, rdx
    mulx    r11, r10, rax          // a13^2
    mov     r9, [rdi+208]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+208], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+216]
    adcx    r9, r8
    mov     [rdi+216], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+112]
    mov     rax, rdx
    mulx    r11, r10, rax          // a14^2
    mov     r9, [rdi+224]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+224], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+232]
    adcx    r9, r8
    mov     [rdi+232], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+120]
    mov     rax, rdx
    mulx    r11, r10, rax          // a15^2
    mov     r9, [rdi+240]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+240], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+248]
    adcx    r9, r8
    mov     [rdi+248], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+128]
    mov     rax, rdx
    mulx    r11, r10, rax          // a16^2
    mov     r9, [rdi+256]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+256], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+264]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+264], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_18_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_18_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+280], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+80], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+88]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+88], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+96]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+96], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+104]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+104], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+112]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+112], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+120]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+120], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+128]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+128], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+136]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+136], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+144], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+136]          // a17*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+152], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+160], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+136]          // a17*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+168], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+176], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+136]          // a17*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+184], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+192], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+184], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+192], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+136]          // a17*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+200], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+208], r10
    # --- 行对: a_9 与 a_10 ---
    mov     r8, [rsi+72]            // u = a_9
    mov     r9, [rsi+80]            // v = a_10
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+80]          // a10
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+152], r13
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+160], r13
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+184], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+192], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+200]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+200], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+208]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+208], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+136]          // a17*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+216], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+224], r10
    # --- 行对: a_11 与 a_12 ---
    mov     r8, [rsi+88]            // u = a_11
    mov     r9, [rsi+96]            // v = a_12
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+96]          // a12
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+184], r13
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+192], r13
    mov     r13, [rdi+200]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+200], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+208]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+208], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+216]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+216], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+224]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+224], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+136]          // a17*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+232], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+240], r10
    # --- 行对: a_13 与 a_14 ---
    mov     r8, [rsi+104]            // u = a_13
    mov     r9, [rsi+112]            // v = a_14
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+216]          // D 预载
    mov     rax, [rsi+112]          // a14
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+216], r13
    mov     r13, [rdi+224]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+224], r13
    mov     r13, [rdi+232]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+232], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+240]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+240], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+136]          // a17*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+248], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+256], r10
    # --- 行对: a_15 与 a_16 ---
    mov     r8, [rsi+120]            // u = a_15
    mov     r9, [rsi+128]            // v = a_16
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+248]          // D 预载
    mov     rax, [rsi+128]          // a16
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+248], r13
    mov     r13, [rdi+256]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+256], r13
    mov     rax, [rsi+136]          // a17*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+264], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+272], r10
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    mov     rax, [rdi+160]
    adc     rax, rax
    mov     [rdi+160], rax
    mov     rax, [rdi+168]
    adc     rax, rax
    mov     [rdi+168], rax
    mov     rax, [rdi+176]
    adc     rax, rax
    mov     [rdi+176], rax
    mov     rax, [rdi+184]
    adc     rax, rax
    mov     [rdi+184], rax
    mov     rax, [rdi+192]
    adc     rax, rax
    mov     [rdi+192], rax
    mov     rax, [rdi+200]
    adc     rax, rax
    mov     [rdi+200], rax
    mov     rax, [rdi+208]
    adc     rax, rax
    mov     [rdi+208], rax
    mov     rax, [rdi+216]
    adc     rax, rax
    mov     [rdi+216], rax
    mov     rax, [rdi+224]
    adc     rax, rax
    mov     [rdi+224], rax
    mov     rax, [rdi+232]
    adc     rax, rax
    mov     [rdi+232], rax
    mov     rax, [rdi+240]
    adc     rax, rax
    mov     [rdi+240], rax
    mov     rax, [rdi+248]
    adc     rax, rax
    mov     [rdi+248], rax
    mov     rax, [rdi+256]
    adc     rax, rax
    mov     [rdi+256], rax
    mov     rax, [rdi+264]
    adc     rax, rax
    mov     [rdi+264], rax
    mov     rax, [rdi+272]
    adc     rax, rax
    mov     [rdi+272], rax
    mov     rax, [rdi+280]
    adc     rax, rax
    mov     [rdi+280], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8
    mov     [rdi+152], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+80]
    mov     rax, rdx
    mulx    r11, r10, rax          // a10^2
    mov     r9, [rdi+160]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+160], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+168]
    adcx    r9, r8
    mov     [rdi+168], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+88]
    mov     rax, rdx
    mulx    r11, r10, rax          // a11^2
    mov     r9, [rdi+176]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+176], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+184]
    adcx    r9, r8
    mov     [rdi+184], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+96]
    mov     rax, rdx
    mulx    r11, r10, rax          // a12^2
    mov     r9, [rdi+192]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+192], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+200]
    adcx    r9, r8
    mov     [rdi+200], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+104]
    mov     rax, rdx
    mulx    r11, r10, rax          // a13^2
    mov     r9, [rdi+208]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+208], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+216]
    adcx    r9, r8
    mov     [rdi+216], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+112]
    mov     rax, rdx
    mulx    r11, r10, rax          // a14^2
    mov     r9, [rdi+224]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+224], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+232]
    adcx    r9, r8
    mov     [rdi+232], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+120]
    mov     rax, rdx
    mulx    r11, r10, rax          // a15^2
    mov     r9, [rdi+240]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+240], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+248]
    adcx    r9, r8
    mov     [rdi+248], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+128]
    mov     rax, rdx
    mulx    r11, r10, rax          // a16^2
    mov     r9, [rdi+256]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+256], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+264]
    adcx    r9, r8
    mov     [rdi+264], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+136]
    mov     rax, rdx
    mulx    r11, r10, rax          // a17^2
    mov     r9, [rdi+272]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+272], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+280]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+280], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

.globl ASM_GSYM(lmmp_sqr_hard_19_)
.p2align 4

ASM_GSYM(lmmp_sqr_hard_19_):

    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
    mov     rdi, rcx                // dst
    mov     rsi, rdx                // numa
#endif
    push    rbx
    push    r13
    push    r14
    push    r15
    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)
    mov     QWORD PTR [rdi+296], 0    // dst[2n-1] = 0 (交叉不触及)
    mov     r8, [rsi]                // a_0
    xor     r14d, r14d              // c = 0
    mov     rax, [rsi+8]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+8], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+16]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+16], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+24]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+24], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+32]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+32], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+40]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+40], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+48]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+48], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+56]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+56], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+64]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+64], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+72]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+72], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+80]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+80], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+88]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+88], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+96]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+96], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+104]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+104], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+112]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+112], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+120]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+120], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+128]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+128], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+136]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+136], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     rax, [rsi+144]
    mul     r8
    add     rax, r14               // lo + c
    mov     [rdi+144], rax
    mov     r14, rdx               // c = hi
    adc     r14, 0
    mov     [rdi+152], r14           // 顶列直存
    # --- 行对: a_1 与 a_2 ---
    mov     r8, [rsi+8]            // u = a_1
    mov     r9, [rsi+16]            // v = a_2
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+24]          // D 预载
    mov     rax, [rsi+16]          // a2
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+24], r13
    mov     r13, [rdi+32]          // D 预载
    mov     rax, [rsi+24]          // a3*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+32], r13
    mov     r13, [rdi+40]          // D 预载
    mov     rax, [rsi+32]          // a4*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+24]          // a3*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+40], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+48]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+32]          // a4*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+48], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+56], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+64], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+144]          // a18*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+136]          // a17*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+144]          // a18*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+160], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+168], r10
    # --- 行对: a_3 与 a_4 ---
    mov     r8, [rsi+24]            // u = a_3
    mov     r9, [rsi+32]            // v = a_4
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+56]          // D 预载
    mov     rax, [rsi+32]          // a4
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+56], r13
    mov     r13, [rdi+64]          // D 预载
    mov     rax, [rsi+40]          // a5*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+64], r13
    mov     r13, [rdi+72]          // D 预载
    mov     rax, [rsi+48]          // a6*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+40]          // a5*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+72], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+80]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+48]          // a6*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+80], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+88], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+96], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+144]          // a18*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+136]          // a17*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+144]          // a18*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+176], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+184], r10
    # --- 行对: a_5 与 a_6 ---
    mov     r8, [rsi+40]            // u = a_5
    mov     r9, [rsi+48]            // v = a_6
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+88]          // D 预载
    mov     rax, [rsi+48]          // a6
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+88], r13
    mov     r13, [rdi+96]          // D 预载
    mov     rax, [rsi+56]          // a7*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+96], r13
    mov     r13, [rdi+104]          // D 预载
    mov     rax, [rsi+64]          // a8*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+56]          // a7*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+104], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+112]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+64]          // a8*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+112], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+120], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+128], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+144]          // a18*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+136]          // a17*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+184], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+144]          // a18*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+192], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+200], r10
    # --- 行对: a_7 与 a_8 ---
    mov     r8, [rsi+56]            // u = a_7
    mov     r9, [rsi+64]            // v = a_8
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+120]          // D 预载
    mov     rax, [rsi+64]          // a8
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+120], r13
    mov     r13, [rdi+128]          // D 预载
    mov     rax, [rsi+72]          // a9*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+128], r13
    mov     r13, [rdi+136]          // D 预载
    mov     rax, [rsi+80]          // a10*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+72]          // a9*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+136], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+144]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+80]          // a10*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+144], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+152], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+160], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+184], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+192], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+200]          // D 预载
    mov     rax, [rsi+144]          // a18*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+136]          // a17*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+200], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+144]          // a18*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+208], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+216], r10
    # --- 行对: a_9 与 a_10 ---
    mov     r8, [rsi+72]            // u = a_9
    mov     r9, [rsi+80]            // v = a_10
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+152]          // D 预载
    mov     rax, [rsi+80]          // a10
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+152], r13
    mov     r13, [rdi+160]          // D 预载
    mov     rax, [rsi+88]          // a11*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+160], r13
    mov     r13, [rdi+168]          // D 预载
    mov     rax, [rsi+96]          // a12*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+88]          // a11*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+168], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+176]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+96]          // a12*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+176], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+184], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+192], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+200]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+200], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+208]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+208], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+216]          // D 预载
    mov     rax, [rsi+144]          // a18*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+136]          // a17*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+216], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+144]          // a18*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+224], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+232], r10
    # --- 行对: a_11 与 a_12 ---
    mov     r8, [rsi+88]            // u = a_11
    mov     r9, [rsi+96]            // v = a_12
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+184]          // D 预载
    mov     rax, [rsi+96]          // a12
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+184], r13
    mov     r13, [rdi+192]          // D 预载
    mov     rax, [rsi+104]          // a13*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+192], r13
    mov     r13, [rdi+200]          // D 预载
    mov     rax, [rsi+112]          // a14*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+104]          // a13*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+200], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+208]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+112]          // a14*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+208], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+216]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+216], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+224]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+224], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+232]          // D 预载
    mov     rax, [rsi+144]          // a18*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+136]          // a17*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+232], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+144]          // a18*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+240], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+248], r10
    # --- 行对: a_13 与 a_14 ---
    mov     r8, [rsi+104]            // u = a_13
    mov     r9, [rsi+112]            // v = a_14
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+216]          // D 预载
    mov     rax, [rsi+112]          // a14
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+216], r13
    mov     r13, [rdi+224]          // D 预载
    mov     rax, [rsi+120]          // a15*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+224], r13
    mov     r13, [rdi+232]          // D 预载
    mov     rax, [rsi+128]          // a16*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+120]          // a15*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+232], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+240]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+128]          // a16*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+240], r13
    adc     r15, 0                 // cv += gamma2
    mov     r13, [rdi+248]          // D 预载
    mov     rax, [rsi+144]          // a18*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+136]          // a17*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+248], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+144]          // a18*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+256], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+264], r10
    # --- 行对: a_15 与 a_16 ---
    mov     r8, [rsi+120]            // u = a_15
    mov     r9, [rsi+128]            // v = a_16
    xor     r15d, r15d             // cv = 0
    mov     r13, [rdi+248]          // D 预载
    mov     rax, [rsi+128]          // a16
    mul     r8
    add     r13, rax               // D + lo_u
    mov     r14, rdx               // cu = hu
    adc     r14, 0
    mov     [rdi+248], r13
    mov     r13, [rdi+256]          // D 预载
    mov     rax, [rsi+136]          // a17*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma
    mov     [rdi+256], r13
    mov     r13, [rdi+264]          // D 预载
    mov     rax, [rsi+144]          // a18*u
    mul     r8
    add     rax, r14               // lo_u + cu
    mov     r14, rdx               // cu = hu
    adc     r14, 0                 // += alpha
    add     r13, rax               // D + lo_u'
    adc     r14, 0                 // cu += gamma1
    mov     rax, [rsi+136]          // a17*v
    mul     r9
    add     rax, r15               // lo_v + cv
    mov     r15, rdx               // cv = hv
    adc     r15, 0                 // += beta
    add     r13, rax               // + lo_v'
    mov     [rdi+264], r13
    adc     r15, 0                 // cv += gamma2
    mov     rax, [rsi+144]          // a18*v 顶部积
    mul     r9
    add     rax, r14               // + cu
    mov     r10, rdx               // Y = hv
    adc     r10, 0                 // Y += alpha
    add     rax, r15               // + cv
    mov     [rdi+272], rax
    adc     r10, 0                 // Y += beta
    mov     [rdi+280], r10
    # --- 单行: a_17 ---
    mov     r8, [rsi+136]            // u = a_17
    xor     r14d, r14d             // c = 0
    mov     r13, [rdi+280]
    mov     rax, [rsi+144]
    mul     r8
    add     rax, r14               // lo + c
    mov     r14, rdx               // c = hi
    adc     r14, 0
    add     r13, rax               // D + lo'
    adc     r14, 0
    mov     [rdi+280], r13
    mov     [rdi+288], r14          // 顶列直存
    # dst *= 2
    mov     rax, [rdi+8]
    add     rax, rax
    mov     [rdi+8], rax
    mov     rax, [rdi+16]
    adc     rax, rax
    mov     [rdi+16], rax
    mov     rax, [rdi+24]
    adc     rax, rax
    mov     [rdi+24], rax
    mov     rax, [rdi+32]
    adc     rax, rax
    mov     [rdi+32], rax
    mov     rax, [rdi+40]
    adc     rax, rax
    mov     [rdi+40], rax
    mov     rax, [rdi+48]
    adc     rax, rax
    mov     [rdi+48], rax
    mov     rax, [rdi+56]
    adc     rax, rax
    mov     [rdi+56], rax
    mov     rax, [rdi+64]
    adc     rax, rax
    mov     [rdi+64], rax
    mov     rax, [rdi+72]
    adc     rax, rax
    mov     [rdi+72], rax
    mov     rax, [rdi+80]
    adc     rax, rax
    mov     [rdi+80], rax
    mov     rax, [rdi+88]
    adc     rax, rax
    mov     [rdi+88], rax
    mov     rax, [rdi+96]
    adc     rax, rax
    mov     [rdi+96], rax
    mov     rax, [rdi+104]
    adc     rax, rax
    mov     [rdi+104], rax
    mov     rax, [rdi+112]
    adc     rax, rax
    mov     [rdi+112], rax
    mov     rax, [rdi+120]
    adc     rax, rax
    mov     [rdi+120], rax
    mov     rax, [rdi+128]
    adc     rax, rax
    mov     [rdi+128], rax
    mov     rax, [rdi+136]
    adc     rax, rax
    mov     [rdi+136], rax
    mov     rax, [rdi+144]
    adc     rax, rax
    mov     [rdi+144], rax
    mov     rax, [rdi+152]
    adc     rax, rax
    mov     [rdi+152], rax
    mov     rax, [rdi+160]
    adc     rax, rax
    mov     [rdi+160], rax
    mov     rax, [rdi+168]
    adc     rax, rax
    mov     [rdi+168], rax
    mov     rax, [rdi+176]
    adc     rax, rax
    mov     [rdi+176], rax
    mov     rax, [rdi+184]
    adc     rax, rax
    mov     [rdi+184], rax
    mov     rax, [rdi+192]
    adc     rax, rax
    mov     [rdi+192], rax
    mov     rax, [rdi+200]
    adc     rax, rax
    mov     [rdi+200], rax
    mov     rax, [rdi+208]
    adc     rax, rax
    mov     [rdi+208], rax
    mov     rax, [rdi+216]
    adc     rax, rax
    mov     [rdi+216], rax
    mov     rax, [rdi+224]
    adc     rax, rax
    mov     [rdi+224], rax
    mov     rax, [rdi+232]
    adc     rax, rax
    mov     [rdi+232], rax
    mov     rax, [rdi+240]
    adc     rax, rax
    mov     [rdi+240], rax
    mov     rax, [rdi+248]
    adc     rax, rax
    mov     [rdi+248], rax
    mov     rax, [rdi+256]
    adc     rax, rax
    mov     [rdi+256], rax
    mov     rax, [rdi+264]
    adc     rax, rax
    mov     [rdi+264], rax
    mov     rax, [rdi+272]
    adc     rax, rax
    mov     [rdi+272], rax
    mov     rax, [rdi+280]
    adc     rax, rax
    mov     [rdi+280], rax
    mov     rax, [rdi+288]
    adc     rax, rax
    mov     [rdi+288], rax
    mov     rax, [rdi+296]
    adc     rax, rax
    mov     [rdi+296], rax
    # 对角平方折叠
    xor     ebx, ebx                // zero, 清 CF/OF
    xor     r8d, r8d                // c = 0
    mov     rdx, [rsi+0]
    mov     rax, rdx
    mulx    r11, r10, rax          // a0^2
    mov     r9, [rdi+0]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+0], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+8]
    adcx    r9, r8
    mov     [rdi+8], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+8]
    mov     rax, rdx
    mulx    r11, r10, rax          // a1^2
    mov     r9, [rdi+16]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+16], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+24]
    adcx    r9, r8
    mov     [rdi+24], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+16]
    mov     rax, rdx
    mulx    r11, r10, rax          // a2^2
    mov     r9, [rdi+32]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+32], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+40]
    adcx    r9, r8
    mov     [rdi+40], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+24]
    mov     rax, rdx
    mulx    r11, r10, rax          // a3^2
    mov     r9, [rdi+48]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+48], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+56]
    adcx    r9, r8
    mov     [rdi+56], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+32]
    mov     rax, rdx
    mulx    r11, r10, rax          // a4^2
    mov     r9, [rdi+64]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+64], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+72]
    adcx    r9, r8
    mov     [rdi+72], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+40]
    mov     rax, rdx
    mulx    r11, r10, rax          // a5^2
    mov     r9, [rdi+80]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+80], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+88]
    adcx    r9, r8
    mov     [rdi+88], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+48]
    mov     rax, rdx
    mulx    r11, r10, rax          // a6^2
    mov     r9, [rdi+96]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+96], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+104]
    adcx    r9, r8
    mov     [rdi+104], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+56]
    mov     rax, rdx
    mulx    r11, r10, rax          // a7^2
    mov     r9, [rdi+112]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+112], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+120]
    adcx    r9, r8
    mov     [rdi+120], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+64]
    mov     rax, rdx
    mulx    r11, r10, rax          // a8^2
    mov     r9, [rdi+128]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+128], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+136]
    adcx    r9, r8
    mov     [rdi+136], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+72]
    mov     rax, rdx
    mulx    r11, r10, rax          // a9^2
    mov     r9, [rdi+144]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+144], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+152]
    adcx    r9, r8
    mov     [rdi+152], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+80]
    mov     rax, rdx
    mulx    r11, r10, rax          // a10^2
    mov     r9, [rdi+160]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+160], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+168]
    adcx    r9, r8
    mov     [rdi+168], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+88]
    mov     rax, rdx
    mulx    r11, r10, rax          // a11^2
    mov     r9, [rdi+176]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+176], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+184]
    adcx    r9, r8
    mov     [rdi+184], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+96]
    mov     rax, rdx
    mulx    r11, r10, rax          // a12^2
    mov     r9, [rdi+192]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+192], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+200]
    adcx    r9, r8
    mov     [rdi+200], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+104]
    mov     rax, rdx
    mulx    r11, r10, rax          // a13^2
    mov     r9, [rdi+208]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+208], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+216]
    adcx    r9, r8
    mov     [rdi+216], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+112]
    mov     rax, rdx
    mulx    r11, r10, rax          // a14^2
    mov     r9, [rdi+224]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+224], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+232]
    adcx    r9, r8
    mov     [rdi+232], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+120]
    mov     rax, rdx
    mulx    r11, r10, rax          // a15^2
    mov     r9, [rdi+240]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+240], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+248]
    adcx    r9, r8
    mov     [rdi+248], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+128]
    mov     rax, rdx
    mulx    r11, r10, rax          // a16^2
    mov     r9, [rdi+256]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+256], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+264]
    adcx    r9, r8
    mov     [rdi+264], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+136]
    mov     rax, rdx
    mulx    r11, r10, rax          // a17^2
    mov     r9, [rdi+272]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+272], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+280]
    adcx    r9, r8
    mov     [rdi+280], r9
    mov     r8, rbx
    adcx    r8, rbx               // c = CF
    mov     rdx, [rsi+144]
    mov     rax, rdx
    mulx    r11, r10, rax          // a18^2
    mov     r9, [rdi+288]
    adox    r9, r8
    adcx    r9, r10
    mov     [rdi+288], r9
    mov     r8, r11
    adcx    r8, rbx
    adox    r8, rbx
    mov     r9, [rdi+296]
    adcx    r9, r8                  // 最高列 (最终进位必为0)
    mov     [rdi+296], r9
    pop     r15
    pop     r14
    pop     r13
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret
