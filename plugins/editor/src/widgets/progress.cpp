// Copyright Chad Engler

#include "he/editor/widgets/progress.h"

#include "he/core/assert.h"
#include "he/core/string_ops.h"
#include "he/core/utils.h"
#include "he/core/math.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    static void RenderRectFilledRangeH(ImDrawList* drawList, const ImRect& bb, ImU32 color, float t0, float t1, float rounding)
    {
        const float clampedMin = ImClamp(t0, 0.0f, 1.0f);
        const float clampedMax = ImClamp(t1, 0.0f, 1.0f);
        if (clampedMin >= clampedMax)
            return;

        const ImVec2 fillMin{ ImLerp(bb.Min.x, bb.Max.x, clampedMin), bb.Min.y };
        const ImVec2 fillMax{ ImLerp(bb.Min.x, bb.Max.x, clampedMax), bb.Max.y };
        drawList->AddRectFilled(fillMin, fillMax, color, rounding);
    }

    static float SpinnerSawTooth(float count, float t)
    {
        return Fmod(count * t, 1.0f);
    }

    static float SpinnerStrokeTween(float count, float t0, float t1, float t)
    {
        t = SpinnerSawTooth(count, t);

        if (t < t0)
            return 0.0f;

        if (t > t1)
            return 1.0f;

        // ease in-out cubic (aka cubic-bezier(0.65, 0, 0.35, 1))
        // https://easings.net/#easeInOutCubic
        const float v = (t - t0) / (t1 - t0);
        return v < 0.5f ? (4.0f * v * v * v) : (1.0f - (Pow((-2.0f * v) + 2.0f, 3.0f) / 2.0f));
    }

    ImVec2 ProgressSpinnerSize(float thickness, float radius)
    {
        const ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        if (radius == 0.0f)
            radius = g.FontSize / 2.0f;

        if (thickness == 0.0f)
            thickness = radius / 4.0f;

        return ImVec2{ (radius + style.FramePadding.x) * 2.0f, (radius + style.FramePadding.y) * 2.0f };
    }

    void ProgressSpinner(float thickness, float radius)
    {
        // Setup item
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        const ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        if (radius == 0.0f)
            radius = g.FontSize / 2.0f;

        if (thickness == 0.0f)
            thickness = radius / 4.0f;

        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size{ (radius + style.FramePadding.x) * 2.0f, (radius + style.FramePadding.y) * 2.0f };

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, 0))
            return;

        // Render
        constexpr uint32_t SegmentCount = 24;
        constexpr float RotateCount = 5.0f; // number of rotations before a repeat
        constexpr float SkipCount = 3.0f; // number of steps to skip each rotation
        constexpr float Period = 5.0f; // number of seconds to complete a rotation
        constexpr float StartAngle = -IM_PI / 2.0f; // start at the top of the circle

        constexpr float MinArc = (30.0f / 360.0f) * MathConstants<float>::Pi2;
        constexpr float MaxArc = (270.0f / 360.0f) * MathConstants<float>::Pi2;
        constexpr float StepOffset = SkipCount * MathConstants<float>::Pi2 / RotateCount;

        const ImVec2 center = ImVec2(pos.x + (size.x / 2.0f), pos.y + (size.y / 2.0f));
        const float t = Fmod(static_cast<float>(g.Time), Period) / Period;

        const float head = SpinnerStrokeTween(RotateCount, 0.0f, 0.5f, t);
        const float tail = SpinnerStrokeTween(RotateCount, 0.5f, 1.0f, t);
        const float step = Floor(Lerp(0.0f, RotateCount, t));
        const float rotation = SpinnerSawTooth(RotateCount, t);
        const float rotationCompensation = Fmod((4.0f * IM_PI) - StepOffset - MaxArc, MathConstants<float>::Pi2);

        const float aMin = StartAngle + (tail * MaxArc) + (rotation * rotationCompensation) - (step * StepOffset);
        const float aMax = aMin + ((head - tail) * MaxArc) + MinArc;

        window->DrawList->PathClear();

        for (uint32_t i = 0; i < SegmentCount; ++i)
        {
            const float a = aMin + (static_cast<float>(i) / SegmentCount) * (aMax - aMin);
            const ImVec2 p{ center.x + (Cos(a) * radius), center.y + (Sin(a) * radius) };
            window->DrawList->PathLineTo(p);
        }

        const ImU32 color = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
        window->DrawList->PathStroke(color, false, thickness);
    }

    void ProgressBar(float value, const ImVec2& size_, const char* overlay)
    {
        // Setup item
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        const ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size = ImGui::CalcItemSize(size_, ImGui::CalcItemWidth(), g.FontSize + (style.FramePadding.y * 2.0f));

        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, 0))
            return;

        const bool indeterminate = value < 0.0f;

        // Render the outside frame
        ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
        bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));

        // Indeterminate bar
        if (indeterminate)
        {
            const float speed = g.FontSize * 0.05f;
            const float phase = Fmod(static_cast<float>(g.Time) * speed, 1.0f);
            const float widthNorm = 0.2f;
            const float t0 = (phase * (1.0f + widthNorm)) - widthNorm;
            const float t1 = t0 + widthNorm;

            RenderRectFilledRangeH(window->DrawList, bb, ImGui::GetColorU32(ImGuiCol_PlotHistogram), t0, t1, style.FrameRounding);
        }
        // normal % value bar
        else
        {
            value = ImSaturate(value);
            RenderRectFilledRangeH(window->DrawList, bb, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 0.0f, value, style.FrameRounding);
        }

        // Default displaying the fraction as percentage string for normal value bars
        char overlay_buf[32];
        if (!indeterminate && !overlay)
        {
            ImFormatString(overlay_buf, IM_ARRAYSIZE(overlay_buf), "%.0f%%", value * 100 + 0.01f);
            overlay = overlay_buf;
        }

        // Render the string if it is not empty
        if (!StrEmpty(overlay))
        {

            const ImVec2 overlaySize = ImGui::CalcTextSize(overlay, NULL);
            if (overlaySize.x > 0.0f)
            {
                const float textX = indeterminate ? (bb.Min.x + bb.Max.x - overlaySize.x) * 0.5f : ImLerp(bb.Min.x, bb.Max.x, value);
                const float clampedX = ImClamp(textX + style.ItemSpacing.x, bb.Min.x, bb.Max.x - overlaySize.x - style.ItemInnerSpacing.x);
                ImGui::RenderTextClipped(ImVec2(clampedX, bb.Min.y), bb.Max, overlay, NULL, &overlaySize, ImVec2(0.0f, 0.5f), &bb);
            }
        }
    }

    void BufferingBar(float value, const ImVec2& size_)
    {
        // Setup item
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        const ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size = ImGui::CalcItemSize(size_, ImGui::CalcItemWidth(), g.FontSize + (style.FramePadding.y * 2.0f));

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, 0))
            return;

        // Render
        const float circleStart = size.x * 0.7f;
        const float circleEnd = size.x;
        const float circleWidth = circleEnd - circleStart;

        const ImU32 bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
        const ImU32 fg = ImGui::GetColorU32(ImGuiCol_PlotHistogram);

        window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg);
        window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + (circleStart * value), bb.Max.y), fg);

        const float t = static_cast<float>(g.Time);
        const float r = size.y / 2.0f;
        const float speed = 1.5f;

        const float a = speed * 0.0f;
        const float b = speed * 0.333f;
        const float c = speed * 0.666f;

        const float o1 = (circleWidth + r) * ((t + a) - (speed * static_cast<int>((t + a) / speed))) / speed;
        const float o2 = (circleWidth + r) * ((t + b) - (speed * static_cast<int>((t + b) / speed))) / speed;
        const float o3 = (circleWidth + r) * ((t + c) - (speed * static_cast<int>((t + c) / speed))) / speed;

        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg);
        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg);
        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg);
    }
}
