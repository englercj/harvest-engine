// Copyright Chad Engler

#include "render_service.h"

#include "he/core/alloca.h"
#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/vector.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/device.h"
#include "he/rhi/instance.h"

namespace he::editor
{
    bool RenderService::Initialize(window::View* view)
    {
        m_view = view;

        // Create instance
        {
            rhi::InstanceDesc desc{};
            desc.enableDebugCpu = true;
            desc.enableDebugGpu = true;
            desc.enableDebugBreakOnError = true;
            desc.enableDebugBreakOnWarning = true;

            Result r = rhi::Instance::Create(desc, m_rhi);
            if (!r)
            {
                HE_LOGF_ERROR(render_service, "Failed to create RHI instance. Error: {}", r);
                return false;
            }
        }

        // Create device
        {
            rhi::DeviceDesc desc{};
            HE_RHI_SET_NAME(desc, "Harvest Device");

            Result r = m_rhi->CreateDevice(desc, m_device);
            if (!r)
            {
                HE_LOGF_ERROR(render_service, "Failed to create RHI device. Error: {}", r);
                return false;
            }
        }

        // Create frame data
        rhi::RenderCmdQueue& cmdQueue = m_device->GetRenderCmdQueue();
        for (uint32_t i = 0; i < HE_LENGTH_OF(m_frameDatas); ++i)
        {
            FrameData& frame = m_frameDatas[i];

            {
                rhi::CmdAllocatorDesc desc{};
                desc.type = rhi::CmdListType::Render;
                HE_RHI_SET_NAME(desc, "Frame Command Pool");
                Result r = m_device->CreateCmdAllocator(desc, frame.cmdAlloc);
                if (!r)
                {
                    HE_LOGF_ERROR(render_service, "Failed to create RHI frame command allocator. Error: {}", r);
                    return false;
                }
            }

            {
                rhi::CpuFenceDesc desc{};
                Result r = m_device->CreateCpuFence(desc, frame.fence);
                if (!r)
                {
                    HE_LOGF_ERROR(render_service, "Failed to create RHI frame fence. Error: {}", r);
                    return false;
                }
            }

            cmdQueue.Signal(frame.fence);
        }

        // Create command list
        {
            rhi::CmdListDesc desc{};
            desc.alloc = m_frameDatas[0].cmdAlloc;
            HE_RHI_SET_NAME(desc, "Frame Command List");
            Result r = m_device->CreateRenderCmdList(desc, m_cmdList);
            if (!r)
            {
                HE_LOGF_ERROR(render_service, "Failed to create RHI command list. Error: {}", r);
                return false;
            }
        }

        // Detect preferred swap chain format
        m_preferredSwapChainFormat = FindPreferredSwapChainFormat();

        // Create swap chain
        {
            rhi::SwapChainDesc desc{};
            desc.nativeViewHandle = view->GetNativeHandle();
            desc.bufferCount = HE_LENGTH_OF(m_frameDatas);
            desc.format = m_preferredSwapChainFormat;
            desc.enableVSync = true;

            Result r = m_device->CreateSwapChain(desc, m_swapChain);
            if (!r)
            {
                HE_LOGF_ERROR(render_service, "Failed to create RHI swap chain. Error: {}", r);
                return false;
            }
        }

        return true;
    }

    void RenderService::Terminate()
    {
        m_device->GetRenderCmdQueue().WaitForFlush();

        m_device->SafeDestroy(m_swapChain);
        m_device->SafeDestroy(m_cmdList);

        for (uint32_t i = 0; i < HE_LENGTH_OF(m_frameDatas); ++i)
        {
            FrameData& frame = m_frameDatas[i];
            m_device->SafeDestroy(frame.fence);
            m_device->SafeDestroy(frame.cmdAlloc);
        }

        m_rhi->DestroyDevice(m_device);
        m_device = nullptr;

        rhi::Instance::Destroy(m_rhi);
    }

    void RenderService::BeginFrame()
    {
        if (!m_swapChain)
            return;

        m_presentTarget = m_device->AcquirePresentTarget(m_swapChain);

        m_frameIndex = (m_frameIndex + 1) % HE_LENGTH_OF(m_frameDatas);
        FrameData& frame = m_frameDatas[m_frameIndex];

        m_device->WaitForFence(frame.fence);
        m_device->ResetCmdAllocator(frame.cmdAlloc);

        m_cmdList->Begin(frame.cmdAlloc);
    }

    void RenderService::EndFrame()
    {
        m_cmdList->End();

        rhi::RenderCmdQueue& cmdQueue = m_device->GetRenderCmdQueue();
        cmdQueue.Submit(m_cmdList);
        cmdQueue.Signal(m_frameDatas[m_frameIndex].fence);
        cmdQueue.Present(m_swapChain);
    }

    void RenderService::SetSize(Vec2i size)
    {
        if (!m_swapChain)
            return;

        rhi::SwapChainDesc desc{};
        desc.bufferCount = HE_LENGTH_OF(m_frameDatas);
        desc.format = m_preferredSwapChainFormat;
        desc.size = size;

        Result r = m_device->UpdateSwapChain(m_swapChain, desc);
        HE_ASSERT(r, HE_KV(result, r));
    }

    rhi::SwapChainFormat RenderService::FindPreferredSwapChainFormat()
    {
        uint32_t count = 0;
        Result r = m_device->GetSwapChainFormats(m_view->GetNativeHandle(), nullptr, count);
        HE_ASSERT(r, HE_KV(result, r));
        HE_ASSERT(count > 0);

        Vector<rhi::SwapChainFormat> formats;
        formats.Resize(count);

        r = m_device->GetSwapChainFormats(m_view->GetNativeHandle(), formats.Data(), count);
        HE_ASSERT(r, HE_KV(result, r));

        for (uint32_t i = 0; i < count; ++i)
        {
            rhi::SwapChainFormat& format = formats[i];

            if (format.colorSpace == rhi::ColorSpace::sRGB)
            {
                return format;
            }
        }

        return formats[0];
    }
}
