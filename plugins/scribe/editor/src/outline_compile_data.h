// Copyright Chad Engler

#pragma once

#include "he/scribe/schema_types.h"

namespace he::scribe::editor
{
    struct CompiledOutlinePoint
    {
        float x{ 0.0f };
        float y{ 0.0f };
    };

    struct CompiledOutlineCommand
    {
        OutlineCommandType type{ OutlineCommandType::MoveTo };
        uint32_t firstPoint{ 0 };
    };
}
