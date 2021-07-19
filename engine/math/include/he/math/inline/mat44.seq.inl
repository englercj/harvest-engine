// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Arithmetic

    inline float Det33(float a, float b, float c, float d, float e, float f, float g, float h, float i)
    {
        return (a * e * i) + (b * f * g) + (c * d * h) - (c * e * g) - (b * d * i) - (a * f * h);
    }

    inline float Determinant(const Mat44& m)
    {
        const Vec4f x = MakeVec4<float>(m.cx);
        const Vec4f y = MakeVec4<float>(m.cy);
        const Vec4f z = MakeVec4<float>(m.cz);
        const Vec4f w = MakeVec4<float>(m.cw);

        return
            x.x * Det33(y.y, y.z, y.w, z.y, z.z, z.w, w.y, w.z, w.w) -
            x.y * Det33(y.x, y.z, y.w, z.x, z.z, z.w, w.x, w.z, w.w) +
            x.z * Det33(y.x, y.y, y.w, z.x, z.y, z.w, w.x, w.y, w.w) -
            x.w * Det33(y.x, y.y, y.z, z.x, z.y, z.z, w.x, w.y, w.z);
    }

    inline Mat44 Adjoint(const Mat44& m)
    {
        Vec4f x = MakeVec4<float>(m.cx);
        Vec4f y = MakeVec4<float>(m.cy);
        Vec4f z = MakeVec4<float>(m.cz);
        Vec4f w = MakeVec4<float>(m.cw);

        return
        {
            {
                 Det33(y.y, y.z, y.w, z.y, z.z, z.w, w.y, w.z, w.w),      // cofact(a0)
                -Det33(x.y, x.z, x.w, z.y, z.z, z.w, w.y, w.z, w.w),      // cofact(b0)
                 Det33(x.y, x.z, x.w, y.y, y.z, y.w, w.y, w.z, w.w),      // cofact(c0)
                -Det33(x.y, x.z, x.w, y.y, y.z, y.w, z.y, z.z, z.w),      // cofact(d0)
            },
            {
                -Det33(y.x, y.z, y.w, z.x, z.z, z.w, w.x, w.z, w.w),      // cofact(a1)
                 Det33(x.x, x.z, x.w, z.x, z.z, z.w, w.x, w.z, w.w),      // cofact(b1)
                -Det33(x.x, x.z, x.w, y.x, y.z, y.w, w.x, w.z, w.w),      // cofact(c1)
                 Det33(x.x, x.z, x.w, y.x, y.z, y.w, z.x, z.z, z.w),      // cofact(d1)
            },
            {
                 Det33(y.x, y.y, y.w, z.x, z.y, z.w, w.x, w.y, w.w),      // cofact(a2)
                -Det33(x.x, x.y, x.w, z.x, z.y, z.w, w.x, w.y, w.w),      // cofact(b2)
                 Det33(x.x, x.y, x.w, y.x, y.y, y.w, w.x, w.y, w.w),      // cofact(c2)
                -Det33(x.x, x.y, x.w, y.x, y.y, y.w, z.x, z.y, z.w),      // cofact(d2)
            },
            {
                -Det33(y.x, y.y, y.z, z.x, z.y, z.z, w.x, w.y, w.z),      // cofact(a3)
                 Det33(x.x, x.y, x.z, z.x, z.y, z.z, w.x, w.y, w.z),      // cofact(b3)
                -Det33(x.x, x.y, x.z, y.x, y.y, y.z, w.x, w.y, w.z),      // cofact(c3)
                 Det33(x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z),      // cofact(d3)
            },
        };
    }

    inline Mat44 Inverse(const Mat44& m)
    {
        float det = Determinant(m);
        return Mul(Adjoint(m), 1.0f / det);
    }

    inline Mat44 InverseTransform(const Mat44& m)
    {
        return Inverse(m);
    }

    inline Mat44 InverseTransformNoScale(const Mat44& m)
    {
        return Inverse(m);
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline Mat44 Transpose(const Mat44& m)
    {
        return
        {
            { m.cx.x, m.cy.x, m.cz.x, m.cw.x },
            { m.cx.y, m.cy.y, m.cz.y, m.cw.y },
            { m.cx.z, m.cy.z, m.cz.z, m.cw.z },
            { m.cx.w, m.cy.w, m.cz.w, m.cw.w },
        };
    }

    inline Vec4a TransformVector(const Mat44& m, const Vec4a& v)
    {
        return
        {
            (m.cx.x * v.x) + (m.cy.x * v.y) + (m.cz.x * v.z) + (m.cw.x * v.w),
            (m.cx.y * v.x) + (m.cy.y * v.y) + (m.cz.y * v.z) + (m.cw.y * v.w),
            (m.cx.z * v.x) + (m.cy.z * v.y) + (m.cz.z * v.z) + (m.cw.z * v.w),
            (m.cx.w * v.x) + (m.cy.w * v.y) + (m.cz.w * v.z) + (m.cw.w * v.w),
        };
    }

    inline Vec4a TransformVector3(const Mat44& m, const Vec4a& v)
    {
        return
        {
            (m.cx.x * v.x) + (m.cy.x * v.y) + (m.cz.x * v.z),
            (m.cx.y * v.x) + (m.cy.y * v.y) + (m.cz.y * v.z),
            (m.cx.z * v.x) + (m.cy.z * v.y) + (m.cz.z * v.z),
            0,
        };
    }

    inline Vec4a TransformPoint(const Mat44& m, const Vec4a& v)
    {
        return
        {
            (m.cx.x * v.x) + (m.cy.x * v.y) + (m.cz.x * v.z) + m.cw.x,
            (m.cx.y * v.x) + (m.cy.y * v.y) + (m.cz.y * v.z) + m.cw.y,
            (m.cx.z * v.x) + (m.cy.z * v.y) + (m.cz.z * v.z) + m.cw.z,
            (m.cx.w * v.x) + (m.cy.w * v.y) + (m.cz.w * v.z) + m.cw.w,
        };
    }

    inline Vec4a ProjectPoint(const Mat44& m, const Vec4a& v)
    {
        const float invW = 1.0f / ((m.cx.w * v.x) + (m.cy.w * v.y) + (m.cz.w * v.z) + m.cw.w);

        return
        {
            invW * ((m.cx.x * v.x) + (m.cy.x * v.y) + (m.cz.x * v.z) + m.cw.x),
            invW * ((m.cx.y * v.x) + (m.cy.y * v.y) + (m.cz.y * v.z) + m.cw.y),
            invW * ((m.cx.z * v.x) + (m.cy.z * v.y) + (m.cz.z * v.z) + m.cw.z),
            1,
        };
    }
}
