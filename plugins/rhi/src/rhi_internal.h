// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/rhi/types.h"

namespace he::rhi
{
    // Forward declare of the api-specific instance creation function
    template <ApiBackend> Result _CreateInstance(Allocator& allocator, Instance*& instance);
}
