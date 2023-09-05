// Copyright Chad Engler

#include "he/core/signal.h"
#include "he/core/types.h"
#include "he/editor/services/app_args_service.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/services/imgui_service.h"
#include "he/editor/services/render_service.h"
#include "he/editor/services/settings_service.h"
#include "he/editor/services/task_service.h"
#include "he/editor/services/workspace_service.h"
#include "he/window/event.h"
#include "he/window/view.h"

namespace he::editor
{
    class ProjectView
    {
    public:
        ProjectView(
            AppArgsService& appArgsService,
            AssetService& assetService,
            EditorData& editorData,
            ImGuiService& imguiService,
            RenderService& renderService,
            SettingsService& settingsService,
            TaskService& taskService,
            WorkspaceService& workspaceService) noexcept;

        bool Initialize();
        void Terminate();

        void OnEvent(const window::Event& ev);
        void Show();

        window::ViewHitArea HitTest(const Vec2i& point);
        window::ViewDropEffect GetDropEffect();

        window::View* GetView() const { return m_view; }

    private:
        bool CreateView();

        void OnViewResized(window::View* view, const Vec2i& size);
        void OnViewRequestClose(window::View* view);

    private:
        AppArgsService& m_appArgsService;
        AssetService& m_assetService;
        EditorData& m_editorData;
        ImGuiService& m_imguiService;
        RenderService& m_renderService;
        SettingsService& m_settingsService;
        TaskService& m_taskService;
        WorkspaceService& m_workspaceService;

        bool m_initialized{ false };
        window::View* m_view{ nullptr };
    };
}
