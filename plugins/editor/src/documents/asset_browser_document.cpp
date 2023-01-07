// Copyright Chad Engler

#include "asset_browser_document.h"

#include "imgui.h"

namespace he::editor
{
    AssetBrowserDocument::AssetBrowserDocument(
        AssetService& assetService,
        ImGuiService& imguiService) noexcept
        : m_browser(assetService, imguiService)
    {
        m_title = "Asset Browser";
    }

    void AssetBrowserDocument::Show()
    {
        m_browser.Show();
    }
}
