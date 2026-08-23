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

#include "../../../../include/lmmp/impl/longlong.h"
#include "../../../../include/lmmp/impl/inlines.h"
#include "../../../../include/lmmp/lmmpn.h"


mp_limb_t lmmp_div_3_2_(mp_ptr restrict numa, mp_srcptr restrict numb, mp_limb_t inv21) {
    mp_limb_t q;
    mp_limb_t r1, r0;
    mp_limb_t a[3] = {numa[0], numa[1], numa[2]};
    _udiv_qr_3by2(q, r1, r0, a[2], a[1], a[0], numb[1], numb[0], inv21);
    numa[1] = r1;
    numa[0] = r0;
    return q;
}

mp_limb_t lmmp_mod_1_(mp_srcptr numa, mp_size_t na, mp_limb_t x) {
    mp_limb_t ah, al;
    if (na == 1) {
        ah = numa[0];
        return ah % x;
    }
    // q: assigned for macro reuse, unused in this logic (known warning)
    mp_limb_t t = numa[na - 2], q = 0, r = 0;
    const int shift = lmmp_leading_zeros_(x);
    if (shift > 0) {
        const int rshift = LIMB_BITS - shift;
        ah = numa[na - 1] >> rshift;
        t = numa[na - 2];
        al = (numa[na - 1] << shift) | (t >> rshift);
        x <<= shift;
        const mp_limb_t inv = lmmp_inv_1_(x);
        _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
        na -= 2;
        while (na-- > 0) {
            ah = r;
            al = t << shift;
            t = numa[na];
            al |= t >> rshift;
            _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
        }
        ah = r;
        al = t << shift;
        _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
        return r >> shift;
    } else {
        ah = 0;
        t = numa[na - 2];
        al = numa[na - 1];
        const mp_limb_t inv = lmmp_inv_1_(x);
        q = al / x;
        r = al % x;
        na -= 2;
        while (na-- > 0) {
            ah = r;
            al = t;
            t = numa[na];
            _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
        }
        ah = r;
        al = t;
        _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
        return r;
    }
}

mp_limb_t lmmp_div_1_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_limb_t x) {
    mp_limb_t ah, al;
    if (na == 1) {
        ah = numa[0];
        if (dstq)
            dstq[0] = ah / x;
        return ah % x;
    }
    if (dstq) {
        mp_limb_t t = numa[na - 2], q = 0, r = 0;
        const int shift = lmmp_leading_zeros_(x);
        if (shift > 0) {
            /*
              ah    al
               X|XXXtttX|XXXmmmX|XXXnnnX|XXX----|
                |000XXXX|tttXXXX|mmmXXXX|nnnXXXX|
                            t    numa[na]

                     ah    al
               X|XXXtttX|XXXmmmX|XXXnnnX|XXX----|
                |000XXXX|tttXXXX|mmmXXXX|nnnXXXX|
                                    t
                                 numa[na]
            */
            const int rshift = LIMB_BITS - shift;
            ah = numa[na - 1] >> rshift;
            t = numa[na - 2];
            al = (numa[na - 1] << shift) | (t >> rshift);
            x <<= shift;
            const mp_limb_t inv = lmmp_inv_1_(x);
            _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
            dstq[na - 1] = q;
            na -= 2;
            while (na-- > 0) {
                ah = r;
                al = t << shift;
                t = numa[na];
                al |= t >> rshift;
                _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
                dstq[na + 1] = q;
            }
            ah = r;
            al = t << shift;
            _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
            dstq[0] = q;
            return r >> shift;
        } else {
            /*
            ah     al
                |000XXXX|tttXXXX|mmmXXXX|nnnXXXX|
                            t    numa[na]
            */
            ah = 0;
            t = numa[na - 2];
            al = numa[na - 1];
            const mp_limb_t inv = lmmp_inv_1_(x);
            q = al / x;
            r = al % x;
            dstq[na - 1] = q;
            na -= 2;
            while (na-- > 0) {
                ah = r;
                al = t;
                t = numa[na];
                _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
                dstq[na + 1] = q;
            }
            ah = r;
            al = t;
            _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
            dstq[0] = q;
            return r;
        }
    } else {
        return lmmp_mod_1_(numa, na, x);
    }
}

void lmmp_mod_2_(mp_srcptr numa, mp_size_t na, mp_ptr numb) {
    mp_limb_t q, r1, r0, a2, a1, a0, b1, b0;
    b1 = numb[1];
    b0 = numb[0];
    if (na == 2) {
        int shift = lmmp_leading_zeros_(b1);
        if (shift > 0) {
            const int rshift = LIMB_BITS - shift;
            b1 = (b1 << shift) | (b0 >> rshift);
            b0 <<= shift;
            a2 = numa[1] >> rshift;
            a1 = (numa[1] << shift) | (numa[0] >> rshift);
            a0 = (numa[0] << shift);
            mp_limb_t inv = lmmp_inv_2_1_(b1, b0);
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            numb[0] = (r0 >> shift) | (r1 << rshift);
            numb[1] = r1 >> shift;
            return;
        } else {
            if (_u128cmp(numa, numb)) {
                numb[0] = numa[0];
                numb[1] = numa[1];
                return;
            } else {
                _u128sub(numb, numa, numb);
                return;
            }
        }
    } else {
        int shift = lmmp_leading_zeros_(b1);
        if (shift > 0) {
            const int rshift = LIMB_BITS - shift;
            b1 = (b1 << shift) | (b0 >> rshift);
            b0 <<= shift;
            const mp_limb_t inv = lmmp_inv_2_1_(b1, b0);
            a2 = numa[na - 1] >> rshift;
            a1 = (numa[na - 1] << shift) | (numa[na - 2] >> rshift);
            a0 = (numa[na - 2] << shift) | (numa[na - 3] >> rshift);
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            na -= 2;
            while (na-- > 1) {
                a2 = r1;
                a1 = r0;
                a0 = (numa[na] << shift) | (numa[na - 1] >> rshift);
                _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            }

            a2 = r1;
            a1 = r0;
            a0 = (numa[na] << shift);
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            numb[0] = (r0 >> shift) | (r1 << rshift);
            numb[1] = r1 >> shift;
            return;
        } else {
            const mp_limb_t inv = lmmp_inv_2_1_(b1, b0);
            a2 = 0;
            a1 = numa[na - 1];
            a0 = numa[na - 2];
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            na -= 2;
            while (na-- > 1) {
                a2 = r1;
                a1 = r0;
                a0 = numa[na];
                _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            }
            a2 = r1;
            a1 = r0;
            a0 = numa[na];
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            numb[0] = r0;
            numb[1] = r1;
            return;
        }
    }
}


void lmmp_div_2_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_ptr numb) {
    mp_limb_t q, r1, r0, a2, a1, a0, b1, b0;
    b1 = numb[1];
    b0 = numb[0];
    if (na == 2) {
        int shift = lmmp_leading_zeros_(b1);
        if (shift > 0) {
            const int rshift = LIMB_BITS - shift;
            b1 = (b1 << shift) | (b0 >> rshift);
            b0 <<= shift;
            a2 = numa[1] >> rshift;
            a1 = (numa[1] << shift) | (numa[0] >> rshift);
            a0 = (numa[0] << shift);
            mp_limb_t inv = lmmp_inv_2_1_(b1, b0);
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            if (dstq)
                dstq[0] = q;
            numb[0] = (r0 >> shift) | (r1 << rshift);
            numb[1] = r1 >> shift;
            return;
        } else {
            if (_u128cmp(numa, numb)) {
                numb[0] = numa[0];
                numb[1] = numa[1];
                if (dstq)
                    dstq[0] = 0;
                return;
            } else {
                _u128sub(numb, numa, numb);
                if (dstq)
                    dstq[0] = 1;
                return;
            }
        }
    }
    if (dstq) {
        int shift = lmmp_leading_zeros_(b1);
        if (shift > 0) {
            /*
              a2    a1    a0
               X|XXXtttX|XXXmmmX|XXXnnnX|XXX----|
                |000XXXX|tttXXXX|mmmXXXX|nnnXXXX|
                                 numa[na]
            */
            const int rshift = LIMB_BITS - shift;
            b1 = (b1 << shift) | (b0 >> rshift);
            b0 <<= shift;
            const mp_limb_t inv = lmmp_inv_2_1_(b1, b0);
            a2 = numa[na - 1] >> rshift;
            a1 = (numa[na - 1] << shift) | (numa[na - 2] >> rshift);
            a0 = (numa[na - 2] << shift) | (numa[na - 3] >> rshift);
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            dstq[na - 2] = q;
            na -= 2;
            while (na-- > 1) {
                a2 = r1;
                a1 = r0;
                a0 = (numa[na] << shift) | (numa[na - 1] >> rshift);
                _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
                dstq[na] = q;
            }

            a2 = r1;
            a1 = r0;
            a0 = (numa[na] << shift);
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            dstq[0] = q;
            numb[0] = (r0 >> shift) | (r1 << rshift);
            numb[1] = r1 >> shift;
            return;
        } else {
            /*
              a2    a1    a0
                |000XXXX|tttXXXX|mmmXXXX|nnnXXXX|
                                 numa[na]
            */
            const mp_limb_t inv = lmmp_inv_2_1_(b1, b0);
            a2 = 0;
            a1 = numa[na - 1];
            a0 = numa[na - 2];
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            dstq[na - 2] = q;
            na -= 2;
            while (na-- > 1) {
                a2 = r1;
                a1 = r0;
                a0 = numa[na];
                _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
                dstq[na] = q;
            }
            a2 = r1;
            a1 = r0;
            a0 = numa[na];
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            dstq[0] = q;
            numb[0] = r0;
            numb[1] = r1;
            return;
        }
    } else {
        int shift = lmmp_leading_zeros_(b1);
        if (shift > 0) {
            const int rshift = LIMB_BITS - shift;
            b1 = (b1 << shift) | (b0 >> rshift);
            b0 <<= shift;
            const mp_limb_t inv = lmmp_inv_2_1_(b1, b0);
            a2 = numa[na - 1] >> rshift;
            a1 = (numa[na - 1] << shift) | (numa[na - 2] >> rshift);
            a0 = (numa[na - 2] << shift) | (numa[na - 3] >> rshift);
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            na -= 2;
            while (na-- > 1) {
                a2 = r1;
                a1 = r0;
                a0 = (numa[na] << shift) | (numa[na - 1] >> rshift);
                _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            }

            a2 = r1;
            a1 = r0;
            a0 = (numa[na] << shift);
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            numb[0] = (r0 >> shift) | (r1 << rshift);
            numb[1] = r1 >> shift;
            return;
        } else {
            const mp_limb_t inv = lmmp_inv_2_1_(b1, b0);
            a2 = 0;
            a1 = numa[na - 1];
            a0 = numa[na - 2];
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            na -= 2;
            while (na-- > 1) {
                a2 = r1;
                a1 = r0;
                a0 = numa[na];
                _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            }
            a2 = r1;
            a1 = r0;
            a0 = numa[na];
            _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
            numb[0] = r0;
            numb[1] = r1;
            return;
        }
    }
}

mp_limb_t lmmp_div_1_s_(mp_ptr restrict dstq, mp_ptr restrict numa, mp_size_t na, mp_limb_t x) {
    mp_limb_t ah, al;
    mp_limb_t t = numa[na - 2], q = 0, r = 0;
    /*
    ah     al
        |000XXXX|tttXXXX|mmmXXXX|nnnXXXX|
                    t    numa[na]
    */
    ah = 0;
    t = numa[na - 2];
    al = numa[na - 1];
    const mp_limb_t inv = lmmp_inv_1_(x);
    q = al / x;
    r = al % x;
    const mp_limb_t qh = q;
    na -= 2;
    while (na-- > 0) {
        ah = r;
        al = t;
        t = numa[na];
        _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
        dstq[na + 1] = q;
    }
    ah = r;
    al = t;
    _udiv_qrnnd_preinv(q, r, ah, al, x, inv);
    dstq[0] = q;
    numa[0] = r;
    return qh;
}

mp_limb_t lmmp_div_2_s_(mp_ptr restrict dstq, mp_ptr restrict numa, mp_size_t na, mp_srcptr restrict numb) {
    mp_limb_t q, r1, r0, a2, a1, a0, b1, b0;
    b1 = numb[1];
    b0 = numb[0];
    /*
      a2    a1    a0
        |000XXXX|tttXXXX|mmmXXXX|nnnXXXX|
                         numa[na]
    */
    const mp_limb_t inv = lmmp_inv_2_1_(b1, b0);
    a2 = 0;
    a1 = numa[na - 1];
    a0 = numa[na - 2];
    _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
    const mp_limb_t qh = q;
    na -= 2;
    while (na-- > 1) {
        a2 = r1;
        a1 = r0;
        a0 = numa[na];
        _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
        dstq[na] = q;
    }
    a2 = r1;
    a1 = r0;
    a0 = numa[na];
    _udiv_qr_3by2(q, r1, r0, a2, a1, a0, b1, b0, inv);
    dstq[0] = q;
    numa[0] = r0;
    numa[1] = r1;
    return qh;
}
