//  Copyright (C) 2026 HJimmyK (Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it
//  under the terms of the GNU Lesser General Public License as
//  published by the Free Software Foundation; either version 3 of
//  the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, see
//  <https://www.gnu.org/licenses/>.
//
//  SPDX-License-Identifier: LGPL-3.0-or-later
//

//
// mp_limb_t lmmp_ifft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb,
//                                mp_size_t l, mp_size_t w, mp_size_t shr);
//
// void lmmp_fft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb, mp_size_t l,
//                          mp_size_t w, mp_size_t shl, mp_limb_t* out);
//
//   蝶形数据面 mega 融合内核：两段扫描 + 连续进位链单次调用完成
//   （fft_ssa.c 蝶形的 PART_SIZE 块循环整体内嵌，块间进位/移位链在寄存器接力）。
//
//   双进位链 + 无标志位移组合（同前轮结论）：
//     shrd/shl/or 均改写 CF/OF，t = (cur>>shr)|(next<<(64-shr)) 用
//     shrx+shlx+lea 组合（位域不交，or 退化为加法，lea 不动标志）。
//     和链(CF): adcx；差链(OF): 补码加法 d = a + ~t + k, k0 = 1 - b, 借位 = !OF。
//   链建立后仅 mov/lea/shlx/shrx/not/adcx/adox/setcc/movzx/jrcxz/jmp 可用。
//
//   段边界标志重建：setc/setno 捕获两链终值 → 位运算构造 magic →
//   单次 add rX,rX 同时落两标志。add 加倍后 CF = bit63, OF = bit63^bit62：
//     目标 (CF=ca, OF=1-b)：bit63 = ca, bit62 = !(ca^b)
//     目标 (CF=ca, OF=1)  ：bit63 = ca, bit62 = ca^1
//
//   入口组数为 0 的 jrcxz 受 8 位位移限制，经紧邻 trampoline 远跳；
//   正常路径用 jmp 跳过 trampoline 进入循环。
//
//   Windows: 第 5+ 参数在 [rsp+40] 起（影子空间之后），入口先读再 push。
//

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_ifft_bfy_fused_
//   扫描1(i∈[0,w)):  numc[l-w+i] = numa[l-w+i] + t_i, numa[l-w+i] -= t_i
//   扫描2(i∈[0,l-w)): numc[i] = numa[i] - t'_i,       numa[i] += t'_i
//   t 流水线读 numb[0..w] / numb[w..l]，全程只读
//   ret = (acy_final << 1) | bcy_final
//
//   Windows: numc=rcx, numa=rdx, numb=r8, l=r9, w=[rsp+40], shr=[rsp+48]
//   SysV:    numc=rdi, numa=rsi, numb=rdx, l=rcx, w=r8, shr=r9
//   寄存器: rdi=numc 游标, rsi=numa 游标, rbx=numb 游标,
//           rcx=组计数, r8=shr, r9=64-shr, r10/r11=CUR/NEXT 流水线,
//           rbp=T1/~t, r12=T2, r13=t, r14=和(adcx), r15=差(adox)
.p2align 4
.globl ASM_GSYM(lmmp_ifft_bfy_fused_)
ASM_GSYM(lmmp_ifft_bfy_fused_):
#ifdef LMMP_WINDOWS
    mov     rax, [rsp+40]       // rax = w
    mov     r10, [rsp+48]       // r10 = shr
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    push    rdi
    push    rsi
    mov     rdi, rcx            // SysV:
    mov     rsi, rdx            //   rdi=numc, rsi=numa,
    mov     rbx, r8             //   rbx=numb, rcx=l
    mov     rcx, r9
#else
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    mov     rbx, rdx            // rbx = numb
    mov     r10, r9             // r10 = shr
    mov     rax, r8             // rax = w
    // rcx = l, rdi/rsi 就位
#endif
    mov     r11, rcx
    sub     r11, rax            // r11 = n2 = l - w
    mov     r8, r10             // r8 = shr（shrx 计数）
    mov     r9d, 64
    sub     r9, r10             // r9 = 64-shr（shlx 计数）
    push    r11                 // 暂存 n2
    push    rcx                 // 暂存 l
    lea     rsi, [rsi + r11*8]  // numa 游标 <- base + (l-w)
    lea     rdi, [rdi + r11*8]  // numc 游标

    test    rax, rax            // w == 0 → 扫描1 空转
    jz      .Lifft_s1empty

    lea     rcx, [rax-1]        // m1 = w-1（流水线 limb 数，顶 limb 用 numb[w]）
    mov     rax, rcx
    and     eax, 7              // rem1
    shr     rcx, 3              // groups1
    mov     r10, 0x4000000000000000
    add     r10, r10            // CF = 0, OF = 1（链起点 bcy=acy=0）
    mov     r10, [rbx]          // CUR = numb[0]
    jrcxz   .Lifft_z1
    jmp     .Lifft_g1

.p2align 4
.Lifft_z1:                      // 扫描1 入口组数为 0 的 trampoline
    jmp     .Lifft_t1enter
.Lifft_g1:                      // 扫描1 主循环: 每迭代 8 limb
    mov     r11, [rbx+8]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi], r14          // 和 → numc
    mov     [rsi], r15          // 差 → numa

    mov     r10, [rbx+16]
    shrx    rbp, r11, r8
    shlx    r12, r10, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+8]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi+8], r14
    mov     [rsi+8], r15

    mov     r11, [rbx+24]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+16]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi+16], r14
    mov     [rsi+16], r15

    mov     r10, [rbx+32]
    shrx    rbp, r11, r8
    shlx    r12, r10, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+24]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi+24], r14
    mov     [rsi+24], r15

    mov     r11, [rbx+40]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+32]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi+32], r14
    mov     [rsi+32], r15

    mov     r10, [rbx+48]
    shrx    rbp, r11, r8
    shlx    r12, r10, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+40]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi+40], r14
    mov     [rsi+40], r15

    mov     r11, [rbx+56]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+48]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi+48], r14
    mov     [rsi+48], r15

    mov     r10, [rbx+64]
    shrx    rbp, r11, r8
    shlx    r12, r10, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+56]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi+56], r14
    mov     [rsi+56], r15

    lea     rbx, [rbx+64]
    lea     rsi, [rsi+64]
    lea     rdi, [rdi+64]
    lea     rcx, [rcx-1]
    jrcxz   .Lifft_t1enter
    jmp     .Lifft_g1

.Lifft_t1enter:
    mov     rcx, rax            // rcx = rem1
.p2align 4
.Lifft_t1:
    jrcxz   .Lifft_top1
    mov     r11, [rbx+8]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi], r14
    mov     [rsi], r15
    mov     r10, r11            // CUR <- NEXT
    lea     rbx, [rbx+8]
    lea     rsi, [rsi+8]
    lea     rdi, [rdi+8]
    lea     rcx, [rcx-1]
    jmp     .Lifft_t1

.Lifft_top1:                    // 扫描1 顶 limb: t = (CUR>>shr) | (numb[w]<<rsh)
    shrx    rbp, r10, r8
    mov     rax, [rbx+8]
    shlx    rax, rax, r9
    lea     rbx, [rbx+8]        // rbx <- &numb[w]（扫描2 起算点）
    lea     r13, [rbp+rax]
    mov     r14, [rsi]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi], r14
    mov     [rsi], r15
    lea     rsi, [rsi+8]        // 游标推进至 base+l（边界按 rsi-l 回基址）
    lea     rdi, [rdi+8]
    setc    al                  // bcy_f
    setno   cl                  // acy_f
    jmp     .Lifft_bdy

.Lifft_s1empty:                 // w == 0：链终值 0/0
    xor     eax, eax
    xor     ecx, ecx

.Lifft_bdy:                     // 段边界：指针回基址、计数、标志重建
    movzx   eax, al
    movzx   ecx, cl
    pop     rdx                 // l
    pop     r11                 // n2
    neg     rdx
    lea     rsi, [rsi + rdx*8]  // numa 游标 <- base（w==0 路径未推进同样成立）
    lea     rdi, [rdi + rdx*8]  // numc 游标 <- base
    test    r11, r11
    jz      .Lifft_ret

    lea     r10, [r11-1]        // m2 = n2-1
    mov     r12, r10
    and     r12d, 7             // rem2
    shr     r10, 3              // groups2
    mov     r13, rcx
    xor     r13, rax            // acy ^ bcy
    xor     r13d, 1             // !(acy ^ bcy)
    shl     r13, 62
    shl     rcx, 63             // bit63 = acy
    or      r13, rcx
    add     r13, r13            // CF = acy（扫描2 和链进位入）, OF = 1-bcy（差链借位入）
    mov     rcx, r10            // groups2
    mov     r10, [rbx]          // CUR = numb[w]
    mov     rax, r12            // rem2
    jrcxz   .Lifft_z2
    jmp     .Lifft_g2

.p2align 4
.Lifft_z2:                      // 扫描2 入口组数为 0 的 trampoline
    jmp     .Lifft_t2enter
.Lifft_g2:                      // 扫描2 主循环: 每迭代 8 limb（存储目标互换）
    mov     r11, [rbx+8]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi], r14          // 和 → numa
    mov     [rdi], r15          // 差 → numc

    mov     r10, [rbx+16]
    shrx    rbp, r11, r8
    shlx    r12, r10, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+8]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi+8], r14
    mov     [rdi+8], r15

    mov     r11, [rbx+24]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+16]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi+16], r14
    mov     [rdi+16], r15

    mov     r10, [rbx+32]
    shrx    rbp, r11, r8
    shlx    r12, r10, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+24]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi+24], r14
    mov     [rdi+24], r15

    mov     r11, [rbx+40]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+32]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi+32], r14
    mov     [rdi+32], r15

    mov     r10, [rbx+48]
    shrx    rbp, r11, r8
    shlx    r12, r10, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+40]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi+40], r14
    mov     [rdi+40], r15

    mov     r11, [rbx+56]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+48]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi+48], r14
    mov     [rdi+48], r15

    mov     r10, [rbx+64]
    shrx    rbp, r11, r8
    shlx    r12, r10, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi+56]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi+56], r14
    mov     [rdi+56], r15

    lea     rbx, [rbx+64]
    lea     rsi, [rsi+64]
    lea     rdi, [rdi+64]
    lea     rcx, [rcx-1]
    jrcxz   .Lifft_t2enter
    jmp     .Lifft_g2

.Lifft_t2enter:
    mov     rcx, rax            // rcx = rem2
.p2align 4
.Lifft_t2:
    jrcxz   .Lifft_top2
    mov     r11, [rbx+8]
    shrx    rbp, r10, r8
    shlx    r12, r11, r9
    lea     r13, [rbp+r12]
    mov     r14, [rsi]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi], r14
    mov     [rdi], r15
    mov     r10, r11
    lea     rbx, [rbx+8]
    lea     rsi, [rsi+8]
    lea     rdi, [rdi+8]
    lea     rcx, [rcx-1]
    jmp     .Lifft_t2

.Lifft_top2:                    // 扫描2 顶 limb: t = (CUR>>shr) | (numb[l]<<rsh)
    shrx    rbp, r10, r8
    mov     rax, [rbx+8]
    shlx    rax, rax, r9
    lea     r13, [rbp+rax]
    mov     r14, [rsi]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rsi], r14          // 和 → numa
    mov     [rdi], r15          // 差 → numc
    // 扫描2 中 CF 链 = acy（和落 numa）、OF 链 = bcy（差落 numc），与扫描1 相反
    setc    al                  // acy_f
    setno   cl                  // bcy_f
    movzx   eax, al
    movzx   ecx, cl
    lea     eax, [rax*2+rcx]    // ret = (acy<<1) | bcy

#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#else
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#endif
    ret

.Lifft_ret:                     // 扫描2 为空：直接用扫描1 捕获值打包
    lea     eax, [rcx*2+rax]

#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#else
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#endif
    ret

// ======================================
// lmmp_fft_bfy_fused_
//   扫描1(j∈[0,l-w)): numc[w+j] = (numa[j]-numb[j]-scyo)<<shl 接链,
//                     numa[j] = numa[j] + numb[j] (+acyo 链)
//   中点: ch = shlcy1 - (scyo1<<shl) 分解 chp/chn 写 out[0..1]
//   扫描2(j∈[0,w)):   numc[j] = (numb[l-w+j]-numa[l-w+j])<<shl 接链(重置),
//                     numa[l-w+j] += numb[l-w+j] (+acyo 链续)
//   out[2] = shlcy2, out[3] = (acy_final<<1) | scyo_final
//
//   Windows: numc=rcx, numa=rdx, numb=r8, l=r9,
//            w=[rsp+40], shl=[rsp+48], out=[rsp+56]
//   SysV:    numc=rdi, numa=rsi, numb=rdx, l=rcx,
//            w=r8, shl=r9, out=[rsp+8]
//   寄存器: rdi=numc 游标, rsi=numa 游标, rbx=numb 游标, rbp=out,
//           rcx=组计数, r8=shl, r9=64-shl, r10=X/D, r11=XS/和,
//           r12=Y→T1, r13=~Y, r14=OUT, r15=PD(上一差)
.p2align 4
.globl ASM_GSYM(lmmp_fft_bfy_fused_)
ASM_GSYM(lmmp_fft_bfy_fused_):
#ifdef LMMP_WINDOWS
    mov     r11, [rsp+40]       // w
    mov     r10, [rsp+48]       // shl
    mov     rax, [rsp+56]       // out
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    push    rdi
    push    rsi
    mov     rdi, rcx            // numc
    mov     rsi, rdx            // numa
    mov     rbx, r8             // numb
    mov     rcx, r9             // l
    mov     rbp, rax            // out_ptr
#else
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    mov     rbx, rdx            // numb
    mov     r11, r8             // w
    mov     r10, r9             // shl
    mov     rax, [rsp+56]       // out（6 个压栈后）
    mov     rbp, rax
    // rdi/rsi/rcx 就位（numc/numa/l）
#endif
    mov     r8, r10             // r8 = shl（shlx 计数）
    mov     r9d, 64
    sub     r9, r10             // r9 = 64-shl（shrx 计数）
    lea     rdi, [rdi + r11*8]  // numc 游标 <- base + w
    mov     rdx, rcx
    sub     rdx, r11            // n1 = l - w
    push    r11                 // 暂存 w（扫描2 计数）
    push    rcx                 // 暂存 l（numc 回基址用）

    mov     r15d, 0             // PD = 0（shl 链起点；mov 不动标志）
    test    rdx, rdx
    jz      .Lfft_s1empty

    mov     rax, rdx
    and     eax, 7              // rem1
    shr     rdx, 3              // groups1
    mov     r10, 0x4000000000000000
    add     r10, r10            // CF = 0, OF = 1（链起点 scyo=acyo=0）
    mov     rcx, rdx            // groups1
    jrcxz   .Lfft_z1
    jmp     .Lfft_g1

.p2align 4
.Lfft_z1:                       // 扫描1 入口组数为 0 的 trampoline
    jmp     .Lfft_t1enter
.Lfft_g1:                       // 扫描1 主循环: 每迭代 8 limb
    mov     r10, [rsi]
    mov     r11, r10
    mov     r12, [rbx]
    mov     r13, r12
    not     r13
    adcx    r11, r12            // 和链: S = X + Y + CF
    adox    r10, r13            // 差链: D = X + ~Y + OF
    mov     [rsi], r11          // numa = S
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi], r14
    mov     r15, r10

    mov     r10, [rsi+8]
    mov     r11, r10
    mov     r12, [rbx+8]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+8], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+8], r14
    mov     r15, r10

    mov     r10, [rsi+16]
    mov     r11, r10
    mov     r12, [rbx+16]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+16], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+16], r14
    mov     r15, r10

    mov     r10, [rsi+24]
    mov     r11, r10
    mov     r12, [rbx+24]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+24], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+24], r14
    mov     r15, r10

    mov     r10, [rsi+32]
    mov     r11, r10
    mov     r12, [rbx+32]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+32], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+32], r14
    mov     r15, r10

    mov     r10, [rsi+40]
    mov     r11, r10
    mov     r12, [rbx+40]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+40], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+40], r14
    mov     r15, r10

    mov     r10, [rsi+48]
    mov     r11, r10
    mov     r12, [rbx+48]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+48], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+48], r14
    mov     r15, r10

    mov     r10, [rsi+56]
    mov     r11, r10
    mov     r12, [rbx+56]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+56], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+56], r14
    mov     r15, r10

    lea     rbx, [rbx+64]
    lea     rsi, [rsi+64]
    lea     rdi, [rdi+64]
    lea     rcx, [rcx-1]
    jrcxz   .Lfft_t1enter
    jmp     .Lfft_g1

.Lfft_t1enter:
    mov     rcx, rax            // rcx = rem1
.p2align 4
.Lfft_t1:
    jrcxz   .Lfft_end1
    mov     r10, [rsi]
    mov     r11, r10
    mov     r12, [rbx]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi], r14
    mov     r15, r10
    lea     rbx, [rbx+8]
    lea     rsi, [rsi+8]
    lea     rdi, [rdi+8]
    lea     rcx, [rcx-1]
    jmp     .Lfft_t1

.Lfft_end1:                     // 扫描1 完成：提取三链终值
    shrx    rax, r15, r9        // shlcy1
    setc    r11b                // acy1
    setno   r12b                // scy1
    movzx   r11d, r11b
    movzx   r12d, r12b
    jmp     .Lfft_mid

.Lfft_s1empty:                  // l == w：链终值全 0
    xor     r11d, r11d
    xor     r12d, r12d
    xor     eax, eax

.Lfft_mid:                      // ch = shlcy1 - (scy1<<shl) 分解
    shlx    r12, r12, r8        // adj
    sub     rax, r12            // CF = (shlcy1 < adj)
    mov     r13, rax
    mov     r14d, 0             // mov 不动标志
    jnc     1f
    neg     r13
    mov     r14, r13            // chn
    mov     r13d, 0             // chp = 0
1:
    mov     [rbp], r13          // out[0] = chp
    mov     [rbp+8], r14        // out[1] = chn
    pop     r10                 // l
    pop     rdx                 // w（n2）
    neg     r10
    lea     rdi, [rdi + r10*8]  // numc 游标 <- base
    test    rdx, rdx
    jz      .Lfft_ret2empty

    mov     rax, rdx
    and     eax, 7              // rem2
    shr     rdx, 3              // groups2
    mov     r15d, 0             // PD 重置（shl 链重开）
    mov     r13, r11
    xor     r13d, 1             // bit62 = acy1 ^ 1
    shl     r13, 62
    shl     r11, 63             // bit63 = acy1
    or      r13, r11
    add     r13, r13            // CF = acy1（和链续）, OF = 1（差链重置 scyo=0）
    mov     rcx, rdx            // groups2
    jrcxz   .Lfft_z2
    jmp     .Lfft_g2

.Lfft_ret2empty:                // w == 0：out[2]=0, out[3]=acy1<<1
    mov     rax, 0
    mov     [rbp+16], rax
    lea     rax, [r11*2]
    mov     [rbp+24], rax

#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#else
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#endif
    ret

.p2align 4
.Lfft_z2:                       // 扫描2 入口组数为 0 的 trampoline
    jmp     .Lfft_t2enter
.Lfft_g2:                       // 扫描2 主循环: 每迭代 8 limb（X 取 numb，Y 取 numa）
    mov     r10, [rbx]
    mov     r11, r10
    mov     r12, [rsi]
    mov     r13, r12
    not     r13
    adcx    r11, r12            // S = numb + numa + acy
    adox    r10, r13            // D2 = numb + ~numa + k
    mov     [rsi], r11          // numa = S
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi], r14
    mov     r15, r10

    mov     r10, [rbx+8]
    mov     r11, r10
    mov     r12, [rsi+8]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+8], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+8], r14
    mov     r15, r10

    mov     r10, [rbx+16]
    mov     r11, r10
    mov     r12, [rsi+16]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+16], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+16], r14
    mov     r15, r10

    mov     r10, [rbx+24]
    mov     r11, r10
    mov     r12, [rsi+24]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+24], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+24], r14
    mov     r15, r10

    mov     r10, [rbx+32]
    mov     r11, r10
    mov     r12, [rsi+32]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+32], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+32], r14
    mov     r15, r10

    mov     r10, [rbx+40]
    mov     r11, r10
    mov     r12, [rsi+40]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+40], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+40], r14
    mov     r15, r10

    mov     r10, [rbx+48]
    mov     r11, r10
    mov     r12, [rsi+48]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+48], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+48], r14
    mov     r15, r10

    mov     r10, [rbx+56]
    mov     r11, r10
    mov     r12, [rsi+56]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi+56], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi+56], r14
    mov     r15, r10

    lea     rbx, [rbx+64]
    lea     rsi, [rsi+64]
    lea     rdi, [rdi+64]
    lea     rcx, [rcx-1]
    jrcxz   .Lfft_t2enter
    jmp     .Lfft_g2

.Lfft_t2enter:
    mov     rcx, rax            // rcx = rem2
.p2align 4
.Lfft_t2:
    jrcxz   .Lfft_end2
    mov     r10, [rbx]
    mov     r11, r10
    mov     r12, [rsi]
    mov     r13, r12
    not     r13
    adcx    r11, r12
    adox    r10, r13
    mov     [rsi], r11
    shlx    r12, r10, r8
    shrx    r14, r15, r9
    lea     r14, [r12+r14]
    mov     [rdi], r14
    mov     r15, r10
    lea     rbx, [rbx+8]
    lea     rsi, [rsi+8]
    lea     rdi, [rdi+8]
    lea     rcx, [rcx-1]
    jmp     .Lfft_t2

.Lfft_end2:
    shrx    rax, r15, r9        // shlcy2
    setc    r11b                // acy_f
    setno   r12b                // scy_f
    movzx   r11d, r11b
    movzx   r12d, r12b
    lea     r10, [r11*2+r12]
    mov     [rbp+16], rax       // out[2] = shlcy2
    mov     [rbp+24], r10       // out[3] = (acy<<1)|scy

#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#else
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#endif
    ret
