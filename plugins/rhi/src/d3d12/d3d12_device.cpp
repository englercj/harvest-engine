// Copyright Chad Engler

#include "d3d12_device.h"

#include "d3d12_cmd_list.h"
#include "d3d12_instance.h"
#include "d3d12_formats.h"

#include "he/core/assert.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/scope_guard.h"
#include "he/math/vec4.h"
#include "he/rhi/utils.h"

#include <type_traits>

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    template <typename T>
    static void FillSamplerDesc(T& d3dDesc, const SamplerDesc& desc)
    {
        D3D12_FILTER_TYPE min = ToDxFilterType(desc.minFilter);
        D3D12_FILTER_TYPE mag = ToDxFilterType(desc.magFilter);
        D3D12_FILTER_TYPE mip = ToDxFilterType(desc.mipFilter);
        D3D12_FILTER_REDUCTION_TYPE reduction = ToDxFilterReductionType(desc.filterReduce);

        D3D12_FILTER filter = desc.maxAnisotropy > 1
            ? D3D12_ENCODE_ANISOTROPIC_FILTER(reduction)
            : D3D12_ENCODE_BASIC_FILTER(min, mag, mip, reduction);

        d3dDesc.Filter = filter;
        d3dDesc.AddressU = ToDxAddressMode(desc.addressU);
        d3dDesc.AddressV = ToDxAddressMode(desc.addressV);
        d3dDesc.AddressW = ToDxAddressMode(desc.addressW);
        d3dDesc.MipLODBias = desc.lodBias;
        d3dDesc.MaxAnisotropy = desc.maxAnisotropy;
        d3dDesc.ComparisonFunc = ToDxComparisonFunc(desc.comparisonFunc);
        d3dDesc.MinLOD = desc.minLOD;
        d3dDesc.MaxLOD = desc.maxLOD;
    }

    DeviceImpl::~DeviceImpl()
    {
        for (uint32_t i = 0; i < HE_LENGTH_OF(m_nullTextureSRV); ++i)
            m_cpuGeneralPool.Free(1, m_nullTextureSRV[i]);

        for (uint32_t i = 0; i < HE_LENGTH_OF(m_nullTextureUAV); ++i)
            m_cpuGeneralPool.Free(1, m_nullTextureUAV[i]);

        m_cpuGeneralPool.Free(1, m_nullBufferSRV);
        m_cpuGeneralPool.Free(1, m_nullBufferUAV);
        m_cpuGeneralPool.Free(1, m_nullBufferCBV);
        m_cpuSamplerPool.Free(1, m_nullSampler);

        m_cpuGeneralPool.Destroy(m_instance->m_allocator);
        m_cpuSamplerPool.Destroy(m_instance->m_allocator);
        m_cpuRtvPool.Destroy(m_instance->m_allocator);
        m_cpuDsvPool.Destroy(m_instance->m_allocator);
        m_gpuGeneralPool.Destroy(m_instance->m_allocator);
        m_gpuSamplerPool.Destroy(m_instance->m_allocator);

        HE_DX_SAFE_RELEASE(m_d3dDevice);
    }

    Result DeviceImpl::Initialize(InstanceImpl* instance, ID3D12Device* device, const AdapterImpl* adapter, const DeviceDesc& desc)
    {
        m_instance = instance;
        m_d3dDevice = device;
        m_adapter = adapter;

        // Gather device information
        m_info.backend = ApiBackend::D3D12;
        m_info.adapter = adapter->info;

        auto SupportShaderModel = [&](ShaderModel shaderModel)
        {
            m_info.supportedShaderModels[static_cast<uint32_t>(shaderModel)] = true;

            if (m_info.preferredShaderModel < shaderModel)
                m_info.preferredShaderModel = shaderModel;
        };

        SupportShaderModel(ShaderModel::Sm_5_0);

        static const D3D_SHADER_MODEL shaderModels[] =
        {
        #if defined(NTDDI_WIN10_FE) && NTDDI_VERSION >= NTDDI_WIN10_FE
            D3D_SHADER_MODEL_6_7,
        #endif
        #if defined(NTDDI_WIN10_VB) && NTDDI_VERSION >= NTDDI_WIN10_VB
            D3D_SHADER_MODEL_6_6,
        #endif
        // Windows 10 1903 "19H1"
        #if defined(NTDDI_WIN10_19H1) && NTDDI_VERSION >= NTDDI_WIN10_19H1
            D3D_SHADER_MODEL_6_5,
        #endif
        // Windows 10 1809 "Redstone 5"
        #if defined(NTDDI_WIN10_RS5) && NTDDI_VERSION >= NTDDI_WIN10_RS5
            D3D_SHADER_MODEL_6_4,
            D3D_SHADER_MODEL_6_3,
        #endif
        // Windows 10 1803 "Redstone 4"
        #if defined(NTDDI_WIN10_RS4) && NTDDI_VERSION >= NTDDI_WIN10_RS4
            D3D_SHADER_MODEL_6_2,
        #endif
        // Windows 10 1709 "Redstone 3"
        #if defined(NTDDI_WIN10_RS3) && NTDDI_VERSION >= NTDDI_WIN10_RS3
            D3D_SHADER_MODEL_6_1,
        #endif
            D3D_SHADER_MODEL_6_0,
            D3D_SHADER_MODEL_5_1
        };

        for (D3D_SHADER_MODEL shaderModel : shaderModels)
        {
            D3D12_FEATURE_DATA_SHADER_MODEL data;
            data.HighestShaderModel = shaderModel;
            if (SUCCEEDED(m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &data, sizeof(data))))
            {
                switch (data.HighestShaderModel)
                {
                    case D3D_SHADER_MODEL_5_1: SupportShaderModel(ShaderModel::Sm_5_1); break;
                    case D3D_SHADER_MODEL_6_0: SupportShaderModel(ShaderModel::Sm_6_0); break;
                #if defined(NTDDI_WIN10_RS3) && NTDDI_VERSION >= NTDDI_WIN10_RS3
                    case D3D_SHADER_MODEL_6_1: SupportShaderModel(ShaderModel::Sm_6_1); break;
                #endif
                #if defined(NTDDI_WIN10_RS4) && NTDDI_VERSION >= NTDDI_WIN10_RS4
                    case D3D_SHADER_MODEL_6_2: SupportShaderModel(ShaderModel::Sm_6_2); break;
                #endif
                #if defined(NTDDI_WIN10_RS5) && NTDDI_VERSION >= NTDDI_WIN10_RS5
                    case D3D_SHADER_MODEL_6_3: SupportShaderModel(ShaderModel::Sm_6_3); break;
                    case D3D_SHADER_MODEL_6_4: SupportShaderModel(ShaderModel::Sm_6_4); break;
                #endif
                #if defined(NTDDI_WIN10_19H1) && NTDDI_VERSION >= NTDDI_WIN10_19H1
                    case D3D_SHADER_MODEL_6_5: SupportShaderModel(ShaderModel::Sm_6_5); break;
                #endif
                #if defined(NTDDI_WIN10_VB) && NTDDI_VERSION >= NTDDI_WIN10_VB
                    case D3D_SHADER_MODEL_6_6: SupportShaderModel(ShaderModel::Sm_6_6); break;
                #endif
                #if defined(NTDDI_WIN10_FE) && NTDDI_VERSION >= NTDDI_WIN10_FE
                    case D3D_SHADER_MODEL_6_7: SupportShaderModel(ShaderModel::Sm_6_7); break;
                #endif
                    default:
                        HE_ASSERT(false, "Unknown Shader Model");
                        break;
                }
            }
        }

        m_info.uploadDataAlignment = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
        m_info.uploadDataPitchAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
        m_info.max1dTextureSize = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        m_info.max2dTextureSize = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        m_info.max3dTextureSize = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        m_info.maxCubeTextureSize = D3D12_REQ_TEXTURECUBE_DIMENSION;
        m_info.maxTextureLayers = Min(D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION, D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);

        // Set the maximum number of frames the application can queue at a single time
        IDXGIDevice1* dxgiDevice1 = nullptr;
        if (SUCCEEDED(m_d3dDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice1))))
        {
            dxgiDevice1->SetMaximumFrameLatency(desc.maxFrameLatency);
            HE_DX_SAFE_RELEASE(dxgiDevice1);
        }

        // Create command queues
        Result r = m_copyCmdQueue.Initialize(this);
        if (!r)
            return r;

        r = m_computeCmdQueue.Initialize(this);
        if (!r)
            return r;

        r = m_renderCmdQueue.Initialize(this);
        if (!r)
            return r;

        // Create descriptor heaps
        {
            D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc{};
            d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            d3dHeapDesc.NumDescriptors = desc.d3d12.cpuDescriptorHeapSizes.buffer;
            d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            HRESULT hr = m_cpuGeneralPool.Create(m_instance->m_allocator, m_d3dDevice, d3dHeapDesc);
            if (FAILED(hr))
                return MakeResult(hr);
            HE_DX_SET_NAME(m_cpuGeneralPool.GetHeap(), "CPU Buffer Heap");
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc{};
            d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            d3dHeapDesc.NumDescriptors = desc.d3d12.cpuDescriptorHeapSizes.sampler;
            d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            HRESULT hr = m_cpuSamplerPool.Create(m_instance->m_allocator, m_d3dDevice, d3dHeapDesc);
            if (FAILED(hr))
                return MakeResult(hr);
            HE_DX_SET_NAME(m_cpuGeneralPool.GetHeap(), "CPU Sampler Heap");
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc{};
            d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            d3dHeapDesc.NumDescriptors = desc.d3d12.cpuDescriptorHeapSizes.renderTarget;
            d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            HRESULT hr = m_cpuRtvPool.Create(m_instance->m_allocator, m_d3dDevice, d3dHeapDesc);
            if (FAILED(hr))
                return MakeResult(hr);
            HE_DX_SET_NAME(m_cpuGeneralPool.GetHeap(), "CPU RTV Heap");
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc{};
            d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            d3dHeapDesc.NumDescriptors = desc.d3d12.cpuDescriptorHeapSizes.depthStencil;
            d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            HRESULT hr = m_cpuDsvPool.Create(m_instance->m_allocator, m_d3dDevice, d3dHeapDesc);
            if (FAILED(hr))
                return MakeResult(hr);
            HE_DX_SET_NAME(m_cpuGeneralPool.GetHeap(), "CPU DSV Heap");
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc{};
            d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            d3dHeapDesc.NumDescriptors = desc.d3d12.gpuDescriptorHeapSizes.buffer;
            d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            HRESULT hr = m_gpuGeneralPool.Create(m_instance->m_allocator, m_d3dDevice, d3dHeapDesc);
            if (FAILED(hr))
                return MakeResult(hr);
            HE_DX_SET_NAME(m_cpuGeneralPool.GetHeap(), "GPU Buffer Heap");
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc{};
            d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            d3dHeapDesc.NumDescriptors = desc.d3d12.gpuDescriptorHeapSizes.sampler;
            d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            HRESULT hr = m_gpuSamplerPool.Create(m_instance->m_allocator, m_d3dDevice, d3dHeapDesc);
            if (FAILED(hr))
                return MakeResult(hr);
            HE_DX_SET_NAME(m_cpuGeneralPool.GetHeap(), "GPU Sampler Heap");
        }

        // Create null resources
        D3D12_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc{};
        d3dSrvDesc.Format = DXGI_FORMAT_R8_UINT;
        d3dSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        D3D12_UNORDERED_ACCESS_VIEW_DESC d3dUavDesc{};
        d3dUavDesc.Format = DXGI_FORMAT_R8_UINT;

        constexpr uint32_t TextureTypeCount = static_cast<uint32_t>(TextureType::_Count);
        for (uint32_t i = 0; i < TextureTypeCount; ++i)
        {
            TextureType type = static_cast<TextureType>(i);

            // Create null texture srv
            d3dSrvDesc.ViewDimension = ToDxSrvDimension(type);
            if (d3dSrvDesc.ViewDimension != D3D12_SRV_DIMENSION_UNKNOWN)
            {
                m_cpuGeneralPool.Alloc(1, m_nullTextureSRV[i]);
                m_d3dDevice->CreateShaderResourceView(nullptr, &d3dSrvDesc, m_nullTextureSRV[i]);
            }

            // Create null texture uav
            d3dUavDesc.ViewDimension = ToDxUavDimension(type);
            if (d3dUavDesc.ViewDimension != D3D12_UAV_DIMENSION_UNKNOWN)
            {
                m_cpuGeneralPool.Alloc(1, m_nullTextureUAV[i]);
                m_d3dDevice->CreateUnorderedAccessView(nullptr, nullptr, &d3dUavDesc, m_nullTextureUAV[i]);
            }
        }

        // Create null buffer srv
        d3dSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        m_cpuGeneralPool.Alloc(1, m_nullBufferSRV);
        m_d3dDevice->CreateShaderResourceView(nullptr, &d3dSrvDesc, m_nullBufferSRV);

        // Create null buffer uav
        d3dUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        m_cpuGeneralPool.Alloc(1, m_nullBufferUAV);
        m_d3dDevice->CreateUnorderedAccessView(nullptr, nullptr, &d3dUavDesc, m_nullBufferUAV);

        // create null buffer cbv
        m_cpuGeneralPool.Alloc(1, m_nullBufferCBV);
        m_d3dDevice->CreateConstantBufferView(nullptr, m_nullBufferCBV);

        // create null sampler
        D3D12_SAMPLER_DESC samplerDesc = {};
        samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        m_cpuSamplerPool.Alloc(1, m_nullSampler);
        m_d3dDevice->CreateSampler(&samplerDesc, m_nullSampler);

        return Result::Success;
    }

    Result DeviceImpl::MakeResult(HRESULT hr)
    {
        if (SUCCEEDED(hr))
            return Result::Success;

        if (hr == DXGI_ERROR_DEVICE_REMOVED)
            hr = m_d3dDevice->GetDeviceRemovedReason();

        return Win32Result(hr);
    }

    Result DeviceImpl::CreateBuffer(const BufferDesc& desc, Buffer*& out)
    {
        out = nullptr;

        // HE_ASSERT(IsAligned<uint32_t>(desc.size, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT)
        //     || !HasFlags(desc.usage, BufferUsage::Constants));

        D3D12_HEAP_PROPERTIES d3dHeapProperties{};
        d3dHeapProperties.Type = ToDxHeapType(desc.heapType);
        d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        d3dHeapProperties.CreationNodeMask = 1;
        d3dHeapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC d3dResourceDesc{};
        d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        d3dResourceDesc.Alignment = 0;
        d3dResourceDesc.Width = desc.size;
        d3dResourceDesc.Height = 1;
        d3dResourceDesc.DepthOrArraySize = 1;
        d3dResourceDesc.MipLevels = 1;
        d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        d3dResourceDesc.SampleDesc.Count = 1;
        d3dResourceDesc.SampleDesc.Quality = 0;
        d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (HasFlags(desc.usage, BufferUsage::ShaderReadWrite))
            d3dResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if (desc.heapType == HeapType::Readback)
            d3dResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        D3D12_RESOURCE_STATES initialState = ToDxBufferResourceState(desc.initialState);

        if (desc.heapType == HeapType::Upload)
            initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        else if (desc.heapType == HeapType::Readback)
            initialState = D3D12_RESOURCE_STATE_COPY_DEST;

        ID3D12Resource* d3dResource = nullptr;
        HRESULT hr = m_d3dDevice->CreateCommittedResource(
            &d3dHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &d3dResourceDesc,
            initialState,
            nullptr,
            IID_PPV_ARGS(&d3dResource));

        if (FAILED(hr))
            return MakeResult(hr);

        HE_DX_SET_NAME(d3dResource, desc.name);

        BufferImpl* buffer = m_instance->m_allocator.New<BufferImpl>();
        buffer->heapType = desc.heapType;
        buffer->usage = desc.usage;
        buffer->size = desc.size;
        buffer->stride = desc.stride;
        buffer->d3dResource = d3dResource;
        buffer->gpuAddress = d3dResource->GetGPUVirtualAddress();

        out = buffer;
        return Result::Success;
    }

    void DeviceImpl::DestroyBuffer(Buffer* buffer_)
    {
        BufferImpl* buffer = static_cast<BufferImpl*>(buffer_);
        HE_DX_SAFE_RELEASE(buffer->d3dResource);
        m_instance->m_allocator.Delete(buffer);
    }

    Result DeviceImpl::CreateBufferView(const BufferViewDesc& desc, BufferView*& out)
    {
        out = nullptr;

        const BufferImpl* buffer = static_cast<const BufferImpl*>(desc.buffer);

        if (!HE_VERIFY(HasFlags(buffer->usage, BufferUsage::ShaderRead)))
            return Result::InvalidParameter;

        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle;
        m_cpuGeneralPool.Alloc(1, d3dCpuHandle);

        D3D12_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc{};
        d3dSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        d3dSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        d3dSrvDesc.Buffer.FirstElement = desc.index;
        d3dSrvDesc.Buffer.NumElements = desc.count;
        d3dSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        uint32_t stride = buffer->stride ? buffer->stride : 1;

        switch (desc.type)
        {
            case BufferViewType::Raw:
                d3dSrvDesc.Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
                d3dSrvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                d3dSrvDesc.Buffer.StructureByteStride = 0;
                break;
            case BufferViewType::Structured:
                d3dSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
                d3dSrvDesc.Buffer.StructureByteStride = stride;
                break;
            case BufferViewType::Typed:
                d3dSrvDesc.Format = ToDxFormat(desc.format);
                d3dSrvDesc.Buffer.StructureByteStride = 0;
                break;
        }

        m_d3dDevice->CreateShaderResourceView(buffer->d3dResource, &d3dSrvDesc, d3dCpuHandle);

        BufferViewImpl* view = m_instance->m_allocator.New<BufferViewImpl>();
        view->buffer = buffer;
        view->d3dCpuHandle = d3dCpuHandle;
        view->gpuAddress = buffer->gpuAddress + (desc.index * stride);

        out = view;
        return Result::Success;
    }

    void DeviceImpl::DestroyBufferView(BufferView* view_)
    {
        BufferViewImpl* view = static_cast<BufferViewImpl*>(view_);
        m_cpuGeneralPool.Free(1, view->d3dCpuHandle);
        m_instance->m_allocator.Delete(view);
    }

    Result DeviceImpl::CreateRWBufferView(const BufferViewDesc& desc, RWBufferView*& out)
    {
        out = nullptr;

        const BufferImpl* buffer = static_cast<const BufferImpl*>(desc.buffer);

        if (!HE_VERIFY(HasFlags(buffer->usage, BufferUsage::ShaderReadWrite)))
            return Result::InvalidParameter;

        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuHandle;
        m_cpuGeneralPool.Alloc(1, d3dCpuHandle, &d3dGpuHandle);

        D3D12_UNORDERED_ACCESS_VIEW_DESC d3dUavDesc{};
        d3dUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        d3dUavDesc.Buffer.FirstElement = desc.index;
        d3dUavDesc.Buffer.NumElements = desc.count;
        d3dUavDesc.Buffer.CounterOffsetInBytes = 0;
        d3dUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        switch (desc.type)
        {
            case BufferViewType::Raw:
                d3dUavDesc.Buffer.Flags |= D3D12_BUFFER_UAV_FLAG_RAW;
                d3dUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                d3dUavDesc.Buffer.StructureByteStride = 0;
                break;
            case BufferViewType::Structured:
                d3dUavDesc.Format = DXGI_FORMAT_UNKNOWN;
                d3dUavDesc.Buffer.StructureByteStride = buffer->stride;
                break;
            case BufferViewType::Typed:
                d3dUavDesc.Format = ToDxFormat(desc.format);
                d3dUavDesc.Buffer.StructureByteStride = 0;
                break;
        }

        m_d3dDevice->CreateUnorderedAccessView(buffer->d3dResource, nullptr, &d3dUavDesc, d3dCpuHandle);

        RWBufferViewImpl* view = m_instance->m_allocator.New<RWBufferViewImpl>();
        view->buffer = buffer;
        view->d3dCpuHandle = d3dCpuHandle;
        view->d3dGpuHandle = d3dGpuHandle;

        out = view;
        return Result::Success;
    }

    void DeviceImpl::DestroyRWBufferView(RWBufferView* view_)
    {
        RWBufferViewImpl* view = static_cast<RWBufferViewImpl*>(view_);
        m_gpuGeneralPool.Free(1, view->d3dCpuHandle);
        m_instance->m_allocator.Delete(view);
    }

    void* DeviceImpl::Map(Buffer* buffer_, uint32_t offset, uint32_t size)
    {
        BufferImpl* buffer = static_cast<BufferImpl*>(buffer_);
        if (buffer->cpuAddress == nullptr)
        {
            // We shouldn't be reading from upload heaps. A range with Begin <= End
            // means we won't read anything
            if (buffer->heapType == HeapType::Upload)
            {
                D3D12_RANGE d3dReadRange{};
                HRESULT hr = buffer->d3dResource->Map(0, &d3dReadRange, &buffer->cpuAddress);
                HE_ASSERT(SUCCEEDED(hr));
                HE_UNUSED(hr);
            }
            // A non-upload buffer with a size > 0 means we do have a read hint.
            else if (size > 0)
            {
                D3D12_RANGE d3dReadRange{};
                d3dReadRange.Begin = offset;
                d3dReadRange.End = offset + size;
                HRESULT hr = buffer->d3dResource->Map(0, &d3dReadRange, &buffer->cpuAddress);
                HE_ASSERT(SUCCEEDED(hr));
                HE_UNUSED(hr);
            }
            // Passing a nullptr read range means we plan to read the entire buffer.
            else
            {
                HRESULT hr = buffer->d3dResource->Map(0, nullptr, &buffer->cpuAddress);
                HE_ASSERT(SUCCEEDED(hr));
                HE_UNUSED(hr);
            }
        }

        return static_cast<uint8_t*>(buffer->cpuAddress) + offset;
    }

    void DeviceImpl::Unmap(Buffer* buffer_)
    {
        BufferImpl* buffer = static_cast<BufferImpl*>(buffer_);
        HE_ASSERT(buffer->cpuAddress);

        // The write range tells D3D what the CPU has modified in this buffer. A range
        // with Begin <= End means we didn't write anything, and nullptr means that we may
        // have written to the entire buffer.
        D3D12_RANGE d3dWriteRange{};
        buffer->d3dResource->Unmap(0, buffer->heapType == HeapType::Readback ? &d3dWriteRange : nullptr);
        buffer->cpuAddress = nullptr;
    }

    Result DeviceImpl::CreateCopyCmdList(const CmdListDesc& desc, CopyCmdList*& out)
    {
        out = nullptr;

        CopyCmdListImpl* cmdList = m_instance->m_allocator.New<CopyCmdListImpl>();
        Result r = cmdList->Initialize(this, desc);
        if (!r)
        {
            m_instance->m_allocator.Delete(cmdList);
            return r;
        }

        out = cmdList;
        return Result::Success;
    }

    void DeviceImpl::DestroyCopyCmdList(CopyCmdList* cmdList_)
    {
        CopyCmdListImpl* cmdList = static_cast<CopyCmdListImpl*>(cmdList_);
        m_instance->m_allocator.Delete(cmdList);
    }

    Result DeviceImpl::CreateComputeCmdList(const CmdListDesc& desc, ComputeCmdList*& out)
    {
        out = nullptr;

        ComputeCmdListImpl* cmdList = m_instance->m_allocator.New<ComputeCmdListImpl>();
        Result r = cmdList->Initialize(this, desc);
        if (!r)
        {
            m_instance->m_allocator.Delete(cmdList);
            return r;
        }

        out = cmdList;
        return Result::Success;
    }

    void DeviceImpl::DestroyComputeCmdList(ComputeCmdList* cmdList_)
    {
        ComputeCmdListImpl* cmdList = static_cast<ComputeCmdListImpl*>(cmdList_);
        m_instance->m_allocator.Delete(cmdList);
    }

    Result DeviceImpl::CreateRenderCmdList(const CmdListDesc& desc, RenderCmdList*& out)
    {
        out = nullptr;

        RenderCmdListImpl* cmdList = m_instance->m_allocator.New<RenderCmdListImpl>();
        Result r = cmdList->Initialize(this, desc);
        if (!r)
        {
            m_instance->m_allocator.Delete(cmdList);
            return r;
        }

        out = cmdList;
        return Result::Success;
    }

    void DeviceImpl::DestroyRenderCmdList(RenderCmdList* cmdList_)
    {
        RenderCmdListImpl* cmdList = static_cast<RenderCmdListImpl*>(cmdList_);
        m_instance->m_allocator.Delete(cmdList);
    }

    Result DeviceImpl::CreateCmdAllocator(const CmdAllocatorDesc& desc, CmdAllocator*& out)
    {
        out = nullptr;

        ID3D12CommandAllocator* d3dCmdAllocator = nullptr;
        HRESULT hr = m_d3dDevice->CreateCommandAllocator(ToDxCmdListType(desc.type), IID_PPV_ARGS(&d3dCmdAllocator));

        if (FAILED(hr))
            return MakeResult(hr);

        CmdAllocatorImpl* pool = m_instance->m_allocator.New<CmdAllocatorImpl>();
        pool->d3dCmdAllocator = d3dCmdAllocator;

        out = pool;
        return Result::Success;
    }

    void DeviceImpl::DestroyCmdAllocator(CmdAllocator* pool_)
    {
        CmdAllocatorImpl* pool = static_cast<CmdAllocatorImpl*>(pool_);
        HE_DX_SAFE_RELEASE(pool->d3dCmdAllocator);
        m_instance->m_allocator.Delete(pool);
    }

    Result DeviceImpl::ResetCmdAllocator(CmdAllocator* pool_)
    {
        CmdAllocatorImpl* pool = static_cast<CmdAllocatorImpl*>(pool_);
        HRESULT hr = pool->d3dCmdAllocator->Reset();
        return MakeResult(hr);
    }

    Result DeviceImpl::CreateDescriptorTable(const DescriptorTableDesc& desc, DescriptorTable*& out)
    {
        out = nullptr;

        if (desc.rangeCount == 0)
            return Result::InvalidParameter;

        uint32_t generalCount = 0;
        uint32_t samplerCount = 0;

        for (uint32_t i = 0; i < desc.rangeCount; ++i)
        {
            const DescriptorRange& range = desc.ranges[i];

            switch (range.type)
            {
                case DescriptorRangeType::StructuredBuffer:
                case DescriptorRangeType::TypedBuffer:
                case DescriptorRangeType::Texture:
                case DescriptorRangeType::RWStructuredBuffer:
                case DescriptorRangeType::RWTypedBuffer:
                case DescriptorRangeType::RWTexture:
                case DescriptorRangeType::ConstantBuffer:
                    generalCount += range.count;
                    break;
                case DescriptorRangeType::Sampler:
                    samplerCount += range.count;
                    break;
            }
        }

        HE_ASSERT(generalCount == 0 || samplerCount == 0, "Sampler ranges cannot be mixed with other types. This is a D3D12 requirement.");

        DescriptorTableImpl* table = m_instance->m_allocator.New<DescriptorTableImpl>(m_instance->m_allocator);
        table->ranges.Clear();
        table->ranges.Insert(0, desc.ranges, desc.ranges + desc.rangeCount);

        if (generalCount)
        {
            HE_ASSERT(generalCount == table->ranges.Size());
            m_gpuGeneralPool.Alloc(generalCount, table->d3dCpuStart, &table->d3dGpuStart);
        }
        else
        {
            HE_ASSERT(samplerCount == table->ranges.Size());
            m_gpuSamplerPool.Alloc(samplerCount, table->d3dCpuStart, &table->d3dGpuStart);
        }

        out = table;
        return Result::Success;
    }

    void DeviceImpl::DestroyDescriptorTable(DescriptorTable* table_)
    {
        DescriptorTableImpl* table = static_cast<DescriptorTableImpl*>(table_);
        m_gpuGeneralPool.Free(static_cast<uint32_t>(table->ranges.Size()), table->d3dCpuStart);
        m_instance->m_allocator.Delete(table);
    }

    template <typename T, typename U>
    void DeviceImpl::SetDescriptorTableViews(
        DescriptorTable* table_,
        uint32_t rangeIndex,
        uint32_t descIndex,
        uint32_t count,
        const U* const* views)
    {
        DescriptorTableImpl* table = static_cast<DescriptorTableImpl*>(table_);

        HE_ASSERT(table->ranges.Size() > rangeIndex);
        HE_ASSERT(table->ranges[rangeIndex].count >= (descIndex + count));

        constexpr bool IsSampler = std::is_same_v<T, SamplerImpl>;
        DescriptorPool& pool = IsSampler ? m_gpuSamplerPool : m_gpuGeneralPool;

        uint32_t offset = 0;
        for (uint32_t i = 0; i < rangeIndex; ++i)
        {
            offset += table->ranges[i].count;
        }

        const D3D12_DESCRIPTOR_HEAP_TYPE heapType = pool.GetHeapType();
        const uint32_t stride = pool.GetStride();

        for (uint32_t i = 0; i < count; ++i)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE dstHandle{ table->d3dCpuStart.ptr + ((offset + descIndex + i) * stride) };
            D3D12_CPU_DESCRIPTOR_HANDLE srcHandle{ 0 };

            const T* view = static_cast<const T*>(views[i]);
            srcHandle = view->d3dCpuHandle;
            m_d3dDevice->CopyDescriptorsSimple(1, dstHandle, srcHandle, heapType);
        }
    }

    void DeviceImpl::SetBufferViews(DescriptorTable* table_, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const BufferView* const* views)
    {
        DescriptorTableImpl* table = static_cast<DescriptorTableImpl*>(table_);
        HE_ASSERT(table->ranges[rangeIndex].type == DescriptorRangeType::StructuredBuffer || table->ranges[rangeIndex].type == DescriptorRangeType::TypedBuffer);
        SetDescriptorTableViews<BufferViewImpl>(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetTextureViews(DescriptorTable* table_, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const TextureView* const* views)
    {
        DescriptorTableImpl* table = static_cast<DescriptorTableImpl*>(table_);
        HE_ASSERT(table->ranges[rangeIndex].type == DescriptorRangeType::Texture);
        SetDescriptorTableViews<TextureViewImpl>(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetRWBufferViews(DescriptorTable* table_, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWBufferView* const* views)
    {
        DescriptorTableImpl* table = static_cast<DescriptorTableImpl*>(table_);
        HE_ASSERT(table->ranges[rangeIndex].type == DescriptorRangeType::RWStructuredBuffer || table->ranges[rangeIndex].type == DescriptorRangeType::RWTypedBuffer);
        SetDescriptorTableViews<RWBufferViewImpl>(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetRWTextureViews(DescriptorTable* table_, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWTextureView* const* views)
    {
        DescriptorTableImpl* table = static_cast<DescriptorTableImpl*>(table_);
        HE_ASSERT(table->ranges[rangeIndex].type == DescriptorRangeType::RWTexture);
        SetDescriptorTableViews<RWTextureViewImpl>(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetConstantBufferViews(DescriptorTable* table_, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const ConstantBufferView* const* views)
    {
        DescriptorTableImpl* table = static_cast<DescriptorTableImpl*>(table_);
        HE_ASSERT(table->ranges[rangeIndex].type == DescriptorRangeType::ConstantBuffer);
        SetDescriptorTableViews<ConstantBufferViewImpl>(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetSamplers(DescriptorTable* table_, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const Sampler* const* samplers)
    {
        DescriptorTableImpl* table = static_cast<DescriptorTableImpl*>(table_);
        HE_ASSERT(table->ranges[rangeIndex].type == DescriptorRangeType::Sampler);
        SetDescriptorTableViews<SamplerImpl>(table, rangeIndex, descIndex, count, samplers);
    }

    Result DeviceImpl::CreateCpuFence(const CpuFenceDesc& desc, CpuFence*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;

        CpuFenceImpl* fence = m_instance->m_allocator.New<CpuFenceImpl>();
        auto guard = MakeScopeGuard([&]() { DestroyCpuFence(fence); });

        HRESULT hr = m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence->d3dFence));
        if (FAILED(hr))
            return MakeResult(hr);

        fence->event = CreateEventW(nullptr, false, false, nullptr);
        if (!fence->event)
            return Result::FromLastError();

        fence->d3dFence->Signal(0);

        out = fence;
        guard.Dismiss();
        return Result::Success;
    }

    void DeviceImpl::DestroyCpuFence(CpuFence* fence_)
    {
        CpuFenceImpl* fence = static_cast<CpuFenceImpl*>(fence_);

        HE_DX_SAFE_RELEASE(fence->d3dFence);

        if (fence->event)
        {
            CloseHandle(fence->event);
        }

        m_instance->m_allocator.Delete(fence);
    }

    Result DeviceImpl::CreateGpuFence(const GpuFenceDesc& desc, GpuFence*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;

        ID3D12Fence* d3dFence = nullptr;
        HRESULT hr = m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3dFence));
        if (FAILED(hr))
            return MakeResult(hr);

        d3dFence->Signal(0);

        GpuFenceImpl* fence = m_instance->m_allocator.New<GpuFenceImpl>();
        fence->d3dFence = d3dFence;

        out = fence;
        return Result::Success;
    }

    void DeviceImpl::DestroyGpuFence(GpuFence* fence_)
    {
        GpuFenceImpl* fence = static_cast<GpuFenceImpl*>(fence_);
        HE_DX_SAFE_RELEASE(fence->d3dFence);
        m_instance->m_allocator.Delete(fence);
    }

    bool DeviceImpl::WaitForFence(const CpuFence* fence_, uint32_t timeoutMs = static_cast<uint32_t>(-1))
    {
        const CpuFenceImpl* fence = static_cast<const CpuFenceImpl*>(fence_);
        DWORD rc = WaitForSingleObject(fence->event, timeoutMs);
        return rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED;
    }

    bool DeviceImpl::IsFenceSignaled(const CpuFence* fence_)
    {
        return WaitForFence(fence_, 0);
    }

    uint64_t DeviceImpl::GetFenceValue(const GpuFence* fence_)
    {
        const GpuFenceImpl* fence = static_cast<const GpuFenceImpl*>(fence_);
        return fence->d3dFence->GetCompletedValue();
    }

    Result DeviceImpl::CreateComputePipeline(const ComputePipelineDesc& desc, ComputePipeline*& out)
    {
        out = nullptr;

        HE_ASSERT(desc.rootSignature && desc.shader);

        D3D12_COMPUTE_PIPELINE_STATE_DESC d3dDesc{};
        d3dDesc.pRootSignature = static_cast<const RootSignatureImpl*>(desc.rootSignature)->d3dRootSignature;
        d3dDesc.CS = static_cast<const ShaderImpl*>(desc.shader)->byteCode;
        d3dDesc.CachedPSO.pCachedBlob = nullptr;
        d3dDesc.CachedPSO.CachedBlobSizeInBytes = 0;
        d3dDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        ID3D12PipelineState* d3dPipeline = nullptr;
        HRESULT hr = m_d3dDevice->CreateComputePipelineState(&d3dDesc, IID_PPV_ARGS(&d3dPipeline));

        if (FAILED(hr))
            return MakeResult(hr);

        HE_DX_SET_NAME(d3dPipeline, desc.name);

        ComputePipelineImpl* pipeline = m_instance->m_allocator.New<ComputePipelineImpl>();
        pipeline->d3dPipeline = d3dPipeline;

        out = pipeline;
        return Result::Success;
    }

    void DeviceImpl::DestroyComputePipeline(ComputePipeline* pipeline_)
    {
        ComputePipelineImpl* pipeline = static_cast<ComputePipelineImpl*>(pipeline_);
        HE_DX_SAFE_RELEASE(pipeline->d3dPipeline);
        m_instance->m_allocator.Delete(pipeline);
    }

    Result DeviceImpl::CreateRenderPipeline(const RenderPipelineDesc& desc, RenderPipeline*& out)
    {
        out = nullptr;

        HE_ASSERT(desc.rootSignature && desc.vertexShader);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dDesc = {};
        d3dDesc.pRootSignature = static_cast<const RootSignatureImpl*>(desc.rootSignature)->d3dRootSignature;

        // Shaders
        d3dDesc.VS = desc.vertexShader ? static_cast<const ShaderImpl*>(desc.vertexShader)->byteCode : D3D12_SHADER_BYTECODE{};
        d3dDesc.PS = desc.pixelShader ? static_cast<const ShaderImpl*>(desc.pixelShader)->byteCode : D3D12_SHADER_BYTECODE{};
        d3dDesc.DS = desc.domainShader ? static_cast<const ShaderImpl*>(desc.domainShader)->byteCode : D3D12_SHADER_BYTECODE{};
        d3dDesc.HS = desc.hullShader ? static_cast<const ShaderImpl*>(desc.hullShader)->byteCode : D3D12_SHADER_BYTECODE{};
        d3dDesc.GS = desc.geometryShader ? static_cast<const ShaderImpl*>(desc.geometryShader)->byteCode : D3D12_SHADER_BYTECODE{};

        // Stream output
        d3dDesc.StreamOutput.pSODeclaration = nullptr;
        d3dDesc.StreamOutput.NumEntries = 0;
        d3dDesc.StreamOutput.pBufferStrides = nullptr;
        d3dDesc.StreamOutput.NumStrides = 0;
        d3dDesc.StreamOutput.RasterizedStream = 0;

        // Blend state
        d3dDesc.BlendState.AlphaToCoverageEnable = desc.blend.alphaToCoverageEnable;
        d3dDesc.BlendState.IndependentBlendEnable = desc.blend.independentBlendEnable;
        for (uint32_t i = 0; i < desc.targets.renderTargetCount; ++i)
        {
            const BlendTargetDesc& blendDesc = desc.blend.targets[i];
            d3dDesc.BlendState.RenderTarget[i].BlendEnable = blendDesc.enable;
            d3dDesc.BlendState.RenderTarget[i].LogicOpEnable = false;
            d3dDesc.BlendState.RenderTarget[i].SrcBlend = ToDxBlend(blendDesc.srcRgb);
            d3dDesc.BlendState.RenderTarget[i].DestBlend = ToDxBlend(blendDesc.dstRgb);
            d3dDesc.BlendState.RenderTarget[i].BlendOp = ToDxBlendOp(blendDesc.opRgb);
            d3dDesc.BlendState.RenderTarget[i].SrcBlendAlpha = ToDxBlend(blendDesc.srcAlpha);
            d3dDesc.BlendState.RenderTarget[i].DestBlendAlpha = ToDxBlend(blendDesc.dstAlpha);
            d3dDesc.BlendState.RenderTarget[i].BlendOpAlpha = ToDxBlendOp(blendDesc.opAlpha);
            d3dDesc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
            d3dDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = static_cast<uint8_t>(blendDesc.writeMask);
        }

        const D3D12_RENDER_TARGET_BLEND_DESC defaultBlend =
        {
            FALSE, FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        };

        for (uint32_t i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        {
            d3dDesc.BlendState.RenderTarget[i] = defaultBlend;
        }

        d3dDesc.SampleMask = UINT_MAX;

        // Raster state
        d3dDesc.RasterizerState.FillMode = ToDxFillMode(desc.raster.fillMode);
        d3dDesc.RasterizerState.CullMode = ToDxCullMode(desc.raster.cullMode);
        d3dDesc.RasterizerState.FrontCounterClockwise = desc.raster.frontCounterClockwise;
        d3dDesc.RasterizerState.DepthBias = desc.raster.depthBias;
        d3dDesc.RasterizerState.DepthBiasClamp = desc.raster.depthBiasClamp;
        d3dDesc.RasterizerState.SlopeScaledDepthBias = desc.raster.slopeScaledDepthBias;
        d3dDesc.RasterizerState.DepthClipEnable = desc.raster.depthClamp;
        d3dDesc.RasterizerState.MultisampleEnable = FALSE;
        d3dDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        d3dDesc.RasterizerState.ForcedSampleCount = 0;
        d3dDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // Depth-stencil state
        d3dDesc.DepthStencilState.DepthEnable = desc.depth.testEnable;
        d3dDesc.DepthStencilState.DepthWriteMask = desc.depth.writeEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        d3dDesc.DepthStencilState.DepthFunc = ToDxComparisonFunc(desc.depth.func);
        d3dDesc.DepthStencilState.StencilEnable = desc.stencil.enable;
        d3dDesc.DepthStencilState.StencilReadMask = desc.stencil.readMask;
        d3dDesc.DepthStencilState.StencilWriteMask = desc.stencil.writeMask;
        d3dDesc.DepthStencilState.FrontFace.StencilFailOp = ToDxStencilOp(desc.stencil.frontFace.failOp);
        d3dDesc.DepthStencilState.FrontFace.StencilDepthFailOp = ToDxStencilOp(desc.stencil.frontFace.depthFailOp);
        d3dDesc.DepthStencilState.FrontFace.StencilPassOp = ToDxStencilOp(desc.stencil.frontFace.passOp);
        d3dDesc.DepthStencilState.FrontFace.StencilFunc = ToDxComparisonFunc(desc.stencil.frontFace.func);
        d3dDesc.DepthStencilState.BackFace.StencilFailOp = ToDxStencilOp(desc.stencil.backFace.failOp);
        d3dDesc.DepthStencilState.BackFace.StencilDepthFailOp = ToDxStencilOp(desc.stencil.backFace.depthFailOp);
        d3dDesc.DepthStencilState.BackFace.StencilPassOp = ToDxStencilOp(desc.stencil.backFace.passOp);
        d3dDesc.DepthStencilState.BackFace.StencilFunc = ToDxComparisonFunc(desc.stencil.backFace.func);

        // Input layout
        D3D12_INPUT_ELEMENT_DESC elems[D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT];
        uint32_t elemCount = 0;

        for (uint32_t bufferIndex = 0; bufferIndex < desc.vertexBufferCount; ++bufferIndex)
        {
            const VertexBufferFormatImpl* vbf = static_cast<const VertexBufferFormatImpl*>(desc.vertexBufferFormats[bufferIndex]);

            for (const VertexAttributeDesc& attr : vbf->attributes)
            {
                D3D12_INPUT_ELEMENT_DESC* elem = &elems[elemCount++];

                elem->SemanticName = attr.semanticName;
                elem->SemanticIndex = attr.semanticIndex;
                elem->Format = ToDxFormat(attr.format);
                elem->InputSlot = bufferIndex;
                elem->AlignedByteOffset = attr.offset;

                if (vbf->stepRate == StepRate::PerVertex)
                {
                    elem->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                    elem->InstanceDataStepRate = 0;
                }
                else
                {
                    elem->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                    elem->InstanceDataStepRate = 1;
                }
            }
        }

        d3dDesc.InputLayout.NumElements = elemCount;
        d3dDesc.InputLayout.pInputElementDescs = elems;

        d3dDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        d3dDesc.PrimitiveTopologyType = ToDxPrimitiveTopologyType(desc.primitiveType);

        // Framebuffer format
        d3dDesc.NumRenderTargets = desc.targets.renderTargetCount;
        for (uint32_t i = 0; i < desc.targets.renderTargetCount; ++i)
            d3dDesc.RTVFormats[i] = ToDxFormat(desc.targets.renderTargetFormats[i]);
        d3dDesc.DSVFormat = ToDxFormat(desc.targets.depthStencilFormat);
        d3dDesc.SampleDesc.Count = static_cast<UINT>(desc.targets.sampleCount);
        d3dDesc.SampleDesc.Quality = 0;

        // Caching
        d3dDesc.CachedPSO.pCachedBlob = nullptr;
        d3dDesc.CachedPSO.CachedBlobSizeInBytes = 0;

        // Flags
        d3dDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        // Create object
        ID3D12PipelineState* d3dPipeline = nullptr;
        HRESULT hr = m_d3dDevice->CreateGraphicsPipelineState(&d3dDesc, IID_PPV_ARGS(&d3dPipeline));

        if (FAILED(hr))
            return MakeResult(hr);

        HE_DX_SET_NAME(d3dPipeline, desc.name);

        RenderPipelineImpl* pipeline = m_instance->m_allocator.New<RenderPipelineImpl>();
        pipeline->d3dPipeline = d3dPipeline;
        pipeline->topology = ToDxPrimitiveTopology(desc.primitiveType);

        out = pipeline;
        return Result::Success;
    }

    void DeviceImpl::DestroyRenderPipeline(RenderPipeline* pipeline_)
    {
        RenderPipelineImpl* pipeline = static_cast<RenderPipelineImpl*>(pipeline_);
        HE_DX_SAFE_RELEASE(pipeline->d3dPipeline);
        m_instance->m_allocator.Delete(pipeline);
    }

    Result DeviceImpl::CreateRootSignature(const RootSignatureDesc& desc, RootSignature*& out)
    {
        out = nullptr;

        D3D12_ROOT_PARAMETER d3dParamDescs[64];
        D3D12_DESCRIPTOR_RANGE d3dRangeDescs[64]; // TODO: dynamically allocate

        HE_ASSERT(desc.slotCount <= HE_LENGTH_OF(d3dParamDescs));

        uint32_t rangeIdx = 0;

        for (uint32_t i = 0; i < desc.slotCount; ++i)
        {
            const SlotDesc& slotDesc = desc.slots[i];
            D3D12_ROOT_PARAMETER& d3dParamDesc = d3dParamDescs[i];
            MemZero(&d3dParamDesc, sizeof(d3dParamDesc));

            d3dParamDesc.ShaderVisibility = ToDxShaderVisibility(slotDesc.stage);

            switch (slotDesc.type)
            {
                case SlotType::DescriptorTable:
                {
                    d3dParamDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    HE_ASSERT((rangeIdx + slotDesc.descriptorTable.rangeCount) <= HE_LENGTH_OF(d3dRangeDescs));
                    uint32_t rangeStart = rangeIdx;
                    for (uint32_t j = 0; j < slotDesc.descriptorTable.rangeCount; ++j)
                    {
                        const DescriptorRange& range = slotDesc.descriptorTable.ranges[j];
                        D3D12_DESCRIPTOR_RANGE& d3dRangeDesc = d3dRangeDescs[rangeIdx++];
                        MemZero(&d3dRangeDesc, sizeof(d3dRangeDesc));
                        d3dRangeDesc.RangeType = ToDxRangeType(range.type);
                        d3dRangeDesc.NumDescriptors = range.count;
                        d3dRangeDesc.BaseShaderRegister = range.baseRegister;
                        d3dRangeDesc.RegisterSpace = range.registerSpace;
                        d3dRangeDesc.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    }
                    d3dParamDesc.DescriptorTable.NumDescriptorRanges = slotDesc.descriptorTable.rangeCount;
                    d3dParamDesc.DescriptorTable.pDescriptorRanges = d3dRangeDescs + rangeStart;
                    break;
                }
                case SlotType::ConstantBuffer:
                    d3dParamDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                    d3dParamDesc.Descriptor.ShaderRegister = slotDesc.constantBuffer.baseRegister;
                    d3dParamDesc.Descriptor.RegisterSpace = slotDesc.constantBuffer.registerSpace;
                    break;
                case SlotType::ConstantValues:
                    d3dParamDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                    d3dParamDesc.Constants.ShaderRegister = slotDesc.constantValues.baseRegister;
                    d3dParamDesc.Constants.RegisterSpace = slotDesc.constantValues.registerSpace;
                    d3dParamDesc.Constants.Num32BitValues = slotDesc.constantValues.num32BitValues;
                    break;
            }
        }

        D3D12_STATIC_SAMPLER_DESC d3dSamplerDescs[16];

        HE_ASSERT(desc.staticSamplerCount <= HE_LENGTH_OF(d3dSamplerDescs));

        for (uint32_t i = 0; i < desc.staticSamplerCount; ++i)
        {
            const StaticSamplerDesc& sampler = desc.staticSamplers[i];
            D3D12_STATIC_SAMPLER_DESC& d3dSamplerDesc = d3dSamplerDescs[i];
            MemZero(&d3dSamplerDesc, sizeof(d3dSamplerDesc));

            FillSamplerDesc(d3dSamplerDesc, sampler);

            d3dSamplerDesc.BorderColor = ToDxStaticBorderColor(sampler.borderColor);
            d3dSamplerDesc.ShaderRegister = sampler.baseRegister;
            d3dSamplerDesc.RegisterSpace = sampler.registerSpace;
            d3dSamplerDesc.ShaderVisibility = ToDxShaderVisibility(sampler.stage);
        }

        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        if (desc.inputAssembler)
            flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        HRESULT hr = S_OK;
        ID3DBlob* d3dBlob = nullptr;
        ID3DBlob* d3dErrorBlob = nullptr;

        HE_AT_SCOPE_EXIT([&]() { HE_DX_SAFE_RELEASE(d3dBlob); HE_DX_SAFE_RELEASE(d3dErrorBlob); });

        if (m_instance->m_D3D12SerializeVersionedRootSignature)
        {
            D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3dVersionedRootDesc{};
            d3dVersionedRootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
            d3dVersionedRootDesc.Desc_1_0.NumParameters = desc.slotCount;
            d3dVersionedRootDesc.Desc_1_0.pParameters = d3dParamDescs;
            d3dVersionedRootDesc.Desc_1_0.NumStaticSamplers = desc.staticSamplerCount;
            d3dVersionedRootDesc.Desc_1_0.pStaticSamplers = d3dSamplerDescs;
            d3dVersionedRootDesc.Desc_1_0.Flags = flags;
            hr = m_instance->m_D3D12SerializeVersionedRootSignature(&d3dVersionedRootDesc, &d3dBlob, &d3dErrorBlob);
        }
        else
        {
            D3D12_ROOT_SIGNATURE_DESC d3dRootDesc{};
            d3dRootDesc.NumParameters = desc.slotCount;
            d3dRootDesc.pParameters = d3dParamDescs;
            d3dRootDesc.NumStaticSamplers = desc.staticSamplerCount;
            d3dRootDesc.pStaticSamplers = d3dSamplerDescs;
            d3dRootDesc.Flags = flags;
            hr = m_instance->m_D3D12SerializeRootSignature(&d3dRootDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &d3dBlob, &d3dErrorBlob);
        }

        // TODO: read errors from blob
        if (FAILED(hr))
            return MakeResult(hr);

        ID3D12RootSignature* d3dRootSignature = nullptr;
        hr = m_d3dDevice->CreateRootSignature(0, d3dBlob->GetBufferPointer(), d3dBlob->GetBufferSize(), IID_PPV_ARGS(&d3dRootSignature));
        if (FAILED(hr))
            return MakeResult(hr);

        HE_DX_SET_NAME(d3dRootSignature, desc.name);

        RootSignatureImpl* rootSignature = m_instance->m_allocator.New<RootSignatureImpl>();
        rootSignature->d3dRootSignature = d3dRootSignature;

        out = rootSignature;
        return Result::Success;
    }

    void DeviceImpl::DestroyRootSignature(RootSignature* signature_)
    {
        RootSignatureImpl* signature = static_cast<RootSignatureImpl*>(signature_);
        HE_DX_SAFE_RELEASE(signature->d3dRootSignature);
        m_instance->m_allocator.Delete(signature);
    }

    Result DeviceImpl::CreateSampler(const SamplerDesc& desc, Sampler*& out)
    {
        out = nullptr;

        D3D12_SAMPLER_DESC d3dDesc;
        FillSamplerDesc(d3dDesc, desc);

        static_assert(sizeof(d3dDesc.BorderColor) == sizeof(decltype(ToDxBorderColor(std::declval<BorderColor>()))), "");
        MemCopy(d3dDesc.BorderColor, ToDxBorderColor(desc.borderColor), sizeof(d3dDesc.BorderColor));

        SamplerImpl* sampler = m_instance->m_allocator.New<SamplerImpl>();
        m_cpuSamplerPool.Alloc(1, sampler->d3dCpuHandle);

        m_d3dDevice->CreateSampler(&d3dDesc, sampler->d3dCpuHandle);

        out = sampler;
        return Result::Success;
    }

    void DeviceImpl::DestroySampler(Sampler* sampler_)
    {
        SamplerImpl* sampler = static_cast<SamplerImpl*>(sampler_);
        m_cpuSamplerPool.Free(1, sampler->d3dCpuHandle);
        m_instance->m_allocator.Delete(sampler);
    }

    Result DeviceImpl::CreateShader(const ShaderDesc& desc, Shader*& out)
    {
        out = nullptr;

        ShaderImpl* shader = m_instance->m_allocator.New<ShaderImpl>();
        shader->stage = desc.stage;
        shader->byteCode.pShaderBytecode = desc.code;
        shader->byteCode.BytecodeLength = desc.codeSize;

        out = shader;
        return Result::Success;
    }

    void DeviceImpl::DestroyShader(Shader* shader_)
    {
        ShaderImpl* shader = static_cast<ShaderImpl*>(shader_);
        m_instance->m_allocator.Delete(shader);
    }

    Result DeviceImpl::CreateSwapChain(const SwapChainDesc& desc, SwapChain*& out)
    {
        out = nullptr;

        HWND hwnd = static_cast<HWND>(desc.nativeViewHandle);

        // Create swap chain
        DXGI_SWAP_CHAIN_DESC1 dxgiDesc{};
        dxgiDesc.Width = desc.size.x;
        dxgiDesc.Height = desc.size.y;
        dxgiDesc.Format = ToDxSwapChainFormat(desc.format.format);
        dxgiDesc.Stereo = false;
        dxgiDesc.SampleDesc.Count = 1;
        dxgiDesc.SampleDesc.Quality = 0;
        dxgiDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        dxgiDesc.BufferCount = desc.bufferCount;
        dxgiDesc.Scaling = ToDxScaling(desc.scalingMode);
        dxgiDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        dxgiDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        dxgiDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        if (m_instance->m_allowTearing)
            dxgiDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        IDXGISwapChain1* dxgiSwapChain1 = nullptr;
        HRESULT hr = m_instance->m_dxgiFactory2->CreateSwapChainForHwnd(
            m_renderCmdQueue.m_d3dCmdQueue,
            hwnd,
            &dxgiDesc,
            nullptr,
            nullptr,
            &dxgiSwapChain1);

        if (FAILED(hr))
            return MakeResult(hr);

        IDXGISwapChain3* dxgiSwapChain3 = nullptr;
        hr = dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain3));
        HE_DX_SAFE_RELEASE(dxgiSwapChain1);

        if (FAILED(hr))
            return MakeResult(hr);

        m_instance->m_dxgiFactory2->MakeWindowAssociation(hwnd, DXGI_MWA_NO_PRINT_SCREEN | DXGI_MWA_NO_ALT_ENTER);

        hr = dxgiSwapChain3->SetMaximumFrameLatency(desc.bufferCount - 1);

        if (FAILED(hr))
        {
            HE_DX_SAFE_RELEASE(dxgiSwapChain3);
            return MakeResult(hr);
        }

        SwapChainImpl* swapChain = m_instance->m_allocator.New<SwapChainImpl>(m_instance->m_allocator);
        swapChain->dxgiSwapChain3 = dxgiSwapChain3;
        swapChain->swapEvent = dxgiSwapChain3->GetFrameLatencyWaitableObject();
        swapChain->format = desc.format;
        swapChain->bufferIndex = 0;
        swapChain->syncInterval = desc.enableVSync ? 1 : 0;
        swapChain->d3dResources.Resize(desc.bufferCount);
        swapChain->d3dRtvCpuHandles.Resize(desc.bufferCount);

        SetColorSpace(swapChain);

        hr = CreateSwapChainResources(swapChain);
        if (FAILED(hr))
        {
            DestroySwapChain(swapChain);
            return MakeResult(hr);
        }

        out = swapChain;
        return Result::Success;
    }

    void DeviceImpl::DestroySwapChain(SwapChain* swapChain_)
    {
        SwapChainImpl* swapChain = static_cast<SwapChainImpl*>(swapChain_);

        for (ID3D12Resource*& d3dResource : swapChain->d3dResources)
        {
            HE_DX_SAFE_RELEASE(d3dResource);
        }

        for (D3D12_CPU_DESCRIPTOR_HANDLE handle : swapChain->d3dRtvCpuHandles)
        {
            m_cpuRtvPool.Free(1, handle);
        }

        CloseHandle(swapChain->swapEvent);

        m_instance->m_allocator.Delete(swapChain->renderTargetView);
        m_instance->m_allocator.Delete(swapChain->texture);

        HE_DX_SAFE_RELEASE(swapChain->dxgiSwapChain3);
        m_instance->m_allocator.Delete(swapChain);
    }

    Result DeviceImpl::UpdateSwapChain(SwapChain* swapChain_, const SwapChainDesc& desc)
    {
        HE_ASSERT(desc.bufferCount > 0 && desc.bufferCount <= DXGI_MAX_SWAP_CHAIN_BUFFERS);

        SwapChainImpl* swapChain = static_cast<SwapChainImpl*>(swapChain_);
        IDXGISwapChain3* dxgiSwapChain3 = swapChain->dxgiSwapChain3;

        m_renderCmdQueue.WaitForFlush();

        HE_ASSERT(swapChain->d3dResources.Size() == swapChain->d3dRtvCpuHandles.Size());

        for (uint32_t i = 0; i < swapChain->d3dResources.Size(); ++i)
        {
            HE_DX_SAFE_RELEASE(swapChain->d3dResources[i]);
            m_cpuRtvPool.Free(1, swapChain->d3dRtvCpuHandles[i]);
        }

        swapChain->d3dResources.Clear();
        swapChain->d3dRtvCpuHandles.Clear();

        DXGI_SWAP_CHAIN_DESC1 dxgiDesc{};
        dxgiSwapChain3->GetDesc1(&dxgiDesc);
        HRESULT hr = dxgiSwapChain3->ResizeBuffers(desc.bufferCount, desc.size.x, desc.size.y, ToDxSwapChainFormat(desc.format.format), dxgiDesc.Flags);

        if (FAILED(hr))
            return MakeResult(hr);

        swapChain->d3dResources.Resize(desc.bufferCount);
        swapChain->d3dRtvCpuHandles.Resize(desc.bufferCount);
        swapChain->format = desc.format;
        swapChain->syncInterval = desc.enableVSync ? 1 : 0;

        SetColorSpace(swapChain);

        hr = CreateSwapChainResources(swapChain);
        if (FAILED(hr))
            return MakeResult(hr);

        return Result::Success;
    }

    PresentTarget DeviceImpl::AcquirePresentTarget(SwapChain* swapChain_)
    {
        SwapChainImpl* swapChain = static_cast<SwapChainImpl*>(swapChain_);

        // 1 second timeout should never happen
        // https://github.com/microsoftarchive/msdn-code-gallery-microsoft/blob/21cb9b6bc0da3b234c5854ecac449cb3bd261f29/Official%20Windows%20Platform%20Sample/DirectX%20latency%20sample/%5BC%2B%2B%5D-DirectX%20latency%20sample/C%2B%2B/DeviceResources.cpp#L665
        WaitForSingleObjectEx(swapChain->swapEvent, 1000, TRUE);

        PresentTarget target;
        target.texture = swapChain->texture;
        target.renderTargetView = swapChain->renderTargetView;

        return target;
    }

    bool DeviceImpl::IsFullscreen(SwapChain* swapChain_)
    {
        SwapChainImpl* swapChain = static_cast<SwapChainImpl*>(swapChain_);
        BOOL state = FALSE;
        HRESULT hr = swapChain->dxgiSwapChain3->GetFullscreenState(&state, nullptr);
        if (SUCCEEDED(hr))
            return !!state;

        return false;
    }

    Result DeviceImpl::SetFullscreen(SwapChain* swapChain_, bool fullscreen)
    {
        SwapChainImpl* swapChain = static_cast<SwapChainImpl*>(swapChain_);
        HRESULT hr = swapChain->dxgiSwapChain3->SetFullscreenState(fullscreen, nullptr);
        if (FAILED(hr))
            return MakeResult(hr);

        return Result::Success;
    }

    Result DeviceImpl::CreateTexture(const TextureDesc& desc, Texture*& out)
    {
        out = nullptr;

        // Heap Properties
        D3D12_HEAP_PROPERTIES d3dHeapProperties;
        d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        d3dHeapProperties.CreationNodeMask = 1;
        d3dHeapProperties.VisibleNodeMask = 1;

        // Resource Description
        D3D12_RESOURCE_DESC d3dResourceDesc;
        d3dResourceDesc.Dimension = ToDxTextureDimension(desc.type);
        d3dResourceDesc.Alignment = 0;
        d3dResourceDesc.Width = desc.size.x;
        d3dResourceDesc.Height = desc.size.y;
        switch (desc.type)
        {
            case TextureType::_3D:
                d3dResourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.size.z);
                break;
            case TextureType::Cube:
            case TextureType::CubeArray:
                d3dResourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.layerCount * 6);
                break;
            default:
                d3dResourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.layerCount);
                break;
        }
        d3dResourceDesc.MipLevels = static_cast<UINT16>(desc.mipCount);
        d3dResourceDesc.Format = ToDxFormat(desc.format);
        d3dResourceDesc.SampleDesc.Count = static_cast<UINT>(desc.sampleCount);
        d3dResourceDesc.SampleDesc.Quality = 0;
        d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (HasFlag(desc.usage, TextureUsage::ShaderReadWrite))
            d3dResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        const bool isRenderTarget = HasFlag(desc.usage, TextureUsage::RenderTarget);
        if (isRenderTarget)
        {
            if (IsDepthFormat(desc.format))
            {
                if (!HasFlag(desc.usage, TextureUsage::ShaderRead))
                    d3dResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

                d3dResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }
            else
                d3dResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }

        // Initial resource state
        D3D12_RESOURCE_STATES d3dResourceState = ToDxTextureResourceState(desc.initialState);

        // Optimized clear value
        D3D12_CLEAR_VALUE d3dClearValue{};
        d3dClearValue.Format = ToDxFormat(desc.format);

        if ((d3dResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
        {
            d3dClearValue.DepthStencil.Depth = desc.optimizedClearDepth;
            d3dClearValue.DepthStencil.Stencil = desc.optimizedClearStencil;
        }
        else if ((d3dResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
        {
            static_assert(sizeof(d3dClearValue.Color) == sizeof(desc.optimizedClearColor), "");
            MemCopy(d3dClearValue.Color, GetPointer(desc.optimizedClearColor), sizeof(d3dClearValue.Color));
        }

        // Create texture
        ID3D12Resource* d3dResource = nullptr;
        HRESULT hr = m_d3dDevice->CreateCommittedResource(
            &d3dHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &d3dResourceDesc,
            d3dResourceState,
            isRenderTarget ? &d3dClearValue : nullptr,
            IID_PPV_ARGS(&d3dResource));

        if (FAILED(hr))
            return MakeResult(hr);

        HE_DX_SET_NAME(d3dResource, desc.name);

        TextureImpl* texture = m_instance->m_allocator.New<TextureImpl>();
        texture->format = desc.format;
        texture->mipCount = desc.mipCount;
        texture->layerCount = desc.layerCount;
        texture->type = desc.type;
        texture->d3dResource = d3dResource;

        out = texture;
        return Result::Success;
    }

    void DeviceImpl::DestroyTexture(Texture* texture_)
    {
        TextureImpl* texture = static_cast<TextureImpl*>(texture_);
        HE_DX_SAFE_RELEASE(texture->d3dResource);
        m_instance->m_allocator.Delete(texture);
    }

    Result DeviceImpl::CreateTextureView(const TextureViewDesc& desc, TextureView*& out)
    {
        out = nullptr;

        const TextureImpl* texture = static_cast<const TextureImpl*>(desc.texture);
        TextureType type = desc.type == TextureType::Unknown ? texture->type : desc.type;
        Format format = desc.format == Format::Invalid ? texture->format : desc.format;

        D3D12_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc{};
        d3dSrvDesc.ViewDimension = ToDxSrvDimension(type);
        d3dSrvDesc.Format = ToDxFormat(format);
        d3dSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        HE_ASSERT(d3dSrvDesc.ViewDimension != D3D12_SRV_DIMENSION_UNKNOWN);

        switch (type)
        {
            case TextureType::_1D:
                d3dSrvDesc.Texture1D.MostDetailedMip = desc.mipIndex;
                d3dSrvDesc.Texture1D.MipLevels = desc.mipCount;
                d3dSrvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
                break;
            case TextureType::_1DArray:
                d3dSrvDesc.Texture1DArray.MostDetailedMip = desc.mipIndex;
                d3dSrvDesc.Texture1DArray.MipLevels = desc.mipCount;
                d3dSrvDesc.Texture1DArray.FirstArraySlice = desc.layerIndex;
                d3dSrvDesc.Texture1DArray.ArraySize = desc.layerCount;
                d3dSrvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
                break;
            case TextureType::_2D:
                d3dSrvDesc.Texture2D.MostDetailedMip = desc.mipIndex;
                d3dSrvDesc.Texture2D.MipLevels = desc.mipCount;
                d3dSrvDesc.Texture2D.PlaneSlice = 0;
                d3dSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                break;
            case TextureType::_2DArray:
                d3dSrvDesc.Texture2DArray.MostDetailedMip = desc.mipIndex;
                d3dSrvDesc.Texture2DArray.MipLevels = desc.mipCount;
                d3dSrvDesc.Texture2DArray.FirstArraySlice = desc.layerIndex;
                d3dSrvDesc.Texture2DArray.ArraySize = desc.layerCount;
                d3dSrvDesc.Texture2DArray.PlaneSlice = 0;
                d3dSrvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
                break;
            case TextureType::_2DMS:
                break;
            case TextureType::_2DMSArray:
                d3dSrvDesc.Texture2DMSArray.FirstArraySlice = desc.layerIndex;
                d3dSrvDesc.Texture2DMSArray.ArraySize = desc.layerCount;
                break;
            case TextureType::Cube:
                d3dSrvDesc.TextureCube.MostDetailedMip = desc.mipIndex;
                d3dSrvDesc.TextureCube.MipLevels = desc.mipCount;
                d3dSrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
                break;
            case TextureType::CubeArray:
                d3dSrvDesc.TextureCubeArray.MostDetailedMip = desc.mipIndex;
                d3dSrvDesc.TextureCubeArray.MipLevels = desc.mipCount;
                d3dSrvDesc.TextureCubeArray.First2DArrayFace = desc.layerIndex;
                d3dSrvDesc.TextureCubeArray.NumCubes = desc.layerCount / 6;
                d3dSrvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
                break;
            case TextureType::_3D:
                d3dSrvDesc.Texture3D.MostDetailedMip = desc.mipIndex;
                d3dSrvDesc.Texture3D.MipLevels = desc.mipCount;
                d3dSrvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
                break;
            default:
                return Result::InvalidParameter;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle;
        m_cpuGeneralPool.Alloc(1, d3dCpuHandle);

        m_d3dDevice->CreateShaderResourceView(texture->d3dResource, &d3dSrvDesc, d3dCpuHandle);

        TextureViewImpl* view = m_instance->m_allocator.New<TextureViewImpl>();
        view->texture = texture;
        view->d3dCpuHandle = d3dCpuHandle;
        view->firstSubresource = desc.mipIndex;

        out = view;
        return Result::Success;
    }

    void DeviceImpl::DestroyTextureView(TextureView* view_)
    {
        TextureViewImpl* view = static_cast<TextureViewImpl*>(view_);
        m_cpuGeneralPool.Free(1, view->d3dCpuHandle);
        m_instance->m_allocator.Delete(view);
    }

    Result DeviceImpl::CreateRWTextureView(const TextureViewDesc& desc, RWTextureView*& out)
    {
        out = nullptr;

        const TextureImpl* texture = static_cast<const TextureImpl*>(desc.texture);
        TextureType type = desc.type == TextureType::Unknown ? texture->type : desc.type;
        Format format = desc.format == Format::Invalid ? texture->format : desc.format;

        D3D12_UNORDERED_ACCESS_VIEW_DESC d3dUavDesc{};
        d3dUavDesc.ViewDimension = ToDxUavDimension(type);
        d3dUavDesc.Format = ToDxFormat(format);

        HE_ASSERT(d3dUavDesc.ViewDimension != D3D12_UAV_DIMENSION_UNKNOWN);

        switch (type)
        {
            case TextureType::_1D:
                d3dUavDesc.Texture1D.MipSlice = desc.mipIndex;
                break;
            case TextureType::_1DArray:
                d3dUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                d3dUavDesc.Texture1DArray.MipSlice = desc.mipIndex;
                d3dUavDesc.Texture1DArray.FirstArraySlice = desc.layerIndex;
                d3dUavDesc.Texture1DArray.ArraySize = desc.layerCount;
                break;
            case TextureType::_2D:
                d3dUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                d3dUavDesc.Texture2D.MipSlice = desc.mipIndex;
                d3dUavDesc.Texture2D.PlaneSlice = 0;
                break;
            case TextureType::_2DArray:
            case TextureType::Cube:
            case TextureType::CubeArray:
                d3dUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                d3dUavDesc.Texture2DArray.MipSlice = desc.mipIndex;
                d3dUavDesc.Texture2DArray.FirstArraySlice = desc.layerIndex;
                d3dUavDesc.Texture2DArray.ArraySize = desc.layerCount;
                d3dUavDesc.Texture2D.PlaneSlice = 0;
                break;
            case TextureType::_3D:
                d3dUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                d3dUavDesc.Texture3D.MipSlice = desc.mipIndex;
                d3dUavDesc.Texture3D.FirstWSlice = 0;
                d3dUavDesc.Texture3D.WSize = ~0U;
                break;
            default:
                return Result::InvalidParameter;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuHandle;
        m_cpuGeneralPool.Alloc(1, d3dCpuHandle, &d3dGpuHandle);

        m_d3dDevice->CreateUnorderedAccessView(texture->d3dResource, nullptr, &d3dUavDesc, d3dCpuHandle);

        RWTextureViewImpl* view = m_instance->m_allocator.New<RWTextureViewImpl>();
        view->texture = texture;
        view->d3dCpuHandle = d3dCpuHandle;
        view->d3dGpuHandle = d3dGpuHandle;
        view->firstSubresource = desc.mipIndex;

        out = view;
        return Result::Success;
    }

    void DeviceImpl::DestroyRWTextureView(RWTextureView* view_)
    {
        RWTextureViewImpl* view = static_cast<RWTextureViewImpl*>(view_);
        m_cpuGeneralPool.Free(1, view->d3dCpuHandle);
        m_instance->m_allocator.Delete(view);
    }

    Result DeviceImpl::CreateRenderTargetView(const TextureViewDesc& desc, RenderTargetView*& out)
    {
        out = nullptr;

        const TextureImpl* texture = static_cast<const TextureImpl*>(desc.texture);
        TextureType type = desc.type == TextureType::Unknown ? texture->type : desc.type;
        Format format = desc.format == Format::Invalid ? texture->format : desc.format;

        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle;

        if (IsDepthFormat(format))
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC d3dDsvDesc;
            d3dDsvDesc.ViewDimension = ToDxDsvDimension(type);
            d3dDsvDesc.Format = ToDxFormat(format);
            d3dDsvDesc.Flags = D3D12_DSV_FLAG_NONE;

            if (desc.readOnlyDepth)
                d3dDsvDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;

            if (desc.readOnlyStencil)
                d3dDsvDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;

            switch (type)
            {
                case TextureType::_1D:
                    d3dDsvDesc.Texture1D.MipSlice = desc.mipIndex;
                    break;
                case TextureType::_1DArray:
                    d3dDsvDesc.Texture1DArray.MipSlice = desc.mipIndex;
                    d3dDsvDesc.Texture1DArray.FirstArraySlice = desc.layerIndex;
                    d3dDsvDesc.Texture1DArray.ArraySize = desc.layerCount;
                    break;
                case TextureType::_2D:
                    d3dDsvDesc.Texture2D.MipSlice = desc.mipIndex;
                    break;
                case TextureType::_2DArray:
                case TextureType::Cube:
                case TextureType::CubeArray:
                    d3dDsvDesc.Texture2DArray.MipSlice = desc.mipIndex;
                    d3dDsvDesc.Texture2DArray.FirstArraySlice = desc.layerIndex;
                    d3dDsvDesc.Texture2DArray.ArraySize = desc.layerCount;
                    break;
                case TextureType::_2DMS:
                    break;
                case TextureType::_2DMSArray:
                    d3dDsvDesc.Texture2DMSArray.FirstArraySlice = desc.layerIndex;
                    d3dDsvDesc.Texture2DMSArray.ArraySize = desc.layerCount;
                    break;
                default:
                    return Result::InvalidParameter;
            }

            m_cpuDsvPool.Alloc(1, d3dCpuHandle);

            m_d3dDevice->CreateDepthStencilView(texture->d3dResource, &d3dDsvDesc, d3dCpuHandle);
        }
        else
        {
            D3D12_RENDER_TARGET_VIEW_DESC d3dRtvDesc;
            d3dRtvDesc.ViewDimension = ToDxRtvDimension(type);
            d3dRtvDesc.Format = ToDxFormat(format);

            switch (type)
            {
                case TextureType::_1D:
                    d3dRtvDesc.Texture1D.MipSlice = desc.mipIndex;
                    break;
                case TextureType::_1DArray:
                    d3dRtvDesc.Texture1DArray.MipSlice = desc.mipIndex;
                    d3dRtvDesc.Texture1DArray.FirstArraySlice = desc.layerIndex;
                    d3dRtvDesc.Texture1DArray.ArraySize = desc.layerCount;
                    break;
                case TextureType::_2D:
                    d3dRtvDesc.Texture2D.MipSlice = desc.layerIndex;
                    d3dRtvDesc.Texture2D.PlaneSlice = 0;
                    break;
                case TextureType::_2DArray:
                case TextureType::Cube:
                case TextureType::CubeArray:
                    d3dRtvDesc.Texture2DArray.MipSlice = desc.mipIndex;
                    d3dRtvDesc.Texture2DArray.FirstArraySlice = desc.layerIndex;
                    d3dRtvDesc.Texture2DArray.ArraySize = desc.layerCount;
                    d3dRtvDesc.Texture2D.PlaneSlice = 0;
                    break;
                case TextureType::_2DMS:
                    break;
                case TextureType::_2DMSArray:
                    d3dRtvDesc.Texture2DMSArray.FirstArraySlice = desc.layerIndex;
                    d3dRtvDesc.Texture2DMSArray.ArraySize = desc.layerCount;
                    break;
                case TextureType::_3D:
                    d3dRtvDesc.Texture3D.MipSlice = desc.mipIndex;
                    d3dRtvDesc.Texture3D.FirstWSlice = desc.layerIndex;
                    d3dRtvDesc.Texture3D.WSize = ~0U;
                    break;
                default:
                    return Result::InvalidParameter;
            }

            m_cpuRtvPool.Alloc(1, d3dCpuHandle);

            m_d3dDevice->CreateRenderTargetView(texture->d3dResource, &d3dRtvDesc, d3dCpuHandle);
        }

        RenderTargetViewImpl* view = m_instance->m_allocator.New<RenderTargetViewImpl>();
        view->texture = texture;
        view->d3dCpuHandle = d3dCpuHandle;
        view->firstSubresource = desc.mipIndex;
        view->format = format;

        out = view;
        return Result::Success;
    }

    void DeviceImpl::DestroyRenderTargetView(RenderTargetView* view_)
    {
        RenderTargetViewImpl* view = static_cast<RenderTargetViewImpl*>(view_);
        m_cpuRtvPool.Free(1, view->d3dCpuHandle);
        m_instance->m_allocator.Delete(view);
    }

    Result DeviceImpl::CreateConstantBufferView(const ConstantBufferViewDesc& desc, ConstantBufferView*& out)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle;
        m_cpuGeneralPool.Alloc(1, d3dCpuHandle);

        const BufferImpl* buffer = static_cast<const BufferImpl*>(desc.buffer);

        D3D12_CONSTANT_BUFFER_VIEW_DESC d3dDesc{};
        d3dDesc.BufferLocation = buffer->gpuAddress + desc.offset;
        d3dDesc.SizeInBytes = desc.size;
        m_d3dDevice->CreateConstantBufferView(&d3dDesc, d3dCpuHandle);

        ConstantBufferViewImpl* view = m_instance->m_allocator.New<ConstantBufferViewImpl>();
        view->d3dCpuHandle = d3dCpuHandle;

        out = view;
        return Result::Success;
    }

    void DeviceImpl::DestroyConstantBufferView(ConstantBufferView* view_)
    {
        ConstantBufferViewImpl* view = static_cast<ConstantBufferViewImpl*>(view_);
        m_cpuGeneralPool.Free(1, view->d3dCpuHandle);
        m_instance->m_allocator.Delete(view);
    }

    Result DeviceImpl::CreateVertexBufferFormat(const VertexBufferFormatDesc& desc, VertexBufferFormat*& out)
    {
        out = nullptr;

        VertexBufferFormatImpl* vbf = m_instance->m_allocator.New<VertexBufferFormatImpl>(m_instance->m_allocator);
        vbf->stride = desc.stride;
        vbf->stepRate = desc.stepRate;
        vbf->attributes.Clear();
        vbf->attributes.Insert(0, desc.attributes, desc.attributes + desc.attributeCount);

        out = vbf;
        return Result::Success;
    }

    void DeviceImpl::DestroyVertexBufferFormat(VertexBufferFormat* vbf_)
    {
        VertexBufferFormatImpl* vbf = static_cast<VertexBufferFormatImpl*>(vbf_);
        m_instance->m_allocator.Delete(vbf);
    }

    Result DeviceImpl::GetSwapChainFormats(void* nvh, uint32_t& count, SwapChainFormat* formats)
    {
        count = 0;

        // Based on HDR support check in D3D12 examples from MS
        // https://github.com/microsoft/DirectX-Graphics-Samples/blob/05c6b6454378cb3501a39528f4417bb30307f146/Samples/UWP/D3D12HDR/src/D3D12HDR.cpp#L1010

        HWND windowHandle = static_cast<HWND>(nvh);
        RECT windowRect{};
        ::GetWindowRect(windowHandle, &windowRect);

        const DisplayImpl* bestDisplay = nullptr;
        int bestIntersectionArea = -1;
        for (const DisplayImpl& display : m_adapter->displays)
        {
            // Get the rectangle bounds of current output
            RECT intersectRect{};
            RECT outputRect{
                display.info.pos.x,
                display.info.pos.y,
                display.info.pos.x + display.info.size.x,
                display.info.pos.y + display.info.size.y,
            };

            int intersectionArea = 0;
            if (::IntersectRect(&intersectRect, &windowRect, &outputRect))
            {
                intersectionArea = (intersectRect.bottom - intersectRect.top) * (intersectRect.right - intersectRect.left);
            }

            if (intersectionArea > bestIntersectionArea)
            {
                bestDisplay = &display;
                bestIntersectionArea = intersectionArea;
            }
        }

        if (bestDisplay)
        {
            // Check if HDR is supported
            if (bestDisplay->info.colorSpace == ColorSpace::HDR10_PQ)
            {
                if (formats)
                {
                    formats[count].format = Format::RGB10A2Unorm;
                    formats[count].colorSpace = ColorSpace::HDR10_PQ;
                }
                ++count;
            }
        }

        if (formats)
        {
            formats[count].format = Format::RGBA8Unorm;
            formats[count].colorSpace = ColorSpace::sRGB;
        }
        ++count;

        return Result::Success;
    }

    HRESULT DeviceImpl::CreateSwapChainResources(SwapChain* swapChain_)
    {
        SwapChainImpl* swapChain = static_cast<SwapChainImpl*>(swapChain_);
        IDXGISwapChain3* dxgiSwapChain3 = swapChain->dxgiSwapChain3;

        HRESULT hr;

        HE_ASSERT(swapChain->d3dResources.Size() == swapChain->d3dRtvCpuHandles.Size());

        // Gather resource pointers and create render target views
        for (uint32_t i = 0; i < swapChain->d3dResources.Size(); ++i)
        {
            hr = dxgiSwapChain3->GetBuffer(i, IID_PPV_ARGS(&swapChain->d3dResources[i]));
            if (FAILED(hr))
                return hr;

            HE_DX_SET_NAME(swapChain->d3dResources[i], "Swap Chain");

            m_cpuRtvPool.Alloc(1, swapChain->d3dRtvCpuHandles[i]);

            D3D12_RENDER_TARGET_VIEW_DESC desc{};
            desc.Format = ToDxSwapChainViewFormat(swapChain->format.format);
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;
            desc.Texture2D.PlaneSlice = 0;

            m_d3dDevice->CreateRenderTargetView(swapChain->d3dResources[i], &desc, swapChain->d3dRtvCpuHandles[i]);
        }

        // Choose backbuffer index
        swapChain->bufferIndex = dxgiSwapChain3->GetCurrentBackBufferIndex();

        // Create or update texture and rtv pointers
        if (!swapChain->texture)
        {
            swapChain->texture = m_instance->m_allocator.New<TextureImpl>();
        }
        else
        {
            *swapChain->texture = {};
        }
        swapChain->texture->format = swapChain->format.format;
        swapChain->texture->d3dResource = swapChain->d3dResources[swapChain->bufferIndex];

        if (!swapChain->renderTargetView)
        {
            swapChain->renderTargetView = m_instance->m_allocator.New<RenderTargetViewImpl>();
        }
        else
        {
            *swapChain->renderTargetView = {};
        }
        swapChain->renderTargetView->texture = swapChain->texture;
        swapChain->renderTargetView->d3dCpuHandle = swapChain->d3dRtvCpuHandles[swapChain->bufferIndex];

        return S_OK;
    }

    void DeviceImpl::SetColorSpace(SwapChain* swapChain_)
    {
        SwapChainImpl* swapChain = static_cast<SwapChainImpl*>(swapChain_);
        IDXGISwapChain3* dxgiSwapChain3 = swapChain->dxgiSwapChain3;

        DXGI_COLOR_SPACE_TYPE reqColorSpace = ToDxColorSpace(swapChain->format.colorSpace);
        swapChain->colorSpaceEnabled = false;

        IDXGISwapChain4* dxgiSwapChain4 = nullptr;
        if (SUCCEEDED(dxgiSwapChain3->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain4))))
        {
            IDXGIOutput* output = nullptr;
            if (SUCCEEDED(dxgiSwapChain4->GetContainingOutput(&output)))
            {
                IDXGIOutput6* output6 = nullptr;
                if (SUCCEEDED(output->QueryInterface(IID_PPV_ARGS(&output6))))
                {
                    DXGI_OUTPUT_DESC1 outputDesc;
                    if (SUCCEEDED(output6->GetDesc1(&outputDesc)))
                    {
                        uint32_t colorSpaceSupport;
                        if (SUCCEEDED(dxgiSwapChain4->CheckColorSpaceSupport(reqColorSpace, &colorSpaceSupport))
                            && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
                        {
                            if (SUCCEEDED(dxgiSwapChain4->SetColorSpace1(reqColorSpace)))
                            {
                                swapChain->colorSpaceEnabled = true;
                            }
                        }
                    }
                    HE_DX_SAFE_RELEASE(output6);
                }
                HE_DX_SAFE_RELEASE(output);
            }
            HE_DX_SAFE_RELEASE(dxgiSwapChain4);
        }
    }
}

#endif
