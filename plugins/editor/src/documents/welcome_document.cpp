// Copyright Chad Engler

#include "welcome_document.h"

#include "dialogs/choice_dialog.h"
#include "dialogs/create_project_dialog.h"
#include "fonts/icons_material_design.h"
#include "widgets/menu.h"
#include "widgets/buttons.h"

#include "he/core/string.h"
#include "he/core/vector.h"

#include "imgui.h"

namespace he::editor
{
    WelcomeDocument::WelcomeDocument(
        DialogService& dialogService,
        ImGuiService& imguiService,
        ProjectService& projectService,
        SettingsService& settingsService,
        UniquePtr<OpenProjectCommand> openProjectCommand,
        UniquePtr<OpenProjectFileCommand> openProjectFileCommand) noexcept
        : m_dialogService(dialogService)
        , m_imguiService(imguiService)
        , m_projectService(projectService)
        , m_settingsService(settingsService)
        , m_openProjectCommand(Move(openProjectCommand))
        , m_openProjectFileCommand(Move(openProjectFileCommand))
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

        if (CommandButton(ICON_MDI_FOLDER_OPEN " Open Project", *m_openProjectCommand))
        {
            RequestClose();
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_MDI_PLUS " Create New Project"))
        {
            m_dialogService.Open<CreateProjectDialog>();
        }
    }

    void WelcomeDocument::ShowRecentSection()
    {
        m_imguiService.PushFont(Font::RegularHeader);
        ImGui::TextUnformatted("Recent");
        m_imguiService.PopFont();
        ImGui::NewLine();

        Settings::Builder& settings = m_settingsService.GetSettings();

        const auto recentProjects = settings.GetRecentProjects();

        if (recentProjects.Size() == 0)
        {
            ImGui::TextUnformatted("No recently opened projects.");
            return;
        }

        for (auto&& recent : recentProjects.AsReader())
        {
            const char* path = recent.GetPath().Data();
            m_openProjectFileCommand->SetPath(path);
            if (CommandLinkButton(path, *m_openProjectFileCommand))
            {
                RequestClose();
            }
        }
    }
}
