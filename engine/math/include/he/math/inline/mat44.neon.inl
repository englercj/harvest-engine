// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Arithmetic

    inline float Det33(float a, float b, float c, float d, float e, float f, float g, float h, float i)
    {
        return (a * e * i) + (b * f * g) + (c * d * h) - (c * e * g) - (b * d * i) - (a * f * h);
    }

    inline float Determinant(const Mat44& m)
    {
        const Vec4f x = MakeVec4<float>(m.cx);
        const Vec4f y = MakeVec4<float>(m.cy);
        const Vec4f z = MakeVec4<float>(m.cz);
        const Vec4f w = MakeVec4<float>(m.cw);

        return
            x.x * Det33(y.y, y.z, y.w, z.y, z.z, z.w, w.y, w.z, w.w) -
            x.y * Det33(y.x, y.z, y.w, z.x, z.z, z.w, w.x, w.z, w.w) +
            x.z * Det33(y.x, y.y, y.w, z.x, z.y, z.w, w.x, w.y, w.w) -
            x.w * Det33(y.x, y.y, y.z, z.x, z.y, z.z, w.x, w.y, w.z);
    }

    inline Mat44 Adjoint(const Mat44& m)
    {
        const Vec4f x = MakeVec4<float>(m.cx);
        const Vec4f y = MakeVec4<float>(m.cy);
        const Vec4f z = MakeVec4<float>(m.cz);
        const Vec4f w = MakeVec4<float>(m.cw);

        return
        {
            {
                 Det33(y.y, y.z, y.w, z.y, z.z, z.w, w.y, w.z, w.w),      // cofact(a0)
                -Det33(x.y, x.z, x.w, z.y, z.z, z.w, w.y, w.z, w.w),      // cofact(b0)
                 Det33(x.y, x.z, x.w, y.y, y.z, y.w, w.y, w.z, w.w),      // cofact(c0)
                -Det33(x.y, x.z, x.w, y.y, y.z, y.w, z.y, z.z, z.w),      // cofact(d0)
            },
            {
                -Det33(y.x, y.z, y.w, z.x, z.z, z.w, w.x, w.z, w.w),      // cofact(a1)
                 Det33(x.x, x.z, x.w, z.x, z.z, z.w, w.x, w.z, w.w),      // cofact(b1)
                -Det33(x.x, x.z, x.w, y.x, y.z, y.w, w.x, w.z, w.w),      // cofact(c1)
                 Det33(x.x, x.z, x.w, y.x, y.z, y.w, z.x, z.z, z.w),      // cofact(d1)
            },
            {
                 Det33(y.x, y.y, y.w, z.x, z.y, z.w, w.x, w.y, w.w),      // cofact(a2)
                -Det33(x.x, x.y, x.w, z.x, z.y, z.w, w.x, w.y, w.w),      // cofact(b2)
                 Det33(x.x, x.y, x.w, y.x, y.y, y.w, w.x, w.y, w.w),      // cofact(c2)
                -Det33(x.x, x.y, x.w, y.x, y.y, y.w, z.x, z.y, z.w),      // cofact(d2)
            },
            {
                -Det33(y.x, y.y, y.z, z.x, z.y, z.z, w.x, w.y, w.z),      // cofact(a3)
                 Det33(x.x, x.y, x.z, z.x, z.y, z.z, w.x, w.y, w.z),      // cofact(b3)
                -Det33(x.x, x.y, x.z, y.x, y.y, y.z, w.x, w.y, w.z),      // cofact(c3)
                 Det33(x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z),      // cofact(d3)
            },
        };
    }

    inline Mat44 Inverse(const Mat44& m)
    {
        float det = Determinant(m);
        return Mul(Adjoint(m), 1.0f / det);
    }

    inline Mat44 InverseTransform(const Mat44& m)
    {
        return Inverse(m);
    }

    inline Mat44 InverseTransformNoScale(const Mat44& m)
    {
        return Inverse(m);
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline Mat44 Transpose(const Mat44& m)
    {
        const float32x4x2_t mc01 = vtrnq_f32(m.cx, m.cy);
        const float32x4x2_t mc23 = vtrnq_f32(m.cz, m.cw);

        return
        {
            vreinterpretq_f32_s64(vtrn1q_s64(vreinterpretq_s64_f32(mc01.val[0]), vreinterpretq_s64_f32(mc23.val[0]))),
            vreinterpretq_f32_s64(vtrn1q_s64(vreinterpretq_s64_f32(mc01.val[1]), vreinterpretq_s64_f32(mc23.val[1]))),
            vreinterpretq_f32_s64(vtrn2q_s64(vreinterpretq_s64_f32(mc01.val[0]), vreinterpretq_s64_f32(mc23.val[0]))),
            vreinterpretq_f32_s64(vtrn2q_s64(vreinterpretq_s64_f32(mc01.val[1]), vreinterpretq_s64_f32(mc23.val[1]))),
        };
    }

    inline Vec4a TransformVector(const Mat44& m, const Vec4a& v)
    {
        float32x4_t r;
    #if HE_CPU_ARM_64
        r = vmulq_laneq_f32(m.cx, v, 0);
        r = vfmaq_laneq_f32(r, m.cy, v, 1);
        r = vfmaq_laneq_f32(r, m.cz, v, 2);
        r = vfmaq_laneq_f32(r, m.cw, v, 3);
    #else
        const float32x4_t vx = vdupq_n_f32(vgetq_lane_f32(v, 0));
        const float32x4_t vy = vdupq_n_f32(vgetq_lane_f32(v, 1));
        const float32x4_t vz = vdupq_n_f32(vgetq_lane_f32(v, 2));
        const float32x4_t vw = vdupq_n_f32(vgetq_lane_f32(v, 3));

        r = vmulq_f32(m.cx, vx);
        r = vmlaq_f32(r, m.cy, vy);
        r = vmlaq_f32(r, m.cz, vz);
        r = vmlaq_f32(r, m.cw, vw);
    #endif
        return r;
    }

    inline Vec4a TransformVector3(const Mat44& m, const Vec4a& v)
    {
        float32x4_t r;
    #if HE_CPU_ARM_64
        r = vmulq_laneq_f32(m.cx, v, 0);
        r = vfmaq_laneq_f32(r, m.cy, v, 1);
        r = vfmaq_laneq_f32(r, m.cz, v, 2);
    #else
        const float32x4_t vx = vdupq_n_f32(vgetq_lane_f32(v, 0));
        const float32x4_t vy = vdupq_n_f32(vgetq_lane_f32(v, 1));
        const float32x4_t vz = vdupq_n_f32(vgetq_lane_f32(v, 2));

        r = vmulq_f32(m.cx, vx);
        r = vmlaq_f32(r, m.cy, vy);
        r = vmlaq_f32(r, m.cz, vz);
    #endif
        return r;
    }

    inline Vec4a TransformPoint(const Mat44& m, const Vec4a& v)
    {
        float32x4_t r;
    #if HE_CPU_ARM_64
        r = vfmaq_laneq_f32(m.cw, m.cx, v, 0);
        r = vfmaq_laneq_f32(r, m.cy, v, 1);
        r = vfmaq_laneq_f32(r, m.cz, v, 2);
    #else
        const float32x4_t vx = vdupq_n_f32(vgetq_lane_f32(v, 0));
        const float32x4_t vy = vdupq_n_f32(vgetq_lane_f32(v, 1));
        const float32x4_t vz = vdupq_n_f32(vgetq_lane_f32(v, 2));

        r = vmlaq_f32(m.cw, m.cx, vx);
        r = vmlaq_f32(r, m.cy, vy);
        r = vmlaq_f32(r, m.cz, vz);
    #endif
        return r;
    }

    inline Vec4a ProjectPoint(const Mat44& m, const Vec4a& v)
    {
        const Vec4a t = TransformPoint(m, v);

        const float32x4_t tw = vdupq_laneq_f32(t, 3);
        const float32x4_t inv = vdivq_f32(vdupq_n_f32(1.0f), tw);

        return vmulq_f32(inv, t);
    }
}
