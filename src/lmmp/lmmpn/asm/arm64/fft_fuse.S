//  Copyright (C) 2026 HJimmyK(Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it under
//  the terms of the GNU Lesser General Public License (LGPL) as published
//  by the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in WITHOUT ANY WARRANTY.
//
//  See <https://www.gnu.org/licenses/>.
//

//
// mp_limb_t lmmp_ifft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb,
//                                mp_size_t l, mp_size_t w, mp_size_t shr);
//
// void lmmp_fft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb, mp_size_t l,
//                          mp_size_t w, mp_size_t shl, mp_limb_t* out);
//
//   蝶形数据面 mega 融合内核（arm64 版，同 x64 fft_fuse.S 的语义）：
//   两段扫描 + 连续进位链单次调用完成，numb 全程只读（移位结果不落地）。
//
//   与 x64 的结构差异：
//   - arm64 无 adcx/adox 双标志链，退回 add_n_sub_n.S 的分块接力模式：
//     块内和链(adcs)与差链(sbcs)各经 C 标志直连，块间经寄存器中转
//     （cset 捕获 → cmp 重建，每链每块 2 周期接力开销）。
//     ifft 用 x8 展开、fft 用 x4 展开（fft 的 c 合成链另占 dtmp/prev 寄存器）。
//   - 位移组合 t = (cur>>shr)|(next<<(64-shr)) 无单指令 extr 可用
//     （移位量为运行时参数，extr 仅接受立即数），用 lsr+lsl+orr 三条
//     （arm64 逻辑指令默认不写标志，无需 x64 的 BMI2 无标志组合舞蹈）。
//   - 段边界无 x64 的"magic add 重建双标志"：进位本就经寄存器接力，
//     段间角色互换 = 两个寄存器交换一次值。
//   - numb 游标不变量：尾 limb 统一用 ldr nxt,[x2,#8]!（指向 CUR），
//     空×8 块、纯尾段、顶 limb（NEXT 天然是 numb[w]/numb[l]）三种
//     路径共享同一循环体，段末游标恰好停在下一段起点。
//
//   进位语义（同 generic/fft_fuse.c）：
//   - ifft: 和链进位/差链借位跨段按输出目标换名（numc 链=bcy, numa 链=acy），
//     段边界互换；ret = (acy<<1)|bcy。
//   - fft: 和链(→numa)跨两段连续；差链与 shl 移位链在段边界重置 0；
//     中点 ch = shlcy1 - (b1<<shl) 分解为 chp/chn，out[2]=shlcy2，
//     out[3] = (acy<<1)|scy。
//

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_ifft_bfy_fused_
//   x0=numc, x1=numa, x2=numb, x3=l, x4=w, x5=shr
//   扫描1(i∈[0,w)):  numc[l-w+i] = numa[l-w+i] + t_i, numa[l-w+i] -= t_i
//   扫描2(i∈[0,l-w)): numc[i] = numa[i] - t'_i,       numa[i] += t'_i
//   t 流水线读 numb[0..w] / numb[w..l]，顶 limb 的 NEXT 天然是段外一字
//   寄存器: x0/x1/x2=游标, x8=shr, x9=64-shr,
//           x10=和链进位, x11=差链借位, x28=CUR/PEEK, x12/x3=临时,
//           x4-x7/x17/x19-x21=a, x13-x16/x22-x27=numb 流(原地化 t)
.p2align 2
.globl ASM_GSYM(lmmp_ifft_bfy_fused_)
ASM_GSYM(lmmp_ifft_bfy_fused_):
    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    stp     x27, x28, [sp, #-16]!
    sub     x25, x3, x4              // n2 = l - w
    lsl     x3, x3, #3
    neg     x3, x3                   // -l*8
    stp     x3, x25, [sp, #-16]!     // [sp]=-l*8, [sp,#8]=n2
    mov     x8, x5                   // shr
    mov     x9, #64
    sub     x9, x9, x5               // 64-shr
    add     x1, x1, x25, lsl #3      // numa 游标 <- base + (l-w)
    add     x0, x0, x25, lsl #3      // numc 游标
    mov     x10, #0                  // 和链进位入
    mov     x11, #0                  // 差链借位入

    cbz     x4, Lifft_bdy            // w == 0 -> 扫描1 空转
    lsr     x24, x4, #3              // 块数 (x8 limb/块)
    and     x25, x4, #7              // 尾 limb 数
    cbz     x24, Lifft_s1pre

.p2align 4
Lifft_s1blk:                         // 扫描1 主循环: 8 limb/迭代（和->numc, 差->numa）
    ldp     x4, x5, [x1], #16
    ldp     x6, x7, [x1], #16
    ldp     x17, x19, [x1], #16
    ldp     x20, x21, [x1], #16
    ldp     x13, x14, [x2], #16
    ldp     x15, x16, [x2], #16
    ldp     x22, x23, [x2], #16
    ldp     x26, x27, [x2], #16
    ldr     x28, [x2]                // peek: t7 的高位源（末块 = numb[w]）

    lsr     x13, x13, x8             // t0 = (n0>>s)|(n1<<rs)
    lsl     x12, x14, x9
    orr     x13, x13, x12
    lsr     x14, x14, x8             // t1
    lsl     x12, x15, x9
    orr     x14, x14, x12
    lsr     x15, x15, x8             // t2
    lsl     x12, x16, x9
    orr     x15, x15, x12
    lsr     x16, x16, x8             // t3
    lsl     x12, x22, x9
    orr     x16, x16, x12
    lsr     x22, x22, x8             // t4
    lsl     x12, x23, x9
    orr     x22, x22, x12
    lsr     x23, x23, x8             // t5
    lsl     x12, x26, x9
    orr     x23, x23, x12
    lsr     x26, x26, x8             // t6
    lsl     x12, x27, x9
    orr     x26, x26, x12
    lsr     x27, x27, x8             // t7
    lsl     x12, x28, x9
    orr     x27, x27, x12

    cmp     x10, #1                  // C = 和链进位入
    adcs    x12, x4, x13
    adcs    x3, x5, x14
    stp     x12, x3, [x0], #16       // 和 -> numc
    adcs    x12, x6, x15
    adcs    x3, x7, x16
    stp     x12, x3, [x0], #16
    adcs    x12, x17, x22
    adcs    x3, x19, x23
    stp     x12, x3, [x0], #16
    adcs    x12, x20, x26
    adcs    x3, x21, x27
    stp     x12, x3, [x0], #16
    cset    x10, cs

    cmp     xzr, x11                 // C = !借位入
    sbcs    x4, x4, x13
    sbcs    x5, x5, x14
    stp     x4, x5, [x1, #-64]       // 差 -> numa（游标已被加载推过）
    sbcs    x6, x6, x15
    sbcs    x7, x7, x16
    stp     x6, x7, [x1, #-48]
    sbcs    x17, x17, x22
    sbcs    x19, x19, x23
    stp     x17, x19, [x1, #-32]
    sbcs    x20, x20, x26
    sbcs    x21, x21, x27
    stp     x20, x21, [x1, #-16]
    cset    x11, cc

    subs    x24, x24, #1
    b.ne    Lifft_s1blk
    cbz     x25, Lifft_bdy           // 游标恰在 &numb[w]
    b       Lifft_s1tail

Lifft_s1pre:                         // 无整块: CUR = numb[0]，x2 指向 CUR
    ldr     x28, [x2]
    mov     x25, x4                  // 全部走尾循环

.p2align 4
Lifft_s1tail:                        // 扫描1 尾 limb（含顶 limb）
    ldr     x13, [x2, #8]!           // NEXT（顶 limb 的 = numb[w]）
    lsr     x14, x28, x8
    lsl     x12, x13, x9
    orr     x14, x14, x12            // t
    mov     x28, x13                 // CUR <- NEXT
    ldr     x4, [x1], #8             // a
    cmp     x10, #1
    adcs    x12, x4, x14
    str     x12, [x0], #8            // 和 -> numc
    cset    x10, cs
    cmp     xzr, x11
    sbcs    x4, x4, x14
    str     x4, [x1, #-8]            // 差 -> numa
    cset    x11, cc
    subs    x25, x25, #1
    b.ne    Lifft_s1tail

Lifft_bdy:                           // 段边界: 游标回基址 + 链角色互换
    ldr     x3, [sp]                 // -l*8
    add     x1, x1, x3               // numa 游标 <- base
    add     x0, x0, x3               // numc 游标 <- base
    mov     x12, x10                 // 交换: 扫描2 和链入 = acy = 差链借位
    mov     x10, x11                 //        扫描2 差链借位入 = bcy = 和链进位
    mov     x11, x12
    ldr     x4, [sp, #8]             // n2 = 扫描2 长度
    cbz     x4, Lifft_ret
    lsr     x24, x4, #3
    and     x25, x4, #7
    cbz     x24, Lifft_s2pre

.p2align 4
Lifft_s2blk:                         // 扫描2 主循环（和->numa, 差->numc，存储目标互换）
    ldp     x4, x5, [x1], #16
    ldp     x6, x7, [x1], #16
    ldp     x17, x19, [x1], #16
    ldp     x20, x21, [x1], #16
    ldp     x13, x14, [x2], #16
    ldp     x15, x16, [x2], #16
    ldp     x22, x23, [x2], #16
    ldp     x26, x27, [x2], #16
    ldr     x28, [x2]                // peek（末块 = numb[l]）

    lsr     x13, x13, x8
    lsl     x12, x14, x9
    orr     x13, x13, x12
    lsr     x14, x14, x8
    lsl     x12, x15, x9
    orr     x14, x14, x12
    lsr     x15, x15, x8
    lsl     x12, x16, x9
    orr     x15, x15, x12
    lsr     x16, x16, x8
    lsl     x12, x22, x9
    orr     x16, x16, x12
    lsr     x22, x22, x8
    lsl     x12, x23, x9
    orr     x22, x22, x12
    lsr     x23, x23, x8
    lsl     x12, x26, x9
    orr     x23, x23, x12
    lsr     x26, x26, x8
    lsl     x12, x27, x9
    orr     x26, x26, x12
    lsr     x27, x27, x8
    lsl     x12, x28, x9
    orr     x27, x27, x12

    cmp     x10, #1
    adcs    x12, x4, x13
    adcs    x3, x5, x14
    stp     x12, x3, [x1, #-64]      // 和 -> numa
    adcs    x12, x6, x15
    adcs    x3, x7, x16
    stp     x12, x3, [x1, #-48]
    adcs    x12, x17, x22
    adcs    x3, x19, x23
    stp     x12, x3, [x1, #-32]
    adcs    x12, x20, x26
    adcs    x3, x21, x27
    stp     x12, x3, [x1, #-16]
    cset    x10, cs

    cmp     xzr, x11
    sbcs    x4, x4, x13
    sbcs    x5, x5, x14
    stp     x4, x5, [x0], #16        // 差 -> numc
    sbcs    x6, x6, x15
    sbcs    x7, x7, x16
    stp     x6, x7, [x0], #16
    sbcs    x17, x17, x22
    sbcs    x19, x19, x23
    stp     x17, x19, [x0], #16
    sbcs    x20, x20, x26
    sbcs    x21, x21, x27
    stp     x20, x21, [x0], #16
    cset    x11, cc

    subs    x24, x24, #1
    b.ne    Lifft_s2blk
    cbz     x25, Lifft_ret
    b       Lifft_s2tail

Lifft_s2pre:
    ldr     x28, [x2]                // CUR = numb[w]
    mov     x25, x4

.p2align 4
Lifft_s2tail:
    ldr     x13, [x2, #8]!           // NEXT（顶 limb 的 = numb[l]）
    lsr     x14, x28, x8
    lsl     x12, x13, x9
    orr     x14, x14, x12
    mov     x28, x13
    ldr     x4, [x1], #8
    cmp     x10, #1
    adcs    x12, x4, x14
    str     x12, [x1, #-8]           // 和 -> numa
    cset    x10, cs
    cmp     xzr, x11
    sbcs    x4, x4, x14
    str     x4, [x0], #8             // 差 -> numc
    cset    x11, cc
    subs    x25, x25, #1
    b.ne    Lifft_s2tail

Lifft_ret:                           // ret = (acy<<1)|bcy（扫描2 终值或空段换名值）
    add     x0, x11, x10, lsl #1
    ldp     x3, x25, [sp], #16
    ldp     x27, x28, [sp], #16
    ldp     x25, x26, [sp], #16
    ldp     x23, x24, [sp], #16
    ldp     x21, x22, [sp], #16
    ldp     x19, x20, [sp], #16
    ret

// ======================================
// lmmp_fft_bfy_fused_
//   x0=numc, x1=numa, x2=numb, x3=l, x4=w, x5=shl, x6=out
//   扫描1(j∈[0,l-w)): numc[w+j] = (numa[j]-numb[j])<<shl 接链, numa[j] += numb[j]
//   中点: ch = shlcy1 - (b1<<shl) -> chp/chn；差链/移位链重置
//   扫描2(j∈[0,w)): numc[j] = (numb[l-w+j]-numa[l-w+j])<<shl 接链,
//                    numa[l-w+j] += numb[l-w+j]（和链续）
//   寄存器: x0/x1/x2=游标, x8=shl, x9=64-shl, x10=和链进位(跨段),
//           x11=差链借位, x12=prev(移位链), x21=dtmp, x3=合成临时,
//           x4-x7=x 流, x13-x16=y 流, x19/x20=c 对
.p2align 2
.globl ASM_GSYM(lmmp_fft_bfy_fused_)
ASM_GSYM(lmmp_fft_bfy_fused_):
    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    sub     x25, x3, x4              // n1 = l - w
    lsl     x3, x3, #3
    neg     x3, x3                   // -l*8
    stp     x3, x6, [sp, #-32]!      // [sp]=-l*8, [sp,#8]=out
    str     x4, [sp, #16]            // [sp,#16]=w（扫描2 长度）
    mov     x8, x5                   // shl
    mov     x9, #64
    sub     x9, x9, x5               // 64-shl
    mov     x10, #0                  // 和链进位
    mov     x11, #0                  // 差链借位
    mov     x12, #0                  // prev（移位链）
    add     x0, x0, x4, lsl #3       // numc 游标 <- base + w

    cbz     x25, Lfft_mid
    lsr     x24, x25, #2             // 块数 (x4 limb/块)
    and     x25, x25, #3             // 尾 limb 数
    cbz     x24, Lfft_s1tail

.p2align 4
Lfft_s1blk:                          // 扫描1 主循环（x=numa, y=numb）
    ldp     x4, x5, [x1], #16
    ldp     x6, x7, [x1], #16
    ldp     x13, x14, [x2], #16
    ldp     x15, x16, [x2], #16
    cmp     xzr, x11                 // C = !借位入
    sbcs    x21, x4, x13             // d0
    lsl     x19, x21, x8
    lsr     x3, x12, x9
    orr     x19, x19, x3             // c0 = (d0<<shl)|(prev>>rshl)
    mov     x12, x21                 // prev = d0
    sbcs    x21, x5, x14             // d1
    lsl     x20, x21, x8
    lsr     x3, x12, x9
    orr     x20, x20, x3             // c1 = (d1<<shl)|(d0>>rshl)
    mov     x12, x21
    stp     x19, x20, [x0], #16      // c -> numc
    sbcs    x21, x6, x15             // d2
    lsl     x19, x21, x8
    lsr     x3, x12, x9
    orr     x19, x19, x3
    mov     x12, x21
    sbcs    x21, x7, x16             // d3
    lsl     x20, x21, x8
    lsr     x3, x12, x9
    orr     x20, x20, x3
    mov     x12, x21
    stp     x19, x20, [x0], #16
    cset    x11, cc

    cmp     x10, #1                  // C = 和链进位入
    adcs    x4, x4, x13
    adcs    x5, x5, x14
    stp     x4, x5, [x1, #-32]       // 和 -> numa（游标已被加载推过）
    adcs    x6, x6, x15
    adcs    x7, x7, x16
    stp     x6, x7, [x1, #-16]
    cset    x10, cs

    subs    x24, x24, #1
    b.ne    Lfft_s1blk
    cbz     x25, Lfft_mid

.p2align 4
Lfft_s1tail:                         // 扫描1 尾 limb（len<4 时直接进入, x25=len&3=len）
    ldr     x4, [x1], #8             // x = numa[j]
    ldr     x13, [x2], #8            // y = numb[j]
    cmp     xzr, x11
    sbcs    x21, x4, x13             // d
    lsl     x19, x21, x8
    lsr     x3, x12, x9
    orr     x19, x19, x3
    mov     x12, x21
    str     x19, [x0], #8            // c -> numc
    cset    x11, cc
    cmp     x10, #1
    adcs    x4, x4, x13              // s
    str     x4, [x1, #-8]            // s -> numa
    cset    x10, cs
    subs    x25, x25, #1
    b.ne    Lfft_s1tail

Lfft_mid:                            // 中点 ch 分解 + 链重置
    ldr     x6, [sp, #8]             // out
    lsr     x13, x12, x9             // shlcy1 = prev>>rshl
    lsl     x14, x11, x8             // adj = b<<shl
    subs    x15, x13, x14
    csel    x15, x15, xzr, hs        // chp
    subs    x16, x14, x13
    csel    x16, x16, xzr, hi        // chn（adj > shlcy 时取 adj-shlcy）
    stp     x15, x16, [x6]           // out[0]=chp, out[1]=chn
    mov     x11, #0                  // 差链重置
    mov     x12, #0                  // 移位链重置
    ldr     x3, [sp]                 // -l*8
    add     x0, x0, x3               // numc 游标 <- base（numa/numb 无缝衔接）
    ldr     x25, [sp, #16]           // w = 扫描2 长度
    cbz     x25, Lfft_end
    lsr     x24, x25, #2
    and     x25, x25, #3
    cbz     x24, Lfft_s2tail

.p2align 4
Lfft_s2blk:                          // 扫描2 主循环（x=numb, y=numa）
    ldp     x4, x5, [x2], #16
    ldp     x6, x7, [x2], #16
    ldp     x13, x14, [x1], #16
    ldp     x15, x16, [x1], #16
    cmp     xzr, x11
    sbcs    x21, x4, x13             // d = numb - numa
    lsl     x19, x21, x8
    lsr     x3, x12, x9
    orr     x19, x19, x3
    mov     x12, x21
    sbcs    x21, x5, x14
    lsl     x20, x21, x8
    lsr     x3, x12, x9
    orr     x20, x20, x3
    mov     x12, x21
    stp     x19, x20, [x0], #16
    sbcs    x21, x6, x15
    lsl     x19, x21, x8
    lsr     x3, x12, x9
    orr     x19, x19, x3
    mov     x12, x21
    sbcs    x21, x7, x16
    lsl     x20, x21, x8
    lsr     x3, x12, x9
    orr     x20, x20, x3
    mov     x12, x21
    stp     x19, x20, [x0], #16
    cset    x11, cc

    cmp     x10, #1
    adcs    x13, x13, x4
    adcs    x14, x14, x5
    stp     x13, x14, [x1, #-32]     // 和 -> numa
    adcs    x15, x15, x6
    adcs    x16, x16, x7
    stp     x15, x16, [x1, #-16]
    cset    x10, cs

    subs    x24, x24, #1
    b.ne    Lfft_s2blk
    cbz     x25, Lfft_end

.p2align 4
Lfft_s2tail:
    ldr     x4, [x2], #8             // x = numb[l-w+j]
    ldr     x13, [x1], #8            // y = numa[l-w+j]
    cmp     xzr, x11
    sbcs    x21, x4, x13
    lsl     x19, x21, x8
    lsr     x3, x12, x9
    orr     x19, x19, x3
    mov     x12, x21
    str     x19, [x0], #8
    cset    x11, cc
    cmp     x10, #1
    adcs    x13, x13, x4
    str     x13, [x1, #-8]
    cset    x10, cs
    subs    x25, x25, #1
    b.ne    Lfft_s2tail

Lfft_end:
    ldr     x6, [sp, #8]             // out
    lsr     x13, x12, x9             // out[2] = shlcy2 = prev>>rshl
    str     x13, [x6, #16]
    add     x13, x11, x10, lsl #1    // out[3] = (acy<<1)|scy
    str     x13, [x6, #24]
    ldp     x3, x6, [sp], #32
    ldp     x25, x26, [sp], #16
    ldp     x23, x24, [sp], #16
    ldp     x21, x22, [sp], #16
    ldp     x19, x20, [sp], #16
    ret
