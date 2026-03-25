// Copyright Chad Engler

#pragma once

#include "he/scribe/context.h"
#include "he/scribe/layout_engine.h"

#include "he/core/span.h"

namespace he::scribe
{
    enum RetainedTextDrawFlags : uint32_t
    {
        RetainedTextDrawFlagUseForegroundColor = 0x01u,
        RetainedTextDrawFlagStroke = 0x02u,
    };

    struct RetainedTextDraw
    {
        uint32_t fontFaceIndex{ 0 };
        uint32_t glyphIndex{ 0 };
        uint32_t flags{ 0 };
        Vec2f position{ 0.0f, 0.0f };
        Vec2f size{ 1.0f, 1.0f };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec2f basisX{ 1.0f, 0.0f };
        Vec2f basisY{ 0.0f, 1.0f };
        Vec2f offset{ 0.0f, 0.0f };
        StrokeStyle strokeStyle{};
    };

    struct RetainedTextQuad
    {
        Vec2f position{ 0.0f, 0.0f };
        Vec2f size{ 1.0f, 1.0f };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec2f basisX{ 1.0f, 0.0f };
        Vec2f basisY{ 0.0f, 1.0f };
        Vec2f offset{ 0.0f, 0.0f };
    };

    struct RetainedTextBuildDesc
    {
        ScribeContext* context{ nullptr };
        Span<const FontFaceHandle> fontFaces{};
        const LayoutResult* layout{ nullptr };
        float fontSize{ 16.0f };
        bool darkBackgroundPreferred{ true };
        Span<const TextStyle> styles{};
    };

    struct RetainedTextInstanceDesc
    {
        Vec2f origin{ 0.0f, 0.0f };
        float scale{ 1.0f };
        Vec4f foregroundColor{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    class RetainedTextModel
    {
    public:
        bool Build(const RetainedTextBuildDesc& desc);
        void Clear();

        bool IsEmpty() const { return m_draws.IsEmpty() && m_quads.IsEmpty(); }
        uint32_t GetDrawCount() const { return m_draws.Size(); }
        uint32_t GetQuadCount() const { return m_quads.Size(); }
        uint32_t GetEstimatedVertexCount() const { return m_estimatedVertexCount; }
        Span<const RetainedTextDraw> GetDraws() const { return m_draws; }
        Span<const RetainedTextQuad> GetQuads() const { return m_quads; }
        ScribeContext* GetContext() const { return m_context; }
        FontFaceHandle GetFontFaceHandle(uint32_t fontFaceIndex) const;

    private:
        ScribeContext* m_context{ nullptr };
        Vector<FontFaceHandle> m_fontFaces{};
        Vector<RetainedTextDraw> m_draws{};
        Vector<RetainedTextQuad> m_quads{};
        uint32_t m_estimatedVertexCount{ 0 };
    };
}
