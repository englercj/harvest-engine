// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/string.h"
#include "he/core/vector.h"
#include "he/editor/editor_data.h"
#include "he/editor/services/imgui_platform_service.h"
#include "he/editor/services/imgui_render_service.h"
#include "he/editor/services/render_service.h"
#include "he/rhi/types.h"
#include "he/window/event.h"
#include "he/window/view.h"

#include "imgui.h"

struct ImPlotStyle;

namespace he::editor
{
    class ImGuiService
    {
    public:
        static const char* DndPayloadId;

    public:
        ImGuiService(
            EditorData& editorData,
            ImGuiPlatformService& imguiPlatformService,
            ImGuiRenderService& imguiRenderService,
            RenderService& renderService) noexcept;

        bool Initialize(window::View* view);
        void Terminate();

        void NewFrame();
        void Update();
        void Render();

        void OnEvent(const window::Event& ev);

        window::ViewDropEffect GetDropEffect(window::View* view) const;
        Span<const String> DragDropPaths() const { return m_dndPaths; }
        void ClearDragDropPaths() { m_dndPaths.Clear(); }

    private:
        void SetupFonts(ImFontAtlas& atlas, float dpiScale);

    private:
        EditorData& m_editorData;
        ImGuiPlatformService& m_imguiPlatformService;
        ImGuiRenderService& m_imguiRenderService;
        RenderService& m_renderService;

        bool m_dndActive{ false };
        Vector<String> m_dndPaths{};
    };
}
