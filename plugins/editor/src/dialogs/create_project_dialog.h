// Copyright Chad Engler

#pragma once

#include "dialog.h"
#include "services/dialog_service.h"
#include "services/platform_service.h"
#include "services/project_service.h"

#include "he/core/path.h"

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

        String m_name{};
        String m_path{};
        String m_assetRoot{};
        String m_errorMessage{};

        bool m_overrideAssetRoot{ false };
    };
}
