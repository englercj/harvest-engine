// Copyright Chad Engler

#pragma once

#include "editor_data.h"
#include "services/directory_service.h"
#include "services/imgui_service.h"
#include "services/log_service.h"
#include "services/main_window_service.h"
#include "services/render_service.h"
#include "services/settings_service.h"
#include "services/workspace_service.h"

#include "he/window/application.h"
#include "he/window/view.h"

namespace he::editor
{
    class EditorApp : public window::Application
    {
    public:
        EditorApp(
            DirectoryService& directoryService,
            ImGuiService& imguiService,
            LogService& logService,
            MainWindowService& mainWindowService,
            RenderService& renderService,
            SettingsService& settingsService,
            WorkspaceService& workspaceService);

        void OnEvent(const window::Event& ev) override;
        void OnTick() override;

        window::ViewHitArea OnHitTest(window::View* view, const Vec2i& point) override;

    private:
        bool OnViewInitialized(window::View* view);
        void OnViewResized(window::View* view, const Vec2i& size);
        void OnViewTerminated(window::View* view);

    private:
        DirectoryService& m_directoryService;
        ImGuiService& m_imguiService;
        LogService& m_logService;
        MainWindowService& m_mainWindowService;
        RenderService& m_renderService;
        SettingsService& m_settingsService;
        WorkspaceService& m_workspaceService;

        bool m_initialized{ false };
    };
}
