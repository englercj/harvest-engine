// Copyright Chad Engler

#pragma once

#include "he/scribe/context.h"
#include "he/scribe/schema_types.h"

#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/math/types.h"

namespace he::scribe
{
    constexpr uint32_t InvalidTextStyleIndex = 0xFFFFFFFFu;

    enum class TextDirection : uint8_t
    {
        Auto,
        LeftToRight,
        RightToLeft,
    };

    enum class TextDecorationFlags : uint32_t
    {
        None = 0x00u,
        Underline = 0x01u,
        Strikethrough = 0x02u,
    };

    constexpr TextDecorationFlags operator|(TextDecorationFlags lhs, TextDecorationFlags rhs) noexcept
    {
        return static_cast<TextDecorationFlags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }

    constexpr TextDecorationFlags operator&(TextDecorationFlags lhs, TextDecorationFlags rhs) noexcept
    {
        return static_cast<TextDecorationFlags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
    }

    constexpr TextDecorationFlags& operator|=(TextDecorationFlags& lhs, TextDecorationFlags rhs) noexcept
    {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr bool HasAnyFlags(TextDecorationFlags value, TextDecorationFlags flags) noexcept
    {
        return static_cast<uint32_t>(value & flags) != 0u;
    }

    enum class TextEffectFlags : uint32_t
    {
        None = 0x00u,
        Shadow = 0x01u,
        Outline = 0x02u,
    };

    constexpr TextEffectFlags operator|(TextEffectFlags lhs, TextEffectFlags rhs) noexcept
    {
        return static_cast<TextEffectFlags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }

    constexpr TextEffectFlags operator&(TextEffectFlags lhs, TextEffectFlags rhs) noexcept
    {
        return static_cast<TextEffectFlags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
    }

    constexpr TextEffectFlags& operator|=(TextEffectFlags& lhs, TextEffectFlags rhs) noexcept
    {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr bool HasAnyFlags(TextEffectFlags value, TextEffectFlags flags) noexcept
    {
        return static_cast<uint32_t>(value & flags) != 0u;
    }

    struct TextFeatureSetting
    {
        uint32_t tag{ 0 };
        uint32_t value{ 0 };
    };

    struct TextStyle
    {
        uint32_t fontFaceIndex{ InvalidTextStyleIndex };
        TextDecorationFlags decorations{ TextDecorationFlags::None };
        TextEffectFlags effects{ TextEffectFlags::None };
        uint32_t firstFeature{ 0 };
        uint32_t featureCount{ 0 };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec4f decorationColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec4f outlineColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        Vec4f shadowColor{ 0.0f, 0.0f, 0.0f, 0.35f };
        Vec2f shadowOffsetEm{ 0.0f, 0.0f };
        float trackingEm{ 0.0f };
        float stretchX{ 1.0f };
        float stretchY{ 1.0f };
        float skewX{ 0.0f };
        float rotationRadians{ 0.0f };
        float baselineShiftEm{ 0.0f };
        float glyphScale{ 1.0f };
        float outlineWidthEm{ 0.0f };
        float decorationThicknessEm{ 0.06f };
        float underlineOffsetEm{ 0.12f };
        float strikethroughOffsetEm{ 0.32f };
    };

    struct TextStyleSpan
    {
        uint32_t textByteStart{ 0 };
        uint32_t textByteEnd{ 0 };
        uint32_t styleIndex{ 0 };
    };

    constexpr uint32_t MakeOpenTypeFeatureTag(char a, char b, char c, char d)
    {
        return static_cast<uint32_t>(static_cast<uint8_t>(a))
            | (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8u)
            | (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16u)
            | (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24u);
    }

    struct LayoutOptions
    {
        float fontSize{ 16.0f };
        float lineHeightScale{ 1.0f };
        float maxWidth{ 0.0f };
        TextDirection direction{ TextDirection::Auto };
        bool wrap{ true };
    };

    struct StyledTextLayoutDesc
    {
        Span<const FontFaceHandle> fontFaces{};
        StringView text{};
        LayoutOptions options{};
        Span<const TextStyle> styles{};
        Span<const TextStyleSpan> styleSpans{};
        Span<const TextFeatureSetting> features{};
    };

    struct ShapedGlyph
    {
        uint32_t glyphIndex{ 0 };
        uint32_t fontFaceIndex{ 0 };
        uint32_t clusterIndex{ 0 };
        uint32_t lineIndex{ 0 };
        uint32_t styleIndex{ 0 };
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
        uint32_t styleIndex{ 0 };
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
        explicit LayoutEngine(ScribeContext& context) noexcept
            : m_context(&context)
        {}

        ~LayoutEngine() noexcept = default;

        void SetContext(ScribeContext& context) { m_context = &context; }

        bool LayoutText(
            LayoutResult& out,
            Span<const FontFaceHandle> faces,
            StringView text,
            const LayoutOptions& options = {}) const;

        bool LayoutStyledText(LayoutResult& out, const StyledTextLayoutDesc& desc) const;

        bool HitTest(const LayoutResult& layout, const Vec2f& point, HitTestResult& out) const;

    private:
        ScribeContext* m_context{ nullptr };
    };
}
