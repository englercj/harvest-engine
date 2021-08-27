#pragma once

#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/math/quat.h"
#include "he/math/quata.h"
#include "he/math/vec2.h"
#include "he/math/vec3.h"
#include "he/math/vec4.h"
#include "he/math/vec4a.h"

#define HE_EXPECT_EQ_ULP(a, b, diff) HE_EXPECT(he::EqualUlp(a, b, diff), a, b)
#define HE_EXPECT_EQ_ULP3(a, b, diff) HE_EXPECT(he::EqualUlp3(a, b, diff), a, b)

namespace he
{
    constexpr bool EqualUlp(float a, float b, int32_t maxUlpDiff)
    {
        // Based on: https://www.gamasutra.com/view/news/162368/Indepth_Comparing_floating_point_numbers_2012_edition.php

        // Treat -0 and +0 as equal
        if (a == b)
            return true;

        const int32_t ai = BitCast<int32_t>(a);
        const int32_t bi = BitCast<int32_t>(b);

        const bool asigned = (ai >> 31) != 0;
        const bool bsigned = (bi >> 31) != 0;

        // Different signs mean they do not match
        if (asigned != bsigned)
            return false;

        const int32_t ulpDiff = ai > bi ? ai - bi : bi - ai;

        return ulpDiff <= maxUlpDiff;
    }

    template <typename T>
    inline bool EqualUlp(const T& a, const T& b, int32_t maxUlpDiff)
    {
        const typename T::Type* fa = GetPointer(a);
        const typename T::Type* fb = GetPointer(b);

        for (uint32_t i = 0; i < T::Size; ++i)
        {
            if (!EqualUlp(fa[i], fb[i], maxUlpDiff))
                return false;
        }

        return true;
    }

    inline bool EqualUlp(const Quat& a, const Quat& b, int32_t maxUlpDiff)
    {
        return EqualUlp(a.x, b.x, maxUlpDiff)
            && EqualUlp(a.y, b.y, maxUlpDiff)
            && EqualUlp(a.z, b.z, maxUlpDiff)
            && EqualUlp(a.w, b.w, maxUlpDiff);
    }

    inline bool EqualUlp(const Quata& a, const Quata& b, int32_t maxUlpDiff)
    {
        return EqualUlp(GetX(a.v), GetX(b.v), maxUlpDiff)
            && EqualUlp(GetY(a.v), GetY(b.v), maxUlpDiff)
            && EqualUlp(GetZ(a.v), GetZ(b.v), maxUlpDiff)
            && EqualUlp(GetW(a.v), GetW(b.v), maxUlpDiff);
    }

    inline bool EqualUlp(const Vec4a& a, const Vec4a& b, int32_t maxUlpDiff)
    {
        return EqualUlp(GetX(a), GetX(b), maxUlpDiff)
            && EqualUlp(GetY(a), GetY(b), maxUlpDiff)
            && EqualUlp(GetZ(a), GetZ(b), maxUlpDiff)
            && EqualUlp(GetW(a), GetW(b), maxUlpDiff);
    }

    inline bool EqualUlp3(const Vec4a& a, const Vec4a& b, int32_t maxUlpDiff)
    {
        return EqualUlp(GetX(a), GetX(b), maxUlpDiff)
            && EqualUlp(GetY(a), GetY(b), maxUlpDiff)
            && EqualUlp(GetZ(a), GetZ(b), maxUlpDiff);
    }
}
