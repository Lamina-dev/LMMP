/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#ifndef LMMP_MPRAND_H
#define LMMP_MPRAND_H

/*
 本头文件提供的随机数发生器均为非密码学安全的伪随机数生成器，仅用于生成随机数序列。
 输入种子类型为int时，意味着无需输入高熵的随机数，我们会通过一些额外的信息（生成序列的长度）
 通过一些简单的算法来生成高熵的新种子（依然可以保证可重用和可复现性）。对于种子类型为
 其他类型，我们会直接使用输入的种子，我们仍然会在发生器中进行一些处理，以保证最低限度的随机性。

 LMMP 主要实现了两个随机数引擎：PCG-XSL-RR-128/64 和 xoshiro256++。
 其中，后者通常为LMMP默认的随机数引擎，生成速度较前者快，大约有1.5倍性能差距。
 可以参考以下文章了解PCG：<https://www.pcg-random.org/paper.html>

 PCG网站<https://www.pcg-random.org>中也有PCG和xoshiro256++及其他随机生成器的比较。

 PCG-XSL-RR-128/64 生成的随机数序列通常拥有比xoshiro256++更好的统计性能，当然两者的统计性能
 在大部分场景下都足够的好。生成速度上，在64位平台且吞吐量较好的情况下，各个随机数发生器平均生成
 单个limb的耗时大致如下：（在生成较短bit长度的随机数时，所有的发生器平均耗时都会提高）
 
    随机生成器          平均耗时（ns）
     mt19937              3.9
  PCG-XSL-RR-128/64       1.3
   xoshiro256++           0.8
    strong_rng            1.4/2.9

    （备注：strong_rng第一个耗时未考虑随机状态初始化的时间，第二个耗时是在初始化状态后，生成单个limb的耗时）

 LMMP 实现的强随机数生成器，通常用于多次大量生成较长的随机大整数序列，比如用于多次生成
 1000个limb长度的随机大整数。其随机状态为一个长度为 k 的limb数组，初始状态由种子决定，每个limb代表一个64bit状态，
 初始状态会由种子和limb的位置通过复杂的hash算法确定，因此初始状态几乎完全不同，用以保证同一个大整数中的各个limb之
 间几乎没有相关性。
 
 在生成单个随机大整数时，强随机生成器的总生成速度会显著慢于PCG-XSL-RR-128/64和xoshiro256++，与cpp标准库的
 梅森旋转法大致相当。这是由于初始化随机状态的巨大开销导致的。在多次生成固定长度的随机大整数序列时，强随机生成
 器的生成速度则比pcg-xsl-rr-128/64略慢，但仍然快于梅森旋转算法（Mersenne Twister）。
 */

#include "lmmp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化全局随机数生成器
 * @param seed 种子
 * @param seed_type 随机数发生器类型（0：pcg64_128，1：xoshiro256++）
 * @warning seed_type 必须在0到1之间，如果不是，则使用 seed_type%2 作为seed_type，
 *          如果全局种子已经初始化，则会更新全局种子，或更新发生器类型
 * @note 由于使用int类型作为种子，我们将会维护一个全局种子随机序列，丰富种子的随机性。
 */
LMMP_API void lmmp_global_rng_init_(int seed, int seed_type);

/**
 * @brief 生成随机大整数（0 - B^n-1 均匀分布）
 * @param dst 随机数存储位置
 * @param n dst的 limb 长度
 * @param seed 种子（建议使用有效位数较多的随机数，以保证随机数质量）
 * @param seed_type 随机数发生器类型（0：pcg64_128，1：xoshiro256++）
 * @warning seed_type 必须在0到1之间，如果不是，则使用 seed_type%2 作为seed_type
 * @return 随机数的 limb 长度（这是由于可能存在生成随机数为0的情况（虽然几乎不可能），
 *         所以返回值可能小于n，但不会大于n）
 */
LMMP_API mp_size_t lmmp_seed_random_(mp_ptr dst, mp_size_t n, mp_limb_t seed, int seed_type);

/**
 * @brief 生成随机大整数（0 - B^n-1 均匀分布）
 * @param dst 随机数存储位置
 * @param n dst的 limb 长度
 * @warning 如果dst==NULL或n==0，则返回0，无其他操作。种子由 lmmp_global_rng_init() 设置，发生器类型由 
 *          lmmp_global_rng_init() 设置，如果没有进行全局初始化，则使用默认种子（默认等价设置全局种子为0，
 *          并不代表全局种子为0）和默认发生器类型（默认为xoshiro256++），即未设置全局种子，行为等价于 
 *          执行了 lmmp_global_rng_init_(0, 1)
 * @note 每调用一次此函数，种子将会进行一次更新，以确保多次调用时的种子不同，但是只要每次调用的方式
 *       和顺序相同，在同一个进程中，每次生成的随机数序列相同。
 * @return 随机数的 limb 长度（由于可能存在生成随机数为0的情况，所以返回值可能小于n，但不会大于n）
 */
LMMP_API mp_size_t lmmp_random_(mp_ptr dst, mp_size_t n);

/**
 * @brief 强随机数生成器结构体
 * @note 内部状态为一个长度为 k 的limb数组，初始状态由种子决定，每生成一次，内部状态将会更新，
 *       内部状态的每个维度都是一个64位无符号整数，且可以认为是独立的。故通常情况下，此生成器得到的
 *       随机数拥有极好的k-维均匀性。对于每个独立的limb，为了提高生成速度，我们使用pcg64-le随机数发生器
 *       虽然各个limb间算法相同，但初始化状态几乎完全不同。
 */
typedef struct lmmp_strong_rng_t lmmp_strong_rng_t;

/**
 * @brief 创建k维度强随机数生成器
 * @param k 随机数长度（单位：limb）
 * @param seed 种子
 * @warning k>0
 * @note 请注意，此操作是一个开销很大的操作，且会分配内存，在仅生成一次的情况下，甚至初始化时间会慢于生成
 *       一个随机数的时间。
 * @return 强随机数生成器指针
 */
LMMP_API lmmp_strong_rng_t* lmmp_strong_rng_init_(mp_size_t k, int seed);

/**
 * @brief 将rng内部状态拓展至k维度
 * @param rng 强随机数生成器指针
 * @param k 随机数长度（单位：limb）
 * @warning rng!=NULL, k>0
 * @note 若k<=rng->stream.k，则不进行任何操作。否则，将rng内部状态拓展至k维度。
 */
LMMP_API void lmmp_strong_rng_extern_(lmmp_strong_rng_t* rng, mp_size_t k);

/**
 * @brief 销毁强随机数生成器
 * @param rng 强随机数生成器指针
 */
LMMP_API void lmmp_strong_rng_free_(lmmp_strong_rng_t* rng);

/**
 * @brief 生成n维度强随机数（0 - B^n-1 均匀分布）
 * @param dst 随机数存储位置（长度为k个limb）
 * @param n dst的 limb 长度（n<=k）
 * @param rng 强随机数生成器指针，每生成一次，内部状态将会更新
 * @warning rng!=NULL, dst!=NULL, 0<n<=k, 
 * @note rng为强随机数生成器指针，每调用一次此函数，内部状态将会更新，以进行重复生成长度相同的随机大整数序列
 *       此方法生成的随机数序列具有极好的k-维均匀性，单个随机大整数间的各个limb都是几乎完全独立的序列。
 * @return 随机数的 limb 长度（由于可能存在生成随机数为0的情况，所以返回值可能小于n，但不会大于n）
 */
LMMP_API mp_size_t lmmp_strong_random_(mp_ptr dst, mp_size_t n, lmmp_strong_rng_t* rng);

#ifdef __cplusplus
}
#endif

#endif // LMMP_MPRAND_H
