// Copyright Chad Engler

#pragma once

#include "he/core/signal.h"
#include "he/core/string.h"
#include "he/editor/services/directory_service.h"
#include "he/editor/services/platform_service.h"
#include "he/editor/schema/plugin.hsc.h"
#include "he/editor/schema/project.hsc.h"
#include "he/schema/layout.h"

namespace he::editor
{
    constexpr const char ProjectExtension[] = ".he_project";
    constexpr const FileDialogFilter ProjectFilters[] =
    {
        { "Harvest Project", "*.he_project" },
    };

    class ProjectService
    {
    public:
        ProjectService(DirectoryService& directoryService) noexcept;

        bool Open(const char* path);
        bool Close();

        bool Reload();
        bool Save() const;

        bool IsOpen() const { return !m_projectPath.IsEmpty() && m_builder.Root().IsValid(); }

        Project::Builder Project() { return m_builder.Root(); }
        Project::Reader Project() const { return m_builder.Root(); }
        Span<const schema::TypedBuilder<editor::Plugin>> Plugins() const { return m_plugins; }

        const String& ProjectPath() const { return m_projectPath; }
        String DataDir() const;

    public:
        using OnLoadSignal = Signal<void()>;
        using OnUnloadSignal = Signal<void()>;

        OnLoadSignal& OnLoad() { return m_onLoadSignal; }
        OnUnloadSignal& OnUnload() { return m_onUnloadSignal; }

    private:
        void ReadPluginFiles();

    private:
        DirectoryService& m_directoryService;

        schema::TypedBuilder<editor::Project> m_builder{};
        String m_projectPath{};

        Vector<schema::TypedBuilder<editor::Plugin>> m_plugins{};

        OnLoadSignal m_onLoadSignal{};
        OnUnloadSignal m_onUnloadSignal{};
    };
}
