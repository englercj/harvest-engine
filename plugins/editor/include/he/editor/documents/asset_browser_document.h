// Copyright Chad Engler

#pragma once

#include "he/editor/documents/document.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/services/imgui_service.h"
#include "he/editor/widgets/asset_browser.h"

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
