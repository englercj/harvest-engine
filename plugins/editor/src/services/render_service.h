// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/rhi/types.h"
#include "he/window/view.h"

namespace he::window { class View; }

namespace he::editor
{
    class RenderService
    {
    public:
        RenderService(Allocator& allocator);

        bool Initialize(window::View* view);
        void Terminate();

        void BeginFrame();
        void EndFrame();

        void SetSize(Vec2i size);

        rhi::SwapChainFormat GetSwapChainFormat() { return m_preferredSwapChainFormat; }

        rhi::Device* GetDevice() { return m_device; }
        rhi::SwapChain* GetSwapChain() { return m_swapChain; }
        rhi::RenderCmdList* GetCmdList() { return m_cmdList; }

        rhi::PresentTarget GetPresentTarget() { return m_presentTarget; }

    private:
        rhi::SwapChainFormat FindPreferredSwapChainFormat();

    private:
        struct FrameData
        {
            rhi::CmdAllocator* cmdAlloc{ nullptr };
            rhi::CpuFence* fence{ nullptr };
        };

    private:
        Allocator& m_allocator;

        rhi::Instance* m_rhi{ nullptr };
        rhi::Device* m_device{ nullptr };
        rhi::SwapChain* m_swapChain{ nullptr };
        rhi::RenderCmdList* m_cmdList{ nullptr };

        rhi::PresentTarget m_presentTarget{};
        rhi::SwapChainFormat m_preferredSwapChainFormat{};

        window::View* m_view{ nullptr };

        uint32_t m_frameIndex{ 0 };
        FrameData m_frameDatas[rhi::MaxFrameCount]{};
    };
}
