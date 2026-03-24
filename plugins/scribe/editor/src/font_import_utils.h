// Copyright Chad Engler

#pragma once

#include "he/scribe/schema_types.h"

#include "he/core/string.h"
#include "he/core/vector.h"

namespace he::scribe::editor
{
    struct FontFaceInfo
    {
        uint32_t faceIndex{ 0 };
        uint32_t faceCount{ 0 };
        String familyName{};
        String styleName{};
        uint32_t glyphCount{ 0 };
        uint32_t unitsPerEm{ 0 };
        uint32_t maxAdvanceWidth{ 0 };
        uint32_t maxAdvanceHeight{ 0 };
        int32_t ascender{ 0 };
        int32_t descender{ 0 };
        int32_t lineHeight{ 0 };
        int32_t capHeight{ 0 };
        bool hasColorGlyphs{ false };
    };

    FontSourceFormat DeduceFontSourceFormat(const char* file);
    bool ReadFontSourceBytes(Vector<uint8_t>& out, const char* file);
    bool InspectFontFace(const Vector<uint8_t>& sourceBytes, uint32_t faceIndex, FontFaceInfo& out);
    bool InspectFontFaces(Vector<FontFaceInfo>& out, const Vector<uint8_t>& sourceBytes);
    bool ExtractFontFaceSourceBytes(Vector<uint8_t>& out, const Vector<uint8_t>& sourceBytes, uint32_t faceIndex);

    void FillFontFaceMetrics(FontFaceMetrics::Builder metrics, const FontFaceInfo& info);
    void FillFontFaceAssetData(ScribeFontFace::Builder assetData, const FontFaceInfo& info);
}
