// Copyright Chad Engler

#include "imgui_service.h"

#include "fonts/IconsFontAwesome5Pro.h"
#include "fonts/fa5-pro-solid-900.ttf.h"
#include "fonts/NotoSans-Regular.ttf.h"
#include "fonts/NotoMono-Regular.ttf.h"

#include "he/core/macros.h"
#include "he/core/string.h"
#include "he/math/float.h"

#include "imgui.h"
#include "misc/freetype/imgui_freetype.h"

namespace he::editor
{
    ImGuiService::ImGuiService(
        Allocator& allocator,
        EditorData& editorData,
        ImGuiPlatformService& imguiPlatformService,
        ImGuiRenderService& imguiRenderService,
        RenderService& renderService)
        : m_allocator(allocator)
        , m_editorData(editorData)
        , m_imguiPlatformService(imguiPlatformService)
        , m_imguiRenderService(imguiRenderService)
        , m_renderService(renderService)
    {
        ImGui::SetAllocatorFunctions(
            [](size_t size, void* alloc) -> void* { return static_cast<Allocator*>(alloc)->Malloc(size); },
            [](void* ptr, void* alloc) { static_cast<Allocator*>(alloc)->Free(ptr); },
            &m_allocator);
    }

    bool ImGuiService::Initialize(window::View* view)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

        SetupColors();

        if (!m_imguiPlatformService.Initialize(m_editorData.device, view, &ImGuiService::SetupStyle, &ImGuiService::SetupFontAtlas, this))
            return false;

        if (!m_imguiRenderService.Initialize(m_renderService.GetSwapChainFormat()))
            return false;

        // TODO: Fonts

        return true;
    }

    void ImGuiService::Terminate()
    {
        m_imguiRenderService.Terminate();
        m_imguiPlatformService.Terminate();
        ImGui::DestroyContext();
    }

    void ImGuiService::NewFrame()
    {
        m_imguiRenderService.NewFrame();
        m_imguiPlatformService.NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiService::Update()
    {
        ImGui::Render();
    }

    void ImGuiService::Render()
    {
        m_imguiRenderService.Render();
    }

    void ImGuiService::OnEvent(const window::Event& ev)
    {
        m_imguiPlatformService.OnEvent(ev);
    }

    ImFont* ImGuiService::GetFont(Font f) const
    {
        HE_ASSERT(f < Font::_Count);
        return ImGui::GetIO().Fonts->Fonts[static_cast<uint32_t>(f)];
    }

    void ImGuiService::PushFont(Font f) const
    {
        ImGui::PushFont(GetFont(f));
    }

    void ImGuiService::PopFont() const
    {
        ImGui::PopFont();
    }

    void ImGuiService::SetupColors()
    {
        ImVec4* colors = ImGui::GetStyle().Colors;

        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.43f, 0.67f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.20f, 0.43f, 0.67f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.31f, 0.55f, 0.78f, 0.77f);
        colors[ImGuiCol_Separator]              = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.31f, 0.55f, 0.78f, 0.77f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.43f, 0.67f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.13f, 0.13f, 0.13f, 0.78f);
    }

    void ImGuiService::SetupStyle(void*, ImGuiStyle& style)
    {
        style.Alpha                             = 1.0f;
        style.WindowPadding                     = ImVec2(8, 8);
        style.WindowRounding                    = 0.0f;
        style.WindowBorderSize                  = 0.0f;
        style.WindowMinSize                     = ImVec2(32, 32);
        style.WindowTitleAlign                  = ImVec2(0.0f, 0.5f);
        style.WindowMenuButtonPosition          = ImGuiDir_Left;
        style.ChildRounding                     = 0.0f;
        style.ChildBorderSize                   = 0.0f;
        style.PopupRounding                     = 0.0f;
        style.PopupBorderSize                   = 1.0f;
        style.FramePadding                      = ImVec2(14, 8);
        style.FrameRounding                     = 0.0f;
        style.FrameBorderSize                   = 0.0f;
        style.ItemSpacing                       = ImVec2(6, 2);
        style.ItemInnerSpacing                  = ImVec2(1, 0);
        style.CellPadding                       = ImVec2(4, 2);
        style.TouchExtraPadding                 = ImVec2(0, 0);
        style.IndentSpacing                     = 21.0f;
        style.ColumnsMinSpacing                 = 6.0f;
        style.ScrollbarSize                     = 20.0f;
        style.ScrollbarRounding                 = 0.0f;
        style.GrabMinSize                       = 12.0f;
        style.GrabRounding                      = 0.0f;
        style.LogSliderDeadzone                 = 4.0f;
        style.TabRounding                       = 0.0f;
        style.TabBorderSize                     = 0.0f;
        style.TabMinWidthForCloseButton         = 0.0f;
        style.ColorButtonPosition               = ImGuiDir_Right;
        style.ButtonTextAlign                   = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign               = ImVec2(0.0f, 0.0f);
        style.DisplayWindowPadding              = ImVec2(19, 19);
        style.DisplaySafeAreaPadding            = ImVec2(3, 3);
        style.MouseCursorScale                  = 1.0f;
        style.AntiAliasedLines                  = true;
        style.AntiAliasedLinesUseTex            = true;
        style.AntiAliasedFill                   = true;
        style.CurveTessellationTol              = 1.25f;
        style.CircleTessellationMaxError        = 0.30f;
    }

    void ImGuiService::SetupFontAtlas(void* ctx, ImFontAtlas& atlas, float dpiScale)
    {
        // NOTE: The order MUST match the Font enum in imgui_service.h

        // Add NotoSans-Regular font
        {
            const float fontSize = Floor(14.0f * dpiScale);

            ImFontConfig config{};
            String::Copy(config.Name, "NotoSans-Regular.ttf");
            atlas.AddFontFromMemoryCompressedTTF(c_NotoSans_Regular_ttf, HE_LENGTH_OF(c_NotoSans_Regular_ttf), fontSize, &config);

            MergeFontAwesome(atlas, 11.0f * dpiScale);
        }

        // Add NotoSans-Regular font for headers
        {
            const float fontSize = Floor(28.0f * dpiScale);

            ImFontConfig config{};
            String::Copy(config.Name, "NotoSans-Regular.ttf");
            atlas.AddFontFromMemoryCompressedTTF(c_NotoSans_Regular_ttf, HE_LENGTH_OF(c_NotoSans_Regular_ttf), fontSize, &config);
        }

        // Add NotoSans-Regular font for titles
        {
            const float fontSize = Floor(56.0f * dpiScale);

            ImFontConfig config{};
            String::Copy(config.Name, "NotoSans-Regular.ttf");
            atlas.AddFontFromMemoryCompressedTTF(c_NotoSans_Regular_ttf, HE_LENGTH_OF(c_NotoSans_Regular_ttf), fontSize, &config);
        }

        // Add NotoMono-Regular font
        {
            const float fontSize = Floor(14.0f * dpiScale);

            ImFontConfig config{};
            String::Copy(config.Name, "NotoMono-Regular.ttf");
            atlas.AddFontFromMemoryCompressedTTF(c_NotoMono_Regular_ttf, HE_LENGTH_OF(c_NotoMono_Regular_ttf), fontSize, &config);

            MergeFontAwesome(atlas, 11.0f * dpiScale);
        }

        // Inform the render service that there's a new font
        ImGuiService* service = static_cast<ImGuiService*>(ctx);
        const bool result = service->m_imguiRenderService.SetupFontAtlas(atlas);
        HE_ASSERT(result);
        HE_UNUSED(result);
    }

    void ImGuiService::MergeFontAwesome(ImFontAtlas& atlas, float scaledFontSize)
    {
        scaledFontSize = Floor(scaledFontSize);

        ImFontConfig config{};
        String::Copy(config.Name, "fa5-pro-solid-900.ttf");
        config.MergeMode = true;
        config.GlyphMinAdvanceX = Floor(scaledFontSize / 2.0f);
        static const ImWchar iconRanges[] { ICON_MIN_FA, ICON_MAX_FA, 0 };

        atlas.AddFontFromMemoryCompressedTTF(c_fa5_pro_solid_900_ttf, HE_LENGTH_OF(c_fa5_pro_solid_900_ttf), scaledFontSize, &config, iconRanges);
    }
}
