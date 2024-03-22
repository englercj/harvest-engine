// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    inline Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y)
    {
        return { { x.v.x, y.v.x, 0.0f, 0.0f } };
    }

    inline Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y, const Vec4a& z)
    {
        return { { x.v.x, y.v.x, z.v.x, 0.0f } };
    }

    inline Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y, const Vec4a& z, const Vec4a& w)
    {
        return { { x.v.x, y.v.x, z.v.x, w.v.x } };
    }

    template <typename T> inline Vec4a MakeVec4a(const Vec2<T>& v)
    {
        return { { static_cast<float>(v.x), static_cast<float>(v.y), 0.0f, 0.0f } };
    }

    template <typename T> inline Vec4a MakeVec4a(const Vec3<T>& v)
    {
        return { { static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z), 0.0f } };
    }

    template <typename T> inline Vec4a MakeVec4a(const Vec4<T>& v)
    {
        return { { static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z), static_cast<float>(v.w) } };
    }

    inline Vec4a SplatZero()
    {
        return { { 0, 0, 0, 0 } };
    }

    inline Vec4a Splat(float x)
    {
        return { { x, x, x, x } };
    }

    inline Vec4a SplatX(const Vec4a& v)
    {
        return { { v.v.x, v.v.x, v.v.x, v.v.x } };
    }

    inline Vec4a SplatY(const Vec4a& v)
    {
        return { { v.v.y, v.v.y, v.v.y, v.v.y } };
    }

    inline Vec4a SplatZ(const Vec4a& v)
    {
        return { { v.v.z, v.v.z, v.v.z, v.v.z } };
    }

    inline Vec4a SplatW(const Vec4a& v)
    {
        return { { v.v.w, v.v.w, v.v.w, v.v.w } };
    }

    // --------------------------------------------------------------------------------------------
    // Component Access

    inline float GetComponent(const Vec4a& v, size_t i)
    {
        HE_ASSERT(i < 4);
        return (&v.v.x)[i];
    }

    inline Vec4a SetComponent(const Vec4a& v, size_t i, float s)
    {
        HE_ASSERT(i < 4);
        Vec4a r = v;
        (&r.v.x)[i] = s;
        return r;
    }

    inline float GetX(const Vec4a& v)
    {
        return v.v.x;
    }

    inline float GetY(const Vec4a& v)
    {
        return v.v.y;
    }

    inline float GetZ(const Vec4a& v)
    {
        return v.v.z;
    }

    inline float GetW(const Vec4a& v)
    {
        return v.v.w;
    }

    inline Vec4a SetX(const Vec4a& v, float s)
    {
        return { { s, v.v.y, v.v.z, v.v.w } };
    }

    inline Vec4a SetY(const Vec4a& v, float s)
    {
        return { { v.v.x, s, v.v.z, v.v.w } };
    }

    inline Vec4a SetZ(const Vec4a& v, float s)
    {
        return { { v.v.x, v.v.y, s, v.v.w } };
    }

    inline Vec4a SetW(const Vec4a& v, float s)
    {
        return { { v.v.x, v.v.y, v.v.z, s } };
    }

    inline Vec4a Load(const float* p)
    {
        return { { p[0], p[1], p[2], p[3] } };
    }

    inline Vec4a LoadU(const float* p)
    {
        return { { p[0], p[1], p[2], p[3] } };
    }

    inline void Store(float* p, const Vec4a& v)
    {
        p[0] = v.v.x;
        p[1] = v.v.y;
        p[2] = v.v.z;
        p[3] = v.v.w;
    }

    inline void StoreU(float* p, const Vec4a& v)
    {
        p[0] = v.v.x;
        p[1] = v.v.y;
        p[2] = v.v.z;
        p[3] = v.v.w;
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    inline bool IsNan(const Vec4a& v)
    {
        return IsNan(v.v.x) || IsNan(v.v.y) || IsNan(v.v.z) || IsNan(v.v.w);
    }

    inline bool IsNan3(const Vec4a& v)
    {
        return IsNan(v.v.x) || IsNan(v.v.y) || IsNan(v.v.z);
    }

    inline bool IsInfinite(const Vec4a& v)
    {
        return IsInfinite(v.v.x) && IsInfinite(v.v.y) && IsInfinite(v.v.z) && IsInfinite(v.v.w);
    }

    inline bool IsInfinite3(const Vec4a& v)
    {
        return IsInfinite(v.v.x) && IsInfinite(v.v.y) && IsInfinite(v.v.z);
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    inline Vec4a Negate(const Vec4a& v)
    {
        return { { -v.v.x, -v.v.y, -v.v.z, -v.v.w } };
    }

    inline Vec4a Add(const Vec4a& a, const Vec4a& b)
    {
        return { { a.v.x + b.v.x, a.v.y + b.v.y, a.v.z + b.v.z, a.v.w + b.v.w } };
    }

    inline Vec4a Add(const Vec4a& a, float b)
    {
        return { { a.v.x + b, a.v.y + b, a.v.z + b, a.v.w + b } };
    }

    inline Vec4a Sub(const Vec4a& a, const Vec4a& b)
    {
        return { { a.v.x - b.v.x, a.v.y - b.v.y, a.v.z - b.v.z, a.v.w - b.v.w } };
    }

    inline Vec4a Sub(const Vec4a& a, float b)
    {
        return { { a.v.x - b, a.v.y - b, a.v.z - b, a.v.w - b } };
    }

    inline Vec4a Mul(const Vec4a& a, const Vec4a& b)
    {
        return { { a.v.x * b.v.x, a.v.y * b.v.y, a.v.z * b.v.z, a.v.w * b.v.w } };
    }

    inline Vec4a Mul(const Vec4a& a, float b)
    {
        return { { a.v.x * b, a.v.y * b, a.v.z * b, a.v.w * b } };
    }

    inline Vec4a MulAdd(const Vec4a& a, const Vec4a& b, const Vec4a& add)
    {
        return Add(add, Mul(a, b));
    }

    inline Vec4a Div(const Vec4a& a, const Vec4a& b)
    {
        return { { a.v.x / b.v.x, a.v.y / b.v.y, a.v.z / b.v.z, a.v.w / b.v.w } };
    }

    inline Vec4a Div(const Vec4a& a, float b)
    {
        return { { a.v.x / b, a.v.y / b, a.v.z / b, a.v.w / b } };
    }

    inline Vec4a Lerp(const Vec4a& a, const Vec4a& b, float t)
    {
        return
        { {
            Lerp(a.v.x, b.v.x, t),
            Lerp(a.v.y, b.v.y, t),
            Lerp(a.v.z, b.v.z, t),
            Lerp(a.v.w, b.v.w, t),
        } };
    }

    inline Vec4a SmoothStep(const Vec4a& a, const Vec4a& b, const Vec4a& t)
    {
        return
        { {
            SmoothStep(a.v.x, b.v.x, t.v.x),
            SmoothStep(a.v.y, b.v.y, t.v.y),
            SmoothStep(a.v.z, b.v.z, t.v.z),
            SmoothStep(a.v.w, b.v.w, t.v.w),
        } };
    }

    inline Vec4a Abs(const Vec4a& v)
    {
        return { { Abs(v.v.x), Abs(v.v.y), Abs(v.v.z), Abs(v.v.w) } };
    }

    inline Vec4a Rcp(const Vec4a& v)
    {
        return { { 1.0f / v.v.x, 1.0f / v.v.y, 1.0f / v.v.z, 1.0f / v.v.w } };
    }

    inline Vec4a RcpSafe(const Vec4a& v)
    {
        return
        { {
            IsZeroSafe(v.v.x) ? 1.0f / v.v.x : 0.0f,
            IsZeroSafe(v.v.y) ? 1.0f / v.v.y : 0.0f,
            IsZeroSafe(v.v.z) ? 1.0f / v.v.z : 0.0f,
            IsZeroSafe(v.v.w) ? 1.0f / v.v.w : 0.0f,
        } };
    }

    inline Vec4a Sqrt(const Vec4a& v)
    {
        return { { Sqrt(v.v.x), Sqrt(v.v.y), Sqrt(v.v.z), Sqrt(v.v.w) } };
    }

    inline Vec4a Rsqrt(const Vec4a& v)
    {
        return { { Rsqrt(v.v.x), Rsqrt(v.v.y), Rsqrt(v.v.z), Rsqrt(v.v.w) } };
    }

    // --------------------------------------------------------------------------------------------
    // Selection

    inline Vec4a Min(const Vec4a& a, const Vec4a& b)
    {
        return { { Min(a.v.x, b.v.x), Min(a.v.y, b.v.y), Min(a.v.z, b.v.z), Min(a.v.w, b.v.w) } };
    }

    inline Vec4a Max(const Vec4a& a, const Vec4a& b)
    {
        return { { Max(a.v.x, b.v.x), Max(a.v.y, b.v.y), Max(a.v.z, b.v.z), Max(a.v.w, b.v.w) } };
    }

    inline Vec4a Clamp(const Vec4a& v, const Vec4a& lo, const Vec4a& hi)
    {
        return { { Clamp(v.v.x, lo.v.x, hi.v.x), Clamp(v.v.y, lo.v.y, hi.v.y), Clamp(v.v.z, lo.v.z, hi.v.z), Clamp(v.v.w, lo.v.w, hi.v.w) } };
    }

    inline float HMin(const Vec4a& v)
    {
        float xy = Min(v.v.x, v.v.y);
        float zw = Min(v.v.z, v.v.w);
        return Min(xy, zw);
    }

    inline float HMax(const Vec4a& v)
    {
        float xy = Max(v.v.x, v.v.y);
        float zw = Max(v.v.z, v.v.w);
        return Max(xy, zw);
    }

    inline float HAdd(const Vec4a& v)
    {
        return (v.v.x + v.v.y) + (v.v.z + v.v.w);
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline float Dot(const Vec4a& a, const Vec4a& b)
    {
        return (a.v.x * b.v.x) + (a.v.y * b.v.y) + (a.v.z * b.v.z) + (a.v.w * b.v.w);
    }

    inline float Dot3(const Vec4a& a, const Vec4a& b)
    {
        return (a.v.x * b.v.x) + (a.v.y * b.v.y) + (a.v.z * b.v.z);
    }

    inline Vec4a Cross(const Vec4a& a, const Vec4a& b)
    {
        return
        { {
            (a.v.y * b.v.z) - (a.v.z * b.v.y),
            (a.v.z * b.v.x) - (a.v.x * b.v.z),
            (a.v.x * b.v.y) - (a.v.y * b.v.x),
            0,
        } };
    }

    inline Vec4a Normalize(const Vec4a& v)
    {
        float d = Rsqrt(LenSquared(v));
        return Mul(Splat(d), v);
    }

    inline Vec4a Normalize3(const Vec4a& v)
    {
        float d = Rsqrt(LenSquared3(v));
        return Mul(Splat(d), v);
    }

    // --------------------------------------------------------------------------------------------
    // Comparison

    inline float BoolToFloat(bool b)
    {
        constexpr float AllBits = BitCast<float>(0xffffffff);
        return b ? AllBits : 0.0f;
    }

    inline Vec4a Lt(const Vec4a& a, const Vec4a& b)
    {
        return
        {
            BoolToFloat(a.v.x < b.v.x),
            BoolToFloat(a.v.y < b.v.y),
            BoolToFloat(a.v.z < b.v.z),
            BoolToFloat(a.v.w < b.v.w),
        };
    }

    inline Vec4a Le(const Vec4a& a, const Vec4a& b)
    {
        return
        {
            BoolToFloat(a.v.x <= b.v.x),
            BoolToFloat(a.v.y <= b.v.y),
            BoolToFloat(a.v.z <= b.v.z),
            BoolToFloat(a.v.w <= b.v.w),
        };
    }

    inline Vec4a Gt(const Vec4a& a, const Vec4a& b)
    {
        return
        {
            BoolToFloat(a.v.x > b.v.x),
            BoolToFloat(a.v.y > b.v.y),
            BoolToFloat(a.v.z > b.v.z),
            BoolToFloat(a.v.w > b.v.w),
        };
    }

    inline Vec4a Ge(const Vec4a& a, const Vec4a& b)
    {
        return
        {
            BoolToFloat(a.v.x >= b.v.x),
            BoolToFloat(a.v.y >= b.v.y),
            BoolToFloat(a.v.z >= b.v.z),
            BoolToFloat(a.v.w >= b.v.w),
        };
    }

    inline Vec4a Eq(const Vec4a& a, const Vec4a& b)
    {
        return
        {
            BoolToFloat(a.v.x == b.v.x),
            BoolToFloat(a.v.y == b.v.y),
            BoolToFloat(a.v.z == b.v.z),
            BoolToFloat(a.v.w == b.v.w),
        };
    }

    inline Vec4a Ne(const Vec4a& a, const Vec4a& b)
    {
        return
        {
            BoolToFloat(a.v.x != b.v.x),
            BoolToFloat(a.v.y != b.v.y),
            BoolToFloat(a.v.z != b.v.z),
            BoolToFloat(a.v.w != b.v.w),
        };
    }

    inline Vec4a EqInt(const Vec4a& a, const Vec4a& b)
    {
        return
        {
            BoolToFloat(BitCast<uint32_t>(a.v.x) == BitCast<uint32_t>(b.v.x)),
            BoolToFloat(BitCast<uint32_t>(a.v.y) == BitCast<uint32_t>(b.v.y)),
            BoolToFloat(BitCast<uint32_t>(a.v.z) == BitCast<uint32_t>(b.v.z)),
            BoolToFloat(BitCast<uint32_t>(a.v.w) == BitCast<uint32_t>(b.v.w)),
        };
    }

    inline bool Any(const Vec4a& cmp)
    {
        return (BitCast<uint32_t>(cmp.v.x) | BitCast<uint32_t>(cmp.v.y) | BitCast<uint32_t>(cmp.v.z) | BitCast<uint32_t>(cmp.v.w)) != 0;
    }

    inline bool Any3(const Vec4a& cmp)
    {
        return (BitCast<uint32_t>(cmp.v.x) | BitCast<uint32_t>(cmp.v.y) | BitCast<uint32_t>(cmp.v.z)) != 0;
    }

    inline bool All(const Vec4a& cmp)
    {
        return BitCast<uint32_t>(cmp.v.x) && BitCast<uint32_t>(cmp.v.y) && BitCast<uint32_t>(cmp.v.z) && BitCast<uint32_t>(cmp.v.w);
    }

    inline bool All3(const Vec4a& cmp)
    {
        return BitCast<uint32_t>(cmp.v.x) && BitCast<uint32_t>(cmp.v.y) && BitCast<uint32_t>(cmp.v.z);
    }
}
