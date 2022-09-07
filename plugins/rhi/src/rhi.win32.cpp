// Copyright Chad Engler

#include "d3d12/d3d12_instance.h"
#include "null/null_instance.h"

#include "he/core/allocator.h"
#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if defined(HE_PLATFORM_API_WIN32)

namespace he::rhi
{
    Instance* _CreateRhiInstance(StringView api, Allocator& allocator)
    {
    #if HE_RHI_ENABLE_D3D12
        if (api.IsEmpty() || api == Api_D3D12)
            return allocator.New<d3d12::InstanceImpl>(allocator);
    #endif

    #if HE_RHI_ENABLE_NULL
        if (api == Api_Null)
            return allocator.New<null::InstanceImpl>(allocator);
    #endif

        return nullptr;
    }
}

#endif
