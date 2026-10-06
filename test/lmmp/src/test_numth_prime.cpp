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

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstdint>
#include <cstring>

using namespace lmmp_test_utils;

namespace {

u64 mod_pow64(u64 a, u64 e, u64 mod) {
    u64 r = 1 % mod;
    a %= mod;
    while (e) {
        if (e & 1) r = (u64)((u128)r * a % mod);
        a = (u64)((u128)a * a % mod);
        e >>= 1;
    }
    return r;
}

bool ref_is_prime64(u64 n) {
    if (n < 2) return false;
    for (u64 p : {2ull, 3ull, 5ull, 7ull, 11ull, 13ull, 17ull, 19ull, 23ull, 29ull, 31ull, 37ull}) {
        if (n == p) return true;
        if (n % p == 0) return false;
    }
    u64 d = n - 1;
    int s = 0;
    while ((d & 1) == 0) { d >>= 1; ++s; }
    // 对 2^64 范围内确定性的 Miller-Rabin 基组
    for (u64 a : {2ull, 325ull, 9375ull, 28178ull, 450775ull, 9780504ull, 1795265022ull}) {
        if (a % n == 0) continue;
        u64 x = mod_pow64(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool composite = true;
        for (int r = 1; r < s; ++r) {
            x = (u64)((u128)x * x % n);
            if (x == n - 1) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}

bool ref_is_prime32(u32 n) {
    return ref_is_prime64(n);
}

/* 128 位参考 Miller-Rabin：前 12 个素数基底（< psi_12 ~ 2^78 确定性，
   更大时误判率 <= 4^-12，测试用途足够）。r128 为原生 __uint128；
   模乘为二进制加倍（先比较后组合，无 129 位回绕），避免
   __int128 乘积截断
*/
using r128 = unsigned __int128;

static r128 mulmod128(r128 a, r128 b, r128 m) {
    r128 r = 0;
    a %= m;
    while (b) {
        if (b & 1) {
            if (r >= m - a) r -= m - a; else r += a;
        }
        b >>= 1;
        if (b) {
            if (a >= m - a) a -= m - a; else a += a;
        }
    }
    return r;
}

static r128 mod_pow128(r128 a, r128 e, r128 m) {
    r128 r = 1 % m;
    a %= m;
    while (e) {
        if (e & 1) r = mulmod128(r, a, m);
        a = mulmod128(a, a, m);
        e >>= 1;
    }
    return r;
}

static bool ref_is_prime128(r128 n) {
    if (n < 2) return false;
    for (u64 p : {2ull, 3ull, 5ull, 7ull, 11ull, 13ull, 17ull, 19ull, 23ull, 29ull, 31ull, 37ull}) {
        if (n == p) return true;
        if (n % p == 0) return false;
    }
    r128 d = n - 1;
    int s = 0;
    while ((d & 1) == 0) { d >>= 1; ++s; }
    for (u64 a : {2ull, 3ull, 5ull, 7ull, 11ull, 13ull, 17ull, 19ull, 23ull, 29ull, 31ull, 37ull}) {
        r128 x = mod_pow128(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool composite = true;
        for (int r = 1; r < s; ++r) {
            x = mulmod128(x, x, n);
            if (x == n - 1) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}

/* 三态期望值：素数时按 SWbound 分界返回 1（确定性）或 2（BPSW 型），
   契约要求 hi > 0 */
static int expect2(u64 lo, u64 hi, bool prime) {
    if (!prime) return 0;
    return (((u128)hi << 64) | lo) < ((((u128)0x2be69) << 64) | 0x51adc5b22410a5fdull) ? 1 : 2;
}

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
    {0xefb4cad164f85da9ull, 0x0000000002874a1full, false},
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
    {0x25e74da2f39f015full, 0x00000000a93c27e4ull, false},
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
    {0xed1137eb3e5e1047ull, 0x000028ad8e5e8733ull, false},
    {0xe3bd76bf57cec991ull, 0x000030083e2681d1ull, false},
    {0xeee4852d330c2395ull, 0x000021512f3334bfull, false},
    {0xcfe680854c95ea73ull, 0x000038eac209f79dull, false},
    {0xa2842cb2fb44c6a3ull, 0x00002c91a0f4dd5aull, false},
    {0xe5fbc6d02bd667d7ull, 0x00002d0f621d123aull, false},
    {0x6cb5b7d2782a1891ull, 0x00003bc198411febull, false},
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

TEST_CASE("numth/prime", is_prime_uint_ulong_notrial) {
    u64 seed = 0xfeed0912eadc23aeull;
    for (u32 n = 0; n < 10000; ++n) {
        bool expect = ref_is_prime32(n);
        TEST_CHECK_MSG(lmmp_is_prime_uint_(n) == expect, "is_prime_uint small");
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(n) == expect, "is_prime_ulong small");
    }

    for (int i = 0; i < 10000; ++i) {
        u64 n = xorshift64(seed);
        bool expect = ref_is_prime64(n);
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(n) == expect, "is_prime_ulong random");
        if (n > 2) TEST_CHECK_MSG(lmmp_is_prime_notrial_(n) == expect, "is_prime_notrial random");
    }
}

TEST_CASE("numth/prime", is_prime_2) {
    struct { u64 hi, lo; bool expect; const char* what; } cases[] = {
        {0x7fffffffffffffffull, 0xffffffffffffffffull, true, "2^127-1 Mersenne"},
        {0xffffffffffffffffull, 0xffffffffffffff61ull, true, "2^128-159 largest prime < 2^128"},
        {0x0000000000000001ull, 0x000000000000000dull, true, "2^64+13"},
        {0x0000000000000001ull, 0x0000000000000001ull, false, "2^64+1 Fermat"},
        {0xffffffffffffffffull, 0xffffffffffffffffull, false, "2^128-1 (3 divides)"},
        {0x000000000000437aull, 0xe92817f9fc85b7e5ull, false, "psi_12 SPSP(2..37)"},
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

    /* 平方数（Lucas 无 D 的防御路径不应误判） */
    {
        u64 p = 0x000001000000000full;  // >2^40 素数
        TEST_CHECK(ref_is_prime64(p));
        u128 sq = (u128)p * p;
        TEST_CHECK_MSG(lmmp_is_prime_2_(lo128(sq), hi128(sq)) == 0, "square rejected");
        u128 sq3 = sq * (u128)3;
        TEST_CHECK_MSG(lmmp_is_prime_2_(lo128(sq3), hi128(sq3)) == 0, "3*square rejected");
    }
}

/*
    is_prime_2 增强测试：128 位独立参考对拍 + 三态类别 + 边界结构。
    契约要求 hi > 0（调用方保证），本 case 的全部合法输入均满足。
    参考为前 12 个素数基底的 MR（< psi_12 ~ 2^78 确定性，更大时误判率
    <= 4^-12，本 case 约千余次调用的总误判期望 ~ 1e-5，足够可靠）
*/
TEST_CASE("numth/prime", is_prime_2_more) {
    const r128 SW = (((r128)0x2be69) << 64) | 0x51adc5b22410a5fdull;
    u64 seed = 0xa5a5137f9e3779b9ull;

    /* 1. SWbound 邻域程序化扫描：确定性(1)/BPSW(2) 切换点两侧，
          每个奇数均与参考对拍（含切点下沿确定性路径与上沿 Lucas 路径） */
    {
        int cnt = 0;
        for (r128 off = 1; off <= 801 && cnt < 400; off += 2) {
            r128 nn[2] = {SW - off, SW + off};
            for (int k = 0; k < 2; k++, cnt++) {
                r128 n = nn[k];
                if (n < 3) continue;
                int r = lmmp_is_prime_2_((u64)n, (u64)(n >> 64));
                bool ref = ref_is_prime128(n);
                int expect = ref ? (n < SW ? 1 : 2) : 0;
                TEST_CHECK_MSG(r == expect, "SW neighborhood tri-state");
            }
        }
    }

    /* 2. 随机全域分层（65..128 bit）：三态与参考对拍 */
    for (int i = 0; i < 200; ++i) {
        int bits = 65 + (int)(xorshift64(seed) % 64);
        /* 顶位强制置 1 保证恰为 bits 位（移位后顶位随机可能使 hi == 0，
           违反契约） */
        r128 n = ((((r128)xorshift64(seed) << 64) | xorshift64(seed)) >> (128 - bits)) |
                 (((r128)1) << (bits - 1)) | 1;
        int r = lmmp_is_prime_2_((u64)n, (u64)(n >> 64));
        bool ref = ref_is_prime128(n);
        int expect = ref ? (n < SW ? 1 : 2) : 0;
        TEST_CHECK_MSG(r == expect, "random tri-state");
    }

    /* 3. 素数密集搜索：多规模点（含 n > 2^127 的 REDC 回绕路径与
          hi = ULONG_MAX 的试除全比较路径），类别必须与规模一致，
          每规模首个素数额外经参考复核 */
    {
        const u64 his[] = {0x0000000000000001ull, 0x0000000000010000ull,
                           0x000000000002be69ull, /* SW 高位侧 */
                           0x6d4b2a51f0e3c918ull, 0x8000000000000000ull,
                           0xffffffffffffffffull};
        for (u64 hi : his) {
            u64 lo = xorshift64(seed) | 1;
            int found = 0;
            r128 n = ((r128)hi << 64) | lo;
            while (found < 4) {
                if (lmmp_is_prime_2_((u64)n, (u64)(n >> 64)) != 0) {
                    int expect = n < SW ? 1 : 2;
                    TEST_CHECK_MSG(lmmp_is_prime_2_((u64)n, (u64)(n >> 64)) == expect,
                                   "prime-search category");
                    if (found == 0) TEST_CHECK(ref_is_prime128(n)); /* 参考复核 */
                    found++;
                }
                n += 2;
            }
        }
    }

    /* 4. FAST_TRIAL_BOUND 两侧：hi < bound 走高位单比较，hi >= bound
          走全 limb 比较（bound = ULONG_MAX - 137） */
    {
        const u64 bhis[] = {(u64)-139, (u64)-138, (u64)-137, (u64)-1};
        for (u64 hi : bhis) {
            u64 lo = xorshift64(seed) | 1;
            for (int k = 0; k < 32; k++, lo += 2) {
                r128 n = ((r128)hi << 64) | lo;
                int r = lmmp_is_prime_2_((u64)n, (u64)(n >> 64));
                bool ref = ref_is_prime128(n);
                int expect = ref ? 2 : 0;
                TEST_CHECK_MSG(r == expect, "trial bound sides");
            }
        }
    }

    /* 5. 特殊结构输入 */
    {
        /* Mersenne/Fermat 型 2^k-1（k = 65..127 抽样） */
        for (int k = 65; k < 128; k += 7) {
            r128 n = (((r128)1) << k) - 1;
            int r = lmmp_is_prime_2_((u64)n, (u64)(n >> 64));
            bool ref = ref_is_prime128(n);
            int expect = ref ? (n < SW ? 1 : 2) : 0;
            TEST_CHECK_MSG(r == expect, "mersenne type");
        }
        /* 完全立方数（r <= 2^42 保证 r^3 < 2^127，hi 恒非零） */
        for (u64 r : {3ull, 5ull, 0x10001ull, 0x3ed45e8bull, 0x10000000000ull}) {
            r128 cu = (r128)r * r * r;
            if (cu >> 64) {
                TEST_CHECK_MSG(lmmp_is_prime_2_((u64)cu, (u64)(cu >> 64)) == 0, "cube rejected");
            }
        }
        /* 随机大素数平方（p < 2^62 保证 p^2 < 2^124 且搜索不越 2^63） */
        for (int i = 0; i < 8; i++) {
            u64 p = (xorshift64(seed) >> 2) | 1;
            while (!ref_is_prime64(p)) p += 2;
            r128 sq = (r128)p * p;
            TEST_CHECK_MSG(lmmp_is_prime_2_((u64)sq, (u64)(sq >> 64)) == 0, "big square rejected");
        }
    }
}

TEST_CASE("numth/prime", next_prev_prime) {
    // 小范围穷举
    ulong prev = 0;
    for (ulong n = 0; n < 2000; ++n) {
        ulong next = lmmp_next_prime_ulong_(n);
        if (n < 2) { TEST_CHECK_EQ(next, 2u); } else {
            bool found = false;
            for (ulong k = n + 1; k < 100000; ++k) {
                if (ref_is_prime64(k)) { TEST_CHECK_EQ(next, k); found = true; break; }
            }
            TEST_CHECK(found);
        }
        ulong p = lmmp_prev_prime_ulong_(n);
        if (n < 2) TEST_CHECK_EQ(p, 0u);
        else {
            bool found = false;
            for (ulong k = n; ; --k) {
                if (ref_is_prime64(k)) { TEST_CHECK_EQ(p, k); found = true; break; }
            }
            TEST_CHECK(found);
        }
        (void)prev;
    }

    // 随机 64 位附近
    u64 seed = 0x230bc3ae812873c0ull;
    for (int i = 0; i < 100; ++i) {
        ulong n = xorshift64(seed) | 1;
        if (n < 2) n = 2;
        ulong next = lmmp_next_prime_ulong_(n);
        TEST_CHECK_MSG(next > n, "next_prime > n");
        TEST_CHECK_MSG(ref_is_prime64(next), "next_prime is prime");
        if (next > n + 1) {
            for (ulong k = n + 1; k < next; ++k) TEST_CHECK_MSG(!ref_is_prime64(k), "next_prime no gap prime");
        }
        ulong p = lmmp_prev_prime_ulong_(n);
        if (p > 0) {
            TEST_CHECK_MSG(ref_is_prime64(p), "prev_prime is prime");
            TEST_CHECK_MSG(p <= n, "prev_prime <= n");
            for (ulong k = p + 1; k <= n; ++k) TEST_CHECK_MSG(!ref_is_prime64(k), "prev_prime no gap prime");
        }
    }
}

TEST_CASE("numth/prime", mulmod_powmod) {
    u64 seed = 0x2cbea9127ca8019full;
    for (int i = 0; i < 1000; ++i) {
        ulong mod = xorshift64(seed) | 3;  // 避免过小
        ulong a = xorshift64(seed) % mod;
        ulong b = xorshift64(seed) % mod;
        ulong q = 0;
        ulong r = lmmp_mulmod_ulong_(a, b, mod, &q);
        u128 prod = (u128)a * b;
        TEST_CHECK_MSG((u128)q * mod + r == prod, "mulmod relation");
        TEST_CHECK_MSG(r < mod, "mulmod rem bound");
    }

    for (int i = 0; i < 500; ++i) {
        u32 mod = (u32)(xorshift64(seed) | 3);
        u32 base = (u32)(xorshift64(seed) % mod);
        ulong exp = xorshift64(seed) & 0xffff;
        u32 r = lmmp_powmod_uint_odd_(base, exp, mod);
        u64 expect = mod_pow64(base, exp, mod);
        TEST_CHECK_EQ(r, (u32)expect);
    }

    for (int i = 0; i < 500; ++i) {
        ulong mod = xorshift64(seed) | 3;
        ulong base = xorshift64(seed) % mod;
        ulong exp = xorshift64(seed) & 0xffff;
        ulong r = lmmp_powmod_ulong_odd_(base, exp, mod);
        u64 expect = mod_pow64(base, exp, mod);
        TEST_CHECK_EQ(r, expect);
    }
}

/*
    ============ >128 位素性检验：lmmp_is_strong_lucas_ / lmmp_is_prime_n_
    ============

    （单轮 MR 原语已降级为 is_prime_n.c 内部 static 实现，不再对外暴露，
    故本文件只测对外接口；其行为由各档位用例与 §4.2 定向向量间接覆盖。）
    参考实现全部独立于库代码：
    - ref_sprp_big：BigInt 教科书 MR（Knuth-D 取模梯子）；
    - ref_strong_lucas_big：标准 (U,V,Q) 单状态阶梯（Crandall-Pomerance
      公式，含 /2 即乘 inv2 的 mod-n 处理），与库内 V-only 判据互为独立
      实现，等价性在 gcd(D,n)=1 下成立；
    - ref_jacobi_si：教科书二进制 Jacobi。
    参考分类取前 12 个素数基底 MR（< psi_12 ~ 2^78 确定性，更大时误判率
    <= 4^-12，本文件调用规模下总误判期望可忽略）。
*/

namespace {

size_t bigint_bits_n(const BigInt& x) {
    u64 top = x.d.back();
    return (x.d.size() - 1) * 64 + (64 - __builtin_clzll(top == 0 ? 1 : top));
}

BigInt mod_school_n(const BigInt& x, const BigInt& m) {
    BigInt q, r;
    q = BigInt::divmod_school(x, m, r);
    (void)q;
    return r;
}

BigInt powmod_school_n(const BigInt& b, const BigInt& e, const BigInt& m) {
    BigInt r(1), base = mod_school_n(b, m);
    size_t ebits = bigint_bits_n(e);
    for (size_t i = 0; i < ebits; ++i) {
        if ((e.d[i / 64] >> (i % 64)) & 1)
            r = mod_school_n(BigInt::mul_school(r, base), m);
        if (i + 1 < ebits)
            base = mod_school_n(BigInt::sqr_school(base), m);
    }
    return r;
}

/* mod-n 加/减（操作数与结果均为 < n 的规范剩余；n 奇）*/
BigInt addmod_big(const BigInt& a, const BigInt& b, const BigInt& n) {
    BigInt s = BigInt::add_abs(a, b);
    if (BigInt::cmp(s, n) >= 0) s = BigInt::sub_abs(s, n);
    return s;
}

BigInt submod_big(const BigInt& a, const BigInt& b, const BigInt& n) {
    return BigInt::cmp(a, b) >= 0 ? BigInt::sub_abs(a, b)
                                  : BigInt::sub_abs(n, BigInt::sub_abs(b, a));
}

/* (b|A)：A 奇正，b 非负（教科书二进制算法）*/
int ref_jacobi_small(u64 b, u64 A) {
    int sign = 1;
    if (b == 0) return 0;
    for (;;) {
        int tz = __builtin_ctzll(b);
        b >>= tz;
        if ((tz & 1) && ((A & 7) == 3 || (A & 7) == 5)) sign = -sign;
        if (b == 1) return sign;
        if (b < A) {
            u64 s = b;
            b = A;
            A = s;
            if ((b & 3) == 3 && (A & 3) == 3) sign = -sign;
        }
        b -= A;
        if (b == 0) return 0;
    }
}

/* (D|n)：D 为小奇（可负），n 为大奇数，经二次互反律折叠 */
int ref_jacobi_si(long D, const BigInt& n) {
    u64 A = D < 0 ? (u64)(-D) : (u64)D;
    u64 b = BigInt::mod_small(n, A);
    int sign = 1;
    if ((A & 3) == 3 && (n.d[0] & 3) == 3) sign = -sign;
    sign *= ref_jacobi_small(b, A);
    if (sign == 0) return 0;
    if (D < 0 && (n.d[0] & 3) == 3) sign = -sign;
    return sign;
}

bool ref_sprp_big(const BigInt& n, const BigInt& b) {
    BigInt nm1 = BigInt::sub_small(n, 1);
    size_t t = 0;
    while (((nm1.d[t / 64] >> (t % 64)) & 1) == 0) ++t;
    BigInt d = BigInt::shr_bits(nm1, t);
    BigInt x = powmod_school_n(b, d, n);
    BigInt m1 = BigInt::sub_small(n, 1);
    if (x == BigInt(1) || x == m1) return true;
    for (size_t r = 1; r < t; ++r) {
        x = mod_school_n(BigInt::sqr_school(x), n);
        if (x == m1) return true;
    }
    return false;
}

bool ref_is_prime_big(const BigInt& n) {
    if (n.d.size() == 1) return ref_is_prime64(n.d[0]);
    for (u64 p : {2ull, 3ull, 5ull, 7ull, 11ull, 13ull, 17ull, 19ull, 23ull, 29ull, 31ull, 37ull}) {
        if (n == BigInt(p)) return true;
        if (BigInt::mod_small(n, p) == 0) return false;
    }
    for (u64 a : {2ull, 3ull, 5ull, 7ull, 11ull, 13ull, 17ull, 19ull, 23ull, 29ull, 31ull, 37ull}) {
        if (!ref_sprp_big(n, BigInt(a))) return false;
    }
    return true;
}

/*
    强 Lucas 参考（Selfridge 方法 A，标准 (U,V,Q) 阶梯）：
      自 (U,V,Qk)=(U_k,V_k,Q^k)（k=1 起，P=1）：
        倍加 k->2k:   U'=U*V, V'=V^2-2Qk, Qk'=Qk^2
        加一 2k->2k+1: U'=(U'+V')/2, V'=(V'+D*U')/2, Qk'=Qk'*Q （/2 即乘 inv2）
      通过: U_d=0 或 V_{d*2^r}=0（0<=r<s），d*2^s=n+1
*/
bool ref_strong_lucas_big(const BigInt& n) {
    long D = 5;
    for (;;) {
        int j = ref_jacobi_si(D, n);
        if (j == 0) return false;
        if (j == -1) break;
        D = D > 0 ? -(D + 2) : -(D - 2);
    }
    long Q = (1 - D) / 4;
    u64 Da = D < 0 ? (u64)(-D) : (u64)D;

    BigInt np1 = BigInt::add_small(n, 1);
    size_t s = 0;
    while (((np1.d[s / 64] >> (s % 64)) & 1) == 0) ++s;
    BigInt d = BigInt::shr_bits(np1, s);

    BigInt inv2 = BigInt::shr_bits(BigInt::add_small(n, 1), 1); /* (n+1)/2 */
    auto mulmod = [&](const BigInt& a, const BigInt& b) { return mod_school_n(BigInt::mul_school(a, b), n); };
    BigInt U(1), V(1); /* U_1 = 1，V_1 = P = 1 */
    BigInt Qres(Q < 0 ? BigInt::sub_abs(n, BigInt((u64)(-Q))) : BigInt((u64)Q)); /* Q 的 mod-n 剩余 */
    BigInt Qk = Qres; /* Q^1 */

    size_t db = bigint_bits_n(d);
    for (size_t i = db - 1; i-- > 0;) {
        BigInt u2 = mulmod(U, V);
        BigInt v2 = submod_big(mulmod(V, V), addmod_big(Qk, Qk, n), n);
        BigInt q2 = mulmod(Qk, Qk);
        if ((d.d[i / 64] >> (i % 64)) & 1) {
            BigInt t = mulmod(addmod_big(u2, v2, n), inv2);   /* U_{2k+1} */
            BigInt w = mulmod(u2, BigInt(Da));                 /* |D|*U_{2k} */
            BigInt v3 = D < 0 ? addmod_big(v2, w, n) : submod_big(v2, w, n);
            v3 = mulmod(v3, inv2);                             /* V_{2k+1} */
            u2 = t;
            v2 = v3;
            q2 = mulmod(q2, Qres); /* Q^{2k+1} */
        }
        U = u2;
        V = v2;
        Qk = q2;
    }
    /* 判据 */
    if (U.is_zero()) return true;
    for (size_t r = 0; r < s; ++r) {
        if (r > 0) {
            V = submod_big(mulmod(V, V), addmod_big(Qk, Qk, n), n);
            Qk = mulmod(Qk, Qk);
        }
        if (V.is_zero()) return true;
    }
    return false;
}

/* 随机大奇数（ limbs 位数，顶两位置 10 保持位数与加 2 余量）*/
BigInt rand_big_odd(u64& seed, size_t limbs) {
    BigInt r;
    r.d.assign(limbs, 0);
    for (size_t i = 0; i < limbs; ++i) r.d[i] = xorshift64(seed);
    r.d[limbs - 1] = (r.d[limbs - 1] >> 2) | ((u64)1 << 62);
    r.d[0] |= 1;
    r.trim();
    return r;
}

BigInt next_prime_big(u64& seed, size_t limbs) {
    BigInt n = rand_big_odd(seed, limbs);
    for (;;) {
        if (ref_is_prime_big(n)) return n;
        n = BigInt::add_small(n, 2);
    }
}

mp_ptr limbs_of(const BigInt& x) {
    mp_ptr p = (mp_ptr)lmmp_alloc(x.d.size() * sizeof(mp_limb_t));
    to_limbs(x, p, (mp_size_t)x.d.size());
    return p;
}

}  // namespace

/*
    §4.2 定向向量：强伪素数（SPRP）结构。

    (1) 经典 SPRP-2 小表（2047、3277、4033、4681、8321、15841、29341…）：
        它们逐一低于本模块契约域 n > 2^128，只能用 64 位接口验证"整条流水线
        必判合数"（基 2 轮放行、后续轮拒判——已独立确认这些小表均非 SPRP(3)）。
    (2) >2^128 的定向向量取 Mersenne 数 M_p = 2^p-1（p 为奇素数且 M_p 合数）：
        ord_{M_p}(2) = p，且 (M_p-1)/2 = 2^(p-1)-1 被 p 整除（费马小定理），
        故 2 是 M_p 的强伪素数基底——基 2 轮必放行，只能由后续随机基底轮拒判，
        正是"能骗过基 2 的合数"必须被拦下的定向检验。其中
        2^137-1、2^149-1、2^163-1 的最小素因子（分别 >2e5、>2e5、150287）都
        超过全部档位的试除上界（<=10000），故 8 个档位都必须在跑完 1 limb 轮
        （或其后轮次）后判合数；2^131-1 的最小素因子为 263，超过 0 档试除上界
        100，0 档同样只能靠随机轮拒判（1 档起由试除拦下）。
*/
TEST_CASE("numth/prime", is_prime_n_spsp2) {
    for (u64 n : {2047ull, 3277ull, 4033ull, 4681ull, 8321ull, 15841ull, 29341ull}) {
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(n) == false, "classic spsp2 rejected (ulong)");
        TEST_CHECK_MSG(lmmp_is_prime_notrial_(n) == false, "classic spsp2 rejected (notrial)");
    }

    for (int p : {131, 137, 149, 163}) {
        BigInt n = BigInt::sub_small(BigInt::shl_bits(BigInt(1), p), 1);
        mp_ptr np = limbs_of(n);
        mp_size_t nn = (mp_size_t)n.d.size();
        for (int s = 0; s <= 7; ++s) {
            TEST_CHECK_MSG(lmmp_is_prime_n_(np, nn, s) == 0, "Mersenne composite rejected by all tiers");
        }
        lmmp_free(np);
    }
}

/* 强 Lucas 精确对拍（独立 U/V 阶梯参考）+ 平方数与素数专项 */
TEST_CASE("numth/prime", is_strong_lucas_n) {
    u64 seed = 0x2f1e3d4c5b6a7988ull;

    /* 素数必过（含各种 D 命中深度）*/
    for (size_t limbs : {3, 4, 5}) {
        for (int k = 0; k < 3; ++k) {
            BigInt p = next_prime_big(seed, limbs);
            mp_ptr pp = limbs_of(p);
            TEST_CHECK_MSG(lmmp_is_strong_lucas_(pp, (mp_size_t)p.d.size()) == 1, "lucas prime pass");
            lmmp_free(pp);
        }
    }
    /* 完全平方数必拒（D 搜索的延迟平方检测路径）*/
    for (int k = 0; k < 6; ++k) {
        BigInt p = next_prime_big(seed, 2);
        BigInt sq = BigInt::mul_school(p, p);
        if (sq.d.size() < 3) sq = BigInt::mul_school(sq, BigInt(3));
        mp_ptr sp = limbs_of(sq);
        TEST_CHECK_MSG(lmmp_is_strong_lucas_(sp, (mp_size_t)sq.d.size()) == 0, "lucas square reject");
        lmmp_free(sp);
    }
    /* 非平方的含平方因子合数（p^2*q / p^3 型）：perfsqr 拦不住、直达
       阶梯判据，正是 U_d=0 与 U_d^2=0 分歧的形态，须与独立参考精确一致 */
    for (int k = 0; k < 8; ++k) {
        BigInt p = next_prime_big(seed, 2);
        BigInt q = next_prime_big(seed, 2);
        BigInt n = k % 2 ? BigInt::mul_school(BigInt::mul_school(p, p), q)
                         : BigInt::mul_school(BigInt::mul_school(p, BigInt(p)), BigInt(p));
        if (n.d.size() < 3) n = BigInt::mul_school(n, BigInt(0x1000000007ull));
        if (n.d.size() < 3) continue;
        if (BigInt::mod_small(n, 3) == 0) n = BigInt::add_small(n, 2); /* 3|n 时 D=9 提前退出的平凡路径 */
        mp_ptr np = limbs_of(n);
        TEST_CHECK_MSG(lmmp_is_strong_lucas_(np, (mp_size_t)n.d.size()) == ref_strong_lucas_big(n),
                       "lucas nonsquarefree exact");
        lmmp_free(np);
    }
    /* 随机 + 结构输入与独立参考精确对拍 */
    for (size_t limbs : {3, 4, 6}) {
        for (int k = 0; k < 12; ++k) {
            BigInt n;
            if (k % 4 == 3) {
                /* 2^m±1 型（长 t / d=1 的阶梯边界）*/
                n = BigInt::shl_bits(BigInt(1), limbs * 64 - 1 - (k % 11));
                n = k % 2 ? BigInt::add_small(n, 1) : BigInt::sub_small(n, 1);
                if ((n.d[0] & 1) == 0 || n.d.size() < 3) n = BigInt::add_small(n, 1);
                if (n.d.size() < 3) continue;
            } else {
                n = rand_big_odd(seed, limbs);
            }
            if (BigInt::mod_small(n, 3) == 0) n = BigInt::add_small(n, 2); /* 避开平凡因子 */
            mp_ptr np = limbs_of(n);
            int expect = ref_strong_lucas_big(n);
            TEST_CHECK_MSG(lmmp_is_strong_lucas_(np, (mp_size_t)n.d.size()) == expect, "lucas exact");
            lmmp_free(np);
        }
    }
}

/* is_prime_n_：结构向量 + 随机分层，全档位与参考分类对拍 */
TEST_CASE("numth/prime", is_prime_n) {
    u64 seed = 0x77aa55eecd119922ull;

    /* 1. 结构向量 */
    {
        std::vector<BigInt> nums;
        nums.push_back(BigInt::add_small(BigInt::shl_bits(BigInt(1), 128), 1)); /* 2^128+1 */
        nums.push_back(BigInt::add_small(BigInt::shl_bits(BigInt(1), 129), 1));
        nums.push_back(BigInt::add_small(BigInt::shl_bits(BigInt(1), 192), 1)); /* 2^192+1 代数分解 */
        nums.push_back(BigInt::add_small(BigInt::shl_bits(BigInt(1), 256), 1)); /* F5 */
        nums.push_back(BigInt::sub_small(BigInt::shl_bits(BigInt(1), 193), 1)); /* 2^193-1 */
        nums.push_back(BigInt::sub_small(BigInt::shl_bits(BigInt(1), 192), 1)); /* 全 1 顶 */
        for (BigInt n : nums) {
            int expect = ref_is_prime_big(n) ? 2 : 0;
            mp_ptr np = limbs_of(n);
            for (int s = 0; s <= 7; ++s)
                TEST_CHECK_MSG(lmmp_is_prime_n_(np, (mp_size_t)n.d.size(), s) == expect, "structural");
            lmmp_free(np);
        }
        /* p²、p·q、97·p、p（各强度）*/
        BigInt p3 = next_prime_big(seed, 3);
        BigInt p2 = next_prime_big(seed, 2);
        std::vector<BigInt> more;
        more.push_back(BigInt::mul_school(p2, p2));        /* 平方 */
        more.push_back(BigInt::mul_school(p3, p2));        /* 半素数（无小因子）*/
        more.push_back(BigInt::mul_school(p3, BigInt(97))); /* 小因子 97 */
        more.push_back(p3);                                 /* 素数 */
        for (BigInt n : more) {
            if (n.d.size() < 3) continue;
            int expect = ref_is_prime_big(n) ? 2 : 0;
            mp_ptr np = limbs_of(n);
            for (int s = 0; s <= 7; ++s)
                TEST_CHECK_MSG(lmmp_is_prime_n_(np, (mp_size_t)n.d.size(), s) == expect, "structured");
            lmmp_free(np);
        }
    }

    /* 2. 偶数与试除命中 */
    {
        BigInt p3 = next_prime_big(seed, 3);
        BigInt even = BigInt::add_small(p3, 1);
        BigInt by3 = BigInt::mul_school(p3, BigInt(3));
        BigInt by97 = BigInt::mul_school(p3, BigInt(9973));
        for (BigInt* n : {&even, &by3, &by97}) {
            mp_ptr np = limbs_of(*n);
            for (int s = 0; s <= 7; ++s)
                TEST_CHECK_MSG(lmmp_is_prime_n_(np, (mp_size_t)n->d.size(), s) == 0, "quick reject");
            lmmp_free(np);
        }
    }

    /* 3. 随机分层：小尺寸全档位，大尺寸抽样档位 */
    for (size_t limbs : {3, 4, 5, 6}) {
        for (int k = 0; k < 8; ++k) {
            BigInt n = rand_big_odd(seed, limbs);
            int expect = ref_is_prime_big(n) ? 2 : 0;
            mp_ptr np = limbs_of(n);
            for (int s = 0; s <= 7; ++s)
                TEST_CHECK_MSG(lmmp_is_prime_n_(np, (mp_size_t)limbs, s) == expect, "random tiers");
            lmmp_free(np);
        }
    }
    for (size_t limbs : {10, 20}) {
        for (int k = 0; k < 2; ++k) {
            BigInt n = rand_big_odd(seed, limbs);
            int expect = ref_is_prime_big(n) ? 2 : 0;
            mp_ptr np = limbs_of(n);
            for (int s : {0, 3, 4, 7})
                TEST_CHECK_MSG(lmmp_is_prime_n_(np, (mp_size_t)limbs, s) == expect, "random large");
            lmmp_free(np);
        }
    }
}
