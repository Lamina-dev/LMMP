/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

/*
    nthroot 家族验证（无 BigInt oracle）：
   1. floor 恒等式：r^root ≤ A < (r+1)^root，两端幂经 lmmp_pow_
      （多 limb 底 × ulong 幂，由 pow 测试套独立保证）重建；
      余数恒等式 A == r^root + rem（库加重建）
   2. 完全幂 ±1 构造（A = S^root 经库 pow 构造）命中精确语义
   3. 认证式窗口探针定向构造（回退带/阈值带：S^root、S^root±{1,2}、
      (S+1)^root-{1,2,3}、S^root+root·S^(root-1)±{1,2}）
   4. 标量档 nthroot_ulong：r^k ≤ n < (r+1)^k（u64 溢出感知重建），
      k=2/3 与 sqrt_ulong/cbrt_ulong 互比
*/

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

// 零垫对齐加法
mp_ptr pad_add(const mp_limb_t* a, mp_size_t la, const mp_limb_t* b, mp_size_t lb, mp_size_t mx,
               mp_ptr sum) {
    lmmp_zero(sum, mx);
    memcpy(sum, a, (size_t)la * 8);
    mp_ptr pad = alloc_limbs(mx);
    lmmp_zero(pad, mx);
    memcpy(pad, b, (size_t)lb * 8);
    lmmp_add_n_(sum, sum, pad, mx);
    lmmp_free(pad);
    return sum;
}

// oracle 校验：r = floor(A^(1/root)) 且（若给 rem）A == r^root + rem。
// 幂经 lmmp_pow_（多 limb 底）重建，dst/工作区由调用方提供。
void check_floor(const mp_limb_t* A, mp_size_t an, const mp_limb_t* r, mp_size_t rn, ulong root,
                 const mp_limb_t* rem, mp_ptr pw, mp_ptr pw1, mp_ptr rp1, mp_ptr sum) {
    mp_size_t cap = root * rn + root + 4;
    // r^root（pow_ 契约 [base,n]>1：r==1 时直接写 1；长度一律取返回值，
    // pow_ 试探性进位写会使缓冲高位残留垃圾）
    mp_size_t pl;
    if (rn == 1 && r[0] == 1) {
        lmmp_zero(pw, cap);
        pw[0] = 1;
        pl = 1;
    } else {
        pl = lmmp_pow_(pw, cap, r, rn, root);
    }
    // (r+1)^root
    lmmp_zero(rp1, cap);
    memcpy(rp1, r, (size_t)rn * 8);
    add_small_inplace(rp1, cap, 1);
    mp_size_t r1n = norm_len(rp1, cap);
    if (r1n == 0) r1n = 1;
    lmmp_zero(pw1, cap);
    mp_size_t p1l = lmmp_pow_(pw1, cap, rp1, r1n, root);
    TEST_CHECK_MSG(mpn_cmp(pw, pl, A, an) <= 0, "r^root <= A");
    TEST_CHECK_MSG(mpn_cmp(A, an, pw1, p1l) < 0, "A < (r+1)^root");
    if (rem) {
        pad_add(pw, pl, rem, (root - 1) * rn + 1, cap, sum);
        TEST_CHECK_MSG(mpn_eq(sum, cap, A, an), "A == r^root + rem");
    }
}

// u64 幂（溢出感知：溢出置 *ovf，值无效）
u64 pow_u64_ovf(u64 r, ulong k, bool* ovf) {
    *ovf = false;
    u64 acc = 1;
    for (ulong i = 0; i < k; ++i) {
        if (acc != 0 && r > UINT64_MAX / acc) *ovf = true;
        acc *= r;
    }
    return acc;
}

}  // namespace

/*
    顶层入口：任意长度/任意顶 limb。na 步进覆盖对齐/非对齐形状，
    root=4 行覆盖 sqrt∘sqrt 专用路线。
*/
TEST_CASE("numth/nthroot", nthroot_top_random_and_powers) {
    u64 seed = 0xF1E2D3C4B5A69788ull;
    const ulong roots[] = {4, 5, 8, 16, 32, 100};
    const mp_size_t NAMAX = 520;
    mp_size_t tpsize = 0;
    for (ulong root : roots)
        tpsize = LMMP_MAX(tpsize, lmmp_nthroot_size_(NAMAX, root));
    mp_ptr numa = alloc_limbs(NAMAX), dst = alloc_limbs(64), tp = alloc_limbs((size_t)tpsize);
    mp_ptr pw = alloc_limbs(NAMAX + 128), pw1 = alloc_limbs(NAMAX + 128),
           rp1 = alloc_limbs(NAMAX + 128), sum = alloc_limbs(NAMAX + 128);

    for (ulong root : roots) {
        for (mp_size_t na = 1; na <= 5 * (mp_size_t)root && na <= 64; na += (mp_size_t)root / 4 + 1) {
            mp_size_t nr = (na + (mp_size_t)root - 1) / (mp_size_t)root;
            mp_ptr dstr = alloc_limbs((size_t)((root - 1) * nr + 1));
            for (int t = 0; t < 6; ++t) {
                for (mp_size_t i = 0; i < na; ++i) numa[i] = xorshift64(seed);
                if (t & 1)
                    numa[na - 1] = ~(u64)0 - xorshift64(seed) % (UINT64_MAX / 4);  // 高顶
                else
                    numa[na - 1] = xorshift64(seed) % 0x10000 + 1;  // 低顶
                int calr = t & 2;
                lmmp_nthroot_(dst, calr ? dstr : NULL, numa, na, root, tp);
                check_floor(numa, na, dst, nr, root, calr ? dstr : NULL, pw, pw1, rp1, sum);
            }
            lmmp_free(dstr);
        }
        // 完全幂/±1（任意长度边界）：S 经库 pow 构造 S^root
        for (mp_size_t nr = 1; nr <= 3; ++nr) {
            mp_ptr S = alloc_limbs(nr);
            // 余数缓冲按根长 +1 档取（S^root 可恰好多一个根 limb，nr2 = nr+1）
            mp_ptr dstr = alloc_limbs((size_t)((root - 1) * (nr + 1) + 2));
            for (int t = 0; t < 3; ++t) {
                random_limbs(S, nr, seed);
                mp_size_t cap = root * nr + root / 2 + 4;
                lmmp_zero(numa, cap);
                mp_size_t na = lmmp_pow_(numa, cap, S, nr, root);  // 长度取返回值
                if (na > NAMAX) continue;
                for (int d = 0; d <= 1; ++d) {
                    if (d) sub_small_inplace(numa, na, 1);  // A = S^root - 1
                    if (numa[na - 1] == 0) continue;
                    mp_size_t nr2 = (na + (mp_size_t)root - 1) / (mp_size_t)root;
                    lmmp_nthroot_(dst, dstr, numa, na, root, tp);
                    check_floor(numa, na, dst, nr2, root, dstr, pw, pw1, rp1, sum);
                }
                // 恢复 S^root（d 循环借用了 numa），下一 t 重建
            }
            lmmp_free(S);
            lmmp_free(dstr);
        }
    }
    lmmp_free(numa); lmmp_free(dst); lmmp_free(tp);
    lmmp_free(pw); lmmp_free(pw1); lmmp_free(rp1); lmmp_free(sum);
}

/*
    认证式窗口探针的定向构造输入：根长 >= 4 limb（bitlen(y) >= 256 gate
    激活探针）下覆盖探针各判决与回退带：

    - A = S^root（完全幂）：必落不确定带 → 全幂回退，输出必须仍精确；
    - A = S^root ± {1,2}：slack 在守卫带边缘；
    - A = (S+1)^root - {1,2,3}：区间顶（e0=1 形状，dec 侧压力）；
    - A = S^root + root·S^(root-1) ± {1,2}：Bernoulli 阈值处——构造必中
      回退带，正确性必须由定理+回退承载。
*/
TEST_CASE("numth/nthroot", nthroot_top_probe_construction) {
    u64 seed = 0xA5A512349ABCDEF7ull;
    const ulong roots[] = {4, 5, 8, 16, 32, 64, 100};
    const mp_size_t NRS[] = {4, 5};

    for (ulong root : roots) {
        for (mp_size_t nr : NRS) {
            /* 统一工作容量：S^root 与 S^root+root·S^(root-1) 均不超过
               (S+1)^root < B^(root*(nr+1))，cap 含冗余 */
            mp_size_t cap = (mp_size_t)root * (nr + 1) + 8;
            mp_size_t tpsz = lmmp_nthroot_size_(cap, root);
            mp_ptr numa = alloc_limbs((size_t)cap);
            mp_ptr dst = alloc_limbs((size_t)(nr + 4));
            mp_ptr dstr = alloc_limbs((size_t)((root - 1) * (nr + 4) + 8));
            mp_ptr tp = alloc_limbs((size_t)tpsz);
            mp_ptr pw = alloc_limbs((size_t)cap),
                   pw1 = alloc_limbs((size_t)cap),
                   rp1 = alloc_limbs((size_t)cap),
                   sum = alloc_limbs((size_t)cap);
            mp_ptr Sp = alloc_limbs(nr), thr = alloc_limbs((size_t)cap);
            for (int t = 0; t < 3; ++t) {
                random_limbs(Sp, nr, seed);
                lmmp_zero(numa, cap);
                lmmp_pow_(numa, cap, Sp, nr, root);  // 恢复 S^root（d 循环借用了 numa）
                // 阈值项 root·S^(root-1)
                lmmp_zero(thr, cap);
                mp_size_t tn = lmmp_pow_(thr, cap, Sp, nr, root - 1);
                u64 hi = lmmp_mul_1_(thr, thr, tn, root);
                if (hi) thr[tn++] = hi;

                struct { int kind; long long delta; } specs[] = {
                    {0, 0}, {0, 1}, {0, -1}, {0, 2}, {0, -2},
                    {1, -1}, {1, -2}, {1, -3},  // (S+1)^root - d
                    {2, 1},  {2, -1}, {2, 2}, {2, -2},  // S^root + root·S^(root-1) ± d
                };
                for (const auto& sp : specs) {
                    mp_size_t na;
                    if (sp.kind == 1) {
                        lmmp_zero(rp1, cap);
                        memcpy(rp1, Sp, (size_t)nr * 8);
                        add_small_inplace(rp1, cap, 1);
                        mp_size_t s1n = norm_len(rp1, cap);
                        if (s1n == 0) s1n = 1;
                        lmmp_zero(numa, cap);
                        na = lmmp_pow_(numa, cap, rp1, s1n, root);
                    } else {
                        lmmp_zero(numa, cap);
                        na = lmmp_pow_(numa, cap, Sp, nr, root);
                        if (sp.kind == 2) {
                            mp_ptr t2 = alloc_limbs(cap);
                            lmmp_zero(t2, cap);
                            memcpy(t2, thr, (size_t)tn * 8);
                            lmmp_add_n_(numa, numa, t2, cap);  // numa += root·S^(root-1)
                            lmmp_free(t2);
                            na = norm_len(numa, cap);
                        }
                    }
                    if (sp.delta > 0) {
                        add_small_inplace(numa, cap, (u64)sp.delta);
                        na = norm_len(numa, cap);  // 进位可能延长（缓冲已零化）
                    } else if (sp.delta < 0) sub_small_inplace(numa, na, (u64)(-sp.delta));
                    if (numa[na - 1] == 0) continue;
                    mp_size_t nr2 = (na + (mp_size_t)root - 1) / (mp_size_t)root;
                    lmmp_nthroot_(dst, dstr, numa, na, root, tp);
                    check_floor(numa, na, dst, nr2, root, dstr, pw, pw1, rp1, sum);
                }
            }
            lmmp_free(numa); lmmp_free(dst); lmmp_free(dstr); lmmp_free(tp);
            lmmp_free(pw); lmmp_free(pw1); lmmp_free(rp1); lmmp_free(sum);
            lmmp_free(Sp); lmmp_free(thr);
        }
    }
}

TEST_CASE("numth/nthroot", nthroot_ulong_scalar) {
    u64 seed = 0x97531eca86420975ull;
    for (int i = 0; i < 1500; ++i) {
        u64 x = xorshift64(seed);
        if (x == 0) x = 1;
        for (ulong k : {2ul, 3ul, 4ul, 5ul, 7ul, 17ul, 63ul}) {
            u64 r = lmmp_nthroot_ulong_(x, k);
            if (k == 2) TEST_CHECK_EQ(r, lmmp_sqrt_ulong_(x));
            if (k == 3) TEST_CHECK_EQ(r, lmmp_cbrt_ulong_(x));
            bool o1, o2;
            u64 lo = pow_u64_ovf(r, k, &o1);
            u64 hi = pow_u64_ovf(r + 1, k, &o2);
            // r^k ≤ x（r^k 溢出即失败：x < 2^64 不可能容纳）
            TEST_CHECK_MSG(!o1 && lo <= x, "r^k <= x");
            // (r+1)^k > x（溢出即天然大于）
            TEST_CHECK_MSG(o2 || hi > x, "x < (r+1)^k");
        }
    }
    // 角点：0/1/全 1 与 root=0
    TEST_CHECK_EQ(lmmp_nthroot_ulong_(0, 5), 0u);
    TEST_CHECK_EQ(lmmp_nthroot_ulong_(1, 5), 1u);
    TEST_CHECK_EQ(lmmp_nthroot_ulong_(UINT64_MAX, 2), 0xffffffffull);
    TEST_CHECK_EQ(lmmp_nthroot_ulong_(UINT64_MAX, 3), 2642245ull);
    TEST_CHECK_EQ(lmmp_nthroot_ulong_(12345, 0), 0u);
}
