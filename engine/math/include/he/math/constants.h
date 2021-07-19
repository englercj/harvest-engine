// Copyright Chad Engler

#pragma once

#include "he/math/types.h"

namespace he
{
    inline constexpr float Float_Pi = 3.141592654f;
    inline constexpr float Float_Pi2 = 6.283185307f;
    inline constexpr float Float_PiHalf = 1.570796327f;
    inline constexpr float Float_PiQuarter = 0.785398163f;
    inline constexpr float Float_Sqrt2 = 1.414213562f;

    inline constexpr float Float_Epsilon = 1.192092896e-7f;
    inline constexpr float Float_Min = 1.175494351e-38f;
    inline constexpr float Float_Max = 3.402823466e+38f;
    inline constexpr float Float_AllBits = -__builtin_nanf("0x3FFFFF");
    inline constexpr float Float_Infinity = __builtin_huge_valf();
    inline constexpr float Float_Nan = Float_Infinity * 0.0f;
    inline constexpr float Float_ZeroSafe = 1000.0f * Float_Min;

    inline constexpr Vec4a Vec4a_Infinity{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity };
    inline constexpr Vec4a Vec4a_Zero = { 0.0f, 0.0f, 0.0f, 0.0f };
    inline constexpr Vec4a Vec4a_One = { 1.0f, 1.0f, 1.0f, 1.0f };
    inline constexpr Vec4a Vec4a_X = { 1.0f, 0.0f, 0.0f, 0.0f };
    inline constexpr Vec4a Vec4a_Y = { 0.0f, 1.0f, 0.0f, 0.0f };
    inline constexpr Vec4a Vec4a_Z = { 0.0f, 0.0f, 1.0f, 0.0f };
    inline constexpr Vec4a Vec4a_W = { 0.0f, 0.0f, 0.0f, 1.0f };

    inline constexpr Vec2f Vec2f_Zero = { 0, 0 };
    inline constexpr Vec2f Vec2f_One = { 1, 1, };
    inline constexpr Vec2f Vec2f_X = { 1, 0 };
    inline constexpr Vec2f Vec2f_Y = { 0, 1 };

    inline constexpr Vec3f Vec3f_Zero = { 0, 0, 0 };
    inline constexpr Vec3f Vec3f_One = { 1, 1, 1 };
    inline constexpr Vec3f Vec3f_X = { 1, 0, 0 };
    inline constexpr Vec3f Vec3f_Y = { 0, 1, 0 };
    inline constexpr Vec3f Vec3f_Z = { 0, 0, 1 };

    inline constexpr Vec4f Vec4f_Zero = { 0, 0, 0, 0 };
    inline constexpr Vec4f Vec4f_One = { 1, 1, 1, 1 };
    inline constexpr Vec4f Vec4f_X = { 1, 0, 0, 0 };
    inline constexpr Vec4f Vec4f_Y = { 0, 1, 0, 0 };
    inline constexpr Vec4f Vec4f_Z = { 0, 0, 1, 0 };
    inline constexpr Vec4f Vec4f_W = { 0, 0, 0, 1 };

    inline constexpr Vec2i Vec2i_Zero = { 0, 0 };
    inline constexpr Vec2i Vec2i_One = { 1, 1 };
    inline constexpr Vec2i Vec2i_X = { 1, 0 };
    inline constexpr Vec2i Vec2i_Y = { 0, 1 };

    inline constexpr Vec3i Vec3i_Zero = { 0, 0, 0 };
    inline constexpr Vec3i Vec3i_One = { 1, 1, 1 };
    inline constexpr Vec3i Vec3i_X = { 1, 0, 0 };
    inline constexpr Vec3i Vec3i_Y = { 0, 1, 0 };
    inline constexpr Vec3i Vec3i_Z = { 0, 0, 1 };

    inline constexpr Vec4i Vec4i_Zero = { 0, 0, 0, 0 };
    inline constexpr Vec4i Vec4i_One = { 1, 1, 1, 1 };
    inline constexpr Vec4i Vec4i_X = { 1, 0, 0, 0 };
    inline constexpr Vec4i Vec4i_Y = { 0, 1, 0, 0 };
    inline constexpr Vec4i Vec4i_Z = { 0, 0, 1, 0 };
    inline constexpr Vec4i Vec4i_W = { 0, 0, 0, 1 };

    inline constexpr Quat Quat_Identity = { 0.0f, 0.0f, 0.0f, 1.0f };
    inline constexpr Quata Quata_Identity = { 0.0f, 0.0f, 0.0f, 1.0f };

    inline constexpr Mat44 Mat44_Zero = { Vec4a_Zero, Vec4a_Zero, Vec4a_Zero, Vec4a_Zero };
    inline constexpr Mat44 Mat44_Identity = { Vec4a_X, Vec4a_Y, Vec4a_Z, Vec4a_W };
}
