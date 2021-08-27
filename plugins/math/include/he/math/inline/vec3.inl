// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Construction

    template <typename T1, typename T2> constexpr Vec3T<T1> MakeVec3(const Vec2T<T2>& v, typename Vec2T<T2>::Type z)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y), static_cast<T1>(z) };
    }

    template <typename T1, typename T2> constexpr Vec3T<T1> MakeVec3(const Vec3T<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y), static_cast<T1>(v.z) };
    }

    template <typename T1, typename T2> constexpr Vec3T<T1> MakeVec3(const Vec4T<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y), static_cast<T1>(v.z) };
    }

    // --------------------------------------------------------------------------------------------
    // Component Access

    template <typename T> constexpr T GetComponent(const Vec3T<T>& v, size_t i)
    {
        HE_ASSERT(i < Vec3T<T>::Size);
        return GetPointer(v)[i];
    }

    template <typename T> constexpr Vec3T<T>& SetComponent(Vec3T<T>& v, size_t i, T s)
    {
        HE_ASSERT(i < Vec3T<T>::Size);
        GetPointer(v)[i] = s;
        return v;
    }

    template <typename T> constexpr T* GetPointer(Vec3T<T>& v)
    {
        return static_cast<T*>(&v.x);
    }

    template <typename T> constexpr const T* GetPointer(const Vec3T<T>& v)
    {
        return static_cast<const T*>(&v.x);
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    template <typename T> constexpr bool IsNan(const Vec3T<T>& v)
    {
        return IsNan(v.x) || IsNan(v.y) || IsNan(v.z);
    }

    template <typename T> constexpr bool IsInfinite(const Vec3T<T>& v)
    {
        return IsInfinite(v.x) || IsInfinite(v.y) || IsInfinite(v.z);
    }

    template <typename T> constexpr bool IsFinite(const Vec3T<T>& v)
    {
        return IsFinite(v.x) && IsFinite(v.y) && IsFinite(v.z);
    }

    template <typename T> constexpr bool IsZeroSafe(const Vec3T<T>& v)
    {
        return Dot(v, v) > Float_ZeroSafe;
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    template <typename T> constexpr Vec3T<T> Negate(const Vec3T<T>& v)
    {
        return { -v.x, -v.y, -v.z };
    }

    template <typename T> constexpr Vec3T<T> Add(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    template <typename T> constexpr Vec3T<T> Add(const Vec3T<T>& a, T b)
    {
        return { a.x + b, a.y + b, a.z + b };
    }

    template <typename T> constexpr Vec3T<T> Sub(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    template <typename T> constexpr Vec3T<T> Sub(const Vec3T<T>& a, T b)
    {
        return { a.x - b, a.y - b, a.z - b };
    }

    template <typename T> constexpr Vec3T<T> Mul(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return { a.x * b.x, a.y * b.y, a.z * b.z };
    }

    template <typename T> constexpr Vec3T<T> Mul(const Vec3T<T>& a, T b)
    {
        return { a.x * b, a.y * b, a.z * b };
    }

    template <typename T> constexpr Vec3T<T> Div(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return { a.x / b.x, a.y / b.y, a.z / b.z };
    }

    template <typename T> constexpr Vec3T<T> Div(const Vec3T<T>& a, T b)
    {
        return { a.x / b, a.y / b, a.z / b };
    }

    template <typename T> constexpr Vec3T<T> MulAdd(const Vec3T<T>& a, const Vec3T<T>& b, const Vec3T<T>& c)
    {
        return { a.x * b.x + c.x, a.y * b.y + c.y, a.z * b.z + c.z };
    }

    template <typename T> constexpr Vec3T<T> Lerp(const Vec3T<T>& a, const Vec3T<T>& b, float t)
    {
        return
        {
            Lerp(a.x, b.x, t),
            Lerp(a.y, b.y, t),
            Lerp(a.z, b.z, t),
        };
    }

    template <typename T> constexpr Vec3T<T> SmoothStep(float a, float b, const Vec3T<T>& t)
    {
        return
        {
            SmoothStep(a, b, t.x),
            SmoothStep(a, b, t.y),
            SmoothStep(a, b, t.z),
        };
    }

    template <typename T> constexpr Vec3T<T> Abs(const Vec3T<T>& v)
    {
        return { Abs(v.x), Abs(v.y), Abs(v.z) };
    }

    template <typename T> constexpr Vec3T<T> Rcp(const Vec3T<T>& v)
    {
        return { 1.0f / v.x, 1.0f / v.y, 1.0f / v.z };
    }

    template <typename T> constexpr Vec3T<T> RcpSafe(const Vec3T<T>& v)
    {
        return
        {
            IsZeroSafe(v.x) ? 1.0f / v.x : 0.0f,
            IsZeroSafe(v.y) ? 1.0f / v.y : 0.0f,
            IsZeroSafe(v.z) ? 1.0f / v.z : 0.0f,
        };
    }

    template <typename T> inline Vec3T<T> Sqrt(const Vec3T<T>& v)
    {
        return { Sqrt(v.x), Sqrt(v.y), Sqrt(v.z) };
    }

    template <typename T> inline Vec3T<T> Rsqrt(const Vec3T<T>& v)
    {
        return { Rsqrt(v.x), Rsqrt(v.y), Rsqrt(v.z) };
    }

    // --------------------------------------------------------------------------------------------
    // Selection

    template <typename T> constexpr Vec3T<T> Min(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return { Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z) };
    }

    template <typename T> constexpr Vec3T<T> Max(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return { Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z) };
    }

    template <typename T> constexpr Vec3T<T> Clamp(const Vec3T<T>& v, const Vec3T<T>& lo, const Vec3T<T>& hi)
    {
        return { Clamp(v.x, lo.x, hi.x), Clamp(v.y, lo.y, hi.y), Clamp(v.z, lo.z, hi.z) };
    }

    template <typename T> constexpr T HMin(const Vec3T<T>& v)
    {
        T xy = Min(v.x, v.y);
        return Min(xy, v.z);
    }

    template <typename T> constexpr T HMax(const Vec3T<T>& v)
    {
        T xy = Max(v.x, v.y);
        return Max(xy, v.z);
    }

    template <typename T> constexpr T HAdd(const Vec3T<T>& v)
    {
        return (v.x + v.y) + v.z;
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    template <typename T> constexpr T Dot(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }

    template <typename T> constexpr Vec3T<T> Cross(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return { (a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x) };
    }

    template <typename T> constexpr T LenSquared(const Vec3T<T>& v)
    {
        return Dot(v, v);
    }

    template <typename T> inline T Len(const Vec3T<T>& v)
    {
        return Sqrt(LenSquared(v));
    }

    template <typename T> inline Vec3T<T> Normalize(const Vec3T<T>& v)
    {
        return Mul(v, Rsqrt(LenSquared(v)));
    }

    template <typename T> inline Vec3T<T> NormalizeSafe(const Vec3T<T>& v)
    {
        return IsZeroSafe(v) ? Normalize(v) : v;
    }

    template <typename T> inline bool IsNormalized(const Vec3T<T>& v)
    {
        return Abs(Len(v) - 1) < 40.0f * Float_Epsilon;
    }

    // --------------------------------------------------------------------------------------------
    // Operators

    template <typename T> constexpr Vec3T<T> operator-(const Vec3T<T>& v) { return Negate(v); }

    template <typename T> constexpr Vec3T<T> operator+(const Vec3T<T>& a, const Vec3T<T>& b) { return Add(a, b); }
    template <typename T> constexpr Vec3T<T> operator-(const Vec3T<T>& a, const Vec3T<T>& b) { return Sub(a, b); }
    template <typename T> constexpr Vec3T<T> operator*(const Vec3T<T>& a, const Vec3T<T>& b) { return Mul(a, b); }
    template <typename T> constexpr Vec3T<T> operator/(const Vec3T<T>& a, const Vec3T<T>& b) { return Div(a, b); }

    template <typename T> constexpr Vec3T<T> operator+(const Vec3T<T>& a, T b) { return Add(a, b); }
    template <typename T> constexpr Vec3T<T> operator-(const Vec3T<T>& a, T b) { return Sub(a, b); }
    template <typename T> constexpr Vec3T<T> operator*(const Vec3T<T>& a, T b) { return Mul(a, b); }
    template <typename T> constexpr Vec3T<T> operator/(const Vec3T<T>& a, T b) { return Div(a, b); }

    template <typename T> constexpr Vec3T<T>& operator+=(Vec3T<T>& a, const Vec3T<T>& b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec3T<T>& operator-=(Vec3T<T>& a, const Vec3T<T>& b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec3T<T>& operator*=(Vec3T<T>& a, const Vec3T<T>& b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec3T<T>& operator/=(Vec3T<T>& a, const Vec3T<T>& b) { a = Div(a, b); return a; }

    template <typename T> constexpr Vec3T<T>& operator+=(Vec3T<T>& a, T b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec3T<T>& operator-=(Vec3T<T>& a, T b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec3T<T>& operator*=(Vec3T<T>& a, T b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec3T<T>& operator/=(Vec3T<T>& a, T b) { a = Div(a, b); return a; }

    template <typename T> constexpr bool operator<(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        if (a.x < b.x) return true;
        if (a.x > b.x) return false;
        if (a.y < b.y) return true;
        if (a.y > b.y) return false;
        return a.z < b.z;
    }

    template <typename T> constexpr bool operator==(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    template <typename T> constexpr bool operator!=(const Vec3T<T>& a, const Vec3T<T>& b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
}
