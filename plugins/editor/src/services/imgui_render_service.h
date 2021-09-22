// Copyright Chad Engler

#pragma once

#include "render_service.h"

#include "he/core/allocator.h"
#include "he/core/vector.h"
#include "he/rhi/types.h"

#include "imgui.h"

namespace he::editor
{
    class ImGuiRenderService
    {
    public:
        ImGuiRenderService(Allocator& allocator, RenderService& renderService);

        bool Initialize(rhi::SwapChainFormat swapChainFormat);
        void Terminate();

        void NewFrame();
        void Render();

        bool SetupFontAtlas(ImFontAtlas& atlas);

    private:
        struct FontResources
        {
            rhi::Texture* texture{ nullptr };
            rhi::TextureView* view{ nullptr };
            rhi::DescriptorTable* table{ nullptr };
        };

        struct FrameBuffers
        {
            rhi::Buffer* indexBuffer{ nullptr };
            rhi::Buffer* vertexBuffer{ nullptr };
            int32_t indexBufferSize{ 0 };
            int32_t vertexBufferSize{ 0 };
        };

        struct FrameData
        {
            rhi::CmdAllocator* cmdAlloc{ nullptr };
            rhi::CpuFence* fence{ nullptr };
            FrameBuffers buffers{};
        };

        struct ViewportData
        {
            rhi::RenderCmdList* cmdList{ nullptr };
            rhi::SwapChain* swapChain{ nullptr };

            uint32_t frameIndex{ 0 };
            FrameData frameDatas[rhi::MaxFrameCount]{};
        };

    private:
        static void CreatePlatformWindow(ImGuiViewport* viewport);
        static void DestroyPlatformWindow(ImGuiViewport* viewport);
        static void SetWindowSize(ImGuiViewport* viewport, ImVec2 size);
        static void RenderWindow(ImGuiViewport* viewport, void*);
        static void SwapBuffers(ImGuiViewport* viewport, void*);

        void RenderDrawData(ImDrawData* drawData, rhi::RenderCmdList* cmdList, FrameBuffers& buffers);
        void SetupRenderState(ImDrawData* drawData, rhi::RenderCmdList* cmdList, FrameBuffers& buffers);

        bool CreateDeviceResources();
        void DestroyDeviceResources();

        bool CreateFontResources(ImFontAtlas& atlas);
        void DestroyFontResources(FontResources& resources);
        void DestroyAllFontResources();

    private:
        Allocator& m_allocator;
        RenderService& m_renderService;

        rhi::SwapChainFormat m_swapChainFormat{};

        rhi::RootSignature* m_rootSignature{ nullptr };
        rhi::VertexBufferFormat* m_vbf{ nullptr };
        rhi::RenderPipeline* m_pipeline{ nullptr };

        Vector<FontResources> m_fontResources;

        // main viewport rendering
        uint32_t m_frameIndex{ 0 };
        FrameBuffers m_frameBuffers[rhi::MaxFrameCount]{};
    };
}
