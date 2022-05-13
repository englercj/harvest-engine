// Copyright Chad Engler

#include "d3d12_resources.h"

#include "he/core/assert.h"

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    DescriptorPool::~DescriptorPool()
    {
        HE_ASSERT(m_heap == nullptr);
    }

    HRESULT DescriptorPool::Create(Allocator& allocator, ID3D12Device* d3dDevice, const D3D12_DESCRIPTOR_HEAP_DESC& d3dHeapDesc)
    {
        HRESULT hr = d3dDevice->CreateDescriptorHeap(&d3dHeapDesc, IID_PPV_ARGS(&m_heap));
        if (FAILED(hr))
            return hr;

        m_heapType = d3dHeapDesc.Type;
        m_stride = d3dDevice->GetDescriptorHandleIncrementSize(d3dHeapDesc.Type);
        m_cpuStart = m_heap->GetCPUDescriptorHandleForHeapStart();
        if (HasFlag(d3dHeapDesc.Flags, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
            m_gpuStart = m_heap->GetGPUDescriptorHandleForHeapStart();
        m_bitAlloc.Create(allocator, d3dHeapDesc.NumDescriptors);

        return S_OK;
    }

    void DescriptorPool::Destroy(Allocator& allocator)
    {
        HE_DX_SAFE_RELEASE(m_heap);
        m_bitAlloc.Destroy(allocator);
    }

    void DescriptorPool::Alloc(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle)
    {
        uint32_t offset = m_bitAlloc.Alloc(count);
        HE_ASSERT(offset != BitmapAllocator::InvalidOffset, HE_MSG("Cannot allocate {} additional descriptors, no block is available.", count));

        cpuHandle = { m_cpuStart.ptr + (offset * m_stride) };

        if (gpuHandle)
            *gpuHandle = { m_gpuStart.ptr + (offset * m_stride) };
    }

    void DescriptorPool::Free(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
    {
        if (cpuHandle.ptr)
        {
            uint32_t offset = static_cast<uint32_t>(cpuHandle.ptr - m_cpuStart.ptr) / m_stride;
            m_bitAlloc.Free(offset, count);
        }
    }
}

#endif
