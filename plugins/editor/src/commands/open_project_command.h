// Copyright Chad Engler

#pragma once

#include "command.h"
#include "open_project_file_command.h"
#include "fonts/icons_material_design.h"
#include "services/platform_service.h"
#include "services/project_service.h"

#include "he/core/types.h"
#include "he/core/unique_ptr.h"

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
