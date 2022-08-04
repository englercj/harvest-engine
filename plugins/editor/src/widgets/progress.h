// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

#include "imgui.h"

namespace he::editor
{
    void ProgressSpinner(float thickness = 5.0f, float radius = 0.0f);
    void ProgressBar(float value, const ImVec2& size_arg = ImVec2(-FLT_MIN, 0), const char* overlay = nullptr);
}
