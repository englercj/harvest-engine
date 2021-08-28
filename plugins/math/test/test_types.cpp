// Copyright Chad Engler

#include "he/math/types.h"

#include "he/core/test.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(math, types, Vec2)
{
    static_assert(IsSpecialization<Vec2f, Vec2>);
    static_assert(std::is_same_v<Vec2f::Type, float>);
    static_assert(Vec2f::Size == 2);

    static_assert(IsSpecialization<Vec2i, Vec2>);
    static_assert(std::is_same_v<Vec2i::Type, int32_t>);
    static_assert(Vec2i::Size == 2);

    static_assert(IsSpecialization<Vec2u, Vec2>);
    static_assert(std::is_same_v<Vec2u::Type, uint32_t>);
    static_assert(Vec2u::Size == 2);

    static_assert(sizeof(Vec2f) == 8);
    static_assert(alignof(Vec2f) == 4);

    constexpr Vec2<float> a{ 1, 2 };

    static_assert(a.x == 1);
    static_assert(a.y == 2);

    HE_EXPECT_EQ(a.x, 1);
    HE_EXPECT_EQ(a.y, 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, types, Vec3)
{
    static_assert(IsSpecialization<Vec3f, Vec3>);
    static_assert(std::is_same_v<Vec3f::Type, float>);
    static_assert(Vec3f::Size == 3);

    static_assert(IsSpecialization<Vec3i, Vec3>);
    static_assert(std::is_same_v<Vec3i::Type, int32_t>);
    static_assert(Vec3i::Size == 3);

    static_assert(IsSpecialization<Vec3u, Vec3>);
    static_assert(std::is_same_v<Vec3u::Type, uint32_t>);
    static_assert(Vec3u::Size == 3);

    static_assert(sizeof(Vec3f) == 12);
    static_assert(alignof(Vec3f) == 4);

    constexpr Vec3<float> a{ 1, 2, 3 };

    static_assert(a.x == 1);
    static_assert(a.y == 2);
    static_assert(a.z == 3);

    HE_EXPECT_EQ(a.x, 1);
    HE_EXPECT_EQ(a.y, 2);
    HE_EXPECT_EQ(a.z, 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, types, Vec4)
{
    static_assert(IsSpecialization<Vec4f, Vec4>);
    static_assert(std::is_same_v<Vec4f::Type, float>);
    static_assert(Vec4f::Size == 4);

    static_assert(IsSpecialization<Vec4i, Vec4>);
    static_assert(std::is_same_v<Vec4i::Type, int32_t>);
    static_assert(Vec4i::Size == 4);

    static_assert(IsSpecialization<Vec4u, Vec4>);
    static_assert(std::is_same_v<Vec4u::Type, uint32_t>);
    static_assert(Vec4u::Size == 4);

    static_assert(sizeof(Vec4f) == 16);
    static_assert(alignof(Vec4f) == 4);

    constexpr Vec4<float> a{ 1, 2, 3, 4 };

    static_assert(a.x == 1);
    static_assert(a.y == 2);
    static_assert(a.z == 3);
    static_assert(a.w == 4);

    HE_EXPECT_EQ(a.x, 1);
    HE_EXPECT_EQ(a.y, 2);
    HE_EXPECT_EQ(a.z, 3);
    HE_EXPECT_EQ(a.w, 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, types, Vec4a)
{
    static_assert(sizeof(Vec4a) == 16);
    static_assert(alignof(Vec4a) == 16);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, types, Quat)
{
    constexpr Quat a{ 1, 2, 3, 4 };

    static_assert(a.x == 1);
    static_assert(a.y == 2);
    static_assert(a.z == 3);
    static_assert(a.w == 4);

    HE_EXPECT_EQ(a.x, 1);
    HE_EXPECT_EQ(a.y, 2);
    HE_EXPECT_EQ(a.z, 3);
    HE_EXPECT_EQ(a.w, 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, types, Quata)
{
    static_assert(sizeof(Quata) == 16);
    static_assert(alignof(Quata) == 16);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, types, Mat44)
{
    static_assert(sizeof(Mat44) == 64);
    static_assert(alignof(Mat44) == 16);
}
