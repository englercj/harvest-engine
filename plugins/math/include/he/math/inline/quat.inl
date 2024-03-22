// Copyright Chad Engler

#include "he/core/assert.h"
#include "he/core/limits.h"
#include "he/core/math.h"
#include "he/core/types.h"
#include "he/math/vec3.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    inline Quat MakeQuat(const Mat44& m)
    {
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/

        const float xx = GetX(m.cx);
        const float xy = GetY(m.cx);
        const float xz = GetZ(m.cx);
        const float yx = GetX(m.cy);
        const float yy = GetY(m.cy);
        const float yz = GetZ(m.cy);
        const float zx = GetX(m.cz);
        const float zy = GetY(m.cz);
        const float zz = GetZ(m.cz);

        const float trace = xx + yy + zz;

        if (trace > 0.0f)
        {
            const float s = 0.5f / Sqrt(trace + 1.0f);
            return
            {
                (yz - zy) * s,
                (zx - xz) * s,
                (xy - yx) * s,
                0.25f / s,
            };
        }

        if (xx > yy && xx > zz)
        {
            float s = 2.0f * Sqrt(1.0f + xx - yy - zz);
            return
            {
                0.25f * s,
                (yx + xy) / s,
                (zx + xz) / s,
                (yz - zy) / s,
            };
        }

        if (yy > zz)
        {
            float s = 2.0f * Sqrt(1.0f + yy - xx - zz);
            return
            {
                (yx + xy) / s,
                0.25f * s,
                (zy + yz) / s,
                (zx - xz) / s,
            };
        }

        float s = 2.0f * Sqrt(1.0f + zz - xx - yy );
        return
        {
            (zx + xz) / s,
            (zy + yz) / s,
            0.25f * s,
            (xy - yx) / s,
        };
    }

    inline Quat MakeQuat(const Vec3f& angles)
    {
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/

        float sr;
        float cr;
        SinCos(angles.x / 2.0f, sr, cr);

        float sp;
        float cp;
        SinCos(angles.y / 2.0f, sp, cp);

        float sy;
        float cy;
        SinCos(angles.z / 2.0f, sy, cy);

        const float cycp = cy * cp;
        const float sysp = sy * sp;

        return
        {
            (cycp * sr) + (sysp * cr),
            (sy * cp * cr) + (cy * sp * sr),
            (cy * sp * cr) - (sy * cp * sr),
            (cycp * cr) - (sysp * sr),
        };
    }

    inline Quat MakeQuat(const Vec3f& axis, float angle)
    {
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToQuaternion/

        float s;
        float c;
        SinCos(0.5f * angle, s, c);

        const Vec3f v = Mul(Normalize(axis), s);
        return { v.x, v.y, v.z, c };
    }

    inline Vec3f ToEuler(const Quat& q)
    {
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

        HE_ASSERT(IsNormalized(q));

        const float test = (q.x * q.y) + (q.z * q.w);
        constexpr float threshold = 0.4999995f;

        if (test > threshold)
        {
            const float pitch = MathConstants<float>::PiHalf;
            const float yaw = 2 * Atan2(q.x, q.w);
            return { 0.0f, pitch, yaw };
        }

        if (test < -threshold)
        {
            const float pitch = -MathConstants<float>::PiHalf;
            const float yaw = -2 * Atan2(q.x, q.w);
            return { 0.0f, pitch, yaw };
        }

        const float xx2 = 2.0f * (q.x * q.x);
        const float yy2 = 2.0f * (q.y * q.y);
        const float zz2 = 2.0f * (q.z * q.z);

        const float roll = Atan2((2.0f * q.x * q.w) - (2.0f * q.y * q.z), 1 - xx2 - zz2);
        const float pitch = Asin(2.0f * test);
        const float yaw = Atan2((2.0f * q.y * q. w) - (2.0f * q.x * q.z), 1 - yy2 - zz2);
        return { roll, pitch, yaw };
    }

    inline void ToAxisAngle(Vec3f& outAxis, float& outAngle, const Quat& q)
    {
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToAngle/

        HE_ASSERT(IsNormalized(q));

        outAngle = 2.0f * Acos(q.w);

        const float s = Sqrt(1 - (q.w * q.w));

        if (IsZeroSafe(s))
        {
            outAxis.x = q.x / s;
            outAxis.y = q.y / s;
            outAxis.z = q.z / s;
        }
        else
        {
            outAxis = Vec3f_X;
        }
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    constexpr bool IsNan(const Quat& q)
    {
        return IsNan(q.x) || IsNan(q.y) || IsNan(q.z) || IsNan(q.w);
    }

    constexpr bool IsInfinite(const Quat& q)
    {
        return IsInfinite(q.x) || IsInfinite(q.y) || IsInfinite(q.z) || IsInfinite(q.w);
    }

    constexpr bool IsFinite(const Quat& q)
    {
        return IsFinite(q.x) && IsFinite(q.y) && IsFinite(q.z) && IsFinite(q.w);
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    constexpr Quat Conjugate(const Quat& q)
    {
        return { -q.x, -q.y, -q.z, q.w };
    }

    constexpr Quat Mul(const Quat& a, const Quat& b)
    {
        return
        {
            (a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y),
            (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z),
            (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x),
            (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z),
        };
    }

    constexpr Quat Mul(const Vec3f& v, const Quat& q)
    {
        return
        {
             (v.x * q.w) + (v.y * q.z) - (v.z * q.y),
            -(v.x * q.z) + (v.y * q.w) + (v.z * q.x),
             (v.x * q.y) - (v.y * q.x) + (v.z * q.w),
            -(v.x * q.x) - (v.y * q.y) - (v.z * q.z),
        };
    }

    constexpr Quat Mul(const Quat& q, const Vec3f& v)
    {
        return
        {
             (q.w * v.x) + (q.y * v.z) - (q.z * v.y),
             (q.w * v.y) - (q.x * v.z) + (q.z * v.x),
             (q.w * v.z) + (q.x * v.y) - (q.y * v.x),
            -(q.x * v.x) - (q.y * v.y) - (q.z * v.z),
        };
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline Vec3f Rotate(const Quat& q, const Vec3f& v)
    {
        Vec3f qv = { q.x, q.y, q.z };
        Vec3f t = Cross(qv, v);
        t = Add(t, Mul(v, q.w));
        t = Cross(qv, t);
        t = Add(v, Add(t, t));
        return t;
    }

    inline Vec3f InvRotate(const Quat& q, const Vec3f& v)
    {
        Vec3f qv{ q.x, q.y, q.z };
        Vec3f t = Cross(v, qv);
        t = Add(t, Mul(v, q.w));
        t = Cross(t, qv);
        t = Add(v, Add(t, t));
        return t;
    }

    constexpr float Dot(const Quat& a, const Quat& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    }

    inline float Len(const Quat& q)
    {
        return Sqrt(Dot(q, q));
    }

    inline Quat Normalize(const Quat& q)
    {
        const float t = Rsqrt(Dot(q, q));
        return { t * q.x, t * q.y, t * q.z, t * q.w };
    }

    inline bool IsNormalized(const Quat& q)
    {
        return Abs(Len(q) - 1) < 40.0f * Limits<float>::Epsilon;
    }

    // --------------------------------------------------------------------------------------------
    // Operators

    constexpr Quat operator-(const Quat& v)
    {
        return { -v.x, -v.y, -v.z, -v.w };
    }

    constexpr Quat operator+(const Quat& a, const Quat& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }

    constexpr Quat operator-(const Quat& a, const Quat& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    }

    constexpr Quat operator*(const Quat& a, const Quat& b)
    {
        return Mul(a, b);
    }

    constexpr Quat operator*(const Quat& a, float b)
    {
        return { a.x * b, a.y * b, a.z * b, a.w * b };
    }

    constexpr Quat& operator+=(Quat& a, const Quat& b)
    {
        a = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
        return a;
    }

    constexpr Quat& operator-=(Quat& a, const Quat& b)
    {
        a = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
        return a;
    }

    constexpr Quat& operator*=(Quat& a, const Quat& b)
    {
        a = Mul(a, b);
        return a;
    }

    constexpr Quat& operator*=(Quat& a, float b)
    {
        a = { a.x * b, a.y * b, a.z * b, a.w * b };
        return a;
    }

    constexpr bool operator<(const Quat& a, const Quat& b)
    {
        if (a.x < b.x) return true;
        if (a.x > b.x) return false;
        if (a.y < b.y) return true;
        if (a.y > b.y) return false;
        if (a.z < b.z) return true;
        if (a.z > b.z) return false;
        return a.w < b.w;
    }

    constexpr bool operator==(const Quat& a, const Quat& b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }

    constexpr bool operator!=(const Quat& a, const Quat& b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
    }
}
