// Copyright Chad Engler

#pragma once

#include "d3d12_common.h"
#include "d3d12_resources.h"

#include "he/core/allocator.h"
#include "he/core/result.h"
#include "he/core/vector.h"
#include "he/rhi/config.h"
#include "he/rhi/instance.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    using Pfn_CreateDXGIFactory2 = HRESULT(WINAPI*)(UINT flags, REFIID riid, void** factory);
    using Pfn_DXGIGetDebugInterface1 = HRESULT(WINAPI*)(UINT flags, REFIID riid, void** debug);

    class InstanceImpl final : public Instance
    {
    public:
        explicit InstanceImpl(Allocator& allocator) noexcept;
        ~InstanceImpl() noexcept;

        Result Initialize(const InstanceDesc& desc) override;

        Allocator& GetAllocator() override { return m_allocator; }
        ApiResult GetApiResult(Result result) const override;

        void GetAdapters(uint32_t& count, const Adapter*& adapters) override;
        void GetDisplays(const Adapter& adapter, uint32_t& count, const Display*& displays) override;
        void GetDisplayModes(const Display& display, uint32_t& count, const DisplayMode*& modes) override;

        const AdapterInfo& GetAdapterInfo(const Adapter& adapter) override;
        const DisplayInfo& GetDisplayInfo(const Display& display) override;
        const DisplayModeInfo& GetDisplayModeInfo(const DisplayMode& mode) override;

        Result CreateDevice(const DeviceDesc& desc, Device*& out) override;
        void DestroyDevice(Device* device) override;

    private:
        void GatherAdapters();

    public:
        Allocator& m_allocator;

        Vector<AdapterImpl> m_adapters;
        uint32_t m_preferredAdapter{ 0 };

        bool m_allowTearing{ false };
        bool m_enableDebugCpu{ false };
        bool m_enableDebugBreakOnError{ false };
        bool m_enableDebugBreakOnWarning{ false };

        IDXGIFactory2* m_dxgiFactory2{ nullptr };
        IDXGIInfoQueue* m_infoQueue{ nullptr };

        // dxgi.dll
        HMODULE m_dxgiLib{ nullptr };
        Pfn_CreateDXGIFactory2 m_CreateDXGIFactory2{ nullptr };
        Pfn_DXGIGetDebugInterface1 m_DXGIGetDebugInterface1{ nullptr };

        // d3d12.dll
        HMODULE m_d3d12Lib{ nullptr };
        PFN_D3D12_CREATE_DEVICE m_D3D12CreateDevice{ nullptr };
        PFN_D3D12_GET_DEBUG_INTERFACE m_D3D12GetDebugInterface{ nullptr };
        PFN_D3D12_SERIALIZE_ROOT_SIGNATURE m_D3D12SerializeRootSignature{ nullptr };
        PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE m_D3D12SerializeVersionedRootSignature{ nullptr };
    };
}

#endif
