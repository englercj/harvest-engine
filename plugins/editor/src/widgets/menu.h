// Copyright Chad Engler

#pragma once

#include "commands/command.h"

#include "he/window/view.h"

#include "imgui.h"

namespace he::editor
{
    void TopLevelIcon();

    // Status Bar

    bool BeginAppStatusBar();
    void EndAppStatusBar();

    bool StatusBarButton(const char* label, float width = 0.0f, ImGuiButtonFlags flags = ImGuiButtonFlags_None);

    // Main Menu bar

    bool BeginAppMainMenuBar();
    void EndAppMainMenuBar();

    bool BeginTopLevelMenu(const char* label, bool enabled = true);
    void EndTopLevelMenu();

    // Vertical menu for use anywhere (like in a popup)

    bool BeginPopupMenu(const char* popupId);
    void EndPopupMenu();

    // General menu items

    bool BeginMenu(const char* label, const char* icon = nullptr, bool enabled = true);
    void EndMenu();

    bool MenuItem(const char* label, const char* icon = nullptr, const char* shortcut = nullptr, bool selected = false, bool enabled = true);
    bool MenuItem(Command& cmd, bool selected = false);
    void MenuSeparator(const char* label = nullptr);

    void MenuSystemButtons(window::View* view, window::ViewHitArea& hitArea);

    // Dock tabs

    bool BeginDockTabContextMenu(const char* id);
    void EndDockTabContextMenu();
}
