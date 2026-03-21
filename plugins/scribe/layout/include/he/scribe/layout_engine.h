// Copyright Chad Engler

#pragma once

#include "he/scribe/runtime_blob.h"

#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/math/types.h"

namespace he::scribe
{
    enum class TextDirection : uint8_t
    {
        Auto,
        LeftToRight,
        RightToLeft,
    };

    struct LayoutOptions
    {
        float fontSize{ 16.0f };
        float lineHeightScale{ 1.0f };
        float maxWidth{ 0.0f };
        TextDirection direction{ TextDirection::Auto };
        bool wrap{ true };
    };

    struct ShapedGlyph
    {
        uint32_t glyphIndex{ 0 };
        uint32_t fontFaceIndex{ 0 };
        uint32_t clusterIndex{ 0 };
        uint32_t lineIndex{ 0 };
        uint32_t textByteStart{ 0 };
        uint32_t textByteEnd{ 0 };
        Vec2f position{ 0.0f, 0.0f };
        Vec2f offset{ 0.0f, 0.0f };
        Vec2f advance{ 0.0f, 0.0f };
    };

    struct TextCluster
    {
        uint32_t textByteStart{ 0 };
        uint32_t textByteEnd{ 0 };
        uint32_t glyphStart{ 0 };
        uint32_t glyphCount{ 0 };
        uint32_t fontFaceIndex{ 0 };
        uint32_t lineIndex{ 0 };
        float advance{ 0.0f };
        float x0{ 0.0f };
        float x1{ 0.0f };
        bool isWhitespace{ false };
    };

    struct TextLine
    {
        uint32_t glyphStart{ 0 };
        uint32_t glyphCount{ 0 };
        uint32_t clusterStart{ 0 };
        uint32_t clusterCount{ 0 };
        float width{ 0.0f };
        float ascent{ 0.0f };
        float descent{ 0.0f };
        float baselineY{ 0.0f };
        float height{ 0.0f };
        TextDirection direction{ TextDirection::LeftToRight };
    };

    struct LayoutResult
    {
        Vector<ShapedGlyph> glyphs{};
        Vector<TextCluster> clusters{};
        Vector<TextLine> lines{};
        float width{ 0.0f };
        float height{ 0.0f };
        uint32_t missingGlyphCount{ 0 };
        uint32_t fallbackGlyphCount{ 0 };

        void Clear();
    };

    struct HitTestResult
    {
        uint32_t lineIndex{ 0 };
        uint32_t clusterIndex{ 0 };
        uint32_t textByteIndex{ 0 };
        float caretX{ 0.0f };
        bool isInside{ false };
        bool isTrailingEdge{ false };
    };

    class LayoutEngine
    {
    public:
        LayoutEngine() noexcept = default;
        ~LayoutEngine() noexcept = default;

        bool LayoutText(
            LayoutResult& out,
            Span<const LoadedFontFaceBlob> faces,
            StringView text,
            const LayoutOptions& options = {}) const;

        bool HitTest(const LayoutResult& layout, const Vec2f& point, HitTestResult& out) const;
    };
}
