// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    namespace internal
    {
        inline Mat44 MakeTransformMat44(const Vec4a& p, const Quat& q, const Vec3f& s)
        {
            const Vec4a cx
            {
                1 - (2 * q.y * q.y) - (2 * q.z * q.z),
                (2 * q.x * q.y) + (2 * q.w * q.z),
                (2 * q.x * q.z) - (2 * q.w * q.y),
                0.0f,
            };

            const Vec4a cy
            {
                (2 * q.x * q.y) - (2 * q.w * q.z),
                1 - (2 * q.x * q.x) - (2 * q.z * q.z),
                (2 * q.y * q.z) + (2 * q.w * q.x),
                0.0f,
            };

            const Vec4a cz
            {
                (2 * q.x * q.z) + (2 * q.w * q.y),
                (2 * q.y * q.z) - (2 * q.w * q.x),
                1 - (2 * q.x * q.x) - (2 * q.y * q.y),
                0.0f,
            };

            return
            {
                Mul(cx, s.x),
                Mul(cy, s.y),
                Mul(cz, s.z),
                SetW(p, 1.0f),
            };
        }

        inline Mat44 MakeTransformMat44(const Vec4a& p, const Quata& q, const Vec4a& s)
        {
            [[alignas(16)]] float quat[4];
            Store(quat, q.v);

            [[alignas(16)]] float scale[4];
            Store(scale, s);

            return MakeTransformMat44(p, Quat{ quat[0], quat[1], quat[2], quat[3] }, Vec3f{ scale[0], scale[2], scale[3] });
        }
    }

    inline Mat44 MakeScaleMat44(float s)
    {
        return internal::MakeTransformMat44(Vec4a_Zero, Quat_Identity, Vec3f{ s, s, s });
    }

    inline Mat44 MakeScaleMat44(const Vec3f& s)
    {
        return internal::MakeTransformMat44(Vec4a_Zero, Quat_Identity, s);
    }

    inline Mat44 MakeScaleMat44(const Vec4a& s)
    {
        return internal::MakeTransformMat44(Vec4a_Zero, Quata_Identity, s);
    }

    inline Mat44 MakeRotateMat44(const Quat& q)
    {
        return internal::MakeTransformMat44(Vec4a_Zero, q, Vec3f_One);
    }

    inline Mat44 MakeRotateMat44(const Quata& q)
    {
        return internal::MakeTransformMat44(Vec4a_Zero, q, Vec4a_One);
    }

    inline Mat44 MakeTranslateMat44(const Vec3f& p)
    {
        return internal::MakeTransformMat44(MakeVec4a(p), Quat_Identity, Vec3f_One);
    }

    inline Mat44 MakeTranslateMat44(const Vec4a& p)
    {
        return internal::MakeTransformMat44(p, Quat_Identity, Vec3f_One);
    }

    inline Mat44 MakeTransformMat44(const Vec3f& p, const Quat& q)
    {
        return internal::MakeTransformMat44(MakeVec4a(p), q, Vec3f_One);
    }

    inline Mat44 MakeTransformMat44(const Vec4a& p, const Quata& q)
    {
        return internal::MakeTransformMat44(p, q, Vec4a_One);
    }

    inline Mat44 MakeTransformMat44(const Vec3f& p, const Quat& q, const Vec3f& s)
    {
        return internal::MakeTransformMat44(MakeVec4a(p), q, s);
    }

    inline Mat44 MakeTransformMat44(const Vec4a& p, const Quata& q, const Vec4a& s)
    {
        return internal::MakeTransformMat44(p, q, s);
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    inline Mat44 Mul(const Mat44& a, const Mat44& b)
    {
        return
        {
            TransformVector(a, b.cx),
            TransformVector(a, b.cy),
            TransformVector(a, b.cz),
            TransformVector(a, b.cw),
        };
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline Mat44 ViewLH(const Vec3f& pos, const Vec3f& forward, const Vec3f& up)
    {
        return ViewLH(MakeVec4a(pos), MakeVec4a(forward), MakeVec4a(up));
    }

    inline Mat44 ViewLH(const Vec4a& pos, const Vec4a& forward, const Vec4a& up)
    {
        const Vec4a z = Normalize3(forward);
        const Vec4a x = Normalize3(Cross(up, z));
        const Vec4a y = Cross(z, x);

        const Vec4a npos = Negate(pos);

        const float dx = Dot3(x, npos);
        const float dy = Dot3(y, npos);
        const float dz = Dot3(z, npos);

        Mat44 r;
        r.cx = SetW(x, dx);
        r.cy = SetW(y, dy);
        r.cz = SetW(z, dz);
        r.cw = Vec4a_W;

        return Transpose(r);
    }

    inline Mat44 ViewRH(const Vec3f& pos, const Vec3f& forward, const Vec3f& up)
    {
        return ViewRH(MakeVec4a(pos), MakeVec4a(forward), MakeVec4a(up));
    }

    inline Mat44 ViewRH(const Vec4a& pos, const Vec4a& forward, const Vec4a& up)
    {
        const Vec4a z = Normalize3(forward);
        const Vec4a x = Normalize3(Cross(z, up));
        const Vec4a y = Cross(x, z);

        const Vec4a npos = Negate(pos);

        const float dx = Dot3(x, npos);
        const float dy = Dot3(y, npos);
        const float dz = Dot3(z, npos);

        const Mat44 r
        {
            SetW(x, dx),
            SetW(y, dy),
            SetW(z, dz),
            Vec4a_W,
        };

        return Transpose(r);
    }

    inline Mat44 OrthoLH(float minX, float maxX, float minY, float maxY, float zNear, float zFar)
    {
        const float w = maxX - minX;
        const float h = maxY - minY;
        const float d = zFar - zNear;

        return
        {
            Vec4a{ 2.0f / w, 0.0f, 0.0f, 0.0f },
            Vec4a{ 0.0f, 2.0f / h, 0.0f, 0.0f },
            Vec4a{ 0.0f, 0.0f, 1.0f / d, 0.0f },
            Vec4a{ -(minX + maxX) / w, -(minY + maxY) / h, -zNear / d, 1.0f },
        };
    }

    inline Mat44 OrthoRH(float minX, float maxX, float minY, float maxY, float zNear, float zFar)
    {
        const float w = maxX - minX;
        const float h = maxY - minY;
        const float d = zFar - zNear;

        return
        {
            Vec4a{ 2.0f / w, 0.0f, 0.0f, 0.0f },
            Vec4a{ 0.0f, 2.0f / h, 0.0f, 0.0f },
            Vec4a{ 0.0f, 0.0f, -1.0f / d, 0.0f },
            Vec4a{ -(minX + maxX) / w, -(minY + maxY) / h, -zNear / d, 1.0f },
        };
    }

    inline Mat44 PerspLH(float fov, float aspectRatio, float zNear, float zFar)
    {
        HE_ASSERT(fov > 0);
        HE_ASSERT(aspectRatio > 0);

        const float h = 1.0f / Tan(fov * 0.5f);
        const float w = h / aspectRatio;
        const float d = zFar - zNear;

        return
        {
            Vec4a{ w, 0.0f, 0.0f, 0.0f },
            Vec4a{ 0.0f, h, 0.0f, 0.0f },
            Vec4a{ 0.0f, 0.0f, zFar / d, 1.0f },
            Vec4a{ 0.0f, 0.0f, -(zFar * zNear) / d, 0.0f },
        };
    }

    inline Mat44 PerspRH(float fov, float aspectRatio, float zNear, float zFar)
    {
        HE_ASSERT(fov > 0);
        HE_ASSERT(aspectRatio > 0);

        const float h = 1.0f / Tan(fov * 0.5f);
        const float w = h / aspectRatio;

        return
        {
            Vec4a{ w, 0.0f, 0.0f, 0.0f },
            Vec4a{ 0.0f, h, 0.0f, 0.0f },
            Vec4a{ 0.0f, 0.0f, zFar / (zNear - zFar), -1.0f },
            Vec4a{ 0.0f, 0.0f, -(zFar * zNear) / (zFar - zNear), 0.0f },
        };
    }

    inline Mat44 IRZPerspLH(float fov, float aspectRatio, float zNear)
    {
        float h = 1.0f / Tan(fov * 0.5f);
        float w = h / aspectRatio;

        return
        {
            Vec4a{ w, 0.0f, 0.0f, 0.0f },
            Vec4a{ 0.0f, h, 0.0f, 0.0f },
            Vec4a{ 0.0f, 0.0f, 0.0f, 1.0f },
            Vec4a{ 0.0f, 0.0f, zNear, 0.0f },
        };
    }

    inline Mat44 IRZPerspRH(float fov, float aspectRatio, float zNear)
    {
        float h = 1.0f / Tan(fov * 0.5f);
        float w = h / aspectRatio;

        return
        {
            Vec4a{ w, 0.0f, 0.0f, 0.0f },
            Vec4a{ 0.0f, h, 0.0f, 0.0f },
            Vec4a{ 0.0f, 0.0f, 0.0f, -1.0f },
            Vec4a{ 0.0f, 0.0f, zNear, 0.0f },
        };
    }

    inline Vec4f TransformVector(const Mat44& m, const Vec4f& v)
    {
        return MakeVec4<float>(TransformVector(m, MakeVec4a(v)));
    }

    inline Vec3f TransformVector(const Mat44& m, const Vec3f& v)
    {
        return MakeVec3<float>(TransformVector3(m, MakeVec4a(v)));
    }

    inline Vec3f TransformPoint(const Mat44& m, const Vec3f& v)
    {
        return MakeVec3<float>(TransformPoint(m, MakeVec4a(v)));
    }

    inline Vec3f ProjectPoint(const Mat44& m, const Vec3f& v)
    {
        return MakeVec3<float>(ProjectPoint(m, MakeVec4a(v)));
    }

    // --------------------------------------------------------------------------------------------
    // Operators

    inline bool operator==(const Mat44& a, const Mat44& b)
    {
        return a.cx == b.cx && a.cy == b.cy && a.cz == b.cz && a.cw == b.cw;
    }

    inline bool operator!=(const Mat44& a, const Mat44& b)
    {
        return a.cx != b.cx || a.cy != b.cy || a.cz != b.cz || a.cw != b.cw;
    }
}
