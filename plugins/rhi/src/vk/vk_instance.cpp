// Copyright Chad Engler

#include "rhi_internal.h"

#include "he/core/log.h"
#include "he/rhi/config.h"

#if HE_RHI_ENABLE_VULKAN

namespace he::rhi::vk
{

}

namespace he::rhi
{
    template <> Result _CreateInstance<ApiBackend::Vulkan>(Allocator& allocator, Instance*& instance)
    {
        HE_LOGF_INFO(rhi, "Initializing Vulkan rendering backend.");
        instance = allocator.New<vk::InstanceImpl>(allocator);
        return Result::Success;
    }
}

#endif
