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

#include <cmath>
#include <cstring>
#include <vector>

#include "lmmp/lmmpn.h"
#include "lmmp/impl/mparam.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

BigInt pow_big(const BigInt& b, ulong e) {
    BigInt r(1);
    for (ulong i = 0; i < e; ++i) r = BigInt::mul_school(r, b);
    return r;
}

// BigInt 高位补零写入 [buf,n]
void big_to_limbs(const BigInt& v, mp_ptr buf, size_t n) {
    std::memset(buf, 0, n * sizeof(mp_limb_t));
    std::memcpy(buf, v.d.data(), v.d.size() * sizeof(mp_limb_t));
}

// oracle 校验：r = floor(A^(1/root)) 且（若给 rem）A = r^root + rem
void check_floor(const BigInt& A, mp_srcptr r, mp_size_t rn, ulong root, mp_srcptr rem = nullptr) {
    BigInt br(r, (size_t)rn);
    BigInt low = pow_big(br, root);
    BigInt high = pow_big(BigInt::add_small(br, 1), root);
    TEST_CHECK_MSG(low <= A, "r^root <= A");
    TEST_CHECK_MSG(A < high, "A < (r+1)^root");
    if (rem) {
        BigInt brem(rem, (size_t)((root - 1) * rn + 1));
        TEST_CHECK_MSG(BigInt::add_abs(low, brem) == A, "A == r^root + rem");
    }
}

}  // namespace

/*
    顶层入口：任意长度/任意顶 limb。na 步进覆盖对齐/非对齐形状，
    root=4 行覆盖 sqrt∘sqrt 专用路线。
*/
TEST_CASE("numth/nthroot", nthroot_top_random_and_powers) {
    u64 seed = 0xF1E2D3C4B5A69788ull;
    const ulong roots[] = {4, 5, 8, 16, 32, 100};
    const mp_size_t NAMAX = 512;
    mp_size_t tpsize = 0;
    for (ulong root : roots)
        for (mp_size_t na = 1; na <= NAMAX; ++na) tpsize = LMMP_MAX(tpsize, lmmp_nthroot_size_(na, root));
    mp_ptr numa = alloc_limbs(NAMAX);
    mp_ptr dst = alloc_limbs(64);
    mp_ptr tp = alloc_limbs((size_t)tpsize);

    for (ulong root : roots) {
        for (mp_size_t na = 1; na <= 5 * (mp_size_t)root && na <= 64; na += (mp_size_t)root / 4 + 1) {
            mp_size_t nr = (na + (mp_size_t)root - 1) / (mp_size_t)root;
            mp_ptr dstr = alloc_limbs((size_t)((root - 1) * nr + 1));
            for (int t = 0; t < 6; ++t) {
                for (mp_size_t i = 0; i < na; ++i) numa[i] = xorshift64(seed);
                if (t & 1)
                    numa[na - 1] = LIMB_MAX - xorshift64(seed) % (LIMB_MAX / 4);  // 高顶
                else
                    numa[na - 1] = xorshift64(seed) % 0x10000 + 1;                // 低顶
                BigInt A(numa, (size_t)na);
                int calr = t & 2;
                lmmp_nthroot_(dst, calr ? dstr : NULL, numa, na, root, tp);
                if (calr)
                    check_floor(A, dst, nr, root, dstr);
                else
                    check_floor(A, dst, nr, root);
            }
            lmmp_free(dstr);
        }
        // 完全幂/±1（任意长度边界）
        for (mp_size_t nr = 1; nr <= 3; ++nr) {
            mp_size_t dstrn = (root - 1) * nr + 1;
            mp_ptr dstr = alloc_limbs((size_t)dstrn);
            for (int t = 0; t < 4; ++t) {
                BigInt S;
                S.d.resize((size_t)nr);
                for (mp_size_t i = 0; i < nr; ++i) S.d[i] = xorshift64(seed);
                S.d[nr - 1] |= 1ull << 63;
                S.trim();
                BigInt A0 = BigInt::pow(S, root);
                for (int d = 0; d <= 1; ++d) {
                    BigInt A = d ? BigInt::sub_small(A0, 1) : A0;
                    if ((mp_size_t)A.d.size() > root * nr + root / 2) continue;
                    mp_size_t na = (mp_size_t)A.d.size();
                    mp_size_t nr2 = (na + (mp_size_t)root - 1) / (mp_size_t)root;
                    if ((root - 1) * nr2 + 1 > dstrn) continue;
                    big_to_limbs(A, numa, na);
                    if (numa[na - 1] == 0) continue;
                    BigInt Ac(numa, (size_t)na);
                    lmmp_nthroot_(dst, dstr, numa, na, root, tp);
                    check_floor(Ac, dst, nr2, root, dstr);
                }
            }
            lmmp_free(dstr);
        }
    }
    lmmp_free(numa);
    lmmp_free(dst);
    lmmp_free(tp);
}

/*
    认证式窗口探针的定向构造输入（第十一轮）：根长 >= 4 limb
    （bitlen(y) >= 256 gate 激活探针）下覆盖探针各判决与回退带：

    - A = S^root（完全幂）：slack ~ 0，必落 |V-Wc| < 2^(hb+69) 不确定带
      → 全幂回退，输出必须仍精确；
    - A = S^root ± {1,2}：slack 在守卫带边缘；
    - A = (S+1)^root - {1,2,3}：区间顶（e0=1 形状，dec 侧压力）；
    - A = S^root + root·S^(root-1) ± {1,2}：Bernoulli 阈值处
      （(y+1)^root - y^root 的首项），done-cert/inc-cert/不确定带
      三者的分界——构造必中回退带，正确性必须由定理+回退承载。

    概率性捷径的用户质询点：随机输入只测快路径命中率；本用例的
    构造输入系统性命中慢路径（回退/边界判决），两端都必须精确。
*/
TEST_CASE("numth/nthroot", nthroot_top_probe_construction) {
    u64 seed = 0xA5A512349ABCDEF7ull;
    const ulong roots[] = {4, 5, 8, 16, 32, 64, 100};
    const mp_size_t NRS[] = {4, 5};

    for (ulong root : roots) {
        for (mp_size_t nr : NRS) {
            mp_size_t namax = (mp_size_t)root * nr + root / 2 + 2;
            mp_size_t tpsz = lmmp_nthroot_size_(namax, root);
            mp_ptr numa = alloc_limbs((size_t)namax);
            mp_ptr dst = alloc_limbs((size_t)nr + 2);
            mp_ptr dstr = alloc_limbs((size_t)((root - 1) * nr + 2));
            mp_ptr tp = alloc_limbs((size_t)tpsz);
            // 阈值项 root·S^(root-1)（root 偶数时附带半阈值 root/2·S^(root-1)）
            int half = (root % 2 == 0);
            for (int t = 0; t < 3; ++t) {
                BigInt S;
                S.d.resize((size_t)nr);
                for (mp_size_t i = 0; i < nr; ++i) S.d[i] = xorshift64(seed);
                S.d[nr - 1] |= 1ull << 63;
                S.trim();
                BigInt A0 = pow_big(S, root);
                BigInt Dfull = BigInt::mul_school(pow_big(S, root - 1), BigInt((u64)root));
                BigInt Dhalf = half ? BigInt::mul_school(pow_big(S, root - 1), BigInt((u64)(root / 2)))
                                    : BigInt();
                BigInt Atop = pow_big(BigInt::add_small(S, 1), root);
                auto run = [&](const BigInt& A) {
                    if (A.d.empty() || A.d.back() == 0) return;
                    mp_size_t na = (mp_size_t)A.d.size();
                    if (na > namax) return;
                    mp_size_t nr2 = (na + (mp_size_t)root - 1) / (mp_size_t)root;
                    big_to_limbs(A, numa, (size_t)na);
                    BigInt Ac(numa, (size_t)na);
                    for (int calr = 0; calr <= 1; ++calr) {
                        big_to_limbs(A, numa, (size_t)na);
                        lmmp_nthroot_(dst, calr ? dstr : NULL, numa, na, root, tp);
                        if (calr)
                            check_floor(Ac, dst, nr2, root, dstr);
                        else
                            check_floor(Ac, dst, nr2, root);
                    }
                };
                run(A0);
                for (u64 d : {1ull, 2ull}) {
                    run(BigInt::add_small(A0, d));
                    run(BigInt::sub_small(A0, (mp_limb_t)d));
                }
                for (u64 d : {0ull, 1ull, 2ull}) {  // 阈值上下 ±：done/inc/回退带分界
                    BigInt Dd = BigInt::add_small(Dfull, d);
                    run(BigInt::add_abs(A0, Dd));
                    if (A0 >= Dd) run(BigInt::sub_abs(A0, Dd));
                }
                if (half) {
                    run(BigInt::add_abs(A0, Dhalf));
                    if (A0 >= Dhalf) run(BigInt::sub_abs(A0, Dhalf));
                }
                for (u64 d : {1ull, 2ull, 3ull}) run(BigInt::sub_small(Atop, (mp_limb_t)d));
            }
            lmmp_free(numa);
            lmmp_free(dst);
            lmmp_free(dstr);
            lmmp_free(tp);
        }
    }
}
