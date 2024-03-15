// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/simd.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Vector Types

    template <typename T> struct Vec2 { using Type = T; static constexpr uint32_t Size = 2; T x, y; };
    template <typename T> struct Vec3 { using Type = T; static constexpr uint32_t Size = 3; T x, y, z; };
    template <typename T> struct Vec4 { using Type = T; static constexpr uint32_t Size = 4; T x, y, z, w; };

    using Vec2f = Vec2<float>;
    using Vec3f = Vec3<float>;
    using Vec4f = Vec4<float>;

    using Vec2d = Vec2<double>;
    using Vec3d = Vec3<double>;
    using Vec4d = Vec4<double>;

    using Vec2i = Vec2<int32_t>;
    using Vec3i = Vec3<int32_t>;
    using Vec4i = Vec4<int32_t>;

    using Vec2u = Vec2<uint32_t>;
    using Vec3u = Vec3<uint32_t>;
    using Vec4u = Vec4<uint32_t>;

    struct Vec4a
    {
        using Type = Simd128;
        static constexpr uint32_t Size = 4;

        Vec4a() = default;
        constexpr Vec4a(Simd128 v) noexcept : v(v) {}
        constexpr Vec4a(float x, float y, float z, float w) noexcept : v(MakeSimd128(x, y, z, w)) {}

        HE_FORCE_INLINE operator Simd128() const { return v; }

        Simd128 v;
    };

    // --------------------------------------------------------------------------------------------
    // Quaternion Types

    struct Quat { float x, y, z, w; };

    struct Quata
    {
        Quata() = default;
        constexpr Quata(Vec4a v) noexcept : v(v) {}
        constexpr Quata(Simd128 v) noexcept : v(v) {}
        constexpr Quata(float x, float y, float z, float w) noexcept : v(x, y, z, w) {}

        Vec4a v;
    };

    // --------------------------------------------------------------------------------------------
    // Matrix Types

    struct Mat44
    {
        // column vectors
        Vec4a cx, cy, cz, cw;
    };
}
