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
// void lmmp_divexact_by3_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
// void lmmp_divexact_by9_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
// void lmmp_divexact_by15_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
//
//   Windows: dst=rcx, numa=rdx, na=r8
//   SysV:    dst=rdi, numa=rsi, na=rdx
//   cal: Hensel 精确除法, 每 limb:
//            l = s - c,  q = l * INV_k mod B,  c' = borrow(l) + floor(k*q/B)
//        三个函数的核心循环结构不同 (进位计算恒等式各异), 勿强行共享。
//        恒等式 (B = 2^64):
//            floor(3q/B)  = 2 - [q<T1] - [q<T2],   T1=0x5555_5555_5555_5556
//                                                T2=0xAAAA_AAAA_AAAA_AAAB (=INV3)
//            floor(9q/B)  = (q>>61) + [l<q]
//                证: t+q=9q≡l (mod B), carry(t+q) ⟺ t≥B−q ⟺ t=(l−q)mod B 回绕 ⟺ q>l
//            floor(15q/B) = (q>>60) - [(q<<4)<q]
//   regs: rdi=dst, rsi=numa, r8=na, rcx=counter, r11=INV/T2, r10=c/borrow,
//         rax=q, r9/rdx=scratch
//   note: c 值域 by3<=3, by9<=9, by15<=15, setc r10b 只写低字节且高位恒 0;
//         by9/by15 的 c 在 rax, 借位经 setb r10b 物化后单条 adc/sbb 折叠。
//

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_divexact_by3_
// ======================================
.p2align 4
.globl ASM_GSYM(lmmp_divexact_by3_)
ASM_GSYM(lmmp_divexact_by3_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    mov     rdi, rcx                    // SysV:
    mov     rsi, rdx                    //   rdi=dst, rsi=numa, r8=na
#else
    mov     r8, rdx                     // r8 = na
#endif
    movabs  r11, 0xAAAAAAAAAAAAAAAB     // T2 = [q>=T2] 阈值, 恰为 INV3
    movabs  r9, 0x5555555555555556      // T1
    xor     r10d, r10d                  // c = 0
    mov     rcx, r8
    and     rcx, 3                      // 余数 limb 先行
    jz      .Lby3_groups

.p2align 4
.Lby3_scalar:
    mov     rax, [rsi]
    sub     rax, r10                    // l = s - c, CF = borrow
    cmc                                 // CF = !borrow
    sbb     r10, r10                    // r10 = borrow - 1 (全宽写, 免部分寄存器合并)
    add     r10, 3                      // r10 = borrow + 2
    imul    rax, r11                    // q
    mov     [rdi], rax
    cmp     rax, r9                     // CF1 = q < T1
    sbb     r10, 0                      // c' -= CF1
    cmp     rax, r11                    // CF2 = q < T2
    sbb     r10, 0                      // c' -= CF2
    lea     rsi, [rsi+8]
    lea     rdi, [rdi+8]
    dec     rcx
    jnz     .Lby3_scalar

.Lby3_groups:
    mov     rcx, r8
    shr     rcx, 2                      // 4 limbs per group
    jz      .Lby3_done

.p2align 4
.Lby3_loop:                             // main loop, 4 limbs
    mov     rax, [rsi]                  // s0
    sub     rax, r10
    cmc
    sbb     r10, r10
    add     r10, 3
    imul    rax, r11                    // q0
    mov     [rdi], rax
    cmp     rax, r9
    sbb     r10, 0
    cmp     rax, r11
    sbb     r10, 0

    mov     rax, [rsi+8]                // s1
    sub     rax, r10
    cmc
    sbb     r10, r10
    add     r10, 3
    imul    rax, r11                    // q1
    mov     [rdi+8], rax
    cmp     rax, r9
    sbb     r10, 0
    cmp     rax, r11
    sbb     r10, 0

    mov     rax, [rsi+16]               // s2
    sub     rax, r10
    cmc
    sbb     r10, r10
    add     r10, 3
    imul    rax, r11                    // q2
    mov     [rdi+16], rax
    cmp     rax, r9
    sbb     r10, 0
    cmp     rax, r11
    sbb     r10, 0

    mov     rax, [rsi+24]               // s3
    sub     rax, r10
    cmc
    sbb     r10, r10
    add     r10, 3
    imul    rax, r11                    // q3
    mov     [rdi+24], rax
    cmp     rax, r9
    sbb     r10, 0
    cmp     rax, r11
    sbb     r10, 0

    lea     rsi, [rsi+32]
    lea     rdi, [rdi+32]
    dec     rcx
    jnz     .Lby3_loop

.Lby3_done:
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

// ======================================
// lmmp_divexact_by9_
// ======================================
.p2align 4
.globl ASM_GSYM(lmmp_divexact_by9_)
ASM_GSYM(lmmp_divexact_by9_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    mov     rdi, rcx
    mov     rsi, rdx
#else
    mov     r8, rdx
#endif
    movabs  r11, 0x8E38E38E38E38E39      // INV9
    xor     eax, eax                    // c = 0
    mov     rcx, r8
    and     rcx, 3
    jz      .Lby9_groups

.p2align 4
.Lby9_scalar:
    mov     r9, [rsi]                   // s
    xor     r10d, r10d
    sub     r9, rax                     // l, CF = borrow
    mov     rax, r9                     // l
    setc    r10b                        // borrow
    imul    rax, r11                    // q
    mov     [rdi], rax
    mov     rdx, rax
    shr     rdx, 61                     // hi = q>>61
    cmp     r9, rax                     // CF = l < q
    mov     rax, r10
    adc     rax, rdx                    // c' = borrow + hi + CF
    lea     rsi, [rsi+8]
    lea     rdi, [rdi+8]
    dec     rcx
    jnz     .Lby9_scalar

.Lby9_groups:
    mov     rcx, r8
    shr     rcx, 2
    jz      .Lby9_done

.p2align 4
.Lby9_loop:                             // main loop, 4 limbs
    mov     r9, [rsi]                   // s0
    xor     r10d, r10d
    sub     r9, rax
    mov     rax, r9                     // l
    setc    r10b
    imul    rax, r11                    // q0
    mov     [rdi], rax
    mov     rdx, rax
    shr     rdx, 61
    cmp     r9, rax                     // CF = l < q
    mov     rax, r10
    adc     rax, rdx

    mov     r9, [rsi+8]                 // s1
    xor     r10d, r10d
    sub     r9, rax
    mov     rax, r9
    setc    r10b
    imul    rax, r11                    // q1
    mov     [rdi+8], rax
    mov     rdx, rax
    shr     rdx, 61
    cmp     r9, rax
    mov     rax, r10
    adc     rax, rdx

    mov     r9, [rsi+16]                // s2
    xor     r10d, r10d
    sub     r9, rax
    mov     rax, r9
    setc    r10b
    imul    rax, r11                    // q2
    mov     [rdi+16], rax
    mov     rdx, rax
    shr     rdx, 61
    cmp     r9, rax
    mov     rax, r10
    adc     rax, rdx

    mov     r9, [rsi+24]                // s3
    xor     r10d, r10d
    sub     r9, rax
    mov     rax, r9
    setc    r10b
    imul    rax, r11                    // q3
    mov     [rdi+24], rax
    mov     rdx, rax
    shr     rdx, 61
    cmp     r9, rax
    mov     rax, r10
    adc     rax, rdx

    lea     rsi, [rsi+32]
    lea     rdi, [rdi+32]
    dec     rcx
    jnz     .Lby9_loop

.Lby9_done:
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

// ======================================
// lmmp_divexact_by15_
// ======================================
.p2align 4
.globl ASM_GSYM(lmmp_divexact_by15_)
ASM_GSYM(lmmp_divexact_by15_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    mov     rdi, rcx
    mov     rsi, rdx
#else
    mov     r8, rdx
#endif
    movabs  r11, 0xEEEEEEEEEEEEEEEF     // INV15
    xor     eax, eax                    // c = 0
    mov     rcx, r8
    and     rcx, 3
    jz      .Lby15_groups

.p2align 4
.Lby15_scalar:
    mov     r9, [rsi]                   // s
    xor     r10d, r10d
    sub     r9, rax                     // l, CF = borrow
    mov     rax, r9                     // l
    setc    r10b                        // borrow
    imul    rax, r11                    // q
    mov     [rdi], rax
    mov     r9, rax                     // q 副本
    mov     rdx, rax
    shr     rdx, 60                     // hi = q>>60
    shl     rax, 4                      // t = q<<4
    cmp     rax, r9                     // CF = t < q
    lea     rax, [r10+rdx]              // borrow + hi
    sbb     rax, 0                      // c' = borrow + hi - CF
    lea     rsi, [rsi+8]
    lea     rdi, [rdi+8]
    dec     rcx
    jnz     .Lby15_scalar

.Lby15_groups:
    mov     rcx, r8
    shr     rcx, 2
    jz      .Lby15_done

.p2align 4
.Lby15_loop:                            // main loop, 4 limbs
    mov     r9, [rsi]                   // s0
    xor     r10d, r10d
    sub     r9, rax
    mov     rax, r9
    setc    r10b
    imul    rax, r11                    // q0
    mov     [rdi], rax
    mov     r9, rax
    mov     rdx, rax
    shr     rdx, 60
    shl     rax, 4
    cmp     rax, r9
    lea     rax, [r10+rdx]
    sbb     rax, 0

    mov     r9, [rsi+8]                 // s1
    xor     r10d, r10d
    sub     r9, rax
    mov     rax, r9
    setc    r10b
    imul    rax, r11                    // q1
    mov     [rdi+8], rax
    mov     r9, rax
    mov     rdx, rax
    shr     rdx, 60
    shl     rax, 4
    cmp     rax, r9
    lea     rax, [r10+rdx]
    sbb     rax, 0

    mov     r9, [rsi+16]                // s2
    xor     r10d, r10d
    sub     r9, rax
    mov     rax, r9
    setc    r10b
    imul    rax, r11                    // q2
    mov     [rdi+16], rax
    mov     r9, rax
    mov     rdx, rax
    shr     rdx, 60
    shl     rax, 4
    cmp     rax, r9
    lea     rax, [r10+rdx]
    sbb     rax, 0

    mov     r9, [rsi+24]                // s3
    xor     r10d, r10d
    sub     r9, rax
    mov     rax, r9
    setc    r10b
    imul    rax, r11                    // q3
    mov     [rdi+24], rax
    mov     r9, rax
    mov     rdx, rax
    shr     rdx, 60
    shl     rax, 4
    cmp     rax, r9
    lea     rax, [r10+rdx]
    sbb     rax, 0

    lea     rsi, [rsi+32]
    lea     rdi, [rdi+32]
    dec     rcx
    jnz     .Lby15_loop

.Lby15_done:
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret
