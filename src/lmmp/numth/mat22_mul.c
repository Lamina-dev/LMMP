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

#include "../../../include/lmmp/impl/signed.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/mat22_mul.h"


int lmmp_mat22_mul_size_(
          lmmp_mat22_t*  dst, 
    const lmmp_mat22_t* matA, 
    const lmmp_mat22_t* matB, 
             mp_size_t*   tn, 
             mp_size_t* maxa
) {
    lmmp_param_assert(matA!= NULL && matB!= NULL && dst!= NULL);
    lmmp_param_assert(tn != NULL);
    if (matA == matB) {
        mp_ssize_t A00 = LMMP_ABS(matA->n00);
        mp_ssize_t A01 = LMMP_ABS(matA->n01);
        mp_ssize_t A10 = LMMP_ABS(matA->n10);
        mp_ssize_t A11 = LMMP_ABS(matA->n11);
        if ((uint64_t)A00 < MAT22_SQR_STRASSEN_THRESHOLD || (uint64_t)A01 < MAT22_SQR_STRASSEN_THRESHOLD ||
            (uint64_t)A10 < MAT22_SQR_STRASSEN_THRESHOLD || (uint64_t)A11 < MAT22_SQR_STRASSEN_THRESHOLD ) {
            dst->n00 = LMMP_MAX((A00 + A00), (A01 + A10));
            dst->n01 = LMMP_MAX((A00 + A01), (A01 + A11));
            dst->n10 = LMMP_MAX((A10 + A00), (A11 + A10));
            dst->n11 = LMMP_MAX((A10 + A01), (A11 + A11));
            *tn = LMMP_MAX(LMMP_MAX(LMMP_MAX(dst->n00, dst->n01), dst->n10), dst->n11) + 1;
            ++(dst->n00);
            ++(dst->n01);
            ++(dst->n10);
            ++(dst->n11);
            return 0;
        } else {
            *maxa = LMMP_MAX(LMMP_MAX(LMMP_MAX(A00, A01), A10), A11) + 1;
            *tn = (*maxa << 1) + 1;
            dst->n00 = *tn;
            dst->n01 = *tn;
            dst->n10 = *tn;
            dst->n11 = *tn;
            return 1;
        }
    } else {
        mp_ssize_t A00 = LMMP_ABS(matA->n00);
        mp_ssize_t A01 = LMMP_ABS(matA->n01);
        mp_ssize_t A10 = LMMP_ABS(matA->n10);
        mp_ssize_t A11 = LMMP_ABS(matA->n11);
        mp_ssize_t B00 = LMMP_ABS(matB->n00);
        mp_ssize_t B01 = LMMP_ABS(matB->n01);
        mp_ssize_t B10 = LMMP_ABS(matB->n10);
        mp_ssize_t B11 = LMMP_ABS(matB->n11);
        if ((uint64_t)A00 < MAT22_MUL_STRASSEN_THRESHOLD || (uint64_t)A01 < MAT22_MUL_STRASSEN_THRESHOLD ||
            (uint64_t)A10 < MAT22_MUL_STRASSEN_THRESHOLD || (uint64_t)A11 < MAT22_MUL_STRASSEN_THRESHOLD ||
            (uint64_t)B00 < MAT22_MUL_STRASSEN_THRESHOLD || (uint64_t)B01 < MAT22_MUL_STRASSEN_THRESHOLD ||
            (uint64_t)B10 < MAT22_MUL_STRASSEN_THRESHOLD || (uint64_t)B11 < MAT22_MUL_STRASSEN_THRESHOLD) {
            dst->n00 = LMMP_MAX((A00 + B00), (A01 + B10));
            dst->n01 = LMMP_MAX((A00 + B01), (A01 + B11));
            dst->n10 = LMMP_MAX((A10 + B00), (A11 + B10));
            dst->n11 = LMMP_MAX((A10 + B01), (A11 + B11));
            *tn = LMMP_MAX(LMMP_MAX(LMMP_MAX(dst->n00, dst->n01), dst->n10), dst->n11);
            ++(dst->n00);
            ++(dst->n01);
            ++(dst->n10);
            ++(dst->n11);
            return 0;
        } else {
            *maxa = LMMP_MAX(LMMP_MAX(LMMP_MAX(A00, A01), A10), A11) + 1;
            *tn = *maxa + LMMP_MAX(LMMP_MAX(LMMP_MAX(B00, B01), B10), B11) + 1;
            dst->n00 = *tn;
            dst->n01 = *tn;
            dst->n10 = *tn;
            dst->n11 = *tn;
            return 1;
        }
    }
}

void lmmp_mat22_mul_basecase_(
          lmmp_mat22_t*  dst,
    const lmmp_mat22_t* matA,
    const lmmp_mat22_t* matB,
                mp_ptr    tp,
             mp_size_t    tn
) {
    lmmp_param_assert(matA != NULL && matB != NULL && dst != NULL);
    lmmp_param_assert(tn > 0);
    if (matA == matB) {
        lmmp_mat22_sqr_basecase_(dst, matA, tp, tn);
        return;
    }
    TEMP_DECL;
    if (tp == NULL)
        tp = TALLOC_TYPE(tn * 2, mp_limb_t);
#define p1 tp
#define p2 tp + tn
    mp_ssize_t pn1, pn2;
    pn1 = lmmp_mul_signed_(p1, matA->a00, matA->n00, matB->a00, matB->n00);
    pn2 = lmmp_mul_signed_(p2, matA->a01, matA->n01, matB->a10, matB->n10);
    dst->n00 = lmmp_add_signed_(dst->a00, p1, pn1, p2, pn2);
    pn1 = lmmp_mul_signed_(p1, matA->a00, matA->n00, matB->a01, matB->n01);
    pn2 = lmmp_mul_signed_(p2, matA->a01, matA->n01, matB->a11, matB->n11);
    dst->n01 = lmmp_add_signed_(dst->a01, p1, pn1, p2, pn2);
    pn1 = lmmp_mul_signed_(p1, matA->a10, matA->n10, matB->a00, matB->n00);
    pn2 = lmmp_mul_signed_(p2, matA->a11, matA->n11, matB->a10, matB->n10);
    dst->n10 = lmmp_add_signed_(dst->a10, p1, pn1, p2, pn2);
    pn1 = lmmp_mul_signed_(p1, matA->a10, matA->n10, matB->a01, matB->n01);
    pn2 = lmmp_mul_signed_(p2, matA->a11, matA->n11, matB->a11, matB->n11);
    dst->n11 = lmmp_add_signed_(dst->a11, p1, pn1, p2, pn2);
#undef p1
#undef p2
    TEMP_FREE;
}

void lmmp_mat22_sqr_basecase_(
          lmmp_mat22_t*  dst,
    const lmmp_mat22_t* matA,
                mp_ptr    tp,
             mp_size_t    tn
) {
    TEMP_DECL;
    if (tp == NULL)
        tp = TALLOC_TYPE(tn * 2, mp_limb_t);
#define p1 tp
#define p2 tp + tn
    mp_ssize_t pn1, pn2;
    pn1 = lmmp_sqr_signed_(p1, matA->a00, matA->n00);
    pn2 = lmmp_mul_signed_(p2, matA->a01, matA->n01, matA->a10, matA->n10);
    dst->n00 = lmmp_add_signed_(dst->a00, p1, pn1, p2, pn2);
    pn1 = lmmp_mul_signed_(p1, matA->a00, matA->n00, matA->a01, matA->n01);
    pn2 = lmmp_mul_signed_(p2, matA->a01, matA->n01, matA->a11, matA->n11);
    dst->n01 = lmmp_add_signed_(dst->a01, p1, pn1, p2, pn2);
    pn1 = lmmp_mul_signed_(p1, matA->a10, matA->n10, matA->a00, matA->n00);
    pn2 = lmmp_mul_signed_(p2, matA->a11, matA->n11, matA->a10, matA->n10);
    dst->n10 = lmmp_add_signed_(dst->a10, p1, pn1, p2, pn2);
    pn1 = lmmp_mul_signed_(p1, matA->a10, matA->n10, matA->a01, matA->n01);
    pn2 = lmmp_sqr_signed_(p2, matA->a11, matA->n11);
    dst->n11 = lmmp_add_signed_(dst->a11, p1, pn1, p2, pn2);
#undef p1
#undef p2
    TEMP_FREE;
}

/*
 * Strassen 2x2 矩阵乘法的 Winograd 变体
 *
 * 输入矩阵：
 *   A = | A11  A12 |
 *       | A21  A22 |
 *   B = | B11  B12 |
 *       | B21  B22 |
 *
 * 输出矩阵 C = A * B：
 *   C = | C11  C12 |
 *       | C21  C22 |
 *
 *
 *   s1 = A22 + A12
 *   s2 = A22 - A21
 *   s3 = s2  + A12 = A22 - A21 + A12
 *   s4 = s3  - A11 = A22 - A21 + A12 - A11
 *
 *   t1 = B22 + B12
 *   t2 = B22 - B21
 *   t3 = t2  + B12 = B22 - B21 + B12
 *   t4 = t3  - B11 = B22 - B21 + B12 - B11
 *
 * 7 个 Strassen 乘积项
 *   p1 = s1  * t1   = (A22 + A12      ) * (B22 + B12      )
 *   p2 = s2  * t2   = (A22 - A21      ) * (B22 - B21      )
 *   p3 = s3  * t3   = (A22 - A21 + A12) * (B22 - B21 + B12)
 *   p4 = A11 * B11
 *   p5 = A12 * B21
 *   p6 = s4  * B12
 *   p7 = A21 * t4
 *
 *   U1 = p3 + p5
 *   U2 = p1 - U1
 *   U3 = U1 - p2
 *
 * result:
 *   C11 = p4 + p5
 *   C12 = U3 - p6
 *   C21 = U2 - p7
 *   C22 = p2 + U2
 *
 * 平方版本（A*A）：所有乘法替换为平方/自身相乘，流程一致。
 */

void lmmp_mat22_mul_strassen_(
          lmmp_mat22_t*  dst, 
    const lmmp_mat22_t* matA, 
    const lmmp_mat22_t* matB, 
                mp_ptr    tp, 
             mp_size_t    tn, 
             mp_size_t  maxa
) {
    lmmp_param_assert(matA != NULL && matB != NULL && dst != NULL);
    lmmp_param_assert(tn > 0 && maxa > 0);
    if (matA == matB) {
        lmmp_mat22_sqr_strassen_(dst, matA, tp, tn);
        return;
    }
    TEMP_B_DECL;
    ++tn;
    if (tp == NULL)
        tp = BALLOC_TYPE(tn * 7, mp_limb_t);

#define A11 (matA->a00) 
#define A12 (matA->a01)
#define A21 (matA->a10)
#define A22 (matA->a11)
#define B11 (matB->a00)
#define B12 (matB->a01)
#define B21 (matB->a10)
#define B22 (matB->a11)
#define A11n (matA->n00)
#define A12n (matA->n01)
#define A21n (matA->n10)
#define A22n (matA->n11)
#define B11n (matB->n00)
#define B12n (matB->n01)
#define B21n (matB->n10)
#define B22n (matB->n11)

#define s1 (dst->a00)
#define s2 (dst->a01)
#define s3 (dst->a10)
#define s4 (dst->a11)
#define t1 (dst->a00 + maxa)
#define t2 (dst->a01 + maxa)
#define t3 (dst->a10 + maxa)
#define t4 (dst->a11 + maxa)
#define p1 (tp)
#define p2 (tp + tn)
#define p3 (tp + 2 * tn)
#define p4 (tp + 3 * tn)
#define p5 (tp + 4 * tn)
#define p6 (tp + 5 * tn)
#define p7 (tp + 6 * tn)
    mp_ssize_t n1, n2, n3, n4, n5, n6, n7, n8;
    n1 = lmmp_add_signed_(s1, A22, A22n, A12, A12n);
    n2 = lmmp_add_signed_(s2, A22, A22n, A21, -A21n);
    n3 = lmmp_add_signed_(s3, s2, n2, A12, A12n);
    n4 = lmmp_add_signed_(s4, s3, n3, A11, -A11n);
    n5 = lmmp_add_signed_(t1, B22, B22n, B12, B12n);
    n6 = lmmp_add_signed_(t2, B22, B22n, B21, -B21n);
    n7 = lmmp_add_signed_(t3, t2, n6, B12, B12n);
    n8 = lmmp_add_signed_(t4, t3, n7, B11, -B11n);

    n1 = lmmp_mul_signed_(p1, s1, n1, t1, n5);
    n5 = lmmp_mul_signed_(p2, s2, n2, t2, n6);
    n2 = lmmp_mul_signed_(p3, s3, n3, t3, n7);
    n7 = lmmp_mul_signed_(p4, A11, A11n, B11, B11n);
    n6 = lmmp_mul_signed_(p5, A12, A12n, B21, B21n);
    n3 = lmmp_mul_signed_(p6, s4, n4, B12, B12n);
    n4 = lmmp_mul_signed_(p7, A21, A21n, t4, n8);

#undef s1
#undef s2
#undef s3
#undef s4
#undef t1
#undef t2
#undef t3
#undef t4

#define p1n n1
#define p2n n5
#define p3n n2
#define p4n n7
#define p5n n6
#define p6n n3
#define p7n n4

#undef A11
#undef A12
#undef A21
#undef A22
#undef B11
#undef B12
#undef B21
#undef B22
#undef A11n
#undef A12n
#undef A21n
#undef A22n
#undef B11n
#undef B12n
#undef B21n
#undef B22n

#define C11 (dst->a00)
#define C12 (dst->a01)
#define C21 (dst->a10)
#define C22 (dst->a11)
#define C11n (dst->n00)
#define C12n (dst->n01)
#define C21n (dst->n10)
#define C22n (dst->n11)

    C11n = lmmp_add_signed_(C11, p4, p4n, p5, p5n);
#define U1 p5 // U1 = p3 + p5
#define U2 p1 // U2 = p1 - U1
#define U3 U1 // U3 = U1 - p2
#define U1n p5n
#define U2n p1n
#define U3n n8
    U1n = lmmp_add_signed_(U1, p3, p3n, p5, p5n);
    U2n = lmmp_add_signed_(U2, p1, p1n, U1, -U1n);
    U3n = lmmp_add_signed_(U3, U1, U1n, p2, -p2n);

    C12n = lmmp_add_signed_(C12, U3, U3n, p6, -p6n);
    C21n = lmmp_add_signed_(C21, U2, U2n, p7, -p7n);
    C22n = lmmp_add_signed_(C22, p2, p2n, U2, U2n);
    TEMP_B_FREE;

#undef C11
#undef C12
#undef C21
#undef C22
#undef C11n
#undef C12n
#undef C21n
#undef C22n
#undef U1
#undef U2
#undef U3
#undef U1n
#undef U2n
#undef U3n
  
#undef p1    
#undef p2    
#undef p3    
#undef p4    
#undef p5    
#undef p6    
#undef p7    
}

void lmmp_mat22_sqr_strassen_(lmmp_mat22_t* dst, const lmmp_mat22_t* mat, mp_ptr tp, mp_size_t tn) {
    lmmp_param_assert(mat != NULL && dst != NULL);
    TEMP_B_DECL;
    ++tn;
    if (tp == NULL)
        tp = BALLOC_TYPE(tn * 7, mp_limb_t);

#define A11 (mat->a00)
#define A12 (mat->a01)
#define A21 (mat->a10)
#define A22 (mat->a11)
#define A11n (mat->n00)
#define A12n (mat->n01)
#define A21n (mat->n10)
#define A22n (mat->n11)

#define s1 (dst->a00)
#define s2 (dst->a01)
#define s3 (dst->a10)
#define s4 (dst->a11)
#define p1 (tp)
#define p2 (tp + tn)
#define p3 (tp + 2 * tn)
#define p4 (tp + 3 * tn)
#define p5 (tp + 4 * tn)
#define p6 (tp + 5 * tn)
#define p7 (tp + 6 * tn)
    mp_ssize_t n1, n2, n3, n4, n5, n6, n7, n8;
    n1 = lmmp_add_signed_(s1, A22, A22n, A12, A12n);
    n2 = lmmp_add_signed_(s2, A22, A22n, A21, -A21n);
    n3 = lmmp_add_signed_(s3, s2, n2, A12, A12n);
    n4 = lmmp_add_signed_(s4, s3, n3, A11, -A11n);

    n1 = lmmp_sqr_signed_(p1, s1, n1);
    n5 = lmmp_sqr_signed_(p2, s2, n2);
    n2 = lmmp_sqr_signed_(p3, s3, n3);
    n7 = lmmp_sqr_signed_(p4, A11, A11n);
    n6 = lmmp_mul_signed_(p5, A12, A12n, A21, A21n);
    n3 = lmmp_mul_signed_(p6, s4, n4, A12, A12n);
    n4 = lmmp_mul_signed_(p7, A21, A21n, s4, n4);

#undef s1
#undef s2
#undef s3
#undef s4

#define p1n n1
#define p2n n5
#define p3n n2
#define p4n n7
#define p5n n6
#define p6n n3
#define p7n n4

#undef A11
#undef A12
#undef A21
#undef A22
#undef A11n
#undef A12n
#undef A21n
#undef A22n

#define C11 (dst->a00)
#define C12 (dst->a01)
#define C21 (dst->a10)
#define C22 (dst->a11)
#define C11n (dst->n00)
#define C12n (dst->n01)
#define C21n (dst->n10)
#define C22n (dst->n11)

    C11n = lmmp_add_signed_(C11, p4, p4n, p5, p5n);
#define U1 p5  // U1 = p3 + p5
#define U2 p1  // U2 = p1 - U1
#define U3 U1  // U3 = U1 - p2
#define U1n p5n
#define U2n p1n
#define U3n n8
    U1n = lmmp_add_signed_(U1, p3, p3n, p5, p5n);
    U2n = lmmp_add_signed_(U2, p1, p1n, U1, -U1n);
    U3n = lmmp_add_signed_(U3, U1, U1n, p2, -p2n);

    C12n = lmmp_add_signed_(C12, U3, U3n, p6, -p6n);
    C21n = lmmp_add_signed_(C21, U2, U2n, p7, -p7n);
    C22n = lmmp_add_signed_(C22, p2, p2n, U2, U2n);
    TEMP_B_FREE;

#undef C11
#undef C12
#undef C21
#undef C22
#undef C11n
#undef C12n
#undef C21n
#undef C22n
#undef U1
#undef U2
#undef U3
#undef U1n
#undef U2n
#undef U3n

#undef p1
#undef p2
#undef p3
#undef p4
#undef p5
#undef p6
#undef p7
}
