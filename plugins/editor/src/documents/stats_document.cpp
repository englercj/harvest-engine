// Copyright Chad Engler

#include "he/editor/documents/stats_document.h"

#include "he/core/memory_ops.h"
#include "he/core/utils.h"
#include "he/editor/widgets/menu.h"

#include "imgui.h"
#include "implot.h"

namespace he::editor
{
    void FpsTracker::Update(float dt)
    {
        const float ms = dt * 1000.0f;

        if (m_count < TimesCount)
        {
            m_times[m_count++] = ms;
            m_total += ms;
            m_min = he::Min(m_min, ms);
            m_max = he::Max(m_max, ms);
            return;
        }

        const float old = m_times[m_offset];

        m_times[m_offset++] = ms;
        m_offset %= TimesCount;

        m_total -= old;
        m_total += ms;

        if (m_min == old || m_max == old)
        {
            ScanForMinMax();
        }
        else
        {
            m_min = he::Min(m_min, ms);
            m_max = he::Max(m_max, ms);
        }
    }

    void FpsTracker::ScanForMinMax()
    {
        m_min = FLT_MAX;
        m_max = FLT_MIN;

        for (float value : Times())
        {
            m_min = he::Min(m_min, value);
            m_max = he::Max(m_max, value);
        }
    }

    StatsDocument::StatsDocument() noexcept
    {
        m_title = "App Stats";
    }

    void StatsDocument::Show()
    {
        m_fps.Update(ImGui::GetIO().DeltaTime);

        const float avg = m_fps.Avg();
        const float fps = 1000.0f / avg;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", avg, fps);

        if (ImPlot::BeginPlot("CPU Frame Time##cpu_frame_time", ImVec2(-1, 0)))
        {
            Span<const float> times = m_fps.Times();

            constexpr ImPlotAxisFlags AxisFlags = ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_NoHighlight;

            ImPlot::SetupAxes("frame", "ms", ImPlotAxisFlags_NoDecorations | AxisFlags, AxisFlags);
            ImPlot::SetupAxesLimits(0, FpsTracker::TimesCount, 0, Max(m_fps.Max(), 20.0f), ImPlotCond_Always);

            ImPlot::PlotBars("##data_bars", times.Data(), times.Size(), 1.0, 0.0, ImPlotBarsFlags_None, m_fps.Offset());

            ImPlot::EndPlot();
        }
    }
}
