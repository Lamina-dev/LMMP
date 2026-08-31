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
 *  This program is distributed in WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/*
hgcd（half-GCD，半扩展欧几里得）算法

一、问题定义

给定 n limb 的数对 (a; b)（a > b > 0），在 O(M(n) log n) 时间内将其归约至约
n/2 limb，同时累积 2x2 变换矩阵 M 满足

    / a \   / m00  m01 \   / a' \
    |   | = |          | * |   |
    \ b /   \ m10  m11 /   \ b' /

其中 (a'; b') 为归约后的数对。矩阵元素均非负，det(M) = ±1。由于 M 幺模，
gcd(a, b) = gcd(a', b') 严格保持。反复调用 hgcd 直至规模降到阈值以下，再用
Lehmer 算法收尾，即得完整 gcd（见 lmmp_gcd_hgcd_）。

二、单步步进（lmmp_hgcd_step_）

每一步尝试将数对消去约 62 位，方式有二：

1. 近似矩阵步。从 (a, b) 顶端对齐提取 127 位近似窗口 (x; y)（按 a 的最高位
   对齐，a 与 b 使用同一尺度），在 (x; y) 上运行小规模欧几里得并累积单 limb
   矩阵 U，约定 (x; y) = U * (u; v)。停机采用 break-before 策略：余数将跌破
   2^65 时不再前进，保证终态 v >= 2^65，从而
     - U 的元素 <= x / v < 2^127 / 2^65 < 2^62，可放入单 limb；
     - 对真值应用的误差（< 2^63 ulp）小于信号（>= 2^65 ulp），U^{-1} 作用
       于完整 (a; b) 的结果保证非负。
   商的计算用 u_hi / (v_hi + 1) 下界估计加减法校正，避免 128 位除法。
   得到 U 后更新 M <- M * U，并将 (V0; V1) = U^{-1} * (a; b) 写回数对。
   det(U) = ±1 决定 V 的符号方向，数对翻转时以列交换后的 U 复合（U*P）。

2. 真除法回退。近似失效（两数顶端 127 位相同，或 y < 2^65 即商过大）时做
   带下限的除法：全除 a = q*b + r 若余数不低于下限则直接采用；否则做部分
   除法 Q = (a - B^(fl-1)) div b，令 r' = a - Q*b >= B^(fl-1)。部分商 Q 可
   为多 limb。

下限 fl = n/2 + 2 是本实现正确性的核心不变量：任何步进结果保证较小分量
>= B^(fl-1)，而矩阵元素 <= B^(n-fl+1) < B^(fl-1)，两者严格分离。这保证了
上层递归做矩阵合并时修补项不会淹没保留的高位（见 adjust 的推导），也使得
"过冲检测 + 回滚"策略成立。

三、递归结构（lmmp_hgcd_core_）

对规模超过递归阈值（HGCD_RECURSE_THRESHOLD）的数对做两阶段分治：

阶段 A：将 a、b 的高半部分（各 hn = n - n/2 limb）复制到独立缓冲，递归
调用 hgcd 将其归约至约 hn/2，得到子矩阵 M1；随后用"高位直写 + 低位修补"
（lmmp_hgcd_adjust_）或模乘合并（lmmp_hgcd_apply_mod_）把 M1 的作用合并到
完整数对。合并后规模约降至 3n/4。

桥接：用少量单步步进将规模从约 3n/4 降到 3n/4 + 1（通常常数步）。

阶段 B：p2 = 2*fl - n + 1，对新的高半部分（约 n - p2 limb）再次递归并
合并，规模降至约 fl，即约一半。

appr（近似）模式（appr 参数）：仅保证 M 累积至足够规模，阶段 B 退化为
纯矩阵复合 M <- M*M1（无数对修补），复合完成立即返回。
由于 lmmp_hgcd_apply_mod_ 以 4 次模乘从完整数对直接重构、不依赖子递归的
数对输出，凡本层用模乘合并处，子递归即可用 appr 模式；用 adjust（需要子
数对的精确归约结果）处子递归必须精确。appr 使每个递归节点的 4 乘法合并
次数从 2 降至 1，大输入下整体约省 15%。

提交与回滚：递归成果先写入 sa/sb 副本，仅当（1）修补无借位（结果非负），
（2）较小分量长度大于 M1 最大元素长度（下限不变量保持），（3）规模确实
下降，三个条件都满足时才提交并复合 M <- M * M1；否则放弃递归成果，数对
保持原状，交由步进循环继续。过冲只会损失性能而不会破坏正确性。

det 符号穿线：矩阵的行列式符号在递归中逐点跟踪（F(q) 型更新变号、列交
换变号等），adjust 直接使用，免去每次两次 Mn*Mn 乘法的判定开销。

四、矩阵复合（lmmp_hgcd_matrix_mul_）

M <- M * M1 经由库内 lmmp_mat22_mul_ 完成，按元素最大长度自动分派：
basecase（8 乘）或 Winograd-Strassen（7 乘；FFT 缓存乘法路径曾实现并
证伪后删除，见 mat22_mul.h 头注）。M 为单位阵时退化为按长度直接拷贝。
矩阵存储为"逐元素显式长度"布局：各元素真实长度存于 M->n[i][j]（归一
化，0 表示零元素），无零填充不变量；与 lmmp_mat22_t 公共前缀同构（指
针强转零开销），配合指针交换实现 O(1) 的行列交换。

五、内存管理

全部内部函数经由穿线的递增分配器（lmmp_hgcd_scratch_t）获取临时空间：
顶层按 lmmp_hgcd_scratch_size_ 给出的上界一次性分配，各函数入口取标记、
出口恢复，兄弟调用复用同一区域，无逐层堆分配。lmmp_gcd_hgcd_ 的数对工作
缓冲与矩阵缓冲也在循环外一次分配复用。scratch 上界按"沿递归路径取 max"
核算（子层临时空间与本层合并相位共享同一区域，仅常驻高位副本跨层叠加，
见 lmmp_hgcd_scratch_size_ 注），约 15n limb，较早期逐层累加的 48n 缩至
约 1/3（51000 limb 约 19.6MB -> 6.1MB）。

六、模乘合并变体（lmmp_hgcd_apply_mod_）

以 4 次 mod B^rn-1 的梅森模乘精确重构 M1^{-1}*(a; b)（真值为正且
< B^nn <= B^(rn-1) 时模结果即真值）。数对折叠至 rn limb，矩阵元素以
原始长度（M1->n，必小于 rn）直接参与模乘，无需填充。fa/fb 各被两个
矩阵元素相乘，经 lmmp_mul_mersenne_cache_ 系列缓存其前向 FFT 变换，
第二次模乘省一次全尺寸前向变换。注意梅森乘对短操作数仍做全尺寸 FFT
（零填充无节省），其相对 lmmp_mul_ 的优势仅在平衡形状下显著，故阈值
取值需实测标定。由 HGCD_MODMUL_THRESHOLD 控制启用规模，正确性已经
与 Lehmer 的批量交叉验证，性能由 LmmpMeasure 的 numth/gcd 测量组标定。

七、复杂度

每层递归的乘法工作为 O(M(n))，递归深度 O(log n)，总计 O(M(n) log n)。
实测约 800 limb 以上优于 Lehmer 的 O(n^2)，4000 limb 约 4 倍加速；
16000-46000 limb 与 GMP 6.3 相当或更快。

主要开销相位（51000 limb 实测，占递归核总耗时）：matrix_mul 约 36%，
apply_mod 约 30%，step 约 20%，adjust 约 3%。

参考：
    GMP 的 hgcd/hgcd_appr/hgcd_reduce/hgcd_matrix_adjust 设计思想，
    https://www.cnblogs.com/whx1003/p/16217087.html
*/

#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mat22_mul.h"
#include "../../../include/lmmp/impl/mul_cache.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


/* 递归阈值：公共长度超过该值时启用分治路径。实测扫描 96~512（本机
   i7-14 代，11k~51k limb 三点 A/B 交替）性能差异在噪声内（±2%），
   维持 128；中小规模（600~8k）192 略优亦在噪声内。待接入 tune 体系 */
#define HGCD_RECURSE_THRESHOLD 128

static mp_size_t lmmp_hgcd_norm_(mp_srcptr p, mp_size_t n) {
    while (n > 0 && p[n - 1] == 0) {
        --n;
    }
    return n;
}

static mp_size_t lmmp_hgcd_matrix_maxlen_(const lmmp_hgcd_matrix_t* M) {
    mp_size_t mx = 0;
    if (M->n[0][0] > mx)
        mx = M->n[0][0];
    if (M->n[0][1] > mx)
        mx = M->n[0][1];
    if (M->n[1][0] > mx)
        mx = M->n[1][0];
    if (M->n[1][1] > mx)
        mx = M->n[1][1];
    return mx;
}

static inline void MUL(mp_ptr dst, mp_srcptr a, mp_size_t an, mp_srcptr b, mp_size_t bn) {
    if (an >= bn) {
        lmmp_mul_(dst, a, an, b, bn);
    } else {
        lmmp_mul_(dst, b, bn, a, an);
    }
}

typedef struct {
    mp_ptr base;
    mp_size_t cap; /* 总容量 */
    mp_size_t used; /* 当前游标 */
} lmmp_hgcd_scratch_t;

/* 从递增分配器取 n limbs（出口用 sc->used = mark 复位，兄弟调用共享同一区域） */
static inline mp_ptr lmmp_hgcd_salloc_(lmmp_hgcd_scratch_t* sc, mp_size_t n) {
    lmmp_debug_assert(sc->used + n <= sc->cap);
    mp_ptr p = sc->base + sc->used;
    sc->used += n;
    return p;
}

/**
 * @brief 计算 hgcd 内部临时空间大小
 * @param n 输入长度
 * @warning n>0
 * @note 峰值沿递归路径取 max 而非逐层求和：递增分配器经 mark 复位使
 *       子层临时空间与本层合并相位（sa/sb 副本、adjust/apply_mod/矩阵
 *       复合缓冲，合计约 14n）共享同一区域，二者不同时达峰；真正叠加
 *       的只有递归期间常驻的高位副本与子矩阵缓冲。子问题规模上界取
 *       2n/3 + 2（阶段 B 的 p2 分裂在最不利时接近该比例，严于实际的
 *       n - n/2 与 2n_cur - n0 - 5）。本层合并相位预算 15n + 512（含
 *       apply_mod 折叠缓冲的 next_size 取整余量与矩阵复合 9(2mx+4)），
 *       步进循环 8n + 64。对比旧的逐层累加（约 48n）内存占用降至约 1/3
 */
static mp_size_t lmmp_hgcd_scratch_size_(mp_size_t n) {
    mp_size_t acc = 0;           /* 递归路径常驻前缀（高位副本累计） */
    mp_size_t best = 8 * n + 64; /* 基础步进循环（含除法回退路径） */
    while (n > HGCD_RECURSE_THRESHOLD) {
        mp_size_t hn = 2 * n / 3 + 2; /* 子问题规模上界（见注） */
        mp_size_t level = acc + 15 * n + 512; /* 本层合并相位峰值 */
        if (level > best) {
            best = level;
        }
        acc += 6 * hn + 16; /* ha/hb/M1buf：子递归期间常驻不释放 */
        mp_size_t steps = acc + 8 * n + 64; /* 桥接/收尾步进循环峰值 */
        if (steps > best) {
            best = steps;
        }
        n = hn;
    }
    mp_size_t tail = acc + 8 * n + 64;
    if (tail > best) {
        best = tail;
    }
    return best;
}

/**
 * @brief 矩阵列交换（指针交换）：对应右乘 P（M <- M*P），用于数对翻转的簿记 
 */
static inline void lmmp_hgcd_matrix_swap_cols_(lmmp_hgcd_matrix_t* M) {
    LMMP_SWAP(M->m[0][0], M->m[0][1], mp_ptr);
    LMMP_SWAP(M->m[1][0], M->m[1][1], mp_ptr);
    LMMP_SWAP(M->n[0][0], M->n[0][1], mp_size_t);
    LMMP_SWAP(M->n[1][0], M->n[1][1], mp_size_t);
}

/**
 * @brief 在 [ap,n] 与 [bp,n] 的 127 位顶端近似上运行小规模欧几里得，累积单 limb 矩阵 U
 * @param pu00 输出矩阵元素 m00（仅返回 1 时写入）
 * @param pu01 输出矩阵元素 m01（仅返回 1 时写入）
 * @param pu10 输出矩阵元素 m10（仅返回 1 时写入）
 * @param pu11 输出矩阵元素 m11（仅返回 1 时写入）
 * @param ap 较大数指针（长度为 n 个limb，ap[n-1]>0）
 * @param bp 较小数指针（长度为 n 个limb，允许高位为零：窗口按 a 的顶位对齐，
 *           b 的高位零只使 y 偏小，不破坏正确性）
 * @param n ap,bp 的公共长度
 * @return 1 表示 U 非单位矩阵（可用近似步进）；0 表示需走真除法回退
 * @warning ap!=NULL, bp!=NULL, n>=3, ap[n-1]>0, [bp,n]>0（值序 a>b）
 * @note 约定 (x;y) = U*(u;v)，元素非负且 < 2^62，det(U) = [1|-1]。
 *       127 位窗口 + break-before 停机（余数将跌破 2^65 时不再前进），
 *       终态 v >= 2^65 保证元素界 x/v < 2^62 与对真值应用的非负性
 *      （误差 < 2·2^62 ulp < 2^63 ulp <= v ulp）。每步约消去 62 位。
 *       商用 u_hi/(v_hi+1) 下界估计 + 减法校正，避免 128 位除法
 */
static int lmmp_hgcd_lehmer2_(
    mp_limb_t* pu00,
    mp_limb_t* pu01,
    mp_limb_t* pu10,
    mp_limb_t* pu11,
    mp_srcptr    ap,
    mp_srcptr    bp,
    mp_size_t     n
) {
    /* 顶端对齐提取 127 位窗口（按 a 的顶位对齐，a 与 b 使用同一尺度） */
    mp_limb_t xh, xl, yh, yl;
    int kz = lmmp_limb_bits_(ap[n - 1]);
    if (kz >= 64) {
        xh = ap[n - 1] >> 1;
        xl = (ap[n - 1] << 63) | (ap[n - 2] >> 1);
        yh = bp[n - 1] >> 1;
        yl = (bp[n - 1] << 63) | (bp[n - 2] >> 1);
    } else if (kz == 63) {
        xh = ap[n - 1];
        xl = ap[n - 2];
        yh = bp[n - 1];
        yl = bp[n - 2];
    } else {
        int sh = 63 - kz; /* [1,62] */
        xh = (ap[n - 1] << sh) | (ap[n - 2] >> (64 - sh));
        xl = (ap[n - 2] << sh) | (ap[n - 3] >> (64 - sh));
        yh = (bp[n - 1] << sh) | (bp[n - 2] >> (64 - sh));
        yl = (bp[n - 2] << sh) | (bp[n - 3] >> (64 - sh));
    }
    if (xh == yh && xl == yl) {
        return 0; /* 顶 127 位相同：两数过近，商失真 */
    }
    if (yh < 2) {
        return 0; /* y < 2^65：商过大，走真除法回退 */
    }

    /* 小规模欧几里得：(u;v) 为 u128 对；除法用 (u_hi / (v_hi+1)) 下界估计 + 减法校正 */
    mp_limb_t u00 = 1, u01 = 0, u10 = 0, u11 = 1;
    mp_limb_t uh = xh, ul = xl, vh = yh, vl = yl;
    for (;;) {
        mp_limb_t q = uh / (vh + 1); /* 下界：q_hat*v <= u，余数非负 */
        mp_limb_t al, ah, bl, bh, rl, rh;
        _umul64to128_(q, vl, &bl, &bh);
        _umul64to128_(q, vh, &al, &ah);
        /* q*v = (al + bh : bl)，ah 必为 0（q*v <= u < 2^127） */
        rl = ul - bl;
        rh = uh - al - bh - (ul < bl);
        lmmp_debug_assert(rh <= uh && (rh < uh || rl <= ul)); /* 估计保证非负 */
        while (rh > vh || (rh == vh && rl >= vl)) {
            /* r >= v：q 偏小，减 v 校正（估计算法保证至多两次） */
            ++q;
            mp_limb_t t = rl - vl;
            int borrow = rl < vl;
            rl = t;
            rh = rh - vh - borrow;
        }
        /* 停机判定：r < 2^65 即 rh < 2 */
        if (rh < 2) {
            break; /* break-before：终态保证 v >= 2^65 */
        }
        /* (u;v) = F(q)*(v;r)；U <- U*F(q)：新 col0 = q*col0 + col1，新 col1 = col0 */
        mp_limb_t a00 = u00, a10 = u10;
        mp_limb_t b00 = q * a00 + u01;
        mp_limb_t b10 = q * a10 + u11;
        u01 = a00;
        u11 = a10;
        u00 = b00;
        u10 = b10;
        uh = vh;
        ul = vl;
        vh = rh;
        vl = rl;
    }
    if (u00 == 1 && u01 == 0 && u10 == 0 && u11 == 1) {
        return 0;
    }
    *pu00 = u00;
    *pu01 = u01;
    *pu10 = u10;
    *pu11 = u11;
    return 1;
}

/**
 * @brief M <- M*U，U 为单 limb 元素的非负矩阵
 * @param M 变换矩阵：输入读各元素 [0,M->n[i][j])，输出写各元素与
 *          M->n[i][j]（新长度至多旧长度+1）
 * @param sc 临时空间（max(M->n[i][j])+2 个limb）
 * @param detsign det(M) 的符号，输入输出同源（乘 det(U)=[1|-1] 调整）
 * @param u00 U 的元素 m00（非负且 < 2^62）
 * @param u01 U 的元素 m01（非负且 < 2^62）
 * @param u10 U 的元素 m10（非负且 < 2^62）
 * @param u11 U 的元素 m11（非负且 < 2^62）
 * @warning sc!=NULL, M!=NULL, max(M->n[i][j])+2 <= M->alloc
 * @note 长度显式维护：col' = u_a*col_a + u_b*col_b 的长度为
 *       L+进位（L = max(la,lb)，双乘进位之和 < 2^63 无再进位），
 *       mul_1/addmul_1 的进位经 [l,L) 区补零对齐后并入顶端
 */
static void lmmp_hgcd_matrix_mul_u_(
    lmmp_hgcd_matrix_t* M,
    lmmp_hgcd_scratch_t* sc,
    int* detsign,
    mp_limb_t u00,
    mp_limb_t u01,
    mp_limb_t u10,
    mp_limb_t u11
) {
    /* det(U) = u00*u11 - u01*u10 = ±1，元素 < 2^62，乘积需 u128 比较 */
    mp_limb_t dlo, dhi, elo, ehi;
    _umul64to128_(u00, u11, &dlo, &dhi);
    _umul64to128_(u01, u10, &elo, &ehi);
    *detsign *= (dhi > ehi || (dhi == ehi && dlo >= elo)) ? 1 : -1;
    mp_size_t mx = lmmp_hgcd_matrix_maxlen_(M);
    lmmp_debug_assert(mx + 2 <= M->alloc);
    mp_size_t mark = sc->used;
    mp_ptr t = lmmp_hgcd_salloc_(sc, mx + 2);

    for (int row = 0; row < 2; ++row) {
        mp_ptr c0 = M->m[row][0];
        mp_ptr c1 = M->m[row][1];
        mp_size_t l0 = M->n[row][0];
        mp_size_t l1 = M->n[row][1];
        mp_size_t L = l0 > l1 ? l0 : l1;

        mp_limb_t c, cc;
        mp_size_t l1n;
        if (u01 == 0 || l0 == 0) {
            if (u11 == 0 || l1 == 0) {
                l1n = 0; /* 两项皆零（det=[1|-1] 保证不会两列同零，防御） */
            } else {
                c = lmmp_mul_1_(t, c1, l1, u11);
                t[l1] = c;
                l1n = l1 + (c != 0);
            }
        } else if (u11 == 0 || l1 == 0) {
            c = lmmp_mul_1_(t, c0, l0, u01);
            t[l0] = c;
            l1n = l0 + (c != 0);
        } else {
            c = lmmp_mul_1_(t, c0, l0, u01);
            t[l0] = c;
            /* 对齐补零（长度差，矩阵元素通常 0~2）；l1 > l0 时 L == l1，
               多补的 [L] 供顶位判断与 add_1_ 回读（见上） */
            lmmp_zero(t + l0 + 1, L - l0 + (l1 > l0));
            cc = lmmp_addmul_1_(t, c1, l1, u11);
            /* 进位属于 [l1] 位置，经 add_1_ 传播，顶端无出界（和 < 2^63） */
            if (cc != 0) {
                lmmp_add_1_(t + l1, t + l1, L + 1 - l1, cc);
            }
            l1n = L + (t[L] != 0);
            if (t[l1n - 1] == 0 && l1n > 1) {
                --l1n;
            }
            if (t[l1n - 1] == 0 && l1n > 1) {
                --l1n;
            }
        }
        /* 新 col0 = u00*col0 + u10*col1，原地（col1 尚未覆写），同构 */
        mp_size_t l0n;
        if (u00 == 0 || l0 == 0) {
            if (u10 == 0 || l1 == 0) {
                l0n = 0;
            } else {
                c = lmmp_mul_1_(c0, c1, l1, u10);
                c0[l1] = c;
                l0n = l1 + (c != 0);
            }
        } else if (u10 == 0 || l1 == 0) {
            c = lmmp_mul_1_(c0, c0, l0, u00);
            c0[l0] = c;
            l0n = l0 + (c != 0);
        } else {
            c = lmmp_mul_1_(c0, c0, l0, u00);
            c0[l0] = c;
            lmmp_zero(c0 + l0 + 1, L - l0 + (l1 > l0)); /* 含顶位预置（见上） */
            cc = lmmp_addmul_1_(c0, c1, l1, u10);
            if (cc != 0) {
                lmmp_add_1_(c0 + l1, c0 + l1, L + 1 - l1, cc);
            }
            l0n = L + (c0[L] != 0);
            if (c0[l0n - 1] == 0 && l0n > 1) {
                --l0n;
            }
            if (c0[l0n - 1] == 0 && l0n > 1) {
                --l0n;
            }
        }
        lmmp_copy(c1, t, l1n);
        M->n[row][0] = l0n;
        M->n[row][1] = l1n;
    }
    sc->used = mark;
}

/**
 * @brief 真除法步进的矩阵累积
 * @param M 变换矩阵：输入读各元素 [0,M->n[i][j])，输出写更新列各元素
 *          与长度（另一列仅按 col 语义搬移/保持）
 * @param sc 临时空间（2*(qn+max(M->n[i][j])+1) limbs）
 * @param detsign det(M) 的符号，输入输出同源（col=0 时乘 det(F)=-1 变号）
 * @param q 商数组（长度为 qn 个limb，q[qn-1]>0）
 * @param qn 商的 limb 长度，qn>0
 * @param col 被更新列编号，亦即右乘矩阵的选择。按本模块约定 (a;b) = M*(a';b')，
 *            数对做一步带商 q 的欧几里得步进 a = q*b + r 后，需右乘 X 使
 *            (a;b) = (M*X)*(新数对)，两种步进方向对应：
 *            col=0（无翻转，新数对 (b; r)，r 作新的较小分量）：
 *                X = F(q) = / q 1 \ ，det = -1；
 *                           \ 1 0 /
 *                列语义：新 col0 = q*col0 + col1，新 col1 = 旧 col0。
 *                验证：F(q)*(b;r) = (q*b+r; b) = (a; b)
 *            col=1（翻转，新数对 (r; b)，r 作新的较大分量；部分除法中
 *                商被压低使 r' >= b 时发生）：
 *                X = E(q) = / 1 q \ ，det = +1；
 *                           \ 0 1 /
 *                列语义：新 col1 = q*col0 + col1，col0 不变。
 *                验证：E(q)*(r;b) = (r+q*b; b) = (a; b)
 *                （E(q) = F(q)*P，P 为列交换，即数对翻转在矩阵侧的簿记）
 * @warning M!=NULL, sc!=NULL, q!=NULL, qn>0, sep(q,M)
 * @note 更新列长度显式维护：t = q*col0 + col1 的长度 = max(lt, l1)+进位，
 *       其中积长 lt = qn+l0 或 qn+l0-1（积 >= B^(qn+l0-2)，顶 limb 单次
 *       判断）；l0==0（零元素）时 t = col1 长度直接已知
 */
static void lmmp_hgcd_matrix_update_q_(
    lmmp_hgcd_matrix_t*  M,
    lmmp_hgcd_scratch_t* sc,
    int*                 detsign,
    mp_srcptr            q,
    mp_size_t            qn,
    int                  col
) {
    if (col == 0) {
        *detsign = -*detsign; /* F(q) det = -1 */
    }
    mp_size_t mx = lmmp_hgcd_matrix_maxlen_(M);
    mp_size_t mark = sc->used;
    mp_ptr t = lmmp_hgcd_salloc_(sc, qn + mx + 1);

    /*
    F(q)：col0' = q*col0 + col1，col1' = col0_old
    E(q)：col1' = q*col0 + col1，col0' 不变
    两种情形的更新列均为 q*col0 + col1，区别在另一列的去向
     */
    for (int row = 0; row < 2; ++row) {
        mp_size_t l0 = M->n[row][0];
        mp_size_t l1 = M->n[row][1];
        mp_size_t lt;
        if (l0 == 0) {
            lmmp_copy(t, M->m[row][1], l1);
            lt = l1;
        } else {
            MUL(t, q, qn, M->m[row][0], l0);
            if (l1 == 0) {
                /* 零元素：lmmp_add_ 对 nb==0 不安全（内联宏经 do-while 汇编），
                   且积 >= B^(qn+l0-2)，长度顶 limb 单次判断 */
                lt = qn + l0 - (t[qn + l0 - 1] == 0);
            } else {
                /* 乘积缓冲 [0, qn+l0) 有效，若 col1 更长需先清零对齐区，
                   再以两者最大长度相加，满足 lmmp_add_ 的 na>=nb 契约 */
                mp_size_t m = qn + l0;
                if (l1 > m) {
                    lmmp_zero(t + m, l1 - m);
                    m = l1;
                }
                mp_limb_t carry = lmmp_add_(t, t, m, M->m[row][1], l1);
                /* lmmp_add_ 等长相加的进位仅经返回值报告，需显式落盘 */
                if (carry != 0) {
                    t[m] = carry;
                }
                lt = m + (carry != 0);
                /* 归一化：和 >= max(积, col1) >= B^(m-1)，至多两个前导零 limb */
                if (t[lt - 1] == 0 && lt > 1) {
                    --lt;
                }
                if (t[lt - 1] == 0 && lt > 1) {
                    --lt;
                }
            }
        }
        int dst = col == 0 ? 0 : 1;
        if (col == 0) {
            /* col1 <- col0_old（指针与长度一并搬移后写回更新列） */
            lmmp_copy(M->m[row][1], M->m[row][0], l0);
            M->n[row][1] = l0;
        }
        lmmp_copy(M->m[row][dst], t, lt);
        M->n[row][dst] = lt;
    }
    sc->used = mark;
}

/**
 * @brief 将连续缓冲初始化为单位矩阵
 * @param M 变换矩阵（写入 M->m 指针、M->n=1、M->alloc）
 * @param buf 连续缓冲（写入全部 4*alloc 个limb，先清零再置对角元为 1）
 * @param alloc 每元素容量（limb）
 * @warning M!=NULL, buf!=NULL, alloc>0
 */
static void lmmp_hgcd_matrix_init_buf_(lmmp_hgcd_matrix_t* M, mp_ptr buf, mp_size_t alloc) {
    lmmp_debug_assert(M != NULL && buf != NULL && alloc > 0);
    /* 初始清零仅为缓冲卫生（一次性）；运行期以长度字段为准，无零填充不变量 */
    lmmp_zero(buf, 4 * alloc);
    M->m[0][0] = buf;
    M->m[0][1] = buf + alloc;
    M->m[1][0] = buf + 2 * alloc;
    M->m[1][1] = buf + 3 * alloc;
    M->n[0][0] = 1;
    M->n[0][1] = 0;
    M->n[1][0] = 0;
    M->n[1][1] = 1;
    M->alloc = alloc;
    M->m[0][0][0] = 1;
    M->m[1][1][0] = 1;
}

/**
 * @brief M <- M*M1（经由库内 lmmp_mat22_mul_，按元素最大长度自动分派
 *        basecase / Winograd-Strassen 路径）
 * @param M 左矩阵，输入读各元素 [0,M->n[i][j])，输出写各元素与长度字段
 *          （新长度 <= max(M->n[i][k]) + max(M1->n[k][j]) + 1）
 * @param M1 右矩阵（只读，各元素 [0,M1->n[i][j]) 有效）
 * @param sc 临时空间（8*(mx(M)+mx(M1))+16 个limb）
 * @warning M!=NULL, M1!=NULL, sc!=NULL, sep(M,M1),
 *          maxlen(M)+maxlen(M1)+4 <= M->alloc
 * @note M 为单位阵时退化为按长度直接拷贝 M1。lmmp_hgcd_matrix_t 与
 *       lmmp_mat22_t 公共前缀同构（m[2][2]+n[2][2]），指针强转零开销
 */
static void lmmp_hgcd_matrix_mul_(lmmp_hgcd_matrix_t* M, const lmmp_hgcd_matrix_t* M1, lmmp_hgcd_scratch_t* sc) {
    mp_size_t mlen = lmmp_hgcd_matrix_maxlen_(M);
    mp_size_t m1len = lmmp_hgcd_matrix_maxlen_(M1);
    if (M->n[0][0] == 1 && M->m[0][0][0] == 1 && M->n[1][1] == 1 && M->m[1][1][0] == 1 &&
        M->n[0][1] == 0 && M->n[1][0] == 0) {
        /* M 为单位阵：M*M1 = M1，按各元素真实长度整段拷贝 */
        lmmp_debug_assert(m1len <= M->alloc);
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                lmmp_copy(M->m[i][j], M1->m[i][j], M1->n[i][j]);
                M->n[i][j] = M1->n[i][j];
            }
        }
        return;
    }

    lmmp_debug_assert(mlen + m1len + 4 <= M->alloc);
    mp_size_t mark = sc->used;
    mp_size_t mx = lmmp_hgcd_matrix_maxlen_(M);
    mp_size_t mx1 = lmmp_hgcd_matrix_maxlen_(M1);
    if (mx1 > mx) {
        mx = mx1;
    }
    mp_ptr tp = lmmp_hgcd_salloc_(sc, 8 * (mx + mx1) + 16);
    lmmp_mat22_mul_((lmmp_mat22_t*)M, (const lmmp_mat22_t*)M, (const lmmp_mat22_t*)M1, tp);
    sc->used = mark;
}

/**
 * @brief 高位直写 + 低位修补：将子递归结果合入完整数对
 * @param M1 子递归累积的矩阵（各元素输入长度为 M1->n 个 limb）
 * @param detsign det(M1) 的符号，[1|-1]
 * @param dap 分量 a 输出数组（写入长度为 n 个limb，返回 0 时内容无效）
 * @param dbp 分量 b 输出数组（写入长度为 n 个limb，返回 0 时内容无效）
 * @param ha 子递归归约后的高位 a（长度为 hn0 个limb）
 * @param hb 子递归归约后的高位 b（长度为 hn0 个limb）
 * @param hn0 子递归返回的公共长度，hn0<=n-p
 * @param ap 源完整较大分量（长度为n个limb，其中低 p 位参与修补）
 * @param bp 源完整较小分量（长度为n个limb）
 * @param n ap,bp 的公共长度
 * @param p 高低位分裂点
 * @param sc 临时空间（4*(p+M1->n+1) 个limb）
 * @return 合入后的公共长度（<=n，两分量的真实长度最大值）；0 表示子归约过冲
 * @warning M1!=NULL, dap!=NULL, dbp!=NULL, sc!=NULL, sep([dap|dbp],[ap|bp|ha|hb]),
 *          n>p>0, ap[n-1]>0
 * @note 真值 (a;b) = M1*(a_hi';b_hi')·B^p + M1*(a_lo;b_lo)，其中 (a_hi';b_hi')
 *       即 (ha;hb)。逆变换 adj(M1)*(a;b) = det*(new_a;new_b) 的低位部分为
 *       ±(m11*a_lo - m01*b_lo)（b 同理），方向由 det(M1)=±1 决定。
 *       输出布局：低 p 位直写乘积，高位段为 ha/hb 与乘积高位相加再减另一乘积。
 *       乘法规模 (p × M1->n)，约为全量矩阵应用的一半
 */
static mp_size_t lmmp_hgcd_adjust_(
    lmmp_hgcd_matrix_t*  M1,
    int                  detsign,
    mp_ptr               dap,
    mp_ptr               dbp,
    mp_size_t*           blen,
    mp_srcptr            ha,
    mp_srcptr            hb,
    mp_size_t            hn0,
    mp_srcptr            ap,
    mp_srcptr            bp,
    mp_size_t            n,
    mp_size_t            p,
    lmmp_hgcd_scratch_t* sc
) {
    mp_size_t mn = lmmp_hgcd_matrix_maxlen_(M1);
    mp_size_t mark = sc->used;
    mp_size_t N = p + mn + 1;
    mp_ptr t0 = lmmp_hgcd_salloc_(sc, N);
    mp_ptr t1 = lmmp_hgcd_salloc_(sc, N);
    mp_ptr t2 = lmmp_hgcd_salloc_(sc, N);
    mp_ptr t3 = lmmp_hgcd_salloc_(sc, N);
    /* 长度前提（由子递归的下限不变量保证，防御性检查） */
    if (n - p < mn || n < p + mn) {
        *blen = 0;
        sc->used = mark;
        return 0;
    }

    /*
    det 由穿线符号跟踪（构造保证 = ±1）。零元素（长度 0）跳过乘法——
    lmmp_mul_ 契约要求 nb>0，乘积缓冲置零以维持后续加减的定长语义。
    乘积仅写 [0, p+n_ij)，而后续加减按 mn 定长读取 [p, p+mn)：
    元素长度 n_ij < mn 时须补零 [p+n_ij, p+mn)，否则读到未写残留
    */
    int det = detsign;
    mp_size_t n11 = M1->n[1][1], n10 = M1->n[1][0], n01 = M1->n[0][1], n00 = M1->n[0][0];
    if (n11 != 0) {
        MUL(t0, ap, p, M1->m[1][1], n11); /* m11*a_lo */
        lmmp_zero(t0 + p + n11, mn - n11);
    } else {
        lmmp_zero(t0, N);
    }
    if (n10 != 0) {
        MUL(t1, ap, p, M1->m[1][0], n10); /* m10*a_lo */
        lmmp_zero(t1 + p + n10, mn - n10);
    } else {
        lmmp_zero(t1, N);
    }
    if (n01 != 0) {
        MUL(t2, bp, p, M1->m[0][1], n01); /* m01*b_lo */
        lmmp_zero(t2 + p + n01, mn - n01);
    } else {
        lmmp_zero(t2, N);
    }
    if (n00 != 0) {
        MUL(t3, bp, p, M1->m[0][0], n00); /* m00*b_lo */
        lmmp_zero(t3 + p + n00, mn - n00);
    } else {
        lmmp_zero(t3, N);
    }

    /*
    det=+1：new_a = ha + t0 - t2，new_b = hb + t3 - t1
    det=-1：new_a = ha + t2 - t0，new_b = hb + t1 - t3
    */
    mp_ptr adda = det > 0 ? t0 : t2;
    mp_ptr suba = det > 0 ? t2 : t0;
    mp_ptr addb = det > 0 ? t3 : t1;
    mp_ptr subb = det > 0 ? t1 : t3;

    /* new_a：低 p 位直写被加积，高位段为 ha 再加被加积高位，最后整体减被减积 */
    lmmp_copy(dap + p, ha, hn0);
    lmmp_zero(dap + p + hn0, n - p - hn0);
    lmmp_copy(dap, adda, p);
    mp_limb_t carry = lmmp_add_(dap + p, dap + p, n - p, adda + p, mn);
    if (carry != 0) {
        sc->used = mark;
        return 0;
    }
    mp_limb_t borrow = lmmp_sub_(dap, dap, n, suba, p + mn);
    if (borrow != 0) {
        sc->used = mark;
        return 0; /* 过冲：new_a < 0 */
    }

    /* new_b：同构 */
    lmmp_copy(dbp + p, hb, hn0);
    lmmp_zero(dbp + p + hn0, n - p - hn0);
    lmmp_copy(dbp, addb, p);
    carry = lmmp_add_(dbp + p, dbp + p, n - p, addb + p, mn);
    if (carry != 0) {
        sc->used = mark;
        return 0;
    }
    borrow = lmmp_sub_(dbp, dbp, n, subb, p + mn);
    if (borrow != 0) {
        sc->used = mark;
        return 0; /* 过冲：new_b < 0 */
    }

    /*
    结果分量经加减抵消，长度无进位信息可用，须扫描获取（数对输出）；
    返回公共长度 rn = max(la,lb)，blen 出参单独给出分量 b 的长度
    （供上层校验，免去二次扫描）。数对为零填充公共长度表示：输出
    需将两分量补零至 rn（上层的规范化序比较按公共长度读取）
    */
    mp_size_t la = n;
    while (la > 0 && dap[la - 1] == 0) {
        --la;
    }
    mp_size_t lb = n;
    while (lb > 0 && dbp[lb - 1] == 0) {
        --lb;
    }
    mp_size_t rn = la > lb ? la : lb;
    lmmp_zero(dap + la, rn - la);
    lmmp_zero(dbp + lb, rn - lb);
    *blen = lb;
    sc->used = mark;
    return rn;
}

/**
 * @brief 单步步进：用近似矩阵 U 或一次（可能部分的）除法归约数对
 * @param M 变换矩阵（累积）：输入读各元素长度为 M->n 个limb，输出零填充至新公共长度
 * @param ap 较大分量输入兼输出数组：输入读 [ap,n)（ap[n-1]>0），输出写 [ap,nn)
 *           并零填充至新公共长度 nn（规范序：归约后较大者写入 ap）
 * @param bp 较小分量输入兼输出数组：输入读 [bp,n)（允许高位为零，[bp,n]>0），
 *           输出写 [bp,nn) 并零填充至 nn
 * @param n ap,bp 的公共长度
 * @param fl 步进下限：结果较小分量 >= B^(fl-1)。保证矩阵元素 <= B^(n-fl+1) < B^(fl-1)，
 *           使上层递归的矩阵修补非负性成立
 * @param sc 临时空间（峰值约 6n+6 个limb：除法回退路径的商/余/被除副本
 *           与矩阵更新的嵌套需求）
 * @param detsign det(M) 的符号，输入输出同源（本函数依更新矩阵调整其值）
 * @return 新公共长度 nn（<=n，等于两分量真实长度的最大值，即 ap[nn-1]>0）；
 *         0 表示无法在不破坏下限的前提下继续归约，数对保持 [0,n) 原值
 * @warning M!=NULL, sc!=NULL, ap!=NULL, bp!=NULL, sep(ap,bp), n>=3, n>fl, a>b>0
 */
static mp_size_t lmmp_hgcd_step_(
    lmmp_hgcd_matrix_t*  M,
    mp_ptr               ap,
    mp_ptr               bp,
    mp_size_t            n,
    mp_size_t            fl,
    lmmp_hgcd_scratch_t* sc,
    int*                 detsign
) {
    mp_limb_t u00, u01, u10, u11;
    if (lmmp_hgcd_lehmer2_(&u00, &u01, &u10, &u11, ap, bp, n)) {
        /*
        (V0;V1) = U^{-1}*(a;b)：V0 = D*(u11*a - u01*b)，V1 = D*(u00*b - u10*a)，
        D = det(U) = ±1。乘减融合（mul_1 + submul_1）于两个暂存中完成并做下限
        检查，通过后才写回（返回 0 时数对必须保持原值，供上层继续使用）。
        det 的符号直接决定减法方向（lehmer2 的 break-before 停机保证被减方
        更大，高位 limb 相等即非负性成立）
        */
        mp_size_t mark = sc->used;
        mp_size_t N = n + 1;
        mp_ptr t0 = lmmp_hgcd_salloc_(sc, N);
        mp_ptr t3 = lmmp_hgcd_salloc_(sc, N);
        mp_limb_t dlo, dhi, elo, ehi;
        _umul64to128_(u00, u11, &dlo, &dhi);
        _umul64to128_(u01, u10, &elo, &ehi);
        int det = (dhi > ehi || (dhi == ehi && dlo >= elo)) ? 1 : -1;
        mp_limb_t h0, h1;
        if (det > 0) {
            /* V0 = u11*a - u01*b；V1 = u00*b - u10*a */
            h0 = lmmp_mul_1_(t0, ap, n, u11);
            h1 = lmmp_submul_1_(t0, bp, n, u01);
            lmmp_debug_assert(h0 == h1);
            h0 = lmmp_mul_1_(t3, bp, n, u00);
            h1 = lmmp_submul_1_(t3, ap, n, u10);
            lmmp_debug_assert(h0 == h1);
        } else {
            /* V0 = u01*b - u11*a；V1 = u10*a - u00*b */
            h0 = lmmp_mul_1_(t0, bp, n, u01);
            h1 = lmmp_submul_1_(t0, ap, n, u11);
            lmmp_debug_assert(h0 == h1);
            h0 = lmmp_mul_1_(t3, ap, n, u10);
            h1 = lmmp_submul_1_(t3, bp, n, u00);
            lmmp_debug_assert(h0 == h1);
        }

        /*
        an,bn ∈ {n-1, n}：V0 = u11*a - u01*b > a/2^62 >= B^(n-1)/2^62
        > 2^(64n-127)，即长度 >= n-1；又 V0 < B^n（mul/submul 高位 limb
        相等的断言保证非负且无出界）——单次顶 limb 判断即得真实长度
        */
        mp_size_t an = n - (t0[n - 1] == 0);
        mp_size_t bn = n - (t3[n - 1] == 0);
        if (an < fl || bn < fl) {
            sc->used = mark;
            return 0; /* 继续将破坏下限（差值已达目标规模），数对保持原值 */
        }
        mp_size_t nn = an > bn ? an : bn;
        lmmp_debug_assert(nn <= n);
        if (lmmp_cmp_(t0, t3, n) >= 0) {
            lmmp_hgcd_matrix_mul_u_(M, sc, detsign, u00, u01, u10, u11); /* M <- M*U */
            lmmp_copy(ap, t0, an);
            lmmp_zero(ap + an, nn - an);
            lmmp_copy(bp, t3, bn);
            lmmp_zero(bp + bn, nn - bn);
        } else {
            lmmp_hgcd_matrix_mul_u_(M, sc, detsign, u01, u00, u11, u10);
            /* 参数即 U*P 元素，mul_u 内部 det 计算已正确 */ /* M <- M*(U*P) */
            lmmp_copy(ap, t3, bn);
            lmmp_zero(ap + bn, nn - bn);
            lmmp_copy(bp, t0, an);
            lmmp_zero(bp + an, nn - an);
        }
        sc->used = mark;
        return nn;
    }

    /*
    除法回退（带下限）：先试全除 a = q*b + r；
    若 r 将跌破下限，改用部分除法：Q = (a - B^(fl-1)) div b，r' = a - Q*b >= B^(fl-1)
    */
    /* 数对为零填充公共长度表示，除数真实长度显式信息不可得，须扫描
       （下限不变量保证 nb >= fl，扫描量有界；此回退路径罕见） */
    mp_size_t nb = lmmp_hgcd_norm_(bp, n);
    mp_size_t mark = sc->used;
    mp_ptr q = lmmp_hgcd_salloc_(sc, n + 2);
    mp_ptr r = lmmp_hgcd_salloc_(sc, n + 2);
    lmmp_div_(q, r, ap, n, bp, nb);
    /*
    全商 qn ∈ {n-nb, n-nb+1}：a >= B^(n-1) 且 b < B^nb，故
    q >= B^(n-nb-1)（nb==n 时 q >= 1，因 a >= b）；q < B^(n-nb+1)
    ——单次顶 limb 判断
    */
    mp_size_t qn = n - nb + 1 - (q[n - nb] == 0);
    /* 余数无下界（可为任意小），长度须扫描；此回退路径罕见 */
    mp_size_t rr = lmmp_hgcd_norm_(r, nb);

    if (rr >= fl) {
        /* 全商：数对更新为 (b; r)，b > r 无翻转 */
        lmmp_hgcd_matrix_update_q_(M, sc, detsign, q, qn, 0);
        mp_size_t nn = nb > rr ? nb : rr;
        lmmp_copy(ap, bp, nb);
        lmmp_zero(ap + nb, nn - nb);
        lmmp_copy(bp, r, rr);
        lmmp_zero(bp + rr, nn - rr);
        sc->used = mark;
        return nn;
    }

    /* 部分除法：t = a - B^(fl-1)（n >= fl+1 保证 a > B^fl > B^(fl-1)，减法不借位穿底） */
    mp_ptr t = lmmp_hgcd_salloc_(sc, n + 1);
    lmmp_copy(t, ap, n);
    lmmp_dec(t + (fl - 1));
    lmmp_div_(q, r, t, n, bp, nb);
    /* 部分商可为零（t < b 时），下界缺失，长度须扫描；此回退路径罕见 */
    qn = lmmp_hgcd_norm_(q, n - nb + 1);
    if (qn == 0) {
        sc->used = mark;
        return 0; /* t < b：两数之差已在下限内，目标达成 */
    }
    /*
    r' = (t mod b) + B^(fl-1)。由 a = q*b + r（r < B^(fl-1)）可推得
    t mod b = b + r - B^(fl-1)，故 r' = b + r <= a < B^n，
    即 rr <= n：数对写界不超出输入公共长度。r 的长度相对 nb 无下界
    （首次步进 b 可远短于下限，r' 由 r 主导），长度信息不存在，
    须扫描；此回退路径罕见
    */
    lmmp_zero(r + nb, n + 2 - nb);
    lmmp_inc(r + (fl - 1));
    rr = lmmp_hgcd_norm_(r, n + 1);
    lmmp_debug_assert(rr >= fl && rr <= n);
    mp_size_t nn = nb > rr ? nb : rr;
    if (lmmp_cmp_(bp, r, nn) > 0) {
        /* b > r'：M <- M*F(Q)，数对 (b; r') 无翻转 */
        lmmp_hgcd_matrix_update_q_(M, sc, detsign, q, qn, 0);
        lmmp_copy(ap, bp, nb);
        lmmp_zero(ap + nb, nn - nb);
        lmmp_copy(bp, r, rr);
        lmmp_zero(bp + rr, nn - rr);
    } else {
        /* r' >= b：M <- M*(F(Q)*P) = M*E(Q)（列1更新），数对 (r'; b) */
        lmmp_hgcd_matrix_update_q_(M, sc, detsign, q, qn, 1);
        lmmp_copy(ap, r, rr);
        lmmp_zero(ap + rr, nn - rr);
        lmmp_zero(bp + nb, nn - nb); /* bp 保持 b，仅清零至公共长度 */
    }
    sc->used = mark;
    return nn;
}

/*
模乘合并阈值：公共长度达到该值时，递归合并走 lmmp_hgcd_apply_mod_（梅森
模乘），否则走 lmmp_hgcd_adjust_（普通乘修补）。
注意梅森乘法对短操作数仍做全尺寸 FFT（零填充无节省），其相对 lmmp_mul_
的优势仅在平衡形状显著；在 hgcd 的不平衡形状（p × M1->n 与 rn × M1->n）
下两者耗时接近
 */
#define HGCD_MODMUL_THRESHOLD 2500

/**
 * @brief 将 [src,sn] 折叠为模 B^rn-1 的表示：写入 [dst,rn)。
*/
static void lmmp_hgcd_fold_mod_(mp_ptr dst, mp_srcptr src, mp_size_t sn, mp_size_t rn) {
    lmmp_copy(dst, src, sn <= rn ? sn : rn);
    lmmp_zero(dst + (sn <= rn ? sn : rn), rn - (sn <= rn ? sn : rn));
    while (sn > rn) {
        src += rn;
        sn -= rn;
        mp_limb_t cy = lmmp_add_(dst, dst, rn, src, sn <= rn ? sn : rn);
        if (cy != 0) {
            lmmp_inc(dst);
        }
    }
}

/**
 * @brief 模 B^rn-1 梅森乘法版矩阵应用：
 *        new = adj(M1)*(a;b)，经 4 次模乘与模减精确重构
 * @param M1 子递归累积的矩阵（只读，各元素长度为 M1->n 个limb，M1->n<rn）
 * @param detsign det(M1) 的符号，[1|-1]
 * @param dap 分量 a 输出数组（写入 rn_out 个limb，rn_out 为返回值；返回 0 时内容无效）
 * @param dbp 分量 b 输出数组（写入 rn_out 个limb）
 * @param ap 源完整较大分量（只读，长度为 n 个limb）
 * @param bp 源完整较小分量（只读，长度为 n 个limb）
 * @param n ap,bp 的公共长度
 * @param sc 临时空间（6*rn 个limb；FFT 缓存上下文另行经由库内临时池分配）
 * @return 合入后的公共长度 rn_out（两分量真实长度最大值）；0 表示过冲
 * @warning M1!=NULL, dap!=NULL, dbp!=NULL, sc!=NULL, sep([dap|dbp],[ap|bp]),
 *          n>0, M1->n>1, ap[n-1]>0
 * @note 真值满足 0 <= new < B^nn <= B^(rn-1)（nn 为下方推导的界），
 *       过冲（new < 0）时模结果回绕为接近 rn 长度，据此刻检测并返回 0。
 *       数对折叠至 rn limb 参与模乘；矩阵元素以原始长度 M1->n 直接参与
 *      （M1->n<rn 保证落在模内）。fa/fb 各被两个矩阵元素乘，经
 *       lmmp_mul_mersenne_cache_ 系列复用其前向 FFT 变换
 */
static mp_size_t lmmp_hgcd_apply_mod_(
    lmmp_hgcd_matrix_t*  M1,
    int                  detsign,
    mp_ptr               dap,
    mp_ptr               dbp,
    mp_size_t*           blen,
    mp_srcptr            ap,
    mp_srcptr            bp,
    mp_size_t            n,
    lmmp_hgcd_scratch_t* sc
) {
    mp_size_t mark = sc->used;
    mp_size_t m00 = M1->n[0][0];
    mp_size_t m01 = M1->n[0][1];
    mp_size_t m10 = M1->n[1][0];
    mp_size_t m11 = M1->n[1][1];
    mp_size_t an = n;
    /* 数对为零填充公共长度表示，b 真实长度显式信息不可得，须扫描
       （下限不变量保证 bn >= fl，扫描量有界） */
    mp_size_t bn = lmmp_hgcd_norm_(bp, n);

    /* 结果 limb 上界：new_a < B^min(an-m00, bn-m10)+1 等 */
    mp_size_t un = (an > m00 ? an - m00 : 1);
    if (bn - m10 < un) un = bn > m10 ? bn - m10 : 1;
    mp_size_t vn = (an > m01 ? an - m01 : 1);
    if (bn > m11 && bn - m11 < vn) vn = bn - m11;
    mp_size_t nn = un > vn ? un : vn;
    ++nn;

    mp_size_t rn = lmmp_fft_next_size_(nn + 1);
    if (rn <= lmmp_hgcd_matrix_maxlen_(M1) || rn > n + 1) {
        *blen = 0;
        sc->used = mark;
        return 0; /* 尺寸关系异常（防御） */
    }

    /* 折叠与填充 */
    mp_ptr fa = lmmp_hgcd_salloc_(sc, rn);
    mp_ptr fb = lmmp_hgcd_salloc_(sc, rn);
    lmmp_hgcd_fold_mod_(fa, ap, an, rn);
    lmmp_hgcd_fold_mod_(fb, bp, bn, rn);

    mp_ptr t11 = lmmp_hgcd_salloc_(sc, rn);
    mp_ptr t01 = lmmp_hgcd_salloc_(sc, rn);
    mp_ptr t00 = lmmp_hgcd_salloc_(sc, rn);
    mp_ptr t10 = lmmp_hgcd_salloc_(sc, rn);
    /*
    fa 与 fb 各参与两次模乘（分别乘两个矩阵元素），用缓存版梅森乘法
    复用其前向变换，第二次模乘省去一次全尺寸前向变换。cache 契约
    要求后续调用与 init 的操作数等长，而逐元素长度可能相差（矩阵元素
    平衡，通常 0~2 limb）：显式补零对齐到 ml
    */
    mp_size_t ml1 = LMMP_MAX(m00, m01);
    mp_size_t ml2 = LMMP_MAX(m10, m11);
    lmmp_zero(M1->m[0][0] + m00, ml1 - m00);
    lmmp_zero(M1->m[0][1] + m01, ml1 - m01);
    lmmp_zero(M1->m[1][0] + m10, ml2 - m10);
    lmmp_zero(M1->m[1][1] + m11, ml2 - m11);
    fft_gr_cache ca, cb;
    lmmp_mul_mersenne_cache_init_(t11, rn, M1->m[1][1], ml2, fa, rn, &ca);
    lmmp_mul_mersenne_cache_(t10, M1->m[1][0], &ca);
    lmmp_mul_mersenne_cache_init_(t01, rn, M1->m[0][1], ml1, fb, rn, &cb);
    lmmp_mul_mersenne_cache_(t00, M1->m[0][0], &cb);
    lmmp_fft_gr_cache_free_(&ca);
    lmmp_fft_gr_cache_free_(&cb);

    /* 模减：x - y (mod B^rn-1)；借位回绕 +1 */
    mp_ptr va, vb;
    mp_limb_t cy;
    if (detsign > 0) {
        va = t11; vb = t00;
        cy = lmmp_sub_n_(va, t11, t01, rn);
        if (cy) lmmp_inc(va);
        cy = lmmp_sub_n_(vb, t00, t10, rn);
        if (cy) lmmp_inc(vb);
    } else {
        va = t01; vb = t10;
        cy = lmmp_sub_n_(va, t01, t11, rn);
        if (cy) lmmp_inc(va);
        cy = lmmp_sub_n_(vb, t10, t00, rn);
        if (cy) lmmp_inc(vb);
    }

    /* 真值校验：结果必须落在 nn limb 内（负值回绕会产生接近 rn 的长度） */
    /* 模减回绕结果无进位信息，长度须扫描（数对中间量） */
    mp_size_t alen = lmmp_hgcd_norm_(va, rn);
    mp_size_t lb = lmmp_hgcd_norm_(vb, rn);
    if (alen > nn || lb > nn) {
        *blen = 0;
        sc->used = mark;
        return 0; /* 过冲 */
    }

    mp_size_t rn_out = alen > lb ? alen : lb;
    if (rn_out == 0) {
        *blen = 0;
        sc->used = mark;
        return 0;
    }
    *blen = lb;
    lmmp_copy(dap, va, alen);
    lmmp_zero(dap + alen, rn_out - alen);
    lmmp_copy(dbp, vb, lb);
    lmmp_zero(dbp + lb, rn_out - lb);
    sc->used = mark;
    return rn_out;
}

/**
 * @brief hgcd 递归核：归约至约 n/2 规模
 * @param M 变换矩阵：入口应为单位矩阵，出口累积全部归约变换（元素零填充至 M->n）
 * @param ap 较大分量输入兼输出数组：输入长度为 n 个limb（ap[n-1]>0），输出写入 rn 个limb）
 * @param bp 较小分量输入兼输出数组：输入长度为 n 个limb（允许高位为零，输出写入 rn 个limb）
 * @param n ap,bp 的公共输入长度
 * @param sc 临时空间（容量由 lmmp_hgcd_scratch_size_ 给出）
 * @param detsign 出参，返回 det(M) 的符号
 * @param appr 1 表示近似模式：仅保证 M 累积至足够规模，
 *             阶段 B 为纯矩阵复合（无数对修补）并立即返回，数对只需服务于自身
 *             步进；调用方走 lmmp_hgcd_apply_mod_（其不需要递归数对输出）时使用
 * @return 归约后公共长度（appr=0 时一般 <= n/2+2）；0 表示未做任何归约（M 保持单位矩阵）
 * @warning M!=NULL, sc!=NULL, ap!=NULL, bp!=NULL, sep(ap,bp), a>b>0, n>=3,
 *          M->alloc>=n+2
 * @note appr=0（精确模式）维持数对精确语义；appr=1 时数对最终停留在约 3n/4，
 *       仅供内部步进使用，调用方不得依赖其值。子递归模式选择：本层用
 *       apply_mod 合并（不需要子数对）时子递归可为 appr；用 adjust（需要子数对
 *       的精确归约结果）时子递归必须精确
 */
static mp_size_t lmmp_hgcd_core_(
    lmmp_hgcd_matrix_t*  M,
    mp_ptr               ap,
    mp_ptr               bp,
    mp_size_t            n,
    lmmp_hgcd_scratch_t* sc,
    int*                 detsign,
    int                  appr
) {
    *detsign = 1; /* 入口为单位矩阵 */
    lmmp_debug_assert(M->alloc >= n + 2);

    mp_size_t s = n / 2 + 1;
    mp_size_t fl = s + 1;
    if (fl > n - 1) {
        fl = n - 1; /* 极小规模的下限放宽（不会作为递归子问题出现） */
    }
    if (n <= fl) {
        return 0; /* n <= 2，无归约意义 */
    }
    int success = 0;
    mp_size_t n0 = n;

    if (n0 > HGCD_RECURSE_THRESHOLD) {
        /* 本层合并方式：模乘合并不需要子递归的数对输出，子递归可用 appr 模式 */
        int use_mod = (n0 >= HGCD_MODMUL_THRESHOLD);

        /* 阶段 A：对高半部分递归，规模降至约 3n/4 */
        mp_size_t p = n0 / 2;
        if (!lmmp_zero_q_(bp + p, n0 - p)) { /* b 高半存在非零才可递归（存在性检查，非求长度） */
            mp_size_t mark = sc->used;
            mp_size_t hn = n0 - p;
            /* 高位副本恰 hn limbs：递归核的数对读写均不超出其公共长度 */
            mp_ptr ha = lmmp_hgcd_salloc_(sc, hn);
            mp_ptr hb = lmmp_hgcd_salloc_(sc, hn);
            lmmp_copy(ha, ap + p, hn);
            lmmp_copy(hb, bp + p, hn);
            lmmp_hgcd_matrix_t M1;
            mp_ptr M1buf = lmmp_hgcd_salloc_(sc, 4 * (hn + 2));
            lmmp_hgcd_matrix_init_buf_(&M1, M1buf, hn + 2);
            int dsub = 1;
            mp_size_t nn = lmmp_hgcd_core_(&M1, ha, hb, hn, sc, &dsub, use_mod);
            if (nn > 0) {
                /*
                高位直写 + 低位修补合入完整数对，校验"较小分量长度 > M1 最大元素长度"
                后才提交，否则视为过冲，放弃递归成果（保持原数对，交由步进循环处理）
                */
                mp_size_t mark2 = sc->used;
                mp_ptr sa = lmmp_hgcd_salloc_(sc, n0 + 1);
                mp_ptr sb = lmmp_hgcd_salloc_(sc, n0 + 1);
                mp_size_t rn;
                mp_size_t blen;
                if (use_mod) {
                    rn = lmmp_hgcd_apply_mod_(&M1, dsub, sa, sb, &blen, ap, bp, n0, sc);
                } else {
                    rn = lmmp_hgcd_adjust_(&M1, dsub, sa, sb, &blen, ha, hb, nn, ap, bp, n0, p, sc);
                }
                mp_size_t maxlen = lmmp_hgcd_matrix_maxlen_(&M1);
                if (rn > 0 && rn < n0 && blen > maxlen) {
                    if (lmmp_cmp_(sa, sb, rn) >= 0) {
                        lmmp_copy(ap, sa, rn);
                        lmmp_copy(bp, sb, rn);
                    } else {
                        /* 规范序翻转：M1 右乘列交换（det 变号），拷贝方向对调 */
                        lmmp_hgcd_matrix_swap_cols_(&M1);
                        dsub = -dsub;
                        lmmp_copy(ap, sb, rn);
                        lmmp_copy(bp, sa, rn);
                    }
                    /* [rn,n0) 为无效残留：后续所有读写均以新公共长度 rn 为界 */
                    n = rn;
                    lmmp_hgcd_matrix_mul_(M, &M1, sc);
                    *detsign *= dsub;
                    success = 1;
                }
                sc->used = mark2;
            }
            sc->used = mark;
        }

        /* 桥接：步进至 3n0/4 + 1（通常只需常数步） */
        mp_size_t n2 = (3 * n0) / 4 + 1;
        while (n > n2) {
            mp_size_t nn = lmmp_hgcd_step_(M, ap, bp, n, fl, sc, detsign);
            if (nn == 0) {
                return success ? n : 0;
            }
            n = nn;
            success = 1;
        }

        /* 阶段 B：再次对高半递归，规模降至约 fl */
        if (n > fl + 2) {
            mp_size_t p2 = 2 * fl - n + 1;
            if (!lmmp_zero_q_(bp + p2, n - p2)) { /* 同上：存在性检查 */
                mp_size_t mark = sc->used;
                mp_size_t hn = n - p2;
                /* 高位副本恰 hn limbs：递归核的数对读写均不超出其公共长度 */
                mp_ptr ha = lmmp_hgcd_salloc_(sc, hn);
                mp_ptr hb = lmmp_hgcd_salloc_(sc, hn);
                lmmp_copy(ha, ap + p2, hn);
                lmmp_copy(hb, bp + p2, hn);
                lmmp_hgcd_matrix_t M1;
                mp_ptr M1buf = lmmp_hgcd_salloc_(sc, 4 * (hn + 2));
                lmmp_hgcd_matrix_init_buf_(&M1, M1buf, hn + 2);
                int dsub = 1;
                /*
                appr 模式的阶段 B 为纯矩阵复合：递归成果只入 M，数对不修补，
                复合完成即返回（矩阵规模已足够，调用方不依赖数对）；
                容量守卫不满足时直接放弃本阶段（不落入精确路径，子数对不精确）。
                精确模式下子递归模式须与阶段 B 自身的合并方式一致（按当前 n 判断）
                */
                if (appr) {
                    mp_size_t nn = lmmp_hgcd_core_(&M1, ha, hb, hn, sc, &dsub, 1);
                    if (nn > 0 && lmmp_hgcd_matrix_maxlen_(M) + lmmp_hgcd_matrix_maxlen_(&M1) + 4 <= M->alloc) {
                        lmmp_hgcd_matrix_mul_(M, &M1, sc);
                        *detsign *= dsub;
                        sc->used = mark;
                        return n; /* 数对停留在约 3n/4，仅供本层步进使用过 */
                    }
                    sc->used = mark;
                } else {
                    mp_size_t nn = lmmp_hgcd_core_(&M1, ha, hb, hn, sc, &dsub, n >= HGCD_MODMUL_THRESHOLD);
                    if (nn > 0) {
                        mp_size_t mark2 = sc->used;
                        mp_ptr sa = lmmp_hgcd_salloc_(sc, n + 1);
                        mp_ptr sb = lmmp_hgcd_salloc_(sc, n + 1);
                        mp_size_t rn;
                        mp_size_t blen;
                        if (n >= HGCD_MODMUL_THRESHOLD) {
                            rn = lmmp_hgcd_apply_mod_(&M1, dsub, sa, sb, &blen, ap, bp, n, sc);
                        } else {
                            rn = lmmp_hgcd_adjust_(&M1, dsub, sa, sb, &blen, ha, hb, nn, ap, bp, n, p2, sc);
                        }
                        mp_size_t maxlen = lmmp_hgcd_matrix_maxlen_(&M1);
                        if (rn > 0 && rn < n && blen > maxlen) {
                            if (lmmp_cmp_(sa, sb, rn) >= 0) {
                                lmmp_copy(ap, sa, rn);
                                lmmp_copy(bp, sb, rn);
                            } else {
                                lmmp_hgcd_matrix_swap_cols_(&M1);
                                dsub = -dsub;
                                lmmp_copy(ap, sb, rn);
                                lmmp_copy(bp, sa, rn);
                            }
                            /* [rn,n) 为无效残留：后续所有读写均以新公共长度 rn 为界 */
                            n = rn;
                            lmmp_hgcd_matrix_mul_(M, &M1, sc);
                            *detsign *= dsub;
                            success = 1;
                        }
                        sc->used = mark2;
                    }
                    sc->used = mark;
                }
            }
        }
    }

    /* 基础步进循环：降至下限 */
    while (n > fl) {
        mp_size_t nn = lmmp_hgcd_step_(M, ap, bp, n, fl, sc, detsign);
        if (nn == 0) {
            return success ? n : 0;
        }
        n = nn;
        success = 1;
    }
    return success ? n : 0;
}

void lmmp_hgcd_matrix_init_(lmmp_hgcd_matrix_t* M, mp_size_t alloc) {
    lmmp_param_assert(M != NULL);
    lmmp_param_assert(alloc > 0);
    mp_ptr buf = (mp_ptr)lmmp_alloc((size_t)4 * alloc * sizeof(mp_limb_t));
    lmmp_hgcd_matrix_init_buf_(M, buf, alloc);
}

void lmmp_hgcd_matrix_free_(lmmp_hgcd_matrix_t* M) {
    lmmp_param_assert(M != NULL);
    lmmp_free(M->m[0][0]);
    M->m[0][0] = NULL;
    M->m[0][1] = NULL;
    M->m[1][0] = NULL;
    M->m[1][1] = NULL;
    M->n[0][0] = M->n[0][1] = M->n[1][0] = M->n[1][1] = 0;
    M->alloc = 0;
}

mp_size_t lmmp_hgcd_(lmmp_hgcd_matrix_t* M, mp_ptr ap, mp_ptr bp, mp_size_t n) {
    lmmp_param_assert(M != NULL && ap != NULL && bp != NULL);
    lmmp_param_assert(n > 0);
    lmmp_debug_assert(M->alloc >= n + 2);
    lmmp_param_assert(ap[n - 1] != 0);
    if (lmmp_cmp_(ap, bp, n) <= 0) {
        return 0; /* a == b：gcd 即自身，无归约意义；a < b 违反契约 */
    }
    /* 一次性 scratch（矩阵元素缓冲需 n+2 容量，由调用方保证） */
    mp_size_t scsz = lmmp_hgcd_scratch_size_(n) + 2 * n + 16;
    mp_ptr scbuf = (mp_ptr)lmmp_alloc((size_t)scsz * sizeof(mp_limb_t));
    lmmp_hgcd_scratch_t sc = {scbuf, scsz, 0};
    int dtop = 1;
    mp_size_t rn = lmmp_hgcd_core_(M, ap, bp, n, &sc, &dtop, 0);
    lmmp_free(scbuf);
    return rn;
}

mp_size_t lmmp_gcd_hgcd_(mp_ptr dst, mp_srcptr ap, mp_size_t an, mp_srcptr bp, mp_size_t bn) {
    lmmp_param_assert(dst != NULL && ap != NULL && bp != NULL);
    lmmp_param_assert(an >= bn && bn > 0);

    /* 工作副本零填充至统一容量（下方的 cap 长度比较与核心归约依赖该填充） */
    mp_size_t cap = an + 2;
    mp_ptr wa = (mp_ptr)lmmp_alloc((size_t)cap * sizeof(mp_limb_t));
    mp_ptr wb = (mp_ptr)lmmp_alloc((size_t)cap * sizeof(mp_limb_t));
    lmmp_copy(wa, ap, an);
    lmmp_zero(wa + an, cap - an);
    lmmp_copy(wb, bp, bn);
    lmmp_zero(wb + bn, cap - bn);

    mp_size_t rn;
    int cmp = lmmp_cmp_(wa, wb, cap);
    if (cmp == 0) {
        rn = an; /* wa 为归一化输入 ap 的副本，真实长度即 an */
        lmmp_copy(dst, wa, rn);
        lmmp_free(wa);
        lmmp_free(wb);
        return rn;
    } else if (cmp < 0) {
        LMMP_SWAP(wa, wb, mp_ptr);
    }

    /* 一次性分配：scratch 与顶层矩阵缓冲，循环内复用（矩阵容量按最大 cur） */
    mp_size_t scsz = lmmp_hgcd_scratch_size_(an);
    mp_ptr scbuf = (mp_ptr)lmmp_alloc((size_t)scsz * sizeof(mp_limb_t));
    lmmp_hgcd_scratch_t sc = {scbuf, scsz, 0};
    mp_ptr mbuf = (mp_ptr)lmmp_alloc((size_t)4 * (an + 2) * sizeof(mp_limb_t));

    mp_size_t cur = an;
    for (;;) {
        if (cur <= HGCD_RECURSE_THRESHOLD) {
            break;
        }
        lmmp_hgcd_matrix_t M;
        lmmp_hgcd_matrix_init_buf_(&M, mbuf, cur + 2);
        int dtop = 1;
        mp_size_t nn = lmmp_hgcd_core_(&M, wa, wb, cur, &sc, &dtop, 0);
        if (nn == 0 || nn >= cur) {
            break; /* 无归约（防御性） */
        }
        /* [nn,cur) 为无效残留：循环内所有读写均以返回长度为界 */
        cur = nn;
        if (lmmp_zero_q_(wb, cur)) {
            break; /* b 归约为 0：gcd = a（存在性检查） */
        }
        if (lmmp_cmp_(wa, wb, cur) < 0) {
            LMMP_SWAP(wa, wb, mp_ptr); /* 指针交换 */
        }
    }

    lmmp_free(scbuf);
    lmmp_free(mbuf);

    mp_size_t ra = lmmp_hgcd_norm_(wa, cur);
    mp_size_t rb = lmmp_hgcd_norm_(wb, cur);
    if (rb == 0) {
        rn = ra;
        lmmp_copy(dst, wa, ra);
    } else {
        rn = lmmp_gcd_lehmer_(dst, wa, ra, wb, rb);
    }
    lmmp_free(wa);
    lmmp_free(wb);
    return rn;
}
