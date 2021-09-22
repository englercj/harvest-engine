// Copyright Chad Engler

#pragma once

#include "document.h"

#include "services/dialog_service.h"
#include "services/imgui_service.h"
#include "services/platform_service.h"

namespace he::editor
{
    class WelcomeDocument : public Document
    {
    public:
        WelcomeDocument(
            DialogService& dialogService,
            ImGuiService& imguiService,
            PlatformService& platformService);

        void Show() override;

    private:
        void ShowStartSection();
        void ShowRecentSection();

    private:
        DialogService& m_dialogService;
        ImGuiService& m_imguiService;
        PlatformService& m_platformService;
    };
}
