// Copyright Chad Engler

#pragma once

namespace he::window { class Device; }

namespace he::editor
{
    struct EditorData
    {
        EditorData() = default;

        int argc{ 0 };
        char** argv{ nullptr };

        window::Device* device{ nullptr };
    };
}
