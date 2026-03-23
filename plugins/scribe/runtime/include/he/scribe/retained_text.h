// Copyright Chad Engler

#pragma once

#include "he/scribe/layout_engine.h"

#include "he/core/span.h"

namespace he::scribe
{
    enum RetainedTextDrawFlags : uint32_t
    {
        RetainedTextDrawFlagUseForegroundColor = 0x01u,
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
    };

    struct RetainedTextBuildDesc
    {
        Span<const LoadedFontFaceBlob> fontFaces{};
        const LayoutResult* layout{ nullptr };
        float fontSize{ 16.0f };
        bool darkBackgroundPreferred{ true };
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

        bool IsEmpty() const { return m_draws.IsEmpty(); }
        uint32_t GetDrawCount() const { return m_draws.Size(); }
        uint32_t GetEstimatedVertexCount() const { return m_estimatedVertexCount; }
        Span<const RetainedTextDraw> GetDraws() const { return m_draws; }
        const LoadedFontFaceBlob* GetFontFace(uint32_t fontFaceIndex) const;

    private:
        Vector<LoadedFontFaceBlob> m_fontFaces{};
        Vector<RetainedTextDraw> m_draws{};
        uint32_t m_estimatedVertexCount{ 0 };
    };
}
