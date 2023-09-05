// Copyright Chad Engler

#pragma once

#include "he/core/module_registry.h"
#include "he/core/types.h"
#include "he/editor/editor_data.h"
#include "he/editor/editor_view.h"
#include "he/editor/project_view.h"
#include "he/editor/services/app_args_service.h"
#include "he/editor/services/directory_service.h"
#include "he/editor/services/log_service.h"
#include "he/editor/services/project_service.h"
#include "he/editor/services/task_service.h"
#include "he/window/application.h"

namespace he::window { struct Event; class View; }

namespace he::editor
{
    class EditorApp : public window::Application
    {
    public:
        EditorApp(
            AppArgsService& appArgsService,
            DirectoryService& directoryService,
            EditorData& editorData,
            EditorView& editorView,
            LogService& logService,
            ModuleRegistry& moduleRegistry,
            ProjectService& projectService,
            ProjectView& projectView,
            TaskService& taskService) noexcept;

        bool Initialize(int argc, char* argv[]);
        void Terminate();

        int Run();

    private:
        void OnEvent(const window::Event& ev) override;
        void OnTick() override;

        window::ViewHitArea OnHitTest(window::View* view, const Vec2i& point) override;
        window::ViewDropEffect OnDragging(window::View* view) override;

        void OnProjectLoaded();
        void OnProjectUnloaded();

    private:
        AppArgsService& m_appArgsService;
        DirectoryService& m_directoryService;
        EditorData& m_editorData;
        EditorView& m_editorView;
        LogService& m_logService;
        ModuleRegistry& m_moduleRegistry;
        ProjectService& m_projectService;
        ProjectView& m_projectView;
        TaskService& m_taskService;

        ProjectService::OnLoadSignal::Binding m_onProjectLoadedBinding;
        ProjectService::OnUnloadSignal::Binding m_onProjectUnloadedBinding;
    };
}
