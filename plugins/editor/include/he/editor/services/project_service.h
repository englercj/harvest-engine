// Copyright Chad Engler

#pragma once

#include "he/core/signal.h"
#include "he/core/string.h"
#include "he/editor/services/directory_service.h"
#include "he/editor/services/platform_service.h"
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

        bool Create(const char* name, const char* path, const char* assetRoot);
        bool Open(const char* path);
        bool Close();

        bool Reload();
        bool Save();

        bool IsOpen() const { return !m_projectPath.IsEmpty() && m_project.IsValid(); }

        Project::Builder& GetProject() { return m_project; }
        const Project::Builder& GetProject() const { return m_project; }

        const String& ProjectPath() const { return m_projectPath; }
        String DataDir() const;

    public:
        using OnLoadSignal = Signal<void()>;
        using OnUnloadSignal = Signal<void()>;

        OnLoadSignal& OnLoad() { return m_onLoadSignal; }
        OnUnloadSignal& OnUnload() { return m_onUnloadSignal; }

    private:
        DirectoryService& m_directoryService;

        schema::Builder m_builder{};
        Project::Builder m_project{};
        String m_projectPath{};

        OnLoadSignal m_onLoadSignal{};
        OnUnloadSignal m_onUnloadSignal{};
    };
}
