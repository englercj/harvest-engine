// Copyright Chad Engler

#include "imgui_widget_document.h"

#include "widgets/buttons.h"
#include "widgets/progress.h"
#include "widgets/misc.h"

#include "imgui.h"

namespace he::editor
{
    ImGuiWidgetDocument::ImGuiWidgetDocument()
    {
        m_title = "ImGui Custom Widgets";
    }

    void ImGuiWidgetDocument::Show()
    {
        ImGui::ShowDemoWindow();

        if (ImGui::CollapsingHeader("Buttons"))
        {
            static int s_clickCount = 0;
            if (LinkButton("Link Button"))
                ++s_clickCount;

            ImGui::SameLine();
            ImGui::Text("Clicked: %d", s_clickCount);

            static bool s_condEnabled = true;
            ImGui::BeginDisabled(!s_condEnabled);
            if (ImGui::Button("Condition Button"))
                s_condEnabled = !s_condEnabled;
            ImGui::EndDisabled();

            static bool s_dialogEnabled = true;
            ImGui::BeginDisabled(!s_dialogEnabled);
            if (DialogButton("Dialog Btn"))
                s_dialogEnabled = !s_dialogEnabled;
            ImGui::EndDisabled();

            if (ImGui::Button("Reset"))
            {
                s_clickCount = 0;
                s_condEnabled = true;
                s_dialogEnabled = true;
            }
        }

        if (ImGui::CollapsingHeader("Progress Spinner"))
        {
            static float s_thickness = 5.0f;
            static float s_radius = 0.0f;

            ProgressSpinner(s_thickness, s_radius);
            ImGui::SliderFloat("Thickness", &s_thickness, 0.0f, 10.0f);
            ImGui::SliderFloat("Radius", &s_radius, 0.0f, 64.0f);
        }

        if (ImGui::CollapsingHeader("Progress Bar"))
        {
            static float s_value = -1.0f;
            static bool s_showOverlay = false;
            static char s_overlay[32];

            ProgressBar(s_value, ImVec2(-FLT_MIN, 0), s_showOverlay ? s_overlay : nullptr);
            ImGui::SliderFloat("Value", &s_value, -1.0f, 1.0f);
            ImGui::Checkbox("Custom Overlay", &s_showOverlay);
            ImGui::InputText("Overlay", s_overlay, HE_LENGTH_OF(s_overlay));
        }

        if (ImGui::CollapsingHeader("Help marker"))
        {
            static char s_overlay[32]{ "This is the help text" };

            ShowHelpMarker(s_overlay);
            ImGui::InputText("Help Text", s_overlay, HE_LENGTH_OF(s_overlay));
        }

        if (ImGui::CollapsingHeader("Item Underline"))
        {
            ImGui::Text("Some underlined text.");
            SetItemUnderLine(ImColor(0xffffffff));
        }
    }
}
