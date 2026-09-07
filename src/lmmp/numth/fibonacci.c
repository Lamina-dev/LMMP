/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/*
F[n] 表示 Fibonacci 数列第 n 项（F[0]=0, F[1]=1, F[n]=F[n-1]+F[n-2]），
并假定了 F[-1] = 1（满足 F[1] = F[0] + F[-1]）。

    F[2k-1] = F[k]^2 + F[k-1]^2
    F[2k+1] = 4*F[k]^2 - F[k-1]^2 + 2*(-1)^k
    F[2k]   = F[2k+1] - F[2k-1] = F[k] * (F[k] + 2*F[k-1])
    F[2k+1] = (2*F[k] + F[k-1]) * (2*F[k] - F[k-1]) + 2*(-1)^k

fib2_core_:

设 n 的二进制为 b_{m}...b_0。从查表得到的起点对
(fp, f1p) = (F[k], F[k-1])（k 即 n 的最高若干位，k <= FIB_TABLE_LIMIT）
出发，自高位向低位逐位扫描：每处理一位，先由前两组恒等式算出
(F[2k+1], F[2k-1])（各需一次平方，共 2 次平方），再按当前位是 0/1 用
F[2k] = F[2k+1] - F[2k-1] 替换掉不需要的那个，把数对推进为
(F[2k], F[2k-1]) 或 (F[2k+1], F[2k])。终态即 (F[n], F[n-1])。

F[2k+1] 中的 2*(-1)^k 修正不需要进位传播：
  - k 奇（-2）：fp 此时恰为 F[k-1]^2。平方数 mod 4 in {0,1}，故其
    bit1 必为 0，fp[0] |= 2 等价于 fp[0] += 2，即给被减数加 2，
    相当于给结果减 2；
  - k 偶（+2）：tp 左移 2 位后低 2 位为 0，tp[0] |= 2 等价于
    tp[0] += 2，直接给 4*F[k]^2 加 2。

lmmp_fibonacci_:

仅求 F[n] 时不必算出整个数对：先以 fibonacci2 计算 k = n/2 处的
(F[k], F[k-1])，再用第四组恒等式一次乘法合并：
  - n 偶：F[n] = F[k] * (F[k] + 2*F[k-1])，1 次乘法；
  - n 奇：F[n] = (2*F[k]+F[k-1]) * (2*F[k]-F[k-1]) + 2*(-1)^(n/2)，
    1 次乘法。
合并乘积直接写入 dst，两操作数长度之和 2*size+2（size 为 F[k] 的
limb 长度）决定了 lmmp_fibonacci_size_ 的取值。

合并处的 ±2 修正在低 limb 上直接完成：
  - n/2 奇（k 奇，-2）：乘积 = F[n] + 2。F[4t+3] mod 8 in {1,2,5}
    （F mod 8 以 12 为周期：0,1,1,2,3,5,0,5,5,2,7,1），不会为 6,7，
    故乘积低 limb >= 2，减 2 绝不借位；
  - n/2 偶（k 偶，+2）：乘积 = F[n] - 2，低 limb 加回 2。GMP 的经验
    结论：F[4t+1] 在 n < 3*2^64+1 之内 mod 2^64 恒不为 0,1（已验证
    至 b=32 且模式规整），故加 2 绝不进位。此处经由 lmmp_add_1_ 做
    完整进位传播，即使该结论有误也只是多花 O(1) 时间，结果仍然正确。

同时覆盖两个函数的最大写入范围（设 size = limbs(F[n>>1])）：
  - fibonacci2_ 倍增循环最后一轮：2*size+1（平方写 2*size 个 limb，
    进位落盘至下标 2*size）；
  - fibonacci_ 合并乘法：xsize+ysize <= 2*size+2。
n <= FIB_TABLE_LIMIT 时仅写 1 个 limb。
*/

#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

#define FIB_TABLE_LIMIT 93

/* log2(phi) = 0.6942419136306174... */
#define FIB_LOG2PHI_FP (0xb1b9d68a8e53425eull)

static const mp_limb_t fib_table[FIB_TABLE_LIMIT + 2] = {
    0x0000000000000001ull, 0x0000000000000000ull, 0x0000000000000001ull, 0x0000000000000001ull,
    0x0000000000000002ull, 0x0000000000000003ull, 0x0000000000000005ull, 0x0000000000000008ull,
    0x000000000000000dull, 0x0000000000000015ull, 0x0000000000000022ull, 0x0000000000000037ull,
    0x0000000000000059ull, 0x0000000000000090ull, 0x00000000000000e9ull, 0x0000000000000179ull,
    0x0000000000000262ull, 0x00000000000003dbull, 0x000000000000063dull, 0x0000000000000a18ull,
    0x0000000000001055ull, 0x0000000000001a6dull, 0x0000000000002ac2ull, 0x000000000000452full,
    0x0000000000006ff1ull, 0x000000000000b520ull, 0x0000000000012511ull, 0x000000000001da31ull,
    0x000000000002ff42ull, 0x000000000004d973ull, 0x000000000007d8b5ull, 0x00000000000cb228ull,
    0x0000000000148addull, 0x0000000000213d05ull, 0x000000000035c7e2ull, 0x00000000005704e7ull,
    0x00000000008cccc9ull, 0x0000000000e3d1b0ull, 0x0000000001709e79ull, 0x0000000002547029ull,
    0x0000000003c50ea2ull, 0x0000000006197ecbull, 0x0000000009de8d6dull, 0x000000000ff80c38ull,
    0x0000000019d699a5ull, 0x0000000029cea5ddull, 0x0000000043a53f82ull, 0x000000006d73e55full,
    0x00000000b11924e1ull, 0x000000011e8d0a40ull, 0x00000001cfa62f21ull, 0x00000002ee333961ull,
    0x00000004bdd96882ull, 0x00000007ac0ca1e3ull, 0x0000000c69e60a65ull, 0x0000001415f2ac48ull,
    0x000000207fd8b6adull, 0x0000003495cb62f5ull, 0x0000005515a419a2ull, 0x00000089ab6f7c97ull,
    0x000000dec1139639ull, 0x000001686c8312d0ull, 0x000002472d96a909ull, 0x000003af9a19bbd9ull,
    0x000005f6c7b064e2ull, 0x000009a661ca20bbull, 0x00000f9d297a859dull, 0x000019438b44a658ull,
    0x000028e0b4bf2bf5ull, 0x000042244003d24dull, 0x00006b04f4c2fe42ull, 0x0000ad2934c6d08full,
    0x0001182e2989ced1ull, 0x0001c5575e509f60ull, 0x0002dd8587da6e31ull, 0x0004a2dce62b0d91ull,
    0x000780626e057bc2ull, 0x000c233f54308953ull, 0x0013a3a1c2360515ull, 0x001fc6e116668e68ull,
    0x00336a82d89c937dull, 0x00533163ef0321e5ull, 0x00869be6c79fb562ull, 0x00d9cd4ab6a2d747ull,
    0x016069317e428ca9ull, 0x023a367c34e563f0ull, 0x039a9fadb327f099ull, 0x05d4d629e80d5489ull,
    0x096f75d79b354522ull, 0x0f444c01834299abull, 0x18b3c1d91e77decdull, 0x27f80ddaa1ba7878ull,
    0x40abcfb3c0325745ull, 0x68a3dd8e61eccfbdull, 0xa94fad42221f2702ull,
};

/**
 * @brief 计算 F[m] 的 limb 长度紧上界
 * @param m Fibonacci 数列的下标
 * @warning m>=0
 * @return limbs(F[m]) <= ret，至多多算 1 个 limb
 */
static inline mp_size_t fib_limbs_bound_(ulong m) {
    return (mp_size_t)(lmmp_mulh_(m, FIB_LOG2PHI_FP) / LIMB_BITS) + 1;
}

mp_size_t lmmp_fibonacci_size_(ulong n) {
    if (n <= FIB_TABLE_LIMIT) {
        return 1;
    }
    return 2 * (fib_limbs_bound_(n >> 1) + 1) + 2;
}

/**
 * @brief Fibonacci 数对倍增核心：fp <- F[n]，f1p <- F[n-1]
 * @param fp F[n] 输出指针
 * @param f1p F[n-1] 输出指针
 * @param tp 临时空间（alloc 个limb）
 * @param alloc fp、f1p、tp 的公共容量（limb数）
 * @param n 下标
 * @warning fp!=NULL, f1p!=NULL, tp!=NULL, sep(fp,f1p,tp),
 *          alloc >= 2*(limbs(F[n>>1])+1)+1
 * @return F[n] 的实际 limb 长度（n==0 时 fp[0]==0）；
 *         F[n-1] 至多多写一个高位0，实际长度为 ret-(f1p[ret-1]==0)
 */
static mp_size_t fib2_core_(mp_ptr fp, mp_ptr f1p, mp_ptr tp, mp_size_t alloc, ulong n) {
    ulong nfirst, mask;

    /* 自顶截去低位直至落入查表范围，mask 对齐到 n 的剩余最高位 */
    mask = 1;
    for (nfirst = n; nfirst > FIB_TABLE_LIMIT; nfirst >>= 1) {
        mask <<= 1;
    }

    f1p[0] = fib_table[nfirst];    /* F[nfirst-1] */
    fp[0] = fib_table[nfirst + 1]; /* F[nfirst] */
    mp_size_t size = 1;

    if (mask == 1) {
        return size;
    }

    do {
        /* 此处 fp == F[k]，f1p == F[k-1]，k 为 n 自 mask 位向上的位串；
           fp 归一化，f1p 至多一个高位0 */
        lmmp_debug_assert(alloc >= 2 * size);
        lmmp_sqr_(tp, fp, size);  /* tp = F[k]^2   */
        lmmp_sqr_(fp, f1p, size); /* fp = F[k-1]^2 */
        size <<= 1;
        size -= (tp[size - 1] == 0); /* fp 归一化故 tp 至多一个高位0 */

        /* F[2k-1] = F[k]^2 + F[k-1]^2 */
        f1p[size] = lmmp_add_n_(f1p, tp, fp, size);

        /* F[2k+1] = 4*F[k]^2 - F[k-1]^2 + 2*(-1)^k，n&mask 即 k 的最低位 */
        lmmp_debug_assert((fp[0] & 2) == 0); /* fp 为平方数，bit1 必为0 */
        fp[0] |= (n & mask ? 2 : 0);         /* k 奇：给被减数加2即结果减2 */
        mp_limb_t c = lmmp_shl_(tp, tp, size, 2); /* tp = 4*F[k]^2（溢出位入c）*/
        tp[0] |= (n & mask ? 0 : 2);         /* k 偶：4*F[k]^2 加2 */
        fp[size] = c - lmmp_sub_n_(fp, tp, fp, size);
        lmmp_debug_assert(alloc >= size + 1);
        size += (fp[size] != 0);

        /* 下一位 n&(mask>>1)：为1则推进到 (F[2k+1], F[2k])，
           为0则推进到 (F[2k], F[2k-1])，F[2k] = F[2k+1] - F[2k-1] */
        mask >>= 1;
        if (n & mask) {
            lmmp_sub_n_(f1p, fp, f1p, size);
        } else {
            lmmp_sub_n_(fp, fp, f1p, size);
            size -= (fp[size - 1] == 0);
        }
    } while (mask != 1);

    return size;
}

mp_size_t lmmp_fibonacci2_(mp_ptr dst, mp_ptr dst2, mp_size_t rn, ulong n) {
    lmmp_param_assert(dst != NULL && dst2 != NULL);
#if LMMP_DEBUG_PARAM_ASSERT_CHECK == 1
    mp_size_t need = lmmp_fibonacci_size_(n);
    lmmp_param_assert(rn >= need);
#endif
    if (n <= FIB_TABLE_LIMIT) {
        dst[0] = fib_table[n + 1]; /* F[n] */
        dst2[0] = fib_table[n];    /* F[n-1]，n==0 时即 F[-1]=1 */
        return 1;
    }
    TEMP_DECL;
    mp_ptr tp = TALLOC_TYPE(rn, mp_limb_t);
    mp_size_t size = fib2_core_(dst, dst2, tp, rn, n);
    TEMP_FREE;
    return size;
}

mp_size_t lmmp_fibonacci_(mp_ptr dst, mp_size_t rn, ulong n) {
    lmmp_param_assert(dst != NULL);
#if LMMP_DEBUG_PARAM_ASSERT_CHECK == 1
    mp_size_t need = lmmp_fibonacci_size_(n);
    lmmp_param_assert(rn >= need);
#else
    (void)rn;
#endif
    if (n <= FIB_TABLE_LIMIT) {
        dst[0] = fib_table[n + 1]; /* F[n]，n==0 时为0 */
        return 1;
    }

    /* k = n/2，先算出 (F[k], F[k-1])，再一次乘法合并。
       limbs_bound(n2)+2 同时覆盖核心循环（2*limbs(F[n2>>1])+1）与
       合并操作数 xp/yp 的写入（size+2，见文件头注三）*/
    ulong n2 = n >> 1;
    mp_size_t xalloc = fib_limbs_bound_(n2) + 2;
    TEMP_DECL;
    mp_ptr xp = TALLOC_TYPE(xalloc, mp_limb_t);
    mp_ptr yp = TALLOC_TYPE(xalloc, mp_limb_t);
    mp_ptr tp = TALLOC_TYPE(xalloc, mp_limb_t);
    mp_size_t size = fib2_core_(xp, yp, tp, xalloc, n2);
    mp_size_t xsize, ysize;

    if (n & 1) {
        /* F[2k+1] = (2F[k]+F[k-1]) * (2F[k]-F[k-1]) + 2*(-1)^k */
        mp_limb_t c2 = lmmp_shl_(dst, xp, size, 1); /* dst 暂存 2F[k] 低 size 位 */
        mp_limb_t c = lmmp_add_n_(xp, dst, yp, size);
        c += c2; /* 2F[k] 的溢出位并入最高 limb，c=[0|1|2] */
        xp[size] = c;
        xsize = size + (c != 0);
        c2 -= lmmp_sub_n_(yp, dst, yp, size); /* yp = 2F[k]-F[k-1]，c2=[0|1] */
        lmmp_debug_assert(c2 <= 1);
        yp[size] = c2;
        ysize = size + c2;

        lmmp_mul_(dst, xp, xsize, yp, ysize);
        size = xsize + ysize;
        if (n & 2) {
            mp_limb_t b = lmmp_sub_1_(dst, dst, size, 2);
            lmmp_debug_assert(b == 0);
        } else {
            mp_limb_t c3 = lmmp_add_1_(dst, dst, size, 2);
            lmmp_debug_assert(c3 == 0);
        }
    } else {
        /* F[2k] = F[k] * (F[k] + 2F[k-1]) */
        mp_limb_t c = lmmp_addshl1_n_(yp, xp, yp, size); /* yp = F[k]+2F[k-1] */
        yp[size] = c;
        ysize = size + (c != 0);

        lmmp_mul_(dst, yp, ysize, xp, size);
        size += ysize;
    }

    size -= (dst[size - 1] == 0);
    TEMP_FREE;
    return size;
}
