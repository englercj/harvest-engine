// Copyright Chad Engler

#include "d3d12_cmd_queue.h"

#include "d3d12_cmd_list.h"
#include "d3d12_common.h"
#include "d3d12_device.h"
#include "d3d12_instance.h"
#include "d3d12_resources.h"

#include "he/core/log.h"

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    // --------------------------------------------------------------------------------------------
    // Base Queue Implementation

    BaseCmdQueueImpl::~BaseCmdQueueImpl()
    {
        if (m_d3dFence)
        {
            WaitForFlushInternal();
        }

        HE_DX_SAFE_RELEASE(m_d3dFence);

        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }

        HE_DX_SAFE_RELEASE(m_d3dCmdQueue);
    }

    void BaseCmdQueueImpl::SignalInternal(CpuFence* fence_)
    {
        CpuFenceImpl* fence = static_cast<CpuFenceImpl*>(fence_);
        uint64_t fenceValue = fence->value++;
        fence->d3dFence->SetEventOnCompletion(fenceValue, fence->event);
        m_d3dCmdQueue->Signal(fence->d3dFence, fenceValue);
    }

    void BaseCmdQueueImpl::SignalInternal(GpuFence* fence_, uint64_t value)
    {
        GpuFenceImpl* fence = static_cast<GpuFenceImpl*>(fence_);
        m_d3dCmdQueue->Signal(fence->d3dFence, value);
    }

    void BaseCmdQueueImpl::WaitInternal(GpuFence* fence_, uint64_t value)
    {
        GpuFenceImpl* fence = static_cast<GpuFenceImpl*>(fence_);
        m_d3dCmdQueue->Wait(fence->d3dFence, value);
    }

    Result BaseCmdQueueImpl::WaitForFlushInternal()
    {
        uint64_t lastSignaledFenceValue = ++m_nextFenceValue;

        m_d3dCmdQueue->Signal(m_d3dFence, lastSignaledFenceValue);

        // If the device was removed we'll get an invalid value
        uint64_t completedFenceValue = m_d3dFence->GetCompletedValue();
        HE_ASSERT(completedFenceValue != UINT64_MAX);

        if (completedFenceValue < lastSignaledFenceValue)
        {
            m_d3dFence->SetEventOnCompletion(lastSignaledFenceValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        return Result::Success;
    }

    Result BaseCmdQueueImpl::InitializeInternal(DeviceImpl* device, D3D12_COMMAND_LIST_TYPE type)
    {
        m_device = device;

        ID3D12Device* d3dDevice = m_device->m_d3dDevice;

        D3D12_COMMAND_QUEUE_DESC d3dDesc{};
        d3dDesc.Type = type;
        d3dDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        d3dDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        HRESULT hr = d3dDevice->CreateCommandQueue(&d3dDesc, IID_PPV_ARGS(&m_d3dCmdQueue));
        if (FAILED(hr))
        {
            HE_LOGF_ERROR(rhi, "Failed to create command queue.");
            return m_device->MakeResult(hr);
        }

        hr = d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3dFence));
        if (FAILED(hr))
        {
            HE_LOGF_ERROR(rhi, "Failed to create fence for command queue.");
            return m_device->MakeResult(hr);
        }

        m_fenceEvent = CreateEventW(nullptr, false, false, nullptr);
        if (!m_fenceEvent)
        {
            HE_LOGF_ERROR(rhi, "Failed to create fence event for command queue.");
            return Result::FromLastError();
        }

        m_d3dFence->Signal(m_nextFenceValue);
        return Result::Success;
    }

    void BaseCmdQueueImpl::SubmitInternal(ID3D12GraphicsCommandList* d3dCmdList)
    {
        uint64_t value = ++m_nextFenceValue;

        ID3D12CommandList* d3dCmdLists[]{ d3dCmdList };
        //m_d3dCmdQueue->Wait(m_d3dFence, value - 1);
        m_d3dCmdQueue->ExecuteCommandLists(1, d3dCmdLists);
        m_d3dCmdQueue->Signal(m_d3dFence, value);
    }

    // --------------------------------------------------------------------------------------------
    // Copy Queue

    Result CopyCmdQueueImpl::Initialize(DeviceImpl* device)
    {
        Result r = InitializeInternal(device, D3D12_COMMAND_LIST_TYPE_COPY);
        HE_DX_SET_NAME(m_d3dCmdQueue, "Copy Cmd Queue");
        return r;
    }

    void CopyCmdQueueImpl::Signal(CpuFence* fence)
    {
        BaseCmdQueueImpl::SignalInternal(fence);
    }

    void CopyCmdQueueImpl::Signal(GpuFence* fence, uint64_t value)
    {
        BaseCmdQueueImpl::SignalInternal(fence, value);
    }

    void CopyCmdQueueImpl::Wait(GpuFence* fence, uint64_t value)
    {
        BaseCmdQueueImpl::WaitInternal(fence, value);
    }

    void CopyCmdQueueImpl::WaitForFlush()
    {
        BaseCmdQueueImpl::WaitForFlushInternal();
    }

    void CopyCmdQueueImpl::Submit(CopyCmdList* cmdList_)
    {
        CopyCmdListImpl* cmdList = static_cast<CopyCmdListImpl*>(cmdList_);
        SubmitInternal(cmdList->m_d3dCmdList);
    }

    // --------------------------------------------------------------------------------------------
    // Compute Queue

    Result ComputeCmdQueueImpl::Initialize(DeviceImpl* device)
    {
        Result r = InitializeInternal(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
        HE_DX_SET_NAME(m_d3dCmdQueue, "Compute Cmd Queue");
        return r;
    }

    void ComputeCmdQueueImpl::Signal(CpuFence* fence)
    {
        BaseCmdQueueImpl::SignalInternal(fence);
    }

    void ComputeCmdQueueImpl::Signal(GpuFence* fence, uint64_t value)
    {
        BaseCmdQueueImpl::SignalInternal(fence, value);
    }

    void ComputeCmdQueueImpl::Wait(GpuFence* fence, uint64_t value)
    {
        BaseCmdQueueImpl::WaitInternal(fence, value);
    }

    void ComputeCmdQueueImpl::WaitForFlush()
    {
        BaseCmdQueueImpl::WaitForFlushInternal();
    }

    void ComputeCmdQueueImpl::Submit(ComputeCmdList* cmdList_)
    {
        ComputeCmdListImpl* cmdList = static_cast<ComputeCmdListImpl*>(cmdList_);
        SubmitInternal(cmdList->m_d3dCmdList);
    }

    // --------------------------------------------------------------------------------------------
    // Render Queue

    Result RenderCmdQueueImpl::Initialize(DeviceImpl* device)
    {
        Result r = InitializeInternal(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
        HE_DX_SET_NAME(m_d3dCmdQueue, "Graphics Cmd Queue");
        return r;
    }

    void RenderCmdQueueImpl::Signal(CpuFence* fence)
    {
        BaseCmdQueueImpl::SignalInternal(fence);
    }

    void RenderCmdQueueImpl::Signal(GpuFence* fence, uint64_t value)
    {
        BaseCmdQueueImpl::SignalInternal(fence, value);
    }

    void RenderCmdQueueImpl::Wait(GpuFence* fence, uint64_t value)
    {
        BaseCmdQueueImpl::WaitInternal(fence, value);
    }

    void RenderCmdQueueImpl::WaitForFlush()
    {
        BaseCmdQueueImpl::WaitForFlushInternal();
    }

    void RenderCmdQueueImpl::Submit(RenderCmdList* cmdList_)
    {
        RenderCmdListImpl* cmdList = static_cast<RenderCmdListImpl*>(cmdList_);
        SubmitInternal(cmdList->m_d3dCmdList);
    }

    Result RenderCmdQueueImpl::Present(SwapChain* swapChain_)
    {
        SwapChainImpl* swapChain = static_cast<SwapChainImpl*>(swapChain_);

        // Present
        uint32_t presentFlags = 0;
        if (swapChain->syncInterval == 0)
            presentFlags |= m_device->m_instance->m_allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

        HRESULT hr = swapChain->dxgiSwapChain3->Present(swapChain->syncInterval, presentFlags);

        // Update render target
        swapChain->bufferIndex = swapChain->dxgiSwapChain3->GetCurrentBackBufferIndex();

        TextureImpl* texture = swapChain->texture;
        RenderTargetViewImpl* rtv = swapChain->renderTargetView;

        texture->d3dResource = swapChain->d3dResources[swapChain->bufferIndex];
        rtv->d3dCpuHandle = swapChain->d3dRtvCpuHandles[swapChain->bufferIndex];

        return m_device->MakeResult(hr);
    }
}

#endif
