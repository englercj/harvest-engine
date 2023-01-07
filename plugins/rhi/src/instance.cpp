// Copyright Chad Engler

#include "he/core/log.h"
#include "he/core/result_fmt.h"
#include "he/rhi/instance.h"
#include "he/rhi/types.h"
#include "he/rhi/utils.h"

namespace he::rhi
{
    extern Instance* _CreateRhiInstance(StringView api, Allocator& allocator);

    Result Instance::Create(const InstanceDesc& desc, Instance*& instance)
    {
        instance = nullptr;
        Allocator& allocator = desc.allocator ? *desc.allocator : Allocator::GetDefault();

        instance = _CreateRhiInstance(desc.api, allocator);
        if (!instance)
        {
            HE_LOG_ERROR(he_rhi, HE_MSG("Failed to create RHI instance. Unsupported API type."), HE_KV(api, desc.api));
            return Result::NotSupported;
        }

        const Result result = instance->Initialize(desc);
        if (!result)
        {
            HE_LOG_ERROR(he_rhi, HE_MSG("Failed to initialize RHI instance."), HE_KV(api, instance->ApiName()), HE_KV(result, result));
            Instance::Destroy(instance);
            instance = nullptr;
            return result;
        }

        HE_LOG_INFO(he_rhi, HE_MSG("Created backend instance for RHI."), HE_KV(api, instance->ApiName()));
        return Result::Success;
    }

    void Instance::Destroy(Instance* instance)
    {
        if (instance)
        {
            instance->GetAllocator().Delete(instance);
        }
    }
}
