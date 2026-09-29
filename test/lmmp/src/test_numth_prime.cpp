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
