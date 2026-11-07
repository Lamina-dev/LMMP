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
//   ifft: Windows: numc=rcx, numa=rdx, numb=r8, l=r9, w=[rsp+40], shr=[rsp+48]
//         SysV:    numc=rdi, numa=rsi, numb=rdx, l=rcx, w=r8, shr=r9
//   fft:  Windows: numc=rcx, numa=rdx, numb=r8, l=r9, w=[rsp+40], shl=[rsp+48],
//                 out=[rsp+56]
//         SysV:    numc=rdi, numa=rsi, numb=rdx, l=rcx, w=r8, shl=r9, out=[rsp+8]
//

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_ifft_bfy_fused_
// ======================================
// rdi=numc, rsi=numa, rbx=numb, rcx=groups, r8=shr, r9=64-shr,
// r10/r11=CUR/NEXT, rbp=T1/~t, r12=T2, r13=t, r14=sum(adcx), r15=diff(adox)
.globl ASM_GSYM(lmmp_ifft_bfy_fused_)
.p2align 4
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
    // rcx = l, rdi/rsi ready
#endif
    mov     r11, rcx
    sub     r11, rax            // r11 = n2 = l - w
    mov     r8, r10             // r8 = shr (shrx cnt)
    mov     r9d, 64
    sub     r9, r10             // r9 = 64-shr (shlx cnt)
    push    r11                 // save n2
    push    rcx                 // save l
    lea     rsi, [rsi + r11*8]  // rsi <- base + (l-w)
    lea     rdi, [rdi + r11*8]  // rdi cursor

    test    rax, rax            // w == 0: scan1 empty
    jz      .Lifft_s1empty

    lea     rcx, [rax-1]        // m1 = w-1
    mov     rax, rcx
    and     eax, 7              // rem1
    shr     rcx, 3              // groups1
    mov     r10, 0x4000000000000000
    add     r10, r10            // CF = 0, OF = 1
    mov     r10, [rbx]          // CUR = numb[0]
    jrcxz   .Lifft_z1
    jmp     .Lifft_g1

.p2align 4
.Lifft_z1:                      // scan1 zero-group trampoline
    jmp     .Lifft_t1enter
.Lifft_g1:                      // scan1 main loop, 8 limbs
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
    mov     [rdi], r14          // sum -> numc
    mov     [rsi], r15          // diff -> numa

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

.Lifft_top1:                    // scan1 top limb
    shrx    rbp, r10, r8
    mov     rax, [rbx+8]
    shlx    rax, rax, r9
    lea     rbx, [rbx+8]        // rbx <- &numb[w]
    lea     r13, [rbp+rax]
    mov     r14, [rsi]
    mov     r15, r14
    mov     rbp, r13
    not     rbp
    adcx    r14, r13
    adox    r15, rbp
    mov     [rdi], r14
    mov     [rsi], r15
    lea     rsi, [rsi+8]        // rsi <- base+l
    lea     rdi, [rdi+8]
    setc    al                  // bcy_f
    setno   cl                  // acy_f
    jmp     .Lifft_bdy

.Lifft_s1empty:                 // w == 0: chain ends 0/0
    xor     eax, eax
    xor     ecx, ecx

.Lifft_bdy:                     // segment boundary
    movzx   eax, al
    movzx   ecx, cl
    pop     rdx                 // l
    pop     r11                 // n2
    neg     rdx
    lea     rsi, [rsi + rdx*8]  // rsi <- base
    lea     rdi, [rdi + rdx*8]  // rdi <- base
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
    add     r13, r13            // CF = acy, OF = 1-bcy
    mov     rcx, r10            // groups2
    mov     r10, [rbx]          // CUR = numb[w]
    mov     rax, r12            // rem2
    jrcxz   .Lifft_z2
    jmp     .Lifft_g2

.p2align 4
.Lifft_z2:                      // scan2 zero-group trampoline
    jmp     .Lifft_t2enter
.Lifft_g2:                      // scan2 main loop, 8 limbs
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
    mov     [rsi], r14          // sum -> numa
    mov     [rdi], r15          // diff -> numc

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

.Lifft_top2:                    // scan2 top limb
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
    mov     [rsi], r14          // sum -> numa
    mov     [rdi], r15          // diff -> numc
    // scan2: CF = acy (sum -> numa), OF = bcy (diff -> numc)
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

.Lifft_ret:                     // scan2 empty: pack scan1 values
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
// ======================================
// rdi=numc, rsi=numa, rbx=numb, rbp=out, rcx=groups, r8=shl, r9=64-shl,
// r10=X/D, r11=XS/sum, r12=Y/T1, r13=~Y, r14=OUT, r15=PD
.globl ASM_GSYM(lmmp_fft_bfy_fused_)
.p2align 4
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
    mov     rax, [rsp+56]       // out (after 6 pushes)
    mov     rbp, rax
    // rdi/rsi/rcx ready (numc/numa/l)
#endif
    mov     r8, r10             // r8 = shl (shlx cnt)
    mov     r9d, 64
    sub     r9, r10             // r9 = 64-shl (shrx cnt)
    lea     rdi, [rdi + r11*8]  // rdi <- base + w
    mov     rdx, rcx
    sub     rdx, r11            // n1 = l - w
    push    r11                 // save w
    push    rcx                 // save l

    mov     r15d, 0             // PD = 0
    test    rdx, rdx
    jz      .Lfft_s1empty

    mov     rax, rdx
    and     eax, 7              // rem1
    shr     rdx, 3              // groups1
    mov     r10, 0x4000000000000000
    add     r10, r10            // CF = 0, OF = 1
    mov     rcx, rdx            // groups1
    jrcxz   .Lfft_z1
    jmp     .Lfft_g1

.p2align 4
.Lfft_z1:                       // scan1 zero-group trampoline
    jmp     .Lfft_t1enter
.Lfft_g1:                       // scan1 main loop, 8 limbs
    mov     r10, [rsi]
    mov     r11, r10
    mov     r12, [rbx]
    mov     r13, r12
    not     r13
    adcx    r11, r12            // S = X + Y + CF
    adox    r10, r13            // D = X + ~Y + OF
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

.Lfft_end1:                     // scan1 done: extract chain ends
    shrx    rax, r15, r9        // shlcy1
    setc    r11b                // acy1
    setno   r12b                // scy1
    movzx   r11d, r11b
    movzx   r12d, r12b
    jmp     .Lfft_mid

.Lfft_s1empty:                  // l == w: chain ends 0
    xor     r11d, r11d
    xor     r12d, r12d
    xor     eax, eax

.Lfft_mid:                      // ch = shlcy1 - (scy1<<shl)
    shlx    r12, r12, r8        // adj
    sub     rax, r12            // CF = (shlcy1 < adj)
    mov     r13, rax
    mov     r14d, 0             // OUT = 0
    jnc     1f
    neg     r13
    mov     r14, r13            // chn
    mov     r13d, 0             // chp = 0
1:
    mov     [rbp], r13          // out[0] = chp
    mov     [rbp+8], r14        // out[1] = chn
    pop     r10                 // l
    pop     rdx                 // w (n2)
    neg     r10
    lea     rdi, [rdi + r10*8]  // rdi <- base
    test    rdx, rdx
    jz      .Lfft_ret2empty

    mov     rax, rdx
    and     eax, 7              // rem2
    shr     rdx, 3              // groups2
    mov     r15d, 0             // PD = 0
    mov     r13, r11
    xor     r13d, 1             // bit62 = acy1 ^ 1
    shl     r13, 62
    shl     r11, 63             // bit63 = acy1
    or      r13, r11
    add     r13, r13            // CF = acy1, OF = 1
    mov     rcx, rdx            // groups2
    jrcxz   .Lfft_z2
    jmp     .Lfft_g2

.Lfft_ret2empty:                // w == 0: out[2]=0, out[3]=acy1<<1
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
.Lfft_z2:                       // scan2 zero-group trampoline
    jmp     .Lfft_t2enter
.Lfft_g2:                       // scan2 main loop, 8 limbs
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
