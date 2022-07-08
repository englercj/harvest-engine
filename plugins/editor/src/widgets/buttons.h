// Copyright Chad Engler

#pragma once

#include "imgui.h"

namespace he::editor
{
    constexpr float UnscaledDialogButtonWidth = 80.0f;

    bool LinkButton(const char* label);
    bool DialogButton(const char* label);
}
