// Copyright Chad Engler

#include "he/editor/editor_view.h"

#include "he/window/event.h"
#include "he/window/view.h"

namespace he::editor
{
    EditorView::EditorView(
        AppArgsService& appArgsService,
        AssetService& assetService,
        EditorData& editorData,
        ImGuiService& imguiService,
        RenderService& renderService,
        WorkspaceService& workspaceService) noexcept
        : m_appArgsService(appArgsService)
        , m_assetService(assetService)
        , m_editorData(editorData)
        , m_imguiService(imguiService)
        , m_renderService(renderService)
        , m_workspaceService(workspaceService)
    {}

    bool EditorView::Initialize()
    {
        if (!CreateView())
        {
            m_editorData.device->Quit(1);
            return false;
        }
        return true;
    }

    void EditorView::Terminate()
    {
        DestroyView();
    }

    bool EditorView::TryTerminate()
    {
        if (m_workspaceService.RequestClose())
        {
            Terminate();
            return true;
        }

        return false;
    }

    void EditorView::OnEvent(const window::Event& ev)
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

    void EditorView::Tick()
    {
        if (!m_view)
            return;

        // Update the application UI
        m_imguiService.NewFrame();
        m_workspaceService.Show();
        m_imguiService.Update();

        // Perform rendering pass
        m_renderService.BeginFrame();
        m_imguiService.Render();
        m_renderService.EndFrame();
    }

    window::ViewHitArea EditorView::HitTest(const Vec2i& point)
    {
        return m_view ? m_workspaceService.GetHitArea(point) : window::ViewHitArea::Normal;
    }

    window::ViewDropEffect EditorView::GetDropEffect()
    {
        return m_view ? m_imguiService.GetDropEffect(m_view) :  window::ViewDropEffect::Reject;
    }

    bool EditorView::CreateView()
    {
        if (m_view)
            return true;

        window::ViewDesc desc{};
        desc.title = "Harvest Editor";
        desc.flags = window::ViewFlag::Default | window::ViewFlag::Borderless | window::ViewFlag::AcceptFiles;

        window::View* view = m_editorData.device->CreateView(desc);
        view->SetVisible(true, true);

        if (!m_assetService.Initialize())
            return false;

        if (!m_renderService.Initialize(view))
            return false;

        if (!m_imguiService.Initialize(view))
            return false;

        if (!m_workspaceService.Initialize(view))
            return false;

        m_view = view;
        return true;
    }

    void EditorView::DestroyView()
    {
        if (!m_view)
            return;

        m_workspaceService.Terminate();
        m_imguiService.Terminate();
        m_renderService.Terminate();
        m_assetService.Terminate();

        m_editorData.device->DestroyView(m_view);
        m_view = nullptr;
    }

    void EditorView::OnViewResized(window::View* view, const Vec2i& size)
    {
        if (m_view != view)
            return;

        m_renderService.SetSize(size);
    }

    void EditorView::OnViewRequestClose(window::View* view)
    {
        if (m_view != view)
            return;

        if (TryTerminate())
        {
            m_editorData.device->Quit(0);
        }
    }
}
