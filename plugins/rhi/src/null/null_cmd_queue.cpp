// Copyright Chad Engler

#include "null_cmd_queue.h"

#include "null_device.h"
#include "null_cmd_list.h"
#include "null_instance.h"
#include "null_resources.h"

#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    // --------------------------------------------------------------------------------------------
    // Copy Queue

    void CopyCmdQueueImpl::Signal(CpuFence* fence_)
    {
        static_cast<CpuFenceImpl*>(fence_)->signaled = true;
    }

    void CopyCmdQueueImpl::Signal(GpuFence* fence_, uint64_t value)
    {
        static_cast<GpuFenceImpl*>(fence_)->value = value;
    }

    void CopyCmdQueueImpl::Wait([[maybe_unused]] GpuFence* fence, [[maybe_unused]] uint64_t value)
    {
    }

    void CopyCmdQueueImpl::WaitForFlush()
    {
    }

    void CopyCmdQueueImpl::Submit([[maybe_unused]] CopyCmdList* cmdList)
    {
    }

    // --------------------------------------------------------------------------------------------
    // Compute Queue

    void ComputeCmdQueueImpl::Signal(CpuFence* fence_)
    {
        static_cast<CpuFenceImpl*>(fence_)->signaled = true;
    }

    void ComputeCmdQueueImpl::Signal(GpuFence* fence_, uint64_t value)
    {
        static_cast<GpuFenceImpl*>(fence_)->value = value;
    }

    void ComputeCmdQueueImpl::Wait([[maybe_unused]] GpuFence* fence, [[maybe_unused]] uint64_t value)
    {
    }

    void ComputeCmdQueueImpl::WaitForFlush()
    {
    }

    void ComputeCmdQueueImpl::Submit([[maybe_unused]] ComputeCmdList* cmdList)
    {
    }

    // --------------------------------------------------------------------------------------------
    // Render Queue

    void RenderCmdQueueImpl::Signal(CpuFence* fence_)
    {
        static_cast<CpuFenceImpl*>(fence_)->signaled = true;
    }

    void RenderCmdQueueImpl::Signal(GpuFence* fence_, uint64_t value)
    {
        static_cast<GpuFenceImpl*>(fence_)->value = value;
    }

    void RenderCmdQueueImpl::Wait([[maybe_unused]] GpuFence* fence, [[maybe_unused]] uint64_t value)
    {
    }

    void RenderCmdQueueImpl::WaitForFlush()
    {
    }

    void RenderCmdQueueImpl::Submit([[maybe_unused]] RenderCmdList* cmdList)
    {
    }

    Result RenderCmdQueueImpl::Present([[maybe_unused]] SwapChain* swapChain)
    {
        return Result::Success;
    }
}

#endif
