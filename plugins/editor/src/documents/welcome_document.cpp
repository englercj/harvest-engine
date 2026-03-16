// Copyright Chad Engler

#include "he/editor/documents/welcome_document.h"

#include "he/core/string.h"
#include "he/core/vector.h"
#include "he/editor/dialogs/choice_dialog.h"
#include "he/editor/dialogs/create_project_dialog.h"
#include "he/editor/framework/imgui_theme.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/widgets/menu.h"
#include "he/editor/widgets/buttons.h"

#include "imgui.h"

namespace he::editor
{
    WelcomeDocument::WelcomeDocument(
        DialogService& dialogService,
        ProjectService& projectService,
        SettingsService& settingsService,
        UniquePtr<OpenProjectCommand> openProjectCommand,
        UniquePtr<OpenProjectFileCommand> openProjectFileCommand) noexcept
        : m_dialogService(dialogService)
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

        if (ImGui::BeginChild("welcome_padding", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None))
        {
            PushFont(Font::RegularTitle);
            ImGui::TextUnformatted("Harvest Editor");
            PopFont();

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
        PushFont(Font::RegularHeader);
        ImGui::TextUnformatted("Start");
        PopFont();
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
        PushFont(Font::RegularHeader);
        ImGui::TextUnformatted("Recent");
        PopFont();
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
