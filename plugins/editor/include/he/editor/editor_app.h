// Copyright Chad Engler

#pragma once

#include "he/editor/editor_data.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/services/imgui_service.h"
#include "he/editor/services/main_window_service.h"
#include "he/editor/services/render_service.h"
#include "he/editor/services/settings_service.h"
#include "he/editor/services/task_service.h"
#include "he/editor/services/workspace_service.h"
#include "he/window/application.h"
#include "he/window/view.h"

namespace he::editor
{
    class EditorApp : public window::Application
    {
    public:
        EditorApp(
            AssetService& assetService,
            ImGuiService& imguiService,
            MainWindowService& mainWindowService,
            RenderService& renderService,
            SettingsService& settingsService,
            TaskService& taskService,
            WorkspaceService& workspaceService) noexcept;

        void OnEvent(const window::Event& ev) override;
        void OnTick() override;

        window::ViewHitArea OnHitTest(window::View* view, const Vec2i& point) override;
        window::ViewDropEffect OnDragging(window::View* view) override;

    private:
        bool OnViewInitialized(window::View* view);
        void OnViewResized(window::View* view, const Vec2i& size);
        void OnViewTerminated(window::View* view);

    private:
        AssetService& m_assetService;
        ImGuiService& m_imguiService;
        MainWindowService& m_mainWindowService;
        RenderService& m_renderService;
        SettingsService& m_settingsService;
        TaskService& m_taskService;
        WorkspaceService& m_workspaceService;

        bool m_initialized{ false };
    };
}
