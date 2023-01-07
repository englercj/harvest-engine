// Copyright Chad Engler

#include "panel_service.h"

#include "widgets/animations.h"
#include "widgets/buttons.h"
#include "widgets/menu.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    PanelService::PanelService(DocumentService& documentService) noexcept
        : m_documentService(documentService)
    {}

    void PanelService::DestroyClosedPanels()
    {
        m_pendingClose.Clear();
    }

    void PanelService::ShowPanels()
    {
        if (!m_entry.panel)
            return;

        const ImGuiViewport* viewport = ImGui::GetWindowViewport();
        const ImVec2 pos = viewport->WorkPos;
        const ImVec2 size = viewport->Size;

        const float height = Max(size.y / 3.0f, 200.0f * ImGui::GetWindowDpiScale());
        const float frame = ImGui::GetFrameHeight();

        const ImVec2 fromPos{ pos.x, pos.y + size.y - frame - (frame*2.0f) };
        const ImVec2 toPos{ pos.x, pos.y + size.y - height - (frame*2.0f) };

        const ImVec2 fromSize{ size.x, frame };
        const ImVec2 toSize{ size.x, height };

        constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
        constexpr double AnimDuration = 0.15;

        double& openTime = m_entry.openTime;
        Document* panel = m_entry.panel.Get();

        if (openTime == 0)
            openTime = ImGui::GetTime();

        AnimNextWindowPos(fromPos, toPos, openTime, AnimDuration);
        AnimNextWindowSize(fromSize, toSize, openTime, AnimDuration);

        const char* label = panel->Label();
        if (ImGui::Begin(label, nullptr, WindowFlags))
        {
            const ImGuiWindow* window = ImGui::GetCurrentWindow();
            const ImRect titleBarRect = window->TitleBarRect();
            ImGui::PushClipRect(titleBarRect.Min, titleBarRect.Max, false);

            const ImGuiStyle& style = ImGui::GetStyle();
            const ImVec2 framePadding = style.FramePadding;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding.x / 2, framePadding.y));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());

            const float btnWidth = 30.0f * ImGui::GetWindowDpiScale();
            const float offset = (btnWidth * 2) + style.ItemSpacing.x + style.WindowBorderSize;
            ImGui::SetCursorPos(ImVec2(window->Size.x - offset, 0.0f));

            if (ImGui::Button(ICON_MDI_DOCK_WINDOW, ImVec2(btnWidth, 0)))
                m_documentService.Open(Move(m_entry.panel));

            ImGui::SameLine();

            if (ImGui::Button(ICON_MDI_CHEVRON_DOWN, ImVec2(btnWidth, 0)))
                Close();

            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
            ImGui::PopClipRect();

            panel->Show();
        }

        ImGui::End();
    }
}
