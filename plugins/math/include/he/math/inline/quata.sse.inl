// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Arithmetic

    inline Quata Conjugate(const Quata& q)
    {
        constexpr Vec4a factor{ -1.0f, -1.0f, -1.0f, 1.0f };
        return _mm_mul_ps(q.v, factor);
    }

    inline Quata Mul(const Quata& a, const Quata& b)
    {
        const __m128 bx = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 by = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 bz = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 bw = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(3, 3, 3, 3));

        const __m128 cx = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(0, 1, 2, 3)); // aw, az, ay, ax
        const __m128 cy = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(1, 0, 3, 2)); // az, aw, ax, ay
        const __m128 cz = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(2, 3, 0, 1)); // ay, ax, aw, az

        const __m128 sx = _mm_setr_ps(0.0f, 0.0f, -0.0f, -0.0f);
        const __m128 sy = _mm_setr_ps(-0.0f, 0.0f, 0.0f, -0.0f);
        const __m128 sz = _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f);

        __m128 r = _mm_mul_ps(a.v, bw);
        r = MulAdd(bx, _mm_xor_ps(cx, sx), r);
        r = MulAdd(by, _mm_xor_ps(cy, sy), r);
        r = MulAdd(bz, _mm_xor_ps(cz, sz), r);
        return r;
    }

    inline Quata Mul(const Vec4a& v, const Quata& q)
    {
        const Vec4a qn = Negate(q.v);

        const __m128 swp0 = _mm_shuffle_ps(q.v, qn, _MM_SHUFFLE(0, 2, 1, 3));
        const __m128 swp1 = _mm_shuffle_ps(swp0, swp0, _MM_SHUFFLE(3, 1, 2, 0));
        const __m128 swp2 = _mm_shuffle_ps(q.v, qn, _MM_SHUFFLE(1, 0, 3, 2));
        const __m128 swp3 = _mm_shuffle_ps(q.v, qn, _MM_SHUFFLE(2, 1, 0, 3));
        const __m128 swp4 = _mm_shuffle_ps(swp3, swp3, _MM_SHUFFLE(3, 0, 1, 2));

        const __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

        const __m128 mul0 = _mm_mul_ps(vx, swp1);
        const __m128 mul1 = _mm_mul_ps(vy, swp2);
        const __m128 mul2 = _mm_mul_ps(vz, swp4);

        const __m128 r = _mm_add_ps(mul0, mul1);
        return _mm_add_ps(r, mul2);
    }

    inline Quata Mul(const Quata& q, const Vec4a& v)
    {
        const Vec4a qn = Negate(q.v);

        const __m128 swp0 = _mm_shuffle_ps(q.v, qn, _MM_SHUFFLE(0, 1, 2, 3));
        const __m128 swp1 = _mm_shuffle_ps(q.v, qn, _MM_SHUFFLE(1, 2, 3, 0));
        const __m128 swp2 = _mm_shuffle_ps(swp1, swp1, _MM_SHUFFLE(3, 0, 1, 2));
        const __m128 swp3 = _mm_shuffle_ps(q.v, qn, _MM_SHUFFLE(2, 0, 3, 1));
        const __m128 swp4 = _mm_shuffle_ps(swp3, swp3, _MM_SHUFFLE(3, 1, 2, 0));

        const __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

        const __m128 mul0 = _mm_mul_ps(vx, swp0);
        const __m128 mul1 = _mm_mul_ps(vy, swp2);
        const __m128 mul2 = _mm_mul_ps(vz, swp4);

        const __m128 r = _mm_add_ps(mul0, mul1);
        return _mm_add_ps(r, mul2);
    }
}
