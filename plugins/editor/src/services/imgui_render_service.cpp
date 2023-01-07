// Copyright Chad Engler

#include "imgui_render_service.h"
#include "shaders/imgui.shaders.h"

#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/utils.h"
#include "he/rhi/device.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"

#include "imgui.h"

#include <cmath>

namespace he::editor
{
    //static float SRGBToLinear(float value)
    //{
    //    if (value <= 0.04045f)
    //        return value / 12.92f;
    //    else
    //        return powf((value + 0.055f) / 1.055f, 2.4f);
    //}

    //static Vec4f SRGBToLinear3(Vec4f value)
    //{
    //    return Vec4f{ SRGBToLinear(value.x), SRGBToLinear(value.y), SRGBToLinear(value.z), value.w };
    //}

    ImGuiRenderService::ImGuiRenderService(RenderService& renderService) noexcept
        : m_renderService(renderService)
    {}

    bool ImGuiRenderService::Initialize(rhi::SwapChainFormat swapChainFormat)
    {
        m_swapChainFormat = swapChainFormat;

        // Setup backend capabilities flags
        ImGuiIO& io = ImGui::GetIO();
        HE_ASSERT(io.BackendRendererUserData == nullptr);

        io.BackendRendererName = "imgui_impl_harvest";
        io.BackendRendererUserData = this;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports; // We can create multi-viewports on the Renderer side (optional)

        // Create a dummy entry for the main window. The render service manages the main window's
        // resources so we don't actually initialize this one.
        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        mainViewport->RendererUserData = nullptr;

        // Setup backend capabilities flags
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
            pio.Renderer_CreateWindow = &ImGuiRenderService::CreatePlatformWindow;
            pio.Renderer_DestroyWindow = &ImGuiRenderService::DestroyPlatformWindow;
            pio.Renderer_SetWindowSize = &ImGuiRenderService::SetWindowSize;
            pio.Renderer_RenderWindow = &ImGuiRenderService::RenderWindow;
            pio.Renderer_SwapBuffers = &ImGuiRenderService::SwapBuffers;
        }

        return true;
    }

    void ImGuiRenderService::Terminate()
    {
        rhi::Device* device = m_renderService.GetDevice();

        device->GetRenderCmdQueue().WaitForFlush();

        for (uint32_t i = 0; i < HE_LENGTH_OF(m_frameBuffers); ++i)
        {
            device->SafeDestroy(m_frameBuffers[i].indexBuffer);
            device->SafeDestroy(m_frameBuffers[i].vertexBuffer);
        }

        ImGui::DestroyPlatformWindows();
        DestroyAllFontResources();
        DestroyDeviceResources();
    }

    void ImGuiRenderService::NewFrame()
    {
        if (!m_rootSignature)
        {
            if (!CreateDeviceResources())
                DestroyDeviceResources();
        }

        // Font must've already been created
        HE_ASSERT(ImGui::GetIO().Fonts->TexID);
    }

    void ImGuiRenderService::Render()
    {
        m_frameIndex = (m_frameIndex + 1) % HE_LENGTH_OF(m_frameBuffers);
        FrameBuffers& buffers = m_frameBuffers[m_frameIndex];

        rhi::RenderCmdList* cmdList = m_renderService.GetCmdList();

        static ImVec4 TitleBgActive = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];
        static const Vec4f ClearColor{ TitleBgActive.x, TitleBgActive.y, TitleBgActive.z, TitleBgActive.w };

        rhi::ColorAttachment colorAttach{};
        colorAttach.action.load = rhi::LoadOp::Clear;
        colorAttach.action.store = rhi::StoreOp::Store;
        colorAttach.action.clearValue = ClearColor;
        colorAttach.view = m_renderService.GetPresentTarget().renderTargetView;
        colorAttach.state = rhi::TextureState::Present;

        rhi::RenderPassDesc renderPass{};
        renderPass.colorAttachmentCount = 1;
        renderPass.colorAttachments = &colorAttach;

        cmdList->BeginRenderPass(renderPass);
        RenderDrawData(ImGui::GetDrawData(), cmdList, buffers);
        cmdList->EndRenderPass();
    }

    bool ImGuiRenderService::SetupFontAtlas(ImFontAtlas& atlas)
    {
        if (!CreateFontResources(atlas))
        {
            DestroyFontResources(m_fontResources.Back());
            m_fontResources.PopBack();
            return false;
        }

        return true;
    }

    void ImGuiRenderService::CreatePlatformWindow(ImGuiViewport* viewport)
    {
        ImGuiRenderService* service = static_cast<ImGuiRenderService*>(ImGui::GetIO().BackendRendererUserData);
        rhi::Device* device = service->m_renderService.GetDevice();

        ViewportData* data = Allocator::GetDefault().New<ViewportData>();

        viewport->RendererUserData = data;

        // PlatformHandleRaw should always be a native view handle, whereas PlatformHandle might
        // be a higher-level handle (e.g. GLFWWindow*, SDL_Window*). Some backends will leave
        // PlatformHandleRaw NULL, in which case we assume PlatformHandle will contain the nvh.
        void* nvh = viewport->PlatformHandleRaw ? viewport->PlatformHandleRaw : viewport->PlatformHandle;
        HE_ASSERT(nvh);

        // Create frame resources
        rhi::RenderCmdQueue& cmdQueue = device->GetRenderCmdQueue();
        for (uint32_t i = 0; i < HE_LENGTH_OF(data->frameDatas); ++i)
        {
            FrameData& frame = data->frameDatas[i];

            {
                rhi::CmdAllocatorDesc desc{};
                desc.type = rhi::CmdListType::Render;
                HE_RHI_SET_NAME(desc, "ImGui Viewport Frame Command Allocator");
                Result r = device->CreateCmdAllocator(desc, frame.cmdAlloc);
                HE_ASSERT(r, HE_KV(result, r));
            }

            {
                rhi::CpuFenceDesc desc{};
                Result r = device->CreateCpuFence(desc, frame.fence);
                HE_ASSERT(r, HE_KV(result, r));
            }

            cmdQueue.Signal(frame.fence);
        }

        // Create command list
        {
            rhi::CmdListDesc desc{};
            desc.alloc = data->frameDatas[0].cmdAlloc;
            HE_RHI_SET_NAME(desc, "ImGui Viewport Command List");

            Result r = device->CreateRenderCmdList(desc, data->cmdList);
            HE_ASSERT(r, HE_KV(result, r));
        }

        // Create swap chain
        {
            rhi::SwapChainDesc desc{};
            desc.nativeViewHandle = nvh;
            desc.bufferCount = HE_LENGTH_OF(data->frameDatas);
            desc.format = service->m_swapChainFormat;
            desc.size = { static_cast<int32_t>(viewport->Size.x), static_cast<int32_t>(viewport->Size.y) };

            Result r = device->CreateSwapChain(desc, data->swapChain);
            HE_ASSERT(r, HE_KV(result, r));
        }
    }

    void ImGuiRenderService::DestroyPlatformWindow(ImGuiViewport* viewport)
    {
        ImGuiRenderService* service = static_cast<ImGuiRenderService*>(ImGui::GetIO().BackendRendererUserData);
        rhi::Device* device = service->m_renderService.GetDevice();
        ViewportData* data = static_cast<ViewportData*>(viewport->RendererUserData);

        if (!data)
            return;

        device->GetRenderCmdQueue().WaitForFlush();

        device->SafeDestroy(data->cmdList);
        device->SafeDestroy(data->swapChain);

        for (uint32_t i = 0; i < HE_LENGTH_OF(data->frameDatas); ++i)
        {
            FrameData& frame = data->frameDatas[i];
            device->SafeDestroy(frame.cmdAlloc);
            device->SafeDestroy(frame.fence);
            device->SafeDestroy(frame.buffers.indexBuffer);
            device->SafeDestroy(frame.buffers.vertexBuffer);
        }

        Allocator::GetDefault().Delete(data);
        viewport->RendererUserData = nullptr;
    }

    void ImGuiRenderService::SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
    {
        ViewportData* data = static_cast<ViewportData*>(viewport->RendererUserData);
        if (!data->swapChain)
            return;

        ImGuiRenderService* service = static_cast<ImGuiRenderService*>(ImGui::GetIO().BackendRendererUserData);
        rhi::Device* device = service->m_renderService.GetDevice();

        rhi::SwapChainDesc desc{};
        desc.bufferCount = HE_LENGTH_OF(data->frameDatas);
        desc.format = service->m_swapChainFormat;
        desc.size = { static_cast<int32_t>(size.x), static_cast<int32_t>(size.y) };

        Result r = device->UpdateSwapChain(data->swapChain, desc);
        HE_ASSERT(r, HE_KV(result, r));
    }

    void ImGuiRenderService::RenderWindow(ImGuiViewport* viewport, void*)
    {
        ImGuiRenderService* service = static_cast<ImGuiRenderService*>(ImGui::GetIO().BackendRendererUserData);
        rhi::Device* device = service->m_renderService.GetDevice();
        ViewportData* data = static_cast<ViewportData*>(viewport->RendererUserData);

        rhi::PresentTarget presentTarget = device->AcquirePresentTarget(data->swapChain);

        data->frameIndex = (data->frameIndex + 1) % HE_LENGTH_OF(data->frameDatas);
        FrameData& frame = data->frameDatas[data->frameIndex];

        device->WaitForFence(frame.fence);
        device->ResetCmdAllocator(frame.cmdAlloc);

        rhi::RenderCmdList* cmdList = data->cmdList;
        cmdList->Begin(frame.cmdAlloc);

        rhi::ColorAttachment colorAttach{};
        if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
            colorAttach.action.load = rhi::LoadOp::Clear;
        else
            colorAttach.action.load = rhi::LoadOp::Load;
        colorAttach.action.store = rhi::StoreOp::Store;
        colorAttach.action.clearValue = { 0, 0, 0, 1.0f };
        colorAttach.view = presentTarget.renderTargetView;
        colorAttach.state = rhi::TextureState::Present;

        rhi::RenderPassDesc renderPass{};
        renderPass.colorAttachmentCount = 1;
        renderPass.colorAttachments = &colorAttach;

        cmdList->BeginRenderPass(renderPass);
        service->RenderDrawData(viewport->DrawData, cmdList, frame.buffers);
        cmdList->EndRenderPass();

        cmdList->End();

        rhi::RenderCmdQueue& cmdQueue = device->GetRenderCmdQueue();
        cmdQueue.Submit(cmdList);
    }

    void ImGuiRenderService::SwapBuffers(ImGuiViewport* viewport, void*)
    {
        ImGuiRenderService* service = static_cast<ImGuiRenderService*>(ImGui::GetIO().BackendRendererUserData);
        rhi::Device* device = service->m_renderService.GetDevice();
        ViewportData* data = static_cast<ViewportData*>(viewport->RendererUserData);
        FrameData& frame = data->frameDatas[data->frameIndex];

        rhi::RenderCmdQueue& cmdQueue = device->GetRenderCmdQueue();
        cmdQueue.Present(data->swapChain);
        cmdQueue.Signal(frame.fence);
    }

    void ImGuiRenderService::RenderDrawData(ImDrawData* drawData, rhi::RenderCmdList* cmdList, FrameBuffers& buffers)
    {
        // Avoid rendering when minimized
        if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
            return;

        rhi::Device* device = m_renderService.GetDevice();

        // Create or grow vertex buffer if needed
        if (buffers.vertexBuffer == nullptr || buffers.vertexBufferSize < drawData->TotalVtxCount)
        {
            device->SafeDestroy(buffers.vertexBuffer);
            buffers.vertexBufferSize = drawData->TotalVtxCount + 5000;

            rhi::BufferDesc desc{};
            desc.heapType = rhi::HeapType::Upload;
            desc.usage = rhi::BufferUsage::Vertices;
            desc.initialState = rhi::BufferState::Vertices;
            desc.size = buffers.vertexBufferSize * sizeof(ImDrawVert);
            HE_RHI_SET_NAME(desc, "ImGui Frame Vertex Buffer");

            Result r = device->CreateBuffer(desc, buffers.vertexBuffer);
            HE_ASSERT(r, HE_KV(result, r));
        }

        // Create or grow index buffer if needed
        if (buffers.indexBuffer == nullptr || buffers.indexBufferSize < drawData->TotalIdxCount)
        {
            device->SafeDestroy(buffers.indexBuffer);
            buffers.indexBufferSize = drawData->TotalIdxCount + 10000;

            rhi::BufferDesc desc{};
            desc.heapType = rhi::HeapType::Upload;
            desc.usage = rhi::BufferUsage::Indices;
            desc.initialState = rhi::BufferState::Indices;
            desc.size = buffers.indexBufferSize * sizeof(ImDrawIdx);
            HE_RHI_SET_NAME(desc, "ImGui Frame Index Buffer");

            Result r = device->CreateBuffer(desc, buffers.indexBuffer);
            HE_ASSERT(r, HE_KV(result, r));
        }

        // Upload vertex/index data into a single contiguous GPU buffer
        void* vertexBufferMem = device->Map(buffers.vertexBuffer);
        void* indexBufferMem = device->Map(buffers.indexBuffer);

        ImDrawVert* vertices = static_cast<ImDrawVert*>(vertexBufferMem);
        ImDrawIdx* indices = static_cast<ImDrawIdx*>(indexBufferMem);
        for (int32_t i = 0; i < drawData->CmdListsCount; ++i)
        {
            const ImDrawList* drawList = drawData->CmdLists[i];
            MemCopy(vertices, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
            MemCopy(indices, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
            vertices += drawList->VtxBuffer.Size;
            indices += drawList->IdxBuffer.Size;
        }
        device->Unmap(buffers.vertexBuffer);
        device->Unmap(buffers.indexBuffer);

        // Setup desired DX state
        SetupRenderState(drawData, cmdList, buffers);

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int vertexOffset = 0;
        int indexOffset = 0;
        ImVec2 clipOffset = drawData->DisplayPos;
        for (int32_t i = 0; i < drawData->CmdListsCount; ++i)
        {
            const ImDrawList* drawList = drawData->CmdLists[i];

            for (int32_t j = 0; j < drawList->CmdBuffer.Size; ++j)
            {
                const ImDrawCmd* drawCmd = &drawList->CmdBuffer[j];

                if (drawCmd->UserCallback)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (drawCmd->UserCallback == ImDrawCallback_ResetRenderState)
                        SetupRenderState(drawData, cmdList, buffers);
                    else
                        drawCmd->UserCallback(drawList, drawCmd);
                }
                else
                {
                    // Apply Scissor, Bind texture, Draw
                    const float left = drawCmd->ClipRect.x - clipOffset.x;
                    const float top = drawCmd->ClipRect.y - clipOffset.y;
                    const float right = drawCmd->ClipRect.z - clipOffset.x;
                    const float bottom = drawCmd->ClipRect.w - clipOffset.y;
                    if (right > left && bottom > top)
                    {
                        const rhi::DescriptorTable* table = drawCmd->GetTexID();
                        cmdList->SetRenderDescriptorTable(1, table);

                        Vec2u scissorPos
                        {
                            static_cast<uint32_t>(left),
                            static_cast<uint32_t>(top),
                        };
                        Vec2u scissorSize
                        {
                            static_cast<uint32_t>(right - left),
                            static_cast<uint32_t>(bottom - top),
                        };
                        cmdList->SetScissor(scissorPos, scissorSize);

                        rhi::DrawIndexedDesc desc{};
                        desc.indexCount = drawCmd->ElemCount;
                        desc.instanceCount = 1;
                        desc.indexStart = drawCmd->IdxOffset + indexOffset;
                        desc.baseVertex = drawCmd->VtxOffset + vertexOffset;
                        desc.baseInstance = 0;
                        cmdList->DrawIndexed(desc);
                    }
                }
            }
            indexOffset += drawList->IdxBuffer.Size;
            vertexOffset += drawList->VtxBuffer.Size;
        }
    }

    void ImGuiRenderService::SetupRenderState(ImDrawData* drawData, rhi::RenderCmdList* cmdList, FrameBuffers& buffers)
    {
        // Setup orthographic projection matrix into our constant buffer
        const float L = drawData->DisplayPos.x;
        const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        const float T = drawData->DisplayPos.y;
        const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        const float mvp[4][4] =
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };
        const Vec4f blendColor{ 0, 0, 0, 0 };

        // Setup viewport
        rhi::Viewport vp{};
        vp.x = 0.0f;
        vp.y = 0.0f;
        vp.width = drawData->DisplaySize.x;
        vp.height = drawData->DisplaySize.y;
        vp.minZ = 0.0f;
        vp.maxZ = 1.0f;
        cmdList->SetViewport(vp);

        // Bind shader resources
        constexpr rhi::IndexType IndexType = sizeof(ImDrawIdx) == 2 ? rhi::IndexType::Uint16 : rhi::IndexType::Uint32;
        cmdList->SetVertexBuffer(0, m_vbf, buffers.vertexBuffer, 0, buffers.vertexBufferSize * sizeof(ImDrawVert));
        cmdList->SetIndexBuffer(buffers.indexBuffer, 0, buffers.indexBufferSize * sizeof(ImDrawIdx), IndexType);
        cmdList->SetRenderPipeline(m_pipeline);
        cmdList->SetRenderRootSignature(m_rootSignature);
        cmdList->SetRender32BitConstantValues(0, mvp, 16);
        cmdList->SetBlendColor(blendColor);
    }

    bool ImGuiRenderService::CreateDeviceResources()
    {
        rhi::Device* device = m_renderService.GetDevice();

        HE_ASSERT(!m_rootSignature);

        // Create root signature
        {
            rhi::DescriptorRange range{};
            range.type = rhi::DescriptorRangeType::Texture;
            range.baseRegister = 0;
            range.registerSpace = 0;
            range.count = 1;

            rhi::SlotDesc slots[2]{};

            slots[0].type = rhi::SlotType::ConstantValues;
            slots[0].stage = rhi::ShaderStage::Vertex;
            slots[0].constantValues.baseRegister = 0;
            slots[0].constantValues.registerSpace = 0;
            slots[0].constantValues.num32BitValues = 16;

            slots[1].type = rhi::SlotType::DescriptorTable;
            slots[1].stage = rhi::ShaderStage::Pixel;
            slots[1].descriptorTable.rangeCount = 1;
            slots[1].descriptorTable.ranges = &range;

            rhi::StaticSamplerDesc samplerDesc{};
            samplerDesc.minFilter = rhi::Filter::Linear;
            samplerDesc.magFilter = rhi::Filter::Linear;
            samplerDesc.mipFilter = rhi::Filter::Linear;
            samplerDesc.filterReduce = rhi::FilterReduce::Standard;
            samplerDesc.addressU = rhi::AddressMode::Wrap;
            samplerDesc.addressV = rhi::AddressMode::Wrap;
            samplerDesc.addressW = rhi::AddressMode::Wrap;
            samplerDesc.maxAnisotropy = 0;
            samplerDesc.comparisonFunc = rhi::ComparisonFunc::Always;
            samplerDesc.borderColor = rhi::BorderColor::TransparentBlack;
            samplerDesc.minLOD = 0;
            samplerDesc.maxLOD = 0;
            samplerDesc.lodBias = 0;
            samplerDesc.stage = rhi::ShaderStage::Pixel;
            samplerDesc.baseRegister = 0;
            samplerDesc.registerSpace = 0;

            rhi::RootSignatureDesc desc{};
            desc.slotCount = HE_LENGTH_OF(slots);
            desc.slots = slots;
            desc.staticSamplerCount = 1;
            desc.staticSamplers = &samplerDesc;
            desc.inputAssembler = true;
            HE_RHI_SET_NAME(desc, "ImGui Root Signature");

            Result r = device->CreateRootSignature(desc, m_rootSignature);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui root signature. Error: {}", r);
                return false;
            }
        }

        // Create vertex buffer format
        {
            rhi::VertexAttributeDesc attributes[]
            {
                { "POSITION", 0, rhi::Format::RG32Float, IM_OFFSETOF(ImDrawVert, pos) },
                { "TEXCOORD", 0, rhi::Format::RG32Float, IM_OFFSETOF(ImDrawVert, uv) },
                { "COLOR", 0, rhi::Format::RGBA8Unorm, IM_OFFSETOF(ImDrawVert, col) },
            };

            rhi::VertexBufferFormatDesc desc{};
            desc.stride = sizeof(ImDrawVert);
            desc.stepRate = rhi::StepRate::PerVertex;
            desc.attributeCount = HE_LENGTH_OF(attributes);
            desc.attributes = attributes;

            Result r = device->CreateVertexBufferFormat(desc, m_vbf);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui vertex buffer format. Error: {}", r);
                return false;
            }
        }

        rhi::Shader* vs = nullptr;
        {
            rhi::ShaderDesc desc{};
            desc.stage = rhi::ShaderStage::Vertex;
        #if defined(HE_PLATFORM_API_WIN32)
            desc.code = c_imgui_vs_dxbc;
            desc.codeSize = sizeof(c_imgui_vs_dxbc);
        #else
            desc.code = c_imgui_vs_spv;
            desc.codeSize = sizeof(c_imgui_vs_spv);
        #endif
            Result r = device->CreateShader(desc, vs);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui vertex shader. Error: {}", r);
                return false;
            }
        }

        rhi::Shader* ps = nullptr;
        {
            rhi::ShaderDesc desc{};
            desc.stage = rhi::ShaderStage::Pixel;
        #if defined(HE_PLATFORM_API_WIN32)
            desc.code = c_imgui_ps_dxbc;
            desc.codeSize = sizeof(c_imgui_ps_dxbc);
        #else
            desc.code = c_imgui_ps_spv;
            desc.codeSize = sizeof(c_imgui_ps_spv);
        #endif
            Result r = device->CreateShader(desc, ps);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui pixel shader. Error: {}", r);
                return false;
            }
        }

        // Create render pipeline
        {
            rhi::RenderPipelineDesc desc{};
            desc.rootSignature = m_rootSignature;
            desc.vertexShader = vs;
            desc.pixelShader = ps;
            desc.vertexBufferCount = 1;
            desc.vertexBufferFormats = &m_vbf;
            desc.primitiveType = rhi::PrimitiveType::TriList;
            desc.blend.targets[0].enable = true;
            desc.blend.targets[0].srcRgb = rhi::BlendFactor::SrcAlpha;
            desc.blend.targets[0].dstRgb = rhi::BlendFactor::InvSrcAlpha;
            desc.blend.targets[0].opRgb = rhi::BlendOp::Add;
            desc.blend.targets[0].srcAlpha = rhi::BlendFactor::One;
            desc.blend.targets[0].dstAlpha = rhi::BlendFactor::InvSrcAlpha;
            desc.blend.targets[0].opAlpha = rhi::BlendOp::Add;
            desc.raster.cullMode = rhi::CullMode::None;
            desc.raster.frontCounterClockwise = false;
            desc.raster.depthClamp = true;
            desc.depth.testEnable = false;
            desc.depth.writeEnable = true;
            desc.depth.func = rhi::ComparisonFunc::Always;
            desc.stencil.enable = false;
            desc.stencil.frontFace.func = rhi::ComparisonFunc::Always;
            desc.stencil.frontFace.failOp = rhi::StencilOp::Keep;
            desc.stencil.frontFace.depthFailOp = rhi::StencilOp::Keep;
            desc.stencil.frontFace.passOp = rhi::StencilOp::Keep;
            desc.stencil.backFace = desc.stencil.frontFace;
            desc.targets.renderTargetCount = 1;
            desc.targets.renderTargetFormats[0] = m_swapChainFormat.format;
            HE_RHI_SET_NAME(desc, "ImGui Render Pipeline");

            Result r = device->CreateRenderPipeline(desc, m_pipeline);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui render pipeline. Error: {}", r);
                return false;
            }
        }

        // No longer need these shader objects
        device->SafeDestroy(vs);
        device->SafeDestroy(ps);

        return true;
    }

    void ImGuiRenderService::DestroyDeviceResources()
    {
        rhi::Device* device = m_renderService.GetDevice();

        device->SafeDestroy(m_pipeline);
        device->SafeDestroy(m_pipeline);
        device->SafeDestroy(m_vbf);
        device->SafeDestroy(m_rootSignature);
    }

    bool ImGuiRenderService::CreateFontResources(ImFontAtlas& atlas)
    {
        // Build texture atlas of font
        HE_ASSERT(atlas.TexID == nullptr);

        uint8_t* fontPixels;
        int fontPixelsWidth = 0;
        int fontPixelsHeight = 0;
        atlas.GetTexDataAsRGBA32(&fontPixels, &fontPixelsWidth, &fontPixelsHeight);

        const Vec3u fontTextureSize{ static_cast<uint32_t>(fontPixelsWidth), static_cast<uint32_t>(fontPixelsHeight), 1 };

        rhi::Device* device = m_renderService.GetDevice();
        FontResources& resources = m_fontResources.EmplaceBack();

        // Create font texture
        {
            rhi::TextureDesc desc{};
            desc.type = rhi::TextureType::_2D;
            desc.format = rhi::Format::RGBA8Unorm;
            desc.size = fontTextureSize;
            desc.initialState = rhi::TextureState::CopyDst;
            HE_RHI_SET_NAME(desc, "ImGui Font Texture");

            Result r = device->CreateTexture(desc, resources.texture);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui font texture. Error: {}", r);
                return false;
            }
        }

        // Upload pixels to the texture
        {
            uint32_t alignment = device->GetInfo().uploadDataPitchAlignment;
            uint32_t uploadPitch = AlignUp<uint32_t>(fontPixelsWidth * 4, alignment);
            uint32_t uploadSize = fontPixelsHeight * uploadPitch;

            // Create the upload buffer
            rhi::BufferDesc desc{};
            desc.heapType = rhi::HeapType::Upload;
            desc.size = uploadSize;
            HE_RHI_SET_NAME(desc, "ImGui Font Upload Buffer");

            rhi::Buffer* uploadBuffer = nullptr;
            HE_AT_SCOPE_EXIT([&]() { device->SafeDestroy(uploadBuffer); });
            Result r = device->CreateBuffer(desc, uploadBuffer);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui font upload buffer. Error: {}", r);
                return false;
            }

            // Copy pixels into the upload buffer
            uint8_t* uploadMem = static_cast<uint8_t*>(device->Map(uploadBuffer));
            for (int32_t i = 0; i < fontPixelsHeight; ++i)
            {
                MemCopy(uploadMem + (i * uploadPitch), fontPixels + (i * fontPixelsWidth * 4), fontPixelsWidth * 4);
            }
            device->Unmap(uploadBuffer);

            // Create resources to perform a copy from the upload buffer to the texture
            rhi::CmdAllocatorDesc copyCmdAllocatorDesc{};
            copyCmdAllocatorDesc.type = rhi::CmdListType::Render;
            HE_RHI_SET_NAME(copyCmdAllocatorDesc, "ImGui Font Upload Cmd Pool");

            rhi::CmdAllocator* copyCmdAllocator = nullptr;
            HE_AT_SCOPE_EXIT([&]() { device->SafeDestroy(copyCmdAllocator); });
            r = device->CreateCmdAllocator(copyCmdAllocatorDesc, copyCmdAllocator);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui font upload cmd pool. Error: {}", r);
                return false;
            }

            rhi::CmdListDesc copyCmdListDesc{};
            copyCmdListDesc.alloc = copyCmdAllocator;
            HE_RHI_SET_NAME(copyCmdListDesc, "ImGui Font Upload Cmd List");

            rhi::RenderCmdList* copyCmdList = nullptr;
            HE_AT_SCOPE_EXIT([&]() { device->SafeDestroy(copyCmdList); });
            r = device->CreateRenderCmdList(copyCmdListDesc, copyCmdList);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui font upload cmd list. Error: {}", r);
                return false;
            }

            // Copy the upload buffer to the texture
            rhi::BufferTextureCopy copyRegion{};
            copyRegion.bufferRowPitch = uploadPitch;
            copyRegion.textureSize = fontTextureSize;

            copyCmdList->Begin(copyCmdAllocator);
            copyCmdList->Copy(uploadBuffer, resources.texture, copyRegion);
            copyCmdList->TransitionBarrier(resources.texture, rhi::TextureState::CopyDst, rhi::TextureState::PixelShaderRead);
            copyCmdList->End();

            device->GetRenderCmdQueue().Submit(copyCmdList);
            device->GetRenderCmdQueue().WaitForFlush();
        }

        // Create font texture view
        {
            rhi::TextureViewDesc desc{};
            desc.texture = resources.texture;

            Result r = device->CreateTextureView(desc, resources.view);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui font texture view. Error: {}", r);
                return false;
            }
        }

        // Create font texture descriptor table
        {
            rhi::DescriptorRange range{};
            range.type = rhi::DescriptorRangeType::Texture;
            range.baseRegister = 0;
            range.registerSpace = 0;
            range.count = 1;

            rhi::DescriptorTableDesc desc{};
            desc.rangeCount = 1;
            desc.ranges = &range;

            Result r = device->CreateDescriptorTable(desc, resources.table);
            if (!r)
            {
                HE_LOGF_ERROR(imgui_render, "Failed to create ImGui font texture view. Error: {}", r);
                return false;
            }
        }

        // Set the view into the descriptor table
        device->SetTextureViews(resources.table, 0, 0, 1, &resources.view);

        // Store the font texture view
        atlas.SetTexID(resources.table);

        return true;
    }

    void ImGuiRenderService::DestroyFontResources(FontResources& resources)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.Fonts->TexID == resources.table)
        {
            io.Fonts->SetTexID(nullptr);
        }

        rhi::Device* device = m_renderService.GetDevice();
        device->SafeDestroy(resources.table);
        device->SafeDestroy(resources.view);
        device->SafeDestroy(resources.texture);
    }

    void ImGuiRenderService::DestroyAllFontResources()
    {
        for (FontResources& resources : m_fontResources)
        {
            DestroyFontResources(resources);
        }
        m_fontResources.Clear();
    }
}
