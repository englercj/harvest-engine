// Copyright Chad Engler

#include "create_project_dialog.h"

#include "dialogs/choice_dialog.h"
#include "services/project_service.h"
#include "services/imgui_service.h"
#include "widgets/buttons.h"
#include "widgets/input_text.h"
#include "widgets/misc.h"

#include "he/core/string.h"

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
        InputText("##project_name", m_name);

        ImGui::NewLine();

        ImGui::TextUnformatted("Project Path");
        if (InputSaveFile("##project_path", m_path, m_platformService, ProjectFilters))
        {
            StringView ext = GetExtension(m_path);
            if (ext.IsEmpty() || ext != ProjectExtension)
            {
                m_path += ProjectExtension;
            }
            m_assetRoot.Clear();
        }

        ImGui::NewLine();

        if (ImGui::Checkbox("Override Asset Root Directory", &m_overrideAssetRoot))
        {
            m_assetRoot.Clear();
            m_errorMessage.Clear();
        }
        ImGui::SameLine();
        ShowHelpMarker(
            "By default assets live in the same directory as the project file.\n\n"
            "Because this value is stored in the project file and shared between all "
            "users of the project, it is stored a relative path to the project file. "
            "This means the path must be at or below the directory of the project file.");

        if (m_overrideAssetRoot)
        {
            if (m_path.IsEmpty())
            {
                ImGui::BeginDisabled();
            }

            ImGui::TextUnformatted("Asset Root");
            if (InputOpenFolder("##asset_root", m_assetRoot, m_platformService))
            {
                m_errorMessage.Clear();

                StringView parentDir = GetDirectory(m_path);
                const bool isSameOrChild = m_assetRoot == parentDir || IsChildPath(m_assetRoot, parentDir);
                if (!isSameOrChild || !MakeRelative(m_assetRoot, parentDir))
                {
                    m_errorMessage = "Asset root directory must be below the project file's directory.";
                    m_assetRoot.Clear();
                }
            }

            if (m_path.IsEmpty())
            {
                ImGui::EndDisabled();
            }
        }

        if (!m_errorMessage.IsEmpty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Color_Error);
            ImGui::TextUnformatted(m_errorMessage.Data());
            ImGui::PopStyleColor();
        }
    }

    void CreateProjectDialog::ShowButtons()
    {
        const bool canSave = !m_name.IsEmpty() && !m_path.IsEmpty();

        ImGui::BeginDisabled(!canSave);
        if (DialogButton("Save"))
        {
            if (!m_projectService.Create(m_name.Data(), m_path.Data(), m_assetRoot.Data()))
            {
                m_dialogService.Open<ChoiceDialog>().Configure(
                    "Error",
                    "Failed to create project file. Check the log for details.",
                    ChoiceDialog::Button::OK);
            }
            Close();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
            Close();
    }
}
