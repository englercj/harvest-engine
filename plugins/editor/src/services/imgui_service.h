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

    class ImGuiService
    {
    public:
        ImGuiService(
            EditorData& editorData,
            ImGuiPlatformService& imguiPlatformService,
            ImGuiRenderService& imguiRenderService,
            RenderService& renderService);

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
        static void SetupStyle(void* ctx, ImGuiStyle& style);
        static void SetupFontAtlas(void* ctx, ImFontAtlas& atlas, float dpiScale);

        static void MergeFontAwesome(ImFontAtlas& atlas, float scaledFontSize);

    private:
        EditorData& m_editorData;
        ImGuiPlatformService& m_imguiPlatformService;
        ImGuiRenderService& m_imguiRenderService;
        RenderService& m_renderService;
    };
}
