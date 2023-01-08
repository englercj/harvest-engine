// Copyright Chad Engler

#pragma once

#include "he/editor/commands/open_project_command.h"
#include "he/editor/documents/document.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/imgui_service.h"
#include "he/editor/services/platform_service.h"
#include "he/editor/services/project_service.h"
#include "he/editor/services/settings_service.h"

namespace he::editor
{
    class WelcomeDocument : public Document
    {
    public:
        WelcomeDocument(
            DialogService& dialogService,
            ImGuiService& imguiService,
            ProjectService& projectService,
            SettingsService& settingsService,
            UniquePtr<OpenProjectCommand> openProjectCommand,
            UniquePtr<OpenProjectFileCommand> openProjectFileCommand) noexcept;

        void Show() override;

    private:
        void ShowStartSection();
        void ShowRecentSection();

    private:
        DialogService& m_dialogService;
        ImGuiService& m_imguiService;
        ProjectService& m_projectService;
        SettingsService& m_settingsService;

        UniquePtr<OpenProjectCommand> m_openProjectCommand;
        UniquePtr<OpenProjectFileCommand> m_openProjectFileCommand;
    };
}
