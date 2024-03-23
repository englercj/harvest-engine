#pragma once

#include "he/core/math.h"
#include "he/core/test.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/math/quat.h"
#include "he/math/quata.h"
#include "he/math/vec2.h"
#include "he/math/vec3.h"
#include "he/math/vec4.h"
#include "he/math/vec4a.h"

#define HE_EXPECT_EQ_ULP3(a, b, diff) HE_EXPECT(he::IsNearlyEqualULP3(a, b, diff), a, b)

namespace he
{
    template <typename T> requires(IsSpecialization<T, Vec2> || IsSpecialization<T, Vec3> || IsSpecialization<T, Vec4> || IsSame<T, Vec4a>)
    inline bool IsNearlyEqualULP(const T& a, const T& b, int32_t maxUlpDiff)
    {
        const typename T::Type* fa = GetPointer(a);
        const typename T::Type* fb = GetPointer(b);

        for (uint32_t i = 0; i < T::Size; ++i)
        {
            if (!IsNearlyEqualULP(fa[i], fb[i], maxUlpDiff))
                return false;
        }

        return true;
    }

    inline bool IsNearlyEqualULP(const Quat& a, const Quat& b, int32_t maxUlpDiff)
    {
        return IsNearlyEqualULP(a.x, b.x, maxUlpDiff)
            && IsNearlyEqualULP(a.y, b.y, maxUlpDiff)
            && IsNearlyEqualULP(a.z, b.z, maxUlpDiff)
            && IsNearlyEqualULP(a.w, b.w, maxUlpDiff);
    }

    inline bool IsNearlyEqualULP(const Quata& a, const Quata& b, int32_t maxUlpDiff)
    {
        return IsNearlyEqualULP(GetX(a.v), GetX(b.v), maxUlpDiff)
            && IsNearlyEqualULP(GetY(a.v), GetY(b.v), maxUlpDiff)
            && IsNearlyEqualULP(GetZ(a.v), GetZ(b.v), maxUlpDiff)
            && IsNearlyEqualULP(GetW(a.v), GetW(b.v), maxUlpDiff);
    }

    inline bool IsNearlyEqualULP(const Vec4a& a, const Vec4a& b, int32_t maxUlpDiff)
    {
        return IsNearlyEqualULP(GetX(a), GetX(b), maxUlpDiff)
            && IsNearlyEqualULP(GetY(a), GetY(b), maxUlpDiff)
            && IsNearlyEqualULP(GetZ(a), GetZ(b), maxUlpDiff)
            && IsNearlyEqualULP(GetW(a), GetW(b), maxUlpDiff);
    }

    inline bool IsNearlyEqualULP3(const Vec4a& a, const Vec4a& b, int32_t maxUlpDiff)
    {
        return IsNearlyEqualULP(GetX(a), GetX(b), maxUlpDiff)
            && IsNearlyEqualULP(GetY(a), GetY(b), maxUlpDiff)
            && IsNearlyEqualULP(GetZ(a), GetZ(b), maxUlpDiff);
    }
}
