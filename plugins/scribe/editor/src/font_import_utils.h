// Copyright Chad Engler

#pragma once

#include "he/scribe/runtime_blob.h"

#include "he/core/string.h"
#include "he/core/vector.h"

namespace he::scribe::editor
{
    struct FontFaceInfo
    {
        uint32_t faceIndex{ 0 };
        uint32_t faceCount{ 0 };
        FontSourceFormat sourceFormat{ FontSourceFormat::Unknown };
        String familyName{};
        String styleName{};
        String postscriptName{};
        uint32_t glyphCount{ 0 };
        uint32_t unitsPerEm{ 0 };
        uint32_t maxAdvanceWidth{ 0 };
        uint32_t maxAdvanceHeight{ 0 };
        int32_t ascender{ 0 };
        int32_t descender{ 0 };
        int32_t lineHeight{ 0 };
        int32_t capHeight{ 0 };
        bool isScalable{ false };
        bool hasColorGlyphs{ false };
        bool hasKerning{ false };
        bool hasHorizontalLayout{ false };
        bool hasVerticalLayout{ false };
    };

    FontSourceFormat DeduceFontSourceFormat(const char* file);
    bool ReadFontSourceBytes(Vector<uint8_t>& out, const char* file);
    bool InspectFontFace(const Vector<uint8_t>& sourceBytes, uint32_t faceIndex, FontSourceFormat sourceFormat, FontFaceInfo& out);

    void FillFontFaceMetrics(FontFaceMetrics::Builder metrics, const FontFaceInfo& info);
    void FillFontFaceImportMetadata(FontFaceImportMetadata::Builder metadata, const FontFaceInfo& info);
    void FillFontFaceAssetData(ScribeFontFace::Builder assetData, const FontFaceInfo& info, bool preserveSourceBytesForShaping);
}
