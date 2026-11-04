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

/* 128 位档（is_prime_2_）验证：写死常数表（Mersenne/SWbound/ψ₁₂ 邻域/
   Carmichael 对抗语料/ctz 回归向量 ~200 条）+ Proth 运行时证书
   （a^((n-1)/2) ≡ -1 经 lmmp_powmod_ 独立梯子，普罗斯定理：有证书 ⟺
   素数）+ 素数乘积（SW 界下确定性拒绝）+ 平方数（Lucas 防御路径）。 */

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_prime_ref.hpp"

using namespace lmmp_test_utils;
using namespace lmmp_test_prime_ref;

namespace {

static const struct { u64 lo, hi; bool expect; } vec128[] = {
    // n = +-1 (mod 2^65) 的素数（高低 limb ctz 组合的回归向量）
    {0x0000000000000001ull, 0x0000000000000012ull, true},
    {0x0000000000000001ull, 0x000000000000007eull, true},
    {0xffffffffffffffffull, 0x0000000000000085ull, true},
    {0x0000000000000001ull, 0x000000000000008eull, true},
    {0x950e87d7f5606615ull, 0x0000000000000001ull, false},
    {0x042db9232c61275dull, 0x0000000000000001ull, false},
    {0x6dbca290a9eab707ull, 0x0000000000000001ull, false},
    {0xc4fd394d4c10a4ffull, 0x0000000000000001ull, false},
    {0x6814a2bc786a6d2dull, 0x0000000000000001ull, false},
    {0xbc051c6ca26b351full, 0x0000000000000001ull, false},
    {0xd4c08880a5a4666dull, 0x0000000000000001ull, false},
    {0xfe5213e529610ae1ull, 0x0000000000000001ull, false},
    {0x6c50afb6e9fb123dull, 0x0000000000000001ull, false},
    {0xebac94af6f28d015ull, 0x0000000000000001ull, false},
    {0x194f9545adba52cfull, 0x0000000000000001ull, true},
    {0x1d4b7ef2c675ce05ull, 0x0000000000000001ull, false},
    {0xd998efd82733e933ull, 0x000000000000002full, false},
    {0xcb57d5d86df216c3ull, 0x0000000000000024ull, false},
    {0x8860a84722025e05ull, 0x000000000000002aull, false},
    {0xc5b864d733176469ull, 0x0000000000000038ull, false},
    {0x7a2f11088d29b147ull, 0x000000000000003bull, true},
    {0x2fcb9940da10faabull, 0x000000000000002bull, false},
    {0xb98937dfef041067ull, 0x0000000000000034ull, false},
    {0x4a2e3224dd4b712full, 0x0000000000000031ull, false},
    {0x07fdc889fa017ed7ull, 0x0000000000000024ull, false},
    {0x425a7de181eeadd7ull, 0x000000000000002eull, false},
    {0xaaabc8d366e0440dull, 0x0000000000000031ull, false},
    {0x1ac44b703371364full, 0x0000000000000031ull, false},
    {0x016590c55646e6d1ull, 0x0000000000002079ull, false},
    {0xf16e981a0b7a6e1dull, 0x0000000000003968ull, false},
    {0x1167fba4a2927979ull, 0x00000000000026d4ull, false},
    {0x5532c8673d01ac0full, 0x000000000000349eull, false},
    {0xee26cbc0358b24d3ull, 0x000000000000328full, true},
    {0x1a7415559bdb39b3ull, 0x0000000000002378ull, false},
    {0xd6257b492186c8b5ull, 0x00000000000034e5ull, false},
    {0x1ec1b0b1dee7539dull, 0x00000000000030c1ull, false},
    {0x1d790bcaeffd4d2dull, 0x00000000000030f3ull, false},
    {0x3537a844de18f50bull, 0x00000000000034dbull, false},
    {0x64b5e3f81a293b3bull, 0x0000000000003d91ull, false},
    {0xb047719de8eef3d7ull, 0x0000000000002a23ull, false},
    {0xf177d49f03ddc3bfull, 0x000000000001a52cull, false},
    {0x7048dacaa745fdd5ull, 0x000000000001a16dull, false},
    {0xfce79398852e0401ull, 0x000000000001c641ull, false},
    {0x80271e94760c9b75ull, 0x0000000000019ca5ull, false},
    {0x293f65848aa18f43ull, 0x000000000001889dull, false},
    {0xb0baf029520e015full, 0x000000000001f27full, false},
    {0x7ad955568f86a26bull, 0x000000000001d90cull, false},
    {0xd487d3421c720603ull, 0x000000000001a11cull, false},
    {0x310288290b43dbfbull, 0x0000000000011cb3ull, false},
    {0x6dbbac73d50ca99full, 0x000000000001d849ull, false},
    {0xb7a13dce8e4595dfull, 0x000000000001e023ull, false},
    {0xed9a76b9e91b8ec1ull, 0x0000000000012527ull, false},
    {0x75c33f8fcb8031ffull, 0x0000000000021662ull, false},
    {0xddfc20fe1e7c31d3ull, 0x00000000000355d3ull, false},
    {0xd17dad339930e76full, 0x000000000002fe19ull, false},
    {0x0deef007acfbba2bull, 0x000000000002938cull, false},
    {0x8fcd110ce94f47b1ull, 0x000000000002575dull, false},
    {0x227d512de1660a41ull, 0x0000000000035b64ull, true},
    {0x2abb018969cbe6ebull, 0x000000000003aa48ull, false},
    {0x93a8b5d809cea2a9ull, 0x000000000002ffa7ull, false},
    {0x602f8e87d16bc8bfull, 0x0000000000035cc1ull, false},
    {0x61ef7dfce376bd79ull, 0x000000000003d21eull, false},
    {0xff5e243c496a590bull, 0x000000000002f5c7ull, false},
    {0x1d2e1a2e089934a9ull, 0x000000000002f7abull, false},
    {0xe443e6031233f1e1ull, 0x00000000000b4a20ull, false},
    {0x3ede6f125ab59d11ull, 0x00000000000e5814ull, false},
    {0xf5d46d8127762b7bull, 0x000000000008b87cull, false},
    {0x60083c7dad1dd141ull, 0x00000000000f9afaull, false},
    {0xb7a68aa8611b9b59ull, 0x00000000000a86fcull, false},
    {0x7893032bd828056full, 0x000000000009c0aeull, false},
    {0x34c8a05ca34be96bull, 0x00000000000da10eull, false},
    {0x921082dfc966aed7ull, 0x00000000000eb7e2ull, false},
    {0x6e5d9a3007c331a3ull, 0x00000000000d4f57ull, false},
    {0xf7767fd63a0806a7ull, 0x000000000008a07aull, false},
    {0xf0723a8383f43dc5ull, 0x0000000000082414ull, false},
    {0x106025b5fb65e625ull, 0x00000000000d0451ull, false},
    {0xa0d72f15feb859ebull, 0x0000000002fa9bedull, false},
    {0x3b97b6c911560053ull, 0x00000000026f8eb8ull, false},
    {0x5fe2b11364b97757ull, 0x00000000025f7a97ull, false},
    {0xbf1317f85a8a9441ull, 0x00000000030cc190ull, false},
    {0xf0b02956ff594f79ull, 0x0000000002ac6c79ull, false},
    {0x2912ab9fa4002d91ull, 0x0000000002e8d474ull, false},
    {0x56761e8879073c59ull, 0x00000000028dcf80ull, false},
    {0xd0efd4ff3912a0fdull, 0x0000000003b0012bull, false},
    {0x8919551203d33d87ull, 0x000000000269137eull, false},
    {0xefb4cad164f85da9ull, 0x00000000028741a1ull, false},
    {0x1732b75d08d75497ull, 0x0000000003189469ull, false},
    {0xec5093da27623245ull, 0x000000000261eaedull, false},
    {0xea45cdaf628e21c9ull, 0x00000000cd8a9084ull, false},
    {0xc231185b0272834full, 0x00000000ab699ad2ull, false},
    {0x6ff327f4119ee915ull, 0x0000000098ca4c3full, false},
    {0x1d5d73026b06b341ull, 0x00000000f2546119ull, false},
    {0x511173b251af8015ull, 0x00000000c3846eceull, false},
    {0x1d74a080ebbfbb2bull, 0x00000000ed8b79edull, false},
    {0x9736b29f0b03d0e1ull, 0x00000000ade3540cull, false},
    {0xecbeb26fceaf0df5ull, 0x00000000d76c473aull, false},
    {0x6782e42f80a0f27dull, 0x00000000acafb91cull, false},
    {0x25e74da2f39f015full, 0x00000000a93c7ae4ull, false},
    {0x1a18b9b1c2c8b503ull, 0x00000000b7b2a53bull, false},
    {0xc0f76e59731535edull, 0x00000000cf7d9b08ull, false},
    {0x3e115e3e75118be1ull, 0x0000000edd801db4ull, false},
    {0x399246294d8fc043ull, 0x0000000cff8f5cffull, false},
    {0xa364f1b057f4865full, 0x0000000bf9f2dce0ull, false},
    {0x1fb37062a68f65c1ull, 0x0000000aca707a92ull, false},
    {0x3ff1295c1d296c15ull, 0x0000000c1455fcadull, false},
    {0xb484b8d3f354db29ull, 0x0000000d7a2ee034ull, false},
    {0x1a46b9e3a2663f03ull, 0x0000000d177d70d6ull, false},
    {0x36a208e01b1b4ee3ull, 0x00000008e33a0336ull, false},
    {0x9d3bd30e22749e55ull, 0x0000000f65265fe5ull, false},
    {0xebe4418c6286ef71ull, 0x0000000e27fcb0f2ull, false},
    {0xcfedc87950868c9dull, 0x00000009784ecbbbull, false},
    {0x106572c92038d12full, 0x0000000f2176822eull, false},
    {0x810287a90cffae31ull, 0x00003f23b03c1008ull, false},
    {0x113167635255aa79ull, 0x000027c16aab79e5ull, false},
    {0x559ccfb8c80ce421ull, 0x00002cff263695f9ull, false},
    {0xc2299345df0b305dull, 0x00002d46dac97abbull, false},
    {0xed1137eb5e5e1047ull, 0x000028ad8e8e8733ull, false},
    {0xe3bd76bf57cec991ull, 0x000030083e2681d1ull, false},
    {0xeee4852d330c2395ull, 0x000021512f3334bfull, false},
    {0xcfe680854c95ea73ull, 0x000038eac209f79dull, false},
    {0xa2842cb2fb44c6a3ull, 0x00002c91a0f4dd5aull, false},
    {0xe5fbc6d02bd667d7ull, 0x00002d0f621d1238ull, false},
    {0x6cb5b7d2721a1891ull, 0x00003bc198411febull, false},
    {0x31afaa01fdc2dbd7ull, 0x000035d827aa949bull, false},
    {0x332508b2d1c97795ull, 0x00b93ad7cba7ddcdull, false},
    {0x4930986a215c9b8bull, 0x00bcaf643fe36a17ull, false},
    {0x4e1309a0fc447a7full, 0x00819d6afe7f773eull, false},
    {0x637118bb0b0e773dull, 0x00ba17e70a7a8b0cull, false},
    {0x20b9122fca694c79ull, 0x00b0773e8ea50117ull, false},
    {0xa544b6d2cf823377ull, 0x00be2e211529057cull, false},
    {0x01d6aedaa22e88e9ull, 0x00e73bb93bc7eeadull, false},
    {0xf332dec5058c062bull, 0x00802df2f9537531ull, false},
    {0x26dd7c451562a837ull, 0x008c72e5f03cde37ull, false},
    {0xeae27c2bcf28335bull, 0x009482fa03ac665dull, false},
    {0x6774a90031d2ba09ull, 0x00e6b37c3fbd6d30ull, false},
    {0xc958935b157304b1ull, 0x009ef804a8e636c6ull, false},
    {0xa7d73426f0aee715ull, 0x32b01555bdca343full, false},
    {0x65c2195389de9f31ull, 0x3ed2bf30a8108c27ull, false},
    {0x938f35b2dc04bbfdull, 0x390b921100cdfa67ull, false},
    {0x890c62927989d7e7ull, 0x24742f054b47a18bull, false},
    {0xd0ae2b468f227e2full, 0x2e7d02750d3832c1ull, false},
    {0xa37579c44c86abf9ull, 0x331a7cd6eecff786ull, false},
    {0x3cd64d14ac521437ull, 0x2e1831695b4be237ull, false},
    {0x3d1791cf2b9550bdull, 0x3316d1c9a89a476eull, false},
    {0x12e2992b24380fb7ull, 0x23ee2059ccc14927ull, false},
    {0x9dca0bdcdd3a68c5ull, 0x3ac38dfdd6290f03ull, false},
    {0x0e8936d8133fee35ull, 0x2b9de39e671eaa35ull, false},
    {0x616eb2a9fb09b28dull, 0x2ab0308b5d235cabull, false},
    {0xad4cf62c94a4f317ull, 0x679daf749ca944bbull, false},
    {0xc1f007cd2413872bull, 0x4c7ef3d3091e9247ull, false},
    {0xe8ed59599a0e9c31ull, 0x581b5d6f9e716b3dull, false},
    {0x92852160c8b912b1ull, 0x6cd6cc248ff5b11bull, false},
    {0xd41339c948a6e7cbull, 0x5e3cd0009f140b4eull, false},
    {0x34186cdd3c3c5141ull, 0x48cdb5333343fd70ull, false},
    {0xbab5120ef942a0f7ull, 0x5e400b6806c1ec71ull, false},
    {0x28e208906796f59full, 0x7decf7dd76c9773aull, false},
    {0xf3d6da5c3df5a975ull, 0x51524f93fef4ca54ull, false},
    {0x2935e72533e01f5dull, 0x71b7e13936a1483cull, false},
    {0x91a6821dd6d9ac33ull, 0x799453bb6b70929bull, true},
    {0x1069a2114ba0d199ull, 0x58ddebc322e9dde7ull, false},
    {0xefb27ceed7b68265ull, 0xe57782da9744782bull, false},
    {0x31eed1a700b6ec29ull, 0x9a1e2d7b24252495ull, false},
    {0xaa1a3b8e0336d241ull, 0xbe96c928f239c6e8ull, false},
    {0xd977bc4f68576763ull, 0xf5c1e0efc83c98eeull, false},
    {0x4788ae323e2a32d9ull, 0xfa5286e53a968735ull, false},
    {0x10fb01d7cb2aaf85ull, 0xe18e124e9a1938b3ull, true},
    {0x31ee89b3b4eaa601ull, 0x94cc7b72fc3196cdull, false},
    {0x56ed6af7e42c893bull, 0xf6db24bb16f42afeull, false},
    {0xbf83c8b771312e2dull, 0xb8a2d999a2f70ca6ull, false},
    {0xcd2f2acc0bcd93bbull, 0xa4eaa48c2f25b957ull, false},
    {0x1765dee27d9f4de1ull, 0xb1546df37cf45bebull, false},
    {0x53f901828270094dull, 0x96ec812185e2bf7aull, false},
    {0xbdca6ba34ba3aec1ull, 0x0000000000000001ull, true},
    {0x3f48dad818d61ccfull, 0x0000000000000001ull, true},
    {0x5cf7913a8a83b817ull, 0x0000000000000001ull, true},
    {0x72e68302bf30028bull, 0x0000000000031e30ull, true},
    {0xe07055c2c6066ea9ull, 0x0000000000028754ull, true},
    {0xc331631cc701924bull, 0x000000000002327cull, true},
    {0x4b3ea92729448615ull, 0x00000000b86af1e9ull, true},
    {0xe6aa9fbe8d2ed5d5ull, 0x00000000b5db7aa4ull, true},
    {0xcc3727a535e6f183ull, 0x00000000d52d7c5cull, true},
    {0x9b5c5903c10b0041ull, 0x0000c903df44645full, true},
    {0x423fdaf1a4a83317ull, 0x00009f63b437842aull, true},
    {0x0240ea4123c4796bull, 0x0000f9ad7a912fe0ull, true},
    {0x181f3b48a4db2427ull, 0xc1387e39b26d94deull, true},
    {0x037b77e84bc2e4e5ull, 0xc36d6e1a55261cfdull, true},
    {0x8af483d92122f4f7ull, 0xfd2c71512c957f86ull, true},
    // SWbound 邻域（0x2be69_51adc5b22410a5fd 本体为合数）
    {0x51adc5b22410a5f5ull, 0x000000000002be69ull, false},
    {0x51adc5b22410a5f7ull, 0x000000000002be69ull, false},
    {0x51adc5b22410a5f9ull, 0x000000000002be69ull, false},
    {0x51adc5b22410a5fbull, 0x000000000002be69ull, false},
    {0x51adc5b22410a5fdull, 0x000000000002be69ull, false},
    {0x51adc5b22410a5ffull, 0x000000000002be69ull, false},
    {0x51adc5b22410a601ull, 0x000000000002be69ull, false},
    {0x51adc5b22410a603ull, 0x000000000002be69ull, false},
    {0x51adc5b22410a605ull, 0x000000000002be69ull, false},
};
}  // namespace

TEST_CASE("numth/prime", is_prime_2) {
    struct { u64 hi, lo; bool expect; const char* what; } cases[] = {
        {0x7fffffffffffffffull, 0xffffffffffffffffull, true, "2^127-1 Mersenne"},
        {0xffffffffffffffffull, 0xffffffffffffff61ull, true, "2^128-159 largest prime < 2^128"},
        {0x0000000000000001ull, 0x000000000000000dull, true, "2^64+13"},
        {0x0000000000000001ull, 0x0000000000000001ull, false, "2^64+1 Fermat"},
        {0xffffffffffffffffull, 0xffffffffffffffffull, false, "2^128-1 (3 divides)"},
        {0x000000000000437aull, 0xe92817f9fc85b7e5ull, false, "psi_12 SPSP(2..37)"},
        {0x000000000000437aull, 0xe92817f9fc85b7e4ull, false, "psi_12-1 even"},
        {0x000000000000437aull, 0xe92817f9fc85b7d1ull, true, "first prime < psi_12"},
        {0x000000000000437aull, 0xe92817f9fc85b7fbull, true, "first prime > psi_12"},
        {0x000000000002be69ull, 0x51adc5b22410a5fdull, false, "psi_13 = SWbound"},
        {0x000000000002be69ull, 0x51adc5b22410a68bull, true, "first prime > SWbound"},
        {0x0000000000000001ull, 0x0000000000000000ull, false, "2^64 even"},
        /* D 搜索上界回归：least |D| > 201 的真素数（CRT 构造 + 扫描验证，
           40 基底 MR 复核），旧版上界 201 会误判为合数（返回 0） */
        {0x800000035f5a7854ull, 0x024690fc2c1cab2dull, true, "prime with least|D|=211"},
        {0x80000023e5c609bfull, 0xa5c985498fc11b3dull, true, "prime with least|D|=227"},
        {0x8000000be28e524cull, 0x609ffaaa3f5d4dd5ull, true, "prime with least|D|=211"},
    };
    for (const auto& c : cases) {
        TEST_CHECK_MSG(lmmp_is_prime_2_(c.lo, c.hi) == expect2(c.lo, c.hi, c.expect), c.what);
    }

    /* 硬编码向量（含 65..128 bit 随机、素数、SWbound 邻域、psi_12/13） */
    for (const auto& v : vec128) {
        TEST_CHECK_MSG(lmmp_is_prime_2_(v.lo, v.hi) == expect2(v.lo, v.hi, v.expect), "vec128");
    }

    /* Proth 认证素数 65..127 bit（k*2^t+1）：被测命中即证书复核（硬断言），
       类别按 SW 分界；SW 两侧各一组（t 定、k 扫描——k=3 密度不足） */
    {
        int found[2] = {0, 0};
        const int groups[2][2] = {{66, 78}, {108, 126}};  // {t_lo, t_hi}：<SW 与 >SW
        mp_limb_t n[4];  // k2t_limbs 清零 t/64+3 个 limb（t<=127 需 4）
        for (int g = 0; g < 2; ++g) {
            for (int t = groups[g][0]; t <= groups[g][1] && found[g] < 2; ++t) {
                for (uint k = 3; k < 999 && found[g] < 2; k += 2) {
                    mp_size_t nn = k2t_limbs(n, t, k, 1);
                    if (nn > 2) break;  // N >= 2^128：超出 is_prime_2_ 契约域
                    int r = lmmp_is_prime_2_(n[0], n[1]);
                    if (r != 0) {
                        TEST_CHECK_MSG(proth_cert_g(k, t), "proth prime certified");
                        TEST_CHECK_MSG(r == (g == 0 ? 1 : 2), "proth prime category");
                        ++found[g];
                    }
                }
            }
            TEST_CHECK_MSG(found[g] >= 1, "proth prime coverage per SW side");
        }
    }

    /* 素数乘积：两 34 bit 素数积 ~68 bit < SWbound（确定性域）→ 硬断言 ==0 */
    {
        u64 seed = 0x70726f74683231ull;
        for (int i = 0; i < 20; ++i) {
            u64 p = 0x100000000ull + (xorshift64(seed) & 0xffffffffull);
            p |= 1;
            while (!ref_is_prime64(p)) p += 2;
            u64 q = 0x100000000ull + (xorshift64(seed) & 0xffffffffull);
            q |= 1;
            while (!ref_is_prime64(q)) q += 2;
            mp_limb_t prod[2];
            lmmp_mullh_(p, q, prod); /* ~68 bit < SW */
            TEST_CHECK_MSG(lmmp_is_prime_2_(prod[0], prod[1]) == 0, "semiprime < SW rejected");
        }
    }

    /* 平方数（Lucas 无 D 的防御路径不应误判） */
    {
        u64 p = 0x000001000000000full;  // >2^40 素数
        TEST_CHECK(ref_is_prime64(p));
        mp_limb_t sq[2];
        lmmp_mullh_(p, p, sq);
        TEST_CHECK_MSG(lmmp_is_prime_2_(sq[0], sq[1]) == 0, "square rejected");
        /* 3*p^2（mullh 一步 + 乘小系数；p^2*3 < 2^128 恒成立） */
        mp_limb_t sq3[3];
        u64 hi = lmmp_mul_1_(sq3, sq, 2, 3);
        TEST_CHECK_MSG(hi == 0, "3*square fits 2 limbs");
        TEST_CHECK_MSG(lmmp_is_prime_2_(sq3[0], sq3[1]) == 0, "3*square rejected");
    }
}
