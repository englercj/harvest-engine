// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Construction

    template <typename T1, typename T2> constexpr Vec2<T1> MakeVec2(const Vec2<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y) };
    }

    template <typename T1, typename T2> constexpr Vec2<T1> MakeVec2(const Vec3<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y) };
    }

    template <typename T1, typename T2> constexpr Vec2<T1> MakeVec2(const Vec4<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y) };
    }

    // --------------------------------------------------------------------------------------------
    // Component Access

    template <typename T> constexpr T GetComponent(const Vec2<T>& v, size_t i)
    {
        HE_ASSERT(i < Vec2<T>::Size);
        return GetPointer(v)[i];
    }

    template <typename T> constexpr Vec2<T>& SetComponent(Vec2<T>& v, size_t i, T s)
    {
        HE_ASSERT(i < Vec2<T>::Size);
        GetPointer(v)[i] = s;
        return v;
    }

    template <typename T> constexpr T* GetPointer(Vec2<T>& v)
    {
        return static_cast<T*>(&v.x);
    }

    template <typename T> constexpr const T* GetPointer(const Vec2<T>& v)
    {
        return static_cast<const T*>(&v.x);
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    template <typename T> constexpr bool IsNan(const Vec2<T>& v)
    {
        return IsNan(v.x) || IsNan(v.y);
    }

    template <typename T> constexpr bool IsInfinite(const Vec2<T>& v)
    {
        return IsInfinite(v.x) || IsInfinite(v.y);
    }

    template <typename T> constexpr bool IsFinite(const Vec2<T>& v)
    {
        return IsFinite(v.x) && IsFinite(v.y);
    }

    template <typename T> constexpr bool IsZeroSafe(const Vec2<T>& v)
    {
        return Dot(v, v) > Limits<T>::ZeroSafe;
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    template <typename T> constexpr Vec2<T> Negate(const Vec2<T>& v)
    {
        return { -v.x, -v.y };
    }

    template <typename T> constexpr Vec2<T> Add(const Vec2<T>& a, const Vec2<T>& b)
    {
        return { a.x + b.x, a.y + b.y };
    }

    template <typename T> constexpr Vec2<T> Add(const Vec2<T>& a, T b)
    {
        return { a.x + b, a.y + b };
    }

    template <typename T> constexpr Vec2<T> Sub(const Vec2<T>& a, const Vec2<T>& b)
    {
        return { a.x - b.x, a.y - b.y };
    }

    template <typename T> constexpr Vec2<T> Sub(const Vec2<T>& a, T b)
    {
        return { a.x - b, a.y - b };
    }

    template <typename T> constexpr Vec2<T> Mul(const Vec2<T>& a, const Vec2<T>& b)
    {
        return { a.x * b.x, a.y * b.y };
    }

    template <typename T> constexpr Vec2<T> Mul(const Vec2<T>& a, T b)
    {
        return { a.x * b, a.y * b };
    }

    template <typename T> constexpr Vec2<T> Div(const Vec2<T>& a, const Vec2<T>& b)
    {
        return { a.x / b.x, a.y / b.y };
    }

    template <typename T> constexpr Vec2<T> Div(const Vec2<T>& a, T b)
    {
        return { a.x / b, a.y / b };
    }

    template <typename T> constexpr Vec2<T> MulAdd(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& c)
    {
        return { a.x * b.x + c.x, a.y * b.y + c.y };
    }

    template <typename T> constexpr Vec2<T> Lerp(const Vec2<T>& a, const Vec2<T>& b, T t)
    {
        return
        {
            Lerp(a.x, b.x, t),
            Lerp(a.y, b.y, t),
        };
    }

    template <typename T> constexpr Vec2<T> SmoothStep(T a, T b, const Vec2<T>& t)
    {
        return
        {
            SmoothStep(a, b, t.x),
            SmoothStep(a, b, t.y),
        };
    }

    template <typename T> constexpr Vec2<T> Abs(const Vec2<T>& v)
    {
        return { Abs(v.x), Abs(v.y) };
    }

    template <typename T> constexpr Vec2<T> Rcp(const Vec2<T>& v)
    {
        return { 1.0f / v.x, 1.0f / v.y };
    }

    template <typename T> constexpr Vec2<T> RcpSafe(const Vec2<T>& v)
    {
        return
        {
            IsZeroSafe(v.x) ? 1.0f / v.x : 0.0f,
            IsZeroSafe(v.y) ? 1.0f / v.y : 0.0f,
        };
    }

    template <typename T> inline Vec2<T> Sqrt(const Vec2<T>& v)
    {
        return { Sqrt(v.x), Sqrt(v.y) };
    }

    template <typename T> inline Vec2<T> Rsqrt(const Vec2<T>& v)
    {
        return { Rsqrt(v.x), Rsqrt(v.y) };
    }

    // --------------------------------------------------------------------------------------------
    // Selection

    template <typename T> constexpr Vec2<T> Min(const Vec2<T>& a, const Vec2<T>& b)
    {
        return { Min(a.x, b.x), Min(a.y, b.y) };
    }

    template <typename T> constexpr Vec2<T> Max(const Vec2<T>& a, const Vec2<T>& b)
    {
        return { Max(a.x, b.x), Max(a.y, b.y) };
    }

    template <typename T> constexpr Vec2<T> Clamp(const Vec2<T>& v, const Vec2<T>& lo, const Vec2<T>& hi)
    {
        return { Clamp(v.x, lo.x, hi.x), Clamp(v.y, lo.y, hi.y) };
    }

    template <typename T> constexpr T HMin(const Vec2<T>& v)
    {
        return Min(v.x, v.y);
    }

    template <typename T> constexpr T HMax(const Vec2<T>& v)
    {
        return Max(v.x, v.y);
    }

    template <typename T> constexpr T HAdd(const Vec2<T>& v)
    {
        return v.x + v.y;
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    template <typename T> constexpr T Dot(const Vec2<T>& a, const Vec2<T>& b)
    {
        return (a.x * b.x) + (a.y * b.y);
    }

    template <typename T> constexpr T LenSquared(const Vec2<T>& v)
    {
        return Dot(v, v);
    }

    template <typename T> inline T Len(const Vec2<T>& v)
    {
        return Sqrt(LenSquared(v));
    }

    template <typename T> inline Vec2<T> Normalize(const Vec2<T>& v)
    {
        return Mul(v, Rsqrt(LenSquared(v)));
    }

    template <typename T> inline Vec2<T> NormalizeSafe(const Vec2<T>& v)
    {
        return IsZeroSafe(v) ? Normalize(v) : v;
    }

    template <typename T> inline bool IsNormalized(const Vec2<T>& v)
    {
        return Abs(Len(v) - 1) < (T(40) * Limits<T>::Epsilon);
    }

    // --------------------------------------------------------------------------------------------
    // Operators

    template <typename T> constexpr Vec2<T> operator-(const Vec2<T>& v) { return Negate(v); }

    template <typename T> constexpr Vec2<T> operator+(const Vec2<T>& a, const Vec2<T>& b) { return Add(a, b); }
    template <typename T> constexpr Vec2<T> operator-(const Vec2<T>& a, const Vec2<T>& b) { return Sub(a, b); }
    template <typename T> constexpr Vec2<T> operator*(const Vec2<T>& a, const Vec2<T>& b) { return Mul(a, b); }
    template <typename T> constexpr Vec2<T> operator/(const Vec2<T>& a, const Vec2<T>& b) { return Div(a, b); }

    template <typename T> constexpr Vec2<T> operator+(const Vec2<T>& a, T b) { return Add(a, b); }
    template <typename T> constexpr Vec2<T> operator-(const Vec2<T>& a, T b) { return Sub(a, b); }
    template <typename T> constexpr Vec2<T> operator*(const Vec2<T>& a, T b) { return Mul(a, b); }
    template <typename T> constexpr Vec2<T> operator/(const Vec2<T>& a, T b) { return Div(a, b); }

    template <typename T> constexpr Vec2<T>& operator+=(Vec2<T>& a, const Vec2<T>& b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec2<T>& operator-=(Vec2<T>& a, const Vec2<T>& b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec2<T>& operator*=(Vec2<T>& a, const Vec2<T>& b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec2<T>& operator/=(Vec2<T>& a, const Vec2<T>& b) { a = Div(a, b); return a; }

    template <typename T> constexpr Vec2<T>& operator+=(Vec2<T>& a, T b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec2<T>& operator-=(Vec2<T>& a, T b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec2<T>& operator*=(Vec2<T>& a, T b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec2<T>& operator/=(Vec2<T>& a, T b) { a = Div(a, b); return a; }

    template <typename T> constexpr bool operator<(const Vec2<T>& a, const Vec2<T>& b)
    {
        if (a.x < b.x) return true;
        if (a.x > b.x) return false;
        return a.y < b.y;
    }

    template <typename T> constexpr bool operator==(const Vec2<T>& a, const Vec2<T>& b)
    {
        return a.x == b.x && a.y == b.y;
    }

    template <typename T> constexpr bool operator!=(const Vec2<T>& a, const Vec2<T>& b)
    {
        return a.x != b.x || a.y != b.y;
    }
}
