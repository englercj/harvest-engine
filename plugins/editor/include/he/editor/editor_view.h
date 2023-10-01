// Copyright Chad Engler

#include "he/core/types.h"
#include "he/editor/services/app_args_service.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/services/imgui_service.h"
#include "he/editor/services/render_service.h"
#include "he/editor/services/workspace_service.h"

namespace he::window { struct Event; class View; }

namespace he::editor
{
    class EditorView
    {
    public:
        EditorView(
            AppArgsService& appArgsService,
            AssetService& assetService,
            EditorData& editorData,
            ImGuiService& imguiService,
            RenderService& renderService,
            WorkspaceService& workspaceService) noexcept;

        bool Initialize();
        void Terminate();

        bool TryTerminate();

        void OnEvent(const window::Event& ev);
        void Tick();

        window::ViewHitArea HitTest(const Vec2i& point);
        window::ViewDropEffect GetDropEffect();

        window::View* GetView() const { return m_view; }

    private:
        bool CreateView();

        void OnViewResized(window::View* view, const Vec2i& size);
        void OnViewRequestClose(window::View* view);

    private:
        AppArgsService& m_appArgsService;
        AssetService& m_assetService;
        EditorData& m_editorData;
        ImGuiService& m_imguiService;
        RenderService& m_renderService;
        WorkspaceService& m_workspaceService;

        bool m_initialized{ false };
        window::View* m_view{ nullptr };
    };
}
