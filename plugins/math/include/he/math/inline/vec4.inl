// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Construction

    template <typename T1, typename T2> constexpr Vec4T<T1> MakeVec4(const Vec2T<T2>& v, typename Vec2T<T2>::Type z, typename Vec2T<T2>::Type w)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y), static_cast<T1>(z), static_cast<T1>(w) };
    }

    template <typename T1, typename T2> constexpr Vec4T<T1> MakeVec4(const Vec3T<T2>& v, typename Vec2T<T2>::Type w)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y), static_cast<T1>(v.z), static_cast<T1>(w) };
    }

    template <typename T1, typename T2> constexpr Vec4T<T1> MakeVec4(const Vec4T<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y), static_cast<T1>(v.z), static_cast<T1>(v.w) };
    }

    // --------------------------------------------------------------------------------------------
    // Component Access

    template <typename T> constexpr T GetComponent(const Vec4T<T>& v, size_t i)
    {
        HE_ASSERT(i < Vec4T<T>::Size);
        return GetPointer(v)[i];
    }

    template <typename T> constexpr Vec4T<T>& SetComponent(Vec4T<T>& v, size_t i, T s)
    {
        HE_ASSERT(i < Vec4T<T>::Size);
        GetPointer(v)[i] = s;
        return v;
    }

    template <typename T> constexpr T* GetPointer(Vec4T<T>& v)
    {
        return static_cast<T*>(&v.x);
    }

    template <typename T> constexpr const T* GetPointer(const Vec4T<T>& v)
    {
        return static_cast<const T*>(&v.x);
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    template <typename T> constexpr bool IsNan(const Vec4T<T>& v)
    {
        return IsNan(v.x) || IsNan(v.y) || IsNan(v.z) || IsNan(v.w);
    }

    template <typename T> constexpr bool IsInfinite(const Vec4T<T>& v)
    {
        return IsInfinite(v.x) || IsInfinite(v.y) || IsInfinite(v.z) || IsInfinite(v.w);
    }

    template <typename T> constexpr bool IsFinite(const Vec4T<T>& v)
    {
        return IsFinite(v.x) && IsFinite(v.y) && IsFinite(v.z) && IsFinite(v.w);
    }

    template <typename T> constexpr bool IsZeroSafe(const Vec4T<T>& v)
    {
        return Dot(v, v) > Float_ZeroSafe;
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    template <typename T> constexpr Vec4T<T> Negate(const Vec4T<T>& v)
    {
        return { -v.x, -v.y, -v.z, -v.w };
    }

    template <typename T> constexpr Vec4T<T> Add(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }

    template <typename T> constexpr Vec4T<T> Add(const Vec4T<T>& a, T b)
    {
        return { a.x + b, a.y + b, a.z + b, a.w + b };
    }

    template <typename T> constexpr Vec4T<T> Sub(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    }

    template <typename T> constexpr Vec4T<T> Sub(const Vec4T<T>& a, T b)
    {
        return { a.x - b, a.y - b, a.z - b, a.w - b };
    }

    template <typename T> constexpr Vec4T<T> Mul(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
    }

    template <typename T> constexpr Vec4T<T> Mul(const Vec4T<T>& a, T b)
    {
        return { a.x * b, a.y * b, a.z * b, a.w * b };
    }

    template <typename T> constexpr Vec4T<T> Div(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
    }

    template <typename T> constexpr Vec4T<T> Div(const Vec4T<T>& a, T b)
    {
        return { a.x / b, a.y / b, a.z / b, a.w / b };
    }

    template <typename T> constexpr Vec4T<T> MulAdd(const Vec4T<T>& a, const Vec4T<T>& b, const Vec4T<T>& c)
    {
        return { a.x * b.x + c.x, a.y * b.y + c.y, a.z * b.z + c.z, a.w * b.w + c.w };
    }

    template <typename T> constexpr Vec4T<T> Lerp(const Vec4T<T>& a, const Vec4T<T>& b, float t)
    {
        return
        {
            Lerp(a.x, b.x, t),
            Lerp(a.y, b.y, t),
            Lerp(a.z, b.z, t),
            Lerp(a.w, b.w, t),
        };
    }

    template <typename T> constexpr Vec4T<T> SmoothStep(float a, float b, const Vec4T<T>& t)
    {
        return
        {
            SmoothStep(a, b, t.x),
            SmoothStep(a, b, t.y),
            SmoothStep(a, b, t.z),
            SmoothStep(a, b, t.w),
        };
    }

    template <typename T> constexpr Vec4T<T> Abs(const Vec4T<T>& v)
    {
        return { Abs(v.x), Abs(v.y), Abs(v.z), Abs(v.w) };
    }

    template <typename T> constexpr Vec4T<T> Rcp(const Vec4T<T>& v)
    {
        return { 1.0f / v.x, 1.0f / v.y, 1.0f / v.z, 1.0f / v.w };
    }

    template <typename T> constexpr Vec4T<T> RcpSafe(const Vec4T<T>& v)
    {
        return
        {
            IsZeroSafe(v.x) ? 1.0f / v.x : 0.0f,
            IsZeroSafe(v.y) ? 1.0f / v.y : 0.0f,
            IsZeroSafe(v.z) ? 1.0f / v.z : 0.0f,
            IsZeroSafe(v.w) ? 1.0f / v.w : 0.0f,
        };
    }

    template <typename T> inline Vec4T<T> Sqrt(const Vec4T<T>& v)
    {
        return { Sqrt(v.x), Sqrt(v.y), Sqrt(v.z), Sqrt(v.w) };
    }

    template <typename T> inline Vec4T<T> Rsqrt(const Vec4T<T>& v)
    {
        return { Rsqrt(v.x), Rsqrt(v.y), Rsqrt(v.z), Rsqrt(v.w) };
    }

    // --------------------------------------------------------------------------------------------
    // Selection

    template <typename T> constexpr Vec4T<T> Min(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        return { Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z), Min(a.w, b.w) };
    }

    template <typename T> constexpr Vec4T<T> Max(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        return { Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z), Max(a.w, b.w) };
    }

    template <typename T> constexpr Vec4T<T> Clamp(const Vec4T<T>& v, const Vec4T<T>& lo, const Vec4T<T>& hi)
    {
        return { Clamp(v.x, lo.x, hi.x), Clamp(v.y, lo.y, hi.y), Clamp(v.z, lo.z, hi.z), Clamp(v.w, lo.w, hi.w) };
    }

    template <typename T> constexpr T HMin(const Vec4T<T>& v)
    {
        T xy = Min(v.x, v.y);
        T zw = Min(v.z, v.w);
        return Min(xy, zw);
    }

    template <typename T> constexpr T HMax(const Vec4T<T>& v)
    {
        T xy = Max(v.x, v.y);
        T zw = Max(v.z, v.w);
        return Max(xy, zw);
    }

    template <typename T> constexpr T HAdd(const Vec4T<T>& v)
    {
        return (v.x + v.y) + (v.z + v.w);
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    template <typename T> constexpr T Dot(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    }

    template <typename T> constexpr T LenSquared(const Vec4T<T>& v)
    {
        return Dot(v, v);
    }

    template <typename T> inline T Len(const Vec4T<T>& v)
    {
        return Sqrt(LenSquared(v));
    }

    template <typename T> inline Vec4T<T> Normalize(const Vec4T<T>& v)
    {
        return Mul(v, Rsqrt(LenSquared(v)));
    }

    template <typename T> inline Vec4T<T> NormalizeSafe(const Vec4T<T>& v)
    {
        return IsZeroSafe(v) ? Normalize(v) : v;
    }

    template <typename T> inline bool IsNormalized(const Vec4T<T>& v)
    {
        return Abs(Len(v) - 1) < 40.0f * Float_Epsilon;
    }

    // --------------------------------------------------------------------------------------------
    // Operators

    template <typename T> constexpr Vec4T<T> operator-(const Vec4T<T>& v) { return Negate(v); }

    template <typename T> constexpr Vec4T<T> operator+(const Vec4T<T>& a, const Vec4T<T>& b) { return Add(a, b); }
    template <typename T> constexpr Vec4T<T> operator-(const Vec4T<T>& a, const Vec4T<T>& b) { return Sub(a, b); }
    template <typename T> constexpr Vec4T<T> operator*(const Vec4T<T>& a, const Vec4T<T>& b) { return Mul(a, b); }
    template <typename T> constexpr Vec4T<T> operator/(const Vec4T<T>& a, const Vec4T<T>& b) { return Div(a, b); }

    template <typename T> constexpr Vec4T<T> operator+(const Vec4T<T>& a, T b) { return Add(a, b); }
    template <typename T> constexpr Vec4T<T> operator-(const Vec4T<T>& a, T b) { return Sub(a, b); }
    template <typename T> constexpr Vec4T<T> operator*(const Vec4T<T>& a, T b) { return Mul(a, b); }
    template <typename T> constexpr Vec4T<T> operator/(const Vec4T<T>& a, T b) { return Div(a, b); }

    template <typename T> constexpr Vec4T<T>& operator+=(Vec4T<T>& a, const Vec4T<T>& b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec4T<T>& operator-=(Vec4T<T>& a, const Vec4T<T>& b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec4T<T>& operator*=(Vec4T<T>& a, const Vec4T<T>& b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec4T<T>& operator/=(Vec4T<T>& a, const Vec4T<T>& b) { a = Div(a, b); return a; }

    template <typename T> constexpr Vec4T<T>& operator+=(Vec4T<T>& a, T b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec4T<T>& operator-=(Vec4T<T>& a, T b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec4T<T>& operator*=(Vec4T<T>& a, T b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec4T<T>& operator/=(Vec4T<T>& a, T b) { a = Div(a, b); return a; }

    template <typename T> constexpr bool operator<(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        if (a.x < b.x) return true;
        if (a.x > b.x) return false;
        if (a.y < b.y) return true;
        if (a.y > b.y) return false;
        if (a.z < b.z) return true;
        if (a.z > b.z) return false;
        return a.w < b.w;
    }

    template <typename T> constexpr bool operator==(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }

    template <typename T> constexpr bool operator!=(const Vec4T<T>& a, const Vec4T<T>& b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
    }
}
