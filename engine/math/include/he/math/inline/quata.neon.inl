// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Arithmetic

    inline Quata Conjugate(const Quata& q)
    {
        constexpr Vec4a factor{ -1.0f, -1.0f, -1.0f, 1.0f };
        return vmulq_f32(q.v, factor);
    }

    inline Quata Mul(const Quata& a, const Quata& b)
    {
        const uint32x4_t p = vdupq_n_u32(0);
        const uint32x4_t n = vdupq_n_u32(0x80000000);

        const uint32x4_t sx = vextq_u32(p, n, 2);   // p2, p3, n0, n1
        const uint32x4_t sy = vextq_u32(sx, sx, 3); // n1, p2, p3, n0
    #if HE_CPU_ARM_64
        const uint32x4_t sz = vzip1q_u32(p, n);     // p0, n0, p1, n1
    #else
        const uint32x4_t sz = vcombine_u32(
            vzip_u32(vget_low_f32(p), vget_low_f32(n)).val[0],
            vzip_u32(vget_low_f32(p), vget_low_f32(n)).val[1]); // p0, n0, p1, n1
    #endif

        const float32x4_t cz = vrev64q_f32(a.v);        // ay, ax, aw, az
        const float32x4_t cx = vextq_f32(cz, cz, 2);    // aw, az, ay, ax
        const float32x4_t cy = vextq_f32(a.v, a.v, 2);  // az, aw, ax, ay

        const float32x4_t csx = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(cx), sx));
        const float32x4_t csy = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(cy), sy));
        const float32x4_t csz = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(cz), sz));

        float32x4_t q;
    #if HE_CPU_ARM_64
        q = vmulq_laneq_f32(a.v, b.v, 3);
        q = vfmaq_laneq_f32(q, csx, b.v, 0);
        q = vfmaq_laneq_f32(q, csy, b.v, 1);
        q = vfmaq_laneq_f32(q, csz, b.v, 2);
    #else
        const float32x4_t bx = vdupq_n_f32(vgetq_lane_f32(b.v, 0));
        const float32x4_t by = vdupq_n_f32(vgetq_lane_f32(b.v, 1));
        const float32x4_t bz = vdupq_n_f32(vgetq_lane_f32(b.v, 2));
        const float32x4_t bw = vdupq_n_f32(vgetq_lane_f32(b.v, 3));

        q = vmulq_f32(a.v, bw);
        q = vmlaq_f32(q, csx, bx);
        q = vmlaq_f32(q, csy, by);
        q = vmlaq_f32(q, csz, bz);
    #endif
        return q;
    }

    inline Quata Mul(const Vec4a& v, const Quata& q)
    {
        const Vec4a qn = Negate(q.v);

        const Vec4a vx = SplatX(v);
        const Vec4a vy = SplatY(v);
        const Vec4a vz = SplatZ(v);

        const float32x4_t swp0 = vcombine_f32(
            vzip_f32(vrev64_f32(vget_high_f32(q.v)), vget_high_f32(qn)).val[0],
            vzip_f32(vrev64_f32(vget_low_f32(q.v)), vget_low_f32(qn)).val[0]);

        const float32x4_t swp1 = vcombine_f32(
            vget_high_f32(q.v),
            vget_low_f32(qn));

        const float32x4_t swp2 = vcombine_f32(
            vzip_f32(vrev64_f32(vget_low_f32(qn)), vget_low_f32(q.v)).val[0],
            vzip_f32(vrev64_f32(vget_high_f32(q.v)), vget_high_f32(qn)).val[0]);

        const float32x4_t mul0 = vmulq_f32(vx, swp0);
        const float32x4_t mul1 = vmulq_f32(vy, swp1);
        const float32x4_t mul2 = vmulq_f32(vz, swp2);

        const float32x4_t r = vaddq_f32(mul0, mul1);
        return vaddq_f32(r, mul2);
    }

    inline Quata Mul(const Quata& q, const Vec4a& v)
    {
        const Vec4a qn = Negate(q.v);

        const Vec4a vx = SplatX(v);
        const Vec4a vy = SplatY(v);
        const Vec4a vz = SplatZ(v);

        const float32x4_t swp0 = vcombine_f32(
            vrev64_f32(vget_high_f32(q.v)),
            vrev64_f32(vget_low_f32(qn)));

        const float32x4_t swp1 = vcombine_f32(
            vzip_f32(vget_high_f32(qn), vrev64_f32(vget_high_f32(q.v))).val[0],
            vzip_f32(vget_low_f32(q.v), vrev64_f32(vget_low_f32(qn))).val[0]);

        const float32x4_t swp2 = vcombine_f32(
            vzip_f32(vrev64_f32(vget_low_f32(q.v)), vget_low_f32(qn)).val[0],
            vzip_f32(vrev64_f32(vget_high_f32(q.v)), vget_high_f32(qn)).val[0]);

        const float32x4_t mul0 = vmulq_f32(vx, swp0);
        const float32x4_t mul1 = vmulq_f32(vy, swp1);
        const float32x4_t mul2 = vmulq_f32(vz, swp2);

        const float32x4_t r = vaddq_f32(mul0, mul1);
        return vaddq_f32(r, mul2);
    }
}
