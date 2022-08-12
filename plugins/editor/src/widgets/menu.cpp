// Copyright Chad Engler

#include "menu.h"

#include "fonts/icons_material_design.h"
#include "services/imgui_service.h"

#include "he/core/assert.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    static const ImVec2 TopLevelIconFramePadding = { 8.0f, 0.0f };
    static const ImVec2 TopLevelMenuItemPadding = { 10.0f, 16.0f };
    static const ImVec2 MenuWindowPadding = { 0.0f, 10.0f };
    static const ImVec2 MenuItemSpacing = { 10.0f, 12.0f };
    static const ImVec2 StatusItemPadding = { 6.0f, 8.0f };
    static const float MenuItemIconWidth = 16.0f;
    static const float MenuSysButtonWidth = 45.0f;

    void TopLevelIcon()
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext& g = *GImGui;
        ImGuiStyle& style = g.Style;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, TopLevelIconFramePadding * g.CurrentDpiScale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_Button]);

        ImGui::Button(ICON_MDI_GRASS, ImVec2(28.0f * g.CurrentDpiScale, -1));

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);
    }

    bool BeginAppStatusBar()
    {
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        const float height = ImGui::GetFrameHeight();

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::GetStyle().Colors[ImGuiCol_Header]);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);

        const bool sidebarOpen = ImGui::BeginViewportSideBar("##AppStatusBar", nullptr, ImGuiDir_Down, height, flags);

        if (!sidebarOpen)
        {
            ImGui::End();
            ImGui::PopStyleColor(2);
            return false;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        const bool menubarOpen = ImGui::BeginMenuBar();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, StatusItemPadding * ImGui::GetWindowDpiScale());
        if (!menubarOpen)
        {
            ImGui::PopStyleVar(3);
            ImGui::End();
            ImGui::PopStyleColor(2);
            return false;
        }

        return menubarOpen;
    }

    void EndAppStatusBar()
    {
        ImGui::EndMenuBar();
        ImGui::PopStyleVar(3);
        ImGui::End();
        ImGui::PopStyleColor(2);
    }

    bool StatusBarButton(const char* label, const ImVec2& size, ImGuiButtonFlags flags)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Header]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_HeaderActive]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_HeaderHovered]);
        const bool clicked = ImGui::ButtonEx(label, size, flags);

        ImGui::PopStyleColor(3);

        return clicked;
    }

    bool BeginAppMainMenuBar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        const bool open = ImGui::BeginMainMenuBar();

        if (!open)
        {
            ImGui::PopStyleVar();
        }

        return open;
    }

    void EndAppMainMenuBar()
    {
        ImGui::EndMainMenuBar();
        ImGui::PopStyleVar();
    }

    bool BeginTopLevelMenu(const char* label, bool enabled)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, TopLevelMenuItemPadding * g.CurrentDpiScale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, MenuWindowPadding * g.CurrentDpiScale);
        const bool open = BeginMenu(label, nullptr, enabled);
        ImGui::PopStyleVar();

        if (open)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, MenuItemSpacing * g.CurrentDpiScale);
        }
        else
        {
            ImGui::PopStyleVar();
        }

        return open;
    }

    void EndTopLevelMenu()
    {
        ImGui::PopStyleVar(2);
        EndMenu();
    }

    static bool ImGuiBeginMenuEx(const char* label, const char* icon, bool enabled)
    {
        using namespace ImGui;

        // //////////
        // This is a copy-paste of the ImGui::BeginMenuEx implementation with a few modifications:
        // 1) ImGuiSelectableFlags_SpanAllColumns flag is added to the selectable for vertical menus.
        // 2) The arrow for sub menus uses a FA icon instead of the render arrow.
        // //////////

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        bool menu_is_open = IsPopupOpen(id, ImGuiPopupFlags_None);

        // Sub-menus are ChildWindow so that mouse can be hovering across them (otherwise top-most popup menu would steal focus and not allow hovering on parent menu)
        ImGuiWindowFlags flags = ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;
        if (window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu))
            flags |= ImGuiWindowFlags_ChildWindow;

        // If a menu with same the ID was already submitted, we will append to it, matching the behavior of Begin().
        // We are relying on a O(N) search - so O(N log N) over the frame - which seems like the most efficient for the expected small amount of BeginMenu() calls per frame.
        // If somehow this is ever becoming a problem we can switch to use e.g. ImGuiStorage mapping key to last frame used.
        if (g.MenusIdSubmittedThisFrame.contains(id))
        {
            if (menu_is_open)
                menu_is_open = BeginPopupEx(id, flags); // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
            else
                g.NextWindowData.ClearFlags();          // we behave like Begin() and need to consume those values
            return menu_is_open;
        }

        // Tag menu as used. Next time BeginMenu() with same ID is called it will append to existing menu
        g.MenusIdSubmittedThisFrame.push_back(id);

        ImVec2 label_size = CalcTextSize(label, NULL, true);
        bool pressed;
        bool menuset_is_open = !(window->Flags & ImGuiWindowFlags_Popup) && (g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].OpenParentId == window->IDStack.back());
        ImGuiWindow* backed_nav_window = g.NavWindow;
        if (menuset_is_open)
            g.NavWindow = window;  // Odd hack to allow hovering across menus of a same menu-set (otherwise we wouldn't be able to hover parent)

                                   // The reference position stored in popup_pos will be used by Begin() to find a suitable position for the child menu,
                                   // However the final position is going to be different! It is chosen by FindBestWindowPosForPopup().
                                   // e.g. Menus tend to overlap each other horizontally to amplify relative Z-ordering.
        ImVec2 popup_pos, pos = window->DC.CursorPos;
        PushID(label);
        if (!enabled)
            BeginDisabled();
        const ImGuiMenuColumns* offsets = &window->DC.MenuColumns;
        if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
        {
            // Menu inside an horizontal menu bar
            // Selectable extend their highlight by half ItemSpacing in each direction.
            // For ChildMenu, the popup position will be overwritten by the call to FindBestWindowPosForPopup() in Begin()
            popup_pos = ImVec2(pos.x - 1.0f - IM_FLOOR(style.ItemSpacing.x * 0.5f), pos.y - style.FramePadding.y + window->MenuBarHeight());
            window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * 0.5f);
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
            float w = label_size.x;
            ImVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
            pressed = Selectable("", menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_DontClosePopups, ImVec2(w, 0.0f));
            RenderText(text_pos, label);
            PopStyleVar();
            window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
        }
        else
        {
            // Menu inside a menu
            // (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
            //  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.
            popup_pos = ImVec2(pos.x, pos.y - style.WindowPadding.y);
            float icon_w = (icon && icon[0]) ? CalcTextSize(icon, NULL).x : 0.0f;
            float checkmark_w = IM_FLOOR(g.FontSize * 1.20f);
            float min_w = window->DC.MenuColumns.DeclColumns(icon_w, label_size.x, 0.0f, checkmark_w); // Feedback to next frame
            float extra_w = ImMax(0.0f, GetContentRegionAvail().x - min_w);
            ImVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
            pressed = Selectable("", menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_SpanAllColumns, ImVec2(min_w, 0.0f));
            RenderText(text_pos, label);
            if (icon_w > 0.0f)
                RenderText(pos + ImVec2(offsets->OffsetIcon, 0.0f), icon);
            //const float arrow_w = ImGui::CalcTextSize(ICON_MDI_CHEVRON_RIGHT).x;
            //RenderArrow(window->DrawList, pos + ImVec2(offsets->OffsetMark + extra_w + g.FontSize * 0.30f, 0.0f), GetColorU32(ImGuiCol_Text), ImGuiDir_Right);
            ImGui::RenderText(pos + ImVec2((offsets->OffsetMark + extra_w + g.FontSize * 0.30f)- (style.ItemSpacing.x * 0.5f), 0.0f), ICON_MDI_CHEVRON_RIGHT, nullptr, false);
        }
        if (!enabled)
            EndDisabled();

        const bool hovered = (g.HoveredId == id) && enabled;
        if (menuset_is_open)
            g.NavWindow = backed_nav_window;

        bool want_open = false;
        bool want_close = false;
        if (window->DC.LayoutType == ImGuiLayoutType_Vertical) // (window->Flags & (ImGuiWindowFlags_Popup|ImGuiWindowFlags_ChildMenu))
        {
            // Close menu when not hovering it anymore unless we are moving roughly in the direction of the menu
            // Implement http://bjk5.com/post/44698559168/breaking-down-amazons-mega-dropdown to avoid using timers, so menus feels more reactive.
            bool moving_toward_other_child_menu = false;

            ImGuiWindow* child_menu_window = (g.BeginPopupStack.Size < g.OpenPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].SourceWindow == window) ? g.OpenPopupStack[g.BeginPopupStack.Size].Window : NULL;
            if (g.HoveredWindow == window && child_menu_window != NULL && !(window->Flags & ImGuiWindowFlags_MenuBar))
            {
                // FIXME-DPI: Values should be derived from a master "scale" factor.
                ImRect next_window_rect = child_menu_window->Rect();
                ImVec2 ta = g.IO.MousePos - g.IO.MouseDelta;
                ImVec2 tb = (window->Pos.x < child_menu_window->Pos.x) ? next_window_rect.GetTL() : next_window_rect.GetTR();
                ImVec2 tc = (window->Pos.x < child_menu_window->Pos.x) ? next_window_rect.GetBL() : next_window_rect.GetBR();
                float extra = ImClamp(ImFabs(ta.x - tb.x) * 0.30f, 5.0f, 30.0f);    // add a bit of extra slack.
                ta.x += (window->Pos.x < child_menu_window->Pos.x) ? -0.5f : +0.5f; // to avoid numerical issues
                tb.y = ta.y + ImMax((tb.y - extra) - ta.y, -100.0f);                // triangle is maximum 200 high to limit the slope and the bias toward large sub-menus // FIXME: Multiply by fb_scale?
                tc.y = ta.y + ImMin((tc.y + extra) - ta.y, +100.0f);
                moving_toward_other_child_menu = ImTriangleContainsPoint(ta, tb, tc, g.IO.MousePos);
                //GetForegroundDrawList()->AddTriangleFilled(ta, tb, tc, moving_within_opened_triangle ? IM_COL32(0,128,0,128) : IM_COL32(128,0,0,128)); // [DEBUG]
            }

            // FIXME: Hovering a disabled BeginMenu or MenuItem won't close us
            if (menu_is_open && !hovered && g.HoveredWindow == window && g.HoveredIdPreviousFrame != 0 && g.HoveredIdPreviousFrame != id && !moving_toward_other_child_menu)
                want_close = true;

            if (!menu_is_open && hovered && pressed) // Click to open
                want_open = true;
            else if (!menu_is_open && hovered && !moving_toward_other_child_menu) // Hover to open
                want_open = true;

            if (g.NavActivateId == id)
            {
                want_close = menu_is_open;
                want_open = !menu_is_open;
            }
            if (g.NavId == id && g.NavMoveDir == ImGuiDir_Right) // Nav-Right to open
            {
                want_open = true;
                NavMoveRequestCancel();
            }
        }
        else
        {
            // Menu bar
            if (menu_is_open && pressed && menuset_is_open) // Click an open menu again to close it
            {
                want_close = true;
                want_open = menu_is_open = false;
            }
            else if (pressed || (hovered && menuset_is_open && !menu_is_open)) // First click to open, then hover to open others
            {
                want_open = true;
            }
            else if (g.NavId == id && g.NavMoveDir == ImGuiDir_Down) // Nav-Down to open
            {
                want_open = true;
                NavMoveRequestCancel();
            }
        }

        if (!enabled) // explicitly close if an open menu becomes disabled, facilitate users code a lot in pattern such as 'if (BeginMenu("options", has_object)) { ..use object.. }'
            want_close = true;
        if (want_close && IsPopupOpen(id, ImGuiPopupFlags_None))
            ClosePopupToLevel(g.BeginPopupStack.Size, true);

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Openable | (menu_is_open ? ImGuiItemStatusFlags_Opened : 0));
        PopID();

        if (!menu_is_open && want_open && g.OpenPopupStack.Size > g.BeginPopupStack.Size)
        {
            // Don't recycle same menu level in the same frame, first close the other menu and yield for a frame.
            OpenPopup(label);
            return false;
        }

        menu_is_open |= want_open;
        if (want_open)
            OpenPopup(label);

        if (menu_is_open)
        {
            SetNextWindowPos(popup_pos, ImGuiCond_Always); // Note: this is super misleading! The value will serve as reference for FindBestWindowPosForPopup(), not actual pos.
            menu_is_open = BeginPopupEx(id, flags); // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
        }
        else
        {
            g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
        }

        return menu_is_open;
    }

    bool BeginMenu(const char* label, const char* icon, bool enabled)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiStyle& style = ImGui::GetStyle();

        if (window->DC.LayoutType == ImGuiLayoutType_Vertical)
        {
            window->DC.CursorPos.x += style.ItemSpacing.x;
        }

        const bool menuIsOpen = ImGuiBeginMenuEx(label, icon, enabled);

        if (menuIsOpen)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, MenuWindowPadding * ImGui::GetWindowDpiScale());

        return menuIsOpen;
    }

    void EndMenu()
    {
        ImGui::PopStyleVar();
        ImGui::EndMenu();
    }

    static bool ImGuiMenuItemEx(const char* label, const char* icon, const char* shortcut, bool selected, bool enabled)
    {
        using namespace ImGui;

        // //////////
        // This is a copy-paste of the ImGui::MenuItemEx implementation with a few modifications:
        // 1) ImGuiSelectableFlags_SpanAllColumns flag is added to the selectable for vertical menus.
        // 2) Shortcuts for vertical menus are right aligned.
        // 3) Modify text rendering for horizontal menus to center the text.
        // //////////

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        ImGuiStyle& style = g.Style;
        ImVec2 pos = window->DC.CursorPos;
        ImVec2 label_size = CalcTextSize(label, NULL, true);

        // We've been using the equivalent of ImGuiSelectableFlags_SetNavIdOnHover on all Selectable() since early Nav system days (commit 43ee5d73),
        // but I am unsure whether this should be kept at all. For now moved it to be an opt-in feature used by menus only.
        bool pressed;
        PushID(label);
        if (!enabled)
            BeginDisabled(true);
        const ImGuiSelectableFlags flags = ImGuiSelectableFlags_SelectOnRelease | ImGuiSelectableFlags_SetNavIdOnHover;
        const ImGuiMenuColumns* offsets = &window->DC.MenuColumns;
        if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
        {
            // Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
            // Note that in this situation: we don't render the shortcut, we render a highlight instead of the selected tick mark.
            float w = label_size.x;
            window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * 0.5f);
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
            pressed = Selectable("", selected, flags, ImVec2(w, 0.0f));
            PopStyleVar();
            RenderText(pos + ImVec2(offsets->OffsetLabel, 0.0f), label);
            //RenderText(pos + ImVec2(offsets->OffsetLabel + IM_FLOOR(style.ItemSpacing.x * 0.5f), IM_FLOOR(style.ItemSpacing.y * 0.5f)), label);
            window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
        }
        else
        {
            // Menu item inside a vertical menu
            // (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
            //  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.
            float icon_w = (icon && icon[0]) ? CalcTextSize(icon, NULL).x : 0.0f;
            float shortcut_w = (shortcut && shortcut[0]) ? CalcTextSize(shortcut, NULL).x : 0.0f;
            float checkmark_w = IM_FLOOR(g.FontSize * 1.20f);
            float min_w = window->DC.MenuColumns.DeclColumns(icon_w, label_size.x, shortcut_w, checkmark_w); // Feedback for next frame
            float stretch_w = ImMax(0.0f, GetContentRegionAvail().x - min_w);
            pressed = Selectable("", false, flags | ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_SpanAllColumns, ImVec2(min_w, 0.0f));
            RenderText(pos + ImVec2(offsets->OffsetLabel, 0.0f), label);
            if (icon_w > 0.0f)
                RenderText(pos + ImVec2(offsets->OffsetIcon, 0.0f), icon);
            if (shortcut_w > 0.0f)
            {
                PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
                ImGui::RenderText(pos + ImVec2(min_w - shortcut_w - style.ItemSpacing.x, 0), shortcut, nullptr, false);
                //RenderText(pos + ImVec2(offsets->OffsetShortcut + stretch_w, 0.0f), shortcut, NULL, false);
                PopStyleColor();
            }
            if (selected)
                RenderCheckMark(window->DrawList, pos + ImVec2(offsets->OffsetMark + stretch_w + g.FontSize * 0.40f, g.FontSize * 0.134f * 0.5f), GetColorU32(ImGuiCol_Text), g.FontSize  * 0.866f);
        }
        IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (selected ? ImGuiItemStatusFlags_Checked : 0));
        if (!enabled)
            EndDisabled();
        PopID();

        return pressed;
    }

    bool MenuItem(const char* label, const char* icon, const char* shortcut, bool selected, bool enabled)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiStyle& style = ImGui::GetStyle();

        if (window->DC.LayoutType == ImGuiLayoutType_Vertical)
        {
            window->DC.CursorPos.x += style.ItemSpacing.x;
        }

        return ImGuiMenuItemEx(label, icon, shortcut, selected, enabled);
    }

    bool MenuItem(Command& cmd, bool selected)
    {
        if (MenuItem(cmd.Label(), cmd.Icon(), cmd.Shortcut(), selected, cmd.CanRun()))
        {
            cmd.Run();
            return true;
        }

        return false;
    }

    void MenuSeparator(const char* label)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext& g = *GImGui;
        ImGuiStyle& style = g.Style;
        ImVec2& pos = window->DC.CursorPos;

        const float halfSpacingY = IM_ROUND(style.ItemSpacing.y * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);

        // Only meant to be used with vertical menus currently
        HE_ASSERT(window->DC.LayoutType == ImGuiLayoutType_Vertical);

        const float cursorStartX = pos.x;

        pos.x += style.ItemSpacing.x;

        ImVec2 labelSize = label ? ImGui::CalcTextSize(label, nullptr, true) : ImVec2(0, 0);
        if (labelSize.x > 0 || labelSize.y > 0)
        {
            ImGui::RenderText(pos, label);
            pos.x += IM_FLOOR(style.ItemSpacing.x * 0.5f) + labelSize.x;
        }
        pos.y += halfSpacingY;

        const float labelSizeX = pos.x - cursorStartX;
        const float lineSizeX = window->Size.x - labelSizeX - style.ItemSpacing.x - style.WindowPadding.x;
        const float thickness = 1.0f;
        const float halfThickness = IM_FLOOR(thickness * 0.5f);

        // Horizontal Separator
        float x1 = pos.x;
        float x2 = pos.x + lineSizeX;

        // FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator
        if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
            x1 += window->DC.Indent.x;

        ImGuiOldColumns* columns = window->DC.CurrentColumns;
        if (columns)
            ImGui::PushColumnsBackground();

        pos.y += IM_ROUND(labelSize.y * 0.5f) - halfSpacingY;

        const ImRect bb(ImVec2(pos.x, pos.y - halfThickness), ImVec2(x2, pos.y + halfThickness + halfSpacingY));
        window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), ImGui::GetColorU32(ImGuiCol_Text), thickness);
        ImGui::ItemSize(bb.GetSize());

        if (columns)
        {
            ImGui::PopColumnsBackground();
            columns->LineMinY = pos.y;
        }

        ImGui::PopStyleColor();
    }

    void MenuSystemButtons(window::View* view, window::ViewHitArea& hitArea)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext& g = *GImGui;
        ImGuiStyle& style = g.Style;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_MenuBarBg]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_ButtonHovered]);

        const ImVec2 sysButtonSize{ MenuSysButtonWidth * g.CurrentDpiScale, -1 };

        const float windowWidth = ImGui::GetWindowWidth();
        ImGui::SameLine(windowWidth - (sysButtonSize.x * 3));

        // Minimize
        if (ImGui::Button(ICON_MDI_WINDOW_MINIMIZE, sysButtonSize))
            view->Minimize();
        if (ImGui::IsItemHovered())
            hitArea = window::ViewHitArea::ButtonMinimize;

        // Maximize & Restore
        if (ImGui::Button(view->IsMaximized() ? ICON_MDI_WINDOW_RESTORE : ICON_MDI_WINDOW_MAXIMIZE, sysButtonSize))
            view->ToggleMaximize();
        if (ImGui::IsItemHovered())
            hitArea = window::ViewHitArea::ButtonMaximizeAndRestore;

        // Close
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.0f, 0.0f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_ButtonHovered]);
        if (ImGui::Button(ICON_MDI_CLOSE, sysButtonSize))
            view->RequestClose();
        if (ImGui::IsItemHovered())
            hitArea = window::ViewHitArea::ButtonClose;
        ImGui::PopStyleColor(2);

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    bool BeginDockTabContextMenu(const char* id)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        if ((window->DockTabItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup(id);
        }

        const float dpiScale = ImGui::GetWindowDpiScale();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, MenuWindowPadding * dpiScale);
        const bool open = ImGui::BeginPopup(id);

        if (!open)
        {
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, MenuItemSpacing * dpiScale);
        }

        return open;
    }

    void EndDockTabContextMenu()
    {
        ImGui::PopStyleVar(2);
        ImGui::EndPopup();
    }
}
