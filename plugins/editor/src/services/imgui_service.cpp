// Copyright Chad Engler

#include "imgui_service.h"

#include "fonts/icons_material_design.h"
#include "fonts/materialdesignicons.ttf.h"
#include "fonts/NotoSans-Regular.ttf.h"
#include "fonts/NotoMono-Regular.ttf.h"

#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/core/string.h"
#include "he/core/string_view_fmt.h"
#include "he/math/float.h"
#include "he/math/types_fmt.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"
#include "misc/freetype/imgui_freetype.h"

namespace he::editor
{
    const char* ImGuiService::DndPayloadId = "HE_DND_FILE";

    ImGuiService::ImGuiService(
        EditorData& editorData,
        ImGuiPlatformService& imguiPlatformService,
        ImGuiRenderService& imguiRenderService,
        RenderService& renderService) noexcept
        : m_editorData(editorData)
        , m_imguiPlatformService(imguiPlatformService)
        , m_imguiRenderService(imguiRenderService)
        , m_renderService(renderService)
    {
        ImGui::SetAllocatorFunctions(
            [](size_t size, void* alloc) -> void* { return static_cast<Allocator*>(alloc)->Malloc(size); },
            [](void* ptr, void* alloc) { static_cast<Allocator*>(alloc)->Free(ptr); },
            &Allocator::GetDefault());
    }

    bool ImGuiService::Initialize(window::View* view)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

        SetupColors();

        auto setupStyle = ImGuiPlatformService::StyleSetupDelegate::Make<&ImGuiService::SetupStyle>(this);
        auto setupFonts = ImGuiPlatformService::FontsSetupDelegate::Make<&ImGuiService::SetupFonts>(this);

        if (!m_imguiPlatformService.Initialize(m_editorData.device, view, setupStyle, setupFonts))
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
        ImPlot::DestroyContext();
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
        ImGui::UpdatePlatformWindows();
        m_imguiPlatformService.UpdateViews();

        if (m_dndActive)
        {
            ImGuiDragDropFlags flags = ImGuiDragDropFlags_SourceExtern | ImGuiDragDropFlags_SourceNoPreviewTooltip;
            if (ImGui::BeginDragDropSource(flags))
            {
                ImGui::SetDragDropPayload(DndPayloadId, nullptr, 0, ImGuiCond_Always);
                ImGui::EndDragDropSource();
            }
        }
    }

    void ImGuiService::Render()
    {
        m_imguiRenderService.Render();
        ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
    }

    void ImGuiService::OnEvent(const window::Event& ev)
    {
        m_imguiPlatformService.OnEvent(ev);

        ImGuiIO& io = ImGui::GetIO();

        switch (ev.kind)
        {
            case window::EventKind::ViewDndStart:
            {
                m_dndActive = true;
                m_dndPaths.Clear();
                io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                break;
            }
            case window::EventKind::ViewDndMove:
            {
                const auto& evt = static_cast<const window::ViewDndMoveEvent&>(ev);
                io.AddMousePosEvent(evt.pos.x, evt.pos.y);
                break;
            }
            case window::EventKind::ViewDndDrop:
            {
                const auto& evt = static_cast<const window::ViewDndDropEvent&>(ev);
                m_dndPaths.EmplaceBack(evt.path);
                break;
            }
            case window::EventKind::ViewDndEnd:
            {
                io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                m_dndActive = false;
                break;
            }
            default:
                break;
        }
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
        colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.24f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.24f, 0.59f, 0.98f, 1.00f);
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

        ImVec4* plotColors = ImPlot::GetStyle().Colors;

        plotColors[ImPlotCol_Line]              = IMPLOT_AUTO_COL;
        plotColors[ImPlotCol_Fill]              = IMPLOT_AUTO_COL;
        plotColors[ImPlotCol_MarkerOutline]     = IMPLOT_AUTO_COL;
        plotColors[ImPlotCol_MarkerFill]        = IMPLOT_AUTO_COL;
        plotColors[ImPlotCol_ErrorBar]          = IMPLOT_AUTO_COL;
        plotColors[ImPlotCol_FrameBg]           = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        plotColors[ImPlotCol_PlotBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
        plotColors[ImPlotCol_PlotBorder]        = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        plotColors[ImPlotCol_LegendBg]          = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        plotColors[ImPlotCol_LegendBorder]      = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        plotColors[ImPlotCol_LegendText]        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        plotColors[ImPlotCol_TitleText]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        plotColors[ImPlotCol_InlayText]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        plotColors[ImPlotCol_AxisText]          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        plotColors[ImPlotCol_AxisGrid]          = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
        plotColors[ImPlotCol_AxisTick]          = IMPLOT_AUTO_COL;
        plotColors[ImPlotCol_AxisBg]            = IMPLOT_AUTO_COL;
        plotColors[ImPlotCol_AxisBgHovered]     = IMPLOT_AUTO_COL;
        plotColors[ImPlotCol_AxisBgActive]      = IMPLOT_AUTO_COL;
        plotColors[ImPlotCol_Selection]         = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        plotColors[ImPlotCol_Crosshairs]        = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    }

    void ImGuiService::SetupStyle(ImGuiStyle& style, ImPlotStyle& plotStyle, float dpiScale)
    {
        #define HE_DPI(x) ImFloor((x) * dpiScale)

        style.Alpha                             = 1.0f;                     // Global alpha applies to everything in Dear ImGui.
        style.DisabledAlpha                     = 0.6f;                     // Additional alpha multiplier applied by BeginDisabled(). Multiply over current value of Alpha.
        style.WindowPadding                     = HE_DPI(ImVec2(8, 8));     // Padding within a window
        style.WindowRounding                    = HE_DPI(0.0f);             // Radius of window corners rounding. Set to 0.0f to have rectangular windows. Large values tend to lead to variety of artifacts and are not recommended.
        style.WindowBorderSize                  = 1.0f;                     // Thickness of border around windows. Generally set to 0.0f or 1.0f. Other values not well tested.
        style.WindowMinSize                     = HE_DPI(ImVec2(32, 32));   // Minimum window size
        style.WindowTitleAlign                  = ImVec2(0.0f, 0.5f);       // Alignment for title bar text
        style.WindowMenuButtonPosition          = ImGuiDir_Left;            // Position of the collapsing/docking button in the title bar (left/right). Defaults to ImGuiDir_Left.
        style.ChildRounding                     = HE_DPI(0.0f);             // Radius of child window corners rounding. Set to 0.0f to have rectangular child windows
        style.ChildBorderSize                   = 1.0f;                     // Thickness of border around child windows. Generally set to 0.0f or 1.0f. Other values not well tested.
        style.PopupRounding                     = HE_DPI(0.0f);             // Radius of popup window corners rounding. Set to 0.0f to have rectangular child windows
        style.PopupBorderSize                   = 1.0f;                     // Thickness of border around popup or tooltip windows. Generally set to 0.0f or 1.0f. Other values not well tested.
        style.FramePadding                      = HE_DPI(ImVec2(14, 8));    // Padding within a framed rectangle (used by most widgets)
        style.FrameRounding                     = HE_DPI(0.0f);             // Radius of frame corners rounding. Set to 0.0f to have rectangular frames (used by most widgets).
        style.FrameBorderSize                   = 1.0f;                     // Thickness of border around frames. Generally set to 0.0f or 1.0f. Other values not well tested.
        style.ItemSpacing                       = HE_DPI(ImVec2(4, 2));     // Horizontal and vertical spacing between widgets/lines
        style.ItemInnerSpacing                  = HE_DPI(ImVec2(4, 4));     // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
        style.CellPadding                       = HE_DPI(ImVec2(4, 2));     // Padding within a table cell
        style.TouchExtraPadding                 = HE_DPI(ImVec2(0, 0));     // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
        style.IndentSpacing                     = HE_DPI(21.0f);            // Horizontal spacing when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
        style.ColumnsMinSpacing                 = HE_DPI(6.0f);             // Minimum horizontal spacing between two columns. Preferably > (FramePadding.x + 1).
        style.ScrollbarSize                     = HE_DPI(18.0f);            // Width of the vertical scrollbar, Height of the horizontal scrollbar
        style.ScrollbarRounding                 = HE_DPI(0.0f);             // Radius of grab corners rounding for scrollbar
        style.GrabMinSize                       = HE_DPI(10.0f);            // Minimum width/height of a grab box for slider/scrollbar
        style.GrabRounding                      = HE_DPI(0.0f);             // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
        style.LogSliderDeadzone                 = HE_DPI(4.0f);             // The size in pixels of the dead-zone around zero on logarithmic sliders that cross zero.
        style.TabRounding                       = HE_DPI(0.0f);             // Radius of upper corners of a tab. Set to 0.0f to have rectangular tabs.
        style.TabBorderSize                     = 0.0f;                     // Thickness of border around tabs.
        style.TabMinWidthForCloseButton         = HE_DPI(0.0f);             // Minimum width for close button to appears on an unselected tab when hovered. Set to 0.0f to always show when hovering, set to FLT_MAX to never show close button unless selected.
        style.ColorButtonPosition               = ImGuiDir_Right;           // Side of the color button in the ColorEdit4 widget (left/right). Defaults to ImGuiDir_Right.
        style.ButtonTextAlign                   = ImVec2(0.5f, 0.5f);       // Alignment of button text when button is larger than text.
        style.SelectableTextAlign               = ImVec2(0.0f, 0.0f);       // Alignment of selectable text. Defaults to (0.0f, 0.0f) (top-left aligned). It's generally important to keep this left-aligned if you want to lay multiple items on a same line.
        style.DisplayWindowPadding              = HE_DPI(ImVec2(19, 19));   // Window position are clamped to be visible within the display area or monitors by at least this amount. Only applies to regular windows.
        style.DisplaySafeAreaPadding            = HE_DPI(ImVec2(3, 3));     // If you cannot see the edge of your screen (e.g. on a TV) increase the safe area padding. Covers popups/tooltips as well regular windows.
        style.MouseCursorScale                  = HE_DPI(1.0f);             // Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). May be removed later.
        style.AntiAliasedLines                  = true;                     // Enable anti-aliased lines/borders. Disable if you are really tight on CPU/GPU.
        style.AntiAliasedLinesUseTex            = true;                     // Enable anti-aliased lines/borders using textures where possible. Require backend to render with bilinear filtering (NOT point/nearest filtering).
        style.AntiAliasedFill                   = true;                     // Enable anti-aliased filled shapes (rounded rectangles, circles, etc.).
        style.CurveTessellationTol              = 1.25f;                    // Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
        style.CircleTessellationMaxError        = 0.30f;                    // Maximum error (in pixels) allowed when using AddCircle()/AddCircleFilled() or drawing rounded corner rectangles with no explicit segment count specified. Decrease for higher quality but more geometry.

        plotStyle.LineWeight                    = HE_DPI(1.0f);             // item line weight in pixels
        plotStyle.Marker                        = ImPlotMarker_None;        // marker specification
        plotStyle.MarkerSize                    = HE_DPI(4.0f);             // marker size in pixels (roughly the marker's "radius")
        plotStyle.MarkerWeight                  = 1.0f;                     // outline weight of markers in pixels
        plotStyle.FillAlpha                     = 1.0f;                     // alpha modifier applied to plot fills
        plotStyle.ErrorBarSize                  = HE_DPI(5.0f);             // error bar whisker width in pixels
        plotStyle.ErrorBarWeight                = 1.5f;                     // error bar whisker weight in pixels
        plotStyle.DigitalBitHeight              = HE_DPI(8.0f);             // digital channels bit height (at y = 1.0f) in pixels
        plotStyle.DigitalBitGap                 = HE_DPI(4.0f);             // digital channels bit padding gap in pixels

        plotStyle.PlotBorderSize                = 1.0f;                     // line thickness of border around plot area
        plotStyle.MinorAlpha                    = 0.25f;                    // alpha multiplier applied to minor axis grid lines
        plotStyle.MajorTickLen                  = HE_DPI(ImVec2(10, 10));   // major tick lengths for X and Y axes
        plotStyle.MinorTickLen                  = HE_DPI(ImVec2(5, 5));     // minor tick lengths for X and Y axes
        plotStyle.MajorTickSize                 = ImVec2(1, 1);             // line thickness of major ticks
        plotStyle.MinorTickSize                 = ImVec2(1, 1);             // line thickness of minor ticks
        plotStyle.MajorGridSize                 = ImVec2(1, 1);             // line thickness of major grid lines
        plotStyle.MinorGridSize                 = ImVec2(1, 1);             // line thickness of minor grid lines
        plotStyle.PlotPadding                   = HE_DPI(ImVec2(10, 10));   // padding between widget frame and plot area, labels, or outside legends (i.e. main padding)
        plotStyle.LabelPadding                  = HE_DPI(ImVec2(5, 5));     // padding between axes labels, tick labels, and plot edge
        plotStyle.LegendPadding                 = HE_DPI(ImVec2(10, 10));   // legend padding from plot edges
        plotStyle.LegendInnerPadding            = HE_DPI(ImVec2(5, 5));     // legend inner padding from legend edges
        plotStyle.LegendSpacing                 = HE_DPI(ImVec2(5, 0));     // spacing between legend entries
        plotStyle.MousePosPadding               = HE_DPI(ImVec2(10, 10));   // padding between plot edge and interior mouse location text
        plotStyle.AnnotationPadding             = HE_DPI(ImVec2(2, 2));     // text padding around annotation labels
        plotStyle.FitPadding                    = HE_DPI(ImVec2(0, 0));     // additional fit padding as a percentage of the fit extents (e.g. ImVec2(0.1f,0.1f) adds 10% to the fit extents of X and Y)
        plotStyle.PlotDefaultSize               = HE_DPI(ImVec2(400, 300)); // default size used when ImVec2(0,0) is passed to BeginPlot
        plotStyle.PlotMinSize                   = HE_DPI(ImVec2(200, 150)); // minimum size plot frame can be when shrunk

        plotStyle.Colormap                      = ImPlotColormap_Deep;      // The current colormap. Set this to either an ImPlotColormap_ enum or an index returned by AddColormap.

        plotStyle.UseLocalTime                  = false;                    // axis labels will be formatted for your timezone when ImPlotAxisFlag_Time is enabled
        plotStyle.Use24HourClock                = false;                    // dates will be formatted according to ISO 8601 where applicable (e.g. YYYY-MM-DD, YYYY-MM, --MM-DD, etc.)
        plotStyle.UseISO8601                    = false;                    // times will be formatted using a 24 hour clock

        #undef HE_DPI
    }

    void ImGuiService::SetupFonts(ImFontAtlas& atlas, float dpiScale)
    {
        // NOTE: The order MUST match the Font enum in imgui_service.h

        // Add NotoSans-Regular font
        {
            const float fontSize = Floor(14.0f * dpiScale);

            ImFontConfig config{};
            String::Copy(config.Name, "NotoSans-Regular.ttf");
            atlas.AddFontFromMemoryCompressedTTF(c_NotoSans_Regular_ttf, HE_LENGTH_OF(c_NotoSans_Regular_ttf), fontSize, &config);

            MergeMaterialDesignIcons(atlas, fontSize);
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
        }

        // Inform the render service that there's a new font
        const bool result = m_imguiRenderService.SetupFontAtlas(atlas);
        HE_ASSERT(result);
        HE_UNUSED(result);
    }

    void ImGuiService::MergeMaterialDesignIcons(ImFontAtlas& atlas, float fontSize)
    {
        // https://materialdesignicons.com/
        static const ImWchar IconRanges[] { ICON_MIN_MDI, ICON_MAX_MDI, 0 };

        ImFontConfig config{};
        String::Copy(config.Name, FONT_ICON_FILE_NAME_MDI);
        config.MergeMode = true;
        config.GlyphMinAdvanceX = Floor(fontSize / 2.0f);
        config.OversampleH = 1;
        config.OversampleV = 1;
        config.PixelSnapH = true;
        config.GlyphOffset = { 0, 2 };

        atlas.AddFontFromMemoryCompressedTTF(c_materialdesignicons_ttf, HE_LENGTH_OF(c_materialdesignicons_ttf), fontSize, &config, IconRanges);
    }
}
