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

#ifndef LMMP_NUMTH_H
#define LMMP_NUMTH_H

#include <stdbool.h>

#include "lmmp.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef uint8_t uchar;
typedef int8_t schar;
typedef uint16_t ushort;
typedef int16_t sshort;
typedef uint32_t uint;
typedef uint64_t ulong;
typedef int32_t sint;
typedef int64_t slong;
typedef uint8_t* ucharp;
typedef int8_t* scharp;
typedef uint16_t* ushortp;
typedef int16_t* sshortp;
typedef uint32_t* uintp;
typedef int32_t* sintp;
typedef uint64_t* ulongp;
typedef int64_t* slongp;


/**
 * @brief 计算 a 在2^32下的逆元
 * @param a 待求逆元
 * @warning a%2==1
 * @return 逆元
 */
LMMP_API uint lmmp_binvert_uint_(uint a);

/**
 * @brief 计算 a 在2^64下的逆元
 * @param a 待求逆元
 * @warning a%2==1
 * @return 逆元
 */
LMMP_API ulong lmmp_binvert_ulong_(ulong a);

/**
 * @brief 计算 [numa,2] 在 B^2 下的逆元
 * @param numa 待求逆元指针（长度为 2 个limb）
 * @param dst 结果指针（长度为 2 个limb）
 * @warning numa!=NULL, dst!=NULL, numa[0]%2==1, eqsep(dst,numa)
 */
LMMP_API void lmmp_binvert_2_(mp_ptr dst, mp_srcptr numa);

/**
 * @brief 计算 [numa,3] 在 B^3 下的逆元
 * @param numa 待求逆元指针（长度为 3 个limb）
 * @param dst 结果指针（长度为 3 个limb）
 * @warning numa!=NULL, dst!=NULL, numa[0]%2==1, sep(dst,numa)
 */
LMMP_API void lmmp_binvert_3_(mp_ptr dst, mp_srcptr numa);

/**
 * @brief 计算 [numa,4] 在 B^4 下的逆元
 * @param numa 待求逆元指针（长度为 4 个limb）
 * @param dst 结果指针（长度为 4 个limb）
 * @warning numa!=NULL, dst!=NULL, numa[0]%2==1, sep(dst,numa)
 */
LMMP_API void lmmp_binvert_4_(mp_ptr dst, mp_srcptr numa);

/**
 * @brief 计算 [numa,n] 在 B^n 下的逆元
 * @param numa 待求逆元指针（长度为 n 个limb）
 * @param dst 结果指针（长度为 n 个limb）
 * @param n 结果的 limb 长度
 * @param tp 临时工作区指针（长度为 5*(n+1)/2 个limb）
 * @warning numa!=NULL, dst!=NULL, numa[0]%2==1, sep(dst,numa,tp)
 */
LMMP_API void lmmp_binvert_n_dc_(mp_ptr dst, mp_srcptr numa, mp_size_t n, mp_ptr tp);

/**
 * @brief 计算 a 在 B^n 下的逆元
 * @param dst 结果指针（长度为 n 个limb）
 * @param a 待求逆元
 * @param n 结果的 limb 长度
 * @warning a%2==1, n>1, dst!=NULL
 */
LMMP_API void lmmp_binvert_unbalance_1_(mp_ptr dst, mp_limb_t a, mp_size_t n);

/**
 * @brief 计算 [numa,2] 在 B^n 下的逆元
 * @param dst 结果指针（长度为 n 个limb）
 * @param numa 待求逆元指针（长度为 2 个limb）
 * @param n 结果的 limb 长度
 * @warning numa[0]%2==1, n>2, dst!=NULL, numa!=NULL, sep(dst,numa)
 */
LMMP_API void lmmp_binvert_unbalance_2_(mp_ptr dst, mp_srcptr numa, mp_size_t n);

/**
 * @brief 计算 [numa,na] 在 B^n 下的逆元
 * @param dst 结果指针（长度为 n 个limb）
 * @param numa 待求逆元指针（长度为 na 个limb）
 * @param na 待求逆元的 limb 长度
 * @param n 结果的 limb 长度
 * @param tp 临时工作区指针（长度为 (9*na+5)/2 个limb）
 * @warning numa[0]%2==1, n>na, dst!=NULL, numa!=NULL, tp!=NULL, sep(dst,numa,tp)
 */
LMMP_API void lmmp_binvert_unbalance_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t n, mp_ptr tp);

/**
 * @brief 计算 [numa,na] 在 B^n 下的逆元
 * @param dst 结果指针（长度为 n 个limb）
 * @param numa 待求逆元指针（长度为 na 个limb）
 * @param na 待求逆元的 limb 长度
 * @param n 结果的 limb 长度
 * @warning n>=na>0, numa!=NULL, dst!=NULL, numa[0]%2==1, sep(dst,numa)
 */
LMMP_API void lmmp_binvert_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t n);

/**
 * @brief 精确除法（[dst,nn]=[np,nn]/d，且余数必须为0）
 * @param dst 结果指针（长度为 nn 个limb）
 * @param np 被除数指针（长度为 nn 个limb）
 * @param nn 被除数的 limb 长度
 * @param d 除数
 * @param dinv 除数的逆元（d*dinv==1 mod 2^64）
 * @warning d%2==1, d*dinv==1 mod 2^64, nn>0, dst!=NULL, np!=NULL, eqsep(dst,np)
 */
LMMP_API void lmmp_divexact_1_(mp_ptr dst, mp_srcptr np, mp_size_t nn, mp_limb_t d, mp_limb_t dinv);

/**
 * @brief 精确除法（[dst,nn]=[np,nn]/[dp,2]，且余数必须为0）
 * @param dst 结果指针（长度为 nn-1 个limb）
 * @param np 被除数指针（长度为 nn 个limb）
 * @param nn 被除数的 limb 长度
 * @param dp 除数指针（长度为 2 个limb）
 * @param dinv 除数的逆元指针（长度为 2 个limb）
 * @warning dp[0]%2==1, dp*dinv==1 mod 2^128, nn>1, dst!=NULL, np!=NULL, dp!=NULL, dinv!=NULL
 *          eqsep(dst,np), sep(dp,dinv,[dst|np])
 */
LMMP_API void lmmp_divexact_2_(mp_ptr dst, mp_srcptr np, mp_size_t nn, mp_srcptr dp, mp_srcptr dinv);

/**
 * @brief 精确除法（[dst,nn]=[np,nn]/[dp,dn]，且余数必须为0）
 * @param dst 结果指针（长度为 nn-dn+1 个limb）
 * @param np 被除数指针（长度为 nn 个limb），将会被修改为全零
 * @param nn 被除数的 limb 长度
 * @param dp 除数指针（长度为 dn 个limb）
 * @param dn 结除数的 limb 长度
 * @param dinv 除数[dp,dn]关于B^dn的逆元，若为NULL，则自动计算
 * @warning dp[0]%2==1, nn>=dn>0, dst!=NULL, np!=NULL, dp!=NULL, eqsep(dst,np), sep(dp,dinv,[dst|np])
 * @note 若dst==np，只会覆写 [dst,nn-dn+1] 区域
 */
LMMP_API void lmmp_divexact_unbalance_(mp_ptr dst, mp_srcptr np, mp_size_t nn, mp_srcptr dp, mp_size_t dn, mp_ptr dinv);

/**
 * @brief 精确除法（[dst,nn]=[np,nn]/[dp,dn]，且余数必须为0），朴素算法
 * @param dst 结果指针（长度为 nn-dn+1 个limb）
 * @param np 被除数指针（长度为 nn 个limb），将会被覆写为全零
 * @param nn 被除数的 limb 长度
 * @param dp 除数指针（长度为 dn 个limb）
 * @param dn 结除数的 limb 长度
 * @param dinv 除数低位dp[0]关于B的逆元
 * @warning dp[0]%2==1, d[0]*dinv==1 mod 2^64, nn>=dn>0, dst!=NULL, np!=NULL, dp!=NULL, eqsep(dst,np), sep(dp,[dst|np])
 * @note 若dst==np，将会覆写 [dst,nn] 区域，其中 [dst,nn-dn+1] 为商，[dst+nn-dn+1,dn-1] 将会被覆写为0
 */
LMMP_API void lmmp_divexact_basecase_(mp_ptr dst, mp_ptr np, mp_size_t nn, mp_srcptr dp, mp_size_t dn, mp_limb_t dinv);

/**
 * @brief 精确除法（[dst,nn]=[np,nn]/[dp,dn]，且余数必须为0），分治算法
 * @param dst 结果指针（长度为 nn-dn+1 个limb）
 * @param np 被除数指针（长度为 nn 个limb）
 * @param nn 被除数的 limb 长度
 * @param dp 除数指针（长度为 dn 个limb）
 * @param dn 结除数的 limb 长度
 * @warning dp[0]%2==1, nn>=dn>0, dn>=nn-dn+1, dst!=NULL, np!=NULL, dp!=NULL, sep(dst,np,dp)
 */
LMMP_API void lmmp_divexact_divide_(mp_ptr dst, mp_srcptr np, mp_size_t nn, mp_srcptr dp, mp_size_t dn);

/**
 * @brief 精确除法（[dst,nn]=[np,nn]/[dp,dn]，且余数必须为0）
 * @param dst 结果指针（长度为 nn-dn+1 个limb）
 * @param np 被除数指针（长度为 nn 个limb）
 * @param nn 被除数的 limb 长度
 * @param dp 除数指针（长度为 dn 个limb）
 * @param dn 结除数的 limb 长度
 * @warning dp[0]%2==1, nn>=dn>0, dst!=NULL, np!=NULL, dp!=NULL, eqsep(dst,np), sep([dst|np],dp)
 */
LMMP_API void lmmp_divexact_(mp_ptr dst, mp_srcptr np, mp_size_t nn, mp_srcptr dp, mp_size_t dn);

/**
 * @brief 计算两个无符号整数的最大公约数
 * @param u 第一个无符号整数
 * @param v 第二个无符号整数
 * @return 最大公约数
 * @warning u!=0, v!=0
 */
LMMP_API mp_limb_t lmmp_gcd_11_(mp_limb_t u, mp_limb_t v);

/**
 * @brief 计算两个无符号整数的最大公约数
 * @param up 第一个无符号整数指针
 * @param un 第一个无符号整数的 limb 长度
 * @param v 第二个无符号整数
 * @warning v!=0, up!=NULL, un>0
 * @return 最大公约数
 */
LMMP_API mp_limb_t lmmp_gcd_1_(mp_srcptr up, mp_size_t un, mp_limb_t v);

/**
 * @brief 计算两个无符号整数的最大公约数
 * @param up 第一个无符号整数指针，长度为 2
 * @param vp 第二个无符号整数指针，长度为 2
 * @param dst 结果指针（长度为 2，两个 limb 都会进行写入，即使最高位可能为0）
 * @warning up!=NULL, vp!=NULL, [up,2]!=0, [vp,2]!=0, dst!=NULL, eqsep(dst,[up|vp])
 * @note up 和 vp 的高位允许为 0，但要求任一个数的高低位不能全为 0
 * @return dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_gcd_22_(mp_ptr dst, mp_srcptr up, mp_srcptr vp);

/**
 * @brief 计算两个无符号整数的最大公约数
 * @param up 第一个无符号整数指针
 * @param un 第一个无符号整数的 limb 长度
 * @param vp 第二个无符号整数指针，长度为 2
 * @param dst 结果指针（长度至少为 2，两个 limb 都会进行写入，即使最高位可能为0）
 * @warning up!=NULL, un>2, vp!=NULL, vp[1]!=0, dst!=NULL, eqsep(dst,[up|vp])
 * @return dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_gcd_2_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr vp);

/**
 * @brief 计算两个无符号整数的最大公约数（Lehmer算法）
 * @param dst 结果指针（长度至少为 min(un,vn)）
 * @param up 第一个无符号整数指针
 * @param un 第一个无符号整数的 limb 长度
 * @param vp 第二个无符号整数指针
 * @param vn 第二个无符号整数的 limb 长度
 * @warning up!=NULL, un>0, vp!=NULL, vn>0, eqsep(dst,[up|vp]), dst!=NULL, up[un-1]!=0, vp[vn-1]!=0
 * @return dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_gcd_lehmer_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn);

/**
 * @brief hgcd 变换矩阵
 * @note  / a \   / m00  m01 \   / a' \
 *        |   | = |          | * |    |
 *        \ b /   \ m10  m11 /   \ b' /
 *       其中 (a;b) 为 hgcd 入口数对，(a';b') 为归约后数对。矩阵元素均非负，
 *       det(M) = ±1，元素值不超过入口较大分量的规模。
 *       每个元素显式存储真实长度 n[i][j]（归一化，顶 limb 非零；
 *       n[i][j]==0 表示零元素），不做零填充：元素缓冲 [0, n[i][j]) 之外
 *       的内容无效，一切访问以长度为准。长度由库内计算过程显式维护
 *       （乘法进位/最高位），不做前导零扫描。
 *       各元素长度 <= alloc。
 */
typedef struct {
    mp_ptr m[2][2];     /* 元素指针，各指向 alloc limbs 的连续区域 */
    mp_size_t n[2][2];  /* 各元素真实长度（归一化；0 表示零元素） */
    mp_size_t alloc;    /* 每元素容量（limb） */
} lmmp_hgcd_matrix_t;

/**
 * @brief 初始化 hgcd 变换矩阵为单位矩阵（堆分配元素缓冲）
 * @param M 变换矩阵
 * @param alloc 每元素容量（limb）。若随后用于 n limb 数对的 hgcd，需 alloc >= n+2
 * @warning M!=NULL, alloc>0
 * @note 使用完毕须调用 lmmp_hgcd_matrix_free_ 释放
 */
LMMP_API void lmmp_hgcd_matrix_init_(lmmp_hgcd_matrix_t* M, mp_size_t alloc);

/**
 * @brief 释放 hgcd 变换矩阵的元素缓冲
 * @param M 变换矩阵
 * @warning M!=NULL, M 须由 lmmp_hgcd_matrix_init_ 初始化且未被释放
 */
LMMP_API void lmmp_hgcd_matrix_free_(lmmp_hgcd_matrix_t* M);

/**
 * @brief 半扩展欧几里得：将数对原地归约至约一半规模，并累积变换矩阵
 * @param M 变换矩阵（入口应为单位矩阵，出口满足 (a;b) = M*(a';b')）
 * @param ap 较大分量输入兼输出数组（长度为 n 个limb，出口保证较大者位于 ap）
 * @param bp 较小分量输入兼输出数组（长度为 n 个limb，允许高位零填充）
 * @param n ap,bp 的公共长度
 * @warning M!=NULL, ap!=NULL, bp!=NULL, sep(ap,bp), n>0, M->alloc>=n+2,
 *          ap[n-1]!=0（a 须归一化；bp 非全零，允许高位零填充），[ap,n]>[bp,n]（值序）
 * @return 归约后公共长度（成功时一般 <= n/2+2；若归约中遇到整除完成态会提前返回，
 *         此时 [bp,*] 可能已为 0，由调用者检查）；0 表示未做任何归约（M 保持单位矩阵）
 * @note gcd(a,b) = gcd(a',b')。渐进复杂度 O(M(n) log n)，M 为乘法复杂度。
 *       长输入下经由 lmmp_gcd_hgcd_ 使用；直接调用时注意 ap/bp 需要各自 n limbs 容量。
 */
LMMP_API mp_size_t lmmp_hgcd_(lmmp_hgcd_matrix_t* M, mp_ptr ap, mp_ptr bp, mp_size_t n);

/**
 * @brief 计算两个无符号整数的最大公约数（hgcd 分治，长输入优选）
 * @param dst 结果指针（长度至少为 bn）
 * @param up 第一个无符号整数数组（长度为 un 个limb）
 * @param un 第一个无符号整数的 limb 长度
 * @param vp 第二个无符号整数数组（长度为 vn 个limb）
 * @param vn 第二个无符号整数的 limb 长度
 * @warning up!=NULL, un>=vn>0, vp!=NULL, dst!=NULL, eqsep(dst,[up|vp]), up[un-1]!=0, vp[vn-1]!=0
 * @return dst 的实际 limb 长度
 * @note 不平衡输入（un >= 2*vn）先做取模预归约将两数长度拉近，再进入
 *       分治主路径：不平衡时 hgcd 的阶段 A 递归失效，预归约普遍更快
 *      （越悬殊收益越大，见 hgcd.c 实现注）
 */
LMMP_API mp_size_t lmmp_gcd_hgcd_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn);

/**
 * @brief 计算两个无符号整数的最大公约数（通用分发入口）
 * @param dst 结果指针（长度至少为 min(un,vn)）
 * @param up 第一个无符号整数指针
 * @param un 第一个无符号整数的 limb 长度
 * @param vp 第二个无符号整数指针
 * @param vn 第二个无符号整数的 limb 长度
 * @warning up!=NULL, un>0, vp!=NULL, vn>0, eqsep(dst,[up|vp]), dst!=NULL, up[un-1]!=0, vp[vn-1]!=0
 * @return dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_gcd_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn);

/**
 * @brief 计算两个无符号整数的乘积，对mod取模，商放入 q 中
 * @param a 第一个无符号整数
 * @param b 第二个无符号整数
 * @param q 商的结果指针
 * @param mod 取模数
 * @warning a < mod, b < mod, q!=NULL
 * @return 余数
 */
LMMP_API ulong lmmp_mulmod_ulong_(ulong a, ulong b, ulong mod, ulongp q);

/**
 * @brief 计算 base^exp 对 mod 取模
 * @param base 底数
 * @param exp 指数
 * @param mod 模数
 * @warning base<mod, mod>1, mod%2==1
 * @return base^exp 对 mod 取模的结果
 */
LMMP_API uint lmmp_powmod_uint_odd_(uint base, ulong exp, uint mod);

/**
 * @brief 计算 base^exp 对 mod 取模
 * @param base 底数
 * @param exp 指数
 * @param mod 模数
 * @warning base<mod, mod>1, mod%2==1
 * @return base^exp 对 mod 取模的结果
 */
LMMP_API ulong lmmp_powmod_ulong_odd_(ulong base, ulong exp, ulong mod);

/**
 * @brief 计算 [bp,n]^[ep,en] mod B^n，并将结果写入 [dst,n]
 * @param dst 结果指针（长度为 n 个limb）
 * @param bp 底数指针
 * @param n 底数的 limb 长度
 * @param ep 指数指针
 * @param en 指数的 limb 长度
 * @warning dst!=NULL, bp!=NULL, ep!=NULL, en>0, n>0, ep[en-1]>0, sep(dst,[bp|ep])
 */
LMMP_API void lmmp_powlo_(mp_ptr dst, mp_srcptr bp, mp_size_t n, mp_srcptr ep, mp_size_t en);

/**
 * @brief 计算 [bp,1]^[ep,en] mod B，并将结果写入 [dst,1]（lmmp_powlo_ 的 1 limb 特化）
 * @param dst 结果指针（长度为 1 个limb）
 * @param bp 底数指针（长度为 1 个limb）
 * @param ep 指数指针
 * @param en 指数的 limb 长度
 * @warning dst!=NULL, bp!=NULL, ep!=NULL, en>0, ep[en-1]>0, sep(dst,[bp|ep])
 * @note 指数为 1 时直接截断取 [bp,1]；单 limb 乘法 mod B 即自然回绕，
 *       全程无归约开销
 */
LMMP_API void lmmp_powlo_1_(mp_ptr dst, mp_srcptr bp, mp_srcptr ep, mp_size_t en);

/**
 * @brief 计算 [bp,2]^[ep,en] mod B^2，并将结果写入 [dst,2]（lmmp_powlo_ 的 2 limb 特化）
 * @param dst 结果指针（长度为 2 个limb）
 * @param bp 底数指针（长度为 2 个limb）
 * @param ep 指数指针
 * @param en 指数的 limb 长度
 * @warning dst!=NULL, bp!=NULL, ep!=NULL, en>0, ep[en-1]>0, sep(dst,[bp|ep])
 * @note 指数为 1 时直接截断取 [bp,2]；乘法与平方均走 longlong.h 的
 *       128 位低位积内联（_umul128to128_），全程无归约开销
 */
LMMP_API void lmmp_powlo_2_(mp_ptr dst, mp_srcptr bp, mp_srcptr ep, mp_size_t en);

/**
 * @brief Montgomery 归约（REDC）：计算 ([tp,2n] + q*[mp,n]) / B^n，其中
 *        q = [tp,n] * [ninv,n] mod B^n，即结果为 [tp,2n] * B^(-n) % [mp,n]
 * @param dst 结果指针（长度为 n 个limb）
 * @param tp 被归约数指针（长度为 2n 个limb）
 * @param ninv -[mp,n]^(-1) mod B^n（长度为 n 个limb）
 * @param mp 模数指针（长度为 n 个limb）
 * @param n 模数的 limb 长度
 * @warning dst!=NULL, tp!=NULL, ninv!=NULL, mp!=NULL, n>0, mp[0]%2==1, mp[n-1]>0,
 *          [ninv,n] == -[mp,n]^(-1) mod B^n, sep(dst,[tp|ninv|mp]), [tp,2n] < B^n*[mp,n]
 * @return 结果最高位（第 n+1 limb 的值，[0|1]），返回值:[dst,n] 即归约结果，且结果 < 2*[mp,n]
 * @note 典型用法（蒙域乘法）：a_m = a*B^n mod m 与 b_m = b*B^n mod m（均 < m）的
 *       全积 [tp,2n] = a_m*b_m 经本函数得 (a*b)*B^n mod m（仍在蒙域），随后若
 *       结果 >= m 再减一次 m 即得 < m 的规范蒙域剩余。
 *       注意：对普通整数 a、b 的全积直接调用得到的是 a*b*B^(-n) mod m（带
 *       Montgomery 缩放因子，条件减法不能将其消去），并非 a*b mod m；如需单次
 *       REDC 完成普通模乘，一操作数取普通域、另一取蒙域：REDC(a*b_m) = a*b mod m
 */
LMMP_API mp_limb_t lmmp_redc_(mp_ptr dst, mp_srcptr tp, mp_srcptr ninv, mp_srcptr mp, mp_size_t n);

/**
 * @brief 计算奇模数模幂 [dst,1] = [bp,1]^[ep,en] % mod
 * @param dst 结果指针（长度为 1 个limb）
 * @param bp 底数指针（长度为 1 个limb）
 * @param ep 指数指针
 * @param en 指数的 limb 长度
 * @param mod 模数
 * @warning dst!=NULL, bp!=NULL, ep!=NULL, en>0, ep[en-1]>0, mod%2==1, mod>1, [bp,1]<mod
 */
LMMP_API void lmmp_powmod_1_(mp_ptr dst, mp_srcptr bp, mp_srcptr ep, mp_size_t en, mp_limb_t mod);

/**
 * @brief 计算奇模数模幂 [dst,2] = [bp,2]^[ep,en] % [mod,2]
 * @param dst 结果指针（长度为 2 个limb）
 * @param bp 底数指针（长度为 2 个limb）
 * @param ep 指数指针
 * @param en 指数的 limb 长度
 * @param mod 模数指针（长度为 2 个limb）
 * @warning dst!=NULL, bp!=NULL, ep!=NULL, mod!=NULL, en>0, ep[en-1]>0, mod[0]%2==1,
 *          mod[1]!=0, [mod,2]>1, [bp,2]<[mod,2]
 */
LMMP_API void lmmp_powmod_2_(mp_ptr dst, mp_srcptr bp, mp_srcptr ep, mp_size_t en, mp_srcptr mod);

/**
 * @brief 计算奇模数模幂 [dst,n] = [bp,n]^[ep,en] % [mp,n]
 * @param dst 结果指针（长度为 n 个limb，规范剩余 < [mp,n]，高位可能为 0）
 * @param bp 底数指针（长度为 n 个limb）
 * @param ep 指数指针
 * @param en 指数的 limb 长度
 * @param mp 模数指针（长度为 n 个limb）
 * @param n 模数的 limb 长度
 * @warning dst!=NULL, bp!=NULL, ep!=NULL, mp!=NULL, en>0, ep[en-1]>0, n>0,
 *          mp[0]%2==1, mp[n-1]>0, [mp,n]>1, [bp,n]<[mp,n], sep(dst,[bp|ep|mp])
 */
LMMP_API void lmmp_powmod_odd_(mp_ptr dst, mp_srcptr bp, mp_srcptr ep, mp_size_t en, mp_srcptr mp, mp_size_t n);

/**
 * @brief 计算任意模数模幂 [dst,n] = [bp,n]^[ep,en] % [mp,n]
 * @param dst 结果指针（长度为 n 个limb，规范剩余 < [mp,n]，高位可能为 0）
 * @param bp 底数指针（长度为 n 个limb）
 * @param ep 指数指针
 * @param en 指数的 limb 长度
 * @param mp 模数指针（长度为 n 个limb，奇偶皆可）
 * @param n 模数的 limb 长度
 * @warning dst!=NULL, bp!=NULL, ep!=NULL, mp!=NULL, en>0, ep[en-1]>0, n>0,
 *          mp[n-1]>0, [mp,n]>1, [bp,n]<[mp,n], sep(dst,[bp|ep|mp])
 */
LMMP_API void lmmp_powmod_(mp_ptr dst, mp_srcptr bp, mp_srcptr ep, mp_size_t en, mp_srcptr mp, mp_size_t n);

/**
 * @brief 大于n的下一个素数
 * @param n 起始点（不含）
 * @warning 如果 n 大于等于ulong可表示最大的质数，则返回ulong_max
 * @return 大于n的下一个素数
 */
LMMP_API ulong lmmp_next_prime_ulong_(ulong n);

/**
 * @brief 小于等于n的上一个素数
 * @param n 起始点（含）
 * @warning 如果 n 小于2，则返回 0
 * @return 小于等于n的上一个素数，如果n恰好为素数，则返回 n
 */
LMMP_API ulong lmmp_prev_prime_ulong_(ulong n);

/**
 * @brief 判断素数
 * @param n 待判断的数
 * @return 若 n 为素数，返回 true，否则返回 false
 */
LMMP_API bool lmmp_is_prime_uint_(uint n);

/**
 * @brief 判断素数
 * @param n 待判断的数
 * @note 如果 n 的实际值小于2^32，此函数不会调用 lmmp_is_prime_uint_，
 *       如果你可以保证 n 的实际值小于2^32，使用 lmmp_is_prime_uint_ 将会更快
 * @return 若 n 为素数，返回 true，否则返回 false
 */
LMMP_API bool lmmp_is_prime_ulong_(ulong n);

/**
 * @brief 判断素数（无试除法）
 * @param n 待判断的数（建议为极有可能为素数的数）
 * @note 不进行试除法过滤，适用于判断已被小素数试除法过滤的数或强伪素数
 * @warning n>2
 * @return 若 n 为素数，返回 true，否则返回 false
 */
LMMP_API bool lmmp_is_prime_notrial_(ulong n);

/**
 * @brief 判断 128 位素数
 * @param lo 待判断数的低 64 位
 * @param hi 待判断数的高 64 位
 * @warning hi>0
 * @note B <= n < SWbound = 3317044064679887385961981（约
 *       2^81.5）时为确定性判据（前 13 个素数基底的 Rabin-Miller，
 *       Sorenson-Webster），返回 1；更大时为 BPSW 型判据（基底 2 强伪素数
 *       测试叠加强 Lucas-Selfridge 测试的 V 阶梯实现），无已知反例，返回 2。
 * @return 0 = 合数；1 = 素数（确定性判据）；2 = 极可能是素数（BPSW 型非确定性判据）
 */
LMMP_API int lmmp_is_prime_2_(mp_limb_t lo, mp_limb_t hi);

/**
 * @brief 强 Lucas-Selfridge 测试（大于 128 位）
 * @param np 待测奇数指针（nn 个limb）
 * @param nn 待测数的 limb 长度
 * @warning np!=NULL, nn>2, np[nn-1]>0, np[0]%2==1
 * @note Selfridge 方法 A：P=1，Q=(1-D)/4，D 取 5,-7,9,-11,... 中首个
 *       (D|n)=-1 者；V-only 阶梯实现，判据与标准 U/V 强 Lucas 测试精确一致
 *       （U_d=0 经 P=1 恒等式 D*U_d = 2V_{d+1} - V_d 以 2V_{d+1}=V_d 检出）。
 *       判定合数的情形含 gcd(|D|,n)>1 与 n 为完全平方数。
 *       单轮成本约为单轮 MR 的 1.6~2 倍
 * @return 1 = 通过强 Lucas 测试；0 = 合数
 */
LMMP_API int lmmp_is_strong_lucas_(mp_srcptr np, mp_size_t nn);

/**
 * @brief 大整数素性检验（强度分档，大于 128 位）
 * @param np 待测数指针（nn 个limb）
 * @param nn 待测数的 limb 长度
 * @param strength 检测强度 [0,7]。随机基底 MR 分两层："1+n" 搭配指先跑至多
 *        x 轮 1 limb 基底（a in [3,B) 均匀，最廉价，尽早拒判）再跑至多 y 轮
 *        全域（n limb，[2,n-2] 均匀）基底。各层均为"至多"轮数，任一轮检出
 *        合数即提前终止：
 *        | 强度 | 基底2 MR | 1 limb 基底 MR | n limb 基底 MR | 强 Lucas |
 *        | --- | ------- | -------------- | -------------- | -------- |
 *        |  0  |   √     |       2        |       2        |    -     |
 *        |  1  |   √     |       3        |       2        |    -     |
 *        |  2  |   √     |       4        |       3        |    -     |
 *        |  3  |   √     |       5        |       4        |    -     |
 *        |  4  |   √     |       0        |       0        |    √     |
 *        |  5  |   √     |       2        |       0        |    √     |
 *        |  6  |   √     |       2        |       1        |    √     |
 *        |  7  |   √     |       3        |       2        |    √     |
 * @warning np!=NULL, nn>2, np[nn-1]>0, 0<=strength<=7
 * @note 契约域 n > 2^128 超过一切已实用化的确定性 MR 基组界（psi_13 约 3.3e24 < 2^82），
         故实际不会返回 1，语义保留与 lmmp_is_prime_2_ 一致。
 * @attention 1 limb 基底空间仅 2^64，由 AGP 定理（无穷多合数的最小 MR 见证
 *            > (log n)^(1/3)）知窄基底类在最坏/对抗情形弱于全宽随机轮：本"1+n"
 *            搭配定位于随机候选（keygen）场景，不承诺对抗性输入下的强度保证。
 * @return 0 = 合数；1 = 素数（确定性判据，本契约域不可达）；
 *         2 = 极大概率为素数
 */
LMMP_API int lmmp_is_prime_n_(mp_srcptr np, mp_size_t nn, int strength);

/**
 * @brief 计算幂次方需要的limb缓冲区长度 [base,n] ^ exp
 * @param base 底数指针
 * @param n 底数 limb 长度
 * @param exp 指数
 * @warning n>0, base[n-1]!=0, [base,n]>1
 * @return 返回值为 [base,n]^exp 需要的 limb 缓冲区长度（比实际长度多）
 */
LMMP_API mp_size_t lmmp_pow_size_(mp_srcptr base, mp_size_t n, ulong exp);

/**
 * @brief 计算幂次方需要的limb缓冲区长度 base ^ exp
 * @param base 底数
 * @param exp 指数
 * @warning exp>0, base>=1
 * @return 返回值为 base^exp 需要的 limb 缓冲区长度（比实际长度多）
 */
LMMP_API mp_size_t lmmp_pow_1_size_(mp_limb_t base, ulong exp);

/**
 * @brief 计算奇数次幂算法 [dst,rn] = [base,n] ^ exp
 * @param dst 结果指针
 * @param rn 结果指针的 limb 缓冲区长度
 * @param base 底数指针
 * @param n 底数指针的 limb 长度
 * @param exp 指数
 * @warning n>0, base[n-1]!=0, sep(dst,base), [base,n]>1, exp>=3, exp%2==1
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_pow_basecase_(mp_ptr dst, mp_size_t rn, mp_srcptr base, mp_size_t n, ulong exp);

/**
 * @brief 计算幂次方 [dst,rn] = [base,1] ^ exp
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param base 底数（4位无符号整数）
 * @param exp 指数
 * @warning 1<=base<=0xf, exp>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_u4_pow_1_(mp_ptr dst, mp_size_t rn, ulong base, ulong exp);

/**
 * @brief 计算幂次方 [dst,rn] = [base,1] ^ exp
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param base 底数（8位无符号整数）
 * @param exp 指数
 * @warning 0<base<=0xff, exp>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_u8_pow_1_(mp_ptr dst, mp_size_t rn, ulong base, ulong exp);

/**
 * @brief 计算幂次方 [dst,rn] = [base,1] ^ exp
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param base 底数（16位无符号整数）
 * @param exp 指数
 * @warning 0<base<=0xffff, exp>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_u16_pow_1_(mp_ptr dst, mp_size_t rn, ulong base, ulong exp);

/**
 * @brief 计算幂次方 [dst,rn] = [base,1] ^ exp
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param base 底数（32位无符号整数）
 * @param exp 指数
 * @warning 0<base<=2^32-1, exp>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_u32_pow_1_(mp_ptr dst, mp_size_t rn, ulong base, ulong exp);

/**
 * @brief 计算幂次方 [dst,rn] = [base,1] ^ exp
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param base 底数（64位无符号整数）
 * @param exp 指数
 * @warning 2^32<=base<=2^64-1, exp>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_u64_pow_1_(mp_ptr dst, mp_size_t rn, ulong base, ulong exp);

/**
 * @brief 计算幂次方 [dst,rn] = [base,1] ^ exp
 * @param dst 结果指针
 * @param base 底数
 * @param exp 指数
 * @warning base>=1, exp>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_pow_1_(mp_ptr dst, mp_size_t rn, mp_limb_t base, ulong exp);

/**
 * @brief 计算幂次方2比特窗口快速幂算法 [dst,rn] = [base,n] ^ exp
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param base 底数指针
 * @param n 底数指针的 limb 长度
 * @param exp 指数
 * @warning n>0, base[n-1]!=0, sep(dst,base), exp>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_pow_win2_(mp_ptr dst, mp_size_t rn, mp_srcptr base, mp_size_t n, ulong exp);

/**
 * @brief 计算大整数幂 [dst,rn] = [base,n] ^ exp
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param base 底数指针
 * @param n 底数指针的 limb 长度
 * @param exp 指数
 * @warning n>0, base[n-1]!=0, sep(dst,base), exp>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_pow_(mp_ptr dst, mp_size_t rn, mp_srcptr base, mp_size_t n, ulong exp);

/**
 * @brief 计算 Fibonacci 数 F[n]（及 F[n-1]）所需的 limb 缓冲区长度
 * @param n Fibonacci 数列的下标
 * @return 缓冲区长度，比 F[n] 的实际长度多 2-4 个 limb
 */
LMMP_API mp_size_t lmmp_fibonacci_size_(ulong n);

/**
 * @brief 计算 Fibonacci 数 [dst,rn] = F[n]
 * @param dst 结果指针
 * @param rn dst 的 limb 缓冲区长度
 * @param n Fibonacci 数列的下标
 * @warning dst!=NULL, rn>=lmmp_fibonacci_size_(n)
 * @return F[n] 的实际 limb 长度（n==0 时 返回 0）
 */
LMMP_API mp_size_t lmmp_fibonacci_(mp_ptr dst, mp_size_t rn, ulong n);

/**
 * @brief 计算 Fibonacci 数对：[dst,rn] = F[n]，[dst2,rn] = F[n-1]
 * @param dst F[n] 结果指针
 * @param dst2 F[n-1] 结果指针
 * @param rn dst 与 dst2 各自的 limb 缓冲区长度
 * @param n Fibonacci 数列的下标
 * @warning dst!=NULL, dst2!=NULL, sep(dst,dst2), rn>=lmmp_fibonacci_size_(n)
 * @return F[n] 的实际 limb 长度（n==0时，返回1）；F[n-1] 的实际长度为ret-(dst2[ret-1]==0)，至多比返回值少1
 */
LMMP_API mp_size_t lmmp_fibonacci2_(mp_ptr dst, mp_ptr dst2, mp_size_t rn, ulong n);

/**
 * @brief 计算 nPr 排列数的 limb 缓冲区长度
 * @param n 排列数的总数
 * @param r 排列数的选择数
 * @param bits 被修改为 nPr 的2的因子数
 * @warning r<=n, bits!=NULL
 * @return nPr 排列数的 limb 缓冲区长度（比实际长度多）
 */
LMMP_API mp_size_t lmmp_nPr_size_(ulong n, ulong r, mp_bitcnt_t* bits);

/**
 * @brief 计算 nPr 排列数的奇数部分
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度（nPr_size_ 函数的返回值 - bits/LIMB_BITS）
 * @param n 排列数的总数
 * @param r 排列数的选择数
 * @warning 0xffff>=n>=r, dst!=NULL, rn>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_odd_nPr_ushort_(mp_ptr dst, mp_size_t rn, ulong n, ulong r);

/**
 * @brief 计算 nPr 排列数的奇数部分
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度（nPr_size_ 函数的返回值 - bits/LIMB_BITS）
 * @param n 排列数的总数
 * @param r 排列数的选择数
 * @warning 0xffffffff>=n>=r, dst!=NULL, rn>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_odd_nPr_uint_(mp_ptr dst, mp_size_t rn, ulong n, ulong r);

/**
 * @brief 计算 nPr 排列数的奇数部分
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度（nPr_size_ 函数的返回值 - bits/LIMB_BITS）
 * @param n 排列数的总数
 * @param r 排列数的选择数
 * @warning n>=r, dst!=NULL, rn>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_odd_nPr_ulong_(mp_ptr dst, mp_size_t rn, ulong n, ulong r);

/**
 * @brief 计算 nPr 排列数 ( nPr = n! / (n-r)! )
 * @param dst 结果指针
 * @param bits nPr 的2的因子数
 * @param rn 结果指针的 limb 长度
 * @param n 排列数的总数
 * @param r 排列数的选择数
 * @warning n>=r, dst!=NULL, rn>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_nPr_(mp_ptr dst, mp_bitcnt_t bits, mp_size_t rn, ulong n, ulong r);

/**
 * @brief 计算 n! 阶乘的 limb 缓冲区长度
 * @param n 阶乘的阶数
 * @param bits 被修改为 n! 的2的因子数
 * @warning bits!=NULL
 * @return n! 阶乘的 limb 缓冲区长度（比实际长度多）
 */
LMMP_API mp_size_t lmmp_factorial_size_(uint n, mp_bitcnt_t* bits);

/**
 * @brief 计算 n! 阶乘的奇数部分
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度（factorial_size_ 函数的返回值 - bits/LIMB_BITS）
 * @param n 阶乘的阶数
 * @warning n>0xffff, dst!=NULL, rn>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_odd_factorial_uint_(mp_ptr dst, mp_size_t rn, uint n);

/**
 * @brief 计算 n! 阶乘
 * @param dst 结果指针
 * @param bits n! 的2的因子数
 * @param rn 结果指针的 limb 长度
 * @param n 阶乘的阶数
 * @warning dst!=NULL, rn>0
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_factorial_(mp_ptr dst, mp_bitcnt_t bits, mp_size_t rn, uint n);

/**
 * @brief 计算 n!! 双阶乘的 limb 缓冲区长度
 * @param n 双阶乘的阶数
 * @param bits 被修改为 n!! 的2的因子数
 * @warning bits!=NULL
 * @return n!! 双阶乘的 limb 缓冲区长度
 */
LMMP_API mp_size_t lmmp_2factorial_size_(uint n, mp_bitcnt_t* bits);

/**
 * @brief 计算 n!! 双阶乘
 * @param dst 结果指针
 * @param bits n!! 的2的因子数
 * @param rn 结果指针的 limb 长度
 * @param n 双阶乘的阶数
 * @warning dst!=NULL, rn>0
 * @note 0的双阶乘为1，n为偶数时，n!!=2*4*...*n，n为奇数时，n!!=1*3*...*n
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_2factorial_(mp_ptr dst, mp_bitcnt_t bits, mp_size_t rn, uint n);

/**
 * @brief 计算hyper阶乘的 limb 缓冲区长度
 * @param n hyper阶乘的阶数
 * @param bits 被修改为 hyper阶乘的2的因子数
 * @warning bits!=NULL
 * @return hyper阶乘的 limb 缓冲区长度
 */
LMMP_API mp_size_t lmmp_hyperfac_size_(ushort n, mp_bitcnt_t* bits);

/**
 * @brief 计算hyper阶乘（k^k累乘至n）
 * @param dst 结果指针
 * @param bits hyper阶乘的2的因子数
 * @param rn 结果指针的 limb 长度
 * @param n hyper阶乘的阶数
 * @warning dst!=NULL, rn>0
 * @return dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_hyperfac_(mp_ptr dst, mp_bitcnt_t bits, mp_size_t rn, ushort n);

/**
 * @brief 计算super阶乘的 limb 缓冲区长度
 * @param n super阶乘的阶数
 * @param bits 被修改为 super阶乘的2的因子数
 * @warning bits!=NULL
 * @return super阶乘的 limb 缓冲区长度
 */
LMMP_API mp_size_t lmmp_superfac_size_(ushort n, mp_bitcnt_t* bits);

/**
 * @brief 计算super阶乘（k!累乘至n）
 * @param dst 结果指针
 * @param bits super阶乘的2的因子数
 * @param rn 结果指针的 limb 长度
 * @param n super阶乘的阶数
 * @warning dst!=NULL, rn>0
 * @return dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_superfac_(mp_ptr dst, mp_bitcnt_t bits, mp_size_t rn, ushort n);

/**
 * @brief 计算质数阶乘的 limb 缓冲区长度
 * @param n 质数阶乘的阶数
 * @return 质数阶乘的 limb 缓冲区长度
 */
LMMP_API mp_size_t lmmp_primefac_size_(uint n);

/**
* @brief 计算质数阶乘（不超过n的质数累乘）
* @param dst 结果指针
* @param rn 结果指针的 limb 长度
* @param n 质数阶乘的阶数
* @warning dst!=NULL, rn>0
* @return dst 的实际 limb 长度
*/
LMMP_API mp_size_t lmmp_primefac_(mp_ptr dst, mp_size_t rn, uint n);

/**
 * @brief 计算 nCr 组合数的 limb 缓冲区长度
 * @param n 组合数的总数
 * @param r 组合数的选择数
 * @param bits 被修改为 nCr 的2的因子数
 * @warning r<=n/2, bits!=NULL
 * @return nCr 组合数的 limb 缓冲区长度（比实际长度多 1-2 个 limb）
 */
LMMP_API mp_size_t lmmp_nCr_size_(uint n, uint r, mp_bitcnt_t* bits);

/**
 * @brief 计算 nCr 组合数的奇数部分
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param n 组合数的总数
 * @param r 组合数的选择数
 * @return 返回 dst 的实际 limb 长度
 * @warning r<=n/2, n<=0xffff, dst!=NULL, rn>0
 */
LMMP_API mp_size_t lmmp_odd_nCr_ushort_(mp_ptr dst, mp_size_t rn, uint n, uint r);

/**
 * @brief 计算 nCr 组合数的奇数部分
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param n 组合数的总数
 * @param r 组合数的选择数
 * @return 返回 dst 的实际 limb 长度
 * @warning r<=n/2, 0xffff<n, dst!=NULL, rn>0
 */
LMMP_API mp_size_t lmmp_odd_nCr_uint_(mp_ptr dst, mp_size_t rn, uint n, uint r);

/**
 * @brief 计算 nCr 组合数 ( nCr = n! / (r!(n-r)!) )
 * @param dst 结果指针
 * @param bits nCr 的2的因子数
 * @param rn 结果指针的 limb 长度
 * @param n 组合数的总数
 * @param r 组合数的选择数
 * @return 返回 dst 的实际 limb 长度
 * @warning r<=n/2, n<=0xffffffff, dst!=NULL, rn>0
 */
LMMP_API mp_size_t lmmp_nCr_(mp_ptr dst, mp_bitcnt_t bits, mp_size_t rn, uint n, uint r);

/**
 * @brief 计算多项式系数的 limb 缓冲区长度
 * @param r 需要计算的系数的数组
 * @param m 系数的个数
 * @param n 输出变量，将会被修改为 r[i] 的总和，即r1+r2+...+rm
 * @return 多项式系数的 limb 缓冲区长度（比实际长度多 1-2 个 limb）
 * @note 多项式系数为 ( r1+r2+...+rm )! / ( r1! * r2! * ... * rm!)
 * @warning 我们使用 ulong* n 来同时计算 r[i] 的总和，因为 n 可能超过 0xffffffff。
 *          我们预计算 n，这不仅可以作为后续多项式系数函数的参数传入。
 *          同时也请调用者注意判断 n 是否超过了 0xffffffff
 *          这是 lmmp_multinomial_ 函数的限制。
 */
LMMP_API mp_size_t lmmp_multinomial_size_(const uintp r, uint m, ulong* n);

/**
 * @brief 计算多项式系数
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param n r[i] 的总和
 * @param r 需要计算的系数的数组
 * @param m 系数的个数
 * @warning m>1, n>0
 * @note 多项式系数为 ( r1+r2+...+rm )! / ( r1! * r2! * ... * rm!)
 * @return 返回 dst 的实际 limb 长度
 */
LMMP_API mp_size_t lmmp_multinomial_(mp_ptr dst, mp_size_t rn, uint n, const uintp r, uint m);

/**
 * @brief 计算等差数列乘积 x(x+m)...(x+n*m)的 limb 缓冲区长度
 * @param x 首项
 * @param n 等差数列共n+1项
 * @param m 公差
 * @warning x>0, m>1, n>0, x+n*m<=0xffffffff
 * @return 等差数列乘积的 limb 缓冲区长度（比实际长度多 1-2 个 limb）
 */
LMMP_API mp_size_t lmmp_arith_seqprod_size_(uint x, uint n, uint m);

/**
 * @brief 计算等差数列乘积 x(x+m)...(x+n*m)
 * @param dst 结果指针
 * @param rn 结果指针的 limb 长度
 * @param x 首项
 * @param n 等差数列共n+1项
 * @param m 公差（大于1）
 * @warning x>0, m>1, n>0, dst!=NULL, rn>0, x+n*m<=0xffffffff
 * @return 结果的实际的 limb 缓冲区长度
 */
LMMP_API mp_size_t lmmp_arith_seqprod_(mp_ptr dst, mp_size_t rn, uint x, uint n, uint m);

/**
 * @brief 试除法，若存在不超过 N 的质因数，则返回true，否则返回false
 * @param np 被除数指针
 * @param nn 被除数的 limb 长度
 * @param N 试除法尝试的质数最大值
 * @warning num!=NULL, nn>0, N>2
 * @note 试除法尝试从 2-N 中所有质数进行试除，如果存在可以整除的素数则立即返回true
 * @return 若存在不超过N，且能整除[np,nn]的素数则返回true，若没有能够整除的素数，则返回false
 */
LMMP_API bool lmmp_trialdiv_(mp_srcptr np, mp_size_t nn, ushort N);

/**
 * @brief 除去[np,nn]中的[dp,dn]的因子
 * @param np 被除数指针，将会被修改为除去因子后的数
 * @param nn 被除数指针的 limb 长度，将会被修改除去因子后的长度
 * @param dp 除数指针
 * @param dn 除数指针的 limb 长度
 * @warning np!=NULL, nn>0, dp!=NULL, dn>0
 * @note 如果[np,nn]能被[dp,dn]整除，则[np,nn]将被修改为除去因子后的数，nn将被修改为除去因子后的长度。
 *       如果不能被整除，则[np,nn]保持不变，并返回0。
 * @return [np,nn]中被[dp,dn]除去的因子的个数，如果不能被整除，则返回0
 */
LMMP_API mp_size_t lmmp_remove_(mp_ptr np, mp_size_t* nn, mp_srcptr dp, mp_size_t dn);

/**
 * @brief 计算算术平方根 floor(sqrt(a))
 * @param a 被开方数
 * @return floor(sqrt(a))
 */
LMMP_API ulong lmmp_sqrt_ulong_(ulong a);

/**
 * @brief 计算算术平方根 floor(sqrt(x))
 * @param dstr 余数指针（1个limb）
 * @param x 被开方数
 * @warning x>=B/4, dstr!=NULL
 * @note [dstr,1]=sqrtrem(x), return floor(sqrt(x))
 * @return floor(sqrt(x))
 */
LMMP_API mp_limb_t lmmp_sqrt_1_(mp_ptr dstr, mp_limb_t x);

/**
 * @brief 计算算术平方根 floor(sqrt([numa,2]))
 * @param dstr 余数指针（2个limb）
 * @param numa 被开方数指针
 * @warning numa[1]>=B/4, dstr!=NULL, numa!=NULL, eqsep(dstr,numa)
 * @note return: floor(sqrt([numa,2])), [dstr,2]=remainder
 * @return floor(sqrt([numa,2]))
 */
LMMP_API mp_limb_t lmmp_sqrt_2_(mp_ptr dstr, mp_srcptr numa);

/**
 * @brief 计算算术平方根 floor(sqrt([numa,3]))
 * @param dsts 结果指针（2个limb）
 * @param dstr 余数指针（3个limb，最高位补零），传 NULL 表示不计算余数
 * @param numa 被开方数指针
 * @warning numa[2]>=B/4, dsts!=NULL, numa!=NULL, eqsep(dsts,numa), eqsep(dstr,numa)
 * @note [dstr,3] = [numa,3] - [dsts,2]^2 <= 2*[dsts,2]
 */
LMMP_API void lmmp_sqrt_3_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa);

/**
 * @brief 计算算术平方根 floor(sqrt([numa,4]))
 * @param dsts 结果指针（2个limb）
 * @param dstr 余数指针（3个limb），传 NULL 表示不计算余数
 * @param numa 被开方数指针
 * @warning numa[3]>=B/4, dsts!=NULL, numa!=NULL, eqsep(dsts,numa), eqsep(dstr,numa)
 * @note [dstr,3] = [numa,4] - [dsts,2]^2 <= 2*[dsts,2]
 */
LMMP_API void lmmp_sqrt_4_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa);

/**
 * @brief 计算算术平方根 floor(sqrt([numa,2*ns]))
 * @param dst 结果指针（ns个limb）
 * @param numa 被开方数指针（2*ns个limb，且会被修改，若计算余数，余数存储在[numa,ns+1]中）
 * @param ns 结果指针的 limb 长度
 * @param tp 临时指针（3*ns/2+1个limb）
 * @param calr 是否计算余数（0表示不计算余数，1表示计算余数）
 * @warning numa[2*ns-1]>=B/4, dst!=NULL, numa!=NULL, tp!=NULL, sep(dst,numa,tp)
 * @note 即使输入calr=0，numa也会被修改，如果calr=1，则[numa,ns+1]将会储存余数。
 */
LMMP_API void lmmp_sqrt_divide_(mp_ptr dst, mp_ptr numa, mp_size_t ns, mp_ptr tp, int calr);

/**
 * @brief 计算逆平方根的定点下近似：记 I=floor(sqrt(B^(2*ns+na)/[numa,na]))，
 *        则 [dstis,ns+1] 满足 I-1 <= [dstis,ns+1] <= I，且 dstis[ns]=1
 * @param dstis 目标数组
 * @param ns dstis数组的 limb 长度为 ns+1
 * @param numa 输入数组
 * @param na numa数组的 limb 长度
 * @warning ns>=3, na>0, ns>=na（否则语义变化，见@note）, numa[na-1]>=B/4,
 *          dstis!=NULL, numa!=NULL, sep(dstis,numa)
 * @note 结果是 I 的至多低估 1 的下近似，绝不高估；随机输入实测几乎恒等于 I，
 *       仅当 I 邻近 B^ns 或 2*B^ns 的构造输入（如 [numa,na]=B^na/4、B^na-1 等）
 *       会得到 I-1。
 *       恒有 dstis[ns]=1，即结果落在 [B^ns, 2*B^ns) 内。
 *       当 ns<na 时语义不同：仅使用 numa 的最高 ns 个 limb（记为 a_top），
 *       结果变为 floor(sqrt(B^(3*ns)/a_top))-[0|1]，与 I 相差 B^((ns-na)/2) 倍。
 */
LMMP_API void lmmp_invsqrt_newton_(mp_ptr dstis, mp_size_t ns, mp_srcptr numa, mp_size_t na);

/**
 * @brief 计算近似平方根 [dsts,nf+na/2+1]=[floor|round](sqrt([numa,na]*B^(2*nf)))
 * @param dsts 目标数组（长度为 nf+na/2+1 个limb）
 * @param numa 输入数组（长度为 na 个limb）
 * @param na numa数组的 limb 长度
 * @param nf 精度因子
 * @warning na>0, nf>=2, nf+na/2+1>=na, dsts!=NULL, numa!=NULL, eqsep(dsts,numa)
 * @note 设 x = sqrt([numa,na]*B^(2*nf))（实数），round 为四舍五入（恰为 1/2 时进位），
 *       则结果恰为 round(x-eps)，其中 0 <= eps < 2^-31（na 为奇数）或 2^-63（na 为偶数）。
 *       eps 非负，故结果绝不超过 round(x)；仅当 x 的小数部分落在 [1/2, 1/2+eps)
 *       时得到 floor(x) 而非 floor(x)+1（随机输入下概率 < 2^-30），此即
 *       [floor|round] 的确切含义。
 */
LMMP_API void lmmp_sqrt_newton_(mp_ptr dsts, mp_srcptr numa, mp_size_t na, mp_size_t nf);

/**
 * @brief 计算 [numa,na] * B^(2*nf) 的平方根和余数
 * @param dsts 平方根结果输出指针（长度为 nf+na/2+1 个limb）
 * @param dstr 余数结果输出指针（NULL表示不计算余数）
 * @param numa 源操作数指针（长度为 na 个limb）
 * @param na 操作数的 limb 长度
 * @param nf 精度因子
 * @note if (dstr != NULL) {
 *           [dsts,nf+na/2+1], [dstr,nf+na/2+1] = sqrtrem([numa,na]*B^(2*nf))
 *       } else {
 *           if (nf == 0) {
 *               [dsts,na/2+1] = floor(sqrt([numa,na]))
 *           } else {
 *               [dsts,nf+na/2+1] = [floor|round](sqrt([numa,na]*B^(2*nf)))
 *           }
 *       }
 * @attention 如你需要精确的floor(sqrt(x))语义，请确保nf==0
 * @warning na>0, numa[na-1]!=0, eqsep(dsts,numa), eqsep(dstr,numa)
 */
LMMP_API void lmmp_sqrt_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa, mp_size_t na, mp_size_t nf);

/**
 * @brief 计算算数立方根 floor(cbrt(n))
 * @param n 被开方数
 * @return floor(cbrt(n))
 * @warning n>0
 * @note 使用Chebyshev估计，当n较大时，此算法更占优势
 */
LMMP_API ulong lmmp_cbrt_chebyshev_(ulong n);

/**
 * @brief 计算算数立方根 floor(cbrt(n))
 * @param n 被开方数
 * @return floor(cbrt(n))
 * @note 会依据n的大小，选择合适的算法
 */
LMMP_API ulong lmmp_cbrt_ulong_(ulong n);

/**
 * @brief 计算算数立方根 floor(cbrt(a0+a1*B+a2*B^2))
 * @param a0 低位 limb
 * @param a1 中位 limb
 * @param a2 高位 limb
 * @warning a1和a2应至少有一个大于0
 * @note a2可以为0，但a1需要大于0，即这个数至少应有65个bit
 * @return floor(cbrt(a0+a1*B+a2*B^2))
 */
LMMP_API mp_limb_t lmmp_cbrt_3_(mp_limb_t a0, mp_limb_t a1, mp_limb_t a2);

/**
 * @brief 计算算术立方根 floor(cbrt([numa,na]))
 * @param dst 结果指针（2个limb）
 * @param numa 被开方数指针
 * @param na 被开方数的 limb 长度
 * @warning 3<na<=6, numa[na-1]!=0, dst!=NULL, numa!=NULL, sep(dst,numa)
 */
LMMP_API void lmmp_cbrt_6_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief 计算算术立方根 floor(cbrt([numa,3*ns]))
 * @param dst 结果指针（ns个limb）
 * @param numa 被开方数指针（3*ns个limb，且会被修改，若计算余数，余数存储在[numa,2*ns+1]中）
 * @param ns 结果指针的 limb 长度
 * @param tp 临时指针（6*ns+4个limb）
 * @param calr 是否计算余数（0表示不计算余数，1表示计算余数）
 * @warning numa[3*ns-1]>=0x6000000000000000, dst!=NULL, numa!=NULL, tp!=NULL, sep(dst,numa,tp)
 * @note 即使输入calr=0，numa也会被修改，如果calr=1，则[numa,2*ns+1]将会储存余数。
 */
LMMP_API void lmmp_cbrt_divide_(mp_ptr dst, mp_ptr numa, mp_size_t ns, mp_ptr tp, int calr);

/**
 * @brief 计算逆立方根的定点下近似：记 I=floor(cbrt(B^(3*ns+na)/[numa,na]))，
 *        则 [dstis,ns+1] 满足 I-1 <= [dstis,ns+1] <= I，且 dstis[ns]>=1
 * @param dstis 目标数组（长度为 ns+1 个limb）
 * @param ns dstis数组的 limb 长度为 ns+1
 * @param numa 输入数组
 * @param na numa数组的 limb 长度
 * @warning ns>=3, na>0, ns>=na, numa[na-1]>=B/8 (0x2000000000000000),
 *          dstis!=NULL, numa!=NULL, sep(dstis,numa)
 * @note 结果是 I 的至多低估 1 的下近似，绝不高估；随机输入实测几乎恒等于 I，
 *       仅当 I 邻近 B^ns 或 2*B^ns 的构造输入（如 [numa,na]=B^na-1、B^na/8
 *       等）会得到 I-1。
 *       恒有 dstis[ns]>=1，即结果落在 [B^ns, 2*B^ns] 内。
 *       当 ns<na 时语义不同：仅使用 numa 的最高 ns 个 limb（记为 a_top），
 *       结果变为 floor(cbrt(B^(4*ns)/a_top))-[0|1]，与 I 相差 B^((ns-na)/3) 倍。
 */
LMMP_API void lmmp_invcbrt_newton_(mp_ptr dstis, mp_size_t ns, mp_srcptr numa, mp_size_t na);

/**
 * @brief 计算近似立方根 [dsts,na/3+nf+1]=[floor|round](cbrt([numa,na]*B^(3*nf)))，
 *        输入须已归一化：na ≡ 0 (mod 3) 且顶 limb >= B/8
 * @param dsts 目标数组（长度至少 na/3+nf+1 个limb）
 * @param numa 输入数组（长度为 na 个limb，已 3 对齐归一化）
 * @param na numa数组的 limb 长度（3 的倍数）
 * @param nf 精度因子
 * @warning na>0, na%3==0, numa[na-1]>=B/8, nf>=2, 3*nf>=2*na+3, dsts!=NULL,
 *          numa!=NULL, sep(dsts,numa)
 * @note 设 x = cbrt([numa,na]*B^(3*nf))（实数），round 为四舍五入（恰为 1/2 时进位），
 *       则结果恰为 round(x-eps)，其中 0 <= eps < 2^-62。eps 非负，故结果绝不超过
 *       round(x)；仅当 x 的小数部分落在 [1/2, 1/2+eps) 时得到 floor(x) 而非
 *       floor(x)+1（随机输入下概率 < 2^-62），此即 [floor|round] 的确切含义。
 */
LMMP_API void lmmp_cbrt_newton_(mp_ptr dsts, mp_srcptr numa, mp_size_t na, mp_size_t nf);

/**
 * @brief 计算 [numa,na] * B^(3*nf) 的立方根和余数
 * @param dsts 立方根结果输出指针（缓冲区长度至少 nf+na/3+3 个limb）
 * @param dstr 余数结果输出指针（缓冲区长度至少 2*(nf+na/3)+3 个limb，NULL 表示不计算余数）
 * @param numa 源操作数指针（长度为 na 个limb）
 * @param na 操作数的 limb 长度
 * @param nf 精度因子
 * @warning na>0, numa[na-1]!=0, eqsep(dsts,numa), eqsep(dstr,numa)
 * @note if (dstr != NULL) {
 *           [dsts],[dstr] = cbrtrem([numa,na]*B^(3*nf))，即精确 floor 立方根
 *           与余数 [numa,na]*B^(3*nf) - [dsts]^3（长度 2*ns+1，ns 为根长）
 *       } else {
 *           if (nf == 0) {
 *               [dsts] = floor(cbrt([numa,na]))
 *           } else {
 *               [dsts] = [floor|round](cbrt([numa,na]*B^(3*nf)))
 *           }
 *       }
 * @attention 如你需要精确的floor(cbrt(x))语义，请确保nf==0或传入dstr
 */
LMMP_API void lmmp_cbrt_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa, mp_size_t na, mp_size_t nf);

/**
 * @brief 计算 floor(n^(1/root))
 * @param n 被开方数
 * @param root 开方次数
 * @return floor(n^(1/root))
 * @note root=0时，返回0
 */
LMMP_API ulong lmmp_nthroot_ulong_(ulong n, ulong root);

/**
 * @brief 计算 lmmp_nthroot_ 所需的 tp 缓冲区长度（任意长度输入入口）
 * @param na 被开方数的 limb 长度
 * @param root 开方次数
 * @return tp 至少需要的 limb 数
 * @warning na>0, root>=4, root<=2^56
 */
LMMP_API mp_size_t lmmp_nthroot_size_(mp_size_t na, ulong root);

/**
 * @brief 计算算术根 [dsts,nr] = floor([numa,na]^(1/root))（任意 na/任意顶 limb）
 * @param dsts 结果指针（nr = ceil(na/root) 个 limb，顶 limb 恒非零）
 * @param dstr 余数结果输出指针（(root-1)*nr+1 个 limb，NULL 表示不计算余数）
 * @param numa 被开方数指针（只读）
 * @param na 被开方数的 limb 长度
 * @param root 开方次数
 * @param tp 临时区（至少 lmmp_nthroot_size_(na,root) 个 limb）
 * @warning na>0, numa[na-1]!=0, root>=4, root<=2^56, sep(dsts,dstr,numa,tp)
 * @note 任意长度输入的统一入口，恒为精确 floor 语义（dstr=NULL 不牺牲
 *       精度，与 lmmp_cbrt_/lmmp_sqrt_ 的 [floor|round] 口径不同）。
 *       对虚拟 Ã = A·2^root 求根后纯移位恢复 S（嵌套 floor 恒等式）：
 *       log2/exp2 128bit 定点种子 + 位窗牛顿精化（层成本 M(e2/64) 与
 *       root 解耦）+ 认证式窗口探针/全幂端点修正。root=4 走
 *       sqrt∘sqrt 专用路线（嵌套 floor 恒等式，余数由两级 sqrtrem
 *       代数展开）。ns<=1 域分派至 lmmp_nthroot_ulong_。
 */
LMMP_API void lmmp_nthroot_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa, mp_size_t na, ulong root, mp_ptr tp);

/**
 * @brief 计算 [p,n] % 2^48-1
 * @param p 被除数指针
 * @param n 被除数的 limb 长度
 * @warning p!=NULL, n>0
 * @return [numa,na] % 2^48-1
 */
LMMP_API mp_limb_t lmmp_mod_2p48sub1_(mp_srcptr p, mp_size_t n);

/**
 * @brief 非完全平方数过滤器
 * @param p 输入数
 * @warning p>0
 * @return false 意味着必定为非完全平方数，所有完全平方数和部分非完全平方数会返回 true
 * @note 此过滤器主要针对随机输入情况，对于大概率是完全平方数的输入，可以无需此函数。
 */
LMMP_API bool lmmp_perfsqr_filter_1_(mp_limb_t p);

/**
 * @brief 非完全平方数过滤器
 * @param p 输入指针
 * @param n 输入的 limb 长度
 * @warning p!=NULL, n>0, p[n-1]>0
 * @return false 意味着必定为非完全平方数，所有完全平方数和部分非完全平方数会返回 true
 * @note 此过滤器主要针对随机输入情况，对于大概率是完全平方数的输入，可以无需此函数。
 */
LMMP_API bool lmmp_perfsqr_filter_(mp_srcptr p, mp_size_t n);

/**
 * @brief 判断[p,n]是否为完全平方数
 * @param p 输入指针
 * @param n 输入的 limb 长度
 * @warning p!=NULL, n>0, p[n-1]>0
 * @return 为完全平方数返回 true，否则返回 false
 */
LMMP_API bool lmmp_perfsqr_(mp_srcptr p, mp_size_t n);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LMMP_NUMTH_H
