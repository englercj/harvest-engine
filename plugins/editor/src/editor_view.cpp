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
        SettingsService& settingsService,
        WorkspaceService& workspaceService) noexcept
        : m_appArgsService(appArgsService)
        , m_assetService(assetService)
        , m_editorData(editorData)
        , m_imguiService(imguiService)
        , m_renderService(renderService)
        , m_settingsService(settingsService)
        , m_workspaceService(workspaceService)
    {}

    bool EditorView::Initialize()
    {
        if (!CreateView())
            m_editorData.device->Quit(1);
    }

    void EditorView::Terminate()
    {
        m_initialized = false;

        m_settingsService.Save();

        m_imguiService.Terminate();
        m_renderService.Terminate();
        m_assetService.Terminate();

        m_editorData.device->Quit(0);
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
        if (!m_initialized)
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

    void EditorView::Show()
    {
        if (!m_initialized)
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
        return m_initialized ? m_workspaceService.GetHitArea(point) : window::ViewHitArea::Normal;
    }

    window::ViewDropEffect EditorView::GetDropEffect()
    {
        return m_initialized ? m_imguiService.GetDropEffect(m_view) :  window::ViewDropEffect::Reject;
    }

    bool EditorView::CreateView()
    {
        if (m_initialized)
            return true;

        window::ViewDesc desc{};
        desc.title = "Harvest Editor";
        desc.flags = window::ViewFlag::Default | window::ViewFlag::Borderless | window::ViewFlag::AcceptFiles;

        m_view = m_editorData.device->CreateView(desc);
        m_initialized = true;

        // Initialize required services
        if (!m_assetService.Initialize())
            return false;

        if (!m_renderService.Initialize(m_view))
            return false;

        if (!m_imguiService.Initialize(m_view))
            return false;

        // Failing to load settings is OK, we'll run with defaults and have an error in the log
        m_settingsService.Reload();
        return true;
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

        TryTerminate();
    }
}
