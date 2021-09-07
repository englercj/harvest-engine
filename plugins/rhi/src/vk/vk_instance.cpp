// Copyright Chad Engler

#include "rhi_internal.h"

#include "he/core/log.h"
#include "he/rhi/config.h"

#if HE_RHI_ENABLE_VULKAN

namespace he::rhi::vulkan
{

}

namespace he::rhi
{
    template <> Result _CreateInstance<ApiBackend::Vulkan>(Allocator& allocator, Instance*& instance)
    {
        HE_UNUSED(allocator);
        HE_LOGF_ERROR(rhi, "Vulkan RHI backend is not yet implemented.");
        instance = nullptr;
        return Result::NotSupported;
    }
}

#endif
