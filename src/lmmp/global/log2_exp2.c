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

#include "../../../include/lmmp/impl/log2_exp2.h"
#include "../../../include/lmmp/impl/longlong.h"

/*
    记 B = 2^64，x 为 [0,1) 上的小数（64bit 或 128bit 定点），本文件计算
    log2(1+x) 与 2^x - 1 的定点值。结构为“查表约简 + 短泰勒级数”：
    log2 查表索引为 5bit（级数 27/14 项），exp2 为 7bit 且 Q 表由
    16/128 两级表运行时合成（级数 13/7 项）。

    约简恒等式（i 为索引，xl 为剩余低位）：

      log2(1+x) = L[i] + log2(1+u)，u = xl/(2^k*(32+i)) < 2^-5
      2^x - 1   = Q * (2^t)，Q = 2^(i/128)，t < 2^-7

    u 由倒数表（2^128/(32+i) 的定点值 lg2_R32）一次乘法获得。级数
    按奇偶次幂分组（w = u^2 或 t^2，均为正系数规避符号分支）：

        log2(1+u) = u*B(w) - w*A(w)
        2^t - 1   = t*B(w) + w*A(w)

    项数由首个截断项 < 2^-66（64bit 输出）/ 2^-130（128bit 输出）
    确定：log2 侧 u < 2^-5 故 m 项覆盖 m*5 个 bit（27/14 项），exp2
    侧 m*7（13/7 项）。lg2 系数按 2^191 尺度截取高 2 limb 存储（c1 =
    1/ln2 > 1 超出定点范围，链与收尾按半尺度 2^63/2^127 推进、结果
    左移 1 位恢复；低 limb 为死数据不存）；xp2 系数均 < 1，按满尺度
    2^192 截取高 2 limb 存储。128bit 版 B/A 链为单循环双累加器交错
    Horner——相对全展开 Estrin 代码减半且无寄存器溢出，实测更快。

    舍入控制：约简变量 u 的定点化为误差主源（~0.7 ulp），其余各级
    floor 系统性偏负，故 64bit 版 B/A 链取 floor 而收尾以 +2^63 /
    舍入位中心化，exp2 的表合成（T16/T8 四舍五入存、T8 运行时再舍入
    取高位）与 s、Q*s 项亦取舍入值。实测全区间误差 |e| <= 2 ulp，
    接口承诺 |e| <= 2。

    常量表由纯整数高精度脚本离线生成（scripts/lmmp/gen_log2_exp2.py，
    atanh/exp 级数求值，ln2 取 768bit 定点），每张表首行为 i=0。
*/

static const uint64_t lg2_L32[32][3] = {
    {0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull},
    {0x6366f25ae368399cull, 0x9b03784b5be08490ull, 0x0b5d69bac77ec398ull},
    {0xca507cfab1d27f98ull, 0xcc53826144575ac3ull, 0x1663f6fac913167cull},
    {0x9e77fd40e0401d05ull, 0x4f78dfa14aa5157aull, 0x2118b119b4f3c72cull},
    {0x49e7cd4744b36080ull, 0x401624140d175ba2ull, 0x2b803473f7ad0f3full},
    {0x41057dd4287e22f1ull, 0xc23d9780306c696aull, 0x359ebc5b69d927dfull},
    {0x99eaddb5a9520d6aull, 0x51b3314f09de6be4ull, 0x3f782d7204d01447ull},
    {0x41e86a7632602708ull, 0x16e52e91300efeefull, 0x49101eac381ce609ull},
    {0x65b157f8deceb53aull, 0x24afdbfd36bf6d33ull, 0x5269e12f346e2bf9ull},
    {0x9f890503285f6d81ull, 0x35482d13dc0f110cull, 0x5b8887367433795eull},
    {0x5dba8beba3cb180bull, 0xcad415ae1a715618ull, 0x646eea247c5c22d2ull},
    {0x14a805ee427197fcull, 0x51bbe3f6289e3ab7ull, 0x6d1fafdce20a8290ull},
    {0x3e730bb7410e895bull, 0xfaf866415554d6bfull, 0x759d4f80cba83bf8ull},
    {0xaf992540238215bbull, 0x64c6001143d6c8d5ull, 0x7dea15a32c1b3b38ull},
    {0x3f2869dd6be1d1ccull, 0x12ba94db12ef0aa8ull, 0x86082806b1d532c4ull},
    {0xeac6aab2e0ef8a67ull, 0xa89d4ee66c3700e3ull, 0x8df988f4ae806f1dull},
    {0x24f3e6a3a259b040ull, 0xa00b120a068badd1ull, 0x95c01a39fbd6879full},
    {0x718d4a9002e2cf95ull, 0x5592074827cb508eull, 0x9d5d9fd5010b3666ull},
    {0xcb62aff1bd9d6a74ull, 0x495fb7fa6d7eda66ull, 0xa4d3c25e68dc57f2ull},
    {0xef44639e542c2fd8ull, 0x6c5e946b4ae30894ull, 0xac241134c4e99e1cull},
    {0x1cf483d2900676c7ull, 0x76da1c872983511eull, 0xb35004723c465e69ull},
    {0xbf0dfbe6ebff7d3bull, 0x2bc1fe8a8648e9ebull, 0xba58feb2703a9e37ull},
    {0x6edbb3eae70d10c1ull, 0xe021361e13a30973ull, 0xc1404eadf38396deull},
    {0xa42463b01fdd3e95ull, 0x1fa8423e8c1443f2ull, 0xc80730b0001667f2ull},
    {0x38c6a548017167caull, 0x2ac903a413e5a847ull, 0xceaecfea80859b33ull},
    {0xbedec4594babbdabull, 0xf1be4359106a19b5ull, 0xd53847ac00a69be6ull},
    {0x05d84c6e2eadff07ull, 0x5b8a19b1c637671full, 0xdba4a47aa996d25aull},
    {0x712f104646b61eb0ull, 0x4c5a724dbd8180f8ull, 0xe1f4e5170d02a99bull},
    {0x8aa53e9c8128657aull, 0xc4baee073d4b1b04ull, 0xe829fb693044b398ull},
    {0xa7bd10d27c064978ull, 0x39d5d6a218c6339full, 0xee44cd59ffab62f3ull},
    {0x06c1f1d26c710872ull, 0x0d1e3f80fbc71454ull, 0xf446359b13539551ull},
    {0x82ae728f4624c84bull, 0x6adf27b820fd03e9ull, 0xfa2f045e7832aa72ull}
};

static const uint64_t lg2_R32[32][2] = {
    {0x0000000000000000ull, 0x0800000000000000ull},
    {0x7c1f07c1f07c1f07ull, 0x07c1f07c1f07c1f0ull},
    {0x8787878787878787ull, 0x0787878787878787ull},
    {0x7507507507507507ull, 0x0750750750750750ull},
    {0x71c71c71c71c71c7ull, 0x071c71c71c71c71cull},
    {0x5306eb3e45306eb3ull, 0x06eb3e45306eb3e4ull},
    {0xf286bca1af286bcaull, 0x06bca1af286bca1aull},
    {0x6906906906906906ull, 0x0690690690690690ull},
    {0x6666666666666666ull, 0x0666666666666666ull},
    {0x63e7063e7063e706ull, 0x063e7063e7063e70ull},
    {0x6186186186186186ull, 0x0618618618618618ull},
    {0xf417d05f417d05f4ull, 0x05f417d05f417d05ull},
    {0x5d1745d1745d1745ull, 0x05d1745d1745d174ull},
    {0x5b05b05b05b05b05ull, 0x05b05b05b05b05b0ull},
    {0x21642c8590b21642ull, 0x0590b21642c8590bull},
    {0x882b9310572620aeull, 0x0572620ae4c415c9ull},
    {0x5555555555555555ull, 0x0555555555555555ull},
    {0x0a72f05397829cbcull, 0x05397829cbc14e5eull},
    {0x51eb851eb851eb85ull, 0x051eb851eb851eb8ull},
    {0x0505050505050505ull, 0x0505050505050505ull},
    {0x4ec4ec4ec4ec4ec4ull, 0x04ec4ec4ec4ec4ecull},
    {0x4873ecade304d487ull, 0x04d4873ecade304dull},
    {0xf684bda12f684bdaull, 0x04bda12f684bda12ull},
    {0x4a7904a7904a7904ull, 0x04a7904a7904a790ull},
    {0x4924924924924924ull, 0x0492492492492492ull},
    {0xf7047dc11f7047dcull, 0x047dc11f7047dc11ull},
    {0x69ee58469ee58469ull, 0x0469ee58469ee584ull},
    {0x15b1e5f75270d045ull, 0x0456c797dd49c341ull},
    {0x4444444444444444ull, 0x0444444444444444ull},
    {0x4325c53ef368eb04ull, 0x04325c53ef368eb0ull},
    {0x4210842108421084ull, 0x0421084210842108ull},
    {0x4104104104104104ull, 0x0410410410410410ull}
};

static const uint64_t xp2_T16[16][2] = {
    {0x0000000000000000ull, 0x0000000000000000ull},
    {0x8b92b71842a98364ull, 0x0b5586cf9890f629ull},
    {0xf7c8c50eb14a7920ull, 0x172b83c7d517adcdull},
    {0x1fadb1c15cb593b0ull, 0x2387a6e75623866cull},
    {0x8d5a46305c85ededull, 0x306fe0a31b7152deull},
    {0x41223e13d773fba3ull, 0x3dea64c12342235bull},
    {0x397afec42e20e036ull, 0x4bfdad5362a271d4ull},
    {0x93015191eb345d89ull, 0x5ab07dd48542958cull},
    {0xb2fb1366ea957d3eull, 0x6a09e667f3bcc908ull},
    {0x51023f6cda1f5ef4ull, 0x7a11473eb0186d7dull},
    {0x7c55a192c9bb3e6full, 0x8ace5422aa0db5baull},
    {0xc46b071f2be58ddbull, 0x9c49182a3f0901c7ull},
    {0x734d1773205a7fbcull, 0xae89f995ad3ad5e8ull},
    {0x0cb12a091ba66794ull, 0xc199bdd85529c222ull},
    {0xa05aeb66e0dca9f6ull, 0xd5818dcfba48725dull},
    {0xf73a18f5db301f87ull, 0xea4afa2a490d9858ull}
};

static const uint64_t xp2_T8[8][3] = {
    {0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull},
    {0x03ec04c360be2404ull, 0x4a66ae336dcdfa40ull, 0x0163da9fb33356d8ull},
    {0x3d70a2cabc5cb89cull, 0xf7caca4f7a29bde9ull, 0x02c9a3e778060ee6ull},
    {0x40bb4bfc05af6456ull, 0x38f9a20da47e6ed0ull, 0x04315e86e7f84bd7ull},
    {0x53e3495f7df4baf8ull, 0x7c548eb68ca417feull, 0x059b0d31585743aeull},
    {0xcb8b092ac75e3117ull, 0xc403a9d87b27ed07ull, 0x0706b29ddf6ddc6dull},
    {0x1b2d6829d8993a0dull, 0x35f25d9427fa2b04ull, 0x0874518759bc808cull},
    {0x2e023da730e7fccbull, 0x1e060c584d6b74baull, 0x09e3ecac6f383452ull}
};

static const uint64_t lg2_c27[27][2] = {
    {0xbe87fed0691d3e88ull, 0xb8aa3b295c17f0bbull},
    {0xdf43ff68348e9f44ull, 0x5c551d94ae0bf85dull},
    {0x3f82aa45785f14d8ull, 0x3d8e13b87407fae9ull},
    {0xefa1ffb41a474fa2ull, 0x2e2a8eca5705fc2eull},
    {0x594e6629ae9f72e8ull, 0x24eed8a1df37fcf2ull},
    {0x9fc15522bc2f8a6cull, 0x1ec709dc3a03fd74ull},
    {0x645c921dc5df9b38ull, 0x1a61762a7aded93full},
    {0x77d0ffda0d23a7d1ull, 0x171547652b82fe17ull},
    {0x6a80e36c7d7506f2ull, 0x1484b13d7c02a8f8ull},
    {0x2ca73314d74fb974ull, 0x12776c50ef9bfe79ull},
    {0x5723a2cd20d41cf5ull, 0x10c9a84994022d28ull},
    {0x4fe0aa915e17c536ull, 0x0f6384ee1d01febaull},
    {0x711e274b1bc72c31ull, 0x0e347ab4698bb00eull},
    {0xb22e490ee2efcd9cull, 0x0d30bb153d6f6c9full},
    {0x731a220de4dfd0f8ull, 0x0c4f9d8b4a67fefbull},
    {0xbbe87fed0691d3e8ull, 0x0b8aa3b295c17f0bull},
    {0xa1cbc3b1e810c771ull, 0x0adcd64dba1f86a1ull},
    {0x354071b63eba8379ull, 0x0a42589ebe01547cull},
    {0x4d65793363d91e3dull, 0x09b81e0fa687ff32ull},
    {0x9653998a6ba7dcbaull, 0x093bb62877cdff3cull},
    {0x76c98609ec9fde68ull, 0x08cb27637e4a486aull},
    {0x2b91d166906a0e7aull, 0x0864d424ca011694ull},
    {0x77969bc6475a50a1ull, 0x080766bf04010a77ull},
    {0x27f05548af0be29bull, 0x07b1c2770e80ff5dull},
    {0xab76146ebc864a2eull, 0x0762f8205fd7ff63ull},
    {0x388f13a58de39618ull, 0x071a3d5a34c5d807ull},
    {0xce2af6797f270250ull, 0x06d6e5bf29563852ull}
};

static const uint64_t xp2_c[13][2] = {
    {0xc9e3b39803f2f6afull, 0xb17217f7d1cf79abull},
    {0xde2d60dd92e6bf95ull, 0x3d7f7bff058b1d50ull},
    {0x99d3b15d995e96f7ull, 0x0e35846b82505fc5ull},
    {0x39977c16a7dd58a0ull, 0x0276556df749cee5ull},
    {0x41c5fda69452fb0cull, 0x005761ff9e299cc4ull},
    {0xb7a58544c3591a0full, 0x000a184897c363c3ull},
    {0x34358a8e643ec734ull, 0x0000ffe5fe2c4586ull},
    {0x23fd8ffe606da77cull, 0x0000162c0223a5c8ull},
    {0x7c3da4a70e5a4ff9ull, 0x000001b5253d395eull},
    {0x8ec9f6fda1d952e7ull, 0x0000001e4cf5158bull},
    {0x1bb24c0f57995e47ull, 0x00000001e8cac735ull},
    {0xfc2985e2b5687e17ull, 0x000000001c3bd650ull},
    {0x166d0f96281ac300ull, 0x0000000001816193ull}
};


// hi128(a*b)：128bit 定点积的最高 128bit
static inline __uint128_t umul128_tohi128(__uint128_t a, __uint128_t b) {
    uint64_t a0 = (uint64_t)a, a1 = (uint64_t)(a >> 64);
    uint64_t b0 = (uint64_t)b, b1 = (uint64_t)(b >> 64);
    __uint128_t p01 = (__uint128_t)a0 * b1;
    __uint128_t p10 = (__uint128_t)a1 * b0;
    __uint128_t p11 = (__uint128_t)a1 * b1;
    __uint128_t w1 = (__uint128_t)(uint64_t)(((__uint128_t)a0 * b0) >> 64) + (uint64_t)p01 + (uint64_t)p10;
    __uint128_t w2 = (p01 >> 64) + (p10 >> 64) + (uint64_t)p11 + (uint64_t)(w1 >> 64);
    return ((__uint128_t)((uint64_t)(p11 >> 64) + (uint64_t)(w2 >> 64)) << 64) | (uint64_t)w2;
}

// r = x + y（128 位；调用点均为 r==x 就地模式，r==y 会丢失进位）
#define add_u128(r, x, y)                                     \
    do {                                                      \
        (r)[0] = (x)[0] + (y)[0];                             \
        (r)[1] = (x)[1] + (y)[1] + ((r)[0] < (y)[0] ? 1 : 0); \
    } while (0)

// r = x - y（128 位；同上）
#define sub_u128(r, x, y)               \
    do {                                \
        uint64_t _b_ = (x)[0] < (y)[0]; \
        (r)[0] = (x)[0] - (y)[0];       \
        (r)[1] = (x)[1] - (y)[1] - _b_; \
    } while (0)

// i += j（192 位；同上）
#define add_u192(i, j)                            \
    do {                                          \
        (i)[0] += (j)[0];                         \
        uint64_t _c_ = ((i)[0] < (j)[0]) ? 1 : 0; \
        (i)[1] += _c_;                            \
        _c_ = ((i)[1] < _c_) ? 1 : 0;             \
        (i)[1] += (j)[1];                         \
        _c_ += ((i)[1] < (j)[1]) ? 1 : 0;         \
        (i)[2] += _c_ + (j)[2];                   \
    } while (0)

// i -= j（192 位；同上）
#define sub_u192(i, j)                             \
    do {                                           \
        uint64_t _b_ = ((i)[0] < (j)[0]) ? 1 : 0;  \
        (i)[0] -= (j)[0];                          \
        uint64_t _b1_ = ((i)[1] < (j)[1]) ? 1 : 0; \
        (i)[1] -= (j)[1];                          \
        _b1_ += ((i)[1] < _b_) ? 1 : 0;            \
        (i)[1] -= _b_;                             \
        (i)[2] = (i)[2] - ((j)[2] + _b1_);         \
    } while (0)


uint64_t log2_fixed_64(uint64_t x) {
    unsigned i = (unsigned)(x >> 59);
    uint64_t xl = x & 0x07FFFFFFFFFFFFFFull;

    /*
        v = u*2^128 = xl*2^69/(32+i)，u < 2^-5。乘子 iv = floor(2^69/(32+i))
        由 128bit 倒数表右移导出；i=0 时精确值 2^64 超出单 limb，钳位为
        2^64-1（v 误差 < xl < 2^59，相对 < 2^-64，可忽略）。xl*iv <
        2^59*2^64 = 2^123，128bit 全积恰为 v 本身（尺度已对齐）。
    */
    uint64_t v[2];
    uint64_t iv = i ? ((lg2_R32[i][1] << 5) | (lg2_R32[i][0] >> 59)) : UINT64_MAX;
    _umul64to128_(xl, iv, &v[0], &v[1]);

    // w = u^2*2^64（链乘子），wf = u^2*2^128（收尾乘子）
    uint64_t rr[4];
    _usqr128to256_(v[1], v[0], rr);
    uint64_t w = rr[3];

    /*
        64bit 输出下 B 误差预算 ~2^-59（经 u 因子放大前），w 每层衰减
        2^-10，整条链单 limb（2^63 半尺度）即可：链值取 lg2_c27 顶
        limb。B 为奇次项（m=1..13），A 为偶次项（m=2..14），首个截断
        项 m=15 < 2^-75 < 2^-66 预算。
    */
    uint64_t B = lg2_c27[12][1];
    for (int k = 10; k >= 0; k -= 2) B = _umul64to64hi_(B, w) + lg2_c27[k][1];
    uint64_t A = lg2_c27[13][1];
    for (int k = 11; k >= 1; k -= 2) A = _umul64to64hi_(A, w) + lg2_c27[k][1];

    // t1 = (v*B)>>64、t2 = (wf*A)>>64，均取 2^127 半尺度
    uint64_t p[2], q[2], t1[2], t2[2];
    _umul64to128_(v[1], B, &p[0], &p[1]);
    _umul64to128_(v[0], B, &q[0], &q[1]);
    t1[0] = q[1] + p[0];
    t1[1] = p[1] + (t1[0] < q[1]);
    _umul64to128_(rr[3], A, &p[0], &p[1]);
    _umul64to128_(rr[2], A, &q[0], &q[1]);
    t2[0] = q[1] + p[0];
    t2[1] = p[1] + (t2[0] < q[1]);

    // result = (L[i] + 2*(t1-t2))>>64，溢出 2^128 时钳位；L 复用 L32 高 2 limb
    sub_u128(t1, t1, t2);
    t1[0] = (t1[0] << 1) | (t1[1] >> 63);
    t1[1] <<= 1;
    add_u128(t1, t1, lg2_L32[i] + 1);
    if (t1[1] < lg2_L32[i][2]) return UINT64_MAX;
    return t1[1];
}

uint64_t exp2_fixed_64(uint64_t x) {
    unsigned i = (unsigned)(x >> 57);
    uint64_t t = x & 0x01FFFFFFFFFFFFFFull;

    // w = t^2*2^64，z = t^4*2^64
    uint64_t wf[2];
    _umul64to128_(t, t, &wf[0], &wf[1]);
    uint64_t w = wf[1];
    uint64_t z = _umul64to64hi_(w, w);

    // B = (b0+w*b1) + z*(b2+w*b3)，A = a0+w*(a1+w*a2)，单 limb 满尺度链
    uint64_t p1 = _umul64to64hi_(xp2_c[2][1], w) + xp2_c[0][1];
    uint64_t p2 = _umul64to64hi_(xp2_c[6][1], w) + xp2_c[4][1];
    uint64_t B = _umul64to64hi_(p2, z) + p1;
    uint64_t A = _umul64to64hi_(xp2_c[5][1], w) + xp2_c[3][1];
    A = _umul64to64hi_(A, w) + xp2_c[1][1];

    // S = s*2^128 = t*B + w*A，sh = round(s*2^64)
    uint64_t s0[2], s1[2];
    _umul64to128_(t, B, &s0[0], &s0[1]);
    _umul64to128_(w, A, &s1[0], &s1[1]);
    uint64_t sl = s0[0] + s1[0];
    uint64_t sh = s0[1] + s1[1] + (sl < s0[0]) + (sl >> 63);

    /*
        q = (2^(i/128)-1)*2^64：两级表 128bit 全量合成（丢弃表低位分数
        会被 T16 的大因子放大 ~1ulp，须完整求和），舍入取高 64bit。
        mm 同样舍入化，全链误差 |e| <= 2。
    */
    unsigned k = i >> 3, m = i & 7;
    uint64_t t8h = xp2_T8[m][1] + (xp2_T8[m][0] >> 63);
    __uint128_t t8t = ((__uint128_t)(xp2_T8[m][2] + (t8h < xp2_T8[m][1])) << 64) | t8h;
    __uint128_t qq = umul128_tohi128(((__uint128_t)xp2_T16[k][1] << 64) | xp2_T16[k][0], t8t);
    uint64_t q0 = xp2_T16[k][0] + (uint64_t)qq;
    uint64_t cy = (q0 < (uint64_t)qq);
    q0 += (uint64_t)t8t;
    cy += (q0 < (uint64_t)t8t);
    uint64_t q = xp2_T16[k][1] + (uint64_t)(t8t >> 64) + (uint64_t)(qq >> 64) + cy + (q0 >> 63);
    uint64_t ml, mh;
    _umul64to128_(q, sh, &ml, &mh);
    uint64_t r = q + sh + mh + (ml >> 63);
    if (r < sh) return UINT64_MAX;
    return r;
}

void log2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low) {
    unsigned i = (unsigned)(high >> 59);
    uint64_t h2 = high & 0x07FFFFFFFFFFFFFFull;

    // xls = xl*2^5（128bit 对齐），v = u*2^128 = hi128(xls*R32[i])
    uint64_t xls[2] = {low << 5, (h2 << 5) | (low >> 59)};
    uint64_t rr[4];
    _umul128to256_(xls[1], xls[0], lg2_R32[i][1], lg2_R32[i][0], rr);
    uint64_t v[2] = {rr[2], rr[3]};

    // wf = u^2*2^128（链乘子）
    _usqr128to256_(v[1], v[0], rr);
    __uint128_t wf = ((__uint128_t)rr[3] << 64) | rr[2];

    /*
        交错 Horner：B 奇链 c25 -> c1（lg2_c27[24] -> [0]），A 偶链
        c26 -> c2（[25] -> [1]），单循环双累加器维持 ILP=2。128bit 输出
        下 B 误差预算 ~2^-122，链取双 limb（2^127 半尺度）；首个截断项
        m=27 < 2^-135 < 2^-130 预算。链值以 __uint128_t 局部量保活，
        避免数组形态在循环内产生栈回读写。
    */
    __uint128_t B = ((__uint128_t)lg2_c27[24][1] << 64) | lg2_c27[24][0];
    __uint128_t A = ((__uint128_t)lg2_c27[25][1] << 64) | lg2_c27[25][0];
    for (int k = 22; k >= 1; k -= 2) {
        B = umul128_tohi128(B, wf) + (((__uint128_t)lg2_c27[k][1] << 64) | lg2_c27[k][0]);
        A = umul128_tohi128(A, wf) + (((__uint128_t)lg2_c27[k + 1][1] << 64) | lg2_c27[k + 1][0]);
    }
    B = umul128_tohi128(B, wf) + (((__uint128_t)lg2_c27[0][1] << 64) | lg2_c27[0][0]);
    A = umul128_tohi128(A, wf) + (((__uint128_t)lg2_c27[1][1] << 64) | lg2_c27[1][0]);

    // P = 2*(u*B - wf*A)，t1/t2 取 256bit 积的高 192bit（2^191 半尺度）
    uint64_t t1[3], t2[3];
    _umul128to256_(v[1], v[0], (uint64_t)(B >> 64), (uint64_t)B, rr);
    t1[0] = rr[1];
    t1[1] = rr[2];
    t1[2] = rr[3];
    _umul128to256_((uint64_t)(wf >> 64), (uint64_t)wf, (uint64_t)(A >> 64), (uint64_t)A, rr);
    t2[0] = rr[1];
    t2[1] = rr[2];
    t2[2] = rr[3];
    sub_u192(t1, t2);
    t1[2] = (t1[2] << 1) | (t1[1] >> 63);
    t1[1] = (t1[1] << 1) | (t1[0] >> 63);
    t1[0] <<= 1;

    // result192 = L32[i] + P，加 2^63 使 >>64 截断舍入化，输出高 128bit
    add_u192(t1, lg2_L32[i]);
    if (t1[2] < lg2_L32[i][2]) {
        dst[0] = dst[1] = UINT64_MAX;
        return;
    }
    t1[0] += 0x8000000000000000ull;
    if (t1[0] < 0x8000000000000000ull) {
        if (++t1[1] == 0 && ++t1[2] == 0) {
            dst[0] = dst[1] = UINT64_MAX;
            return;
        }
    }
    dst[0] = t1[1];
    dst[1] = t1[2];
}

void exp2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low) {
    unsigned i = (unsigned)(high >> 57);
    uint64_t W[2] = {low, high & 0x01FFFFFFFFFFFFFFull};  // W = t*2^128（精确）

    // wf = t^2*2^128（链乘子）
    uint64_t rr[4], zz[4];
    _usqr128to256_(W[1], W[0], rr);
    __uint128_t wfe = ((__uint128_t)rr[3] << 64) | rr[2];

    /*
        交错 Horner：B 奇链 c13 -> c1（xp2_c[12] -> [0]），A 偶链
        c12 -> c2（[11] -> [1]），链双 limb（2^128 满尺度，xp2 系数
        均 < 1）；首个截断项 m=15 < 2^-133 < 2^-130 预算。
    */
    __uint128_t B = ((__uint128_t)xp2_c[12][1] << 64) | xp2_c[12][0];
    __uint128_t A = ((__uint128_t)xp2_c[11][1] << 64) | xp2_c[11][0];
    // 常量小 trip 循环在 -O3 下会被完全展开（代码膨胀 + spill 回潮），
    // 压回滚动形态
    #pragma GCC unroll 1
    for (int k = 10; k >= 1; k -= 2) {
        B = umul128_tohi128(B, wfe) + (((__uint128_t)xp2_c[k][1] << 64) | xp2_c[k][0]);
        A = umul128_tohi128(A, wfe) + (((__uint128_t)xp2_c[k - 1][1] << 64) | xp2_c[k - 1][0]);
    }
    B = umul128_tohi128(B, wfe) + (((__uint128_t)xp2_c[0][1] << 64) | xp2_c[0][0]);

    // S = s*2^192 = t*B + w*A（取 256bit 积高 192bit），sh = s*2^128
    uint64_t S[3], u2[3];
    _umul128to256_(W[1], W[0], (uint64_t)(B >> 64), (uint64_t)B, rr);
    S[0] = rr[1];
    S[1] = rr[2];
    S[2] = rr[3];
    _umul128to256_((uint64_t)(wfe >> 64), (uint64_t)wfe, (uint64_t)(A >> 64), (uint64_t)A, zz);
    u2[0] = zz[1];
    u2[1] = zz[2];
    u2[2] = zz[3];
    add_u192(S, u2);
    uint64_t sh[2];
    sh[0] = S[1] + (S[0] >> 63);
    sh[1] = S[2] + (sh[0] < S[1]);

    // q = (2^(i/128)-1)*2^128 = T16 + T8高2limb(舍入) + hi128(T16*T8)
    unsigned k = i >> 3, m = i & 7;
    uint64_t t8h = xp2_T8[m][1] + (xp2_T8[m][0] >> 63);
    uint64_t t8t[2] = {t8h, xp2_T8[m][2] + (t8h < xp2_T8[m][1])};
    uint64_t q[2], m2[2];
    _umul128to256_(xp2_T16[k][1], xp2_T16[k][0], t8t[1], t8t[0], rr);
    m2[0] = rr[2] + (rr[1] >> 63);
    m2[1] = rr[3] + (m2[0] < rr[2]);
    q[0] = xp2_T16[k][0];
    q[1] = xp2_T16[k][1];
    add_u128(q, q, t8t);
    add_u128(q, q, m2);

    /*
        result = q + sh + round128(q*sh)：q/sh 的构造与 m 的 hi128 均已
        舍入化，加 1 中心化后全链 |e| <= 2。
    */
    _umul128to256_(q[1], q[0], sh[1], sh[0], rr);
    m2[0] = rr[2] + (rr[1] >> 63);
    m2[1] = rr[3] + (m2[0] < rr[2]);
    uint64_t r0 = q[0], r1 = q[1], cy;
    r0 += sh[0], cy = (r0 < sh[0]);
    r1 += cy, cy = (r1 < cy);
    r1 += sh[1], cy += (r1 < sh[1]);
    if (cy) {
        dst[0] = dst[1] = UINT64_MAX;
        return;
    }
    r0 += m2[0], cy = (r0 < m2[0]);
    r1 += cy, cy = (r1 < cy);
    r1 += m2[1], cy += (r1 < m2[1]);
    dst[0] = r0;
    dst[1] = r1;
}
