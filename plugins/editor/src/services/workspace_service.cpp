// Copyright Chad Engler

#include "workspace_service.h"

#include "di.h"
#include "dialogs/choice_dialog.h"
#include "documents/document.h"
#include "documents/stats_document.h"
#include "documents/imgui_stack_tool_document.h"
#include "documents/imgui_style_editor_document.h"
#include "documents/welcome_document.h"
#include "fonts/IconsFontAwesome5Pro.h"
#include "widgets/menu.h"

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
        MainWindowService& mainWindowService,
        PlatformService& platformService)
        : m_dialogService(dialogService)
        , m_documentService(documentService)
        , m_imguiService(imguiService)
        , m_mainWindowService(mainWindowService)
        , m_platformService(platformService)
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

                if (BeginMenu("Create New Asset", ICON_FA_FILE_PLUS))
                {
                    MenuItem("Texture1");
                    MenuItem("Texture2");
                    MenuItem("Texture3");

                    EndMenu();
                }
                MenuItem("Open Asset...", ICON_FA_FOLDER_OPEN, "Ctrl+O", false, false);

                if (BeginMenu("Open Recent Asset", ICON_FA_HOURGLASS_HALF))
                {
                    MenuItem("Some Asset");

                    EndMenu();
                }
                MenuItem("Import Asset...", ICON_FA_FILE_IMPORT);
                MenuItem("Export Asset...", ICON_FA_FILE_EXPORT);

                // Save menu
                MenuSeparator("Save");

                MenuItem("Save", ICON_FA_SAVE, "Ctrl+S");
                MenuItem("Save As...", nullptr, "Ctrl+Alt+S");
                MenuItem("Save All", nullptr, "Ctrl+Shift+S");

                // Project menu
                MenuSeparator("Project");

                if (MenuItem("Open Project...", ICON_FA_FOLDER_OPEN, nullptr, false, false))
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

                if (MenuItem("Exit", ICON_FA_TIMES_CIRCLE, "Alt+F4"))
                    view->RequestClose();

                EndTopLevelMenu();
            }

            if (BeginTopLevelMenu("Edit"))
            {
                MenuSeparator("History");

                MenuItem("Undo", ICON_FA_UNDO, "Ctrl+Z");
                MenuItem("Redo", ICON_FA_REDO, "Ctrl+Y");

                MenuSeparator("Edit");

                MenuItem("Cut", ICON_FA_CUT, "Ctrl+X");
                MenuItem("Copy", ICON_FA_CLIPBOARD, "Ctrl+C");
                MenuItem("Paste", ICON_FA_PASTE, "Ctrl+V");
                MenuItem("Clone", ICON_FA_CLONE, "Ctrl+V");
                MenuItem("Delete", ICON_FA_TRASH, "Ctrl+V");

                MenuSeparator("Configuration");

                MenuItem("User Settings...", ICON_FA_USER_COG);
                MenuItem("Project Settings...", ICON_FA_SLIDERS_V);
                MenuItem("Plugins...", ICON_FA_PLUG);

                EndTopLevelMenu();
            }

            if (BeginTopLevelMenu("Tools"))
            {
                if (MenuItem("ImGui Stack Tool"))
                    m_documentService.Open<ImGuiStackToolDocument>();

                if (MenuItem("ImGui Style Editor"))
                    m_documentService.Open<ImGuiStyleEditorDocument>();

                if (MenuItem("Stats"))
                    m_documentService.Open<StatsDocument>();

                EndTopLevelMenu();
            }

            if (BeginTopLevelMenu("Help"))
            {
                MenuSeparator("Documentation");

                if (MenuItem("Welcome", ICON_FA_HOME))
                    m_documentService.Open<WelcomeDocument>();
                MenuItem("API Reference  " ICON_FA_EXTERNAL_LINK_ALT, ICON_FA_BOOK);
                MenuItem("Tutorials  " ICON_FA_EXTERNAL_LINK_ALT, ICON_FA_BOOK_READER);
                MenuItem("Report Bug  " ICON_FA_EXTERNAL_LINK_ALT, ICON_FA_BUG);

                MenuSeparator("Application");

                if (MenuItem("About", ICON_FA_QUESTION_CIRCLE))
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
            StatusBarButton(ICON_FA_FOLDER_TREE " Asset Browser " ICON_FA_CHEVRON_UP);
            StatusBarButton(ICON_FA_ALIGN_JUSTIFY " Console " ICON_FA_CHEVRON_UP);

            ImGui::Dummy(ImVec2(16.0f, 0) * dpiScale);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f * dpiScale, ImGui::GetStyle().FramePadding.y));

            StatusBarButton(ICON_FA_TIMES_OCTAGON " 01");
            StatusBarButton(ICON_FA_EXCLAMATION_TRIANGLE " 0");
            StatusBarButton(ICON_FA_INFO_CIRCLE " 0");

            ImGui::PopStyleVar();

            EndAppStatusBar();
        }
    }
}
