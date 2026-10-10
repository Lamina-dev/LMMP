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

//
// void lmmp_sqrlo_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t n);
//
//   Windows: dst=rcx, numa=rdx, n=r8
//   SysV:    dst=rdi, numa=rsi, n=rdx
//   cal: dst[0..n) = (numa^2) mod B^n
//
//   三角分解 U^2 = 2*T + D (mod B^n)：
//     行 j 计算 T 的一片：dst[2j+1..n-1] += a_j * a[j+1..n-1-j]，
//     乘加链恰好终止于第 n-1 列，链尾进位落在第 n 列直接丢弃。
//     行 0 为 mul_1 现写链（.Lmul1_*），行 1.. 为 addmul_1 累加链（.Lam_*），
//     末尾一趟融合的 2*dst + a_i^2 对角收尾（.Ldiag_*）。
//   寻址约定：rdi = dst + 8n 固定，rsi = numa + 8n - 8j 随行下移，
//   行窗口 [rbx,-1]（rbx = 2j+1-n）相对两锚同偏移读写。
//

#include "lmmp_asm.h"

.text

#ifdef LMMP_WINDOWS
    #define P_DST rcx
    #define P_NUMA rdx
    #define P_N r8
#else
    #define P_DST rdi
    #define P_NUMA rsi
    #define P_N rdx
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

// ====================== .text ======================

.text
.globl ASM_GSYM(lmmp_sqrlo_basecase_)
.p2align 4

ASM_GSYM(lmmp_sqrlo_basecase_):
    test    P_N, P_N
    jz      .Lret                 // n == 0
    cmp     P_N, 1
    jne     .Lbig
    mov     rax, [P_NUMA]         // n == 1: dst[0] = lo(a0^2)
    mul     rax
    mov     [P_DST], rax
.Lret:
    ret

.Lbig:
    PUSH_ALL
    PUSH_EXTRA

#ifdef LMMP_WINDOWS
    mov     rdi, P_DST            // dst -> rdi
    mov     rsi, P_NUMA           // numa -> rsi
    mov     rdx, P_N              // n -> rdx
#endif

    mov     r12, rsi              // r12 = numa（对角收尾用）
    mov     r13, rdi              // r13 = dst
    mov     r14, rdx              // r14 = n
    lea     rsi, [rsi + rdx*8]    // rsi = numa + n
    lea     rdi, [rdi + rdx*8]    // rdi = dst + n
    lea     rbx, [rdx - 1]
    neg     rbx                   // rbx = 1-n（行 0 窗口基址）

    mov     r9, [rsi + rbx*8 - 8] // r9 = a_0
    mov     rax, [rsi + rbx*8]    // rax = a_1
    mul     r9                    // rdx:rax = a_0 * a_1（第 1 列）

    test    bl, 1
    jnz     .Lmul1_odd

.Lmul1_even:                      // 窗口基址为偶
    mov     r10, rax
    mov     r11, rdx
    mov     rax, [rsi + rbx*8 + 8]
    test    bl, 2
    jnz     .Lmul1_even_2mod4

    lea     rbp, [rbx + 2]        // rbx ≡ 0 (mod 4)
    jmp     .Lmul1_l0

.Lmul1_even_2mod4:                // rbx ≡ 2 (mod 4)
    lea     rbp, [rbx]
    jmp     .Lmul1_l2

.Lmul1_odd:                       // 窗口基址为奇
    mov     r11, rax
    mov     r10, rdx
    test    bl, 2
    jz      .Lmul1_odd_3mod4

    lea     rbp, [rbx + 3]        // rbx ≡ 1 (mod 4)
    test    rbp, rbp
    js      .Lmul1_l1
    mov     [rdi - 8], rax        // 窗口长度 1（n == 2）：dst[1] = lo(a0*a1)
    jmp     .Lrow

.Lmul1_odd_3mod4:                 // rbx ≡ 3 (mod 4)
    lea     rbp, [rbx + 1]
    mov     rax, [rsi + rbx*8 + 8]
    jmp     .Lmul1_l3

.p2align 4
.Lmul1_tp:                        // mul_1 主循环，4 limbs
    mov     r10, rdx
    add     r11, rax
.Lmul1_l1:
    mov     rax, [rsi + rbp*8 - 16]
    adc     r10, 0
    mul     r9
    add     r10, rax
    mov     [rdi + rbp*8 - 24], r11
    mov     rax, [rsi + rbp*8 - 8]
    mov     r11, rdx
    adc     r11, 0
.Lmul1_l0:
    mul     r9
    mov     [rdi + rbp*8 - 16], r10
    add     r11, rax
    mov     r10, rdx
    mov     rax, [rsi + rbp*8]
    adc     r10, 0
.Lmul1_l3:
    mul     r9
    mov     [rdi + rbp*8 - 8], r11
    mov     r11, rdx
    add     r10, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r11, 0
.Lmul1_l2:
    mul     r9
    mov     [rdi + rbp*8], r10
    add     rbp, 4
    jnc     .Lmul1_tp

.Lmul1_ed:
    add     r11, rax
    adc     rdx, 0
    mov     [rdi - 8], r11        // dst[n-1]
    // carry discarded

// ======================================
// 行循环：行 j 的窗口 [2j+1-n, -1]（rdi 固定 dst+n，
// rsi = numa + n - 8j 随行下移，切片/目标列共用偏移）
// ======================================
.p2align 4
.Lrow:
    add     rbx, 2                // 下一行窗口基址 = 2j+1-n
    jns     .Ldiag                // 基址 >= 0：行耗尽
    sub     rsi, 8
    mov     r9, [rsi + rbx*8 - 8] // r9 = a_j
    mov     rax, [rsi + rbx*8]    // rax = a_{j+1}
    mul     r9

    test    bl, 1
    jnz     .Lrow_odd

.Lrow_even:
    mov     r10, rax
    mov     r11, rdx
    mov     rax, [rsi + rbx*8 + 8]
    test    bl, 2
    jnz     .Lrow_even_2mod4

    lea     rbp, [rbx + 2]        // rbx ≡ 0 (mod 4)
    jmp     .Lam_l0

.Lrow_even_2mod4:                 // rbx ≡ 2 (mod 4)
    lea     rbp, [rbx]
    jmp     .Lam_l2

.Lrow_odd:
    mov     r11, rax
    mov     r10, rdx
    test    bl, 2
    jz      .Lrow_odd_3mod4

.Lrow_odd_1mod4:                  // rbx ≡ 1 (mod 4)
    lea     rbp, [rbx + 3]
    test    rbp, rbp
    js      .Lam_l1
    add     r11, [rdi - 8]        // 窗口长度 1：dst[n-1] += lo
    adc     r10, 0
    mov     [rdi - 8], r11
    // hi discarded
    jmp     .Lrow

.Lrow_odd_3mod4:                  // rbx ≡ 3 (mod 4)
    lea     rbp, [rbx + 1]
    mov     rax, [rsi + rbx*8 + 8]
    jmp     .Lam_l3

.p2align 4
.Lam_tp:                          // addmul_1 主循环，4 limbs
    mov     r10, rdx
    add     r11, rax
.Lam_l1:
    mov     rax, [rsi + rbp*8 - 16]
    adc     r10, 0
    mul     r9
    add     r11, [rdi + rbp*8 - 24]
    adc     r10, 0
    mov     [rdi + rbp*8 - 24], r11
    add     r10, rax
    mov     rax, [rsi + rbp*8 - 8]
    mov     r11, rdx
    adc     r11, 0
.Lam_l0:
    mul     r9
    add     r10, [rdi + rbp*8 - 16]
    adc     r11, 0
    mov     [rdi + rbp*8 - 16], r10
    add     r11, rax
    mov     r10, rdx
    mov     rax, [rsi + rbp*8]
    adc     r10, 0
.Lam_l3:
    mul     r9
    add     r11, [rdi + rbp*8 - 8]
    adc     r10, 0
    mov     [rdi + rbp*8 - 8], r11
    mov     r11, rdx
    add     r10, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r11, 0
.Lam_l2:
    mul     r9
    add     r10, [rdi + rbp*8]
    adc     r11, 0
    mov     [rdi + rbp*8], r10
    add     rbp, 4
    jnc     .Lam_tp

.Lam_ed:
    add     r11, rax
    adc     rdx, 0
    add     r11, [rdi - 8]        // dst[n-1] += 累计值
    adc     rdx, 0
    mov     [rdi - 8], r11
    // carry discarded
    jmp     .Lrow

// ======================================
// 对角收尾：dst = 2*dst + sum a_i^2*B^2i，截断于 n
//   r12 = numa, r13 = dst, r14 = n
//   全局 1 位 CF 进位链贯穿：对内仅 setc/lea/dec/jecxx（不触 CF 的
//   指令做控制流），指针步进用 lea；pending（对角高半+存位 ≤ B-1）
//   常驻 r10，偶列乘积 a_i^2 现场产生。
// ======================================
.Ldiag:
    mov     rax, [r12]
    mul     rax                   // rdx:rax = a_0^2
    mov     [r13], rax            // dst[0] = lo(a_0^2)（T[0] 恒 0，无出链）
    mov     r10, rdx              // pending = hi(a_0^2)
    lea     rdi, [r13 + 8]        // &dst[1]
    lea     rsi, [r12 + 8]        // &a[1]
    lea     rcx, [r14 - 1]
    shr     rcx, 1                // 对数 = (n-1)/2（列 1..n-1）
    mov     r11, r14
    and     r11, 1                // 0 = n 偶（有末尾孤奇列）
    xor     ebx, ebx              // c = 0
    clc
    jecxz   .Ldiag_tail

.p2align 4
.Ldiag_pair:
    mov     r8, [rdi]             // 奇列（无对角）
    mov     r9, [rdi + 8]         // 偶列（对角 a_i）
    mov     rax, [rsi]
    adc     r8, r8
    adc     r9, r9
    setc    bl                    // 存链位（mul 将毁 CF）
    mul     rax                   // rdx:rax = a_i^2
    add     r8, r10               // 奇列 += pending
    adc     r9, rax               // 偶列 += lo(a_i^2)，CF 出链
    mov     [rdi], r8
    mov     [rdi + 8], r9
    lea     r10, [rdx + rbx]      // 新 pending = hi(a_i^2) + c ≤ B-1
    lea     rdi, [rdi + 16]
    lea     rsi, [rsi + 8]
    dec     ecx
    jnz     .Ldiag_pair

.Ldiag_tail:
    dec     r11                   // dec 不触 CF；原 0（n 偶）回绕非零落入尾部
    jz      .Ldone                // 原 1（n 奇）：无末尾孤奇列
    mov     r8, [rdi]
    adc     r8, r8                // 2d + 链 CF
    add     r8, r10               // + pending，出格进位丢弃
    mov     [rdi], r8

.Ldone:
    POP_EXTRA
    POP_ALL
    ret
