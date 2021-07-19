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

    template <typename T> struct Vec2T { using Type = T; static constexpr uint32_t Size = 2; T x, y; };
    template <typename T> struct Vec3T { using Type = T; static constexpr uint32_t Size = 3; T x, y, z; };
    template <typename T> struct Vec4T { using Type = T; static constexpr uint32_t Size = 4; T x, y, z, w; };

    using Vec2f = Vec2T<float>;
    using Vec3f = Vec3T<float>;
    using Vec4f = Vec4T<float>;

    using Vec2i = Vec2T<int32_t>;
    using Vec3i = Vec3T<int32_t>;
    using Vec4i = Vec4T<int32_t>;

    using Vec2u = Vec2T<uint32_t>;
    using Vec3u = Vec3T<uint32_t>;
    using Vec4u = Vec4T<uint32_t>;

    struct Vec4a
    {
        using Type = Simd128;
        static constexpr uint32_t Size = 4;

        Vec4a() = default;
        constexpr Vec4a(Simd128 v) : v(v) {}
        constexpr Vec4a(float x, float y, float z, float w) : v(MakeSimd128(x, y, z, w)) {}

        HE_FORCE_INLINE operator Simd128() const { return v; }

        Simd128 v;
    };

    // --------------------------------------------------------------------------------------------
    // Quaternion Types

    struct Quat { float x, y, z, w; };

    struct Quata
    {
        Quata() = default;
        constexpr Quata(Vec4a v) : v(v) {}
        constexpr Quata(Simd128 v) : v(v) {}
        constexpr Quata(float x, float y, float z, float w) : v(x, y, z, w) {}

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
