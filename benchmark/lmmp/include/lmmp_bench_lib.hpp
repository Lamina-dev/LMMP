/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef LMMP_BENCH_LIB_HPP
#define LMMP_BENCH_LIB_HPP

#include "lmmp/lmmp.h"
#include "lmmp/lmmpn.h"
#include "lmmp/mprand.h"
#include "lmmp/numth.h"
#include "lmmp_bench.hpp"

/*
    基准函数库：全部以操作规模为入参、返回 double（微秒/op）。

    长度入参（limb 数）覆盖多精度运算，如 bench_mul(a, b) 测量 [a] 与 [b]
    limbs 随机数组的乘法耗时；位宽入参（bit 数）覆盖标量数论判定，如
    bench_is_prime_ulong(bits)。采样次数与聚合策略经末位 BenchOpts 按上下文
    指定，缺省为 5 样本取最小值。

    调用前须已 lmmp_global_init()（用例程序 main 已保证）。
*/

namespace lmmp_bench {

/* volatile 汇，防止标量返回值被优化掉 */
inline volatile mp_limb_t sink = 0;

/* 分配 n limbs（含库内分配记账） */
inline mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

/* [p,n] = 随机数，顶 limb 置最高位（归一化），seed 固定保证可复现 */
inline void fill_random(mp_ptr p, mp_size_t n, mp_limb_t seed) {
    lmmp_seed_random_(p, n, seed, 1);
    if (n > 0) p[n - 1] |= (mp_limb_t)1 << 63;
}

/* ---- 打印辅助：统一 "name(规模)" 标签格式 ---- */
inline void report_n(const char* name, mp_size_t n, double us, double bytes = 0.0) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s(%lluL)", name, (unsigned long long)n);
    report(buf, us, bytes);
}

inline void report_ab(const char* name, mp_size_t a, mp_size_t b, double us, double bytes = 0.0) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s(%llux%llu)", name, (unsigned long long)a,
                  (unsigned long long)b);
    report(buf, us, bytes);
}

inline void report_bits(const char* name, uint bits, double us) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s(%ub)", name, bits);
    report(buf, us);
}

/* ---- lmmpn 低层原语（长度入参，[p,n] 均为随机归一化数组） ---- */
double bench_add_n(mp_size_t n, const BenchOpts& o = {});
double bench_add_nc(mp_size_t n, const BenchOpts& o = {});
double bench_sub_n(mp_size_t n, const BenchOpts& o = {});
double bench_sub_nc(mp_size_t n, const BenchOpts& o = {});
double bench_mul_1(mp_size_t n, const BenchOpts& o = {});
double bench_addmul_1(mp_size_t n, const BenchOpts& o = {});
double bench_submul_1(mp_size_t n, const BenchOpts& o = {});
double bench_addshl1_n(mp_size_t n, const BenchOpts& o = {});
double bench_subshl1_n(mp_size_t n, const BenchOpts& o = {});
double bench_shl(mp_size_t n, const BenchOpts& o = {});
double bench_shr(mp_size_t n, const BenchOpts& o = {});
double bench_shlnot(mp_size_t n, const BenchOpts& o = {});
double bench_not(mp_size_t n, const BenchOpts& o = {});
double bench_shr1add_n(mp_size_t n, const BenchOpts& o = {});
double bench_shr1sub_n(mp_size_t n, const BenchOpts& o = {});

/**
 * @brief lmmp_mul_basecase_ 基础乘法耗时
 * @param a 被乘数长度（limbs）
 * @param b 乘数长度（limbs），无需 a>=b（内部自动交换）
 */
double bench_mul_basecase(mp_size_t a, mp_size_t b, const BenchOpts& o = {});
double bench_sqr_basecase(mp_size_t n, const BenchOpts& o = {});
double bench_mullo_basecase(mp_size_t n, const BenchOpts& o = {});

double bench_div_1(mp_size_t n, const BenchOpts& o = {});
double bench_div_1_s(mp_size_t n, const BenchOpts& o = {});
double bench_mod_1(mp_size_t n, const BenchOpts& o = {});
double bench_div_2(mp_size_t n, const BenchOpts& o = {});
double bench_div_2_s(mp_size_t n, const BenchOpts& o = {});
double bench_mod_2(mp_size_t n, const BenchOpts& o = {});
double bench_div_3_2(const BenchOpts& o = {});
double bench_inv_1(const BenchOpts& o = {});
double bench_inv_2_1(const BenchOpts& o = {});

/* ---- lmmpn 高层运算（长度入参） ---- */
/**
 * @brief lmmp_mul_ 乘法耗时：[a] 与 [b] limbs 随机数组相乘
 * @param a 操作数一长度（limbs）
 * @param b 操作数二长度（limbs），无需 a>=b（内部自动交换）
 */
double bench_mul(mp_size_t a, mp_size_t b, const BenchOpts& o = {});
double bench_mul_n(mp_size_t n, const BenchOpts& o = {});
double bench_sqr(mp_size_t n, const BenchOpts& o = {});

/**
 * @brief Toom 乘法耗时（lmmp_mul_toom22_/33_/44_）
 * @warning 库契约要求近平衡（如 toom22 需 4/5<=min/max），不平衡输入请用 bench_mul
 */
double bench_mul_toom22(mp_size_t a, mp_size_t b, const BenchOpts& o = {});
double bench_mul_toom33(mp_size_t a, mp_size_t b, const BenchOpts& o = {});
double bench_mul_toom44(mp_size_t a, mp_size_t b, const BenchOpts& o = {});
double bench_mul_fft(mp_size_t a, mp_size_t b, const BenchOpts& o = {});

/**
 * @brief 梅森模乘耗时（lmmp_mul_mersenne_fft_/dc_，等长 [n,n] 输入，
 *        rn = 对应 next_size_((n+n+1)>>1) 内部计算）
 * @warning 梅森域约束 0<=[numa,n]<B^rn 仅平衡输入可满足，故只收单一长度
 */
double bench_mul_mersenne_fft(mp_size_t n, const BenchOpts& o = {});
double bench_mul_mersenne_dc(mp_size_t n, const BenchOpts& o = {});

double bench_mullo(mp_size_t n, const BenchOpts& o = {});
double bench_mullo_dc(mp_size_t n, const BenchOpts& o = {});
double bench_mullo_fft(mp_size_t n, const BenchOpts& o = {});

/**
 * @brief lmmp_div_ 除法耗时：[a] limbs 被除数除以 [b] limbs 归一化除数
 * @param a 被除数长度（limbs）
 * @param b 除数长度（limbs），无需 a>=b（内部自动交换）
 */
double bench_div(mp_size_t a, mp_size_t b, const BenchOpts& o = {});
double bench_sqrt(mp_size_t n, const BenchOpts& o = {});
double bench_gcd_lehmer(mp_size_t n, const BenchOpts& o = {});
double bench_gcd_hgcd(mp_size_t n, const BenchOpts& o = {});

/**
 * @brief lmmp_fibonacci_ 耗时
 * @param n 斐波那契下标（结果规模内部由 lmmp_fibonacci_size_ 推导）
 */
double bench_fib(ulong n, const BenchOpts& o = {});

/**
 * @brief lmmp_factorial_ 耗时
 * @param n 阶乘下标（结果规模内部由 lmmp_factorial_size_ 推导）
 */
double bench_factorial(uint n, const BenchOpts& o = {});

/* ---- numth 模幂（长度入参，RSA 典型形态：指数与模数同规模） ---- */
/**
 * @brief lmmp_powmod_odd_ 奇模数模幂耗时（Montgomery REDC + 滑动窗口）
 * @param n 模数/指数/底数长度（limbs），指数顶位置 1
 */
double bench_powmod_odd(mp_size_t n, const BenchOpts& o = {});

/**
 * @brief lmmp_powmod_ 偶模数模幂耗时（CRT：奇部 REDC + 2^k 部 powlo）
 * @param n 模数等长度（limbs）
 * @param nb2 模数 2 的幂因子档位：m = 2^(64*(nb2-1)+1)*odd（nb2=1 即 m=2*odd）
 * @warning 1<=nb2<n，n<3 时 nb2 强制为 1
 */
double bench_powmod_even(mp_size_t n, uint nb2 = 1, const BenchOpts& o = {});
double bench_powlo(mp_size_t n, const BenchOpts& o = {});

/* ---- numth 素性判定（位宽入参） ---- */

/* 素性基准的输入池形态：随机奇数（合数为主）/ 素数（最坏情形全基底执行）
   / 试除幸存者（无 <=1000 素因子，齐次完整负载） */
enum class PrimePool { ODD, PRIME, COPRIME };

/**
 * @brief lmmp_is_prime_notrial_ 耗时（免试除判素）
 * @param bits 输入位宽（2..64；PRIME 池经 next_prime 可能上跨一档）
 */
double bench_is_prime_notrial(uint bits, PrimePool kind = PrimePool::ODD, const BenchOpts& o = {});

/**
 * @brief lmmp_is_prime_ulong_ 耗时
 * @param bits 输入位宽（2..64；PRIME 池经 next_prime 可能上跨一档）
 */
double bench_is_prime_ulong(uint bits, PrimePool kind = PrimePool::ODD, const BenchOpts& o = {});

/**
 * @brief lmmp_is_prime_2_ 耗时（双 limb 判素）
 * @param bits 输入位宽（65..128，顶位恒置 1）
 */
double bench_is_prime_2(uint bits, PrimePool kind = PrimePool::ODD, const BenchOpts& o = {});

/**
 * @brief lmmp_is_prime_n_ 耗时
 * @param n 输入长度（limbs，顶位置 1）
 * @param strength 强度档（0/2/4/7...）
 */
double bench_is_prime_n(mp_size_t n, int strength = 4, PrimePool kind = PrimePool::COPRIME,
                        const BenchOpts& o = {});
double bench_is_strong_lucas(mp_size_t n, PrimePool kind = PrimePool::COPRIME,
                             const BenchOpts& o = {});

/**
 * @brief lmmp_is_prothprime_ 耗时（Proth 数 N=k*2^t+1 单梯子定理判决）
 * @param t 2 的幂指数（N 的规模档，t>=1），k<=2^32 内部搜索
 * @param prime_pool true 构造 Proth 素数池（判决最贵），false 试除幸存合数池
 */
double bench_is_prothprime(uint t, bool prime_pool = true, const BenchOpts& o = {});

/**
 * @brief lmmp_is_prime_n_ 在 Proth 形状输入上的耗时（与 bench_is_prothprime 同池对照）
 */
double bench_is_prime_n_proth(uint t, int strength = 4, bool prime_pool = true,
                              const BenchOpts& o = {});

}  // namespace lmmp_bench

#endif  // LMMP_BENCH_LIB_HPP
