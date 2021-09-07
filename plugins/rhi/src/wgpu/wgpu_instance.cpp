// Copyright Chad Engler

#include "rhi_internal.h"

#include "he/core/log.h"
#include "he/rhi/config.h"

#if HE_RHI_ENABLE_WEBGPU

namespace he::rhi::webgpu
{

}

namespace he::rhi
{
    template <> Result _CreateInstance<ApiBackend::WebGPU>(Allocator& allocator, Instance*& instance)
    {
        HE_LOGF_ERROR(rhi, "WebGPU RHI backend is not yet implemented.");
        instance = nullptr;
        return Result::NotSupported;
    }
}

#endif
