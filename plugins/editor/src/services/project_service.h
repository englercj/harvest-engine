// Copyright Chad Engler

#pragma once

#include "directory_service.h"
#include "platform_service.h"
#include "schema/project.hsc.h"

#include "he/core/string.h"
#include "he/schema/layout.h"

namespace he::editor
{
    using Project = schema::Project;

    constexpr const char ProjectExtension[] = ".he_project";
    constexpr const FileDialogFilter ProjectFilters[] =
    {
        { "Harvest Project", "*.he_project" },
    };

    class ProjectService
    {
    public:
        ProjectService(DirectoryService& directoryService);

        bool Create(const char* name, const char* path);
        bool Open(const char* path);
        bool Close();

        bool Reload();
        bool Save();

        bool IsOpen() const { return !m_projectPath.IsEmpty(); }

        Project::Builder& GetProject() { return m_project; }
        const Project::Builder& GetProject() const { return m_project; }

        String GetResourceDir() const;

    private:
        DirectoryService& m_directoryService;

        he::schema::Builder m_builder{};
        Project::Builder m_project{};
        String m_projectPath{};
    };
}
