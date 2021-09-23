// Copyright Chad Engler

#include "rhi_internal.h"

#include "he/core/log.h"
#include "he/rhi/instance.h"
#include "he/rhi/types.h"
#include "he/rhi/utils.h"

namespace he::rhi
{
    const char* AsString(ApiResult x)
    {
        switch (x)
        {
            case ApiResult::Success: return "Success";
            case ApiResult::Failure: return "Failure";
            case ApiResult::DeviceLost: return "DeviceLost";
            case ApiResult::OutOfMemory: return "OutOfMemory";
            case ApiResult::NotFound: return "NotFound";
        }

        return "<unknown>";
    }

    const char* AsString(ApiBackend x)
    {
        switch (x)
        {
            case ApiBackend::Unknown: return "Unknown";
            case ApiBackend::Null: return "NULL";
            case ApiBackend::D3D12: return "D3D12";
            case ApiBackend::Vulkan: return "Vulkan";
            case ApiBackend::WebGPU: return "WebGPU";
        }

        return "<unknown>";
    }

    Result CreateInstance(const InstanceDesc& desc, Instance*& instance)
    {
        Allocator& allocator = desc.allocator ? *desc.allocator : CrtAllocator::Get();

        instance = nullptr;

        Result result = Result::Success;

        ApiBackend api = desc.api == ApiBackend::Unknown ? GetDefaultApiBackend() : desc.api;

        switch (api)
        {
        #if HE_RHI_ENABLE_NULL
            case ApiBackend::Null:
                result = _CreateInstance<ApiBackend::Null>(allocator, instance);
                break;
        #endif
        #if HE_RHI_ENABLE_D3D12
            case ApiBackend::D3D12:
                result = _CreateInstance<ApiBackend::D3D12>(allocator, instance);
                break;
        #endif
        #if HE_RHI_ENABLE_VULKAN
            case ApiBackend::Vulkan:
                result = _CreateInstance<ApiBackend::Vulkan>(allocator, instance);
                break;
        #endif
        #if HE_RHI_ENABLE_WEBGPU
            case ApiBackend::WebGPU:
                result = _CreateInstance<ApiBackend::WebGPU>(allocator, instance);
                break;
        #endif
            default:
                HE_LOGF_ERROR(rhi, "Failed to create RHI instance. Unsupported API type: {}", desc.api);
                result = Result::InvalidParameter;
        }

        if (!result)
            return result;

        result = instance->Initialize(desc);

        if (!result)
        {
            DestroyInstance(instance);
            instance = nullptr;
            return result;
        }

        return Result::Success;
    }

    void DestroyInstance(Instance* instance)
    {
        if (instance)
        {
            instance->GetAllocator().Delete(instance);
        }
    }
}
