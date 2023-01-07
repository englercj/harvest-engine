// Copyright Chad Engler

#pragma once

#include "document.h"
#include "services/asset_service.h"
#include "services/imgui_service.h"
#include "widgets/asset_browser.h"

namespace he::editor
{
    class AssetBrowserDocument : public Document
    {
    public:
        AssetBrowserDocument(
            AssetService& assetService,
            ImGuiService& imguiService) noexcept;

        void Show() override;

    private:
        AssetBrowser m_browser;
    };
}
