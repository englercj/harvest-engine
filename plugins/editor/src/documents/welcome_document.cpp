// Copyright Chad Engler

#include "welcome_document.h"

#include "fonts/IconsFontAwesome5Pro.h"
#include "widgets/menu.h"
#include "widgets/buttons.h"

#include "imgui.h"

namespace he::editor
{
    WelcomeDocument::WelcomeDocument(
        DialogService& dialogService,
        ImGuiService& imguiService,
        PlatformService& platformService)
        : m_dialogService(dialogService)
        , m_imguiService(imguiService)
        , m_platformService(platformService)
    {
        m_title = "Welcome";
    }

    void WelcomeDocument::Show()
    {
        const float dpiScale = ImGui::GetWindowDpiScale();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(48.0f * dpiScale, 64.0f * dpiScale));

        if (ImGui::BeginChild("welcome_padding", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
        {
            m_imguiService.PushFont(Font::RegularTitle);
            ImGui::TextUnformatted("Harvest Editor");
            m_imguiService.PopFont();

            ImGui::NewLine();
            ImGui::NewLine();

            ShowStartSection();
            ImGui::NewLine();
            ImGui::NewLine();

            ShowRecentSection();
            ImGui::NewLine();
            ImGui::NewLine();
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();
    }

    void WelcomeDocument::ShowStartSection()
    {
        m_imguiService.PushFont(Font::RegularHeader);
        ImGui::TextUnformatted("Start");
        m_imguiService.PopFont();
        ImGui::NewLine();

        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Project"))
        {
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_PLUS " Create New Project"))
        {
            RequestClose();
        }
    }

    void WelcomeDocument::ShowRecentSection()
    {
        m_imguiService.PushFont(Font::RegularHeader);
        ImGui::TextUnformatted("Recent");
        m_imguiService.PopFont();
        ImGui::NewLine();

        ImGui::TextUnformatted("No recently opened projects.");
    }
}
