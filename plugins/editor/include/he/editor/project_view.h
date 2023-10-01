// Copyright Chad Engler

#include "he/core/signal.h"
#include "he/core/types.h"
#include "he/editor/services/app_args_service.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/imgui_service.h"
#include "he/editor/services/render_service.h"
#include "he/editor/services/settings_service.h"
#include "he/editor/services/task_service.h"
#include "he/editor/widgets/app_frame.h"
#include "he/window/event.h"
#include "he/window/view.h"

namespace he::editor
{
    class ProjectView
    {
    public:
        ProjectView(
            AppArgsService& appArgsService,
            DialogService& dialogService,
            EditorData& editorData,
            ImGuiService& imguiService,
            RenderService& renderService,
            SettingsService& settingsService,
            TaskService& taskService,
            UniquePtr<AppFrame> appFrame) noexcept;

        bool Initialize();
        void Terminate();

        void OnEvent(const window::Event& ev);
        void Tick();

        window::ViewHitArea HitTest(const Vec2i& point);
        window::ViewDropEffect GetDropEffect();

        window::View* GetView() const { return m_view; }

    private:
        void Show();
        void ShowMainMenu();
        bool CreateView();

        void OnViewResized(window::View* view, const Vec2i& size);
        void OnViewRequestClose(window::View* view);

    private:
        AppArgsService& m_appArgsService;
        DialogService& m_dialogService;
        EditorData& m_editorData;
        ImGuiService& m_imguiService;
        RenderService& m_renderService;
        SettingsService& m_settingsService;
        TaskService& m_taskService;

        UniquePtr<AppFrame> m_appFrame;

        bool m_initialized{ false };
        window::View* m_view{ nullptr };
    };
}
