// Copyright Chad Engler

#include "editor_app.h"

#include "he/core/async_file.h"

namespace he::editor
{
    EditorApp::EditorApp(
        DirectoryService& directoryService,
        FileLoaderService& fileLoaderService,
        ImGuiService& imguiService,
        LogService& logService,
        MainWindowService& mainWindowService,
        RenderService& renderService,
        SettingsService& settingsService,
        TaskService& taskService,
        WorkspaceService& workspaceService)
        : m_directoryService(directoryService)
        , m_fileLoaderService(fileLoaderService)
        , m_imguiService(imguiService)
        , m_logService(logService)
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

        switch (ev.type)
        {
            case window::EventType::Initialized:
            {
                const auto& evt = static_cast<const window::InitializedEvent&>(ev);
                if (!OnViewInitialized(evt.view))
                    m_mainWindowService.Quit(1);
                break;
            }
            case window::EventType::ViewRequestClose:
            {
                const auto& evt = static_cast<const window::ViewRequestCloseEvent&>(ev);
                OnViewTerminated(evt.view);
                break;
            }
            case window::EventType::ViewResized:
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
        if (m_mainWindowService.GetView() != view || !m_initialized)
            return window::ViewHitArea::Normal;

        return m_workspaceService.GetHitArea(point);
    }

    bool EditorApp::OnViewInitialized(window::View* view)
    {
        if (m_initialized)
            return true;

        m_initialized = true;

        m_mainWindowService.SetView(view);

        // Initialize required services
        if (!m_directoryService.CreateAll())
            return false;

        if (!m_logService.Initialize())
            return false;

        if (!m_fileLoaderService.Initialize())
            return false;

        if (!m_renderService.Initialize(view))
            return false;

        if (!m_imguiService.Initialize(view))
            return false;

        if (!m_taskService.Initialize())
            return false;

        AsyncFileIOConfig config;
        config.executor = &m_taskService;
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

        ShutdownAsyncFileIO();

        m_settingsService.Save();

        m_taskService.Terminate();
        m_imguiService.Terminate();
        m_renderService.Terminate();
        m_mainWindowService.Quit(0);
    }
}
