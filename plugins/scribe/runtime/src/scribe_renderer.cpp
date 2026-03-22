// Copyright Chad Engler

#include "he/scribe/renderer.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/compiled_vector_image.h"

#include "shaders/scribe.shaders.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/utils.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/device.h"

#include <cstddef>
#include <utility>

namespace he::scribe
{
    struct GlyphAtlas
    {
        rhi::Texture* curveTexture{ nullptr };
        rhi::TextureView* curveView{ nullptr };
        rhi::Texture* bandTexture{ nullptr };
        rhi::TextureView* bandView{ nullptr };
        rhi::DescriptorTable* descriptorTable{ nullptr };
        const void* curveData{ nullptr };
        Vec2u curveSize{ 0, 0 };
        uint32_t curveRowPitch{ 0 };
        const void* bandData{ nullptr };
        Vec2u bandSize{ 0, 0 };
        uint32_t bandRowPitch{ 0 };
        uint32_t refCount{ 0 };
        bool cached{ false };
    };

    namespace
    {
        constexpr uint32_t VertexShaderConstantCount = 20;

        bool CreateUploadBuffer(
            rhi::Device& device,
            rhi::Buffer*& out,
            uint32_t size,
            uint32_t stride,
            const char* name)
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

        bool UploadTexture2D(
            rhi::Device& device,
            rhi::Texture*& outTexture,
            rhi::TextureView*& outView,
            const TextureDataDesc& textureData,
            rhi::Format format,
            const char* textureName,
            const char* uploadBufferName)
        {
            const Vec3u textureSize{ textureData.size.x, textureData.size.y, 1 };

            {
                rhi::TextureDesc desc{};
                desc.type = rhi::TextureType::_2D;
                desc.format = format;
                desc.size = textureSize;
                desc.initialState = rhi::TextureState::CopyDst;
                HE_RHI_SET_NAME(desc, textureName);

                Result r = device.CreateTexture(desc, outTexture);
                if (!r)
                {
                    HE_LOGF_ERROR(scribe_render, "Failed to create texture '{}'. Error: {}", textureName, r);
                    return false;
                }
            }

            const uint32_t alignment = device.GetDeviceInfo().uploadDataPitchAlignment;
            const uint32_t uploadPitch = AlignUp<uint32_t>(textureData.rowPitch, alignment);
            const uint32_t uploadSize = textureData.size.y * uploadPitch;

            rhi::Buffer* uploadBuffer = nullptr;
            HE_AT_SCOPE_EXIT([&]() { device.SafeDestroy(uploadBuffer); });

            {
                rhi::BufferDesc desc{};
                desc.heapType = rhi::HeapType::Upload;
                desc.usage = rhi::BufferUsage::CopySrc;
                desc.size = uploadSize;
                HE_RHI_SET_NAME(desc, uploadBufferName);

                Result r = device.CreateBuffer(desc, uploadBuffer);
                if (!r)
                {
                    HE_LOGF_ERROR(scribe_render, "Failed to create upload buffer '{}'. Error: {}", uploadBufferName, r);
                    return false;
                }
            }

            uint8_t* uploadMem = static_cast<uint8_t*>(device.Map(uploadBuffer));
            const uint8_t* src = static_cast<const uint8_t*>(textureData.data);
            for (uint32_t y = 0; y < textureData.size.y; ++y)
            {
                MemCopy(uploadMem + (y * uploadPitch), src + (y * textureData.rowPitch), textureData.rowPitch);
            }
            device.Unmap(uploadBuffer);

            rhi::CmdAllocator* cmdAllocator = nullptr;
            rhi::RenderCmdList* cmdList = nullptr;
            HE_AT_SCOPE_EXIT([&]()
            {
                device.SafeDestroy(cmdList);
                device.SafeDestroy(cmdAllocator);
            });

            {
                rhi::CmdAllocatorDesc desc{};
                desc.type = rhi::CmdListType::Render;
                HE_RHI_SET_NAME(desc, "Scribe Upload Cmd Allocator");

                Result r = device.CreateCmdAllocator(desc, cmdAllocator);
                if (!r)
                {
                    HE_LOGF_ERROR(scribe_render, "Failed to create upload cmd allocator. Error: {}", r);
                    return false;
                }
            }

            {
                rhi::CmdListDesc desc{};
                desc.alloc = cmdAllocator;
                HE_RHI_SET_NAME(desc, "Scribe Upload Cmd List");

                Result r = device.CreateRenderCmdList(desc, cmdList);
                if (!r)
                {
                    HE_LOGF_ERROR(scribe_render, "Failed to create upload cmd list. Error: {}", r);
                    return false;
                }
            }

            rhi::BufferTextureCopy copyRegion{};
            copyRegion.bufferRowPitch = uploadPitch;
            copyRegion.textureSize = textureSize;

            Result r = cmdList->Begin(cmdAllocator);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to begin upload cmd list. Error: {}", r);
                return false;
            }

            cmdList->Copy(uploadBuffer, outTexture, copyRegion);
            cmdList->TransitionBarrier(outTexture, rhi::TextureState::CopyDst, rhi::TextureState::PixelShaderRead);
            r = cmdList->End();
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to end upload cmd list. Error: {}", r);
                return false;
            }

            rhi::RenderCmdQueue& cmdQueue = device.GetRenderCmdQueue();
            cmdQueue.Submit(cmdList);
            cmdQueue.WaitForFlush();

            {
                rhi::TextureViewDesc desc{};
                desc.texture = outTexture;

                Result viewResult = device.CreateTextureView(desc, outView);
                if (!viewResult)
                {
                    HE_LOGF_ERROR(scribe_render, "Failed to create texture view '{}'. Error: {}", textureName, viewResult);
                    return false;
                }
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

        PackedGlyphVertex TransformVertex(const PackedGlyphVertex& in, const DrawGlyphDesc& draw)
        {
            const float a00 = draw.size.x * draw.basisX.x;
            const float a01 = draw.size.x * draw.basisY.x;
            const float a10 = draw.size.y * draw.basisX.y;
            const float a11 = draw.size.y * draw.basisY.y;

            const float offsetX = draw.position.x + (draw.size.x * draw.offset.x);
            const float offsetY = draw.position.y + (draw.size.y * draw.offset.y);

            PackedGlyphVertex out = in;
            out.pos.x = offsetX + (a00 * in.pos.x) + (a01 * in.pos.y);
            out.pos.y = offsetY + (a10 * in.pos.x) + (a11 * in.pos.y);
            out.pos.z = (a00 * in.pos.z) + (a01 * in.pos.w);
            out.pos.w = (a10 * in.pos.z) + (a11 * in.pos.w);

            const float det = (a00 * a11) - (a01 * a10);
            if (Abs(det) > 1.0e-8f)
            {
                const float invDet = 1.0f / det;
                const float it00 = a11 * invDet;
                const float it01 = -a10 * invDet;
                const float it10 = -a01 * invDet;
                const float it11 = a00 * invDet;

                const float j0x = in.jac.x;
                const float j0y = in.jac.y;
                const float j1x = in.jac.z;
                const float j1y = in.jac.w;
                out.jac.x = (it00 * j0x) + (it10 * j0y);
                out.jac.y = (it01 * j0x) + (it11 * j0y);
                out.jac.z = (it00 * j1x) + (it10 * j1y);
                out.jac.w = (it01 * j1x) + (it11 * j1y);
            }
            else
            {
                out.jac = in.jac;
            }

            out.col = MultiplyColor(in.col, draw.color);
            return out;
        }
    }

    Renderer::~Renderer() noexcept
    {
        Terminate();
    }

    bool Renderer::Initialize(rhi::Device& device, rhi::Format targetFormat)
    {
        HE_ASSERT(!m_device);

        m_device = &device;
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
        m_frame = {};
        Vector<GlyphAtlas*> cachedAtlases = std::move(m_cachedAtlases);
        m_cachedAtlases.Clear();
        for (GlyphAtlas*& atlas : cachedAtlases)
        {
            atlas->cached = false;
            ReleaseAtlas(atlas);
        }
        for (StreamBuffer& streamBuffer : m_streamBuffers)
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
        atlas->refCount = 1;

        if (!UploadTexture2D(
            *m_device,
            atlas->curveTexture,
            atlas->curveView,
            curveTexture,
            rhi::Format::RGBA16Float,
            "Scribe Curve Texture",
            "Scribe Curve Upload Buffer"))
        {
            ReleaseAtlas(atlas);
            return false;
        }

        if (!UploadTexture2D(
            *m_device,
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

    bool Renderer::CreateCachedAtlas(
        GlyphAtlas*& out,
        const TextureDataDesc& curveTexture,
        const TextureDataDesc& bandTexture)
    {
        out = nullptr;
        for (GlyphAtlas* atlas : m_cachedAtlases)
        {
            if (atlas
                && atlas->cached
                && (atlas->curveData == curveTexture.data)
                && (atlas->curveSize.x == curveTexture.size.x)
                && (atlas->curveSize.y == curveTexture.size.y)
                && (atlas->curveRowPitch == curveTexture.rowPitch)
                && (atlas->bandData == bandTexture.data)
                && (atlas->bandSize.x == bandTexture.size.x)
                && (atlas->bandSize.y == bandTexture.size.y)
                && (atlas->bandRowPitch == bandTexture.rowPitch))
            {
                ++atlas->refCount;
                out = atlas;
                return true;
            }
        }

        GlyphAtlas* atlas = nullptr;
        if (!CreateDedicatedAtlas(atlas, curveTexture, bandTexture))
        {
            return false;
        }

        atlas->cached = true;
        atlas->curveData = curveTexture.data;
        atlas->curveSize = curveTexture.size;
        atlas->curveRowPitch = curveTexture.rowPitch;
        atlas->bandData = bandTexture.data;
        atlas->bandSize = bandTexture.size;
        atlas->bandRowPitch = bandTexture.rowPitch;
        m_cachedAtlases.PushBack(atlas);
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

        HE_ASSERT(atlas->refCount > 0);
        --atlas->refCount;
        if (atlas->refCount == 0)
        {
            if (atlas->cached)
            {
                for (uint32_t index = 0; index < m_cachedAtlases.Size(); ++index)
                {
                    if (m_cachedAtlases[index] == atlas)
                    {
                        m_cachedAtlases.Erase(index);
                        break;
                    }
                }
            }

            m_device->SafeDestroy(atlas->descriptorTable);
            m_device->SafeDestroy(atlas->bandView);
            m_device->SafeDestroy(atlas->bandTexture);
            m_device->SafeDestroy(atlas->curveView);
            m_device->SafeDestroy(atlas->curveTexture);
            Allocator::GetDefault().Delete(atlas);
        }

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

    bool Renderer::CreateCompiledGlyphResource(
        GlyphResource& out,
        const LoadedFontFaceBlob& fontFace,
        uint32_t glyphIndex)
    {
        CompiledGlyphResourceData glyphData{};
        if (!BuildCompiledGlyphResourceData(glyphData, fontFace, glyphIndex))
        {
            return false;
        }

        GlyphResource resource{};
        MemCopy(resource.vertices, glyphData.vertices, sizeof(glyphData.vertices));
        resource.vertexCount = glyphData.createInfo.vertexCount;
        if (!CreateCachedAtlas(resource.atlas, glyphData.createInfo.curveTexture, glyphData.createInfo.bandTexture))
        {
            DestroyGlyphResource(resource);
            return false;
        }

        out = resource;
        return true;
    }

    bool Renderer::CreateCompiledVectorShapeResource(
        GlyphResource& out,
        const LoadedVectorImageBlob& image,
        uint32_t shapeIndex)
    {
        CompiledVectorShapeResourceData shapeData{};
        if (!BuildCompiledVectorShapeResourceData(shapeData, image, shapeIndex))
        {
            return false;
        }

        GlyphResource resource{};
        MemCopy(resource.vertices, shapeData.vertices, sizeof(shapeData.vertices));
        resource.vertexCount = shapeData.createInfo.vertexCount;
        if (!CreateCachedAtlas(resource.atlas, shapeData.createInfo.curveTexture, shapeData.createInfo.bandTexture))
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
        m_lastSubmittedDrawCount = 0;
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
            if (EnsureStreamBufferCapacity(streamBuffer, vertexDataSize))
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
    }

    bool Renderer::EnsureStreamBufferCapacity(StreamBuffer& streamBuffer, uint32_t minSize)
    {
        if (streamBuffer.buffer && (streamBuffer.size >= minSize))
        {
            return true;
        }

        m_device->SafeDestroy(streamBuffer.buffer);
        streamBuffer.size = 0;

        const uint32_t requestedSize = Max(minSize, static_cast<uint32_t>(sizeof(PackedGlyphVertex) * 256));
        if (!CreateUploadBuffer(*m_device, streamBuffer.buffer, requestedSize, sizeof(PackedGlyphVertex), "Scribe Stream Vertex Buffer"))
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
        if (!m_batches.IsEmpty() && (m_batches.Back().atlas == draw.glyph->atlas))
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
        m_streamVertices.Resize(oldSize + draw.glyph->vertexCount);
        for (uint32_t vertexIndex = 0; vertexIndex < draw.glyph->vertexCount; ++vertexIndex)
        {
            m_streamVertices[oldSize + vertexIndex] = TransformVertex(draw.glyph->vertices[vertexIndex], draw);
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

        rhi::Shader* vs = nullptr;
        rhi::Shader* ps = nullptr;

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
            const rhi::VertexBufferFormat* vbfs[]{ m_vertexBufferFormat };

            rhi::RenderPipelineDesc desc{};
            desc.rootSignature = m_rootSignature;
            desc.vertexShader = vs;
            desc.pixelShader = ps;
            desc.vertexBufferCount = 1;
            desc.vertexBufferFormats = vbfs;
            desc.primitiveType = rhi::PrimitiveType::TriList;
            desc.blend.targets[0].enable = true;
            desc.blend.targets[0].srcRgb = rhi::BlendFactor::SrcAlpha;
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
                m_device->SafeDestroy(ps);
                m_device->SafeDestroy(vs);
                return false;
            }
        }

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

        m_device->SafeDestroy(m_pipeline);
        m_device->SafeDestroy(m_vertexBufferFormat);
        m_device->SafeDestroy(m_rootSignature);
    }
}
