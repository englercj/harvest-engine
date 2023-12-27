// Copyright Chad Engler

#include "he/editor/dialogs/create_project_dialog.h"

#include "he/core/string.h"
#include "he/editor/dialogs/choice_dialog.h"
#include "he/editor/framework/imgui_theme.h"
#include "he/editor/services/project_service.h"
#include "he/editor/widgets/buttons.h"
#include "he/editor/widgets/input_text.h"
#include "he/editor/widgets/misc.h"

#include "imgui.h"

namespace he::editor
{
    CreateProjectDialog::CreateProjectDialog(
        DialogService& dialogService,
        PlatformService& platformService,
        ProjectService& projectService) noexcept
        : m_dialogService(dialogService)
        , m_platformService(platformService)
        , m_projectService(projectService)
    {
        m_title = "Create Project";
    }

    void CreateProjectDialog::ShowContent()
    {
        ImGui::TextUnformatted("Project Name");
        ImGui::SameLine();
        ShowHelpMarker("The human-readable name of your project.");
        InputText("##project_name", m_projectName);

        ImGui::NewLine();

        ImGui::TextUnformatted("Project Directory");
        ImGui::SameLine();
        ShowHelpMarker("This is the directory where your new project will be generated.");
        InputOpenFolder("##project_path", m_projectPath, m_platformService);

        ImGui::NewLine();

        ImGui::TextUnformatted("Engine Directory");
        ImGui::SameLine();
        ShowHelpMarker("This is the directory where the Harvest Engine lives.");
        InputOpenFolder("##engine_path", m_enginePath, m_platformService);
    }

    void CreateProjectDialog::ShowButtons()
    {
        const bool canSave = !m_projectName.IsEmpty() && !m_projectPath.IsEmpty() && !m_enginePath.IsEmpty();

        ImGui::BeginDisabled(!canSave);
        if (DialogButton("Create & Open"))
        {
            if (!m_projectService.CreateAndOpen(m_projectName, m_projectPath, m_enginePath))
            {
                m_dialogService.Open<ChoiceDialog>().Configure(
                    "Error",
                    "Failed to create project file. Check the log for details.",
                    ChoiceDialog::Button::OK);
            }
            Close();
        }
        ImGui::EndDisabled();
        if (!canSave && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("All fields are required to create a project.");

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
            Close();
    }
}
