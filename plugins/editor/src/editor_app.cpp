// Copyright Chad Engler

#include "he/editor/editor_app.h"

#include "he/core/async_file.h"

namespace he::editor
{
    EditorApp::EditorApp(
        AssetService& assetService,
        ImGuiService& imguiService,
        MainWindowService& mainWindowService,
        RenderService& renderService,
        SettingsService& settingsService,
        TaskService& taskService,
        WorkspaceService& workspaceService) noexcept
        : m_assetService(assetService)
        , m_imguiService(imguiService)
        , m_mainWindowService(mainWindowService)
        , m_renderService(renderService)
        , m_settingsService(settingsService)
        , m_taskService(taskService)
        , m_workspaceService(workspaceService)
    {}

    void EditorApp::OnEvent(const window::Event& ev)
    {
        if (m_initialized)
            m_imguiService.OnEvent(ev);

        switch (ev.kind)
        {
            case window::EventKind::Initialized:
            {
                const auto& evt = static_cast<const window::InitializedEvent&>(ev);
                if (!OnViewInitialized(evt.view))
                    m_mainWindowService.Quit(1);
                break;
            }
            case window::EventKind::ViewRequestClose:
            {
                const auto& evt = static_cast<const window::ViewRequestCloseEvent&>(ev);
                OnViewTerminated(evt.view);
                break;
            }
            case window::EventKind::ViewResized:
            {
                const auto& evt = static_cast<const window::ViewResizedEvent&>(ev);
                OnViewResized(evt.view, evt.size);
                break;
            }
            default:
                break;
        }
    }

    void EditorApp::OnTick()
    {
        // Update the application UI
        m_imguiService.NewFrame();
        m_workspaceService.Show();
        m_imguiService.Update();

        // Perform rendering pass
        m_renderService.BeginFrame();
        m_imguiService.Render();
        m_renderService.EndFrame();
    }

    window::ViewHitArea EditorApp::OnHitTest(window::View* view, const Vec2i& point)
    {
        if (!m_initialized || m_mainWindowService.GetView() != view)
            return window::ViewHitArea::Normal;

        return m_workspaceService.GetHitArea(point);
    }

    window::ViewDropEffect EditorApp::OnDragging(window::View* view)
    {
        if (!m_initialized)
            return window::ViewDropEffect::Reject;

        return m_imguiService.GetDropEffect(view);
    }

    bool EditorApp::OnViewInitialized(window::View* view)
    {
        if (m_initialized)
            return true;

        m_initialized = true;

        m_mainWindowService.SetView(view);

        // Initialize required services
        if (!m_taskService.Initialize())
            return false;

        if (!m_assetService.Initialize())
            return false;

        // Startup rendering
        if (!m_renderService.Initialize(view))
            return false;

        if (!m_imguiService.Initialize(view))
            return false;

        AsyncFileIOConfig config;
        config.threadpool.executor = &m_taskService;
        StartupAsyncFileIO(config);

        // Failing to load settings is OK, we'll run with defaults and have an error in the log
        m_settingsService.Reload();
        return true;
    }

    void EditorApp::OnViewResized(window::View* view, const Vec2i& size)
    {
        if (m_mainWindowService.GetView() != view)
            return;

        m_renderService.SetSize(size);
    }

    void EditorApp::OnViewTerminated(window::View* view)
    {
        if (m_mainWindowService.GetView() != view)
            return;

        m_initialized = false;

        m_settingsService.Save();

        ShutdownAsyncFileIO();

        m_imguiService.Terminate();
        m_renderService.Terminate();
        m_assetService.Terminate();
        m_taskService.Terminate();

        m_mainWindowService.Quit(0);
    }
}
