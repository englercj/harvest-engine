// Copyright Chad Engler

#pragma once

#include "he/core/string.h"

#include "imgui.h"

namespace he::editor
{
    bool InputText(const char* label, String& str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* userData = nullptr);
}
