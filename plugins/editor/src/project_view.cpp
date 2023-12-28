// Copyright Chad Engler

#include "he/editor/project_view.h"

#include "he/editor/dialogs/about_dialog.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/widgets/menu.h"

namespace he::editor
{
    ProjectView::ProjectView(
        AppArgsService& appArgsService,
        DialogService& dialogService,
        EditorData& editorData,
        ImGuiService& imguiService,
        RenderService& renderService,
        SettingsService& settingsService,
        TaskService& taskService,
        UniquePtr<AppFrame> appFrame,
        UniquePtr<OpenProjectFileCommand> openProjectFileCommand) noexcept
        : m_appArgsService(appArgsService)
        , m_dialogService(dialogService)
        , m_editorData(editorData)
        , m_imguiService(imguiService)
        , m_renderService(renderService)
        , m_settingsService(settingsService)
        , m_taskService(taskService)
        , m_appFrame(Move(appFrame))
        , m_openProjectFileCommand(Move(openProjectFileCommand))
    {}

    bool ProjectView::Initialize()
    {
        if (!CreateView())
        {
            m_editorData.device->Quit(1);
            return false;
        }
        return true;
    }

    void ProjectView::Terminate()
    {
        DestroyView();
    }

    void ProjectView::OnEvent(const window::Event& ev)
    {
        if (!m_view)
            return;

        m_imguiService.OnEvent(ev);

        switch (ev.kind)
        {
            case window::EventKind::ViewRequestClose:
            {
                const auto& evt = static_cast<const window::ViewRequestCloseEvent&>(ev);
                OnViewRequestClose(evt.view);
                break;
            }
            case window::EventKind::ViewResized:
            {
                const auto& evt = static_cast<const window::ViewResizedEvent&>(ev);
                OnViewResized(evt.view, evt.size);
                break;
            }
            default:
                break;
        }
    }

    void ProjectView::Tick()
    {
        if (!m_view)
            return;

        // Update the application UI
        m_imguiService.NewFrame();
        Show();
        m_imguiService.Update();

        // Perform rendering pass
        m_renderService.BeginFrame();
        m_imguiService.Render();
        m_renderService.EndFrame();

        // Run project open at the end of the frame, if the user selected a project to open.
        // We do this here because opening the project will cause the editor app to terminate
        // this view, which terminates our services.
        if (m_openProjectFileCommand->CanRun())
        {
            m_openProjectFileCommand->Run();
        }
    }

    window::ViewHitArea ProjectView::HitTest(const Vec2i& point)
    {
        return m_view ? m_appFrame->GetHitArea(point) : window::ViewHitArea::Normal;
    }

    window::ViewDropEffect ProjectView::GetDropEffect()
    {
        return m_view ? m_imguiService.GetDropEffect(m_view) :  window::ViewDropEffect::Reject;
    }

    void ProjectView::Show()
    {
        if (!m_view)
            return;

        ShowMainMenu();

        // TODO: Big font
        ImGui::TextUnformatted("Projects");
        ImGui::SameLine();
        ImGui::Button("Add Existing");
        ImGui::SameLine();
        ImGui::Button("Create New"); // TODO: Primary button color

        m_openProjectFileCommand->SetPath("");

        const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
        if (ImGui::BeginTable("##project_list", 2, flags))
        {
            ImGui::TableSetupColumn("Project Name");
            ImGui::TableSetupColumn("Path");
            ImGui::TableHeadersRow();

            const ImGuiSelectableFlags selectFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_AllowDoubleClick;

            const schema::List<RecentProject>::Reader recentProjects = m_settingsService.GetSettings().GetRecentProjects();
            for (const RecentProject::Reader project : recentProjects)
            {
                ImGui::PushID(project.GetPath().Data());

                // Project Name & selectable row
                ImGui::TableNextColumn();
                bool selected = false;
                if (ImGui::Selectable("##project_select", &selected, selectFlags))
                {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        m_openProjectFileCommand->SetPath(project.GetPath());
                    }
                }
                ImGui::SameLine();
                ImGui::TextUnformatted(ICON_MDI_NOTEBOOK_EDIT " ");
                ImGui::SameLine();
                ImGui::TextUnformatted(project.GetName().Data());

                // Project Path
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(project.GetPath().Data());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void ProjectView::ShowMainMenu()
    {
        if (!m_appFrame->BeginAppMainMenuBar())
            return;

        if (BeginTopLevelMenu("Help"))
        {
            MenuSeparator("Documentation");

            // TODO
            MenuItem("API Reference  " ICON_MDI_OPEN_IN_NEW, ICON_MDI_BOOK_OPEN);
            MenuItem("Tutorials  " ICON_MDI_OPEN_IN_NEW, ICON_MDI_BOOK_OPEN_VARIANT);
            MenuItem("Report Bug  " ICON_MDI_OPEN_IN_NEW, ICON_MDI_BUG);

            MenuSeparator("Application");

            if (MenuItem("Licenses"))
            {
                // TODO
            }

            if (MenuItem("About", ICON_MDI_HELP_CIRCLE))
            {
                m_dialogService.Open<AboutDialog>();
            }

            EndTopLevelMenu();
        }

        m_appFrame->EndAppMainMenuBar();
    }

    bool ProjectView::CreateView()
    {
        if (m_view)
            return true;

        window::ViewDesc desc{};
        desc.title = "Harvest Editor";
        desc.flags = window::ViewFlag::Default | window::ViewFlag::Borderless | window::ViewFlag::AcceptFiles;

        window::View* view = m_editorData.device->CreateView(desc);
        view->SetVisible(true, true);

        if (!m_renderService.Initialize(view))
            return false;

        if (!m_imguiService.Initialize(view))
            return false;

        m_appFrame->SetView(view);
        m_view = view;
        return true;
    }

    void ProjectView::DestroyView()
    {
        if (!m_view)
            return;

        m_appFrame->SetView(nullptr);

        m_imguiService.Terminate();
        m_renderService.Terminate();

        m_editorData.device->DestroyView(m_view);
        m_view = nullptr;
    }

    void ProjectView::OnViewResized(window::View* view, const Vec2i& size)
    {
        if (m_view != view)
            return;

        m_renderService.SetSize(size);
    }

    void ProjectView::OnViewRequestClose(window::View* view)
    {
        if (m_view != view)
            return;

        Terminate();
        m_editorData.device->Quit(0);
    }
}
