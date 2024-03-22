// Copyright Chad Engler

#include "null_cmd_queue.h"

#include "null_device.h"
#include "null_cmd_list.h"
#include "null_instance.h"

#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    // --------------------------------------------------------------------------------------------
    // Copy Queue

    void CopyCmdQueueImpl::Signal([[maybe_unused]] CpuFence* fence)
    {
    }

    void CopyCmdQueueImpl::Signal([[maybe_unused]] GpuFence* fence, [[maybe_unused]] uint64_t value)
    {
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

    void ComputeCmdQueueImpl::Signal([[maybe_unused]] CpuFence* fence)
    {
    }

    void ComputeCmdQueueImpl::Signal([[maybe_unused]] GpuFence* fence, [[maybe_unused]] uint64_t value)
    {
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

    void RenderCmdQueueImpl::Signal([[maybe_unused]] CpuFence* fence)
    {
    }

    void RenderCmdQueueImpl::Signal([[maybe_unused]] GpuFence* fence, [[maybe_unused]] uint64_t value)
    {
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
