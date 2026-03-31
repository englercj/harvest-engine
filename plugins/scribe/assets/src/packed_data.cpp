// Copyright Chad Engler

#include "he/scribe/packed_data.h"

#include "he/core/math.h"
#include "he/core/utils.h"

namespace he::scribe
{
    uint16_t PackFloat16(float value) noexcept
    {
        const uint32_t bits = BitCast<uint32_t>(value);
        const uint32_t sign = (bits >> 16) & 0x8000u;
        int32_t exponent = static_cast<int32_t>((bits >> 23) & 0xffu) - 127 + 15;
        uint32_t mantissa = bits & 0x007fffffu;

        if (((bits >> 23) & 0xffu) == 0xffu)
        {
            if (mantissa != 0)
            {
                return static_cast<uint16_t>(sign | 0x7e00u);
            }

            return static_cast<uint16_t>(sign | 0x7c00u);
        }

        if (exponent <= 0)
        {
            if (exponent < -10)
            {
                return static_cast<uint16_t>(sign);
            }

            mantissa |= 0x00800000u;
            const uint32_t shift = static_cast<uint32_t>(1 - exponent);
            const uint32_t rounding = 1u << (shift + 12u - 1u);
            const uint32_t halfMantissa = (mantissa + rounding) >> (shift + 13u);
            return static_cast<uint16_t>(sign | halfMantissa);
        }

        if (exponent >= 31)
        {
            return static_cast<uint16_t>(sign | 0x7c00u);
        }

        mantissa += 0x00001000u;
        if ((mantissa & 0x00800000u) != 0u)
        {
            mantissa = 0;
            exponent += 1;
            if (exponent >= 31)
            {
                return static_cast<uint16_t>(sign | 0x7c00u);
            }
        }

        return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exponent) << 10u) | (mantissa >> 13u));
    }

    PackedCurveTexel PackCurveTexel(float x, float y, float z, float w) noexcept
    {
        PackedCurveTexel texel{};
        texel.x = PackFloat16(x);
        texel.y = PackFloat16(y);
        texel.z = PackFloat16(z);
        texel.w = PackFloat16(w);
        return texel;
    }
}
