// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Construction

    template <typename T1, typename T2> constexpr Vec3<T1> MakeVec3(const Vec2<T2>& v, typename Vec2<T2>::Type z)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y), static_cast<T1>(z) };
    }

    template <typename T1, typename T2> constexpr Vec3<T1> MakeVec3(const Vec3<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y), static_cast<T1>(v.z) };
    }

    template <typename T1, typename T2> constexpr Vec3<T1> MakeVec3(const Vec4<T2>& v)
    {
        return { static_cast<T1>(v.x), static_cast<T1>(v.y), static_cast<T1>(v.z) };
    }

    // --------------------------------------------------------------------------------------------
    // Component Access

    template <typename T> constexpr T GetComponent(const Vec3<T>& v, size_t i)
    {
        HE_ASSERT(i < Vec3<T>::Size);
        return GetPointer(v)[i];
    }

    template <typename T> constexpr Vec3<T>& SetComponent(Vec3<T>& v, size_t i, T s)
    {
        HE_ASSERT(i < Vec3<T>::Size);
        GetPointer(v)[i] = s;
        return v;
    }

    template <typename T> constexpr T* GetPointer(Vec3<T>& v)
    {
        return static_cast<T*>(&v.x);
    }

    template <typename T> constexpr const T* GetPointer(const Vec3<T>& v)
    {
        return static_cast<const T*>(&v.x);
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    template <typename T> constexpr bool IsNan(const Vec3<T>& v)
    {
        return IsNan(v.x) || IsNan(v.y) || IsNan(v.z);
    }

    template <typename T> constexpr bool IsInfinite(const Vec3<T>& v)
    {
        return IsInfinite(v.x) || IsInfinite(v.y) || IsInfinite(v.z);
    }

    template <typename T> constexpr bool IsFinite(const Vec3<T>& v)
    {
        return IsFinite(v.x) && IsFinite(v.y) && IsFinite(v.z);
    }

    template <typename T> constexpr bool IsZeroSafe(const Vec3<T>& v)
    {
        return Dot(v, v) > Limits<T>::ZeroSafe;
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    template <typename T> constexpr Vec3<T> Negate(const Vec3<T>& v)
    {
        return { -v.x, -v.y, -v.z };
    }

    template <typename T> constexpr Vec3<T> Add(const Vec3<T>& a, const Vec3<T>& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    template <typename T> constexpr Vec3<T> Add(const Vec3<T>& a, T b)
    {
        return { a.x + b, a.y + b, a.z + b };
    }

    template <typename T> constexpr Vec3<T> Sub(const Vec3<T>& a, const Vec3<T>& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    template <typename T> constexpr Vec3<T> Sub(const Vec3<T>& a, T b)
    {
        return { a.x - b, a.y - b, a.z - b };
    }

    template <typename T> constexpr Vec3<T> Mul(const Vec3<T>& a, const Vec3<T>& b)
    {
        return { a.x * b.x, a.y * b.y, a.z * b.z };
    }

    template <typename T> constexpr Vec3<T> Mul(const Vec3<T>& a, T b)
    {
        return { a.x * b, a.y * b, a.z * b };
    }

    template <typename T> constexpr Vec3<T> Div(const Vec3<T>& a, const Vec3<T>& b)
    {
        return { a.x / b.x, a.y / b.y, a.z / b.z };
    }

    template <typename T> constexpr Vec3<T> Div(const Vec3<T>& a, T b)
    {
        return { a.x / b, a.y / b, a.z / b };
    }

    template <typename T> constexpr Vec3<T> MulAdd(const Vec3<T>& a, const Vec3<T>& b, const Vec3<T>& c)
    {
        return { a.x * b.x + c.x, a.y * b.y + c.y, a.z * b.z + c.z };
    }

    template <typename T> constexpr Vec3<T> Lerp(const Vec3<T>& a, const Vec3<T>& b, T t)
    {
        return
        {
            Lerp(a.x, b.x, t),
            Lerp(a.y, b.y, t),
            Lerp(a.z, b.z, t),
        };
    }

    template <typename T> constexpr Vec3<T> SmoothStep(T a, T b, const Vec3<T>& t)
    {
        return
        {
            SmoothStep(a, b, t.x),
            SmoothStep(a, b, t.y),
            SmoothStep(a, b, t.z),
        };
    }

    template <typename T> constexpr Vec3<T> Abs(const Vec3<T>& v)
    {
        return { Abs(v.x), Abs(v.y), Abs(v.z) };
    }

    template <typename T> constexpr Vec3<T> Rcp(const Vec3<T>& v)
    {
        return { 1.0f / v.x, 1.0f / v.y, 1.0f / v.z };
    }

    template <typename T> constexpr Vec3<T> RcpSafe(const Vec3<T>& v)
    {
        return
        {
            IsZeroSafe(v.x) ? 1.0f / v.x : 0.0f,
            IsZeroSafe(v.y) ? 1.0f / v.y : 0.0f,
            IsZeroSafe(v.z) ? 1.0f / v.z : 0.0f,
        };
    }

    template <typename T> inline Vec3<T> Sqrt(const Vec3<T>& v)
    {
        return { Sqrt(v.x), Sqrt(v.y), Sqrt(v.z) };
    }

    template <typename T> inline Vec3<T> Rsqrt(const Vec3<T>& v)
    {
        return { Rsqrt(v.x), Rsqrt(v.y), Rsqrt(v.z) };
    }

    // --------------------------------------------------------------------------------------------
    // Selection

    template <typename T> constexpr Vec3<T> Min(const Vec3<T>& a, const Vec3<T>& b)
    {
        return { Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z) };
    }

    template <typename T> constexpr Vec3<T> Max(const Vec3<T>& a, const Vec3<T>& b)
    {
        return { Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z) };
    }

    template <typename T> constexpr Vec3<T> Clamp(const Vec3<T>& v, const Vec3<T>& lo, const Vec3<T>& hi)
    {
        return { Clamp(v.x, lo.x, hi.x), Clamp(v.y, lo.y, hi.y), Clamp(v.z, lo.z, hi.z) };
    }

    template <typename T> constexpr T HMin(const Vec3<T>& v)
    {
        T xy = Min(v.x, v.y);
        return Min(xy, v.z);
    }

    template <typename T> constexpr T HMax(const Vec3<T>& v)
    {
        T xy = Max(v.x, v.y);
        return Max(xy, v.z);
    }

    template <typename T> constexpr T HAdd(const Vec3<T>& v)
    {
        return (v.x + v.y) + v.z;
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    template <typename T> constexpr T Dot(const Vec3<T>& a, const Vec3<T>& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }

    template <typename T> constexpr Vec3<T> Cross(const Vec3<T>& a, const Vec3<T>& b)
    {
        return { (a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x) };
    }

    template <typename T> constexpr T LenSquared(const Vec3<T>& v)
    {
        return Dot(v, v);
    }

    template <typename T> inline T Len(const Vec3<T>& v)
    {
        return Sqrt(LenSquared(v));
    }

    template <typename T> inline Vec3<T> Normalize(const Vec3<T>& v)
    {
        return Mul(v, Rsqrt(LenSquared(v)));
    }

    template <typename T> inline Vec3<T> NormalizeSafe(const Vec3<T>& v)
    {
        return IsZeroSafe(v) ? Normalize(v) : v;
    }

    template <typename T> inline bool IsNormalized(const Vec3<T>& v)
    {
        return Abs(Len(v) - 1) < (T(40) * Limits<T>::Epsilon);
    }

    // --------------------------------------------------------------------------------------------
    // Operators

    template <typename T> constexpr Vec3<T> operator-(const Vec3<T>& v) { return Negate(v); }

    template <typename T> constexpr Vec3<T> operator+(const Vec3<T>& a, const Vec3<T>& b) { return Add(a, b); }
    template <typename T> constexpr Vec3<T> operator-(const Vec3<T>& a, const Vec3<T>& b) { return Sub(a, b); }
    template <typename T> constexpr Vec3<T> operator*(const Vec3<T>& a, const Vec3<T>& b) { return Mul(a, b); }
    template <typename T> constexpr Vec3<T> operator/(const Vec3<T>& a, const Vec3<T>& b) { return Div(a, b); }

    template <typename T> constexpr Vec3<T> operator+(const Vec3<T>& a, T b) { return Add(a, b); }
    template <typename T> constexpr Vec3<T> operator-(const Vec3<T>& a, T b) { return Sub(a, b); }
    template <typename T> constexpr Vec3<T> operator*(const Vec3<T>& a, T b) { return Mul(a, b); }
    template <typename T> constexpr Vec3<T> operator/(const Vec3<T>& a, T b) { return Div(a, b); }

    template <typename T> constexpr Vec3<T>& operator+=(Vec3<T>& a, const Vec3<T>& b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec3<T>& operator-=(Vec3<T>& a, const Vec3<T>& b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec3<T>& operator*=(Vec3<T>& a, const Vec3<T>& b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec3<T>& operator/=(Vec3<T>& a, const Vec3<T>& b) { a = Div(a, b); return a; }

    template <typename T> constexpr Vec3<T>& operator+=(Vec3<T>& a, T b) { a = Add(a, b); return a; }
    template <typename T> constexpr Vec3<T>& operator-=(Vec3<T>& a, T b) { a = Sub(a, b); return a; }
    template <typename T> constexpr Vec3<T>& operator*=(Vec3<T>& a, T b) { a = Mul(a, b); return a; }
    template <typename T> constexpr Vec3<T>& operator/=(Vec3<T>& a, T b) { a = Div(a, b); return a; }

    template <typename T> constexpr bool operator<(const Vec3<T>& a, const Vec3<T>& b)
    {
        if (a.x < b.x) return true;
        if (a.x > b.x) return false;
        if (a.y < b.y) return true;
        if (a.y > b.y) return false;
        return a.z < b.z;
    }

    template <typename T> constexpr bool operator==(const Vec3<T>& a, const Vec3<T>& b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    template <typename T> constexpr bool operator!=(const Vec3<T>& a, const Vec3<T>& b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
}
