// Copyright Chad Engler

#include "he/editor/services/dialog_service.h"

#include "he/editor/widgets/menu.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    void DialogService::DestroyClosedDialogs()
    {
        for (uint32_t i = 0; i < m_dialogs.Size();)
        {
            UniquePtr<Dialog>& dialog = m_dialogs[i];

            if (dialog->IsClosing())
            {
                m_dialogs.Erase(i, 1);
            }
            else
            {
                ++i;
            }
        }
    }

    void DialogService::ShowDialogs()
    {
        const float dpiScale = ImGui::GetWindowDpiScale();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f * dpiScale);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::GetStyle().Colors[ImGuiCol_Header]);

        for (UniquePtr<Dialog>& dialog : m_dialogs)
        {
            const char* label = dialog->Label();
            const ImGuiID id = ImGui::GetID(label);

            if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_None))
            {
                ImGui::OpenPopup(id);
            }

            ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal(label, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                dialog->ShowContent();

                ImGui::NewLine();
                if (ImGui::BeginChild("##button_well", ImVec2(0, ImGui::GetFrameHeightWithSpacing())))
                {
                    dialog->ShowButtons();
                }
                ImGui::EndChild();

                if (dialog->IsClosing())
                    ImGui::CloseCurrentPopup();

                ImGui::EndPopup();
            }
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void DialogService::CloseAll()
    {
        for (UniquePtr<Dialog>& dialog : m_dialogs)
        {
            dialog->Close();
        }
    }
}
