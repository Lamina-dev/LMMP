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
// mp_limb_t lmmp_add_n_sub_n_(mp_ptr dsta, mp_ptr dstb, mp_srcptr numa, mp_srcptr numb,
//                             mp_size_t n);
//
//   Windows: dsta=rcx, dstb=rdx, numa=r8, numb=r9, n=[rsp+40]
//   SysV:    dsta=rdi, dstb=rsi, numa=rdx, numb=rcx, n=r8
//   ret rax = 2*carry + borrow
//
//   cal: [dsta,n] = numa + numb, [dstb,n] = numa - numb
//
//   双进位链 (adcx/adox 互不侵犯对方标志):
//     和链(CF): adcx s, b            s = a + b + carry
//     差链(OF): 补码加法 d = a + ~b + k, k = 1 - borrow
//               k_0 = 1 (无初始借位), 链上传递 k_{i+1} = 该加法进位,
//               故最终借位 = !OF
//     初始化: mov r10, 0x4000000000000000; add r10, r10
//             (2^62 + 2^62 = 2^63: 有符号溢出置 OF=1, 无符号不进位置 CF=0)
//
//   eqsep(dsta|dstb, numa|numb) 重叠安全: 每肢两加载先于两存储。
//   链建立后所有指令不得破坏 CF/OF: 指针调整只用 lea, 循环计数只用
//   lea+jrcxz+jmp (dec/sub/add/cmp/test 均会改写 CF 或 OF); 长跳转
//   (test/jz)仅在链建立之前使用。
//

#include "lmmp_asm.h"

.text
.p2align 4

.globl ASM_GSYM(lmmp_add_n_sub_n_)

ASM_GSYM(lmmp_add_n_sub_n_):
#ifdef LMMP_WINDOWS
    mov     rax, [rsp+40]         // rax = n
    push    rbx
    push    rsi
    push    rdi
    mov     rdi, rcx              // rdi = dsta
    mov     rsi, rdx              // rsi = dstb
    mov     rdx, r8               // rdx = numa
    mov     r8,  r9               // r8  = numb
    mov     rcx, rax              // rcx = n
#else
    push    rbx
    mov     rax, r8               // rax = n
    mov     r8,  rcx              // r8  = numb
    mov     rcx, rax              // rcx = n
#endif
    shr     rcx, 2                // rcx = n/4 (主循环迭代数)
    and     eax, 3                // rax = n mod 4 (尾部 limb 数)
    mov     rbx, rax              // rbx = 尾部计数
    test    rcx, rcx              // (链未建立, 标志可动)
    jz      .Lsmall               // n < 4, 只有尾部

    mov     r10, 0x4000000000000000
    add     r10, r10              // OF = 1 (k_0), CF = 0

.p2align 4
.Lloop:                           // 主循环: 每迭代 4 肢
    mov     r9,  [rdx]            // r9  = a0
    mov     r10, [r8]             // r10 = b0
    mov     r11, r9               // r11 = a0 (差链副本)
    adcx    r9,  r10              // s0 = a0 + b0 + CF
    not     r10                   // r10 = ~b0 (not 不动标志)
    adox    r11, r10              // d0 = a0 + ~b0 + OF
    mov     [rdi], r9
    mov     [rsi], r11

    mov     r9,  [rdx+8]
    mov     r10, [r8+8]
    mov     r11, r9
    adcx    r9,  r10
    not     r10
    adox    r11, r10
    mov     [rdi+8], r9
    mov     [rsi+8], r11

    mov     r9,  [rdx+16]
    mov     r10, [r8+16]
    mov     r11, r9
    adcx    r9,  r10
    not     r10
    adox    r11, r10
    mov     [rdi+16], r9
    mov     [rsi+16], r11

    mov     r9,  [rdx+24]
    mov     r10, [r8+24]
    mov     r11, r9
    adcx    r9,  r10
    not     r10
    adox    r11, r10
    mov     [rdi+24], r9
    mov     [rsi+24], r11

    lea     rdx, [rdx+32]
    lea     r8,  [r8+32]
    lea     rdi, [rdi+32]
    lea     rsi, [rsi+32]
    lea     rcx, [rcx-1]
    jrcxz   .Ltail
    jmp     .Lloop

.Lsmall:                          // n < 4: 尾部路径独立初始化标志
    mov     r10, 0x4000000000000000
    add     r10, r10              // OF = 1, CF = 0

.Ltail:                           // 尾部: rbx = n mod 4 肢
    mov     rcx, rbx              // mov 不动标志
.Ltail_chk:
    jrcxz   .Lend
    mov     r9,  [rdx]
    mov     r10, [r8]
    mov     r11, r9
    adcx    r9,  r10
    not     r10
    adox    r11, r10
    mov     [rdi], r9
    mov     [rsi], r11
    lea     rdx, [rdx+8]
    lea     r8,  [r8+8]
    lea     rdi, [rdi+8]
    lea     rsi, [rsi+8]
    lea     rcx, [rcx-1]
    jmp     .Ltail_chk

.Lend:
    setc    al                    // al = carry
    setno   cl                    // cl = !OF = borrow
    movzx   eax, al
    movzx   ecx, cl
    lea     eax, [rax*2+rcx]      // rax = 2*carry + borrow

#ifndef LMMP_WINDOWS
    pop     rbx
#else
    pop     rdi
    pop     rsi
    pop     rbx
#endif
    ret
