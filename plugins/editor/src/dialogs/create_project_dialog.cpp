// Copyright Chad Engler

#include "create_project_dialog.h"

#include "services/project_service.h"
#include "widgets/buttons.h"
#include "widgets/input_text.h"

#include "he/core/string.h"

#include "imgui.h"

namespace he::editor
{
    CreateProjectDialog::CreateProjectDialog(
        PlatformService& platformService,
        ProjectService& projectService)
        : m_platformService(platformService)
        , m_projectService(projectService)
    {
        m_title = "Create Project";
    }

    void CreateProjectDialog::ShowContent()
    {
        ImGui::TextUnformatted("Project Name");
        InputText("##project_name", m_name);

        ImGui::NewLine();

        ImGui::TextUnformatted("Location");
        InputText("##project_path", m_path);
        ImGui::SameLine();
        if (ImGui::Button("Browse..."))
        {
            FileDialogConfig config{};
            config.filters = ProjectFilters;
            config.filterCount = HE_LENGTH_OF(ProjectFilters);

            String path;
            if (m_platformService.SaveFileDialog(config, path))
            {
                StringView ext = GetExtension(path);
                if (ext.IsEmpty() || ext != ProjectExtension)
                {
                    path += ProjectExtension;
                }

                m_path = Move(path);
            }
        }
    }

    void CreateProjectDialog::ShowButtons()
    {
        const bool canSave = !m_name.IsEmpty() && !m_path.IsEmpty();

        if (DialogButton("Save", canSave))
        {
            if (m_projectService.Create(m_name.Data(), m_path.Data()))
            {
                RequestClose();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
            RequestClose();
    }
}
