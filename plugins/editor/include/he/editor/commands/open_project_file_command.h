// Copyright Chad Engler

#pragma once

#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/editor/commands/command.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/project_service.h"

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
        void SetPath(StringView path) { m_path = path; }

    private:
        DialogService& m_dialogService;
        ProjectService& m_projectService;

        String m_path;
    };
}
