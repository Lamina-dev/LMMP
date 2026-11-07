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
// mp_limb_t lmmp_ifft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb,
//                                mp_size_t l, mp_size_t w, mp_size_t shr);
//
// void lmmp_fft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb, mp_size_t l,
//                          mp_size_t w, mp_size_t shl, mp_limb_t* out);

//
//   ifft: numc=x0, numa=x1, numb=x2, l=x3, w=x4, shr=x5
//   fft:  numc=x0, numa=x1, numb=x2, l=x3, w=x4, shl=x5, out=x6

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_ifft_bfy_fused_
// ======================================
// x0/x1/x2=cursor, x8=shr, x9=64-shr, x10=sum carry, x11=diff borrow,
// x28=CUR/PEEK, x12/x3=tmp, x4-x7/x17/x19-x21=a, x13-x16/x22-x27=numb
.globl ASM_GSYM(lmmp_ifft_bfy_fused_)
.p2align 2
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
    add     x1, x1, x25, lsl #3      // x1 <- base + (l-w)
    add     x0, x0, x25, lsl #3      // x0 cursor
    mov     x10, #0                  // sum carry in
    mov     x11, #0                  // diff borrow in

    cbz     x4, Lifft_bdy            // w == 0: scan1 empty
    lsr     x24, x4, #3              // groups (8 limbs)
    and     x25, x4, #7              // tail limbs
    cbz     x24, Lifft_s1pre

.p2align 4
Lifft_s1blk:                         // scan1 main loop, 8 limbs
    ldp     x4, x5, [x1], #16
    ldp     x6, x7, [x1], #16
    ldp     x17, x19, [x1], #16
    ldp     x20, x21, [x1], #16
    ldp     x13, x14, [x2], #16
    ldp     x15, x16, [x2], #16
    ldp     x22, x23, [x2], #16
    ldp     x26, x27, [x2], #16
    ldr     x28, [x2]                // peek (last block: numb[w])

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

    cmp     x10, #1                  // C = sum carry in
    adcs    x12, x4, x13
    adcs    x3, x5, x14
    stp     x12, x3, [x0], #16       // sum -> numc
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

    cmp     xzr, x11                 // C = !borrow in
    sbcs    x4, x4, x13
    sbcs    x5, x5, x14
    stp     x4, x5, [x1, #-64]       // diff -> numa
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
    cbz     x25, Lifft_bdy           // cursor at &numb[w]
    b       Lifft_s1tail

Lifft_s1pre:                         // no full block: CUR = numb[0]
    ldr     x28, [x2]
    mov     x25, x4                  // all limbs -> tail loop

.p2align 4
Lifft_s1tail:                        // scan1 tail (incl. top limb)
    ldr     x13, [x2, #8]!           // NEXT (top: numb[w])
    lsr     x14, x28, x8
    lsl     x12, x13, x9
    orr     x14, x14, x12            // t
    mov     x28, x13                 // CUR <- NEXT
    ldr     x4, [x1], #8             // a
    cmp     x10, #1
    adcs    x12, x4, x14
    str     x12, [x0], #8            // sum -> numc
    cset    x10, cs
    cmp     xzr, x11
    sbcs    x4, x4, x14
    str     x4, [x1, #-8]            // diff -> numa
    cset    x11, cc
    subs    x25, x25, #1
    b.ne    Lifft_s1tail

Lifft_bdy:                           // segment boundary: swap chains
    ldr     x3, [sp]                 // -l*8
    add     x1, x1, x3               // x1 <- base
    add     x0, x0, x3               // x0 <- base
    mov     x12, x10                 // swap: sum carry in = diff borrow
    mov     x10, x11                 //       diff borrow in = sum carry
    mov     x11, x12
    ldr     x4, [sp, #8]             // n2 = scan2 len
    cbz     x4, Lifft_ret
    lsr     x24, x4, #3
    and     x25, x4, #7
    cbz     x24, Lifft_s2pre

.p2align 4
Lifft_s2blk:                         // scan2 main loop, 8 limbs
    ldp     x4, x5, [x1], #16
    ldp     x6, x7, [x1], #16
    ldp     x17, x19, [x1], #16
    ldp     x20, x21, [x1], #16
    ldp     x13, x14, [x2], #16
    ldp     x15, x16, [x2], #16
    ldp     x22, x23, [x2], #16
    ldp     x26, x27, [x2], #16
    ldr     x28, [x2]                // peek (last block: numb[l])

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
    stp     x12, x3, [x1, #-64]      // sum -> numa
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
    stp     x4, x5, [x0], #16        // diff -> numc
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
    ldr     x13, [x2, #8]!           // NEXT (top: numb[l])
    lsr     x14, x28, x8
    lsl     x12, x13, x9
    orr     x14, x14, x12
    mov     x28, x13
    ldr     x4, [x1], #8
    cmp     x10, #1
    adcs    x12, x4, x14
    str     x12, [x1, #-8]           // sum -> numa
    cset    x10, cs
    cmp     xzr, x11
    sbcs    x4, x4, x14
    str     x4, [x0], #8             // diff -> numc
    cset    x11, cc
    subs    x25, x25, #1
    b.ne    Lifft_s2tail

Lifft_ret:                           // ret = (acy<<1)|bcy
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
// ======================================
// x0/x1/x2=cursor, x8=shl, x9=64-shl, x10=sum carry, x11=diff borrow,
// x12=prev(shift chain), x21=dtmp, x3=tmp, x4-x7=x stream,
// x13-x16=y stream, x19/x20=c pair
.globl ASM_GSYM(lmmp_fft_bfy_fused_)
.p2align 2
ASM_GSYM(lmmp_fft_bfy_fused_):
    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    sub     x25, x3, x4              // n1 = l - w
    lsl     x3, x3, #3
    neg     x3, x3                   // -l*8
    stp     x3, x6, [sp, #-32]!      // [sp]=-l*8, [sp,#8]=out
    str     x4, [sp, #16]            // [sp,#16]=w
    mov     x8, x5                   // shl
    mov     x9, #64
    sub     x9, x9, x5               // 64-shl
    mov     x10, #0                  // sum carry
    mov     x11, #0                  // diff borrow
    mov     x12, #0                  // prev
    add     x0, x0, x4, lsl #3       // x0 <- base + w

    cbz     x25, Lfft_mid
    lsr     x24, x25, #2             // groups (4 limbs)
    and     x25, x25, #3             // tail limbs
    cbz     x24, Lfft_s1tail

.p2align 4
Lfft_s1blk:                          // scan1 main loop (x=numa, y=numb)
    ldp     x4, x5, [x1], #16
    ldp     x6, x7, [x1], #16
    ldp     x13, x14, [x2], #16
    ldp     x15, x16, [x2], #16
    cmp     xzr, x11                 // C = !borrow in
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

    cmp     x10, #1                  // C = sum carry in
    adcs    x4, x4, x13
    adcs    x5, x5, x14
    stp     x4, x5, [x1, #-32]       // sum -> numa
    adcs    x6, x6, x15
    adcs    x7, x7, x16
    stp     x6, x7, [x1, #-16]
    cset    x10, cs

    subs    x24, x24, #1
    b.ne    Lfft_s1blk
    cbz     x25, Lfft_mid

.p2align 4
Lfft_s1tail:                         // scan1 tail limbs
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

Lfft_mid:                            // mid: split ch, reset chains
    ldr     x6, [sp, #8]             // out
    lsr     x13, x12, x9             // shlcy1 = prev>>rshl
    lsl     x14, x11, x8             // adj = b<<shl
    subs    x15, x13, x14
    csel    x15, x15, xzr, hs        // chp
    subs    x16, x14, x13
    csel    x16, x16, xzr, hi        // chn
    stp     x15, x16, [x6]           // out[0]=chp, out[1]=chn
    mov     x11, #0                  // diff chain reset
    mov     x12, #0                  // shift chain reset
    ldr     x3, [sp]                 // -l*8
    add     x0, x0, x3               // x0 <- base
    ldr     x25, [sp, #16]           // w = scan2 len
    cbz     x25, Lfft_end
    lsr     x24, x25, #2
    and     x25, x25, #3
    cbz     x24, Lfft_s2tail

.p2align 4
Lfft_s2blk:                          // scan2 main loop (x=numb, y=numa)
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
    stp     x13, x14, [x1, #-32]     // sum -> numa
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
