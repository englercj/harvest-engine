// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Construction

    template <typename T1, typename T2> constexpr Vec2T<T1> MakeVec2(const Vec2T<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y) };
    }

    template <typename T1, typename T2> constexpr Vec2T<T1> MakeVec2(const Vec3T<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y) };
    }

    template <typename T1, typename T2> constexpr Vec2T<T1> MakeVec2(const Vec4T<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y) };
    }

    // --------------------------------------------------------------------------------------------
    // Component Access

    template <typename T> constexpr T GetComponent(const Vec2T<T>& v, size_t i)
    {
        HE_ASSERT(i < Vec2T<T>::Size);
        return GetPointer(v)[i];
    }

    template <typename T> constexpr Vec2T<T>& SetComponent(Vec2T<T>& v, size_t i, T s)
    {
        HE_ASSERT(i < Vec2T<T>::Size);
        GetPointer(v)[i] = s;
        return v;
    }

    template <typename T> constexpr T* GetPointer(Vec2T<T>& v)
    {
        return static_cast<T*>(&v.x);
    }

    template <typename T> constexpr const T* GetPointer(const Vec2T<T>& v)
    {
        return static_cast<const T*>(&v.x);
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    template <typename T> constexpr bool IsNan(const Vec2T<T>& v)
    {
        return IsNan(v.x) || IsNan(v.y);
    }

    template <typename T> constexpr bool IsInfinite(const Vec2T<T>& v)
    {
        return IsInfinite(v.x) || IsInfinite(v.y);
    }

    template <typename T> constexpr bool IsFinite(const Vec2T<T>& v)
    {
        return IsFinite(v.x) && IsFinite(v.y);
    }

    template <typename T> constexpr bool IsZeroSafe(const Vec2T<T>& v)
    {
        return Dot(v, v) > Float_ZeroSafe;
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    template <typename T> constexpr Vec2T<T> Negate(const Vec2T<T>& v)
    {
        return { -v.x, -v.y };
    }

    template <typename T> constexpr Vec2T<T> Add(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        return { a.x + b.x, a.y + b.y };
    }

    template <typename T> constexpr Vec2T<T> Add(const Vec2T<T>& a, T b)
    {
        return { a.x + b, a.y + b };
    }

    template <typename T> constexpr Vec2T<T> Sub(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        return { a.x - b.x, a.y - b.y };
    }

    template <typename T> constexpr Vec2T<T> Sub(const Vec2T<T>& a, T b)
    {
        return { a.x - b, a.y - b };
    }

    template <typename T> constexpr Vec2T<T> Mul(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        return { a.x * b.x, a.y * b.y };
    }

    template <typename T> constexpr Vec2T<T> Mul(const Vec2T<T>& a, T b)
    {
        return { a.x * b, a.y * b };
    }

    template <typename T> constexpr Vec2T<T> Div(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        return { a.x / b.x, a.y / b.y };
    }

    template <typename T> constexpr Vec2T<T> Div(const Vec2T<T>& a, T b)
    {
        return { a.x / b, a.y / b };
    }

    template <typename T> constexpr Vec2T<T> MulAdd(const Vec2T<T>& a, const Vec2T<T>& b, const Vec2T<T>& c)
    {
        return { a.x * b.x + c.x, a.y * b.y + c.y };
    }

    template <typename T> constexpr Vec2T<T> Lerp(const Vec2T<T>& a, const Vec2T<T>& b, float t)
    {
        return
        {
            Lerp(a.x, b.x, t),
            Lerp(a.y, b.y, t),
        };
    }

    template <typename T> constexpr Vec2T<T> SmoothStep(float a, float b, const Vec2T<T>& t)
    {
        return
        {
            SmoothStep(a, b, t.x),
            SmoothStep(a, b, t.y),
        };
    }

    template <typename T> constexpr Vec2T<T> Abs(const Vec2T<T>& v)
    {
        return { Abs(v.x), Abs(v.y) };
    }

    template <typename T> constexpr Vec2T<T> Rcp(const Vec2T<T>& v)
    {
        return { 1.0f / v.x, 1.0f / v.y };
    }

    template <typename T> constexpr Vec2T<T> RcpSafe(const Vec2T<T>& v)
    {
        return
        {
            IsZeroSafe(v.x) ? 1.0f / v.x : 0.0f,
            IsZeroSafe(v.y) ? 1.0f / v.y : 0.0f,
        };
    }

    template <typename T> inline Vec2T<T> Sqrt(const Vec2T<T>& v)
    {
        return { Sqrt(v.x), Sqrt(v.y) };
    }

    template <typename T> inline Vec2T<T> Rsqrt(const Vec2T<T>& v)
    {
        return { Rsqrt(v.x), Rsqrt(v.y) };
    }

    // --------------------------------------------------------------------------------------------
    // Selection

    template <typename T> constexpr Vec2T<T> Min(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        return { Min(a.x, b.x), Min(a.y, b.y) };
    }

    template <typename T> constexpr Vec2T<T> Max(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        return { Max(a.x, b.x), Max(a.y, b.y) };
    }

    template <typename T> constexpr Vec2T<T> Clamp(const Vec2T<T>& v, const Vec2T<T>& lo, const Vec2T<T>& hi)
    {
        return { Clamp(v.x, lo.x, hi.x), Clamp(v.y, lo.y, hi.y) };
    }

    template <typename T> constexpr T HMin(const Vec2T<T>& v)
    {
        return Min(v.x, v.y);
    }

    template <typename T> constexpr T HMax(const Vec2T<T>& v)
    {
        return Max(v.x, v.y);
    }

    template <typename T> constexpr T HAdd(const Vec2T<T>& v)
    {
        return v.x + v.y;
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    template <typename T> constexpr T Dot(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        return (a.x * b.x) + (a.y * b.y);
    }

    template <typename T> constexpr T LenSquared(const Vec2T<T>& v)
    {
        return Dot(v, v);
    }

    template <typename T> inline T Len(const Vec2T<T>& v)
    {
        return Sqrt(LenSquared(v));
    }

    template <typename T> inline Vec2T<T> Normalize(const Vec2T<T>& v)
    {
        return Mul(v, Rsqrt(LenSquared(v)));
    }

    template <typename T> inline Vec2T<T> NormalizeSafe(const Vec2T<T>& v)
    {
        return IsZeroSafe(v) ? Normalize(v) : v;
    }

    template <typename T> inline bool IsNormalized(const Vec2T<T>& v)
    {
        return Abs(Len(v) - 1) < 40.0f * Float_Epsilon;
    }

    // --------------------------------------------------------------------------------------------
    // Operators

    template <typename T> constexpr Vec2T<T> operator-(const Vec2T<T>& v) { return Negate(v); }

    template <typename T> constexpr Vec2T<T> operator+(const Vec2T<T>& a, const Vec2T<T>& b) { return Add(a, b); }
    template <typename T> constexpr Vec2T<T> operator-(const Vec2T<T>& a, const Vec2T<T>& b) { return Sub(a, b); }
    template <typename T> constexpr Vec2T<T> operator*(const Vec2T<T>& a, const Vec2T<T>& b) { return Mul(a, b); }
    template <typename T> constexpr Vec2T<T> operator/(const Vec2T<T>& a, const Vec2T<T>& b) { return Div(a, b); }

    template <typename T> constexpr Vec2T<T> operator+(const Vec2T<T>& a, T b) { return Add(a, b); }
    template <typename T> constexpr Vec2T<T> operator-(const Vec2T<T>& a, T b) { return Sub(a, b); }
    template <typename T> constexpr Vec2T<T> operator*(const Vec2T<T>& a, T b) { return Mul(a, b); }
    template <typename T> constexpr Vec2T<T> operator/(const Vec2T<T>& a, T b) { return Div(a, b); }

    template <typename T> constexpr Vec2T<T>& operator+=(Vec2T<T>& a, const Vec2T<T>& b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec2T<T>& operator-=(Vec2T<T>& a, const Vec2T<T>& b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec2T<T>& operator*=(Vec2T<T>& a, const Vec2T<T>& b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec2T<T>& operator/=(Vec2T<T>& a, const Vec2T<T>& b) { a = Div(a, b); return a; }

    template <typename T> constexpr Vec2T<T>& operator+=(Vec2T<T>& a, T b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec2T<T>& operator-=(Vec2T<T>& a, T b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec2T<T>& operator*=(Vec2T<T>& a, T b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec2T<T>& operator/=(Vec2T<T>& a, T b) { a = Div(a, b); return a; }

    template <typename T> constexpr bool operator<(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        if (a.x < b.x) return true;
        if (a.x > b.x) return false;
        return a.y < b.y;
    }

    template <typename T> constexpr bool operator==(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        return a.x == b.x && a.y == b.y;
    }

    template <typename T> constexpr bool operator!=(const Vec2T<T>& a, const Vec2T<T>& b)
    {
        return a.x != b.x || a.y != b.y;
    }
}
