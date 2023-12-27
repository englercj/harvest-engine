// Copyright Chad Engler

#pragma once

#include "he/core/signal.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
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
        struct PluginEntry
        {
            String filePath;
            schema::TypedBuilder<editor::Plugin> plugin;
        };

    public:
        ProjectService(DirectoryService& directoryService) noexcept;

        bool CreateAndOpen(StringView projectName, StringView projectDir, StringView enginePath);

        bool Open(StringView path);
        bool Close();

        bool Reload();
        bool Save() const;

        bool IsOpen() const { return !m_projectPath.IsEmpty() && m_builder.Root().IsValid(); }

        Project::Builder Project() { return m_builder.Root(); }
        Project::Reader Project() const { return m_builder.Root(); }
        Span<const PluginEntry> Plugins() const { return m_plugins; }

        const String& ProjectPath() const { return m_projectPath; }
        String DataDir() const;

    public:
        using OnLoadSignal = Signal<void()>;
        using OnUnloadSignal = Signal<void()>;

        OnLoadSignal& OnLoad() { return m_onLoadSignal; }
        OnUnloadSignal& OnUnload() { return m_onUnloadSignal; }

    private:
        bool LoadProjectInternal();
        void GenerateProjectId();
        bool ReadPluginFiles();

    private:
        DirectoryService& m_directoryService;

        schema::TypedBuilder<editor::Project> m_builder{};
        String m_projectPath{};

        Vector<PluginEntry> m_plugins{};

        OnLoadSignal m_onLoadSignal{};
        OnUnloadSignal m_onUnloadSignal{};
    };
}
