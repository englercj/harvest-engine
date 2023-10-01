// Copyright Chad Engler

#include "he/editor/documents/asset_browser_document.h"

#include "imgui.h"

namespace he::editor
{
    AssetBrowserDocument::AssetBrowserDocument(UniquePtr<AssetBrowser> browser) noexcept
        : m_browser(Move(browser))
    {
        m_title = "Asset Browser";
    }

    void AssetBrowserDocument::Show()
    {
        m_browser->Show();
    }
}
