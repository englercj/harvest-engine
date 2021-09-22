// Copyright Chad Engler

#include "frame_time_histogram.h"

#include "he/core/memory_ops.h"
#include "he/math/float.h"

#include "imgui.h"

namespace he::editor
{
    void FrameTimeHistogram::Clear()
    {
        m_timesTotal = 0.0f;
        m_countsTotal = 0.0f;
        MemZero(m_times, sizeof(m_times));
        MemZero(m_counts, sizeof(m_counts));
        MemZero(m_hitchTimes, sizeof(m_hitchTimes));
        MemZero(m_hitchCounts, sizeof(m_hitchCounts));
        MemZero(m_frameTimes, sizeof(m_frameTimes));
    }

    void FrameTimeHistogram::Update(float deltaTime)
    {
        HE_ASSERT(deltaTime >= 0.0f);

        MemMove(m_frameTimes, m_frameTimes + 1, sizeof(float) * (HistorySize - 1));
        m_frameTimes[HistorySize - 1] = (deltaTime / m_unitDivisor);

        size_t bin = GetBin(deltaTime);

        m_times[bin] += deltaTime;
        m_timesTotal += deltaTime;

        m_counts[bin] += 1.0f;
        m_countsTotal += 1.0f;

        float hitch = Abs(m_lastDeltaTime - deltaTime);
        size_t deltaBin = GetBin(hitch);
        m_hitchTimes[deltaBin] += hitch;
        m_hitchCounts[deltaBin] += 1.0f;
        m_lastDeltaTime = deltaTime;
    }

    void FrameTimeHistogram::Show()
    {
        size_t shownCount = 0;

        if (ImGui::CollapsingHeader("Frame Times"))
        {
            ++shownCount;
            ImGui::PlotHistogram("##frame_times", m_frameTimes, HistorySize, 0, nullptr, FLT_MAX, FLT_MAX, m_size);
        }

        if (ImGui::CollapsingHeader("Time Histogram"))
        {
            ++shownCount;
            ImGui::PlotHistogram("##time_histogram", m_times, HistorySize, 0, nullptr, FLT_MAX, FLT_MAX, m_size);
            PlotRefreshLines(m_timesTotal, m_times);
        }

        if (ImGui::CollapsingHeader("Count Histogram"))
        {
            ++shownCount;
            ImGui::PlotHistogram("##count_histogram", m_counts, HistorySize, 0, nullptr, FLT_MAX, FLT_MAX, m_size);
            PlotRefreshLines(m_countsTotal, m_counts);
        }

        if (ImGui::CollapsingHeader("Hitch Time Histogram"))
        {
            ++shownCount;
            ImGui::PlotHistogram("##hitch_time_histogram", m_hitchTimes, HistorySize, 0, nullptr, FLT_MAX, FLT_MAX, m_size);
            PlotRefreshLines();
        }

        if (ImGui::CollapsingHeader("Hitch Count Histogram"))
        {
            ++shownCount;
            ImGui::PlotHistogram("##hitch_count_histogram", m_hitchCounts, HistorySize, 0, nullptr, FLT_MAX, FLT_MAX, m_size);
            PlotRefreshLines();
        }

        if (ImGui::Button("Clear"))
        {
            Clear();
        }

        CalcHistogramSize(shownCount);
    }

    size_t FrameTimeHistogram::GetBin(float time)
    {
        size_t bin = static_cast<size_t>(Floor(time / m_unitDivisor));
        if (bin >= HistorySize)
        {
            bin = HistorySize - 1;
        }
        return bin;
    }

    void FrameTimeHistogram::PlotRefreshLines(float total, float* values)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 pad = style.FramePadding;
        ImVec2 min = ImGui::GetItemRectMin();
        min.x += pad.x;
        ImVec2 max = ImGui::GetItemRectMax();
        max.x -= pad.x;

        float xRefresh = (max.x - min.x) * m_targetRefreshRate / (m_unitDivisor * HistorySize);

        float xCurr = xRefresh + min.x;
        while (xCurr < max.x)
        {
            float xP = Ceil(xCurr); // use ceil to get integer coords or else lines look odd
            draw->AddLine(ImVec2(xP, min.y), ImVec2(xP, max.y), 0x50FFFFFF);
            xCurr += xRefresh;
        }

        if (values)
        {
            // calc markers
            float currTotal = 0.0f;
            int mark = 0;
            for (int i = 0; i < HistorySize && mark < MarkerCount; ++i)
            {
                currTotal += values[i];
                if (total * m_markers[mark] < currTotal)
                {
                    // use ceil to get integer coords or else lines look odd
                    float xP = Ceil(static_cast<float>(i + 1) / static_cast<float>(HistorySize) * (max.x - min.x) + min.x);
                    draw->AddLine(ImVec2(xP, min.y), ImVec2(xP, max.y), 0xFFFF0000);
                    ++mark;
                }
            }
        }
    }

    void FrameTimeHistogram::CalcHistogramSize(size_t shownCount)
    {
        ImVec2 wRegion = ImGui::GetContentRegionMax();
        float heightGone = 7.0f * ImGui::GetFrameHeightWithSpacing();
        wRegion.y -= heightGone;
        wRegion.y /= static_cast<float>(shownCount);
        const ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 pad = style.FramePadding;
        wRegion.x -= 2.0f * pad.x;
        m_size = wRegion;
    }
}
