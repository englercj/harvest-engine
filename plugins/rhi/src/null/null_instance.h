// Copyright Chad Engler

#pragma once

#include "he/rhi/config.h"
#include "he/rhi/instance.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    class DeviceImpl;

    class InstanceImpl final : public Instance
    {
    public:
        explicit InstanceImpl(Allocator& allocator) noexcept;

        Result Initialize(const InstanceDesc& desc) override;

        Allocator& GetAllocator() override;
        ApiResult GetApiResult(Result result) const override;

        void GetAdapters(uint32_t& count, const Adapter*& adapters) override;
        void GetDisplays(const Adapter& adapter, uint32_t& count, const Display*& displays) override;
        void GetDisplayModes(const Display& display, uint32_t& count, const DisplayMode*& modes) override;

        const AdapterInfo& GetAdapterInfo(const Adapter& adapter) override;
        const DisplayInfo& GetDisplayInfo(const Display& display) override;
        const DisplayModeInfo& GetDisplayModeInfo(const DisplayMode& mode) override;

        Result CreateDevice(const DeviceDesc& desc, Device*& out) override;
        void DestroyDevice(Device* device) override;

    public:
        Allocator& m_allocator;

        struct AdapterImpl : Adapter {} m_adapter;
        struct DisplayImpl : Display {} m_display;
        struct DisplayModeImpl : DisplayMode {} m_displayMode;
    };
}

#endif
