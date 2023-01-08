// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/editor/commands/command.h"
#include "he/editor/commands/open_project_file_command.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/services/platform_service.h"
#include "he/editor/services/project_service.h"

namespace he::editor
{
    class OpenProjectCommand : public Command
    {
    public:
        OpenProjectCommand(
            PlatformService& platformService,
            ProjectService& projectService,
            UniquePtr<OpenProjectFileCommand> openProjectFileCommand) noexcept;

        bool CanRun() const override;
        void Run() override;

        const char* Label() const override { return "Open Project..."; }
        const char* Icon() const override { return ICON_MDI_FOLDER_OPEN; }

    private:
        PlatformService& m_platformService;
        ProjectService& m_projectService;

        UniquePtr<OpenProjectFileCommand> m_openProjectFileCommand;
    };
}
