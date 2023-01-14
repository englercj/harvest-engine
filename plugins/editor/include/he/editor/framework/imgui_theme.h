// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

#include "imgui.h"
#include "implot.h"

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

    ImFont* GetFont(Font f);
    void PushFont(Font f);
    void PopFont();

    void SetThemeFonts(ImFontAtlas& atlas, float dpiScale);
    void SetThemeColors(ImGuiStyle& style, ImPlotStyle& plotStyle);
    void SetThemeStyle(ImGuiStyle& style, ImPlotStyle& plotStyle, float dpiScale);
}
