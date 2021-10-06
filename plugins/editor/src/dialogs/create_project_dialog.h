// Copyright Chad Engler

#pragma once

#include "dialog.h"
#include "services/platform_service.h"
#include "services/project_service.h"

#include "he/core/path.h"

namespace he::editor
{
    class CreateProjectDialog : public Dialog
    {
    public:
        CreateProjectDialog(
            PlatformService& platformService,
            ProjectService& projectService);

        void ShowContent() override;
        void ShowButtons() override;

    private:
        PlatformService& m_platformService;
        ProjectService& m_projectService;

        String m_name{};
        String m_path{};
    };
}
