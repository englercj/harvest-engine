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
#include "he/editor/dialogs/import_asset_dialog.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/widgets/buttons.h"
#include "he/editor/widgets/menu.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "fmt/format.h"

namespace he::editor
{
    AssetBrowser::AssetBrowser(
        AssetService& assetService,
        DialogService& dialogService,
        ImGuiService& imguiService) noexcept
        : m_assetService(assetService)
        , m_dialogService(dialogService)
        , m_imguiService(imguiService)
    {
        m_assetService.OnDbInit().Attach<&AssetBrowser::HandleDbInitialized>(this);
        HandleDbInitialized(m_assetService.AssetDB().IsInitialized());
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
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
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

        if (FileDragDrop())
        {
            Span<const String> files = m_imguiService.DragDropPaths();
            for (const String& file : files)
            {
                m_dialogService.Open<ImportAssetDialog>().Configure(file.Data(), node.path);
            }
            m_imguiService.ClearDragDropPaths();
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

            for (const TreeNode& child : node.children)
            {
                // Name
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ICON_MDI_FOLDER);
                ImGui::SameLine();
                ImGui::TextUnformatted(child.name.Begin(), child.name.End());

                // Type
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("folder");

                // UUID
                ImGui::TableNextColumn();

                // State
                ImGui::TableNextColumn();

                // File UUID
                ImGui::TableNextColumn();
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
        if (node.path.IsEmpty())
            return;

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

    void AssetBrowser::HandleDbInitialized(bool initialized)
    {
        if (m_root.name.IsEmpty())
            m_root.name = "assets";

        if (initialized)
        {
            const assets::AssetDatabase& db = m_assetService.AssetDB();
            const String& assetRoot = db.AssetRoot();

            m_root.path = assetRoot;
        }
        else
        {
            m_root.path.Clear();
            m_root.children.Clear();
            m_root.assets.Clear();
            m_root.listedAt = { 0 };
            m_root.queriedAt = { 0 };
        }
    }

    bool AssetBrowser::FileDragDrop()
    {
        bool accepted = false;

        if (ImGui::BeginDragDropTarget())
        {
            accepted = ImGui::AcceptDragDropPayload(ImGuiService::DndPayloadId, ImGuiDragDropFlags_AcceptNoDrawDefaultRect | ImGuiDragDropFlags_AcceptNoPreviewTooltip);
            ImGui::EndDragDropTarget();
        }

        return accepted;
    }
}
