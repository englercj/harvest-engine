// Copyright Chad Engler

#pragma once

#include "he/scribe/context.h"
#include "he/scribe/retained_text.h"
#include "he/scribe/schema_types.h"

#include "he/core/vector.h"
#include "he/math/types.h"

namespace he::scribe
{
    enum RetainedVectorImageDrawFlags : uint32_t
    {
        RetainedVectorImageDrawFlagStroke = 0x01u,
        RetainedVectorImageDrawFlagUseCompiledShape = 0x02u,
    };

    struct RetainedVectorImageDraw
    {
        uint32_t shapeIndex{ 0 };
        uint32_t flags{ 0 };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec2f offset{ 0.0f, 0.0f };
        StrokeStyle strokeStyle{};
    };

    struct RetainedVectorImageBuildDesc
    {
        ScribeContext* context{ nullptr };
        VectorImageHandle image{};
        bool includeFill{ true };
        Vec4f strokeColor{ 0.0f, 0.0f, 0.0f, 0.0f };
        StrokeStyle strokeStyle{};
    };

    struct RetainedVectorImageInstanceDesc
    {
        Vec2f origin{ 0.0f, 0.0f };
        float scale{ 1.0f };
        Vec4f tint{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    class RetainedVectorImageModel
    {
    public:
        bool Build(const RetainedVectorImageBuildDesc& desc);
        void Clear();

        bool IsEmpty() const { return m_draws.IsEmpty() && m_textDraws.IsEmpty(); }
        uint32_t GetDrawCount() const { return m_draws.Size() + m_textDraws.Size(); }
        uint32_t GetShapeDrawCount() const { return m_draws.Size(); }
        uint32_t GetTextDrawCount() const { return m_textDraws.Size(); }
        uint32_t GetEstimatedVertexCount() const { return m_estimatedVertexCount; }
        Span<const RetainedVectorImageDraw> GetDraws() const { return m_draws; }
        Span<const RetainedTextDraw> GetTextDraws() const { return m_textDraws; }
        ScribeContext* GetContext() const { return m_context; }
        VectorImageHandle GetImageHandle() const { return m_image; }
        Vec2f GetViewBoxSize() const { return m_viewBoxSize; }
        FontFaceHandle GetFontFaceHandle(uint32_t fontFaceIndex) const;

    private:
        ScribeContext* m_context{ nullptr };
        VectorImageHandle m_image{};
        Vector<FontFaceHandle> m_fontFaces{};
        Vector<RetainedVectorImageDraw> m_draws{};
        Vector<RetainedTextDraw> m_textDraws{};
        Vec2f m_viewBoxSize{ 0.0f, 0.0f };
        uint32_t m_estimatedVertexCount{ 0 };
    };
}
