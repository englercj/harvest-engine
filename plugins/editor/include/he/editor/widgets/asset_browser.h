// Copyright Chad Engler

#pragma once

#include "he/assets/asset_models.h"
#include "he/core/clock.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/imgui_service.h"

namespace he::editor
{
    class AssetBrowser
    {
    public:
        enum class ViewType : uint8_t
        {
            List,
            Grid,
        };

    public:
        AssetBrowser(
            AssetService& assetService,
            DialogService& dialogService,
            ImGuiService& imguiService) noexcept;

        ~AssetBrowser() noexcept;

        void Show();

    private:
        struct TreeNode
        {
            String name{};
            String path{};
            Vector<TreeNode> children{};
            Vector<assets::AssetModel> assets{};

            uint32_t rootIndex{ 0 };
            MonotonicTime listedAt{ 0 };
            MonotonicTime queriedAt{ 0 };
        };

    private:
        void ShowActionBar();
        void ShowTreeNode(TreeNode& node, TreeNode*& selected);
        void ShowAssetList(TreeNode& node);
        void ShowAssetGrid(TreeNode& node);
        void ShowNoAssets();

        bool ShouldListDirectory(const TreeNode& node);
        bool ShouldQueryAssets(const TreeNode& node);

        void ListDirectory(TreeNode& node);
        void QueryAssets(TreeNode& node);

        bool FileDragDrop();

    private:
        AssetService& m_assetService;
        DialogService& m_dialogService;
        ImGuiService& m_imguiService;

        Vector<TreeNode> m_roots{};
        String m_selectedPath{};
        uint32_t m_selectedRootIndex{ 0 };

        ViewType m_viewType{ ViewType::List };

        String m_buffer{};

        AssetService::DbInitSignal::Binding m_dbInitBinding{};
        assets::AssetUuid m_selectedAsset{};
    };
}
