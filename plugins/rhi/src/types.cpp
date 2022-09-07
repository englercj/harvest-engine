// Copyright Chad Engler

#include "he/core/enum_ops.h"
#include "he/rhi/types.h"

namespace he
{
    template <>
    const char* AsString(rhi::ApiResult x)
    {
        switch (x)
        {
            case rhi::ApiResult::Success: return "Success";
            case rhi::ApiResult::Failure: return "Failure";
            case rhi::ApiResult::DeviceLost: return "DeviceLost";
            case rhi::ApiResult::OutOfMemory: return "OutOfMemory";
            case rhi::ApiResult::NotFound: return "NotFound";
        }

        return "<unknown>";
    }
}
