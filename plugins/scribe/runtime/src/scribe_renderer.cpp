// Copyright Chad Engler

#include "he/scribe/renderer.h"

#include "he/scribe/compiled_font.h"

#include "shaders/scribe.shaders.h"

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

namespace he::scribe
{
    namespace
    {
        constexpr uint32_t VertexShaderConstantCount = 20;

        bool CreateUploadVertexBuffer(
            rhi::Device& device,
            rhi::Buffer*& out,
            const void* data,
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

            void* dst = device.Map(out);
            MemCopy(dst, data, size);
            device.Unmap(out);
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

            cmdList->Begin(cmdAllocator);
            cmdList->Copy(uploadBuffer, outTexture, copyRegion);
            cmdList->TransitionBarrier(outTexture, rhi::TextureState::CopyDst, rhi::TextureState::PixelShaderRead);
            cmdList->End();

            rhi::RenderCmdQueue& cmdQueue = device.GetRenderCmdQueue();
            cmdQueue.Submit(cmdList);
            cmdQueue.WaitForFlush();

            {
                rhi::TextureViewDesc desc{};
                desc.texture = outTexture;

                Result r = device.CreateTextureView(desc, outView);
                if (!r)
                {
                    HE_LOGF_ERROR(scribe_render, "Failed to create texture view '{}'. Error: {}", textureName, r);
                    return false;
                }
            }

            return true;
        }

        float PackBits(uint32_t value)
        {
            return BitCast<float>(value);
        }

        void BuildDrawConstants(float* outConstants, const Vec2u& targetSize, const Vec2f& position, const Vec2f& size)
        {
            HE_ASSERT(outConstants);

            const float width = static_cast<float>(targetSize.x);
            const float height = static_cast<float>(targetSize.y);

            outConstants[0] = (2.0f * size.x) / width;
            outConstants[1] = 0.0f;
            outConstants[2] = 0.0f;
            outConstants[3] = (2.0f * position.x) / width - 1.0f;

            outConstants[4] = 0.0f;
            outConstants[5] = (-2.0f * size.y) / height;
            outConstants[6] = 0.0f;
            outConstants[7] = 1.0f - (2.0f * position.y) / height;

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
        m_draws.Clear();
        m_frame = {};
        DestroyDeviceResources();
        m_targetFormat = rhi::Format::Invalid;
        m_device = nullptr;
    }

    bool Renderer::CreateGlyphResource(GlyphResource& out, const GlyphResourceCreateInfo& desc)
    {
        HE_ASSERT(m_device);

        if (!HE_VERIFY(
            desc.vertices != nullptr
            && desc.vertexCount > 0
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

        if (!CreateUploadVertexBuffer(
            *m_device,
            resource.vertexBuffer,
            desc.vertices,
            desc.vertexCount * sizeof(PackedGlyphVertex),
            sizeof(PackedGlyphVertex),
            "Scribe Glyph Vertex Buffer"))
        {
            DestroyGlyphResource(resource);
            return false;
        }

        if (!UploadTexture2D(
            *m_device,
            resource.curveTexture,
            resource.curveView,
            desc.curveTexture,
            rhi::Format::RGBA16Float,
            "Scribe Curve Texture",
            "Scribe Curve Upload Buffer"))
        {
            DestroyGlyphResource(resource);
            return false;
        }

        if (!UploadTexture2D(
            *m_device,
            resource.bandTexture,
            resource.bandView,
            desc.bandTexture,
            rhi::Format::RG16Uint,
            "Scribe Band Texture",
            "Scribe Band Upload Buffer"))
        {
            DestroyGlyphResource(resource);
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

        Result r = m_device->CreateDescriptorTable(tableDesc, resource.descriptorTable);
        if (!r)
        {
            HE_LOGF_ERROR(scribe_render, "Failed to create glyph descriptor table. Error: {}", r);
            DestroyGlyphResource(resource);
            return false;
        }

        const rhi::TextureView* curveView = resource.curveView;
        const rhi::TextureView* bandView = resource.bandView;
        m_device->SetTextureViews(resource.descriptorTable, 0, 0, 1, &curveView);
        m_device->SetTextureViews(resource.descriptorTable, 1, 0, 1, &bandView);

        resource.vertexCount = desc.vertexCount;
        out = resource;
        return true;
    }

    bool Renderer::CreateCompiledGlyphResource(
        GlyphResource& out,
        const LoadedFontFaceBlob& fontFace,
        uint32_t glyphIndex,
        const Vec4f& color)
    {
        CompiledGlyphResourceData glyphData{};
        if (!BuildCompiledGlyphResourceData(glyphData, fontFace, glyphIndex, color))
        {
            return false;
        }

        return CreateGlyphResource(out, glyphData.createInfo);
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
        if (!m_device)
        {
            resource = {};
            return;
        }

        m_device->SafeDestroy(resource.descriptorTable);
        m_device->SafeDestroy(resource.bandView);
        m_device->SafeDestroy(resource.bandTexture);
        m_device->SafeDestroy(resource.curveView);
        m_device->SafeDestroy(resource.curveTexture);
        m_device->SafeDestroy(resource.vertexBuffer);
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
        m_draws.Clear();
        return true;
    }

    void Renderer::QueueDraw(const DrawGlyphDesc& desc)
    {
        if (!desc.glyph || !desc.glyph->vertexBuffer || (desc.glyph->vertexCount == 0))
        {
            return;
        }

        QueuedDraw& draw = m_draws.EmplaceBack();
        draw.glyph = desc.glyph;
        draw.position = desc.position;
        draw.size = desc.size;
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

        if (!m_draws.IsEmpty())
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

            for (const QueuedDraw& draw : m_draws)
            {
                EmitDraw(draw);
            }
        }

        m_frame.cmdList->EndRenderPass();
        m_frame = {};
        m_draws.Clear();
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
            slots[0].stage = rhi::ShaderStage::Vertex;
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

    void Renderer::EmitDraw(const QueuedDraw& draw)
    {
        HE_ASSERT(m_frame.cmdList);
        HE_ASSERT(draw.glyph);

        float constants[VertexShaderConstantCount]{};
        BuildDrawConstants(constants, m_frame.targetSize, draw.position, draw.size);

        m_frame.cmdList->SetVertexBuffer(
            0,
            m_vertexBufferFormat,
            draw.glyph->vertexBuffer,
            0,
            draw.glyph->vertexCount * sizeof(PackedGlyphVertex));
        m_frame.cmdList->SetRender32BitConstantValues(0, constants, VertexShaderConstantCount);
        m_frame.cmdList->SetRenderDescriptorTable(1, draw.glyph->descriptorTable);

        rhi::DrawDesc desc{};
        desc.vertexCount = draw.glyph->vertexCount;
        desc.instanceCount = 1;
        desc.vertexStart = 0;
        desc.baseInstance = 0;
        m_frame.cmdList->Draw(desc);
    }
}
