// Copyright Chad Engler

#pragma once

#include "he/core/path.h"
#include "he/editor/dialogs/dialog.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/platform_service.h"
#include "he/editor/services/project_service.h"

namespace he::editor
{
    class CreateProjectDialog : public Dialog
    {
    public:
        CreateProjectDialog(
            DialogService& dialogService,
            PlatformService& platformService,
            ProjectService& projectService) noexcept;

        void ShowContent() override;
        void ShowButtons() override;

    private:
        DialogService& m_dialogService;
        PlatformService& m_platformService;
        ProjectService& m_projectService;

        String m_projectName{};
        String m_projectPath{};
        String m_enginePath{};
    };
}
