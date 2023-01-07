// Copyright Chad Engler

#pragma once

#include "editor_data.h"
#include "imgui_platform_service.h"
#include "imgui_render_service.h"
#include "render_service.h"

#include "he/core/allocator.h"
#include "he/core/string.h"
#include "he/core/vector.h"
#include "he/rhi/types.h"
#include "he/window/event.h"
#include "he/window/view.h"

#include "imgui.h"

struct ImPlotStyle;

namespace he::editor
{
    enum class Font : uint8_t
    {
        Regular,
        RegularHeader,
        RegularTitle,
        Monospace,

        _Count,
    };

    struct Colors
    {
        static constexpr ImVec4 Error{ 1.00f, 0.60f, 0.00f, 1.00f };
    };

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

        ImFont* GetFont(Font f) const;
        void PushFont(Font f) const;
        void PopFont() const;

        Span<const String> DragDropPaths() const { return m_dndPaths; }
        void ClearDragDropPaths() { m_dndPaths.Clear(); }

    private:
        static void SetupColors();

        void SetupStyle(ImGuiStyle& style, ImPlotStyle& plotStyle, float dpiScale);
        void SetupFonts(ImFontAtlas& atlas, float dpiScale);

        static void MergeMaterialDesignIcons(ImFontAtlas& atlas, float fontSize);

    private:
        EditorData& m_editorData;
        ImGuiPlatformService& m_imguiPlatformService;
        ImGuiRenderService& m_imguiRenderService;
        RenderService& m_renderService;

        bool m_dndActive{ false };
        Vector<String> m_dndPaths{};
    };
}
