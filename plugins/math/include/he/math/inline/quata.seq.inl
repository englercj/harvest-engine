// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Arithmetic

    inline Quata Conjugate(const Quata& q)
    {
        return { -q.v.x, -q.v.y, -q.v.z, q.v.w };
    }

    inline Quata Mul(const Quata& a, const Quata& b)
    {
        return
        {
            (a.v.w * b.v.x) + (a.v.x * b.v.w) + (a.v.y * b.v.z) - (a.v.z * b.v.y),
            (a.v.w * b.v.y) - (a.v.x * b.v.z) + (a.v.y * b.v.w) + (a.v.z * b.v.x),
            (a.v.w * b.v.z) + (a.v.x * b.v.y) - (a.v.y * b.v.x) + (a.v.z * b.v.w),
            (a.v.w * b.v.w) - (a.v.x * b.v.x) - (a.v.y * b.v.y) - (a.v.z * b.v.z),
        };
    }

    inline Quata Mul(const Vec4a& v, const Quata& q)
    {
        return
        {
             (v.x * q.v.w) + (v.y * q.v.z) - (v.z * q.v.y),
            -(v.x * q.v.z) + (v.y * q.v.w) + (v.z * q.v.x),
             (v.x * q.v.y) - (v.y * q.v.x) + (v.z * q.v.w),
            -(v.x * q.v.x) - (v.y * q.v.y) - (v.z * q.v.z),
        };
    }

    inline Quata Mul(const Quata& q, const Vec4a& v)
    {
        return
        {
             (q.v.w * v.x) + (q.v.y * v.z) - (q.v.z * v.y),
             (q.v.w * v.y) - (q.v.x * v.z) + (q.v.z * v.x),
             (q.v.w * v.z) + (q.v.x * v.y) - (q.v.y * v.x),
            -(q.v.x * v.x) - (q.v.y * v.y) - (q.v.z * v.z),
        };
    }
}
