// Copyright Chad Engler

#include "d3d12_instance.h"

#include "d3d12_device.h"
#include "d3d12_formats.h"
#include "rhi_internal.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string_fmt.h"
#include "he/math/types_fmt.h"

#if HE_RHI_ENABLE_D3D12

// avoid linking dxguid.lib
#define HE_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

HE_DEFINE_GUID(DXGI_DEBUG_ALL, 0xe48ae283, 0xda80, 0x490b, 0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8);
HE_DEFINE_GUID(DXGI_DEBUG_DXGI, 0x25cddaa4, 0xb1c6, 0x47e1, 0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a);

namespace he::rhi::d3d12
{
    InstanceImpl::InstanceImpl(Allocator& allocator)
        : m_allocator(allocator)
        , m_adapters(allocator)
    { }

    InstanceImpl::~InstanceImpl()
    {
        HE_DX_SAFE_RELEASE(m_dxgiFactory2);
        HE_DX_SAFE_RELEASE(m_infoQueue);

        IDXGIDebug1* dxgiDebug1 = nullptr;
        if (SUCCEEDED(m_DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
        {
            OutputDebugStringW(L"D3D12 Live Objects:\n");
            dxgiDebug1->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            HE_DX_SAFE_RELEASE(dxgiDebug1);
        }

        if (m_dxgiLib)
            FreeLibrary(m_dxgiLib);

        if (m_d3d12Lib)
            FreeLibrary(m_d3d12Lib);
    }

    Result InstanceImpl::Initialize(const InstanceDesc& desc)
    {
        HE_ASSERT(desc.allocator == nullptr || desc.allocator == &m_allocator);

        // Load dxgi.dll and pointers
        m_dxgiLib = LoadLibraryW(L"dxgi.dll");
        if (!m_dxgiLib)
        {
            HE_LOGF_ERROR(he_rhi, "D3D12 intialization error: Failed to load dxgi.dll library.");
            return Result::FromLastError();
        }

        m_CreateDXGIFactory2 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(m_dxgiLib, "CreateDXGIFactory2"));
        if (!m_CreateDXGIFactory2)
        {
            HE_LOGF_ERROR(he_rhi, "D3D12 intialization error: Failed to get CreateDXGIFactory2 proc.");
            return Result::FromLastError();
        }

        m_DXGIGetDebugInterface1 = reinterpret_cast<PFN_DXGI_GET_DEBUG_INTERFACE1>(GetProcAddress(m_dxgiLib, "DXGIGetDebugInterface1"));

        // Load d3d12.dll and pointers
        m_d3d12Lib = LoadLibraryW(L"d3d12.dll");
        if (!m_d3d12Lib)
        {
            HE_LOGF_ERROR(he_rhi, "D3D12 intialization error: Failed to load d3d12.dll library.");
            return Result::FromLastError();
        }

        m_D3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(m_d3d12Lib, "D3D12CreateDevice"));
        if (!m_D3D12CreateDevice)
        {
            HE_LOGF_ERROR(he_rhi, "D3D12 intialization error: Failed to get D3D12CreateDevice proc.");
            return Result::FromLastError();
        }

        m_D3D12SerializeRootSignature = reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(GetProcAddress(m_d3d12Lib, "D3D12SerializeRootSignature"));
        m_D3D12SerializeVersionedRootSignature = reinterpret_cast<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>(GetProcAddress(m_d3d12Lib, "D3D12SerializeVersionedRootSignature"));
        if (!m_D3D12SerializeRootSignature && !m_D3D12SerializeVersionedRootSignature)
        {
            HE_LOGF_ERROR(he_rhi, "D3D12 initialization error: Failed to get D3D12SerializeRootSignature or D3D12SerializeVersionedRootSignature proc.");
            return Result::FromLastError();
        }

        m_D3D12GetDebugInterface = reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(GetProcAddress(m_d3d12Lib, "D3D12GetDebugInterface"));

        // Create the DXGI factory
        UINT flags = 0;
        if (desc.enableDebugCpu)
            flags |= DXGI_CREATE_FACTORY_DEBUG;

        HRESULT hr = m_CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_dxgiFactory2));
        if (FAILED(hr))
        {
            Result r = Win32Result(hr);
            HE_LOGF_ERROR(he_rhi, "D3D12 intialization error: CreateDXGIFactory2 failed with error: {}", r);
            return r;
        }

        // Enumerate adapters
        GatherAdapters();

        // DXGI debug features
        if (desc.enableDebugCpu)
        {
            if (m_DXGIGetDebugInterface1)
            {
                IDXGIDebug1* dxgiDebug1 = nullptr;
                if (SUCCEEDED(m_DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
                {
                    if (SUCCEEDED(dxgiDebug1->QueryInterface(IID_PPV_ARGS(&m_infoQueue))))
                    {
                        m_infoQueue->SetMessageCountLimit(DXGI_DEBUG_DXGI, ~0ull);

                        if (desc.enableDebugBreakOnError)
                        {
                            m_infoQueue->SetBreakOnSeverity(DXGI_DEBUG_DXGI, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
                            m_infoQueue->SetBreakOnSeverity(DXGI_DEBUG_DXGI, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                        }

                        if (desc.enableDebugBreakOnWarning)
                        {
                            m_infoQueue->SetBreakOnSeverity(DXGI_DEBUG_DXGI, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
                        }
                    }
                    HE_DX_SAFE_RELEASE(dxgiDebug1);
                }
            }
        }

        m_enableDebugCpu = desc.enableDebugCpu;
        m_enableDebugBreakOnError = desc.enableDebugBreakOnError;
        m_enableDebugBreakOnWarning = desc.enableDebugBreakOnWarning;

        // D3D12 debug features
        if (m_D3D12GetDebugInterface && (desc.enableDebugCpu || desc.enableDebugGpu))
        {
            ID3D12Debug* d3d12Debug = nullptr;
            if (SUCCEEDED(m_D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
            {
                if (desc.enableDebugCpu)
                {
                    d3d12Debug->EnableDebugLayer();
                }

                if (desc.enableDebugGpu)
                {
                    ID3D12Debug1* d3d12Debug1 = nullptr;
                    if (SUCCEEDED(d3d12Debug->QueryInterface(IID_PPV_ARGS(&d3d12Debug1))))
                    {
                        d3d12Debug1->SetEnableGPUBasedValidation(TRUE);
                        HE_DX_SAFE_RELEASE(d3d12Debug1);
                    }
                }
                HE_DX_SAFE_RELEASE(d3d12Debug);
            }
        }

        BOOL allowTearing = FALSE;
        IDXGIFactory5* dxgiFactory5 = nullptr;
        if (SUCCEEDED(m_dxgiFactory2->QueryInterface(IID_PPV_ARGS(&dxgiFactory5))))
        {
            if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            {
                m_allowTearing = !!allowTearing;
            }

            HE_DX_SAFE_RELEASE(dxgiFactory5);
        }

        return Result::Success;
    }

    ApiResult InstanceImpl::GetApiResult(Result result) const
    {
        switch (result.GetCode())
        {
            case S_OK:
                return ApiResult::Success;
            case ERROR_NOT_ENOUGH_MEMORY:
            case ERROR_OUTOFMEMORY:
            case E_OUTOFMEMORY:
                return ApiResult::OutOfMemory;
            case DXGI_ERROR_DEVICE_REMOVED:
            case DXGI_ERROR_DEVICE_HUNG:
            case DXGI_ERROR_DEVICE_RESET:
            case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
            case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
            case DXGI_ERROR_INVALID_CALL:
                return ApiResult::DeviceLost;
            case DXGI_ERROR_NOT_FOUND:
                return ApiResult::NotFound;
        }

        return ApiResult::Failure;
    }

    void InstanceImpl::GetAdapters(uint32_t& count, const Adapter*& adapters)
    {
        count = static_cast<uint32_t>(m_adapters.Size());
        adapters = m_adapters.Data();
    }

    void InstanceImpl::GetDisplays(const Adapter& adapter_, uint32_t& count, const Display*& displays)
    {
        const AdapterImpl& adapter = static_cast<const AdapterImpl&>(adapter_);

        count = static_cast<uint32_t>(adapter.displays.Size());
        displays = adapter.displays.Data();
    }

    void InstanceImpl::GetDisplayModes(const Display& display_, uint32_t& count, const DisplayMode*& modes)
    {
        const DisplayImpl& display = static_cast<const DisplayImpl&>(display_);

        count = static_cast<uint32_t>(display.modes.Size());
        modes = display.modes.Data();
    }

    const AdapterInfo& InstanceImpl::GetAdapterInfo(const Adapter& adapter)
    {
        return static_cast<const AdapterImpl&>(adapter).info;
    }

    const DisplayInfo& InstanceImpl::GetDisplayInfo(const Display& display)
    {
        return static_cast<const DisplayImpl&>(display).info;
    }

    const DisplayModeInfo& InstanceImpl::GetDisplayModeInfo(const DisplayMode& mode)
    {
        return static_cast<const DisplayModeImpl&>(mode).info;
    }

    Result InstanceImpl::CreateDevice(const DeviceDesc& desc, Device*& out)
    {
        out = nullptr;

        uint32_t adapterIndex = 0;

        // Choose adapter
        IDXGIAdapter1* adapter1 = nullptr;
        if (desc.luid == 0)
        {
            HRESULT hr = m_dxgiFactory2->EnumAdapters1(m_preferredAdapter, &adapter1);
            if (FAILED(hr))
                return Win32Result(hr);
        }
        else
        {
            IDXGIAdapter1* candidateAdapter1 = nullptr;
            for (uint32_t i = 0; m_dxgiFactory2->EnumAdapters1(i, &candidateAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
                candidateAdapter1->GetDesc1(&dxgiAdapterDesc1);

                if (MemEqual(&desc.luid, &dxgiAdapterDesc1.AdapterLuid, sizeof(uint64_t)))
                {
                    adapterIndex = i;
                    adapter1 = candidateAdapter1;
                    break;
                }

                HE_DX_SAFE_RELEASE(candidateAdapter1);
            }
        }

        if (!adapter1)
            return Win32Result(ERROR_NOT_FOUND);

        HE_AT_SCOPE_EXIT([&]() { HE_DX_SAFE_RELEASE(adapter1); });

        // Create the device
        D3D_FEATURE_LEVEL featureLevels[] =
        {
        //
        #if defined(NTDDI_WIN10_FE) && NTDDI_VERSION >= NTDDI_WIN10_FE
            D3D_FEATURE_LEVEL_12_2,
        #endif
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        HRESULT hr = S_OK;
        ID3D12Device* d3dDevice = nullptr;

        for (uint32_t i = 0; i < HE_LENGTH_OF(featureLevels); ++i)
        {
            const D3D_FEATURE_LEVEL flevel = featureLevels[i];
            hr = m_D3D12CreateDevice(adapter1, flevel, IID_PPV_ARGS(&d3dDevice));
            if (SUCCEEDED(hr))
            {
                HE_LOGF_TRACE(he_rhi, "D3D12 device created at level: {}.{}", (flevel >> 12) & 0xf, (flevel >> 7) & 0xf);
                uint32_t numNodes = d3dDevice->GetNodeCount();
                HE_LOGF_TRACE(he_rhi, "    GPU Architecture (num nodes: {}):", numNodes);
                for (uint32_t j = 0; j < numNodes; ++j)
                {
                    D3D12_FEATURE_DATA_ARCHITECTURE arch;
                    arch.NodeIndex = j;
                    hr = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch));
                    HE_ASSERT(SUCCEEDED(hr));
                    HE_LOGF_TRACE(he_rhi, "        Node #{}: TileBasedRenderer {}, UMA {}, CacheCoherentUMA {}", j, arch.TileBasedRenderer, arch.UMA, arch.CacheCoherentUMA);
                }
                break;
            }
        }

        if (FAILED(hr))
            return Win32Result(hr);

        HE_ASSERT(d3dDevice);
        HE_DX_SET_NAME(d3dDevice, desc.name);

        ID3D12InfoQueue* d3dInfoQueue = nullptr;
        if (SUCCEEDED(d3dDevice->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
        {
            d3dInfoQueue->SetMessageCountLimit(~0ull);

            if (m_enableDebugBreakOnError)
            {
                d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            }

            if (m_enableDebugBreakOnWarning)
            {
                d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
            }
            HE_DX_SAFE_RELEASE(d3dInfoQueue);
        }

        // Initialize the device impl
        DeviceImpl* device = m_allocator.New<DeviceImpl>();
        Result result = device->Initialize(this, d3dDevice, &m_adapters[adapterIndex], desc);
        if (!result)
        {
            m_allocator.Delete(device);
            return result;
        }

        out = device;
        return Result::Success;
    }

    void InstanceImpl::DestroyDevice(Device* device)
    {
        m_allocator.Delete(device);
    }

    void InstanceImpl::GatherAdapters()
    {
        m_adapters.Clear();
        m_preferredAdapter = 0;

        IDXGIAdapter1* dxgiAdapter1 = nullptr;
        for (uint32_t i = 0; m_dxgiFactory2->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            AdapterImpl& adapter = m_adapters.EmplaceBack(m_allocator);
            AdapterInfo& adapterInfo = adapter.info;
            adapterInfo.vendorId = static_cast<AdapterVendor>(dxgiAdapterDesc1.VendorId);
            adapterInfo.deviceId = static_cast<uint16_t>(dxgiAdapterDesc1.DeviceId);
            adapterInfo.dedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
            adapterInfo.dedicatedSystemMemory = dxgiAdapterDesc1.DedicatedSystemMemory;
            adapterInfo.sharedSystemMemory = dxgiAdapterDesc1.SharedSystemMemory;

            static_assert(sizeof(AdapterInfo::luid) == sizeof(DXGI_ADAPTER_DESC1::AdapterLuid), "");
            MemCopy(&adapterInfo.luid, &dxgiAdapterDesc1.AdapterLuid, sizeof(adapterInfo.luid));

            char description[256];
            WCToMBStr(description, HE_LENGTH_OF(description), dxgiAdapterDesc1.Description);

            HE_LOGF_TRACE(he_rhi, "Adapter #{} ({:#018x}) {}", i, adapterInfo.luid, description);
            HE_LOGF_TRACE(he_rhi, "    VendorId: {:#010x}, DeviceId: {:#010x}, SubSysId: {:#010x}, Revision: {:#010x}",
                dxgiAdapterDesc1.VendorId, dxgiAdapterDesc1.DeviceId, dxgiAdapterDesc1.SubSysId, dxgiAdapterDesc1.Revision);
            HE_LOGF_TRACE(he_rhi, "    Video Memory: {}, System Memory: {}, Shared Memory: {}",
                dxgiAdapterDesc1.DedicatedVideoMemory, dxgiAdapterDesc1.DedicatedSystemMemory, dxgiAdapterDesc1.SharedSystemMemory);

            const bool isHardwareAdapter = (dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
            if (isHardwareAdapter)
            {
                AdapterImpl& preferred = m_adapters[m_preferredAdapter];
                if (dxgiAdapterDesc1.DedicatedVideoMemory > preferred.info.dedicatedVideoMemory)
                    m_preferredAdapter = i;
            }

            IDXGIOutput* dxgiOutput = nullptr;
            for (uint32_t j = 0; dxgiAdapter1->EnumOutputs(j, &dxgiOutput) != DXGI_ERROR_NOT_FOUND; ++j)
            {
                DisplayImpl& display = adapter.displays.EmplaceBack(m_allocator);

                DXGI_OUTPUT_DESC dxgiOutputDesc{};
                dxgiOutput->GetDesc(&dxgiOutputDesc);

                if (!dxgiOutputDesc.AttachedToDesktop)
                    continue;

                IDXGIOutput6* dxgiOutput6 = nullptr;
                if (SUCCEEDED(dxgiOutput->QueryInterface(IID_PPV_ARGS(&dxgiOutput6))))
                {
                    DXGI_OUTPUT_DESC1 dxgiOutputDesc1{};
                    if (SUCCEEDED(dxgiOutput6->GetDesc1(&dxgiOutputDesc1)))
                    {
                        display.info.colorSpace = FromDxColorSpace(dxgiOutputDesc1.ColorSpace);
                    }
                    HE_DX_SAFE_RELEASE(dxgiOutput6);
                }

                WCToMBStr(display.info.name, dxgiOutputDesc.DeviceName);

                RECT& r = dxgiOutputDesc.DesktopCoordinates;
                display.info.pos = { r.left, r.top };
                display.info.size = { r.right - r.left, r.bottom - r.top };

                HE_LOGF_TRACE(he_rhi, "    Display #{}", j);
                HE_LOGF_TRACE(he_rhi, "        Name: {}", display.info.name);
                HE_LOGF_TRACE(he_rhi, "        Position: {}", display.info.pos);
                HE_LOGF_TRACE(he_rhi, "        Size: {}", display.info.size);
                HE_LOGF_TRACE(he_rhi, "        AttachedToDesktop: {}", dxgiOutputDesc.AttachedToDesktop);
                HE_LOGF_TRACE(he_rhi, "        Rotation: {}", dxgiOutputDesc.Rotation);

                // Must match ToDxSwapChainViewFormat/FromDxSwapChainViewFormat in formats.cpp
                constexpr DXGI_FORMAT SupportedSwapChainViewFormats[]
                {
                    DXGI_FORMAT_R8G8B8A8_UNORM,
                    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
                    DXGI_FORMAT_R16G16B16A16_FLOAT,
                    DXGI_FORMAT_R10G10B10A2_UNORM,
                };

                for (uint32_t k = 0; k < HE_LENGTH_OF(SupportedSwapChainViewFormats); ++k)
                {
                    DXGI_FORMAT format = SupportedSwapChainViewFormats[k];
                    UINT count = 0;
                    dxgiOutput->GetDisplayModeList(format, DXGI_ENUM_MODES_INTERLACED, &count, nullptr);

                    if (count == 0)
                        continue;

                    DXGI_MODE_DESC* dxgiModeDescs = m_allocator.Malloc<DXGI_MODE_DESC>(count);
                    dxgiOutput->GetDisplayModeList(format, DXGI_ENUM_MODES_INTERLACED, &count, dxgiModeDescs);

                    for (uint32_t w = 0; w < count; ++w)
                    {
                        DXGI_MODE_DESC& dxgiModeDesc = dxgiModeDescs[w];
                        DXGI_RATIONAL& rr = dxgiModeDesc.RefreshRate;

                        DisplayModeImpl& displayMode = display.modes.EmplaceBack();
                        displayMode.info.resolution = { static_cast<int>(dxgiModeDesc.Width), static_cast<int>(dxgiModeDesc.Height) };
                        displayMode.info.refreshRate = rr.Numerator == 0 ? 0.0f : (rr.Numerator / static_cast<float>(rr.Denominator));
                        displayMode.info.format = FromDxSwapChainViewFormat(format);
                    }

                    m_allocator.Free(dxgiModeDescs);
                }

                HE_DX_SAFE_RELEASE(dxgiOutput);
            }

            HE_DX_SAFE_RELEASE(dxgiAdapter1);
        }
    }
}

namespace he::rhi
{
    template <> Result _CreateInstance<ApiBackend::D3D12>(Allocator& allocator, Instance*& instance)
    {
        HE_LOGF_INFO(he_rhi, "Initializing D3D12 rendering backend.");
        instance = allocator.New<d3d12::InstanceImpl>(allocator);
        return Result::Success;
    }
}

#endif
