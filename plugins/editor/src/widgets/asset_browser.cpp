// Copyright Chad Engler

#include "he/editor/widgets/asset_browser.h"

#include "he/assets/asset_database.h"
#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/appender.h"
#include "he/core/directory.h"
#include "he/core/enum_ops.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/string_fmt.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/widgets/buttons.h"
#include "he/editor/widgets/menu.h"

#include "imgui.h"
#include "fmt/format.h"

namespace he::editor
{
    AssetBrowser::AssetBrowser(
        AssetService& assetService,
        ImGuiService& imguiService) noexcept
        : m_assetService(assetService)
        , m_imguiService(imguiService)
    {
        const assets::AssetDatabase& db = m_assetService.AssetDB();
        const String& assetRoot = db.AssetRoot();

        m_root.name = "assets";
        m_root.path = assetRoot;
    }

    void AssetBrowser::Show()
    {
        ShowActionBar();

        if (ImGui::BeginTable("##asset_browser_layout", 2, ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("##directory_tree");
            ImGui::TableSetupColumn("##asset_grid");

            TreeNode* selected = nullptr;
            ImGui::TableNextColumn();
            ShowTreeNode(m_root, selected);

            ImGui::TableNextColumn();
            if (selected)
            {
                switch (m_viewType)
                {
                    case ViewType::List: ShowAssetList(*selected); break;
                    case ViewType::Grid: ShowAssetGrid(*selected); break;
                }
            }
            else
            {
                ShowNoAssets();
            }

            // TODO: This is using the label as the area right now, need to do better! Maybe `InvisibleButton`?
            if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
            {
                if (payload->IsDataType(ImGuiService::DndPayloadId))
                {
                    ImGui::GetWindowDrawList()->AddRect(
                        ImGui::GetItemRectMin(),
                        ImGui::GetItemRectMax(),
                        ImGui::GetColorU32(ImGuiCol_DragDropTarget),
                        0.0f,
                        ImDrawFlags_None,
                        2.0f);
                }
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (ImGui::AcceptDragDropPayload(ImGuiService::DndPayloadId, ImGuiDragDropFlags_AcceptNoDrawDefaultRect | ImGuiDragDropFlags_AcceptNoPreviewTooltip))
                {
                    Span<const String> files = m_imguiService.DragDropPaths();
                    HE_LOGF_INFO(he_editor, "FILES DROPPED: {}", fmt::join(files.begin(), files.end(), " | "));
                    m_imguiService.ClearDragDropPaths();
                    // TODO: imports
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::EndTable();
        }
    }

    void AssetBrowser::ShowActionBar()
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float dpiScale = ImGui::GetWindowDpiScale();

        // Back/forward buttons for navigating folder history
        ImGui::Button(ICON_MDI_ARROW_LEFT);
        ImGui::SameLine();
        ImGui::Button(ICON_MDI_ARROW_RIGHT);

        // TODO: breadcrumb
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x / 2, style.ItemSpacing.y / 2));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.y / 2, style.FramePadding.y));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
        ImGui::SameLine(0.0f, 16.0f * dpiScale);
        ImGui::TextUnformatted(ICON_MDI_FOLDER);
        ImGui::SameLine();
        ImGui::Button("assets");
        ImGui::SameLine();
        ImGui::TextUnformatted(ICON_MDI_CHEVRON_RIGHT);
        ImGui::SameLine();
        ImGui::Button("test");
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        const char* settingsBtnText = ICON_MDI_COG " Settings " ICON_MDI_CHEVRON_DOWN;
        const float btnWidth = ImGui::CalcTextSize(settingsBtnText).x;
        const float offset = btnWidth + (style.FramePadding.x * 2) + style.ItemSpacing.x + style.WindowBorderSize;
        ImGui::SameLine(ImGui::GetWindowWidth() - offset);
        if (BeginPopupMenuButton(settingsBtnText, "asset_browser_settings_popup"))
        {
            MenuSeparator("View");

            if (MenuItem("Details", ICON_MDI_VIEW_HEADLINE, nullptr, m_viewType == ViewType::List))
                m_viewType = ViewType::List;

            if (MenuItem("Icon Grid", ICON_MDI_VIEW_MODULE, nullptr, m_viewType == ViewType::Grid))
                m_viewType = ViewType::Grid;

            EndPopupMenuButton();
        }
    }

    void AssetBrowser::ShowTreeNode(TreeNode& node, TreeNode*& selected)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
        if (m_selectedPath == node.path)
        {
            selected = &node;
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool open = ImGui::TreeNodeEx(node.name.Data(), flags);
        if (ImGui::IsItemClicked())
        {
            selected = &node;
            m_selectedPath = node.path;
        }

        if (open)
        {
            if (ShouldListDirectory(node))
                ListDirectory(node);

            for (TreeNode& child : node.children)
            {
                ShowTreeNode(child, selected);
            }

            ImGui::TreePop();
        }
    }

    void AssetBrowser::ShowAssetList(TreeNode& node)
    {
        if (ShouldQueryAssets(node))
            QueryAssets(node);

        // TODO: Sorting

        if (ImGui::BeginTable("##asset_grid", 5, ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("UUID");
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupColumn("File UUID");
            ImGui::TableHeadersRow();

            if (node.assets.IsEmpty())
            {
                ImGui::EndTable();
                ShowNoAssets();
                return;
            }

            for (const assets::AssetModel& asset : node.assets)
            {
                // Name
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(asset.name.Begin(), asset.name.End());

                // Type
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(asset.type.Begin(), asset.type.End());

                // UUID
                m_buffer.Clear();
                fmt::format_to(Appender(m_buffer), "{}", asset.uuid);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m_buffer.Begin(), m_buffer.End());

                // State
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(AsString(asset.state));

                // File UUID
                m_buffer.Clear();
                fmt::format_to(Appender(m_buffer), "{}", asset.fileUuid);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m_buffer.Begin(), m_buffer.End());
            }

            ImGui::EndTable();
        }
    }

    void AssetBrowser::ShowAssetGrid(TreeNode& node)
    {
        if (ShouldQueryAssets(node))
            QueryAssets(node);

        if (node.assets.IsEmpty())
        {
            ShowNoAssets();
            return;
        }

        ImGui::TextUnformatted("Grid TODO");
    }

    void AssetBrowser::ShowNoAssets()
    {
        ImGui::TextUnformatted("No assets in directory.");
    }

    bool AssetBrowser::ShouldListDirectory(const TreeNode& node)
    {
        constexpr Duration UpdateFrequency = FromPeriod<Seconds>(60);
        const MonotonicTime now = MonotonicClock::Now();
        const Duration diff = now - node.listedAt;
        return diff > UpdateFrequency;
    }

    bool AssetBrowser::ShouldQueryAssets(const TreeNode& node)
    {
        constexpr Duration UpdateFrequency = FromPeriod<Seconds>(30);
        const MonotonicTime now = MonotonicClock::Now();
        const Duration diff = now - node.queriedAt;
        return diff > UpdateFrequency;
    }

    void AssetBrowser::ListDirectory(TreeNode& node)
    {
        DirectoryScanner scanner;
        Result r = scanner.Open(node.path.Data());
        if (!r)
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to open directory for scanning."),
                HE_KV(path, node.path),
                HE_KV(result, r));
            return;
        }

        node.listedAt = MonotonicClock::Now();
        node.children.Clear();

        DirectoryScanner::Entry entry;
        while (scanner.NextEntry(entry))
        {
            if (entry.isDirectory)
            {
                TreeNode& child = node.children.EmplaceBack();
                child.name = entry.name;
                child.path = node.path;
                ConcatPath(child.path, entry.name);
            }
        }
    }

    void AssetBrowser::QueryAssets(TreeNode& node)
    {
        assets::AssetDatabase& db = m_assetService.AssetDB();

        if (!db.IsInitialized())
            return;

        node.queriedAt = MonotonicClock::Now();
        node.assets.Clear();

        if (!assets::AssetModel::FindAll(db, node.path.Data(), node.assets, assets::AssetFilePathTag{}))
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to query for assets in path."),
                HE_KV(path, node.path));
        }
    }
}
