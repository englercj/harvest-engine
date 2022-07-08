// Copyright Chad Engler

#pragma once

#include "directory_service.h"
#include "platform_service.h"
#include "schema/project.hsc.h"

#include "he/core/signal.h"
#include "he/core/string.h"
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
        ProjectService(DirectoryService& directoryService);

        bool Create(const char* name, const char* path, const char* assetRoot);
        bool Open(const char* path);
        bool Close();

        bool Reload();
        bool Save();

        bool IsOpen() const { return !m_projectPath.IsEmpty() && m_project.IsValid(); }

        schema::Project::Builder& Project() { return m_project; }
        const schema::Project::Builder& Project() const { return m_project; }

        const String& ProjectPath() const { return m_projectPath; }
        String DataDir() const;

    public:
        using OnLoadSignal = Signal<void()>;
        using OnUnloadSignal = Signal<void()>;

        OnLoadSignal& OnLoad() { return m_onLoadSignal; }
        OnUnloadSignal& OnUnload() { return m_onUnloadSignal; }

    private:
        DirectoryService& m_directoryService;

        he::schema::Builder m_builder{};
        schema::Project::Builder m_project{};
        String m_projectPath{};

        OnLoadSignal m_onLoadSignal{};
        OnUnloadSignal m_onUnloadSignal{};
    };
}
