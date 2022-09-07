// Copyright Chad Engler

#pragma once

#include "he/rhi/cmd_queue.h"
#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    class CopyCmdQueueImpl final : public CopyCmdQueue
    {
    public:
        void Signal(CpuFence* fence) override;
        void Signal(GpuFence* fence, uint64_t value) override;
        void Wait(GpuFence* fence, uint64_t value) override;

        void WaitForFlush() override;

        void Submit(CopyCmdList* cmdList) override;
    };

    class ComputeCmdQueueImpl final : public ComputeCmdQueue
    {
    public:
        void Signal(CpuFence* fence) override;
        void Signal(GpuFence* fence, uint64_t value) override;
        void Wait(GpuFence* fence, uint64_t value) override;

        void WaitForFlush() override;

        void Submit(ComputeCmdList* cmdList) override;
    };

    class RenderCmdQueueImpl final : public RenderCmdQueue
    {
    public:
        void Signal(CpuFence* fence) override;
        void Signal(GpuFence* fence, uint64_t value) override;
        void Wait(GpuFence* fence, uint64_t value) override;

        void WaitForFlush() override;

        void Submit(RenderCmdList* cmdList) override;
        Result Present(SwapChain* swapChain) override;
    };
}

#endif
