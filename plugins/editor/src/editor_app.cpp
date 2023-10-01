// Copyright Chad Engler

#include "he/editor/editor_app.h"

#include "he/core/async_file.h"

#include "he/window/event.h"

#include <iostream>

namespace he::editor
{
    EditorApp::EditorApp(
        AppArgsService& appArgsService,
        DirectoryService& directoryService,
        EditorData& editorData,
        EditorView& editorView,
        LogService& logService,
        ModuleRegistry& moduleRegistry,
        ProjectService& projectService,
        ProjectView& projectView,
        SettingsService& settingsService,
        TaskService& taskService) noexcept
        : m_appArgsService(appArgsService)
        , m_directoryService(directoryService)
        , m_editorData(editorData)
        , m_editorView(editorView)
        , m_logService(logService)
        , m_moduleRegistry(moduleRegistry)
        , m_projectService(projectService)
        , m_projectView(projectView)
        , m_settingsService(settingsService)
        , m_taskService(taskService)
    {}

    bool EditorApp::Initialize(int argc, char* argv[])
    {
        // Initialize the directory service first so that the log service has existing directories
        // to write data into.
        if (!m_directoryService.CreateAll())
            return false;

        // Initialize the log service as early as possible, so we can capture as many logs as possible.
        // In particular we want to capture any failed module loads to the log file.
        if (!m_logService.Initialize())
            return false;

        // Create and startup the static modules that were registered by any linked libraries.
        // TODO: load dynamic modules
        m_moduleRegistry.LoadStaticModules();
        if (!m_moduleRegistry.StartupAllModules())
            return false;

        // Create and initialize the application args service so our editor app can read them.
        if (!m_appArgsService.Initialize(argc, argv) || m_appArgsService.Flags().help)
        {
            const String help = m_appArgsService.Help();
            std::cerr << help.Data() << std::endl;
            return false;
        }

        // Now that modules are started create the editor data necessary to run the application and
        // kick off the app.
        m_editorData.device = window::Device::Create();
        if (!m_editorData.device)
            return false;

        // Start the task service so the async file IO system can use it.
        if (!m_taskService.Initialize())
            return false;

        // Startup the async file io system for the the application to use.
        AsyncFileIOConfig config{};
        config.threadpool.executor = &m_taskService;
        const Result r = StartupAsyncFileIO(config);
        if (!r)
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to startup async file IO system."), HE_KV(result, r));
            return false;
        }

        m_onProjectLoadedBinding = m_projectService.OnLoad().Attach<&EditorApp::OnProjectLoaded>(this);
        m_onProjectUnloadedBinding = m_projectService.OnUnload().Attach<&EditorApp::OnProjectUnloaded>(this);
        return true;
    }

    void EditorApp::Terminate()
    {
        m_onProjectLoadedBinding.Detach();
        m_onProjectUnloadedBinding.Detach();

        m_projectView.Terminate();
        m_editorView.Terminate();

        ShutdownAsyncFileIO();

        m_taskService.Terminate();
        window::Device::Destroy(m_editorData.device);
        m_editorData.device = nullptr;
        m_moduleRegistry.ShutdownAllModules();
        m_moduleRegistry.UnloadAllModules();
        m_logService.Terminate();
    }

    int EditorApp::Run()
    {
        return m_editorData.device->Run(*this);
    }

    void EditorApp::OnEvent(const window::Event& ev)
    {
        if (ev.kind == window::EventKind::Initialized)
        {
            m_projectView.Initialize();
        }
        else
        {
            m_editorView.OnEvent(ev);
            m_projectView.OnEvent(ev);
        }
    }

    void EditorApp::OnTick()
    {
        m_editorView.Tick();
        m_projectView.Tick();
    }

    window::ViewHitArea EditorApp::OnHitTest(window::View* view, const Vec2i& point)
    {
        if (view == m_editorView.GetView())
            return m_editorView.HitTest(point);
        else if (view == m_projectView.GetView())
            return m_projectView.HitTest(point);
        else
            return window::ViewHitArea::NotInView;
    }

    window::ViewDropEffect EditorApp::OnDragging(window::View* view)
    {
        if (view == m_editorView.GetView())
            return m_editorView.GetDropEffect();
        else if (view == m_projectView.GetView())
            return m_projectView.GetDropEffect();
        else
            return window::ViewDropEffect::Reject;
    }

    void EditorApp::OnProjectLoaded()
    {
        m_projectView.Terminate();
        m_editorView.Initialize();
    }

    void EditorApp::OnProjectUnloaded()
    {
        // TODO: This may happen when we're closing the app, in which case we don't want to
        // re-initialize the project view.
        m_editorView.Terminate();
        m_projectView.Initialize();
    }
}
