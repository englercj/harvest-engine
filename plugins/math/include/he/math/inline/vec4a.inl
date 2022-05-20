// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    template <typename T> inline Vec2<T> MakeVec2(const Vec4a& v)
    {
        alignas(16) float r[4];
        Store(r, v);
        return { static_cast<T>(r[0]), static_cast<T>(r[1]) };
    }

    template <typename T> inline Vec3<T> MakeVec3(const Vec4a& v)
    {
        alignas(16) float r[4];
        Store(r, v);
        return { static_cast<T>(r[0]), static_cast<T>(r[1]), static_cast<T>(r[2]) };
    }

    template <typename T> inline Vec4<T> MakeVec4(const Vec4a& v)
    {
        alignas(16) float r[4];
        Store(r, v);
        return { static_cast<T>(r[0]), static_cast<T>(r[1]), static_cast<T>(r[2]), static_cast<T>(r[3]) };
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    inline bool IsZeroSafe(const Vec4a& v)
    {
        const float d = Dot(v, v);
        return d > Float_ZeroSafe;
    }

    inline bool IsZeroSafe3(const Vec4a& v)
    {
        const float d = Dot3(v, v);
        return d > Float_ZeroSafe;
    }

    inline bool IsFinite(const Vec4a& v)
    {
        return !IsInfinite(v);
    }

    inline bool IsFinite3(const Vec4a& v)
    {
        return !IsInfinite3(v);
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline float LenSquared(const Vec4a& v)
    {
        return Dot(v, v);
    }

    inline float LenSquared3(const Vec4a& v)
    {
        return Dot3(v, v);
    }

    inline float Len(const Vec4a& v)
    {
        return Sqrt(LenSquared(v));
    }

    inline float Len3(const Vec4a& v)
    {
        return Sqrt(LenSquared3(v));
    }

    inline bool IsNormalized(const Vec4a& v)
    {
        return Abs(Len(v) - 1) < 40.0f * Float_Epsilon;
    }

    inline bool IsNormalized3(const Vec4a& v)
    {
        return Abs(Len3(v) - 1) < 40.0f * Float_Epsilon;
    }

    // --------------------------------------------------------------------------------------------
    // Vector operator overloads

    inline Vec4a operator-(const Vec4a& v) { return Negate(v); }

    inline Vec4a operator+(const Vec4a& a, const Vec4a& b) { return Add(a, b); }
    inline Vec4a operator-(const Vec4a& a, const Vec4a& b) { return Sub(a, b); }
    inline Vec4a operator*(const Vec4a& a, const Vec4a& b) { return Mul(a, b); }
    inline Vec4a operator/(const Vec4a& a, const Vec4a& b) { return Div(a, b); }

    inline Vec4a operator+(const Vec4a& a, float b) { return Add(a, b); }
    inline Vec4a operator-(const Vec4a& a, float b) { return Sub(a, b); }
    inline Vec4a operator*(const Vec4a& a, float b) { return Mul(a, b); }
    inline Vec4a operator/(const Vec4a& a, float b) { return Div(a, b); }

    inline Vec4a& operator+=(Vec4a& a, const Vec4a& b) { a = Add(a, b); return a; }
    inline Vec4a& operator-=(Vec4a& a, const Vec4a& b) { a = Sub(a, b); return a; }
    inline Vec4a& operator*=(Vec4a& a, const Vec4a& b) { a = Mul(a, b); return a; }
    inline Vec4a& operator/=(Vec4a& a, const Vec4a& b) { a = Div(a, b); return a; }

    inline Vec4a& operator+=(Vec4a& a, float b) { a = Add(a, b); return a; }
    inline Vec4a& operator-=(Vec4a& a, float b) { a = Sub(a, b); return a; }
    inline Vec4a& operator*=(Vec4a& a, float b) { a = Mul(a, b); return a; }
    inline Vec4a& operator/=(Vec4a& a, float b) { a = Div(a, b); return a; }

    inline bool operator<(const Vec4a& a, const Vec4a& b)
    {
        Vec4a ltCmp = Lt(a, b);
        Vec4a gtCmp = Gt(a, b);

        alignas(16) uint32_t ltMask[4];
        Store(reinterpret_cast<float*>(ltMask), ltCmp);

        alignas(16) uint32_t gtMask[4];
        Store(reinterpret_cast<float*>(gtMask), gtCmp);

        constexpr uint32_t CmpTrue = 0xffffffff;

        if (ltMask[0] == CmpTrue) return true;
        if (gtMask[0] == CmpTrue) return false;
        if (ltMask[1] == CmpTrue) return true;
        if (gtMask[1] == CmpTrue) return false;
        if (ltMask[2] == CmpTrue) return true;
        if (gtMask[2] == CmpTrue) return false;
        return ltMask[3] == CmpTrue;
    }
    inline bool operator==(const Vec4a& a, const Vec4a& b) { return All(Eq(a, b)); }
    inline bool operator!=(const Vec4a& a, const Vec4a& b) { return Any(Ne(a, b)); }
}
