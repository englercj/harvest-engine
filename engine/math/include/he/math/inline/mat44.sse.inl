// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Arithmetic

    // for column major matrix
    // we use __m128 to represent 2x2 matrix as A = | A0  A2 |
    //                                              | A1  A3 |
    // 2x2 column major Matrix multiply A*B
    HE_FORCE_INLINE __m128 _Mat22_Mul(__m128 a, __m128 b)
    {
        const __m128 a_zwxy = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), _MM_SHUFFLE(1, 0, 3, 2)));
        const __m128 b_xxww = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(b), _MM_SHUFFLE(3, 3, 0, 0)));
        const __m128 b_yyzz = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(b), _MM_SHUFFLE(2, 2, 1, 1)));

        return _mm_add_ps(_mm_mul_ps(a, a_zwxy), _mm_mul_ps(b_xxww, b_yyzz));
    }
    // 2x2 column major Matrix adjugate multiply (A#)*B
    HE_FORCE_INLINE __m128 _Mat22_AdjMul(__m128 a, __m128 b)
    {
        const __m128 a_wxwx = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), _MM_SHUFFLE(0, 3, 0, 3)));
        const __m128 a_zyzy = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), _MM_SHUFFLE(1, 2, 1, 2)));
        const __m128 b_yxwz = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(b), _MM_SHUFFLE(2, 3, 0, 1)));

        return _mm_sub_ps(_mm_mul_ps(a_wxwx, b), _mm_mul_ps(a_zyzy, b_yxwz));

    }
    // 2x2 column major Matrix multiply adjugate A*(B#)
    HE_FORCE_INLINE __m128 _Mat2_MulAdj(__m128 a, __m128 b)
    {
        const __m128 a_zwxy = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), _MM_SHUFFLE(1, 0, 3, 2)));
        const __m128 b_wwxx = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(b), _MM_SHUFFLE(0, 0, 3, 3)));
        const __m128 b_yyzz = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(b), _MM_SHUFFLE(2, 2, 1, 1)));

        return _mm_sub_ps(_mm_mul_ps(a, b_wwxx), _mm_mul_ps(a_zwxy, b_yyzz));
    }

    inline Mat44 Inverse(const Mat44& m)
    {
        // Based on: https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html

        // use block matrix method
        // A is a matrix, then i(A) or iA means inverse of A, A# (or A_ in code) means adjugate of A, |A| (or detA in code) is determinant, tr(A) is trace

        // sub matrices
        __m128 A = _mm_movelh_ps(m.cx, m.cy);
        __m128 B = _mm_movehl_ps(m.cy, m.cx);
        __m128 C = _mm_movelh_ps(m.cz, m.cw);
        __m128 D = _mm_movehl_ps(m.cw, m.cz);

        // determinant as (|A| |B| |C| |D|)
        __m128 detSub = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(m.cx, m.cz, _MM_SHUFFLE(2, 0, 2, 0)), _mm_shuffle_ps(m.cy, m.cw, _MM_SHUFFLE(3, 1, 3, 1))),
            _mm_mul_ps(_mm_shuffle_ps(m.cx, m.cz, _MM_SHUFFLE(3, 1, 3, 1)), _mm_shuffle_ps(m.cy, m.cw, _MM_SHUFFLE(2, 0, 2, 0))));

        __m128 detA = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(detSub), _MM_SHUFFLE(0, 0, 0, 0)));
        __m128 detB = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(detSub), _MM_SHUFFLE(1, 1, 1, 1)));
        __m128 detC = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(detSub), _MM_SHUFFLE(2, 2, 2, 2)));
        __m128 detD = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(detSub), _MM_SHUFFLE(3, 3, 3, 3)));

        // let iM = 1/|M| * | X  Y |
        //                  | Z  W |

        // D#C
        __m128 D_C = _Mat22_AdjMul(D, C);
        // A#B
        __m128 A_B = _Mat22_AdjMul(A, B);
        // X# = |D|A - B(D#C)
        __m128 X_ = _mm_sub_ps(_mm_mul_ps(detD, A), _Mat22_Mul(B, D_C));
        // W# = |A|D - C(A#B)
        __m128 W_ = _mm_sub_ps(_mm_mul_ps(detA, D), _Mat22_Mul(C, A_B));

        // |M| = |A|*|D| + ... (continue later)
        __m128 detM = _mm_mul_ps(detA, detD);

        // Y# = |B|C - D(A#B)#
        __m128 Y_ = _mm_sub_ps(_mm_mul_ps(detB, C), _Mat2_MulAdj(D, A_B));
        // Z# = |C|B - A(D#C)#
        __m128 Z_ = _mm_sub_ps(_mm_mul_ps(detC, B), _Mat2_MulAdj(A, D_C));

        // |M| = |A|*|D| + |B|*|C| ... (continue later)
        detM = _mm_add_ps(detM, _mm_mul_ps(detB, detC));

        // tr((A#B)(D#C))
        __m128 tr = _mm_mul_ps(A_B, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(D_C), _MM_SHUFFLE(3, 1, 2, 0))));
        tr = _mm_hadd_ps(tr, tr);
        tr = _mm_hadd_ps(tr, tr);
        // |M| = |A|*|D| + |B|*|C| - tr((A#B)(D#C)
        detM = _mm_sub_ps(detM, tr);

        const __m128 adjSignMask = _mm_setr_ps(1.f, -1.f, -1.f, 1.f);
        // (1/|M|, -1/|M|, -1/|M|, 1/|M|)
        __m128 rDetM = _mm_div_ps(adjSignMask, detM);

        X_ = _mm_mul_ps(X_, rDetM);
        Y_ = _mm_mul_ps(Y_, rDetM);
        Z_ = _mm_mul_ps(Z_, rDetM);
        W_ = _mm_mul_ps(W_, rDetM);

        // apply adjugate and store, here we combine adjugate shuffle and store shuffle
        Mat44 r;
        r.cx = _mm_shuffle_ps(X_, Y_, _MM_SHUFFLE(1, 3, 1, 3));
        r.cy = _mm_shuffle_ps(X_, Y_, _MM_SHUFFLE(0, 2, 0, 2));
        r.cz = _mm_shuffle_ps(Z_, W_, _MM_SHUFFLE(1, 3, 1, 3));
        r.cw = _mm_shuffle_ps(Z_, W_, _MM_SHUFFLE(0, 2, 0, 2));
        return r;
    }

    inline Mat44 InverseTransform(const Mat44& m)
    {
        // Based on: https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html

        Mat44 r;

        // transpose 3x3, we know m03 = m13 = m23 = 0
        const __m128 t0 = _mm_movelh_ps(m.cx, m.cy); // 00, 01, 10, 11
        const __m128 t1 = _mm_movehl_ps(m.cy, m.cx); // 02, 03, 12, 13
        r.cx = _mm_shuffle_ps(t0, m.cz, _MM_SHUFFLE(3, 0, 2, 0)); // 00, 10, 20, 23(=0)
        r.cy = _mm_shuffle_ps(t0, m.cz, _MM_SHUFFLE(3, 1, 3, 1)); // 01, 11, 21, 23(=0)
        r.cz = _mm_shuffle_ps(t1, m.cz, _MM_SHUFFLE(3, 2, 2, 0)); // 02, 12, 22, 23(=0)

        // (SizeSqr(mVec[0]), SizeSqr(mVec[1]), SizeSqr(mVec[2]), 0)
        __m128 sizeSqr;
        sizeSqr = _mm_mul_ps(r.cx, r.cx);
        sizeSqr = _mm_add_ps(sizeSqr, _mm_mul_ps(r.cy, r.cy));
        sizeSqr = _mm_add_ps(sizeSqr, _mm_mul_ps(r.cz, r.cz));

        // optional test to avoid divide by 0
        const __m128 one = _mm_set1_ps(1.f);

        // for each component, if(sizeSqr < SMALL_NUMBER) sizeSqr = 1;
        const __m128 rSizeSqr = _mm_blendv_ps(_mm_div_ps(one, sizeSqr), one, _mm_cmplt_ps(sizeSqr, _mm_set1_ps(1.e-8f)));

        r.cx = _mm_mul_ps(r.cx, rSizeSqr);
        r.cy = _mm_mul_ps(r.cy, rSizeSqr);
        r.cz = _mm_mul_ps(r.cz, rSizeSqr);

        // last line
        r.cw = _mm_mul_ps(r.cx, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.cw), _MM_SHUFFLE(0, 0, 0, 0))));
        r.cw = _mm_add_ps(r.cw, _mm_mul_ps(r.cy, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.cw), _MM_SHUFFLE(1, 1, 1, 1)))));
        r.cw = _mm_add_ps(r.cw, _mm_mul_ps(r.cz, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.cw), _MM_SHUFFLE(2, 2, 2, 2)))));
        r.cw = _mm_sub_ps(_mm_setr_ps(0.f, 0.f, 0.f, 1.f), r.cw);

        return r;
    }

    inline Mat44 InverseTransformNoScale(const Mat44& m)
    {
        // Based on: https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html

        Mat44 r;

        // transpose 3x3, we know m03 = m13 = m23 = 0
        const __m128 t0 = _mm_movelh_ps(m.cx, m.cy); // 00, 01, 10, 11
        const __m128 t1 = _mm_movehl_ps(m.cy, m.cx); // 02, 03, 12, 13
        r.cx = _mm_shuffle_ps(t0, m.cz, _MM_SHUFFLE(3, 0, 2, 0)); // 00, 10, 20, 23(=0)
        r.cy = _mm_shuffle_ps(t0, m.cz, _MM_SHUFFLE(3, 1, 3, 1)); // 01, 11, 21, 23(=0)
        r.cz = _mm_shuffle_ps(t1, m.cz, _MM_SHUFFLE(3, 2, 2, 0)); // 02, 12, 22, 23(=0)

        // last line
        r.cw = _mm_mul_ps(r.cx, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.cw), _MM_SHUFFLE(0, 0, 0, 0))));
        r.cw = _mm_add_ps(r.cw, _mm_mul_ps(r.cy, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.cw), _MM_SHUFFLE(1, 1, 1, 1)))));
        r.cw = _mm_add_ps(r.cw, _mm_mul_ps(r.cz, _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(m.cw), _MM_SHUFFLE(2, 2, 2, 2)))));
        r.cw = _mm_sub_ps(_mm_setr_ps(0.f, 0.f, 0.f, 1.f), r.cw);

        return r;
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline Mat44 Transpose(const Mat44& m)
    {
        const __m128 swp0 = _mm_shuffle_ps(m.cx, m.cy, _MM_SHUFFLE(1, 0, 1, 0));
        const __m128 swp1 = _mm_shuffle_ps(m.cx, m.cy, _MM_SHUFFLE(3, 2, 3, 2));
        const __m128 swp2 = _mm_shuffle_ps(m.cz, m.cw, _MM_SHUFFLE(1, 0, 1, 0));
        const __m128 swp3 = _mm_shuffle_ps(m.cz, m.cw, _MM_SHUFFLE(3, 2, 3, 2));

        return
        {
            _mm_shuffle_ps(swp0, swp2, _MM_SHUFFLE(2, 0, 2, 0)),
            _mm_shuffle_ps(swp0, swp2, _MM_SHUFFLE(3, 1, 3, 1)),
            _mm_shuffle_ps(swp1, swp3, _MM_SHUFFLE(2, 0, 2, 0)),
            _mm_shuffle_ps(swp1, swp3, _MM_SHUFFLE(3, 1, 3, 1)),
        };
    }

    inline Vec4a TransformVector(const Mat44& m, const Vec4a& v)
    {
        const __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 vw = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));

        __m128 r = _mm_mul_ps(m.cx, vx);
        r = MulAdd(m.cy, vy, r);
        r = MulAdd(m.cz, vz, r);
        r = MulAdd(m.cw, vw, r);
        return r;
    }

    inline Vec4a TransformVector3(const Mat44& m, const Vec4a& v)
    {
        const __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

        __m128 r = _mm_mul_ps(m.cx, vx);
        r = MulAdd(m.cy, vy, r);
        r = MulAdd(m.cz, vz, r);
        return r;
    }

    inline Vec4a TransformPoint(const Mat44& m, const Vec4a& v)
    {
        const __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

        __m128 r;
        r = MulAdd(m.cx, vx, m.cw);
        r = MulAdd(m.cy, vy, r);
        r = MulAdd(m.cz, vz, r);
        return r;
    }

    inline Vec4a ProjectPoint(const Mat44& m, const Vec4a& v)
    {
        const Vec4a t = TransformPoint(m, v);

        const __m128 tw = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 3, 3, 3));
        const __m128 inv = _mm_div_ps(_mm_set_ps1(1.0f), tw);

        return _mm_mul_ps(inv, t);
    }
}
