// Copyright Chad Engler

#pragma once

#include "imgui.h"

namespace he::editor
{
    constexpr float UnscaledDialogButtonWidth = 80.0f;

    bool LinkButton(const char* label);
    bool ConditionButton(const char* label, bool enabled = true, const ImVec2& size = ImVec2(0, 0));
    bool DialogButton(const char* label, bool enabled = true);
}
