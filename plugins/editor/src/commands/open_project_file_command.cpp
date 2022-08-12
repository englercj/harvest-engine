// Copyright Chad Engler

#include "open_project_file_command.h"

#include "dialogs/choice_dialog.h"

namespace he::editor
{
    OpenProjectFileCommand::OpenProjectFileCommand(
        DialogService& dialogService,
        ProjectService& projectService) noexcept
        : m_dialogService(dialogService)
        , m_projectService(projectService)
    {}

    bool OpenProjectFileCommand::CanRun() const
    {
        return !m_path.IsEmpty() && !m_projectService.IsOpen();
    }

    void OpenProjectFileCommand::Run()
    {
        if (!m_projectService.Open(m_path.Data()))
        {
            m_dialogService.Open<ChoiceDialog>().Configure(
                "Error",
                "Failed to open project file. Check the log for details.",
                ChoiceDialog::Button::OK);
        }
    }
}
