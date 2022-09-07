// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    inline Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y)
    {
    #if HE_CPU_ARM_64
        float32x4_t zipped = vzip1q_f32(x, y);
    #else
        #error "TODO"
    #endif
        int64x2_t reinterpreted = vreinterpretq_s64_f32(zipped);
        int64x2_t truncated = vsetq_lane_s64(0, reinterpreted, 1);
        return vreinterpretq_f32_s64(truncated);
    }

    inline Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y, const Vec4a& z)
    {
    #if HE_CPU_ARM_64
        float32x4_t low = vzip1q_f32(x, y);
    #else
        #error "TODO"
    #endif
        float32x4_t mid = vcopyq_laneq_f32(low, 2, z, 0);
        float32x4_t high = vsetq_lane_f32(0.0f, mid, 3);
        return high;
    }

    inline Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y, const Vec4a& z, const Vec4a& w)
    {
    #if HE_CPU_ARM_64
        float32x4_t low = vzip1q_f32(x, y);
    #else
        #error "TODO"
    #endif
        float32x4_t mid = vcopyq_laneq_f32(low, 2, z, 0);
        float32x4_t high = vcopyq_laneq_f32(mid, 3, w, 0);
        return high;
    }

    template <> inline Vec4a MakeVec4a(const Vec2<float>& v)
    {
        int64x2_t zero = vdupq_n_s64(0);
        int64x2_t result = vld1q_lane_s64(reinterpret_cast<const int64_t*>(&v), zero, 0);
        return vreinterpretq_f32_s64(result);
    }

    template <> inline Vec4a MakeVec4a(const Vec2<int32_t>& v)
    {
        int64x2_t zero = vdupq_n_s64(0);
        int64x2_t result = vld1q_lane_s64(reinterpret_cast<const int64_t*>(&v), zero, 0);
        return vcvtq_f32_s32(vreinterpretq_s32_s64(result));
    }

    template <> inline Vec4a MakeVec4a(const Vec2<uint32_t>& v)
    {
        uint64x2_t zero = vdupq_n_u64(0);
        uint64x2_t result = vld1q_lane_u64(reinterpret_cast<const uint64_t*>(&v), zero, 0);
        return vcvtq_f32_u32(vreinterpretq_u32_u64(result));
    }

    template <> inline Vec4a MakeVec4a(const Vec3<float>& v)
    {
        int64x2_t zero = vdupq_n_s64(0);
        int64x2_t low = vld1q_lane_s64(reinterpret_cast<const int64_t*>(&v), zero, 0);
        return vsetq_lane_f32(v.z, vreinterpretq_f32_s64(low), 2);
    }

    template <> inline Vec4a MakeVec4a(const Vec3<int32_t>& v)
    {
        int64x2_t zero = vdupq_n_s64(0);
        int64x2_t low = vld1q_lane_s64(reinterpret_cast<const int64_t*>(&v), zero, 0);
        int32x4_t loaded = vsetq_lane_s32(v.z, vreinterpretq_s32_s64(low), 2);
        return vcvtq_f32_s32(loaded);
    }

    template <> inline Vec4a MakeVec4a(const Vec3<uint32_t>& v)
    {
        uint64x2_t zero = vdupq_n_u64(0);
        uint64x2_t low = vld1q_lane_u64(reinterpret_cast<const uint64_t*>(&v), zero, 0);
        uint32x4_t loaded = vsetq_lane_u32(v.z, vreinterpretq_u32_u64(low), 2);
        return vcvtq_f32_u32(loaded);
    }

    template <> inline Vec4a MakeVec4a(const Vec4<float>& v)
    {
        return vld1q_f32(&v.x);
    }

    template <> inline Vec4a MakeVec4a(const Vec4<int32_t>& v)
    {
        int32x4_t loaded = vld1q_s32(reinterpret_cast<const int*>(&v));
        return vcvtq_f32_s32(loaded);
    }

    template <> inline Vec4a MakeVec4a(const Vec4<uint32_t>& v)
    {
        uint32x4_t loaded = vld1q_u32(reinterpret_cast<const uint32_t*>(&v));
        return vcvtq_f32_u32(loaded);
    }

    inline Vec4a SplatZero()
    {
        return vdupq_n_f32(0.0f);
    }

    inline Vec4a Splat(float x)
    {
        return vdupq_n_f32(x);
    }

    inline Vec4a SplatX(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vdupq_laneq_f32(v, 0);
    #else
        return vdupq_n_f32(vgetq_lane_f32(v, 0));
    #endif
    }

    inline Vec4a SplatY(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vdupq_laneq_f32(v, 1);
    #else
        return vdupq_n_f32(vgetq_lane_f32(v, 1));
    #endif
    }

    inline Vec4a SplatZ(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vdupq_laneq_f32(v, 2);
    #else
        return vdupq_n_f32(vgetq_lane_f32(v, 2));
    #endif
    }

    inline Vec4a SplatW(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vdupq_laneq_f32(v, 3);
    #else
        return vdupq_n_f32(vgetq_lane_f32(v, 3));
    #endif
    }

    // --------------------------------------------------------------------------------------------
    // Component Access

    inline float GetComponent(const Vec4a& v, size_t i)
    {
        HE_ASSERT(i < 4);
    #if HE_COMPILER_MSVC
        return v.v.n128_f32[i];
    #else
        return v.v[i];
    #endif
    }

    inline Vec4a SetComponent(const Vec4a& v, size_t i, float s)
    {
        HE_ASSERT(i < 4);
        Vec4a r = v;
    #if HE_COMPILER_MSVC
        r.v.n128_f32[i] = s;
    #else
        r.v[i] = s;
    #endif
        return r;
    }

    inline float GetX(const Vec4a& v)
    {
        return vgetq_lane_f32(v, 0);
    }

    inline float GetY(const Vec4a& v)
    {
        return vgetq_lane_f32(v, 1);
    }

    inline float GetZ(const Vec4a& v)
    {
        return vgetq_lane_f32(v, 2);
    }

    inline float GetW(const Vec4a& v)
    {
        return vgetq_lane_f32(v, 3);
    }

    inline Vec4a SetX(const Vec4a& v, float s)
    {
        return vsetq_lane_f32(s, v, 0);
    }

    inline Vec4a SetY(const Vec4a& v, float s)
    {
        return vsetq_lane_f32(s, v, 1);
    }

    inline Vec4a SetZ(const Vec4a& v, float s)
    {
        return vsetq_lane_f32(s, v, 2);
    }

    inline Vec4a SetW(const Vec4a& v, float s)
    {
        return vsetq_lane_f32(s, v, 3);
    }

    inline Vec4a Load(const float* p)
    {
        return vld1q_f32(p);
    }

    inline Vec4a LoadU(const float* p)
    {
        return vld1q_f32(p);
    }

    inline void Store(float* p, const Vec4a& v)
    {
        vst1q_f32(p, v);
    }

    inline void StoreU(float* p, const Vec4a& v)
    {
        vst1q_f32(p, v);
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    inline bool IsNan(const Vec4a& v)
    {
        return Any(Ne(v, v));
    }

    inline bool IsNan3(const Vec4a& v)
    {
        constexpr uint32_t CmpTrue = ~0u;

        uint32x4_t cmp0 = vceqq_f32(v, v);
        uint32x4_t cmp1 = vsetq_lane_f32(CmpTrue, cmp0, 3);

    #if HE_CPU_ARM_64
        return vminvq_u32(cmp1) == CmpTrue;
    #else
        uint32x2_t cmp2 = vpmin_u32(vget_low_u32(cmp1), vget_high_u32(cmp1));
        uint32_t r = vgetq_lane_u32(vpmin_u32(cmp2, cmp2), 0);
        return r == CmpTrue;
    #endif
    }

    inline bool IsInfinite(const Vec4a& v)
    {
        uint32x4_t a = vandq_u32(vreinterpretq_u32_f32(v), Vec4a_AbsMask);
        uint32x4_t cmp0 = vreinterpretq_f32_u32(vceqq_f32(vreinterpretq_f32_u32(a), Vec4a_Infinity));

    #if HE_CPU_ARM_64
        return vmaxvq_u32(cmp0) != 0;
    #else
        uint32x2_t cmp1 = vpmax_u32(vget_low_u32(cmp0), vget_high_u32(cmp0));
        uint32_t r = vgetq_lane_u32(vpmax_u32(cmp1, cmp1), 0);
        return r != 0;
    #endif
    }

    inline bool IsInfinite3(const Vec4a& v)
    {
        uint32x4_t a = vandq_u32(vreinterpretq_u32_f32(v), Vec4a_AbsMask);
        float32x4_t cmp0 = vceqq_f32(vreinterpretq_f32_u32(a), Vec4a_Infinity);
        uint32x4_t cmp1 = vsetq_lane_u32(0, cmp0, 3);

    #if HE_CPU_ARM_64
        return vmaxvq_u32(cmp1) != 0;
    #else
        uint32x2_t cmp2 = vpmax_u32(vget_low_u32(cmp1), vget_high_u32(cmp1));
        uint32_t r = vgetq_lane_u32(vpmax_u32(cmp2, cmp2), 0);
        return r != 0;
    #endif
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    inline Vec4a Negate(const Vec4a& v)
    {
        return vnegq_f32(v);
    }

    inline Vec4a Add(const Vec4a& a, const Vec4a& b)
    {
        return vaddq_f32(a, b);
    }

    inline Vec4a Add(const Vec4a& a, float b)
    {
        return vaddq_f32(a, vdupq_n_f32(b));
    }

    inline Vec4a Sub(const Vec4a& a, const Vec4a& b)
    {
        return vsubq_f32(a, b);
    }

    inline Vec4a Sub(const Vec4a& a, float b)
    {
        return vsubq_f32(a, vdupq_n_f32(b));
    }

    inline Vec4a Mul(const Vec4a& a, const Vec4a& b)
    {
        return vmulq_f32(a, b);
    }

    inline Vec4a Mul(const Vec4a& a, float b)
    {
        return vmulq_f32(a, vdupq_n_f32(b));
    }

    inline Vec4a MulAdd(const Vec4a& a, const Vec4a& b, const Vec4a& add)
    {
        return vmlaq_f32(add, a, b);
    }

    inline Vec4a Div(const Vec4a& a, const Vec4a& b)
    {
    #if HE_CPU_ARM_64
        return vdivq_f32(a, b);
    #else
        // 2 iterations of Newton-Raphson refinement of reciprocal
        float32x4_t rcp = vrecpeq_f32(b);
        float32x4_t s0 = vrecpsq_f32(rcp, a);
        rcp = vmulq_f32(s0, rcp);
        s0 = vrecpsq_f32(rcp, b);
        rcp = vmulq_f32(s0, rcp);
        return vmulq_f32(a, rcp);
    #endif
    }

    inline Vec4a Div(const Vec4a& a, float b)
    {
        return Div(a, vdupq_n_f32(b));
    }

    inline Vec4a Lerp(const Vec4a& a, const Vec4a& b, float t)
    {
        float32x4_t l = vsubq_f32(a, b);
        return vmlaq_n_f32(a, l, t);
    }

    inline Vec4a SmoothStep(const Vec4a& a, const Vec4a& b, const Vec4a& t)
    {
        Vec4a te = vsubq_f32(t, a);
        Vec4a ee = vsubq_f32(b, a);
        Vec4a v = Div(te, ee);

        Vec4a zero = vdupq_n_f32(0.0f);
        Vec4a one = vdupq_n_f32(1.0f);

        Vec4a c = vmaxq_f32(zero, vminq_f32(v, one));

        Vec4a three = vdupq_n_f32(3.0f);
        return vmulq_f32(vmulq_f32(c, c), vsubq_f32(three, vaddq_f32(c, c)));
    }

    inline Vec4a Abs(const Vec4a& v)
    {
        return vabsq_f32(v);
    }

    inline Vec4a Rcp(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vdivq_f32(vdupq_n_f32(1.0f), v);
    #else
        // 2 iterations of Newton-Raphson refinement of reciprocal
        float32x4_t rcp = vrecpeq_f32(v);
        float32x4_t s0 = vrecpsq_f32(rcp, v);
        rcp = vmulq_f32(s0, rcp);
        s0 = vrecpsq_f32(rcp, v);
        return vmulq_f32(s0, rcp);
    #endif
    }

    inline Vec4a RcpSafe(const Vec4a& v)
    {
        Vec4a rcp = Rcp(v);
        uint32x4_t p = vcgtq_f32(vabsq_f32(v), vdupq_n_f32(Float_ZeroSafe));
        return vbslq_f32(vreinterpretq_u32_f32(p), rcp, Vec4a_Zero);
    }

    inline Vec4a Sqrt(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vsqrtq_f32(v);
    #else
        // 3 iterations of Newton-Raphson refinement of sqrt
        float32x4_t s0 = vrsqrteq_f32(v);
        float32x4_t p0 = vmulq_f32(v, s0);
        float32x4_t r0 = vrsqrtsq_f32(p0, s0);

        float32x4_t s1 = vmulq_f32(s0, r0);
        float32x4_t p1 = vmulq_f32(v, s1);
        float32x4_t r1 = vrsqrtsq_f32(p1, s1);

        float32x4_t s2 = vmulq_f32(s1, r1);
        float32x4_t p2 = vmulq_f32(v, s2);
        float32x4_t r2 = vrsqrtsq_f32(p2, s2);

        float32x4_t s3 = vmulq_f32(s2, r2);

        float32x4_t eqInf = vreinterpretq_f32_u32(vceqq_s32(vreinterpretq_s32_f32(v), vreinterpretq_s32_f32(Vec4a_Infinity)));
        float32x4_t eqZero = vreinterpretq_f32_u32(vceqq_f32(v, vdupq_n_f32(0)));
        float32x4_t select = vreinterpretq_f32_u32(vceqq_s32(vreinterpretq_s32_f32(eqInf), vreinterpretq_s32_f32(eqZero)));

        float32x4_t mul0 = vmulq_f32(v, s3);
        return vbslq_f32(vreinterpretq_u32_f32(select), mul0, v);
    #endif
    }

    inline Vec4a Rsqrt(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vdivq_f32(vdupq_n_f32(1.0f), vsqrtq_f32(v));
    #else
        // 2 iterations of Newton-Raphson refinement of reciprocal
        float32x4_t s0 = vrsqrteq_f32(v);
        float32x4_t p0 = vmulq_f32(v, s0);
        float32x4_t r0 = vrsqrtsq_f32(p0, s0);

        float32x4_t s1 = vmulq_f32(s0, r0);
        float32x4_t p1 = vmulq_f32(v, s1);
        float32x4_t r1 = vrsqrtsq_f32(p1, s1);

        return vmulq_f32(s1, r1);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    // Selection

    inline Vec4a Min(const Vec4a& a, const Vec4a& b)
    {
        return vminq_f32(a, b);
    }

    inline Vec4a Max(const Vec4a& a, const Vec4a& b)
    {
        return vmaxq_f32(a, b);
    }

    inline Vec4a Clamp(const Vec4a& v, const Vec4a& lo, const Vec4a& hi)
    {
        return vmaxq_f32(lo, vminq_f32(v, hi));
    }

    inline float HMin(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vminvq_f32(v);
    #else
        float32x2_t m0 = vpmin_f32(vget_low_f32(v), vget_high_f32(v));
        float32x2_t m1 = vpmin_f32(m0, m0);
        return vget_lane_f32(m1, 0);
    #endif
    }

    inline float HMax(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vmaxvq_f32(v);
    #else
        float32x2_t m0 = vpmax_f32(vget_low_f32(v), vget_high_f32(v));
        float32x2_t m1 = vpmax_f32(m0, m0);
        return vget_lane_f32(m1, 0);
    #endif
    }

    inline float HAdd(const Vec4a& v)
    {
    #if HE_CPU_ARM_64
        return vaddvq_f32(v);
    #else
        float32x2_t add0 = vadd_f32(vget_low_f32(v), vget_high_f32(v));
        float32x2_t add1 = vpadd_f32(add0, add0);
        return vget_lane_f32(add1, 0);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline float Dot(const Vec4a& a, const Vec4a& b)
    {
    #if HE_CPU_ARM_64
        float32x4_t mul0 = vmulq_f32(a, b);
        return vaddvq_f32(mul0);
    #else
        float32x4_t mul0 = vmulq_f32(a, b);
        float32x2_t add0 = vadd_f32(vget_low_f32(mul0), vget_high_f32(mul0));
        float32x2_t add1 = vpadd_f32(add0, add0);
        return vget_lane_f32(add1, 0);
    #endif
    }

    inline float Dot3(const Vec4a& a, const Vec4a& b)
    {
    #if HE_CPU_ARM_64
        float32x4_t mul0 = vmulq_f32(a, b);
        float32x4_t xyz0 = vsetq_lane_f32(0.0f, mul0, 3);
        return vaddvq_f32(xyz0);
    #else
        float32x4_t mul0 = vmulq_f32(a, b);
        float32x2_t low = vget_low_f32(mul0);
        float32x2_t high = vget_high_f32(mul0);
        float32x2_t add0 = vpadd_f32(low, low);
        float32x2_t high0 = vdup_lane_f32(high, 0);
        float32x2_t add1 = vadd_f32(add0, high0);
        return vget_lane_f32(add1, 0);
    #endif
    }

    inline Vec4a Cross(const Vec4a& a, const Vec4a& b)
    {
        float32x4_t rot0 = vextq_f32(a, a, 1);
        float32x4_t rot1 = vextq_f32(b, b, 1);

        float32x4_t mul0 = vmulq_f32(a, rot1);
        float32x4_t mul1 = vmlsq_f32(mul0, rot0, b);

        return mul1;
    }

    inline Vec4a Normalize(const Vec4a& v)
    {
        float32x4_t p = vmulq_f32(v, v);

    #if HE_CPU_ARM_64
        p = vpaddq_f32(p, p);
        p = vpaddq_f32(p, p);
    #else
        float32x2_t add0 = vpadd_f32(vget_low_f32(p), vget_high_f32(p));
        float32x2_t add1 = vpadd_f32(t, t);
        p = vcombine_f32(add1, add1);
    #endif

        float32x4_t vd = vrsqrteq_f32(p);
        return vmulq_f32(v, vd);
    }

    inline Vec4a Normalize3(const Vec4a& v)
    {
        float32x4_t p = vmulq_f32(v, v);
        p = vsetq_lane_f32(0.0f, p, 3);

    #if HE_CPU_ARM_64
        p = vpaddq_f32(p, p);
        p = vpaddq_f32(p, p);
    #else
        float32x2_t add0 = vpadd_f32(vget_low_f32(p), vget_high_f32(p));
        float32x2_t add1 = vpadd_f32(t, t);
        p = vcombine_f32(add1, add1);
    #endif

        float32x4_t vd = vrsqrteq_f32(p);
        return vmulq_f32(v, vd);
    }

    // --------------------------------------------------------------------------------------------
    // Comparison

    inline Vec4a Lt(const Vec4a& a, const Vec4a& b)
    {
        return vreinterpretq_f32_u32(vcltq_f32(a, b));
    }

    inline Vec4a Le(const Vec4a& a, const Vec4a& b)
    {
        return vreinterpretq_f32_u32(vcleq_f32(a, b));
    }

    inline Vec4a Gt(const Vec4a& a, const Vec4a& b)
    {
        return vreinterpretq_f32_u32(vcgtq_f32(a, b));
    }

    inline Vec4a Ge(const Vec4a& a, const Vec4a& b)
    {
        return vreinterpretq_f32_u32(vcgeq_f32(a, b));
    }

    inline Vec4a Eq(const Vec4a& a, const Vec4a& b)
    {
        return vreinterpretq_f32_u32(vceqq_f32(a, b));
    }

    inline Vec4a Ne(const Vec4a& a, const Vec4a& b)
    {
        return vreinterpretq_f32_u32(vmvnq_u32(vceqq_f32(a, b)));
    }

    inline Vec4a EqInt(const Vec4a& a, const Vec4a& b)
    {
        return vreinterpretq_f32_u32(vceqq_s32(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)));
    }

    inline bool Any(const Vec4a& cmp)
    {
    #if HE_CPU_ARM_64
        return vmaxvq_u32(vreinterpretq_u32_f32(cmp)) != 0;
    #else
        uint32x2_t cmp1 = vpmax_u32(vget_low_u32(cmp0), vget_high_u32(cmp0));
        uint32_t r = vgetq_lane_u32(vpmax_u32(cmp1, cmp1), 0);
        return r != 0;
    #endif
    }

    inline bool Any3(const Vec4a& cmp)
    {
        return Any(vsetq_lane_f32(0.0f, cmp, 3));
    }

    inline bool All(const Vec4a& cmp)
    {
    #if HE_CPU_ARM_64
        return vminvq_u32(vreinterpretq_u32_f32(cmp)) != 0;
    #else
        uint32x2_t cmp1 = vpmin_u32(vget_low_u32(cmp0), vget_high_u32(cmp0));
        uint32_t r = vgetq_lane_u32(vpmin_u32(cmp1, cmp1), 0);
        return r != 0;
    #endif
    }

    inline bool All3(const Vec4a& cmp)
    {
        return All(vsetq_lane_f32(Float_AllBits, cmp, 3));
    }
}
