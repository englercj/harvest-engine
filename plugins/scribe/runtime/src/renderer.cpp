// Copyright Chad Engler

#include "he/scribe/renderer.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/context.h"
#include "he/scribe/retained_text.h"
#include "he/scribe/retained_vector_image.h"

#include "glyph_atlas.h"
#include "glyph_atlas_utils.h"

#include "shaders/scribe.shaders.h"
#include "shaders/solid.shaders.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/hash.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/utils.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/device.h"

#include <cstddef>
namespace he::scribe
{
    namespace
    {
        constexpr uint32_t VertexShaderConstantCount = 20;

        struct GlyphTransformState
        {
            float a00{ 0.0f };
            float a01{ 0.0f };
            float a10{ 0.0f };
            float a11{ 0.0f };
            float offsetX{ 0.0f };
            float offsetY{ 0.0f };
            float it00{ 0.0f };
            float it01{ 0.0f };
            float it10{ 0.0f };
            float it11{ 0.0f };
            bool hasInverseJacobian{ false };
            Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        };

        RetainedVectorImageShapeResourceKind GetShapeResourceKind(const RetainedVectorImageDraw& draw)
        {
            return ((draw.flags & RetainedVectorImageDrawFlagRuntimeRestroke) != 0)
                ? RetainedVectorImageShapeResourceKind::RuntimeRestroke
                : RetainedVectorImageShapeResourceKind::CompiledShape;
        }

        bool CreateUploadBuffer(
            rhi::Device& device,
            rhi::Buffer*& out,
            uint32_t size,
            uint32_t stride,
            [[maybe_unused]] const char* name)
        {
            rhi::BufferDesc desc{};
            desc.heapType = rhi::HeapType::Upload;
            desc.usage = rhi::BufferUsage::Vertices;
            desc.initialState = rhi::BufferState::Vertices;
            desc.size = size;
            desc.stride = stride;
            HE_RHI_SET_NAME(desc, name);

            Result r = device.CreateBuffer(desc, out);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create upload vertex buffer. Error: {}", r);
                return false;
            }
            return true;
        }

        float PackBits(uint32_t value)
        {
            return BitCast<float>(value);
        }

        Vec4f MultiplyColor(const Vec4f& a, const Vec4f& b)
        {
            return {
                a.x * b.x,
                a.y * b.y,
                a.z * b.z,
                a.w * b.w
            };
        }

        Vec4f PremultiplyColor(const Vec4f& color)
        {
            return {
                color.x * color.w,
                color.y * color.w,
                color.z * color.w,
                color.w
            };
        }

        void BuildFrameConstants(
            float* outConstants,
            const Vec2u& targetSize)
        {
            HE_ASSERT(outConstants);

            const float width = static_cast<float>(targetSize.x);
            const float height = static_cast<float>(targetSize.y);
            outConstants[0] = 2.0f / width;
            outConstants[1] = 0.0f;
            outConstants[2] = 0.0f;
            outConstants[3] = -1.0f;

            outConstants[4] = 0.0f;
            outConstants[5] = -2.0f / height;
            outConstants[6] = 0.0f;
            outConstants[7] = 1.0f;

            outConstants[8] = 0.0f;
            outConstants[9] = 0.0f;
            outConstants[10] = 0.0f;
            outConstants[11] = 0.5f;

            outConstants[12] = 0.0f;
            outConstants[13] = 0.0f;
            outConstants[14] = 0.0f;
            outConstants[15] = 1.0f;

            outConstants[16] = width;
            outConstants[17] = height;
            outConstants[18] = 0.0f;
            outConstants[19] = 0.0f;
        }

        GlyphTransformState BuildGlyphTransformState(const DrawGlyphDesc& draw)
        {
            GlyphTransformState state{};
            state.a00 = draw.size.x * draw.basisX.x;
            state.a01 = draw.size.x * draw.basisY.x;
            state.a10 = draw.size.y * draw.basisX.y;
            state.a11 = draw.size.y * draw.basisY.y;
            state.offsetX = draw.position.x + (draw.size.x * draw.offset.x);
            state.offsetY = draw.position.y + (draw.size.y * draw.offset.y);
            state.color = draw.color;

            const float det = (state.a00 * state.a11) - (state.a01 * state.a10);
            state.hasInverseJacobian = Abs(det) > 1.0e-8f;
            if (state.hasInverseJacobian)
            {
                const float invDet = 1.0f / det;
                state.it00 = state.a11 * invDet;
                state.it01 = -state.a10 * invDet;
                state.it10 = -state.a01 * invDet;
                state.it11 = state.a00 * invDet;
            }

            return state;
        }

        PackedGlyphVertex TransformVertex(const PackedGlyphVertex& in, const GlyphTransformState& state)
        {
            PackedGlyphVertex out = in;
            out.pos.x = state.offsetX + (state.a00 * in.pos.x) + (state.a01 * in.pos.y);
            out.pos.y = state.offsetY + (state.a10 * in.pos.x) + (state.a11 * in.pos.y);
            out.pos.z = (state.a00 * in.pos.z) + (state.a01 * in.pos.w);
            out.pos.w = (state.a10 * in.pos.z) + (state.a11 * in.pos.w);

            if (state.hasInverseJacobian)
            {
                const float j0x = in.jac.x;
                const float j0y = in.jac.y;
                const float j1x = in.jac.z;
                const float j1y = in.jac.w;
                out.jac.x = (state.it00 * j0x) + (state.it10 * j0y);
                out.jac.y = (state.it01 * j0x) + (state.it11 * j0y);
                out.jac.z = (state.it00 * j1x) + (state.it10 * j1y);
                out.jac.w = (state.it01 * j1x) + (state.it11 * j1y);
            }
            else
            {
                out.jac = in.jac;
            }

            out.col = PremultiplyColor(MultiplyColor(in.col, state.color));
            return out;
        }

        PackedQuadVertex MakeQuadVertex(float x, float y, const Vec4f& color)
        {
            PackedQuadVertex vertex{};
            vertex.pos = { x, y };
            vertex.col = PremultiplyColor(color);
            return vertex;
        }
    }

    Renderer::~Renderer() noexcept
    {
        Terminate();
    }

    bool Renderer::Initialize(rhi::Format targetFormat)
    {
        m_device = m_context.GetDevice();
        HE_ASSERT(m_device);
        m_targetFormat = targetFormat;

        if (!CreateDeviceResources())
        {
            Terminate();
            return false;
        }

        return true;
    }

    void Renderer::Terminate()
    {
        if (!m_device)
        {
            return;
        }

        m_device->GetRenderCmdQueue().WaitForFlush();
        m_batches.Clear();
        m_streamVertices.Clear();
        m_quadVertices.Clear();
        m_frame = {};
        for (StreamBuffer& streamBuffer : m_streamBuffers)
        {
            m_device->SafeDestroy(streamBuffer.buffer);
            streamBuffer = {};
        }
        for (StreamBuffer& streamBuffer : m_quadStreamBuffers)
        {
            m_device->SafeDestroy(streamBuffer.buffer);
            streamBuffer = {};
        }
        DestroyDeviceResources();
        m_targetFormat = rhi::Format::Invalid;
        m_device = nullptr;
    }

    bool Renderer::CreateDedicatedAtlas(
        GlyphAtlas*& out,
        const TextureDataDesc& curveTexture,
        const TextureDataDesc& bandTexture)
    {
        HE_ASSERT(m_device);
        out = nullptr;
        GlyphAtlas* atlas = Allocator::GetDefault().New<GlyphAtlas>();

        if (!UploadTexturePair2D(
            *m_device,
            atlas->curveTexture,
            atlas->curveView,
            curveTexture,
            rhi::Format::RGBA16Float,
            "Scribe Curve Texture",
            "Scribe Curve Upload Buffer",
            atlas->bandTexture,
            atlas->bandView,
            bandTexture,
            rhi::Format::RG16Uint,
            "Scribe Band Texture",
            "Scribe Band Upload Buffer"))
        {
            ReleaseAtlas(atlas);
            return false;
        }

        rhi::DescriptorRange ranges[2]{};
        ranges[0].type = rhi::DescriptorRangeType::Texture;
        ranges[0].baseRegister = 0;
        ranges[0].registerSpace = 0;
        ranges[0].count = 1;

        ranges[1].type = rhi::DescriptorRangeType::Texture;
        ranges[1].baseRegister = 1;
        ranges[1].registerSpace = 0;
        ranges[1].count = 1;

        rhi::DescriptorTableDesc tableDesc{};
        tableDesc.rangeCount = HE_LENGTH_OF(ranges);
        tableDesc.ranges = ranges;

        Result r = m_device->CreateDescriptorTable(tableDesc, atlas->descriptorTable);
        if (!r)
        {
            HE_LOGF_ERROR(scribe_render, "Failed to create glyph descriptor table. Error: {}", r);
            ReleaseAtlas(atlas);
            return false;
        }

        const rhi::TextureView* curveView = atlas->curveView;
        const rhi::TextureView* bandView = atlas->bandView;
        m_device->SetTextureViews(atlas->descriptorTable, 0, 0, 1, &curveView);
        m_device->SetTextureViews(atlas->descriptorTable, 1, 0, 1, &bandView);

        out = atlas;
        return true;
    }

    void Renderer::ReleaseAtlas(GlyphAtlas*& atlas)
    {
        if (!atlas || !m_device)
        {
            atlas = nullptr;
            return;
        }

        m_device->SafeDestroy(atlas->descriptorTable);
        m_device->SafeDestroy(atlas->bandView);
        m_device->SafeDestroy(atlas->bandTexture);
        m_device->SafeDestroy(atlas->curveView);
        m_device->SafeDestroy(atlas->curveTexture);
        Allocator::GetDefault().Delete(atlas);

        atlas = nullptr;
    }

    bool Renderer::CreateGlyphResource(GlyphResource& out, const GlyphResourceCreateInfo& desc)
    {
        HE_ASSERT(m_device);

        if (!HE_VERIFY(
            desc.vertices != nullptr
            && (desc.vertexCount > 0)
            && (desc.vertexCount <= ScribeGlyphVertexCount)
            && desc.curveTexture.data != nullptr
            && desc.curveTexture.size.x > 0
            && desc.curveTexture.size.y > 0
            && desc.bandTexture.data != nullptr
            && desc.bandTexture.size.x > 0
            && desc.bandTexture.size.y > 0,
            HE_MSG("Invalid glyph resource create info.")))
        {
            return false;
        }

        GlyphResource resource{};
        MemCopy(resource.vertices, desc.vertices, desc.vertexCount * sizeof(PackedGlyphVertex));
        resource.vertexCount = desc.vertexCount;
        if (!CreateDedicatedAtlas(resource.atlas, desc.curveTexture, desc.bandTexture))
        {
            DestroyGlyphResource(resource);
            return false;
        }

        out = resource;
        return true;
    }

    bool Renderer::CreateDebugGlyphResource(GlyphResource& out)
    {
        Vector<PackedGlyphVertex> vertices{};
        vertices.Resize(6);

        const uint32_t glyphLocPacked = 0;
        const uint32_t bandInfoPacked = 0;
        const Vec4f identityJac{ 1.0f, 0.0f, 0.0f, 1.0f };
        const Vec4f banding{ 0.0f, 0.0f, 0.0f, 0.0f };
        const Vec4f color{ 0.95f, 0.28f, 0.14f, 1.0f };
        const float glyphLocBits = PackBits(glyphLocPacked);
        const float bandInfoBits = PackBits(bandInfoPacked);

        auto makeVertex = [&](float x, float y, float nx, float ny) -> PackedGlyphVertex
        {
            PackedGlyphVertex v{};
            v.pos = { x, y, nx, ny };
            v.tex = { x, y, glyphLocBits, bandInfoBits };
            v.jac = identityJac;
            v.bnd = banding;
            v.col = color;
            return v;
        };

        vertices[0] = makeVertex(0.0f, 0.0f, -1.0f, -1.0f);
        vertices[1] = makeVertex(1.0f, 0.0f, 1.0f, -1.0f);
        vertices[2] = makeVertex(1.0f, 1.0f, 1.0f, 1.0f);
        vertices[3] = makeVertex(0.0f, 0.0f, -1.0f, -1.0f);
        vertices[4] = makeVertex(1.0f, 1.0f, 1.0f, 1.0f);
        vertices[5] = makeVertex(0.0f, 1.0f, -1.0f, 1.0f);

        const PackedCurveTexel curveTexels[] =
        {
            PackCurveTexel(1.0f, 1.0f, 1.0f, 0.5f),
            PackCurveTexel(1.0f, 0.0f, 0.0f, 0.0f),
            PackCurveTexel(0.0f, 1.0f, 0.5f, 1.0f),
            PackCurveTexel(1.0f, 1.0f, 0.0f, 0.0f),
            PackCurveTexel(1.0f, 0.0f, 0.5f, 0.0f),
            PackCurveTexel(0.0f, 0.0f, 0.0f, 0.0f),
            PackCurveTexel(0.0f, 0.0f, 0.0f, 0.5f),
            PackCurveTexel(0.0f, 1.0f, 0.0f, 0.0f),
        };

        Vector<PackedBandTexel> bandTexels{};
        bandTexels.Resize(ScribeBandTextureWidth);
        bandTexels[0] = { 4, 2 };
        bandTexels[1] = { 4, 6 };
        bandTexels[2] = { 0, 0 };
        bandTexels[3] = { 2, 0 };
        bandTexels[4] = { 4, 0 };
        bandTexels[5] = { 6, 0 };
        bandTexels[6] = { 2, 0 };
        bandTexels[7] = { 0, 0 };
        bandTexels[8] = { 6, 0 };
        bandTexels[9] = { 4, 0 };

        GlyphResourceCreateInfo createInfo{};
        createInfo.vertices = vertices.Data();
        createInfo.vertexCount = vertices.Size();
        createInfo.curveTexture.data = curveTexels;
        createInfo.curveTexture.size = { 8, 1 };
        createInfo.curveTexture.rowPitch = sizeof(curveTexels);
        createInfo.bandTexture.data = bandTexels.Data();
        createInfo.bandTexture.size = { ScribeBandTextureWidth, 1 };
        createInfo.bandTexture.rowPitch = ScribeBandTextureWidth * sizeof(PackedBandTexel);

        return CreateGlyphResource(out, createInfo);
    }

    void Renderer::DestroyGlyphResource(GlyphResource& resource)
    {
        ReleaseAtlas(resource.atlas);
        resource = {};
    }

    bool Renderer::BeginFrame(const FrameDesc& desc)
    {
        if (!HE_VERIFY(
            m_device != nullptr
            && desc.cmdList != nullptr
            && desc.targetView != nullptr
            && desc.targetSize.x > 0
            && desc.targetSize.y > 0,
            HE_MSG("Invalid scribe frame description.")))
        {
            return false;
        }

        m_frame = desc;
        m_streamBufferIndex = (m_streamBufferIndex + 1) % HE_LENGTH_OF(m_streamBuffers);
        m_batches.Clear();
        m_streamVertices.Clear();
        m_quadVertices.Clear();
        m_lastSubmittedDrawCount = 0;
        return true;
    }

    void Renderer::ReserveQueuedVertexCapacity(uint32_t vertexCount, uint32_t batchCount)
    {
        if (vertexCount > m_streamVertices.Capacity())
        {
            m_streamVertices.Reserve(vertexCount);
        }

        if (batchCount > m_batches.Capacity())
        {
            m_batches.Reserve(batchCount);
        }
    }

    bool Renderer::PrepareRetainedText(const RetainedTextModel& text)
    {
        text.ClearPreparedGlyphResources();

        const Span<const RetainedTextDraw> draws = text.GetDraws();
        for (uint32_t drawIndex = 0; drawIndex < draws.Size(); ++drawIndex)
        {
            const RetainedTextDraw& draw = draws[drawIndex];
            const GlyphResource* glyphResource = nullptr;
            const bool ok = (draw.flags & RetainedTextDrawFlagStroke) != 0
                ? m_context.TryGetStrokedGlyphResource(text.GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, draw.strokeStyle, glyphResource)
                : m_context.TryGetGlyphResource(text.GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, glyphResource);
            if (!ok)
            {
                continue;
            }

            text.SetPreparedGlyphResource(drawIndex, *glyphResource);
        }

        return true;
    }

    bool Renderer::PrepareRetainedVectorImage(const RetainedVectorImageModel& image)
    {
        for (const RetainedVectorImageDraw& draw : image.GetDraws())
        {
            const GlyphResource* shapeResource = nullptr;
            const bool ok = image.TryGetPreparedShapeResource(
                draw.shapeIndex,
                GetShapeResourceKind(draw),
                draw.strokeStyle,
                *this,
                shapeResource);
            if (!ok)
            {
                continue;
            }
        }

        for (const RetainedTextDraw& draw : image.GetTextDraws())
        {
            const GlyphResource* glyphResource = nullptr;
            const bool ok = (draw.flags & RetainedTextDrawFlagStroke) != 0
                ? m_context.TryGetStrokedGlyphResource(image.GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, draw.strokeStyle, glyphResource)
                : m_context.TryGetGlyphResource(image.GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, glyphResource);
            if (!ok)
            {
                continue;
            }
        }

        return true;
    }

    void Renderer::QueueDraw(const DrawGlyphDesc& desc)
    {
        if (!desc.glyph || !desc.glyph->atlas || (desc.glyph->vertexCount == 0))
        {
            return;
        }

        AppendDrawVertices(desc);
    }

    void Renderer::QueueQuad(const DrawQuadDesc& desc)
    {
        const float a00 = desc.size.x * desc.basisX.x;
        const float a01 = desc.size.x * desc.basisY.x;
        const float a10 = desc.size.y * desc.basisX.y;
        const float a11 = desc.size.y * desc.basisY.y;

        const float offsetX = desc.position.x + (desc.size.x * desc.offset.x);
        const float offsetY = desc.position.y + (desc.size.y * desc.offset.y);

        auto pushVertex = [&](float x, float y)
        {
            const float tx = offsetX + (a00 * x) + (a01 * y);
            const float ty = offsetY + (a10 * x) + (a11 * y);
            m_quadVertices.PushBack(MakeQuadVertex(tx, ty, desc.color));
        };

        m_quadVertices.Reserve(m_quadVertices.Size() + 6);
        pushVertex(0.0f, 0.0f);
        pushVertex(1.0f, 0.0f);
        pushVertex(1.0f, 1.0f);
        pushVertex(0.0f, 0.0f);
        pushVertex(1.0f, 1.0f);
        pushVertex(0.0f, 1.0f);
    }

    void Renderer::QueueRetainedText(const RetainedTextModel& text, const RetainedTextInstanceDesc& instance)
    {
        ReserveQueuedVertexCapacity(
            m_streamVertices.Size() + text.GetEstimatedVertexCount(),
            m_batches.Size() + text.GetDrawCount());

        const Span<const RetainedTextDraw> draws = text.GetDraws();
        for (uint32_t drawIndex = 0; drawIndex < draws.Size(); ++drawIndex)
        {
            const RetainedTextDraw& draw = draws[drawIndex];
            const GlyphResource* glyphResource = text.GetPreparedGlyphResource(drawIndex);
            if (glyphResource == nullptr)
            {
                const bool ok = (draw.flags & RetainedTextDrawFlagStroke) != 0
                    ? m_context.TryGetStrokedGlyphResource(text.GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, draw.strokeStyle, glyphResource)
                    : m_context.TryGetGlyphResource(text.GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, glyphResource);
                if (!ok)
                {
                    continue;
                }
            }

            DrawGlyphDesc desc{};
            desc.glyph = glyphResource;
            desc.position = {
                instance.origin.x + (draw.position.x * instance.scale),
                instance.origin.y + (draw.position.y * instance.scale)
            };
            desc.size = {
                draw.size.x * instance.scale,
                draw.size.y * instance.scale
            };
            desc.color = (draw.flags & RetainedTextDrawFlagUseForegroundColor) != 0
                ? MultiplyColor(instance.foregroundColor, draw.color)
                : draw.color;
            desc.basisX = draw.basisX;
            desc.basisY = draw.basisY;
            desc.offset = draw.offset;
            QueueDraw(desc);
        }

        for (const RetainedTextQuad& quad : text.GetQuads())
        {
            DrawQuadDesc desc{};
            desc.position = {
                instance.origin.x + (quad.position.x * instance.scale),
                instance.origin.y + (quad.position.y * instance.scale)
            };
            desc.size = {
                quad.size.x * instance.scale,
                quad.size.y * instance.scale
            };
            desc.color = MultiplyColor(quad.color, instance.foregroundColor);
            desc.basisX = quad.basisX;
            desc.basisY = quad.basisY;
            desc.offset = quad.offset;
            QueueQuad(desc);
        }
    }

    void Renderer::QueueRetainedVectorImage(const RetainedVectorImageModel& image, const RetainedVectorImageInstanceDesc& instance)
    {
        ReserveQueuedVertexCapacity(
            m_streamVertices.Size() + image.GetEstimatedVertexCount(),
            m_batches.Size() + image.GetDrawCount());

        for (const RetainedVectorImageDraw& draw : image.GetDraws())
        {
            const GlyphResource* shapeResource = nullptr;
            const bool ok = image.TryGetPreparedShapeResource(
                draw.shapeIndex,
                GetShapeResourceKind(draw),
                draw.strokeStyle,
                *this,
                shapeResource);
            if (!ok)
            {
                continue;
            }

            DrawGlyphDesc desc{};
            desc.glyph = shapeResource;
            desc.position = instance.origin;
            desc.size = { instance.scale, instance.scale };
            desc.color = MultiplyColor(draw.color, instance.tint);
            desc.offset = draw.offset;
            QueueDraw(desc);
        }

        for (const RetainedTextDraw& draw : image.GetTextDraws())
        {
            const GlyphResource* glyphResource = nullptr;
            const bool ok = (draw.flags & RetainedTextDrawFlagStroke) != 0
                ? m_context.TryGetStrokedGlyphResource(image.GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, draw.strokeStyle, glyphResource)
                : m_context.TryGetGlyphResource(image.GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, glyphResource);
            if (!ok)
            {
                continue;
            }

            DrawGlyphDesc desc{};
            desc.glyph = glyphResource;
            desc.position = {
                instance.origin.x + (draw.position.x * instance.scale),
                instance.origin.y + (draw.position.y * instance.scale)
            };
            desc.size = {
                draw.size.x * instance.scale,
                draw.size.y * instance.scale
            };
            desc.color = MultiplyColor(draw.color, instance.tint);
            desc.basisX = draw.basisX;
            desc.basisY = draw.basisY;
            desc.offset = draw.offset;
            QueueDraw(desc);
        }
    }

    void Renderer::EndFrame()
    {
        if (!m_frame.cmdList || !m_frame.targetView)
        {
            return;
        }

        rhi::ColorAttachment colorAttachment{};
        colorAttachment.action.load = m_frame.clearTarget ? rhi::LoadOp::Clear : rhi::LoadOp::Load;
        colorAttachment.action.store = rhi::StoreOp::Store;
        colorAttachment.action.clearValue = m_frame.clearColor;
        colorAttachment.view = m_frame.targetView;
        colorAttachment.state = m_frame.targetState;

        rhi::RenderPassDesc passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        m_frame.cmdList->BeginRenderPass(passDesc);

        if (m_frame.gpuTimer.querySet && m_frame.gpuTimer.resolveBuffer)
        {
            m_frame.cmdList->WriteTimestamp(m_frame.gpuTimer.querySet, m_frame.gpuTimer.startQueryIndex);
        }

        if (!m_streamVertices.IsEmpty())
        {
            rhi::Viewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(m_frame.targetSize.x);
            viewport.height = static_cast<float>(m_frame.targetSize.y);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            m_frame.cmdList->SetViewport(viewport);
            m_frame.cmdList->SetScissor({ 0, 0 }, m_frame.targetSize);
            m_frame.cmdList->SetRenderRootSignature(m_rootSignature);
            m_frame.cmdList->SetRenderPipeline(m_pipeline);
            m_frame.cmdList->SetBlendColor({ 0, 0, 0, 0 });

            StreamBuffer& streamBuffer = m_streamBuffers[m_streamBufferIndex];
            const uint32_t vertexDataSize = m_streamVertices.Size() * sizeof(PackedGlyphVertex);
            if (EnsureStreamBufferCapacity(
                    streamBuffer,
                    vertexDataSize,
                    sizeof(PackedGlyphVertex),
                    "Scribe Stream Vertex Buffer"))
            {
                void* dst = m_device->Map(streamBuffer.buffer, 0, vertexDataSize);
                if (dst)
                {
                    MemCopy(dst, m_streamVertices.Data(), vertexDataSize);
                    m_device->Unmap(streamBuffer.buffer);

                    float constants[VertexShaderConstantCount]{};
                    BuildFrameConstants(constants, m_frame.targetSize);
                    m_frame.cmdList->SetVertexBuffer(0, m_vertexBufferFormat, streamBuffer.buffer, 0, vertexDataSize);
                    m_frame.cmdList->SetRender32BitConstantValues(0, constants, VertexShaderConstantCount);
                    m_lastSubmittedDrawCount = m_batches.Size();

                    for (const StreamBatch& batch : m_batches)
                    {
                        m_frame.cmdList->SetRenderDescriptorTable(1, batch.atlas->descriptorTable);

                        rhi::DrawDesc desc{};
                        desc.vertexCount = batch.vertexCount;
                        desc.instanceCount = 1;
                        desc.vertexStart = batch.vertexStart;
                        desc.baseInstance = 0;
                        m_frame.cmdList->Draw(desc);
                    }
                }
            }
        }

        if (!m_quadVertices.IsEmpty())
        {
            StreamBuffer& quadStreamBuffer = m_quadStreamBuffers[m_streamBufferIndex];
            const uint32_t quadVertexDataSize = m_quadVertices.Size() * sizeof(PackedQuadVertex);
            if (EnsureStreamBufferCapacity(
                    quadStreamBuffer,
                    quadVertexDataSize,
                    sizeof(PackedQuadVertex),
                    "Scribe Quad Vertex Buffer"))
            {
                void* dst = m_device->Map(quadStreamBuffer.buffer, 0, quadVertexDataSize);
                if (dst)
                {
                    MemCopy(dst, m_quadVertices.Data(), quadVertexDataSize);
                    m_device->Unmap(quadStreamBuffer.buffer);

                    float constants[VertexShaderConstantCount]{};
                    BuildFrameConstants(constants, m_frame.targetSize);
                    m_frame.cmdList->SetRenderRootSignature(m_quadRootSignature);
                    m_frame.cmdList->SetRenderPipeline(m_quadPipeline);
                    m_frame.cmdList->SetRender32BitConstantValues(0, constants, VertexShaderConstantCount);
                    m_frame.cmdList->SetVertexBuffer(0, m_quadVertexBufferFormat, quadStreamBuffer.buffer, 0, quadVertexDataSize);

                    rhi::DrawDesc desc{};
                    desc.vertexCount = m_quadVertices.Size();
                    desc.instanceCount = 1;
                    desc.vertexStart = 0;
                    desc.baseInstance = 0;
                    m_frame.cmdList->Draw(desc);
                    ++m_lastSubmittedDrawCount;
                }
            }
        }

        if (m_frame.gpuTimer.querySet && m_frame.gpuTimer.resolveBuffer)
        {
            m_frame.cmdList->WriteTimestamp(m_frame.gpuTimer.querySet, m_frame.gpuTimer.endQueryIndex);
        }

        m_frame.cmdList->EndRenderPass();

        if (m_frame.gpuTimer.querySet && m_frame.gpuTimer.resolveBuffer)
        {
            const uint32_t firstQueryIndex = Min(m_frame.gpuTimer.startQueryIndex, m_frame.gpuTimer.endQueryIndex);
            const uint32_t lastQueryIndex = Max(m_frame.gpuTimer.startQueryIndex, m_frame.gpuTimer.endQueryIndex);
            m_frame.cmdList->ResolveTimestamps(
                m_frame.gpuTimer.querySet,
                firstQueryIndex,
                (lastQueryIndex - firstQueryIndex) + 1,
                m_frame.gpuTimer.resolveBuffer,
                m_frame.gpuTimer.resolveBufferOffset);
        }

        m_frame = {};
        m_batches.Clear();
        m_streamVertices.Clear();
        m_quadVertices.Clear();
    }

    bool Renderer::EnsureStreamBufferCapacity(StreamBuffer& streamBuffer, uint32_t minSize, uint32_t stride, const char* name)
    {
        if (streamBuffer.buffer && (streamBuffer.size >= minSize))
        {
            return true;
        }

        m_device->SafeDestroy(streamBuffer.buffer);
        streamBuffer.size = 0;

        const uint32_t requestedSize = Max(minSize, static_cast<uint32_t>(stride * 256));
        if (!CreateUploadBuffer(*m_device, streamBuffer.buffer, requestedSize, stride, name))
        {
            return false;
        }

        streamBuffer.size = requestedSize;
        return true;
    }

    void Renderer::AppendDrawVertices(const DrawGlyphDesc& draw)
    {
        HE_ASSERT(draw.glyph);
        HE_ASSERT(draw.glyph->atlas);

        StreamBatch* batch = nullptr;
        if (m_glyphBatchingEnabled && !m_batches.IsEmpty() && (m_batches.Back().atlas == draw.glyph->atlas))
        {
            batch = &m_batches.Back();
        }
        else
        {
            StreamBatch& newBatch = m_batches.EmplaceBack();
            newBatch.atlas = draw.glyph->atlas;
            newBatch.vertexStart = m_streamVertices.Size();
            newBatch.vertexCount = 0;
            batch = &newBatch;
        }

        const uint32_t oldSize = m_streamVertices.Size();
        m_streamVertices.Expand(draw.glyph->vertexCount, DefaultInit);
        const GlyphTransformState transformState = BuildGlyphTransformState(draw);
        for (uint32_t vertexIndex = 0; vertexIndex < draw.glyph->vertexCount; ++vertexIndex)
        {
            m_streamVertices[oldSize + vertexIndex] = TransformVertex(draw.glyph->vertices[vertexIndex], transformState);
        }

        batch->vertexCount += draw.glyph->vertexCount;
    }

    bool Renderer::CreateDeviceResources()
    {
        HE_ASSERT(m_device);

        {
            rhi::DescriptorRange ranges[2]{};
            ranges[0].type = rhi::DescriptorRangeType::Texture;
            ranges[0].baseRegister = 0;
            ranges[0].registerSpace = 0;
            ranges[0].count = 1;

            ranges[1].type = rhi::DescriptorRangeType::Texture;
            ranges[1].baseRegister = 1;
            ranges[1].registerSpace = 0;
            ranges[1].count = 1;

            rhi::SlotDesc slots[2]{};

            slots[0].type = rhi::SlotType::ConstantValues;
            slots[0].stage = rhi::ShaderStage::All;
            slots[0].constantValues.baseRegister = 0;
            slots[0].constantValues.registerSpace = 0;
            slots[0].constantValues.num32BitValues = VertexShaderConstantCount;

            slots[1].type = rhi::SlotType::DescriptorTable;
            slots[1].stage = rhi::ShaderStage::Pixel;
            slots[1].descriptorTable.rangeCount = HE_LENGTH_OF(ranges);
            slots[1].descriptorTable.ranges = ranges;

            rhi::RootSignatureDesc desc{};
            desc.slotCount = HE_LENGTH_OF(slots);
            desc.slots = slots;
            desc.inputAssembler = true;
            HE_RHI_SET_NAME(desc, "Scribe Root Signature");

            Result r = m_device->CreateRootSignature(desc, m_rootSignature);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create root signature. Error: {}", r);
                return false;
            }
        }

        {
            rhi::SlotDesc slots[1]{};
            slots[0].type = rhi::SlotType::ConstantValues;
            slots[0].stage = rhi::ShaderStage::All;
            slots[0].constantValues.baseRegister = 0;
            slots[0].constantValues.registerSpace = 0;
            slots[0].constantValues.num32BitValues = VertexShaderConstantCount;

            rhi::RootSignatureDesc desc{};
            desc.slotCount = HE_LENGTH_OF(slots);
            desc.slots = slots;
            desc.inputAssembler = true;
            HE_RHI_SET_NAME(desc, "Scribe Quad Root Signature");

            Result r = m_device->CreateRootSignature(desc, m_quadRootSignature);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create quad root signature. Error: {}", r);
                return false;
            }
        }

        {
            rhi::VertexAttributeDesc attributes[] =
            {
                { "ATTRIB", 0, rhi::Format::RGBA32Float, offsetof(PackedGlyphVertex, pos) },
                { "ATTRIB", 1, rhi::Format::RGBA32Float, offsetof(PackedGlyphVertex, tex) },
                { "ATTRIB", 2, rhi::Format::RGBA32Float, offsetof(PackedGlyphVertex, jac) },
                { "ATTRIB", 3, rhi::Format::RGBA32Float, offsetof(PackedGlyphVertex, bnd) },
                { "ATTRIB", 4, rhi::Format::RGBA32Float, offsetof(PackedGlyphVertex, col) },
            };

            rhi::VertexBufferFormatDesc desc{};
            desc.stride = sizeof(PackedGlyphVertex);
            desc.stepRate = rhi::StepRate::PerVertex;
            desc.attributeCount = HE_LENGTH_OF(attributes);
            desc.attributes = attributes;

            Result r = m_device->CreateVertexBufferFormat(desc, m_vertexBufferFormat);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create vertex buffer format. Error: {}", r);
                return false;
            }
        }

        {
            rhi::VertexAttributeDesc attributes[] =
            {
                { "ATTRIB", 0, rhi::Format::RG32Float, offsetof(PackedQuadVertex, pos) },
                { "ATTRIB", 1, rhi::Format::RGBA32Float, offsetof(PackedQuadVertex, col) },
            };

            rhi::VertexBufferFormatDesc desc{};
            desc.stride = sizeof(PackedQuadVertex);
            desc.stepRate = rhi::StepRate::PerVertex;
            desc.attributeCount = HE_LENGTH_OF(attributes);
            desc.attributes = attributes;

            Result r = m_device->CreateVertexBufferFormat(desc, m_quadVertexBufferFormat);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create quad vertex buffer format. Error: {}", r);
                return false;
            }
        }

        rhi::Shader* vs = nullptr;
        rhi::Shader* ps = nullptr;
        rhi::Shader* quadVs = nullptr;
        rhi::Shader* quadPs = nullptr;

        {
            rhi::ShaderDesc desc{};
            desc.stage = rhi::ShaderStage::Vertex;
        #if defined(HE_PLATFORM_API_WIN32)
            desc.code = c_scribe_vs_dxil;
            desc.codeSize = sizeof(c_scribe_vs_dxil);
        #else
            desc.code = c_scribe_vs_spv;
            desc.codeSize = sizeof(c_scribe_vs_spv);
        #endif
            Result r = m_device->CreateShader(desc, vs);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create vertex shader. Error: {}", r);
                return false;
            }
        }

        {
            rhi::ShaderDesc desc{};
            desc.stage = rhi::ShaderStage::Pixel;
        #if defined(HE_PLATFORM_API_WIN32)
            desc.code = c_scribe_ps_dxil;
            desc.codeSize = sizeof(c_scribe_ps_dxil);
        #else
            desc.code = c_scribe_ps_spv;
            desc.codeSize = sizeof(c_scribe_ps_spv);
        #endif
            Result r = m_device->CreateShader(desc, ps);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create pixel shader. Error: {}", r);
                m_device->SafeDestroy(vs);
                return false;
            }
        }

        {
            rhi::ShaderDesc desc{};
            desc.stage = rhi::ShaderStage::Vertex;
        #if defined(HE_PLATFORM_API_WIN32)
            desc.code = c_solid_vs_dxil;
            desc.codeSize = sizeof(c_solid_vs_dxil);
        #else
            desc.code = c_solid_vs_spv;
            desc.codeSize = sizeof(c_solid_vs_spv);
        #endif
            Result r = m_device->CreateShader(desc, quadVs);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create quad vertex shader. Error: {}", r);
                m_device->SafeDestroy(ps);
                m_device->SafeDestroy(vs);
                return false;
            }
        }

        {
            rhi::ShaderDesc desc{};
            desc.stage = rhi::ShaderStage::Pixel;
        #if defined(HE_PLATFORM_API_WIN32)
            desc.code = c_solid_ps_dxil;
            desc.codeSize = sizeof(c_solid_ps_dxil);
        #else
            desc.code = c_solid_ps_spv;
            desc.codeSize = sizeof(c_solid_ps_spv);
        #endif
            Result r = m_device->CreateShader(desc, quadPs);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create quad pixel shader. Error: {}", r);
                m_device->SafeDestroy(quadVs);
                m_device->SafeDestroy(ps);
                m_device->SafeDestroy(vs);
                return false;
            }
        }

        {
            const rhi::VertexBufferFormat* vbfs[]{ m_vertexBufferFormat };

            rhi::RenderPipelineDesc desc{};
            desc.rootSignature = m_rootSignature;
            desc.vertexShader = vs;
            desc.pixelShader = ps;
            desc.vertexBufferCount = 1;
            desc.vertexBufferFormats = vbfs;
            desc.primitiveType = rhi::PrimitiveType::TriList;
            desc.blend.targets[0].enable = true;
            desc.blend.targets[0].srcRgb = rhi::BlendFactor::One;
            desc.blend.targets[0].dstRgb = rhi::BlendFactor::InvSrcAlpha;
            desc.blend.targets[0].opRgb = rhi::BlendOp::Add;
            desc.blend.targets[0].srcAlpha = rhi::BlendFactor::One;
            desc.blend.targets[0].dstAlpha = rhi::BlendFactor::InvSrcAlpha;
            desc.blend.targets[0].opAlpha = rhi::BlendOp::Add;
            desc.raster.cullMode = rhi::CullMode::None;
            desc.raster.depthClamp = true;
            desc.depth.testEnable = false;
            desc.depth.writeEnable = false;
            desc.depth.func = rhi::ComparisonFunc::Always;
            desc.targets.renderTargetCount = 1;
            desc.targets.renderTargetFormats[0] = m_targetFormat;
            HE_RHI_SET_NAME(desc, "Scribe Render Pipeline");

            Result r = m_device->CreateRenderPipeline(desc, m_pipeline);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create render pipeline. Error: {}", r);
                m_device->SafeDestroy(quadPs);
                m_device->SafeDestroy(quadVs);
                m_device->SafeDestroy(ps);
                m_device->SafeDestroy(vs);
                return false;
            }
        }

        {
            const rhi::VertexBufferFormat* vbfs[]{ m_quadVertexBufferFormat };

            rhi::RenderPipelineDesc desc{};
            desc.rootSignature = m_quadRootSignature;
            desc.vertexShader = quadVs;
            desc.pixelShader = quadPs;
            desc.vertexBufferCount = 1;
            desc.vertexBufferFormats = vbfs;
            desc.primitiveType = rhi::PrimitiveType::TriList;
            desc.blend.targets[0].enable = true;
            desc.blend.targets[0].srcRgb = rhi::BlendFactor::One;
            desc.blend.targets[0].dstRgb = rhi::BlendFactor::InvSrcAlpha;
            desc.blend.targets[0].opRgb = rhi::BlendOp::Add;
            desc.blend.targets[0].srcAlpha = rhi::BlendFactor::One;
            desc.blend.targets[0].dstAlpha = rhi::BlendFactor::InvSrcAlpha;
            desc.blend.targets[0].opAlpha = rhi::BlendOp::Add;
            desc.raster.cullMode = rhi::CullMode::None;
            desc.raster.depthClamp = true;
            desc.depth.testEnable = false;
            desc.depth.writeEnable = false;
            desc.depth.func = rhi::ComparisonFunc::Always;
            desc.targets.renderTargetCount = 1;
            desc.targets.renderTargetFormats[0] = m_targetFormat;
            HE_RHI_SET_NAME(desc, "Scribe Quad Render Pipeline");

            Result r = m_device->CreateRenderPipeline(desc, m_quadPipeline);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create quad render pipeline. Error: {}", r);
                m_device->SafeDestroy(quadPs);
                m_device->SafeDestroy(quadVs);
                m_device->SafeDestroy(ps);
                m_device->SafeDestroy(vs);
                return false;
            }
        }

        m_device->SafeDestroy(quadPs);
        m_device->SafeDestroy(quadVs);
        m_device->SafeDestroy(ps);
        m_device->SafeDestroy(vs);
        return true;
    }

    void Renderer::DestroyDeviceResources()
    {
        if (!m_device)
        {
            return;
        }

        m_device->SafeDestroy(m_quadPipeline);
        m_device->SafeDestroy(m_quadVertexBufferFormat);
        m_device->SafeDestroy(m_quadRootSignature);
        m_device->SafeDestroy(m_pipeline);
        m_device->SafeDestroy(m_vertexBufferFormat);
        m_device->SafeDestroy(m_rootSignature);
    }
}
