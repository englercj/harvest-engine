// Copyright Chad Engler

#pragma once

#include "he/scribe/schema/runtime_blob.hsc.h"

#include "he/core/types.h"

namespace he::scribe
{
    // M0 format marker for the first Harvest-owned compiled font/vector blob schema.
    inline constexpr uint32_t RuntimeBlobFormatVersion = 1;
}
