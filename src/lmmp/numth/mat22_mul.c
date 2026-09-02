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

实现路径（按元素最大长度 mx 分派）：
- mx < MAT22_MUL_STRASSEN_THRESHOLD：乘法 basecase（8 乘）；
  平方对称 basecase（5 乘）；
- 否则：Winograd-Strassen。乘法 7 乘；平方利用 A*A 组合量对称
  （t_i = s_i），7 乘中 4 次为平方（lmmp_sqr_ 快于 lmmp_mul_）。
组合量与乘积安置在变宽槽位：每个组合量恰被一个乘积消耗，乘积依次
回填释放的槽位，全程无额外缓冲；各槽宽度按驻留内容的最大长度界限
逐段推导（见各函数 @note 的界限表），合计：乘法 8*(mx(a)+mx(b))+16、
平方 15*mx(a)+14 limbs，替代早期的等宽 9*(2*mx+4)。

符号性利用：输入矩阵元素恒非负，s1/t1 类纯加组合量与 p4/p5 类
原元素积走无符号助手（mat22_add_/mat22_mul_/mat22_sqr_），仅
减法组合量（s2/s4）与含其乘积、U 链需要带符号表示（mat22_addsg_/
mat22_mulsg_）；平方对符号不敏感，带符号组合量取绝对长度后同样
走无符号平方。

历史注记：FFT 缓存乘法路径（8 乘 + lmmp_mul_fft_cache_ 复用短侧
前向变换）曾在此实现并经实测证伪，仅省 4 次前向变换不敌第 8 次乘法
的点积与逆变换开销，且前向变换份额随规模衰减（34%@2000 ->
17%@16000），渐进劣于 Strassen，已删除。

另一族曾探索的方案：Strassen 的 7 个乘积共享前向变换
（各因子一次前向 + 逐积一次点乘与逆变换，梅森域）。经核算证伪：
7 个乘积的 14 个因子两两互异（Strassen 正是以不共享操作数换取
乘法次数），无变换可复用，与逐积 lmmp_mul_（CRT 拆分：两半尺寸
模乘的变换总量与之等价）无净差异，已回退。hgcd 侧 apply_mod 的
fa/fb 复用（每因子参与两次乘法）是变换共享的真实机会，既有
lmmp_mul_mersenne_cache_ 系列已实现；频域线性组合（两积之差的
系数域模减，省一次逆变换）因系数域 ±1 单位簿记与装配校正不兼容
（负真值固定偏差）未采用。

第二族（双缓存频域 basecase）。basecase 的 8 个乘积中每元素恰出现
两次（a_i0 参与 p_i0 与 p_i1，b_0j 参与 p_0j 与 p_1j），
缓存各元素的前向变换后每积只付点乘与逆变换，这是变换共享的真实
机会（Strassen 的 14 个因子两两互异，无共享）。曾完整实现两个变体
并验证正确性：

变体一（全尺寸单域）：8 元素各一次梅森域前向（rn = next_size(2mx+1)，
单模精确积免 CRT），8 积各一次点乘逆变换。实测单积成本与 lmmp_mul_
持平（0.82~1.04，随 next_size 量化波动——全尺寸各分量付 ~2.2 倍罚
金），mat22 级元素 2800~9500 带内快 5~10%，带外劣化（1700~2600 慢
14~30%，>=10000 慢 7~15%）。

变体二（CRT 半尺寸双域，即 lmmp_mul_ 的内部结构）：8 元素在梅森
（B^hn-1）与费马（B^hn+1）两域各缓存一次前向（hn = next_size(mx)，
单域成本约完整乘法的 44~55%），16 次前向对比逐积 lmmp_mul_ 的 32
次半尺寸前向省一半，逐积双域点乘逆变换后按 lmmp_mul_fft_ 组合式
重建。消除了变体一的全尺寸罚金：收益带扩至 [3000, 17000]，带内快
2~8%，12000/16000 亦小幅领先；但 1800~2600 仍慢 7~15%（小尺寸前
向份额低，8 乘比 Strassen 7 乘多付点乘），20000 起内存足迹侵蚀。

两变体 hgcd 端到端多轮交错 A/B 均实测中性（±1%，11k~51k 与
5k~16k 两档）：结构性上限为——共享仅省前向变换（每积约 20% 的一
半），却多付第 8 个乘积（约 -14%）与拷贝/分配开销，净得 2~8%；
而 hgcd 矩阵乘工作量约六成在带外（顶层元素 ~n/2 超上界），带内
约 43%×6% ≈ 1% 总量，低于噪声地板。已移除，此处存档防止重复投入。

实现技术要点（若复活可直接取用）：缓存变换的复用需整块拷贝 + 指
针按基址差重定位 + "空闲块"随迁（蝶形交换使变换结束后恰有一个不
被引用的空闲块，其位置须与指针一并重定位，否则逆变换蝶形会覆写
仍被引用的系数）；两域系数长公式有双重差异——费马域基式 +2 且规
整粒度为 2^k（旋钮位移 i*n>>k 需 n 整除 K），梅森域为 2^(k-1)，
统一时极易踩坑（本轮即因误统一规整粒度致费马残差错误）。逐积
CRT 组合要求该积两因子长度和 > hn，元素极度不均时须回退
lmmp_mul_。若未来有逐元素乘积密集且元素规模居中（3k~17k）的新
消费者，本方案可净赚 2~8%。
*/

#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/mat22_mul.h"

/**
 * @brief 带符号加法
 * @param r 结果指针（长度为 maxabs(xs,ys)+1）
 * @param x 第一个加数指针，长度为abs(xs)
 * @param xs 绝对值表示第一个加数指针长度，符号表示此数正负
 * @param y 第二个加数指针，长度为abs(ys)
 * @param ys 绝对值表示第二个加数指针长度，符号表示此数正负
 * @warning r!=NULL, x!=NULL, y!=NULL, eqsep(r,[xs|ys])
 * @return 绝对值表示结果的实际长度，符号表示符号
 */
static mp_ssize_t mat22_addsg_(mp_ptr r, mp_srcptr x, mp_ssize_t xs, mp_srcptr y, mp_ssize_t ys) {
    int xneg = xs < 0;
    int yneg = ys < 0;
    mp_size_t xa = xs < 0 ? (mp_size_t)(-xs) : (mp_size_t)xs;
    mp_size_t ya = ys < 0 ? (mp_size_t)(-ys) : (mp_size_t)ys;
    if (xa == 0) {
        if (r != y) {
            lmmp_copy(r, y, ya);
        }
        return ys;
    }
    if (ya == 0) {
        if (r != x) {
            lmmp_copy(r, x, xa);
        }
        return xs;
    }
    if (xneg == yneg) {
        /* 同号：较长春在前相加，进位落盘 */
        mp_srcptr lp = xa >= ya ? x : y;
        mp_srcptr sp = xa >= ya ? y : x;
        mp_size_t m = xa >= ya ? xa : ya;
        mp_limb_t c = lmmp_add_(r, lp, m, sp, xa >= ya ? ya : xa);
        if (c != 0) {
            r[m] = c;
            ++m;
        }
        return xneg ? -(mp_ssize_t)m : (mp_ssize_t)m;
    }
    int xbig = xa > ya || (xa == ya && lmmp_cmp_(x, y, xa) >= 0);
    mp_srcptr big = xbig ? x : y;
    mp_srcptr sml = xbig ? y : x;
    mp_size_t ba = xbig ? xa : ya;
    mp_size_t sa = xbig ? ya : xa;
    lmmp_sub_(r, big, ba, sml, sa);

    while (ba > 0 && r[ba - 1] == 0) {
        --ba;
    }
    int rneg = xbig ? xneg : yneg;
    return rneg ? -(mp_ssize_t)ba : (mp_ssize_t)ba;
}

/**
 * @brief 带符号乘法
 * @param r 结果指针（长度为 abs(xs)+abs(ys)）
 * @param x 第一个乘数指针，长度为abs(xs)
 * @param xs 绝对值表示第一个乘数指针长度，符号表示此数正负
 * @param y 第二个乘数指针，长度为abs(ys)
 * @param ys 绝对值表示第二个乘数指针长度，符号表示此数正负
 * @warning r!=NULL, x!=NULL, y!=NULL, sep(r,[xs|ys])
 * @return 绝对值表示结果的实际长度，符号表示符号
 */
static mp_ssize_t mat22_mulsg_(mp_ptr r, mp_srcptr x, mp_ssize_t xs, mp_srcptr y, mp_ssize_t ys) {
    int neg = (xs < 0) != (ys < 0);
    mp_size_t xa = xs < 0 ? (mp_size_t)(-xs) : (mp_size_t)xs;
    mp_size_t ya = ys < 0 ? (mp_size_t)(-ys) : (mp_size_t)ys;
    if (xa == 0 || ya == 0) {
        return 0;
    }
    if (xa >= ya) {
        lmmp_mul_(r, x, xa, y, ya);
    } else {
        lmmp_mul_(r, y, ya, x, xa);
    }
    mp_size_t l = xa + ya;
    l -= (r[l - 1] == 0);
    return neg ? -(mp_ssize_t)l : (mp_ssize_t)l;
}

/**
 * @brief 无符号加法
 * @param r 结果指针（长度为 max(xs,ys)+1）
 * @param x 第一个加数指针，长度为xs
 * @param xs 第一个加数指针长度
 * @param y 第二个加数指针，长度为ys
 * @param ys 第二个加数指针长度
 * @warning r!=NULL, x!=NULL, y!=NULL, eqsep(r,[xs|ys])
 * @return 结果的实际长度
 * @note 零操作数（长度 0）时和为另一操作数，拷贝返回其长度
 */
static mp_size_t mat22_add_(mp_ptr r, mp_srcptr x, mp_size_t xs, mp_srcptr y, mp_size_t ys) {
    if (xs == 0) {
        if (r != y) {
            lmmp_copy(r, y, ys);
        }
        return ys;
    }
    if (ys == 0) {
        if (r != x) {
            lmmp_copy(r, x, xs);
        }
        return xs;
    }
    if (xs > ys) {
        mp_limb_t c = lmmp_add_(r, x, xs, y, ys);
        r[xs] = c;
        return xs + ((c > 0) ? 1 : 0);
    } else {
        mp_limb_t c = lmmp_add_(r, y, ys, x, xs);
        r[ys] = c;
        return ys + ((c > 0) ? 1 : 0);
    }
}

/**
 * @brief 无符号乘法
 * @param r 结果指针（长度为 abs(xs)+abs(ys)）
 * @param x 第一个乘数指针，长度为abs(xs)
 * @param xs 第一个乘数指针长度
 * @param y 第二个乘数指针，长度为abs(ys)
 * @param ys 第二个乘数指针长度
 * @warning r!=NULL, x!=NULL, y!=NULL, sep(r,[xs|ys])
 * @return 结果的实际长度
 */
static mp_size_t mat22_mul_(mp_ptr r, mp_srcptr x, mp_size_t xs, mp_srcptr y, mp_size_t ys) {
    if (xs == 0 || ys == 0) {
        return 0;
    }
    if (xs >= ys) {
        lmmp_mul_(r, x, xs, y, ys);
    } else {
        lmmp_mul_(r, y, ys, x, xs);
    }
    mp_size_t l = xs + ys;
    l -= (r[l - 1] == 0);
    return l;
}

/**
 * @brief 无符号平方
 * @param r 结果指针（长度为 2*xs）
 * @param x 源指针，长度为xs
 * @param xs 源指针长度
 * @warning r!=NULL, x!=NULL, sep(r,x)
 * @return 结果的实际长度
 * @note xs==0 时结果为零（lmmp_sqr_ 对零长度不安全，须在此拦截）
 */
static mp_size_t mat22_sqr_(mp_ptr r, mp_srcptr x, mp_size_t xs) {
    if (xs == 0) {
        return 0;
    }
    lmmp_sqr_(r, x, xs);
    mp_size_t l = 2 * xs;
    l -= (r[l - 1] == 0);
    return l;
}

/**
 * @brief 元素长度最大值
 * @param a 输入矩阵
 * @warning a!=NULL
 * @return 返回 a 矩阵的元素长度最大值
 */
static mp_size_t mat22_maxlen_(const lmmp_mat22_t* a) {
    mp_size_t mx = 0;
    if (a->n[0][0] > mx)
        mx = a->n[0][0];
    if (a->n[0][1] > mx)
        mx = a->n[0][1];
    if (a->n[1][0] > mx)
        mx = a->n[1][0];
    if (a->n[1][1] > mx)
        mx = a->n[1][1];
    return mx;
}

static mp_size_t mat22_abslen_(mp_ssize_t l) {
    return l < 0 ? (mp_size_t)(-l) : (mp_size_t)l;
}

/**
 * @brief 计算 D <- A*B（2x2 非负矩阵朴素乘法）
 * @param dst 结果矩阵，各个元素结果写入 dst->p 中的数组，对应实际长度被写入到 dst->n 中
 * @param a 左矩阵（元素长度为 a->n[i][j]，可与 dst 别名，即支持 D <- D*B）
 * @param b 右矩阵（元素长度为 b->n[i][j]，不可与 dst 别名）
 * @param tp 临时空间（4*(mx(a)+mx(b)) 个limb）
 * @warning dst!=NULL, a!=NULL, b!=NULL, tp!=NULL, sep(dst,b), eqsep(dst,a),
 *          dst 各元素缓冲容量至少为 mx(a)+mx(b)+1
 */
static void mat22_mul_basecase_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, const lmmp_mat22_t* b, mp_ptr tp) {
    mp_size_t pn = mat22_maxlen_(a) + mat22_maxlen_(b);
    for (int i = 0; i < 2; ++i) {
        mp_ptr pA = tp, pB = tp + pn, pC = tp + 2 * pn, pD = tp + 3 * pn;
        mp_size_t lA = mat22_mul_(pA, a->p[i][0], a->n[i][0], b->p[0][0], b->n[0][0]);
        mp_size_t lB = mat22_mul_(pB, a->p[i][1], a->n[i][1], b->p[1][0], b->n[1][0]);
        mp_size_t lC = mat22_mul_(pC, a->p[i][0], a->n[i][0], b->p[0][1], b->n[0][1]);
        mp_size_t lD = mat22_mul_(pD, a->p[i][1], a->n[i][1], b->p[1][1], b->n[1][1]);
        /* a 行元素（可能即 dst 行缓冲）的读取已完毕，结果直写 */
        dst->n[i][0] = mat22_add_(dst->p[i][0], pA, lA, pB, lB);
        dst->n[i][1] = mat22_add_(dst->p[i][1], pC, lC, pD, lD);
    }
}

/*
   Winograd-Strassen 乘法

    A = /a00 a01\，B = /b00 b01\。组合量与乘积安置在 9 个变宽槽位：每个组合量
        \a10 a11/      \b10 b11/

     s1 = a11+a01   t1 = b11+b01    p1 = s1*t1   p2 = s2*t2   p3 = s3*t3
     s2 = a11-a10   t2 = b11-b10    p4 = a00*b00 p5 = a01*b10
     s3 = s2+a01    t3 = t2+b01     p6 = s4*b01  p7 = a10*t4
     s4 = s3-a00    t4 = t3-b00
     U1 = p3+p5
     U2 = p1-U1
     U3 = U1-p2

     C00 = p4+p5
     C01 = U3-p6
     C10 = U2-p7
     C11 = p2+U2

     C = A*B = /c00 c01\
               \c10 c11/
*/

/**
 * @brief 计算 D <- A*B（2x2 非负矩阵 Winograd-Strassen 乘法，7 次大数乘）
 * @param dst 结果矩阵，各个元素结果写入 dst->p 中的数组，对应实际长度被写入到 dst->n 中
 * @param a 左矩阵（元素长度为 a->n[i][j]，可与 dst 别名，即支持 D <- D*B）
 * @param b 右矩阵（元素长度为 b->n[i][j]，不可与 dst 别名）
 * @param tp 临时空间（8*(mx(a)+mx(b))+16 个limb）
 * @warning dst!=NULL, a!=NULL, b!=NULL, tp!=NULL, sep(dst,b), eqsep(dst,a),
 *          dst 各元素缓冲容量至少为 mx(a)+mx(b)+4
 * @note 各段长度界限与槽位分配（mxa = mx(a)，mxb = mx(b)）：
 *
 *       [s1,mxa+1]     无符号加法，长度至多+1
 *       [s2,mxa]       带符号减法，长度不会增加
 *       [s3,mxa+1]     带符号加法，长度至多+1
 *       [s4,mxa+2]     带符号加法，长度至多+1
 *       t1~t4 同构（mxa 换为 mxb）
 *
 *       [p1,mxa+mxb+2] = [s1]*[t1]（乘积长度为因子长度之和）
 *       [p2,mxa+mxb]   = [s2]*[t2]
 *       [p3,mxa+mxb+2] = [s3]*[t3]
 *       [p4,mxa+mxb]   = a00*b00（原始元素非负，无符号乘法）
 *       [p5,mxa+mxb]   = a01*b10（同上）
 *       [p6,mxa+mxb+2] = [s4]*b01
 *       [p7,mxa+mxb+2] = a10*[t4]
 *
 *       [U1,mxa+mxb+3] = p3+p5（带符号加法，长度至多 max+1）
 *       [U2,mxa+mxb+4] = p1-U1
 *       [U3,mxa+mxb+4] = U1-p2
 *
 *       槽位（按驻留内容的最大写入范围计宽，顺序分配）：
 *       s1槽（后驻 p2）     mxa+mxb
 *       s2槽（后驻 p3,U1,U3）mxa+mxb+4
 *       s3槽（后驻 p6）     mxa+mxb+2
 *       s4槽（仅组合量）    mxa+2
 *       t1槽（后驻 p4）     mxa+mxb
 *       t2槽（后驻 p5）     mxa+mxb
 *       t3槽（后驻 p7）     mxa+mxb+2
 *       t4槽（仅组合量）    mxb+2
 *       空槽（驻 p1,U2）    mxa+mxb+4
 *       合计 8*(mxa+mxb)+16 limbs。最终结果直写 dst：C00 = p4+p5 为
 *       无符号加法；其余三个经带符号加法，瞬态写入范围至多
 *       mx(a)+mx(b)+4（U3/U2 的归一化长度由恒等式 U3=C01+p6、
 *       U2=C10+p7 与结果界限定于 mxa+mxb+3，加法至多再+1）
 */
static void mat22_mul_strassen_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, const lmmp_mat22_t* b, mp_ptr tp) {
    mp_size_t mxa = mat22_maxlen_(a);
    mp_size_t mxb = mat22_maxlen_(b);
    mp_size_t off = 0;
    mp_ptr s1 = tp + off;
    off += mxa + mxb; /* s1 槽，后驻 p2 */
    mp_ptr s2 = tp + off;
    off += mxa + mxb + 4; /* s2 槽，后驻 p3、U1、U3 */
    mp_ptr s3 = tp + off;
    off += mxa + mxb + 2; /* s3 槽，后驻 p6 */
    mp_ptr s4 = tp + off;
    off += mxa + 2; /* s4 槽，仅组合量 */
    mp_ptr t1 = tp + off;
    off += mxa + mxb; /* t1 槽，后驻 p4 */
    mp_ptr t2 = tp + off;
    off += mxa + mxb; /* t2 槽，后驻 p5 */
    mp_ptr t3 = tp + off;
    off += mxa + mxb + 2; /* t3 槽，后驻 p7 */
    mp_ptr t4 = tp + off;
    off += mxb + 2; /* t4 槽，仅组合量 */
    mp_ptr spare = tp + off; /* 空槽，驻 p1、U2，宽 mxa+mxb+4 */
    mp_ssize_t ns1, ns2, ns3, ns4, nt1, nt2, nt3, nt4;

    ns1 = mat22_add_(s1, a->p[1][1], a->n[1][1], a->p[0][1], a->n[0][1]);
    ns2 = mat22_addsg_(s2, a->p[1][1], (mp_ssize_t)a->n[1][1], a->p[1][0], -(mp_ssize_t)a->n[1][0]);
    ns3 = mat22_addsg_(s3, s2, ns2, a->p[0][1], (mp_ssize_t)a->n[0][1]);
    ns4 = mat22_addsg_(s4, s3, ns3, a->p[0][0], -(mp_ssize_t)a->n[0][0]);

    nt1 = mat22_add_(t1, b->p[1][1], b->n[1][1], b->p[0][1], b->n[0][1]);
    nt2 = mat22_addsg_(t2, b->p[1][1], (mp_ssize_t)b->n[1][1], b->p[1][0], -(mp_ssize_t)b->n[1][0]);
    nt3 = mat22_addsg_(t3, t2, nt2, b->p[0][1], (mp_ssize_t)b->n[0][1]);
    nt4 = mat22_addsg_(t4, t3, nt3, b->p[0][0], -(mp_ssize_t)b->n[0][0]);

    /* 乘积槽位：p1 用空槽；p2/p3/p6 回填已消耗的 s 槽；p4/p5/p7 占 t 槽。
       p1/p4/p5 的因子均非负（s1/t1 无符号和，a0j/bj0 原始元素），用无符号乘法 */
    mp_ptr p1 = spare, p2 = s1, p3 = s2, p4 = t1, p5 = t2, p6 = s3, p7 = t3;
    mp_ssize_t np1 = mat22_mul_(p1, s1, ns1, t1, nt1);
    mp_ssize_t np2 = mat22_mulsg_(p2, s2, ns2, t2, nt2); /* s2、t2 已消耗 */
    mp_ssize_t np3 = mat22_mulsg_(p3, s3, ns3, t3, nt3); /* s3、t3 已消耗 */
    mp_ssize_t np4 = mat22_mul_(p4, a->p[0][0], a->n[0][0], b->p[0][0], b->n[0][0]);
    mp_ssize_t np5 = mat22_mul_(p5, a->p[0][1], a->n[0][1], b->p[1][0], b->n[1][0]);
    mp_ssize_t np6 = mat22_mulsg_(p6, s4, ns4, b->p[0][1], (mp_ssize_t)b->n[0][1]); /* s4 已消耗 */
    mp_ssize_t np7 = mat22_mulsg_(p7, a->p[1][0], (mp_ssize_t)a->n[1][0], t4, nt4); /* t4 已消耗 */

    /* U 链安置在已消耗的槽位内 */
    mp_ssize_t nu1 = mat22_addsg_(p3, p3, np3, p5, np5);  /* U1 = p3+p5，存 p3 槽 */
    mp_ssize_t nu2 = mat22_addsg_(p1, p1, np1, p3, -nu1); /* U2 = p1-U1，存 p1 槽 */
    mp_ssize_t nu3 = mat22_addsg_(p3, p3, nu1, p2, -np2); /* U3 = U1-p2，存 p3 槽 */

    /* C00 = p4+p5 两非负积之和，无符号加法直写；其余带符号 */
    dst->n[0][0] = mat22_add_(dst->p[0][0], p4, np4, p5, np5);
    dst->n[0][1] = mat22_abslen_(mat22_addsg_(dst->p[0][1], p3, nu3, p6, -np6)); /* C01 = U3-p6 */
    dst->n[1][0] = mat22_abslen_(mat22_addsg_(dst->p[1][0], p1, nu2, p7, -np7)); /* C10 = U2-p7 */
    dst->n[1][1] = mat22_abslen_(mat22_addsg_(dst->p[1][1], p2, np2, p1, nu2));  /* C11 = p2+U2 */
}

/**
 * @brief 计算 D <- A*A（2x2 非负矩阵朴素平方，5 次大数乘）
 * @param dst 结果矩阵，各个元素结果写入 dst->p 中的数组，对应实际长度被写入到 dst->n 中
 * @param a 源矩阵（元素长度为 a->n[i][j]，可与 dst 别名）
 * @param tp 临时空间（5*mx(a)+2 个limb）
 * @warning dst!=NULL, a!=NULL, tp!=NULL, eqsep(dst,a),
 *          dst 各元素缓冲容量至少为 2*mx(a)+2
 * @note 各段长度界限与槽位分配（mxa = mx(a)，全部无符号）：
 *
 *       [s,mxa+1]       = a00+a11，无符号加法，长度至多+1
 *       [cross,2mxa]    = a01*a10
 *       [C01,2mxa+1]    = a01*s，即 [mxa]*[mxa+1]
 *       [C10,2mxa+1]    = a10*s，同上
 *       [对角元,2mxa+1] = a00^2/a11^2 [2mxa] 加 cross，长度至多+1
 *
 *       槽位（顺序分配）：cross 槽 2mxa、s 槽 mxa+1、tmp 中转槽
 *       2mxa+1（容纳最大乘积的写入范围）→ 合计 5mxa+2 limbs。
 *       别名安全性：s 与 cross 先行完成对 a 全部元素的复用读取；
 *       C01/C10 的因子 a01/a10 各仅参与自身结果与 cross，末读后经
 *       tmp 拷入 dst；对角元的直写发生在 a00/a11 末读之后
 */
static void mat22_sqr_basecase_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, mp_ptr tp) {
    mp_size_t mxa = mat22_maxlen_(a);
    mp_size_t off = 0;
    mp_ptr cross = tp + off;
    off += 2 * mxa; /* cross 槽 */
    mp_ptr s = tp + off;
    off += mxa + 1; /* s 槽 */
    mp_ptr tmp = tp + off; /* tmp 中转槽，宽 2mxa+1 */

    mp_size_t ls = mat22_add_(s, a->p[0][0], a->n[0][0], a->p[1][1], a->n[1][1]);
    mp_size_t lcross = mat22_mul_(cross, a->p[0][1], a->n[0][1], a->p[1][0], a->n[1][0]);

    mp_size_t l01 = mat22_mul_(tmp, a->p[0][1], a->n[0][1], s, ls);
    dst->n[0][1] = l01;
    lmmp_copy(dst->p[0][1], tmp, l01);
    mp_size_t l10 = mat22_mul_(tmp, a->p[1][0], a->n[1][0], s, ls);
    dst->n[1][0] = l10;
    lmmp_copy(dst->p[1][0], tmp, l10);

    mp_size_t l00 = mat22_sqr_(tmp, a->p[0][0], a->n[0][0]);
    dst->n[0][0] = mat22_add_(dst->p[0][0], tmp, l00, cross, lcross);
    mp_size_t l11 = mat22_sqr_(tmp, a->p[1][1], a->n[1][1]);
    dst->n[1][1] = mat22_add_(dst->p[1][1], tmp, l11, cross, lcross);
}

/**
 * @brief 计算 D <- A*A（2x2 非负矩阵 Winograd-Strassen 平方，7 次大数乘中 4 次为平方）
 * @param dst 结果矩阵，各个元素结果写入 dst->p 中的数组，对应实际长度被写入到 dst->n 中
 * @param a 源矩阵（元素长度为 a->n[i][j]，可与 dst 别名）
 * @param tp 临时空间（15*mx(a)+14 个limb）
 * @warning dst!=NULL, a!=NULL, tp!=NULL, eqsep(dst,a),
 *          dst 各元素缓冲容量至少为 2*mx(a)+4
 * @note 各段长度界限与槽位分配（mxa = mx(a)，组合量对称 t_i = s_i）：
 *
 *       [s1,mxa+1]     无符号加法，长度至多+1
 *       [s2,mxa]       带符号减法，长度不会增加
 *       [s3,mxa+1]     带符号加法，长度至多+1
 *       [s4,mxa+2]     带符号加法，长度至多+1
 *
 *       [p1,2mxa+2] = s1^2（s1 非负，无符号平方，平方对符号不敏感，
 *                    带符号组合量取绝对长度后同样走无符号平方）
 *       [p2,2mxa]   = s2^2
 *       [p3,2mxa+2] = s3^2
 *       [p4,2mxa]   = a00^2（原始元素非负，无符号平方）
 *       [p5,2mxa]   = a01*a10（无符号乘法）
 *       [p6,2mxa+2] = [s4]*a01（带符号乘法）
 *       [p7,2mxa+2] = a10*[s4]（带符号乘法）
 *
 *       [U1,2mxa+3] = p3+p5（带符号加法，长度至多 max+1）
 *       [U2,2mxa+4] = p1-U1
 *       [U3,2mxa+4] = U1-p2
 *
 *       槽位（按驻留内容的最大写入范围计宽，顺序分配）：
 *       s1槽（后驻 p2）      2mxa
 *       s2槽（后驻 p3,U1,U3）2mxa+4
 *       s3槽（后驻 p6）      2mxa+2
 *       s4槽（仅组合量）     mxa+2
 *       空槽（驻 p1,U2）     2mxa+4
 *       p4槽                2mxa
 *       p5槽                2mxa
 *       p7槽                2mxa+2
 *       合计 15mxa+14 limbs。结果直写 dst：C00 = p4+p5 无符号加法，
 *       其余带符号，瞬态写入范围至多 2*mx(a)+4（同乘法的恒等式论证）
 */
static void mat22_sqr_strassen_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, mp_ptr tp) {
    mp_size_t mxa = mat22_maxlen_(a);
    mp_size_t off = 0;
    mp_ptr s1 = tp + off;
    off += 2 * mxa; /* s1 槽，后驻 p2 */
    mp_ptr s2 = tp + off;
    off += 2 * mxa + 4; /* s2 槽，后驻 p3、U1、U3 */
    mp_ptr s3 = tp + off;
    off += 2 * mxa + 2; /* s3 槽，后驻 p6 */
    mp_ptr s4 = tp + off;
    off += mxa + 2; /* s4 槽，仅组合量 */
    mp_ptr spare = tp + off;
    off += 2 * mxa + 4; /* 空槽，驻 p1、U2 */
    mp_ptr p4 = tp + off;
    off += 2 * mxa; /* p4 槽 */
    mp_ptr p5 = tp + off;
    off += 2 * mxa; /* p5 槽 */
    mp_ptr p7 = tp + off; /* p7 槽，宽 2mxa+2 */
    mp_ssize_t ns1, ns2, ns3, ns4;

    ns1 = mat22_add_(s1, a->p[1][1], a->n[1][1], a->p[0][1], a->n[0][1]);
    ns2 = mat22_addsg_(s2, a->p[1][1], (mp_ssize_t)a->n[1][1], a->p[1][0], -(mp_ssize_t)a->n[1][0]);
    ns3 = mat22_addsg_(s3, s2, ns2, a->p[0][1], (mp_ssize_t)a->n[0][1]);
    ns4 = mat22_addsg_(s4, s3, ns3, a->p[0][0], -(mp_ssize_t)a->n[0][0]);

    /* 平方对符号不敏感：带符号组合量取绝对长度走无符号平方 */
    mp_ptr p1 = spare, p2 = s1, p3 = s2, p6 = s3;
    mp_ssize_t np1 = (mp_ssize_t)mat22_sqr_(p1, s1, ns1);
    mp_ssize_t np2 = (mp_ssize_t)mat22_sqr_(p2, s2, mat22_abslen_(ns2)); /* s2 已消耗 */
    mp_ssize_t np3 = (mp_ssize_t)mat22_sqr_(p3, s3, mat22_abslen_(ns3)); /* s3 已消耗 */
    mp_ssize_t np4 = (mp_ssize_t)mat22_sqr_(p4, a->p[0][0], a->n[0][0]);
    mp_ssize_t np5 = (mp_ssize_t)mat22_mul_(p5, a->p[0][1], a->n[0][1], a->p[1][0], a->n[1][0]);
    mp_ssize_t np6 = mat22_mulsg_(p6, s4, ns4, a->p[0][1], (mp_ssize_t)a->n[0][1]); /* s4 尚需用于 p7 */
    mp_ssize_t np7 = mat22_mulsg_(p7, a->p[1][0], (mp_ssize_t)a->n[1][0], s4, ns4); /* 对称 t4=s4，此后 s4 已消耗 */

    mp_ssize_t nu1 = mat22_addsg_(p3, p3, np3, p5, np5);  /* U1 = p3+p5 */
    mp_ssize_t nu2 = mat22_addsg_(p1, p1, np1, p3, -nu1); /* U2 = p1-U1 */
    mp_ssize_t nu3 = mat22_addsg_(p3, p3, nu1, p2, -np2); /* U3 = U1-p2 */
    /* C00 = p4+p5 两非负积之和，无符号加法直写；其余带符号
      （别名安全性同乘法：末次原元素读取在 np7） */
    dst->n[0][0] = mat22_add_(dst->p[0][0], p4, np4, p5, np5);
    dst->n[0][1] = mat22_abslen_(mat22_addsg_(dst->p[0][1], p3, nu3, p6, -np6)); /* C01 = U3-p6 */
    dst->n[1][0] = mat22_abslen_(mat22_addsg_(dst->p[1][0], p1, nu2, p7, -np7)); /* C10 = U2-p7 */
    dst->n[1][1] = mat22_abslen_(mat22_addsg_(dst->p[1][1], p2, np2, p1, nu2));  /* C11 = p2+U2 */
}

static int mat22_all_big_(const lmmp_mat22_t* x, mp_size_t th) {
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            if (x->n[i][j] < th) {
                return 0;
            }
        }
    }
    return 1;
}

void lmmp_mat22_mul_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, const lmmp_mat22_t* b, mp_ptr tp) {
    lmmp_param_assert(dst != NULL && a != NULL && b != NULL && tp != NULL);
    if (mat22_all_big_(a, MAT22_MUL_STRASSEN_THRESHOLD) && mat22_all_big_(b, MAT22_MUL_STRASSEN_THRESHOLD)) {
        mat22_mul_strassen_(dst, a, b, tp);
    } else {
        mat22_mul_basecase_(dst, a, b, tp);
    }
}

void lmmp_mat22_sqr_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, mp_ptr tp) {
    lmmp_param_assert(dst != NULL && a != NULL && tp != NULL);
    if (mat22_all_big_(a, MAT22_SQR_STRASSEN_THRESHOLD)) {
        mat22_sqr_strassen_(dst, a, tp);
    } else {
        mat22_sqr_basecase_(dst, a, tp);
    }
}
