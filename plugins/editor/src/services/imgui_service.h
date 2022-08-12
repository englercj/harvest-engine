// Copyright Chad Engler

#pragma once

#include "editor_data.h"
#include "imgui_platform_service.h"
#include "imgui_render_service.h"
#include "render_service.h"

#include "he/core/allocator.h"
#include "he/rhi/types.h"
#include "he/window/event.h"
#include "he/window/view.h"

#include "imgui.h"

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

    inline constexpr ImVec4 Color_Error{ 1.00f, 0.60f, 0.00f, 1.00f };

    class ImGuiService
    {
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

    private:
        static void SetupColors();

        void SetupStyle(ImGuiStyle& style);
        void SetupFonts(ImFontAtlas& atlas, float dpiScale);

        static void MergeMaterialDesignIcons(ImFontAtlas& atlas, float fontSize);

    private:
        EditorData& m_editorData;
        ImGuiPlatformService& m_imguiPlatformService;
        ImGuiRenderService& m_imguiRenderService;
        RenderService& m_renderService;
    };
}
