// Copyright Chad Engler

#include "he/editor/commands/open_project_command.h"

namespace he::editor
{
    OpenProjectCommand::OpenProjectCommand(
        PlatformService& platformService,
        ProjectService& projectService,
        UniquePtr<OpenProjectFileCommand> openProjectFileCommand) noexcept
        : m_platformService(platformService)
        , m_projectService(projectService)
        , m_openProjectFileCommand(Move(openProjectFileCommand))
    {}

    bool OpenProjectCommand::CanRun() const
    {
        return !m_projectService.IsOpen();
    }

    void OpenProjectCommand::Run()
    {
        FileDialogConfig config{};
        config.filters = ProjectFilters;
        config.filterCount = HE_LENGTH_OF(ProjectFilters);

        String path;
        if (m_platformService.OpenFileDialog(path, config))
        {
            m_openProjectFileCommand->SetPath(path.Data());
            m_openProjectFileCommand->Run();
        }
    }
}
