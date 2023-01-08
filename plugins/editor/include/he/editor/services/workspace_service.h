// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/document_service.h"
#include "he/editor/services/imgui_service.h"
#include "he/editor/services/log_service.h"
#include "he/editor/services/main_window_service.h"
#include "he/editor/services/panel_service.h"
#include "he/editor/services/platform_service.h"
#include "he/editor/services/task_service.h"
#include "he/editor/commands/open_project_command.h"
#include "he/window/view.h"

namespace he::editor
{
    class Document;

    class WorkspaceService
    {
    public:
        WorkspaceService(
            AssetService& assetService,
            DialogService& dialogService,
            DocumentService& documentService,
            ImGuiService& imguiService,
            LogService& logService,
            MainWindowService& mainWindowService,
            PanelService& panelService,
            PlatformService& platformService,
            ProjectService& projectService,
            TaskService& taskService,
            UniquePtr<OpenProjectCommand> openProjectCommand) noexcept;

        void Show();

        window::ViewHitArea GetHitArea(const Vec2i& point) const;

    private:
        void ShowAppMenuBar();
        void ShowAppStatusBar();

    private:
        AssetService& m_assetService;
        DialogService& m_dialogService;
        DocumentService& m_documentService;
        ImGuiService& m_imguiService;
        LogService& m_logService;
        MainWindowService& m_mainWindowService;
        PanelService& m_panelService;
        PlatformService& m_platformService;
        ProjectService& m_projectService;
        TaskService& m_taskService;

        UniquePtr<OpenProjectCommand> m_openProjectCommand;

        window::ViewHitArea m_menuHitArea{ window::ViewHitArea::Normal };
    };
}
