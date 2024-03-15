// Copyright Chad Engler

#pragma once

#include "he/core/limits.h"
#include "he/core/utils.h"
#include "he/math/types.h"

namespace he
{
    inline constexpr Vec4a Vec4a_AbsMask{ BitCast<float>(0x7fffffff), BitCast<float>(0x7fffffff), BitCast<float>(0x7fffffff), BitCast<float>(0x7fffffff) };
    inline constexpr Vec4a Vec4a_Infinity{ Limits<float>::Infinity, Limits<float>::Infinity, Limits<float>::Infinity, Limits<float>::Infinity };
    inline constexpr Vec4a Vec4a_Zero{ 0.0f, 0.0f, 0.0f, 0.0f };
    inline constexpr Vec4a Vec4a_One{ 1.0f, 1.0f, 1.0f, 1.0f };
    inline constexpr Vec4a Vec4a_X{ 1.0f, 0.0f, 0.0f, 0.0f };
    inline constexpr Vec4a Vec4a_Y{ 0.0f, 1.0f, 0.0f, 0.0f };
    inline constexpr Vec4a Vec4a_Z{ 0.0f, 0.0f, 1.0f, 0.0f };
    inline constexpr Vec4a Vec4a_W{ 0.0f, 0.0f, 0.0f, 1.0f };

    inline constexpr Vec2f Vec2f_Infinity{ Limits<float>::Infinity, Limits<float>::Infinity };
    inline constexpr Vec2f Vec2f_Zero{ 0, 0 };
    inline constexpr Vec2f Vec2f_One{ 1, 1, };
    inline constexpr Vec2f Vec2f_X{ 1, 0 };
    inline constexpr Vec2f Vec2f_Y{ 0, 1 };

    inline constexpr Vec3f Vec3f_Infinity{ Limits<float>::Infinity, Limits<float>::Infinity, Limits<float>::Infinity };
    inline constexpr Vec3f Vec3f_Zero{ 0, 0, 0 };
    inline constexpr Vec3f Vec3f_One{ 1, 1, 1 };
    inline constexpr Vec3f Vec3f_X{ 1, 0, 0 };
    inline constexpr Vec3f Vec3f_Y{ 0, 1, 0 };
    inline constexpr Vec3f Vec3f_Z{ 0, 0, 1 };

    inline constexpr Vec4f Vec4f_Infinity{ Limits<float>::Infinity, Limits<float>::Infinity, Limits<float>::Infinity, Limits<float>::Infinity };
    inline constexpr Vec4f Vec4f_Zero{ 0, 0, 0, 0 };
    inline constexpr Vec4f Vec4f_One{ 1, 1, 1, 1 };
    inline constexpr Vec4f Vec4f_X{ 1, 0, 0, 0 };
    inline constexpr Vec4f Vec4f_Y{ 0, 1, 0, 0 };
    inline constexpr Vec4f Vec4f_Z{ 0, 0, 1, 0 };
    inline constexpr Vec4f Vec4f_W{ 0, 0, 0, 1 };

    inline constexpr Vec2i Vec2i_Zero{ 0, 0 };
    inline constexpr Vec2i Vec2i_One{ 1, 1 };
    inline constexpr Vec2i Vec2i_X{ 1, 0 };
    inline constexpr Vec2i Vec2i_Y{ 0, 1 };

    inline constexpr Vec3i Vec3i_Zero{ 0, 0, 0 };
    inline constexpr Vec3i Vec3i_One{ 1, 1, 1 };
    inline constexpr Vec3i Vec3i_X{ 1, 0, 0 };
    inline constexpr Vec3i Vec3i_Y{ 0, 1, 0 };
    inline constexpr Vec3i Vec3i_Z{ 0, 0, 1 };

    inline constexpr Vec4i Vec4i_Zero{ 0, 0, 0, 0 };
    inline constexpr Vec4i Vec4i_One{ 1, 1, 1, 1 };
    inline constexpr Vec4i Vec4i_X{ 1, 0, 0, 0 };
    inline constexpr Vec4i Vec4i_Y{ 0, 1, 0, 0 };
    inline constexpr Vec4i Vec4i_Z{ 0, 0, 1, 0 };
    inline constexpr Vec4i Vec4i_W{ 0, 0, 0, 1 };

    inline constexpr Quat Quat_Identity{ 0.0f, 0.0f, 0.0f, 1.0f };
    inline constexpr Quata Quata_Identity{ 0.0f, 0.0f, 0.0f, 1.0f };

    inline constexpr Mat44 Mat44_Zero{ Vec4a_Zero, Vec4a_Zero, Vec4a_Zero, Vec4a_Zero };
    inline constexpr Mat44 Mat44_Identity{ Vec4a_X, Vec4a_Y, Vec4a_Z, Vec4a_W };
}
