// Copyright Chad Engler

#include "he/editor/widgets/asset_browser.h"

#include "he/assets/asset_database.h"
#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/directory.h"
#include "he/core/enum_ops.h"
#include "he/core/fmt.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_ops.h"
#include "he/editor/dialogs/import_asset_dialog.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/widgets/buttons.h"
#include "he/editor/widgets/menu.h"
#include "he/editor/widgets/progress.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    AssetBrowser::AssetBrowser(
        AssetService& assetService,
        DialogService& dialogService,
        ImGuiService& imguiService) noexcept
        : m_assetService(assetService)
        , m_dialogService(dialogService)
        , m_imguiService(imguiService)
    {}

    AssetBrowser::~AssetBrowser() noexcept
    {
        m_dbInitBinding.Detach();
    }

    void AssetBrowser::Show()
    {
        if (!m_assetService.IsAssetDBReady())
        {
            ImGui::TextUnformatted("Scanning assets files...");
            ProgressBar(-1.0f);
            return;
        }

        if (m_roots.IsEmpty())
        {
            const Span<const AssetService::ContentModule> contents = m_assetService.ContentModules();
            for (const AssetService::ContentModule& content : contents)
            {
                TreeNode& root = m_roots.EmplaceBack();
                root.name = content.mod.GetName();
                root.rootIndex = m_roots.Size() - 1;

                ListDirectory(root);
                QueryAssets(root);
            }
        }

        ShowActionBar();

        if (ImGui::BeginTable("##asset_browser_layout", 2, ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("##directory_tree");
            ImGui::TableSetupColumn("##asset_grid");

            TreeNode* selected = nullptr;
            ImGui::TableNextColumn();

            for (TreeNode& root : m_roots)
            {
                ShowTreeNode(root, selected);
            }

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

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x / 2, style.ItemSpacing.y / 2));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.y / 2, style.FramePadding.y));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
        ImGui::SameLine(0.0f, 16.0f * dpiScale);
        ImGui::TextUnformatted(ICON_MDI_FOLDER);

        ImGui::SameLine();
        const TreeNode& root = m_roots[m_selectedRootIndex];
        if (ImGui::Button(root.name.Data()))
        {
            m_selectedPath.Clear();
        }

        if (!m_selectedPath.IsEmpty())
        {
            const char* begin = m_selectedPath.Data();
            do
            {
                const char* end = StrFind(begin, '/');
                if (end == nullptr)
                    end = m_selectedPath.End();

                ImGui::SameLine();
                ImGui::TextUnformatted(ICON_MDI_CHEVRON_RIGHT);

                m_buffer.Assign(begin, end);
                ImGui::SameLine();
                if (ImGui::Button(m_buffer.Data()))
                {
                    m_selectedPath.Resize(static_cast<uint32_t>(end - m_selectedPath.Begin()));
                }

                begin = end + 1;
            } while (begin < m_selectedPath.End());
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        const char* settingsBtnText = ICON_MDI_COG " Settings " ICON_MDI_CHEVRON_DOWN;
        const float btnWidth = ImGui::CalcTextSize(settingsBtnText).x;
        const float offset = btnWidth + (style.FramePadding.x * 2) + style.ItemSpacing.x + style.WindowBorderSize;
        ImGui::SameLine(ImGui::GetWindowWidth() - offset);
        if (BeginPopupMenuButton(settingsBtnText, "asset_browser_settings_popup"))
        {
            MenuSeparator("View");

            if (MenuItem("Details", ICON_MDI_VIEW_HEADLINE, nullptr, m_viewType == ViewType::List))
            {
                m_viewType = ViewType::List;
            }

            if (MenuItem("Icon Grid", ICON_MDI_VIEW_MODULE, nullptr, m_viewType == ViewType::Grid))
            {
                m_viewType = ViewType::Grid;
            }

            EndPopupMenuButton();
        }
    }

    void AssetBrowser::ShowTreeNode(TreeNode& node, TreeNode*& selected)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (m_selectedPath == node.path && m_selectedRootIndex == node.rootIndex)
        {
            selected = &node;
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool open = ImGui::TreeNodeEx(node.name.Data(), flags);
        if (ImGui::IsItemClicked())
        {
            selected = &node;
            m_selectedPath = node.path;
            m_selectedRootIndex = node.rootIndex;
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
        const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

        if (ImGui::BeginTable("##asset_grid", 4, flags))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("UUID");
            ImGui::TableSetupColumn("State");
            ImGui::TableHeadersRow();

            const ImGuiSelectableFlags selectFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_AllowDoubleClick;

            for (const TreeNode& child : node.children)
            {
                ImGui::PushID(child.name.Begin(), child.name.End());

                // Name
                ImGui::TableNextColumn();
                bool selected = false;
                if (ImGui::Selectable("##folder_select", &selected, selectFlags))
                {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        ConcatPath(m_selectedPath, child.name);
                    }
                    else
                    {
                        m_selectedAsset = assets::AssetUuid{};
                    }
                }
                ImGui::SameLine();
                ImGui::TextUnformatted(ICON_MDI_FOLDER " ");
                ImGui::SameLine();
                ImGui::TextUnformatted(child.name.Begin(), child.name.End());

                // Type
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("folder");

                // UUID
                ImGui::TableNextColumn();

                // State
                ImGui::TableNextColumn();

                ImGui::PopID();
            }

            for (const assets::AssetModel& asset : node.assets)
            {
                const char* uuidBytes = reinterpret_cast<const char*>(asset.uuid.val.m_bytes);
                constexpr uint32_t UuidBytesLen = sizeof(asset.uuid.val.m_bytes);
                ImGui::PushID(uuidBytes, uuidBytes + UuidBytesLen);

                // Name
                ImGui::TableNextColumn();
                bool selected = m_selectedAsset == asset.uuid;
                if (ImGui::Selectable("##asset_select", &selected, selectFlags))
                {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        HE_LOGF_INFO(he_editor, "TODO: open asset {}", asset.uuid);
                    }
                    else
                    {
                        m_selectedAsset = asset.uuid;
                    }
                }
                // TODO: Asset context menu.
                ImGui::SameLine();
                ImGui::TextUnformatted(ICON_MDI_FILE_DOCUMENT " "); // TODO: asset type icons
                ImGui::SameLine();
                ImGui::TextUnformatted(asset.name.Begin(), asset.name.End());

                // Type
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(asset.assetType.String());

                // UUID
                m_buffer.Clear();
                FormatTo(m_buffer, "{}", asset.uuid);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m_buffer.Begin(), m_buffer.End());

                // State
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(EnumToString(asset.state));

                ImGui::PopID();
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
        ImGui::TextUnformatted("No assets in selected directory.");
    }

    bool AssetBrowser::ShouldListDirectory(const TreeNode& node)
    {
        constexpr Duration UpdateFrequency = FromPeriod<Seconds>(30);
        const MonotonicTime now = MonotonicClock::Now();
        const Duration diff = now - node.listedAt;
        return diff > UpdateFrequency;
    }

    bool AssetBrowser::ShouldQueryAssets(const TreeNode& node)
    {
        constexpr Duration UpdateFrequency = FromPeriod<Seconds>(10);
        const MonotonicTime now = MonotonicClock::Now();
        const Duration diff = now - node.queriedAt;
        return diff > UpdateFrequency;
    }

    void AssetBrowser::ListDirectory(TreeNode& node)
    {
        assets::AssetDatabase& db = m_assetService.AssetDB();

        if (!db.IsInitialized())
            return;

        const AssetService::ContentModule& content = m_assetService.ContentModules()[node.rootIndex];
        String path = content.rootPath;
        ConcatPath(path, node.path);

        DirectoryScanner scanner;
        Result r = scanner.Open(path.Data());
        if (!r)
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to open directory for scanning."),
                HE_KV(path, path),
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
                child.rootIndex = node.rootIndex;
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

        constexpr sqlite::RawSqlQuery raw
        {
            "SELECT * FROM asset JOIN asset_file",
        };

        if (!assets::AssetModel::FindAllByAssetFilePath(db, node.assets, node.path))
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to query for assets in path."),
                HE_KV(path, node.path));
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
