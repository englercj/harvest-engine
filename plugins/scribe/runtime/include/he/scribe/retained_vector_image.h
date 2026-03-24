// Copyright Chad Engler

#pragma once

#include "he/scribe/schema_types.h"

#include "he/core/vector.h"
#include "he/math/types.h"

namespace he::scribe
{
    struct RetainedVectorImageDraw
    {
        uint32_t shapeIndex{ 0 };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec2f offset{ 0.0f, 0.0f };
    };

    struct RetainedVectorImageBuildDesc
    {
        const VectorImageResourceReader* image{ nullptr };
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

        bool IsEmpty() const { return m_draws.IsEmpty(); }
        uint32_t GetDrawCount() const { return m_draws.Size(); }
        uint32_t GetEstimatedVertexCount() const { return m_estimatedVertexCount; }
        Span<const RetainedVectorImageDraw> GetDraws() const { return m_draws; }
        const VectorImageResourceReader* GetImage() const;
        Vec2f GetViewBoxSize() const { return m_viewBoxSize; }
        uint64_t GetImageHash() const { return m_imageHash; }

    private:
        Vector<schema::Word> m_imageStorage{};
        VectorImageResourceReader m_image{};
        Vector<RetainedVectorImageDraw> m_draws{};
        Vec2f m_viewBoxSize{ 0.0f, 0.0f };
        uint64_t m_imageHash{ 0 };
        uint32_t m_estimatedVertexCount{ 0 };
    };
}
