// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    inline Quata MakeQuata(const Mat44& m)
    {
        // TODO: Optimize
        const Quat q = MakeQuat(m);
        return Quata{ q.x, q.y, q.z, q.w };
    }

    inline Quata MakeQuata(const Vec4a& angles)
    {
        // TODO: Optimize
        const Quat q = MakeQuat(MakeVec3<float>(angles));
        return Quata{ q.x, q.y, q.z, q.w };
    }

    inline Quata MakeQuata(const Vec4a& axis, float angle)
    {
        const float halfAngle = 0.5f * angle;
        const float s = Sin(halfAngle);
        const float c = Cos(halfAngle);

        return Quata{ SetW(Mul(Splat(s), axis), c) };
    }

    inline Vec4a ToEuler(const Quata& q)
    {
        alignas(16) float v[4];
        Store(v, q.v);

        return MakeVec4a(ToEuler(Quat{ v[0], v[1], v[2], v[3] }));
    }

    inline void ToAxisAngle(Vec4a& outAxis, float& outAngle, const Quata& q)
    {
        Vec4a axis{ q.v };
        if (IsZeroSafe3(axis))
            axis = Normalize3(q.v);

        outAxis = axis;
        outAngle = 2.0f * Acos(GetW(q.v));
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    inline bool IsNan(const Quata& q)
    {
        return IsNan(q.v);
    }

    inline bool IsInfinite(const Quata& q)
    {
        return IsInfinite(q.v);
    }

    inline bool IsFinite(const Quata& q)
    {
        return IsFinite(q.v);
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline Vec4a Rotate(const Quata& q, const Vec4a& v)
    {
        Vec4a qw = SplatW(q.v);
        Vec4a t = Cross(q.v, v);
        t = Add(t, Mul(v, qw));
        t = Cross(q.v, t);
        t = Add(v, Add(t, t));
        return t;
    }

    inline Vec4a InvRotate(const Quata& q, const Vec4a& v)
    {
        Vec4a qw = SplatW(q.v);
        Vec4a t = Cross(v, q.v);
        t = Add(t, Mul(v, qw));
        t = Cross(t, q.v);
        t = Add(v, Add(t, t));
        return t;
    }

    inline float Dot(const Quata& a, const Quata& b)
    {
        return Dot(a.v, b.v);
    }

    inline float Len(const Quata& q)
    {
        return Len(q.v);
    }

    inline Quata Normalize(const Quata& q)
    {
        return { Normalize(q.v) };
    }

    inline bool IsNormalized(const Quata& q)
    {
        return IsNormalized(q.v);
    }

    // --------------------------------------------------------------------------------------------
    // Operators

    inline Quata operator-(const Quata& q) { return { -q.v }; }

    inline Quata operator+(const Quata& a, const Quata& b) { return { a.v + b.v }; }
    inline Quata operator-(const Quata& a, const Quata& b) { return { a.v - b.v }; }
    inline Quata operator*(const Quata& a, const Quata& b) { return { a.v * b.v }; }

    inline Quata operator*(const Quata& a, float b) { return { a.v * Splat(b) }; }

    inline Quata& operator+=(Quata& a, const Quata& b) { a.v += b.v; return a; }
    inline Quata& operator-=(Quata& a, const Quata& b) { a.v -= b.v; return a; }
    inline Quata& operator*=(Quata& a, const Quata& b) { a.v *= b.v; return a; }

    inline Quata& operator*=(Quata& a, float b) { a.v *= Splat(b); return a; }

    inline bool operator<(const Quata& a, const Quata& b) { return a.v < b.v; }
    inline bool operator==(const Quata& a, const Quata& b) { return a.v == b.v; }
    inline bool operator!=(const Quata& a, const Quata& b) { return a.v != b.v; }
}
