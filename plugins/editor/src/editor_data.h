// Copyright Chad Engler

#pragma once

namespace he::window { class Device; }

namespace he::editor
{
    struct EditorData
    {
        int argc{ 0 };
        char** argv{ nullptr };

        window::Device* device{ nullptr };
    };
}
