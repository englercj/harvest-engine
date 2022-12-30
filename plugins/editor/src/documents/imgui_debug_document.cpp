// Copyright Chad Engler

#include "imgui_debug_document.h"

#include "services/asset_edit_service.h"
#include "widgets/buttons.h"
#include "widgets/progress.h"
#include "widgets/property_grid.h"
#include "widgets/misc.h"

#include "he/assets/types.h"
#include "he/schema/types.h"

#include "imgui.h"

namespace he::editor
{
    ImGuiDebugDocument::ImGuiDebugDocument(TypeEditUIService& editUIService) noexcept
        : m_editUIService(editUIService)
    {
        m_title = "ImGui Debugging";
    }

    void ImGuiDebugDocument::Show()
    {
        if (m_demoOpen)
        {
            // TODO: Need to prevent this from docking. When it does, things explode!
            // No way to specify window flags outside passing them to Begin() though so might need
            ImGui::ShowDemoWindow(&m_demoOpen);
        }

        if (ImGui::BeginTabBar("ImGui Debug Tabs"))
        {
            if (ImGui::BeginTabItem("Custom Widgets"))
            {
                ShowCustomWidgetsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Property Grid"))
            {
                ShowPropertyGridTab();
                ImGui::EndTabItem();
            }

            ImGui::BeginDisabled(m_demoOpen);
            if (ImGui::TabItemButton("ImGui Tools " ICON_MDI_OPEN_IN_NEW, ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
            {
                m_demoOpen = true;
            }
            ImGui::EndDisabled();

            ImGui::EndTabBar();
        }
    }

    void ImGuiDebugDocument::ShowCustomWidgetsTab()
    {
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

    void ImGuiDebugDocument::ShowPropertyGridTab()
    {
        static SchemaEditContext s_ctx{ assets::schema::Asset::Reader{} };

        assets::schema::Asset::Builder asset = s_ctx.Data().As<assets::schema::Asset>();
        if (asset.GetData().IsNull())
        {
            FillUuidV4(asset.InitUuid());
            asset.InitType(assets::schema::Texture2D::AssetTypeName);
            asset.InitName("Test Texture");

            assets::schema::Texture2D::Builder tex = asset.GetBuilder()->AddStruct<assets::schema::Texture2D>();
            asset.GetData().Set(tex);

            asset.GetBuilder()->SetRoot(asset);
        }

        SchemaEdit edit;
        PropertyGrid(s_ctx.Data().AsReader(), m_editUIService, edit);

        s_ctx.PushEdit(Move(edit));
    }
}
