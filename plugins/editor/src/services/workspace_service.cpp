// Copyright Chad Engler

#include "he/editor/services/workspace_service.h"

#include "he/core/fmt.h"
#include "he/editor/di.h"
#include "he/editor/dialogs/about_dialog.h"
#include "he/editor/documents/asset_browser_document.h"
#include "he/editor/documents/dev_console_document.h"
#include "he/editor/documents/document.h"
#include "he/editor/documents/stats_document.h"
#include "he/editor/documents/imgui_debug_document.h"
#include "he/editor/documents/log_document.h"
#include "he/editor/documents/welcome_document.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/widgets/animations.h"
#include "he/editor/widgets/menu.h"
#include "he/editor/widgets/progress.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    WorkspaceService::WorkspaceService(
        AssetService& assetService,
        DialogService& dialogService,
        DocumentService& documentService,
        ImGuiService& imguiService,
        LogService& logService,
        PanelService& panelService,
        PlatformService& platformService,
        ProjectService& projectService,
        TaskService& taskService,
        UniquePtr<AppFrame> appFrame,
        UniquePtr<OpenProjectCommand> openProjectCommand) noexcept
        : m_assetService(assetService)
        , m_dialogService(dialogService)
        , m_documentService(documentService)
        , m_imguiService(imguiService)
        , m_logService(logService)
        , m_panelService(panelService)
        , m_platformService(platformService)
        , m_projectService(projectService)
        , m_taskService(taskService)
        , m_appFrame(Move(appFrame))
        , m_openProjectCommand(Move(openProjectCommand))
    {}

    bool WorkspaceService::Initialize(window::View* view)
    {
        m_view = view;
        m_appFrame->SetView(view);
        return true;
    }

    void WorkspaceService::Terminate()
    {
        m_appFrame->SetView(nullptr);
        m_view = nullptr;
    }

    void WorkspaceService::Show()
    {
        m_documentService.DestroyClosedDocuments();
        m_dialogService.DestroyClosedDialogs();
        m_panelService.DestroyClosedPanels();

        ShowAppMenuBar();
        ShowAppStatusBar();

        m_documentService.ShowDocuments();
        m_dialogService.ShowDialogs();
        m_panelService.ShowPanels();
    }

    bool WorkspaceService::RequestClose()
    {
        // TODO: Don't close documents which have pending changes.
        m_documentService.CloseAll();
        m_documentService.DestroyClosedDocuments();

        m_dialogService.CloseAll();
        m_dialogService.DestroyClosedDialogs();

        m_panelService.Close();
        m_panelService.DestroyClosedPanels();
        return true;
    }

    window::ViewHitArea WorkspaceService::GetHitArea(const Vec2i& point) const
    {
        return m_appFrame->GetHitArea(point);
    }

    void WorkspaceService::ShowAppMenuBar()
    {
        if (!m_view)
            return;

        if (m_appFrame->BeginAppMainMenuBar())
        {
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
                MenuItem("Create ");
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
                MenuItem("Save All", ICON_MDI_CONTENT_SAVE_ALL, "Ctrl+Shift+S");

                // Project menu
                MenuSeparator("Project");

                MenuItem(*m_openProjectCommand);

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
                    m_view->RequestClose();

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
                if (MenuItem("ImGui Debug"))
                    m_documentService.Open<ImGuiDebugDocument>();

                if (MenuItem("Stats"))
                    m_documentService.Open<StatsDocument>();

                EndTopLevelMenu();
            }

            if (BeginTopLevelMenu("Help"))
            {
                MenuSeparator("Documentation");

                if (MenuItem("Welcome", ICON_MDI_HOME))
                    m_documentService.Open<WelcomeDocument>();

                // TODO
                MenuItem("API Reference  " ICON_MDI_OPEN_IN_NEW, ICON_MDI_BOOK_OPEN);
                MenuItem("Tutorials  " ICON_MDI_OPEN_IN_NEW, ICON_MDI_BOOK_OPEN_VARIANT);
                MenuItem("Report Bug  " ICON_MDI_OPEN_IN_NEW, ICON_MDI_BUG);

                MenuSeparator("Application");

                if (MenuItem("Licenses"))
                {
                    // TODO
                }

                if (MenuItem("About", ICON_MDI_HELP_CIRCLE))
                {
                    m_dialogService.Open<AboutDialog>();
                }

                EndTopLevelMenu();
            }

            Document* activeDocument = m_documentService.ActiveDocument();
            if (activeDocument)
                activeDocument->ShowMainMenu();

            m_appFrame->EndAppMainMenuBar();
        }
    }

    template <typename T>
    static void StatusBarPanel(PanelService& panelService, const char* openLabel, const char* closeLabel)
    {
        const bool isOpen = panelService.IsOpen<T>();
        const char* label = isOpen ? closeLabel : openLabel;
        if (StatusBarButton(label))
        {
            if (isOpen)
                panelService.Close();
            else
                panelService.Open<T>();
        }
    }

    void WorkspaceService::ShowAppStatusBar()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        if (BeginAppStatusBar())
        {
            constexpr const char* AssetBrowserOpen = ICON_MDI_FILE_TREE " Asset Browser " ICON_MDI_CHEVRON_UP;
            constexpr const char* AssetBrowserClose = ICON_MDI_FILE_TREE " Asset Browser " ICON_MDI_CHEVRON_DOWN;
            StatusBarPanel<AssetBrowserDocument>(m_panelService, AssetBrowserOpen, AssetBrowserClose);

            constexpr const char* DevConsoleOpen = ICON_MDI_CONSOLE_LINE " Developer Console " ICON_MDI_CHEVRON_UP;
            constexpr const char* DevConsoleClose = ICON_MDI_CONSOLE_LINE " Developer Console " ICON_MDI_CHEVRON_DOWN;
            StatusBarPanel<DevConsoleDocument>(m_panelService, DevConsoleOpen, DevConsoleClose);

            const bool isLogOpen = m_panelService.IsOpen<LogDocument>();
            static String s_buf;
            s_buf.Clear();
            FormatTo(s_buf, ICON_MDI_ALERT_OCTAGON " {} " ICON_MDI_ALERT " {} " ICON_MDI_INFORMATION " {} {}",
                m_logService.GetNumEntries(LogLevel::Error),
                m_logService.GetNumEntries(LogLevel::Warn),
                m_logService.GetNumEntries(LogLevel::Info),
                isLogOpen ? ICON_MDI_CHEVRON_DOWN : ICON_MDI_CHEVRON_UP);
            StatusBarPanel<LogDocument>(m_panelService, s_buf.Data(), s_buf.Data());

            s_buf.Clear();

            bool showSpinner = false;

            if (m_projectService.IsOpen() && !m_assetService.IsAssetDBReady())
            {
                showSpinner = true;
                s_buf = "Updating asset database...";
            }
            else if (!m_taskService.IsEmpty())
            {
                const uint32_t pending = m_taskService.PendingSize();
                const uint32_t running = m_taskService.RunningSize();

                showSpinner = true;
                FormatTo(s_buf, "Running {} tasks ({} pending)...", running, pending);
            }
            else
            {
                s_buf = ICON_MDI_CHECK " Ready";
            }

            ImVec2 spinnerSize = showSpinner ? ProgressSpinnerSize() : ImVec2{};
            const float width = spinnerSize.x + ImGui::CalcTextSize(s_buf.Data()).x + (style.FramePadding.x * 3);
            ImGui::SameLine(ImGui::GetWindowWidth() - width);

            if (showSpinner)
                ProgressSpinner();

            ImGui::TextUnformatted(s_buf.Data());

            EndAppStatusBar();
        }
    }
}
