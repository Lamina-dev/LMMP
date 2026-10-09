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

#ifndef __LMMP_POWMOD_H__
#define __LMMP_POWMOD_H__

#include "../lmmpn.h"
#include "../numth.h"
#include "longlong.h"
#include "mparam.h"
#include "mul_cache.h"

/*
    奇模数 Montgomery 域公共核心：powmod.c（模幂梯子）与 is_prime_n.c
    （素性检验）共用（两处原为同源 static 拷贝，现收敛为本头文件单一定义，
    以 static inline 保持原零调用开销特征）。

    记 R = B^n，m 为奇模数（n limb），ninv = -m^(-1) mod B^n。
    单步 REDC：给定 t = a*b < m*B^n（[tp,2n]），令

        q = t_lo * ninv mod B^n        （q*m ≡ -t_lo (mod B^n)）

    则 (t + q*m)/B^n ≡ t*R^(-1) (mod m) 且 < 2m。低半部分 t_lo + (q*m)_lo
    恰为 B^n*[t_lo!=0]，无需逐 limb 相加，故

        u = t_hi + mulhi(q, m) + [t_lo!=0]

    高半积 mulhi(q,m) 的三种来源（按规模分层）：
      1. basecase（n < REDC_BASECASE_THRESHOLD）：链式 Hensel 归约——
         n 次 addmul_1 逐 limb 消零（q_j = up[0]*ninv1 mod B），成本约一次
         basecase 乘法，且只需单 limb 逆元（其 clobber 输入的特性见各调用点）；
      2. 全积取高半：lmmp_mul_n_ 后读高 n limb（中尺寸段）；
      3. 梅森折叠（n >= REDC_MERSENNE_THRESHOLD）：由于 (q*m) mod B^n
         = -t_lo mod B^n 是已知量 L，与 binvert_mulhi_ 同构——先算 V = q*m
         mod (B^msz-1)（msz 为 admissible 尺寸，模数 m 一侧的变换全程缓存
         复用），从 V 中减去 L 后旋转载出高半。两操作数均 < B^n 保证
         hi <= B^n-2，拼接表示不会落在 B^msz-1 的二义点上；而 L==0 强制
         q==0、积为零，也不与零类的非规范表示冲突。

    蒙域值 x̃ = x*R mod m；进蒙域（redcify）用一次 (x*B^n) mod m 除法（低位
    补 n 零 limb 实现 <<B^n），比 RR=B^2n mod m 加乘加归约的旧路径省约两个
    M(n) 的预处理；basecase 层连全长 binvert 也一并省去（仅需低 limb 逆元）。

    映射 x -> x*R mod m 为 Z/m 上的环同构（m 奇故 gcd(R,m)=1）：乘法（REDC）
    与加减法均保持，且 1 与 -1 的蒙域剩余为 one = B^n mod m 与 m1 = m - one
    （anchors 档预计算）。素性检验的 ±1 探测因此可全程留在蒙域进行，
    免去出蒙域归约。
*/

/* REDC 梯子上下文：模数一侧的全部预计算（ninv/one/m1/折叠分派）与工作区。
   arena 分段布局（lmmp_mont_need_，随规模与 anchors 裁剪）：
     anchors：one(n) | m1(n)          —— 蒙域 ±1 探测基准（is_prime_n 用）
     basecase（n < REDC_BASECASE_THRESHOLD）：无 core 段
     中层：ninv(n) | q(n) | mscratch(2n) | mulhi(2n)                == 6n
     折叠层（n >= REDC_MERSENNE_THRESHOLD）：mulhi(2n) 换
           Lbuf(n) | hi(n) | V(msz)                                 == 6n + msz
   one/m1 之后的段与调用者的梯子段续接（单块打包） */
typedef struct {
    mp_size_t n;
    mp_srcptr m;     /* 模数 */
    mp_ptr ninv;     /* -m^(-1) mod B^n（basecase 层不使用，为 NULL） */
    mp_limb_t ninv1; /* -m^(-1) mod B（basecase 层专用） */
    int fold;        /* n >= REDC_MERSENNE_THRESHOLD 时走梅森折叠 */
    mp_size_t msz;   /* 折叠尺寸（fold 时有效） */
    mp_ptr one;      /* [n] B^n mod m（蒙域 1，无 anchors 档为 NULL） */
    mp_ptr m1;       /* [n] m - one（蒙域 -1，无 anchors 档为 NULL） */
    mp_ptr q;        /* [n]        q = t_lo * ninv mod B^n */
    mp_ptr Lbuf;     /* [n]        -t_lo mod B^n */
    mp_ptr hi;       /* [n]        q*m 的高 n limb（折叠路径输出） */
    mp_ptr mulhi;    /* [2n]       全积路径的 q*m（非折叠路径） */
    mp_ptr V;        /* [msz]      折叠积 q*m mod B^msz-1 */
    mp_ptr mscratch; /* [2n]       mullo scratch */
    fft_gr_cache mcache;
    int mcache_on;
    fft_mullo_cache ncache;
    int ncache_on;
} lmmp_mont_t;

/**
 * @brief 蒙域上下文的分段工作区长度
 * @param n 模数 limb 长度
 * @param anchors 1 = 含 one/m1 探测基准段，0 = 仅 REDC 工作段
 * @return 蒙域段所需的 limb 数
 */
static inline mp_size_t lmmp_mont_need_(mp_size_t n, int anchors) {
    if (n < REDC_BASECASE_THRESHOLD) return anchors ? 2 * n : 0;
    if (n >= REDC_MERSENNE_THRESHOLD) return 6 * n + lmmp_fft_next_size_((2 * n + 1) >> 1) + (anchors ? 2 * n : 0);
    return 6 * n + (anchors ? 2 * n : 0);
}

/**
 * @brief 从梅森折叠积中提取高半积（低位已知变体的 mulhi）
 * @param hi 结果指针（n 个limb），接收 [q,n]*[mp,n] div B^n
 * @param V 输入兼工作区（msz 个limb），入口为 q*m mod B^msz-1，出口被破坏
 * @param L 已知低位（n 个limb），满足 q*m ≡ L (mod B^n)，此处 L = -t_lo mod B^n
 * @param n 操作数长度
 * @param msz 折叠尺寸，admissible 且 n <= msz < 2n
 * @warning sep(hi,V), V 的 msz-n 个高位 limb 亦会被读写
 * @note 设 P = q*m = hi*B^n + L，则 W = (V-L) mod B^msz-1 = hi*B^n mod B^msz-1，
 *       即 hi 的第 j limb 恰位于 W[(j+n) mod msz]，旋转载出即得
 */
static inline void lmmp_mont_fold_hi_(
    mp_ptr    restrict hi,
    mp_ptr     restrict V,
    mp_srcptr restrict  L,
    mp_size_t             n,
    mp_size_t           msz
) {
    mp_size_t fn = msz - n; /* hi 中线性对位部分长度（取自 V+n） */
    mp_size_t sn = n - fn;  /* hi 中环绕部分长度（取自 V 头部） */
    lmmp_debug_assert(msz >= n && 2 * n > msz);

    mp_limb_t bor = lmmp_sub_n_(V, V, L, n);
    if (bor) {
        /* 数组当前值 = V - L + B^n（借位等价于吸收在第 n 个 limb 上） */
        if (msz > n)
            bor = lmmp_sub_1_(V + n, V + n, msz - n, 1);
        /* 若高位段借位（此时 V < L，高位段必为全零，减 1 后变全 1），
           或 msz==n（借位即 V<L），整体再减 1 补回 B^msz-1 */
        if (msz == n || bor)
            lmmp_dec(V);
    }
    lmmp_copy(hi, V + n, fn);
    lmmp_copy(hi + fn, V, sn);
}

/**
 * @brief basecase REDC：链式 Hensel 归约，逐 limb 消零
 * @param dst 结果指针（n 个limb），接收 ([up,2n] + q*m)/B^n 的低 n limb
 * @param up 输入兼工作区（2n 个limb，出口被破坏）
 * @param m 模数（n 个limb）
 * @param n 操作数长度
 * @param ninv1 -m^(-1) mod B（单 limb）
 * @warning sep(dst,up), n < REDC_BASECASE_THRESHOLD 时才是优选路径
 * @return 进位（[0|1]），返回值:[dst,n] 即 REDC 结果 < 2m（t < B^n*m 时）
 * @note 每轮 q = up[0]*ninv1 mod B 使 up[0] 恰好归零，addmul_1 的进位
 *       存入刚清零的 limb；n 轮后高半与累积进位的低半相加即得结果。
 *       总成本 n 次 addmul_1 ≈ 一次 basecase 乘法
 */
static inline mp_limb_t lmmp_mont_redc_bc_(
    mp_ptr    restrict dst,
    mp_ptr    restrict  up,
    mp_srcptr restrict   m,
    mp_size_t              n,
    mp_limb_t            ninv1
) {
    mp_ptr restrict up0 = up;
    for (mp_size_t j = n; j > 0; j--) {
        mp_limb_t cy = lmmp_addmul_1_(up, m, n, up[0] * ninv1);
        lmmp_debug_assert(up[0] == 0);
        up[0] = cy;
        up++;
    }
    return lmmp_add_n_(dst, up0 + n, up0, n);
}

/**
 * @brief REDC 单步并规范化：[dst,n] = (t + q*m)/B^n mod m，结果 < m
 * @param dst 结果指针（n 个limb）
 * @param tp 被归约数兼工作区（2n 个limb，t < B^n*[m,n]，出口被破坏）
 * @param mc 蒙域上下文（须已完成 lmmp_mont_init_）
 * @warning sep(dst,tp), dst 与 mc 内各缓冲区均分离
 */
static inline void lmmp_mont_redc_(mp_ptr restrict dst, mp_ptr restrict tp, lmmp_mont_t* restrict mc) {
    mp_size_t n = mc->n;
    mp_ptr restrict hi;

    if (n < REDC_BASECASE_THRESHOLD) {
        mp_limb_t cy = lmmp_mont_redc_bc_(dst, tp, mc->m, n, mc->ninv1);
        if (cy) {
            /* 值 >= B^n > m，减 m 必然可行；借位恰好抵消进位 limb */
            (void)lmmp_sub_n_(dst, dst, mc->m, n);
        } else if (lmmp_cmp_(dst, mc->m, n) >= 0) {
            lmmp_sub_n_(dst, dst, mc->m, n);
        }
        return;
    }

    mp_limb_t carry = !lmmp_zero_q_(tp, n);

    if (n < MULLO_DC_THRESHOLD) {
        lmmp_mullo_dc_(mc->q, tp, mc->ninv, mc->mscratch, n);
    } else if (mc->ncache_on == 0) {
        lmmp_mullo_fft_cache_init_(mc->q, tp, mc->ninv, n, mc->mscratch, &mc->ncache);
        mc->ncache_on = 1;
    } else {
        lmmp_mullo_fft_cache_(mc->q, tp, mc->mscratch, &mc->ncache);
    }

    if (mc->fold == 0) {
        lmmp_mul_n_(mc->mulhi, mc->q, mc->m, n);
        hi = mc->mulhi + n;
    } else {
        if (mc->mcache_on == 0) {
            lmmp_mul_mersenne_cache_init_(mc->V, mc->msz, mc->q, n, mc->m, n, &mc->mcache);
            mc->mcache_on = 1;
        } else {
            lmmp_mul_mersenne_cache_(mc->V, mc->q, &mc->mcache);
        }
        if (carry) {
            lmmp_not_(mc->Lbuf, tp, n);
            lmmp_inc(mc->Lbuf);
        } else {
            lmmp_zero(mc->Lbuf, n);
        }
        lmmp_mont_fold_hi_(mc->hi, mc->V, mc->Lbuf, n, mc->msz);
        hi = mc->hi;
    }

    /* u = t_hi + hi + carry，结果 carry_out:[dst,n] < 2m */
    mp_limb_t cy = lmmp_add_nc_(dst, tp + n, hi, n, carry);
    if (cy) {
        /* 值 >= B^n > m，减 m 必然可行；n limb 减法的借位恰好抵消进位 limb */
        (void)lmmp_sub_n_(dst, dst, mc->m, n);
    } else if (lmmp_cmp_(dst, mc->m, n) >= 0) {
        lmmp_sub_n_(dst, dst, mc->m, n);
    }
}

/**
 * @brief 进蒙域：[dst,n] = [xp,n]*B^n mod [mp,n]
 * @param dst 结果指针（n 个limb）
 * @param xp 普通域输入（n 个limb，eqsep(dst,xp)）
 * @param n 操作数长度
 * @param prod 工作区（2n 个limb，出口被破坏，sep(dst,xp,mp)）
 * @param mp 模数（n 个limb）
 * @warning n>0, mp[0]%2==1, mp[n-1]>0, sep([prod|dst],mp)
 */
static inline void lmmp_mont_redcify_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict   xp,
    mp_size_t             n,
    mp_ptr    restrict prod,
    mp_srcptr restrict   mp
) {
    lmmp_zero(prod, n);
    lmmp_copy(prod + n, xp, n);
    lmmp_div_(NULL, dst, prod, 2 * n, mp, n);
}

/**
 * @brief 蒙域上下文初始化：切分 REDC 工作段并预计算 ninv 与探测基准
 * @param mc 上下文（出口就绪）
 * @param mp 模数（n 个limb，奇，mc->m 引用之，生命周期须覆盖上下文全程）
 * @param n 模数 limb 长度
 * @param arena 工作区分段头（lmmp_mont_need_(n,anchors) 个limb）
 * @param anchors 1 = 预计算 one/m1（蒙域 ±1 探测基准），0 = 免配
 * @param prod [2n] redcify 工作区（与 arena 分段分离，出口被破坏）
 * @warning n>0, mp[0]%2==1, mp[n-1]>0, sep([arena|prod],mp)
 * @note 分段布局见 lmmp_mont_t 注释；TEMP 工作区由调用者 TEMP_FREE 统一
 *       回收，FFT 变换缓存另经 lmmp_mont_free_ 释放
 */
static inline void lmmp_mont_init_(
    lmmp_mont_t* mc,
    mp_srcptr     mp,
    mp_size_t      n,
    mp_ptr      arena,
    int       anchors,
    mp_ptr restrict prod
) {
    mc->n = n;
    mc->m = mp;
    mc->fold = n >= REDC_MERSENNE_THRESHOLD;
    mc->msz = mc->fold ? lmmp_fft_next_size_((2 * n + 1) >> 1) : 0;
    mc->mcache_on = 0;
    mc->ncache_on = 0;
    if (mc->fold)
        lmmp_debug_assert(2 * n > mc->msz && mc->msz >= n);

    if (anchors) {
        mc->one = arena;
        mc->m1 = arena + n;
    } else {
        mc->one = mc->m1 = NULL;
    }
    if (n < REDC_BASECASE_THRESHOLD) {
        mc->ninv = NULL;
        mc->ninv1 = 0 - lmmp_binvert_ulong_(mp[0]);
        mc->q = mc->Lbuf = mc->hi = mc->mscratch = mc->mulhi = mc->V = NULL;
    } else {
        mp_size_t core = anchors ? 2 * n : 0;
        mc->ninv1 = 0;
        mc->ninv = arena + core;
        mc->q = arena + core + n;
        mc->mscratch = arena + core + 2 * n;
        if (mc->fold) {
            mc->Lbuf = arena + core + 4 * n;
            mc->hi = arena + core + 5 * n;
            mc->V = arena + core + 6 * n;
            mc->mulhi = NULL;
        } else {
            mc->mulhi = arena + core + 4 * n;
            mc->Lbuf = mc->hi = mc->V = NULL;
        }
        /* ninv = -m^(-1) mod B^n：m 奇故其逆亦奇，取反加一不会越顶 */
        lmmp_binvert_(mc->ninv, mp, n, n);
        lmmp_not_(mc->ninv, mc->ninv, n);
        lmmp_inc(mc->ninv);
    }

    if (anchors) {
        /* one = B^n mod m：低位补 n 零 limb 实现 <<B^n，除法取余（m1 段先
           兼作被除数的高半载体） */
        lmmp_zero(mc->m1, n);
        mc->m1[0] = 1;
        lmmp_mont_redcify_(mc->one, mc->m1, n, prod, mp);
        /* m1 = m - one（one != 0 恒成立，无借位） */
        (void)lmmp_sub_n_(mc->m1, mp, mc->one, n);
    }
}

/* 释放蒙域上下文的 FFT 变换缓存（TEMP 工作区由调用者 TEMP_FREE 统一回收） */
static inline void lmmp_mont_free_(lmmp_mont_t* mc) {
    if (mc->mcache_on)
        lmmp_fft_gr_cache_free_(&mc->mcache);
    if (mc->ncache_on)
        lmmp_mullo_cache_free_(&mc->ncache);
}

/* ============ 稀疏模数蒙域旁路（lmmp_mont_sp_t，不接入 lmmp_mont_t） ============ */

/*
    形状 m = ε + Kv·B^p（ε ∈ {+1,−1}，Kv ≤ 2 limb，p ≥ 1，n ≥ 2）：
      +1 形（Proth）：limb0 = 1、limb 1..p−1 全零、顶部 ≤2 limb 簇 = Kv；
      −1 形（Mersenne/Riesel）：limb 0..p−1 全 B−1、顶部簇读值 Kv−1
                               （m = Kv·B^p − 1，簇全 B−1 已被探测排除，Kv 无越顶）。

    单步 REDC 与 lmmp_mont_redc_bc_ 同为链式 Hensel 逐 limb 消零，但每轮
    addmul_1 经伸缩恒等式坍缩为 O(1)：

        q·m·B^64j = q·Kv·B^(64(j+p)) + ε·q·B^64j

    乘子由 m ≡ ε (mod B) 免费给出（ε=+1 ⟹ m^(-1)=1 ⟹ q = B − up[j]，本位
    u0+q = B 恰清零、进位 1 入 j+1；ε=−1 ⟹ q = up[j]，本位 u0−q 恰清零无
    借位），中间 limb 零贡献，唯一的实做是 j+p 处 q·Kv 的 ≤3 limb 乘加与
    有界进位游程（add_1 遇非 B−1 即停，游程被后续 O(1) 次写入重建，充电
    论证总游程 O(n)）。p ≥ 1 保证各操作只触 ≥ j+1 位，低 j limb 恒零，
    n 轮后高 n limb 即结果。整步 O(n)，全尺寸优于 bc/中层/折叠三层
    （后三者 ≈ 一次 M(n)）——本旁路因此独立于 lmmp_mont_t 的尺寸分派，
    形状命中即用，上下文无任何工作段与缓存（init 免配、无 free）。

    值域核算：表示值 = 数组 + top·B^2n ≡ t + (Σq_j B^j)m < 2m·B^n ≤ 2B^2n，
    数组非负 ⟹ top ∈ {0,1}（越顶进位逐次捕获累加）；结果 = 高半 + top·B^n
    ∈ [0,2m)，规范化至多减 m 一次。
*/
typedef struct {
    mp_size_t n;   /* 模数 limb 长度 */
    mp_srcptr m;   /* 模数（引用调用者内存，生命周期须覆盖上下文全程） */
    int form;      /* 形状：+1 / −1（0 = 非稀疏，上下文未装配） */
    mp_size_t p;   /* 簇偏移（1 <= p <= n-1） */
    mp_limb_t kv0; /* Kv 低 limb */
    mp_limb_t kv1; /* Kv 高 limb（1 limb 簇时为 0；−1 形为簇读值+1 的产物） */
} lmmp_mont_sp_t;

/**
 * @brief 稀疏形状探测与上下文装配（O(n) 扫描，免分配）
 * @param mc 上下文（出口：形状命中时就绪，否则 form=0 其余字段无效）
 * @param mp 模数（n 个limb，奇，顶 limb 非零）
 * @param n 模数 limb 长度
 * @warning n >= 2
 * @return 形状（+1/−1），非稀疏返回 0
 */
static inline int lmmp_mont_sp_init_(lmmp_mont_sp_t* mc, mp_srcptr mp, mp_size_t n) {
    lmmp_debug_assert(n >= 2 && mp[0] % 2 == 1 && mp[n - 1] != 0);
    mc->n = n;
    mc->m = mp;
    if (mp[0] == 1) {
        mp_size_t i = 1;
        while (i < n && mp[i] == 0) i++;
        if (n - i < 1 || n - i > 2) return mc->form = 0;
        mc->form = 1;
        mc->p = i;
        mc->kv0 = mp[i];
        mc->kv1 = (n - i == 2) ? mp[n - 1] : 0;
        return 1;
    }
    if (mp[0] == LIMB_MAX) {
        mp_size_t i = 1;
        while (i < n && mp[i] == LIMB_MAX) i++;
        if (i >= n || n - i > 2) return mc->form = 0;
        mc->form = -1;
        mc->p = i;
        /* Kv = 簇读值 + 1；mp[i] != B-1（探测保证）⟹ kv0 不回绕、无进位 */
        mc->kv0 = mp[i] + 1;
        mc->kv1 = (n - i == 2) ? mp[n - 1] : 0;
        return -1;
    }
    return mc->form = 0;
}

/**
 * @brief 稀疏模数 REDC 单步并规范化：[dst,n] = t*B^(-n) mod m，结果 < m
 * @param dst 结果指针（n 个limb）
 * @param tp 被归约数兼工作区（2n 个limb，t < m*B^n，出口被破坏）
 * @param mc 稀疏蒙域上下文（须已完成 lmmp_mont_sp_init_ 且 form != 0）
 * @warning sep(dst,tp)，dst 与 mc->m 分离
 */
static inline void lmmp_mont_sp_redc_(mp_ptr restrict dst, mp_ptr restrict tp, lmmp_mont_sp_t* restrict mc) {
    mp_size_t n = mc->n, n2 = 2 * n;
    mp_size_t p = mc->p;
    mp_limb_t kv0 = mc->kv0, kv1 = mc->kv1;
    mp_ptr up = tp;
    mp_limb_t top = 0;

    for (mp_size_t j = 0; j < n; j++, up++) {
        mp_limb_t u0 = up[0];
        if (u0 == 0) continue;
        mp_limb_t q;
        if (mc->form > 0) {
            /* +1 形：q = B − u0，本位清零、进位 1 入 j+1（游程遇非 B−1 即停） */
            q = LIMB_MAX - u0 + 1;
            up[0] = 0;
            top += lmmp_add_1_(up + 1, up + 1, n2 - j - 1, 1);
        } else {
            /* −1 形：q = u0，本位恰清零无借位 */
            q = u0;
            up[0] = 0;
        }
        /* 绝对位 j+p 处 += q·Kv（up 已前移 j，故相对偏移恰为 p；Kv <= 2 limb
           ⟹ 积 <= 3 limb，绝对位 j+p+1 <= 2n-1 恒在界内） */
        mp_size_t w = p;
        u128 t = (u128)q * kv0;
        u128 s = (u128)up[w] + (mp_limb_t)t;
        up[w] = (mp_limb_t)s;
        t = (u128)q * kv1 + (mp_limb_t)(t >> 64);
        s = (u128)up[w + 1] + (mp_limb_t)t + (mp_limb_t)(s >> 64);
        up[w + 1] = (mp_limb_t)s;
        mp_limb_t cy = (mp_limb_t)(t >> 64) + (mp_limb_t)(s >> 64);
        if (cy) {
            /* 绝对位 j+p+2 = 2n（1 limb 簇顶格）时进位直达 top */
            if (j + w + 2 < n2)
                cy = lmmp_add_1_(up + w + 2, up + w + 2, n2 - j - w - 2, cy);
            top += cy;
        }
    }
    lmmp_debug_assert(top <= 1);

    /* 低 n limb 恒零；结果 = 高半 + top（虚进位），< 2m，规范化一次 */
    lmmp_copy(dst, tp + n, n);
    if (top) {
        /* 值 >= B^n > m，减 m 必然可行，借位恰好抵消虚进位 */
        (void)lmmp_sub_n_(dst, dst, mc->m, n);
    } else if (lmmp_cmp_(dst, mc->m, n) >= 0) {
        lmmp_sub_n_(dst, dst, mc->m, n);
    }
}

/**
 * @brief 模幂滑动窗口尺寸（按指数位长分档）
 * @param eb 指数位长（>0）
 * @return 窗口宽 k ∈ [1,10]，奇次幂表长 2^(k-1)
 */
static inline unsigned lmmp_powmod_win_size_(mp_bitcnt_t eb) {
    unsigned k;
    static mp_bitcnt_t x[] = {7, 25, 81, 241, 673, 1793, 4609, 11521, 28161, ~(mp_bitcnt_t)0};
    for (k = 0; eb > x[k++];);
    return k;
}

/**
 * @brief 蒙域梯子（lmmp_powmod_odd_mont_）的调用者工作区长度
 * @param n 模数 limb 长度
 * @param eb 指数位长（>0）
 * @return 所需 limb 数：b2(n) + prod(2n) + 奇次幂表(n << (win-1))
 */
static inline mp_size_t lmmp_powmod_odd_mont_need_(mp_size_t n, mp_bitcnt_t eb) {
    return 3 * n + (n << (lmmp_powmod_win_size_(eb) - 1));
}

/**
 * @brief 奇模数蒙域模幂梯子（滑动窗口），结果留在蒙域
 * @param u 出口（n 个limb）= b^e·R mod m < m，亦为梯子累加器
 * @param bp 普通域基底（n 个limb，0 < [bp,n] < [mc->m,n]）
 * @param ep 指数（en 个limb，顶 limb 非零）
 * @param en 指数 limb 长度
 * @param mc 蒙域上下文（须已完成 lmmp_mont_init_）
 * @param b2 表生成暂存（n 个limb，b²R mod m，出口死）
 * @param prod 工作区（2n 个limb，平方/乘法全积兼 redcify，出口被破坏）
 * @param pp 奇次幂表（n << (lmmp_powmod_win_size_(eb)-1) 个limb，出口死）
 * @warning n>2 建议走 1/2 limb 特化（见 lmmp_powmod_odd_），sep 各段与 mc
 * @note 大尺寸比二进制梯子省约 30% 乘法；e=1 时即 redcify。同一 mc 跨多次
 *       调用可复用 ninv 与 FFT 变换缓存（同模数批量模幂/素性检验轮）
 */
void lmmp_powmod_odd_mont_(
    mp_ptr          restrict u,
    mp_srcptr       restrict bp,
    mp_srcptr       restrict ep,
    mp_size_t                 en,
    lmmp_mont_t* restrict     mc,
    mp_ptr          restrict b2,
    mp_ptr          restrict prod,
    mp_ptr          restrict pp
);

#endif /* __LMMP_POWMOD_H__ */
