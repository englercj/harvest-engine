// Copyright Chad Engler

#pragma once

#include "command.h"
#include "fonts/icons_material_design.h"
#include "services/dialog_service.h"
#include "services/project_service.h"

#include "he/core/string.h"
#include "he/core/types.h"

namespace he::editor
{
    class OpenProjectFileCommand : public Command
    {
    public:
        OpenProjectFileCommand(
            DialogService& dialogService,
            ProjectService& projectService) noexcept;

        bool CanRun() const override;
        void Run() override;

        const char* Label() const override { return "Open Project File"; }
        const char* Icon() const override { return ICON_MDI_FOLDER_OPEN; }

        const String& Path() const { return m_path; }
        void SetPath(const char* path) { m_path = path; }

    private:
        DialogService& m_dialogService;
        ProjectService& m_projectService;

        String m_path;
    };
}
