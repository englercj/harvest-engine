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

    void CopyCmdQueueImpl::Signal(CpuFence* fence)
    {
        HE_UNUSED(fence);
    }

    void CopyCmdQueueImpl::Signal(GpuFence* fence, uint64_t value)
    {
        HE_UNUSED(fence, value);
    }

    void CopyCmdQueueImpl::Wait(GpuFence* fence, uint64_t value)
    {
        HE_UNUSED(fence, value);
    }

    void CopyCmdQueueImpl::WaitForFlush()
    {

    }

    void CopyCmdQueueImpl::Submit(CopyCmdList* cmdList)
    {
        HE_UNUSED(cmdList);
    }

    // --------------------------------------------------------------------------------------------
    // Compute Queue

    void ComputeCmdQueueImpl::Signal(CpuFence* fence)
    {
        HE_UNUSED(fence);
    }

    void ComputeCmdQueueImpl::Signal(GpuFence* fence, uint64_t value)
    {
        HE_UNUSED(fence, value);
    }

    void ComputeCmdQueueImpl::Wait(GpuFence* fence, uint64_t value)
    {
        HE_UNUSED(fence, value);
    }

    void ComputeCmdQueueImpl::WaitForFlush()
    {

    }

    void ComputeCmdQueueImpl::Submit(ComputeCmdList* cmdList)
    {
        HE_UNUSED(cmdList);
    }

    // --------------------------------------------------------------------------------------------
    // Render Queue

    void RenderCmdQueueImpl::Signal(CpuFence* fence)
    {
        HE_UNUSED(fence);
    }

    void RenderCmdQueueImpl::Signal(GpuFence* fence, uint64_t value)
    {
        HE_UNUSED(fence, value);
    }

    void RenderCmdQueueImpl::Wait(GpuFence* fence, uint64_t value)
    {
        HE_UNUSED(fence, value);
    }

    void RenderCmdQueueImpl::WaitForFlush()
    {

    }

    void RenderCmdQueueImpl::Submit(RenderCmdList* cmdList)
    {
        HE_UNUSED(cmdList);
    }

    Result RenderCmdQueueImpl::Present(SwapChain* swapChain)
    {
        HE_UNUSED(swapChain);
        return Result::Success;
    }
}

#endif
