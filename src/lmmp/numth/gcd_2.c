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

#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"



/*
 FIXME: 内联lmmp_tailing_zeros_()函数，使用编译器内置函数，可以显著提升性能。
 尝试优化控制流，减少分支与跳转。
 */

mp_size_t lmmp_gcd_22_(mp_ptr dst, mp_srcptr up, mp_srcptr vp) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(up != NULL);
    lmmp_param_assert(vp != NULL);
    lmmp_param_assert(!(up[1] == 0 && up[0] == 0));
    lmmp_param_assert(!(vp[1] == 0 && vp[0] == 0));
    mp_limb_t u[2] = { up[0], up[1] };
    mp_limb_t v[2] = { vp[0], vp[1] };
    int k, cnt;

    if (u[1] == 0 && v[1] == 0) {
        dst[0] = lmmp_gcd_11_(u[0], v[0]);
        dst[1] = 0;
        return 1;
    } else if (u[1] == 0) {
        cnt = lmmp_tailing_zeros_(u[0] | v[0]);
        u[0] = u[0] >> lmmp_tailing_zeros_(u[0]);
        goto gcd_1_2;
    } else if (v[1] == 0) {
        cnt = lmmp_tailing_zeros_(u[0] | v[0]);
        v[0] = v[0] >> lmmp_tailing_zeros_(v[0]);
        goto gcd_2_1;
    }

    if (u[0] == 0 && v[0] == 0) {
        dst[0] = lmmp_gcd_11_(u[1], v[1]); 
        dst[1] = 0;
        return 1;
    } else if (u[0] == 0) {
        u[0] = u[1] >> lmmp_tailing_zeros_(u[1]);
        cnt = lmmp_tailing_zeros_(v[0]);
        goto gcd_1_2;
    } else if (v[0] == 0) {
        v[0] = v[1] >> lmmp_tailing_zeros_(v[1]);
        cnt = lmmp_tailing_zeros_(u[0]);
        goto gcd_2_1;
    }
    cnt = lmmp_tailing_zeros_(u[0] | v[0]);
    k = lmmp_tailing_zeros_(u[0]);

    if (k > 0)
        _u128lshr(u, u, k);
    k = lmmp_tailing_zeros_(v[0]);
    if (k > 0)
        _u128lshr(v, v, k);
    while (!(u[0] == v[0] && u[1] == v[1])) {
        if (u[1] == 0 && v[1] != 0) goto gcd_1_2;
        if (v[1] == 0 && u[1] != 0) goto gcd_2_1;
        if (u[1] == 0 && v[1] == 0) goto gcd_1_1;

        if (_u128cmp(u, v)) {
            _u128sub(v, v, u);
            if (v[0] == 0) {
                v[0] = v[1] >> lmmp_tailing_zeros_(v[1]);
                goto gcd_2_1;
            } else if (v[1] == 0) {
                v[0] = v[0] >> lmmp_tailing_zeros_(v[0]);
                goto gcd_2_1;
            }
            k = lmmp_tailing_zeros_(v[0]);
            // k > 0
            _u128lshr(v, v, k);
        } else {
            _u128sub(u, u, v);
            if (u[0] == 0) {
                u[0] = u[1] >> lmmp_tailing_zeros_(u[1]);
                goto gcd_1_2;
            } else if (u[1] == 0) {
                u[0] = u[0] >> lmmp_tailing_zeros_(u[0]);
                goto gcd_1_2;
            }
            k = lmmp_tailing_zeros_(u[0]);
            // k > 0
            _u128lshr(u, u, k);
        }
    }
    dst[0] = u[0];
    dst[1] = u[1];
    if (cnt > 0)
        _u128lshl(dst, dst, cnt);
    return 2;

    gcd_1_2:   // [u,1]  ,  [v,2]
        k = lmmp_tailing_zeros_(v[0]);
        if (k > 0)
            _u128lshr(v, v, k);
        while (v[1] != 0) {
            _u128sub64(v, v, u[0]);
            if (v[1] == 0) 
                goto gcd_1_1;
            if (v[0] == 0) {
                v[0] = v[1] >> lmmp_tailing_zeros_(v[1]);
                goto gcd_1_1;
            }
            k = lmmp_tailing_zeros_(v[0]);
            // k > 0
            _u128lshr(v, v, k);
        }
        goto gcd_1_1;

    gcd_2_1:   // [u,2]  ,  [v,1]
        k = lmmp_tailing_zeros_(u[0]);
        if (k > 0)
            _u128lshr(u, u, k);
        while (u[1] != 0) {
            _u128sub64(u, u, v[0]);
            if (u[1] == 0)
                goto gcd_1_1;
            if (u[0] == 0) {
                u[0] = u[1] >> lmmp_tailing_zeros_(u[1]);
                goto gcd_1_1;
            }
            k = lmmp_tailing_zeros_(u[0]);
            // k > 0
            _u128lshr(u, u, k);
        }
        goto gcd_1_1;

    gcd_1_1:   // [u,1]  ,  [v,1]
        while (u[0] != v[0]) {
            if (u[0] > v[0]) {
                u[0] -= v[0];
                u[0] >>= lmmp_tailing_zeros_(u[0]);
            } else {
                v[0] -= u[0];
                v[0] >>= lmmp_tailing_zeros_(v[0]);
            }
        }
        dst[0] = u[0];
        dst[1] = 0;
        if (cnt > 0)
            _u128lshl(dst, dst, cnt);
        return dst[1] == 0 ?  1  :  2;
}

mp_size_t lmmp_gcd_2_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr vp) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(up != NULL);
    lmmp_param_assert(vp != NULL);
    lmmp_param_assert(un > 2);
    lmmp_param_assert(vp[1] != 0);
    mp_limb_t u[2] = {vp[0], vp[1]};
    lmmp_mod_2_(up, un, u);
    if (u[1] == 0 && u[0] == 0) {
        dst[0] = vp[0];
        dst[1] = vp[1];
        return 2;
    } else {
        return lmmp_gcd_22_(dst, vp, u);
    }
}