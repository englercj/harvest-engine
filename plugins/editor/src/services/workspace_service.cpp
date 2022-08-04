// Copyright Chad Engler

#include "workspace_service.h"

#include "di.h"
#include "dialogs/choice_dialog.h"
#include "documents/document.h"
#include "documents/stats_document.h"
#include "documents/imgui_stack_tool_document.h"
#include "documents/imgui_style_editor_document.h"
#include "documents/imgui_widget_document.h"
#include "documents/welcome_document.h"
#include "fonts/icons_material_design.h"
#include "widgets/menu.h"
#include "widgets/progress.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "fmt/core.h"

namespace he::editor
{
    WorkspaceService::WorkspaceService(
        DialogService& dialogService,
        DocumentService& documentService,
        ImGuiService& imguiService,
        LogService& logService,
        MainWindowService& mainWindowService,
        PlatformService& platformService,
        TaskService& taskService)
        : m_dialogService(dialogService)
        , m_documentService(documentService)
        , m_imguiService(imguiService)
        , m_logService(logService)
        , m_mainWindowService(mainWindowService)
        , m_platformService(platformService)
        , m_taskService(taskService)
    {}

    void WorkspaceService::Show()
    {
        m_documentService.DestroyClosedDocuments();
        m_dialogService.DestroyClosedDialogs();

        ShowAppMenuBar();
        ShowAppStatusBar();

        m_documentService.ShowDocuments();
        m_dialogService.ShowDialogs();
    }

    window::ViewHitArea WorkspaceService::GetHitArea(const Vec2i& point) const
    {
        // Gather the window size so we can hit test for resize areas
        window::View* view = m_mainWindowService.GetView();
        Vec2i size = view->GetSize();

        // Check if the point is outside the window
        if (point.x < 0 || point.x > size.x || point.y < 0 || point.y > size.y)
            return window::ViewHitArea::NotInView;

        // Check if the point lies within any of the resize zones.
        constexpr window::ViewHitArea HitAreaZones[3][3] =
        {
            { window::ViewHitArea::ResizeTopLeft,       window::ViewHitArea::ResizeTop,     window::ViewHitArea::ResizeTopRight },
            { window::ViewHitArea::ResizeLeft,          window::ViewHitArea::Normal,        window::ViewHitArea::ResizeRight },
            { window::ViewHitArea::ResizeBottomLeft,    window::ViewHitArea::ResizeBottom,  window::ViewHitArea::ResizeBottomRight },
        };
        uint32_t zoneRow = 1;
        uint32_t zoneCol = 1;

        const float ResizeBorderSize = 10.0f;

        if (point.x < ResizeBorderSize)
            zoneCol = 0;
        else if (point.x > (size.x - ResizeBorderSize))
            zoneCol = 2;

        if (point.y < ResizeBorderSize)
            zoneRow = 0;
        else if (point.y > (size.y - ResizeBorderSize))
            zoneRow = 2;

        window::ViewHitArea hitArea = HitAreaZones[zoneRow][zoneCol];

        // If the point is in the client area zone (not a resize zone) then return our cached menu hit area
        if (hitArea == window::ViewHitArea::Normal)
            return m_menuHitArea;

        // If any point is in a resize zone, but a popup is open, then disallow resizing
        if (ImGui::IsPopupOpen(ImGuiID(0), ImGuiPopupFlags_AnyPopupId))
            return window::ViewHitArea::Normal;

        return hitArea;
    }

    void WorkspaceService::ShowAppMenuBar()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        window::View* view = m_mainWindowService.GetView();

        m_menuHitArea = window::ViewHitArea::Normal;

        const bool focused = view->IsFocused();

        if (!focused)
        {
            ImVec4 bgColor = style.Colors[ImGuiCol_MenuBarBg];
            bgColor = ImLerp(bgColor, ImVec4(0, 0, 0, 1), 0.25f);
            ImGui::PushStyleColor(ImGuiCol_MenuBarBg, bgColor);

            ImVec4 textColor = style.Colors[ImGuiCol_Text];
            textColor = ImLerp(textColor, ImVec4(0, 0, 0, 1), 0.35f);
            ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        }

        if (BeginAppMainMenuBar())
        {
            TopLevelIcon();
            if (ImGui::IsItemHovered())
                m_menuHitArea = window::ViewHitArea::SystemMenu;

            if (BeginTopLevelMenu("File"))
            {
                // Assets menu
                MenuSeparator("Assets");

                if (BeginMenu("Create New Asset", ICON_MDI_FILE_PLUS))
                {
                    MenuItem("Texture1");
                    MenuItem("Texture2");
                    MenuItem("Texture3");

                    EndMenu();
                }
                MenuItem("Open Asset...", ICON_MDI_FOLDER_OPEN, "Ctrl+O", false, false);

                if (BeginMenu("Open Recent Asset", ICON_MDI_TIMER_SAND))
                {
                    MenuItem("Some Asset");

                    EndMenu();
                }
                MenuItem("Import Asset...", ICON_MDI_FILE_IMPORT);
                MenuItem("Export Asset...", ICON_MDI_FILE_EXPORT);

                // Save menu
                MenuSeparator("Save");

                MenuItem("Save", ICON_MDI_CONTENT_SAVE, "Ctrl+S");
                MenuItem("Save As...", nullptr, "Ctrl+Alt+S");
                MenuItem("Save All", ICON_MDI_CONTENT_SAVE_ALL, "Ctrl+Shift+S");

                // Project menu
                MenuSeparator("Project");

                if (MenuItem("Open Project...", ICON_MDI_FOLDER_OPEN, nullptr, false, false))
                {
                }

                if (BeginMenu("Open Recent Project", nullptr, false))
                {
                    EndMenu();
                }

                if (MenuItem("Close Project", nullptr, nullptr, false, false))
                {
                }

                // Application menu
                MenuSeparator("Exit");

                if (MenuItem("Exit", ICON_MDI_CLOSE_CIRCLE, "Alt+F4"))
                    view->RequestClose();

                EndTopLevelMenu();
            }

            if (BeginTopLevelMenu("Edit"))
            {
                MenuSeparator("History");

                MenuItem("Undo", ICON_MDI_UNDO, "Ctrl+Z");
                MenuItem("Redo", ICON_MDI_REDO, "Ctrl+Y");

                MenuSeparator("Edit");

                MenuItem("Cut", ICON_MDI_CONTENT_CUT, "Ctrl+X");
                MenuItem("Copy", ICON_MDI_CONTENT_COPY, "Ctrl+C");
                MenuItem("Paste", ICON_MDI_CONTENT_PASTE, "Ctrl+V");
                MenuItem("Clone", ICON_MDI_CONTENT_DUPLICATE, "Ctrl+V");
                MenuItem("Delete", ICON_MDI_DELETE, "Delete");

                MenuSeparator("Configuration");

                MenuItem("User Settings...", ICON_MDI_ACCOUNT_COG);
                MenuItem("Project Settings...", ICON_MDI_COG);

                EndTopLevelMenu();
            }

            if (BeginTopLevelMenu("Tools"))
            {
                if (MenuItem("ImGui Stack Tool"))
                    m_documentService.Open<ImGuiStackToolDocument>();

                if (MenuItem("ImGui Style Editor"))
                    m_documentService.Open<ImGuiStyleEditorDocument>();

                if (MenuItem("ImGui Custom Widgets"))
                    m_documentService.Open<ImGuiWidgetDocument>();

                if (MenuItem("Stats"))
                    m_documentService.Open<StatsDocument>();

                EndTopLevelMenu();
            }

            if (BeginTopLevelMenu("Help"))
            {
                MenuSeparator("Documentation");

                if (MenuItem("Welcome", ICON_MDI_HOME))
                    m_documentService.Open<WelcomeDocument>();
                MenuItem("API Reference  " ICON_MDI_OPEN_IN_NEW, ICON_MDI_BOOK_OPEN);
                MenuItem("Tutorials  " ICON_MDI_OPEN_IN_NEW, ICON_MDI_BOOK_OPEN_VARIANT);
                MenuItem("Report Bug  " ICON_MDI_OPEN_IN_NEW, ICON_MDI_BUG);

                MenuSeparator("Application");

                if (MenuItem("Licenses"))
                {

                }

                if (MenuItem("About", ICON_MDI_HELP_CIRCLE))
                {
                    m_dialogService.Open<ChoiceDialog>().Configure(
                        "About Harvest",
                        "Harvest Engine v1.0.0\n\nCopyright© 2021 Chad Engler, All rights reserved.",
                        ChoiceDialog::Button::OK);
                }

                EndTopLevelMenu();
            }

            MenuSystemButtons(view, m_menuHitArea);

            // If we're in the main menu bar, not over anything more specific, set us to draggable.
            if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
                m_menuHitArea = window::ViewHitArea::Draggable;

            EndAppMainMenuBar();
        }

        if (!focused)
            ImGui::PopStyleColor(2);
    }

    void WorkspaceService::ShowAppStatusBar()
    {
        const float dpiScale = ImGui::GetWindowDpiScale();

        if (BeginAppStatusBar())
        {
            StatusBarButton(ICON_MDI_FILE_TREE " Asset Browser " ICON_MDI_CHEVRON_UP);
            StatusBarButton(ICON_MDI_CONSOLE_LINE " Console " ICON_MDI_CHEVRON_UP);

            ImGui::Dummy(ImVec2(16.0f, 0) * dpiScale);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f * dpiScale, ImGui::GetStyle().FramePadding.y));

            String buf;

            buf.Clear();
            fmt::format_to(Appender(buf), ICON_MDI_ALERT_OCTAGON " {}", m_logService.GetNumEntries(LogLevel::Error));
            StatusBarButton(buf.Data());

            buf.Clear();
            fmt::format_to(Appender(buf), ICON_MDI_ALERT " {}", m_logService.GetNumEntries(LogLevel::Warn));
            StatusBarButton(buf.Data());

            buf.Clear();
            fmt::format_to(Appender(buf), ICON_MDI_INFORMATION " {}", m_logService.GetNumEntries(LogLevel::Info));
            StatusBarButton(buf.Data());

            ImGui::PopStyleVar();

            if (m_taskService.IsEmpty())
            {
                constexpr char ReadyText[] = ICON_MDI_CHECK " Ready";

                ImGuiStyle& style = ImGui::GetStyle();
                const float width = ImGui::CalcTextSize(ReadyText).x + (style.FramePadding.x * 3);
                ImGui::SameLine(ImGui::GetWindowWidth() - width);
                ImGui::Text(ReadyText);
            }
            else
            {
                const uint32_t pending = m_taskService.PendingSize();
                const uint32_t running = m_taskService.RunningSize();

                ProgressSpinner();
                ImGui::Text("Running %u tasks (%u pending)...", running, pending);
            }

            EndAppStatusBar();
        }
    }
}
