// Copyright Chad Engler

#pragma once

#include "commands/command.h"

#include "imgui.h"

namespace he::editor
{
    constexpr float UnscaledDialogButtonWidth = 80.0f;

    bool LinkButton(const char* label);
    bool DialogButton(const char* label);

    bool CommandButton(const char* label, Command& cmd);
    bool CommandLinkButton(const char* label, Command& cmd);
}
