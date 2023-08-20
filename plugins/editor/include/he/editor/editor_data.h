// Copyright Chad Engler

#pragma once

#include "he/core/args.h"
#include "he/core/vector.h"

namespace he::window { class Device; }

namespace he::editor
{
    struct EditorData
    {
        EditorData() = default;
        window::Device* device{ nullptr };
    };
}
