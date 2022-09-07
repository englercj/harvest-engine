// Copyright Chad Engler

#pragma once

#include "d3d12_common.h"

#include "he/rhi/cmd_queue.h"
#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    class DeviceImpl;

    class BaseCmdQueueImpl
    {
    public:
        ~BaseCmdQueueImpl() noexcept;

    protected:
        void SignalInternal(CpuFence* fence);
        void SignalInternal(GpuFence* fence, uint64_t value);
        void WaitInternal(GpuFence* fence, uint64_t value);

        Result WaitForFlushInternal();

        Result InitializeInternal(DeviceImpl* device, D3D12_COMMAND_LIST_TYPE type);

        void SubmitInternal(ID3D12GraphicsCommandList* d3dCmdList);

    public:
        DeviceImpl* m_device{ nullptr };
        ID3D12CommandQueue* m_d3dCmdQueue{ nullptr };

        // Value that will be signaled by the command queue next
        uint64_t m_nextFenceValue{ 0 };

        // Fence is signaled right after a command list has been submitted for execution.
        // All command lists with fence value less than or equal to the signaled value are
        // garuanteed to be finished by the GPU.
        ID3D12Fence* m_d3dFence{ nullptr };

        // Event handle for sleeping the CPU when waiting for the GPU
        HANDLE m_fenceEvent{ nullptr };
    };

    class CopyCmdQueueImpl final : public CopyCmdQueue, public BaseCmdQueueImpl
    {
    public:
        Result Initialize(DeviceImpl* device);

        void Signal(CpuFence* fence) override;
        void Signal(GpuFence* fence, uint64_t value) override;
        void Wait(GpuFence* fence, uint64_t value) override;

        void WaitForFlush() override;

        void Submit(CopyCmdList* cmdList) override;
    };

    class ComputeCmdQueueImpl final : public ComputeCmdQueue, public BaseCmdQueueImpl
    {
    public:
        Result Initialize(DeviceImpl* device);

        void Signal(CpuFence* fence) override;
        void Signal(GpuFence* fence, uint64_t value) override;
        void Wait(GpuFence* fence, uint64_t value) override;

        void WaitForFlush() override;

        void Submit(ComputeCmdList* cmdList) override;
    };

    class RenderCmdQueueImpl final : public RenderCmdQueue, public BaseCmdQueueImpl
    {
    public:
        Result Initialize(DeviceImpl* device);

        void Signal(CpuFence* fence) override;
        void Signal(GpuFence* fence, uint64_t value) override;
        void Wait(GpuFence* fence, uint64_t value) override;

        void WaitForFlush() override;

        void Submit(RenderCmdList* cmdList) override;
        Result Present(SwapChain* swapChain) override;
    };
}

#endif
