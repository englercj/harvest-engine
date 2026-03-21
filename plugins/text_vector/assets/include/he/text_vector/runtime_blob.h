// Copyright Chad Engler

#pragma once

#include "he/text_vector/schema/runtime_blob.hsc.h"

#include "he/core/types.h"

namespace he::text_vector
{
    // M0 format marker for the first Harvest-owned compiled font/vector blob schema.
    inline constexpr uint32_t RuntimeBlobFormatVersion = 1;
}
