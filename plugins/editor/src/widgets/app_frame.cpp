// Copyright Chad Engler

#include "he/editor/widgets/app_frame.h"

#include "he/editor/widgets/menu.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    window::ViewHitArea AppFrame::GetHitArea(const Vec2i& point) const
    {
        if (!m_view)
            return window::ViewHitArea::NotInView;

        const Vec2i size = m_view->GetSize();

        // Check if the point is outside the view. Since the point is view relative, we can just
        // check if it's outside the view's size.
        if (point.x < 0 || point.x > size.x || point.y < 0 || point.y > size.y)
            return window::ViewHitArea::NotInView;

        // If the point is within the view, and a popup is open, then return normal to prevent
        // resizing the border of the window.
        if (ImGui::IsPopupOpen(ImGuiID(0), ImGuiPopupFlags_AnyPopupId))
            return window::ViewHitArea::Normal;

        // Check if the point lies within any of the resize zones.
        constexpr window::ViewHitArea HitAreaZones[3][3] =
        {
            { window::ViewHitArea::ResizeTopLeft,       window::ViewHitArea::ResizeTop,     window::ViewHitArea::ResizeTopRight },
            { window::ViewHitArea::ResizeLeft,          window::ViewHitArea::Normal,        window::ViewHitArea::ResizeRight },
            { window::ViewHitArea::ResizeBottomLeft,    window::ViewHitArea::ResizeBottom,  window::ViewHitArea::ResizeBottomRight },
        };
        uint32_t zoneRow = 1;
        uint32_t zoneCol = 1;

        const float ResizeBorderSize = 10.0f;

        if (point.x < ResizeBorderSize)
            zoneCol = 0;
        else if (point.x > (size.x - ResizeBorderSize))
            zoneCol = 2;

        if (point.y < ResizeBorderSize)
            zoneRow = 0;
        else if (point.y > (size.y - ResizeBorderSize))
            zoneRow = 2;

        const window::ViewHitArea hitArea = HitAreaZones[zoneRow][zoneCol];

        // If the point is in the client area zone, and not in a resize zone, then return our
        // cached menu hit area. This will be `Normal` no menu action is active.
        return hitArea == window::ViewHitArea::Normal ? m_menuHitArea : hitArea;
    }

    bool AppFrame::BeginAppMainMenuBar()
    {
        m_menuHitArea = window::ViewHitArea::Normal;

        if (!m_view)
            return false;

        const bool focused = m_view->IsFocused();

        if (!focused)
        {
            ImGuiStyle& style = ImGui::GetStyle();

            ImVec4 bgColor = style.Colors[ImGuiCol_MenuBarBg];
            bgColor = ImLerp(bgColor, ImVec4(0, 0, 0, 1), 0.25f);
            ImGui::PushStyleColor(ImGuiCol_MenuBarBg, bgColor);

            ImVec4 textColor = style.Colors[ImGuiCol_Text];
            textColor = ImLerp(textColor, ImVec4(0, 0, 0, 1), 0.35f);
            ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        }

        if (editor::BeginAppMainMenuBar())
        {
            TopLevelIcon();
            if (ImGui::IsItemHovered())
                m_menuHitArea = window::ViewHitArea::SystemMenu;

            return true;
        }

        if (!focused)
            ImGui::PopStyleColor(2);

        return false;
    }

    void AppFrame::EndAppMainMenuBar()
    {
        if (!m_view)
            return;

        MenuSystemButtons(m_view, m_menuHitArea);

        // If we're in the main menu bar, not over anything more specific, set us to draggable.
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            m_menuHitArea = window::ViewHitArea::Draggable;

        editor::EndAppMainMenuBar();

        if (!m_view->IsFocused())
            ImGui::PopStyleColor(2);
    }
}
