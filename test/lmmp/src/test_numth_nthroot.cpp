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

u64 rnd_nonzero(u64& s) {
    u64 v = xorshift64(s);
    return v ? v : 1;
}

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

// 根结果与期望值的直查比较（用于完全幂构造输入）
void check_eq(const mp_ptr got, mp_size_t gn, const BigInt& want, const char* msg) {
    BigInt bg(got, (size_t)gn);
    if (!(bg == want)) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "%s: got %zu limbs top=0x%016llx want %zu limbs top=0x%016llx", msg,
                      (size_t)gn, (unsigned long long)(gn ? got[gn - 1] : 0), want.d.size(),
                      (unsigned long long)want.d.back());
        TEST_CHECK_MSG(false, buf);
    } else
        TEST_CHECK_MSG(true, msg);
}

}  // namespace

TEST_CASE("numth/nthroot", nthroot_1_random_and_powers) {
    u64 seed = 0x9E3779B97F4A7C15ull;
    const ulong roots[] = {4, 5, 6, 8, 12, 16, 24, 31, 32, 33, 64, 100, 128, 1000};
    mp_ptr numa = alloc_limbs(1000);

    for (ulong root : roots) {
        // 随机输入（顶 limb 任意非零）
        for (int t = 0; t < 40; ++t) {
            for (ulong i = 0; i < root; ++i) numa[i] = xorshift64(seed);
            numa[root - 1] = rnd_nonzero(seed);
            mp_limb_t r = lmmp_nthroot_1_(numa, root);
            check_floor(BigInt(numa, (size_t)root), &r, 1, root);
        }
        // 完全幂边界：A=r^root / -1 / +1 的期望值分别为 r-1 / r
        for (int t = 0; t < 25; ++t) {
            // 顶 2bit 置 1 保证 r^root 恰占 root 个 limb（r >= 2^63.585
            // >= 2^(64-64/root)，覆盖 root <= 154）
            mp_limb_t r = xorshift64(seed) | 0xC000000000000000ull;
            BigInt A = pow_big(BigInt(r), root);
            if (A.d.size() != (size_t)root) continue;  // 理论恒等，防御性跳过

            big_to_limbs(A, numa, root);
            mp_limb_t got = lmmp_nthroot_1_(numa, root);
            check_eq(&got, 1, BigInt(r), "nthroot_1 exact");

            big_to_limbs(BigInt::sub_small(A, 1), numa, root);
            got = lmmp_nthroot_1_(numa, root);
            check_eq(&got, 1, BigInt(r - 1), "nthroot_1 exact-1");

            big_to_limbs(BigInt::add_small(A, 1), numa, root);
            got = lmmp_nthroot_1_(numa, root);
            check_eq(&got, 1, BigInt(r), "nthroot_1 exact+1");
        }
    }
    lmmp_free(numa);
}

TEST_CASE("numth/nthroot", nthroot_2_random_and_powers) {
    u64 seed = 0x1145141919810ull;
    const ulong roots[] = {4, 5, 7, 16, 32, 33, 64, 128, 1000};
    mp_ptr numa = alloc_limbs(2 * 1000 + 2);
    mp_ptr dst = alloc_limbs(2);

    for (ulong root : roots) {
        // 随机输入：na 在 (root, 2root] 内采样，校验根与余数
        for (int t = 0; t < 60; ++t) {
            mp_size_t na = root + 1 + (mp_size_t)(xorshift64(seed) % (ulong)root);
            std::memset(numa, 0, (size_t)(2 * root) * sizeof(mp_limb_t));
            for (mp_size_t i = 0; i < na; ++i) numa[i] = xorshift64(seed);
            numa[na - 1] = rnd_nonzero(seed);
            BigInt A(numa, (size_t)(2 * root));
            lmmp_nthroot_2_(dst, numa, na, root, 1);
            check_floor(A, dst, 2, root, numa);
        }
        // 完全幂边界（2 limb 底数）
        for (int t = 0; t < 25; ++t) {
            mp_limb_t rb[2] = {xorshift64(seed), (xorshift64(seed) | 0x8000000000000000ull)};
            BigInt r(rb, 2);
            BigInt A = pow_big(r, root);
            if (A.d.size() <= (size_t)root || A.d.size() > (size_t)2 * root) continue;
            BigInt A1 = BigInt::sub_small(A, 1);
            if (A1.d.size() <= (size_t)root) continue;  // 边界回退到 1 limb 根域

            big_to_limbs(A, numa, 2 * root);
            lmmp_nthroot_2_(dst, numa, (mp_size_t)A.d.size(), root, 1);
            check_eq(dst, 2, r, "nthroot_2 exact");

            big_to_limbs(A1, numa, 2 * root);
            lmmp_nthroot_2_(dst, numa, (mp_size_t)A1.d.size(), root, 1);
            check_eq(dst, 2, BigInt::sub_small(r, 1), "nthroot_2 exact-1");

            BigInt Ap = BigInt::add_small(A, 1);
            if (Ap.d.size() > (size_t)2 * root) continue;  // 全 1 进位边界
            big_to_limbs(Ap, numa, 2 * root);
            lmmp_nthroot_2_(dst, numa, (mp_size_t)Ap.d.size(), root, 1);
            check_eq(dst, 2, r, "nthroot_2 exact+1");
        }
    }
    lmmp_free(numa);
    lmmp_free(dst);
}

TEST_CASE("numth/nthroot", nthroot_divide_random) {
    u64 seed = 0xDEADBEEFCAFEF00Dull;
    const ulong roots[] = {4, 5, 7, 8, 64};
    const mp_size_t nss[] = {1, 2, 3, 4, 5, 6, 8, 12, 20, 32};

    for (ulong root : roots) {
        mp_limb_t minl = lmmp_nthroot_divide_min_(root);
        mp_size_t tpsize = 0;
        for (mp_size_t ns : nss) tpsize = LMMP_MAX(tpsize, lmmp_nthroot_divide_size_(ns, root));
        mp_ptr numa = alloc_limbs(64 * 32 + 2);
        mp_ptr dst = alloc_limbs(64);
        mp_ptr tp = alloc_limbs((size_t)tpsize);

        for (mp_size_t ns : nss) {
            for (int t = 0; t < 30; ++t) {
                for (mp_size_t i = 0; i < root * ns; ++i) numa[i] = xorshift64(seed);
                numa[root * ns - 1] = minl + xorshift64(seed) % (LIMB_MAX - minl);
                BigInt A(numa, (size_t)(root * ns));
                int calr = t & 1;
                lmmp_nthroot_divide_(dst, numa, ns, root, tp, calr);
                if (calr)
                    check_floor(A, dst, ns, root, numa);
                else
                    check_floor(A, dst, ns, root);
                TEST_CHECK_MSG(dst[ns - 1] != 0, "root top limb nonzero");
            }
        }
        lmmp_free(numa);
        lmmp_free(dst);
        lmmp_free(tp);
    }

    // 大 root（BigInt oracle 的 pow_big 为 O(root^2*ns^2)，迭代数收敛以控时长）
    {
        const ulong big_roots[] = {100, 1000};
        const mp_size_t nss[] = {1, 2, 3, 4, 5, 6, 8, 12, 20, 32};
        mp_size_t tpsize = 0;
        for (ulong root : big_roots)
            for (mp_size_t ns : nss) tpsize = LMMP_MAX(tpsize, lmmp_nthroot_divide_size_(ns, root));
        mp_ptr numa = alloc_limbs((size_t)(100 * 32) + 2);
        mp_ptr dst = alloc_limbs(32);
        mp_ptr tp = alloc_limbs((size_t)tpsize);

        for (ulong root : big_roots) {
            mp_limb_t minl = lmmp_nthroot_divide_min_(root);
            for (mp_size_t ns : nss) {
                if (root == 1000 && ns > 3) break;  // root=1000 内存/时长受限
                for (int t = 0; t < 4; ++t) {
                    for (mp_size_t i = 0; i < root * ns; ++i) numa[i] = xorshift64(seed);
                    numa[root * ns - 1] = minl + xorshift64(seed) % (LIMB_MAX - minl);
                    BigInt A(numa, (size_t)(root * ns));
                    int calr = t & 1;
                    lmmp_nthroot_divide_(dst, numa, ns, root, tp, calr);
                    if (calr)
                        check_floor(A, dst, ns, root, numa);
                    else
                        check_floor(A, dst, ns, root);
                    TEST_CHECK_MSG(dst[ns - 1] != 0, "root top limb nonzero");
                }
            }
        }
        lmmp_free(numa);
        lmmp_free(dst);
        lmmp_free(tp);
    }
}

TEST_CASE("numth/nthroot", nthroot_divide_exact_powers) {
    u64 seed = 0xABCDEF0123456789ull;
    const ulong roots[] = {4, 5, 7, 16, 32, 64, 100, 128};
    const mp_size_t nss[] = {1, 2, 3, 5, 9};

    for (ulong root : roots) {
        mp_limb_t minl = lmmp_nthroot_divide_min_(root);
        const mp_size_t maxns = 9;
        mp_ptr numa = alloc_limbs((size_t)(root * maxns) + 2);
        mp_ptr dst = alloc_limbs(maxns);
        mp_ptr tp = alloc_limbs((size_t)lmmp_nthroot_divide_size_(maxns, root));

        for (mp_size_t ns : nss) {
            int done = 0;
            for (int attempt = 0; attempt < 4000 && done < 10; ++attempt) {
                // 随机 r（ns limb），顶 limb 高 56bit 置 1 使 A=r^root 顶 limb
                // 恒过归一化下限（(1-2^-8)^128·B ≈ 0.61B > MIN(128) ≈ 0.49B），
                // 免去拒绝采样
                mp_ptr r = alloc_limbs(ns);
                for (mp_size_t i = 0; i < ns; ++i) r[i] = xorshift64(seed);
                r[ns - 1] = (r[ns - 1] & 0xFFull) | 0xFFFFFFFFFFFFFF00ull;
                BigInt br(r, (size_t)ns);
                lmmp_free(r);
                BigInt A = pow_big(br, root);
                if (A.d.size() != (size_t)(root * ns)) continue;
                if (A.d.back() < minl) continue;
                ++done;
                BigInt rem_m1 = BigInt::sub_abs(BigInt::sub_small(A, 1), pow_big(BigInt::sub_small(br, 1), root));

                big_to_limbs(A, numa, (size_t)(root * ns));
                lmmp_nthroot_divide_(dst, numa, ns, root, tp, 1);
                check_eq(dst, ns, br, "divide exact");
                TEST_CHECK_MSG(BigInt(numa, (size_t)((root - 1) * ns + 1)) == BigInt(0), "divide exact rem=0");

                // calr=0 补零截断快速路径（完全幂是截断正确性的边界输入；
                // numa 已被上面的调用覆写，须重灌）
                big_to_limbs(A, numa, (size_t)(root * ns));
                lmmp_nthroot_divide_(dst, numa, ns, root, tp, 0);
                check_eq(dst, ns, br, "divide exact calr0");

                big_to_limbs(BigInt::sub_small(A, 1), numa, root * ns);
                lmmp_nthroot_divide_(dst, numa, ns, root, tp, 1);
                check_eq(dst, ns, BigInt::sub_small(br, 1), "divide exact-1");
                TEST_CHECK_MSG(BigInt(numa, (size_t)((root - 1) * ns + 1)) == rem_m1, "divide exact-1 rem");

                big_to_limbs(BigInt::add_small(A, 1), numa, root * ns);
                lmmp_nthroot_divide_(dst, numa, ns, root, tp, 1);
                check_eq(dst, ns, br, "divide exact+1");
                TEST_CHECK_MSG(BigInt(numa, (size_t)((root - 1) * ns + 1)) == BigInt(1), "divide exact+1 rem=1");
            }
            TEST_CHECK_MSG(done > 0, "collected exact-power samples");
        }
        lmmp_free(numa);
        lmmp_free(dst);
        lmmp_free(tp);
    }
}

TEST_CASE("numth/nthroot", nthroot_divide_edges) {
    u64 seed = 0x5EED5EED5EED5EEDull;
    const ulong roots[] = {4, 5, 7, 16, 32, 64, 100, 128};

    for (ulong root : roots) {
        mp_limb_t minl = lmmp_nthroot_divide_min_(root);
        // MIN 解析量级：B*2^(-root/(root-1))（注意指数非整数，用 pow 而非 ldexp）
        double ref = std::pow(2.0, 64.0 - (double)root / (double)(root - 1));
        TEST_CHECK_MSG((double)minl > ref * 0.999 && (double)minl < ref * 1.001, "min_ magnitude");

        for (mp_size_t ns : {1, 2, 3}) {
            mp_ptr numa = alloc_limbs((size_t)(root * ns) + 2);
            mp_ptr dst = alloc_limbs(ns);
            mp_ptr tp = alloc_limbs((size_t)lmmp_nthroot_divide_size_(ns, root));

            // 全 MAX
            for (mp_size_t i = 0; i < root * ns; ++i) numa[i] = LIMB_MAX;
            BigInt A1(numa, (size_t)(root * ns));
            lmmp_nthroot_divide_(dst, numa, ns, root, tp, 1);
            check_floor(A1, dst, ns, root, numa);

            // 顶恰为 MIN，余 0
            std::memset(numa, 0, (size_t)(root * ns) * sizeof(mp_limb_t));
            numa[root * ns - 1] = minl;
            BigInt A2(numa, (size_t)(root * ns));
            lmmp_nthroot_divide_(dst, numa, ns, root, tp, 1);
            check_floor(A2, dst, ns, root, numa);

            // 顶恰为 MIN，低位随机（calr=0 路径）
            for (mp_size_t i = 0; i < root * ns - 1; ++i) numa[i] = xorshift64(seed);
            numa[root * ns - 1] = minl;
            BigInt A3(numa, (size_t)(root * ns));
            lmmp_nthroot_divide_(dst, numa, ns, root, tp, 0);
            check_floor(A3, dst, ns, root);

            // 顶 = MIN+1，低位全 MAX
            std::memset(numa, 0xFF, (size_t)(root * ns) * sizeof(mp_limb_t));
            numa[root * ns - 1] = minl + 1;
            BigInt A4(numa, (size_t)(root * ns));
            lmmp_nthroot_divide_(dst, numa, ns, root, tp, 1);
            check_floor(A4, dst, ns, root, numa);

            lmmp_free(numa);
            lmmp_free(dst);
            lmmp_free(tp);
        }
    }
}

/*
    边界构造：A = (S+1)^root - d（小 d）。此类输入使 frac(A^(1/root))
    逼近 1，曾暴露 calr=0 免验证捷径（nov 补零截断）的数学错误：
    floor((A*B^root)^(1/root)) = S*B + e 的 e 一般非 0，e 逼近 B-1 时
    牛顿商高估 1 即令截断进位输出 S+1（root=8/32 曾 100% 复现）。
    该捷径已删除，本用例防其以任何形式回归。
*/
/*
    顶层入口：任意长度/任意顶 limb。三档路径（直接/纯移位/一般）由输入
    形状自然覆盖：na ≡ 0 mod root 且顶过 minl → 直接；bitlen ≡ 0 mod
    root → 纯移位；其余 → 辅助常数链。
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

TEST_CASE("numth/nthroot", nthroot_divide_just_below_powers) {
    u64 seed = 0x9E3779B97F4A7C15ull;
    const ulong roots[] = {4, 8, 32, 100};
    const mp_size_t nss[] = {3, 5, 8};

    for (ulong root : roots) {
        mp_limb_t minl = lmmp_nthroot_divide_min_(root);
        for (mp_size_t ns : nss) {
            mp_ptr numa = alloc_limbs((size_t)(root * ns));
            mp_ptr dst = alloc_limbs((size_t)(ns + 1));
            mp_ptr tp = alloc_limbs((size_t)lmmp_nthroot_divide_size_(ns, root));
            for (int t = 0; t < 12; ++t) {
                BigInt S;
                S.d.resize((size_t)ns);
                for (mp_size_t i = 0; i < ns; ++i) S.d[i] = xorshift64(seed);
                S.d[ns - 1] |= 1ull << 63;
                S.trim();
                BigInt S1 = BigInt::add_small(S, 1);
                for (int d = 1; d <= 3; ++d) {
                    BigInt A = BigInt::pow(S1, root);
                    A = BigInt::sub_small(A, (mp_limb_t)d);
                    if ((mp_size_t)A.d.size() != root * ns) continue;
                    big_to_limbs(A, numa, (size_t)(root * ns));
                    if (numa[root * ns - 1] < minl) continue;
                    for (int calr = 0; calr <= 1; ++calr) {
                        big_to_limbs(A, numa, (size_t)(root * ns));
                        if (calr)
                            lmmp_nthroot_divide_(dst, numa, ns, root, tp, 1);
                        else
                            lmmp_nthroot_divide_(dst, numa, ns, root, tp, 0);
                        if (calr)
                            check_floor(A, dst, ns, root, numa);
                        else
                            check_floor(A, dst, ns, root);
                    }
                }
            }
            lmmp_free(numa);
            lmmp_free(dst);
            lmmp_free(tp);
        }
    }
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
