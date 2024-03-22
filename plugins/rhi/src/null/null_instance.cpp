// Copyright Chad Engler

#include "null_instance.h"

#include "null_device.h"
#include "null_cmd_list.h"
#include "null_instance.h"

#include "he/core/assert.h"
#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    InstanceImpl::InstanceImpl(Allocator& allocator) noexcept
        : m_allocator(allocator)
    {}

    Result InstanceImpl::Initialize(const InstanceDesc& desc)
    {
        HE_ASSERT(desc.allocator == nullptr || desc.allocator == &m_allocator);
        return Result::Success;
    }

    Allocator& InstanceImpl::GetAllocator()
    {
        return m_allocator;
    }

    StringView InstanceImpl::ApiName() const
    {
        return Api_Null;
    }

    ApiResult InstanceImpl::GetApiResult([[maybe_unused]] Result result) const
    {
        return ApiResult::Success;
    }

    void InstanceImpl::GetAdapters(uint32_t& count, const Adapter*& adapters)
    {
        count = 1;
        adapters = &m_adapter;
    }

    void InstanceImpl::GetDisplays([[maybe_unused]] const Adapter& adapter, uint32_t& count, const Display*& displays)
    {
        count = 1;
        displays = &m_display;
    }

    void InstanceImpl::GetDisplayModes([[maybe_unused]] const Display& display, uint32_t& count, const DisplayMode*& modes)
    {
        count = 1;
        modes = &m_displayMode;
    }

    const AdapterInfo& InstanceImpl::GetAdapterInfo([[maybe_unused]] const Adapter& adapter)
    {
        static AdapterInfo s_info{};
        return s_info;
    }

    const DisplayInfo& InstanceImpl::GetDisplayInfo([[maybe_unused]] const Display& display)
    {
        static DisplayInfo s_info{ m_allocator };
        return s_info;
    }

    const DisplayModeInfo& InstanceImpl::GetDisplayModeInfo([[maybe_unused]] const DisplayMode& mode)
    {
        static DisplayModeInfo s_info{};
        return s_info;
    }

    Result InstanceImpl::CreateDevice([[maybe_unused]] const DeviceDesc& desc, Device*& out)
    {
        out = m_allocator.New<DeviceImpl>(this);
        return Result::Success;
    }

    void InstanceImpl::DestroyDevice(Device* device)
    {
        m_allocator.Delete(device);
    }
}

#endif
