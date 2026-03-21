// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he::scribe
{
    inline constexpr uint32_t ScribeBandTextureWidth = 4096;
    inline constexpr uint32_t ScribeGlyphVertexCount = 6;

    inline constexpr uint32_t CompiledFontGlyphFlagHasGeometry = 1u << 0;
    inline constexpr uint32_t CompiledFontGlyphFlagEvenOdd = 1u << 1;

    struct PackedCurveTexel
    {
        uint16_t x{ 0 };
        uint16_t y{ 0 };
        uint16_t z{ 0 };
        uint16_t w{ 0 };
    };

    struct PackedBandTexel
    {
        uint16_t x{ 0 };
        uint16_t y{ 0 };
    };

    uint16_t PackFloat16(float value) noexcept;
    PackedCurveTexel PackCurveTexel(float x, float y, float z, float w) noexcept;
}
