// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/rhi/types.h"

namespace he::rhi
{
    /// Base command queue interface shared by all command queue types.
    class CmdQueue
    {
    public:
        virtual ~CmdQueue() = default;

        /// Adds a GPU-side signal to the queue that will set the fence.
        ///
        /// \param[in] fence The fence to be signaled.
        virtual void Signal(CpuFence* fence) = 0;

        /// Adds a GPU-side signal to the queue that will set the fence to a particular value.
        ///
        /// \param[in] fence The fence to be signaled.
        /// \param[in] value The value to set the fence to.
        virtual void Signal(GpuFence* fence, uint64_t value) = 0;

        /// Adds a GPU-side wait until the fence reaches or exceeds the specified value.
        ///
        /// \param[in] fence The fence to wait on.
        /// \param[in] value The value to wait for the fence to reach, or exceed.
        virtual void Wait(GpuFence* fence, uint64_t value) = 0;

        /// Puts the current thread into a wait state. Blocks until the queue has flushed
        /// all commands on the GPU.
        virtual void WaitForFlush() = 0;

        /// Gets the frequency of timestamp values written by this queue in ticks per second.
        virtual uint64_t GetTimestampFrequency() const = 0;
    };

    /// Command queue for submitting copy command lists.
    class CopyCmdQueue : public CmdQueue
    {
    public:
        /// Submit the list of commands for execution in the queue. The command list can be reset
        /// and reused immediately after calling this function, however the CmdAllocator that was
        /// used must not be reset until all the commands complete execution on the GPU.
        ///
        /// \param[in] cmdList The list of commands to execute.
        virtual void Submit(CopyCmdList* cmdList) = 0;
    };

    /// Command queue for submitting compute command lists.
    class ComputeCmdQueue : public CmdQueue
    {
    public:
        /// Submit the list of commands for execution in the queue. The command list can be reset
        /// and reused immediately after calling this function, however the CmdAllocator that was
        /// used must not be reset until all the commands complete execution on the GPU.
        ///
        /// \param[in] cmdList The list of commands to execute.
        virtual void Submit(ComputeCmdList* cmdList) = 0;
    };

    /// Command queue for submitting render command lists.
    class RenderCmdQueue : public CmdQueue
    {
    public:
        /// Submit the list of commands for execution in the queue. The command list can be reset
        /// and reused immediately after calling this function, however the CmdAllocator that was
        /// used must not be reset until all the commands complete execution on the GPU.
        ///
        /// \param[in] cmdList The list of commands to execute.
        virtual void Submit(RenderCmdList* cmdList) = 0;

        /// Presents the swap chain to the display.
        ///
        /// \param[in] swapChain The swap chain to present.
        virtual Result Present(SwapChain* swapChain) = 0;
    };
}
