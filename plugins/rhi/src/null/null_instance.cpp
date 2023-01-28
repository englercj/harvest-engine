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

    ApiResult InstanceImpl::GetApiResult(Result result) const
    {
        HE_UNUSED(result);
        return ApiResult::Success;
    }

    void InstanceImpl::GetAdapters(uint32_t& count, const Adapter*& adapters)
    {
        count = 1;
        adapters = &m_adapter;
    }

    void InstanceImpl::GetDisplays(const Adapter& adapter, uint32_t& count, const Display*& displays)
    {
        HE_UNUSED(adapter);
        count = 1;
        displays = &m_display;
    }

    void InstanceImpl::GetDisplayModes(const Display& display, uint32_t& count, const DisplayMode*& modes)
    {
        HE_UNUSED(display);
        count = 1;
        modes = &m_displayMode;
    }

    const AdapterInfo& InstanceImpl::GetAdapterInfo(const Adapter& adapter)
    {
        HE_UNUSED(adapter);
        static AdapterInfo s_info{};
        return s_info;
    }

    const DisplayInfo& InstanceImpl::GetDisplayInfo(const Display& display)
    {
        HE_UNUSED(display);
        static DisplayInfo s_info{ m_allocator };
        return s_info;
    }

    const DisplayModeInfo& InstanceImpl::GetDisplayModeInfo(const DisplayMode& mode)
    {
        HE_UNUSED(mode);
        static DisplayModeInfo s_info{};
        return s_info;
    }

    Result InstanceImpl::CreateDevice(const DeviceDesc& desc, Device*& out)
    {
        HE_UNUSED(desc);
        out = m_allocator.New<DeviceImpl>(this);
        return Result::Success;
    }

    void InstanceImpl::DestroyDevice(Device* device)
    {
        m_allocator.Delete(device);
    }
}

#endif
