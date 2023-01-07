// Copyright Chad Engler

#pragma once

#include "commands/command.h"

#include "imgui.h"

namespace he::editor
{
    bool LinkButton(const char* label);
    bool DialogButton(const char* label);
    float DialogButtonWidth();

    bool CommandButton(const char* label, Command& cmd);
    bool CommandLinkButton(const char* label, Command& cmd);

    bool BeginPopupMenuButton(const char* label, const char* popupId, const ImVec2& size = ImVec2(0, 0));
    void EndPopupMenuButton();
}
