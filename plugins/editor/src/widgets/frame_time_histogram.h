// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/clock.h"

#include "imgui.h"

namespace he::editor
{
    class FrameTimeHistogram
    {
    public:
        static constexpr int HistorySize = 101;
        static constexpr int MarkerCount = 2;

        FrameTimeHistogram() { Clear(); }

        void Clear();

        template <typename T>
        void SetDisplayUnits() { m_unitDivisor = static_cast<float>(T::Ratio / static_cast<double>(Seconds::Ratio)); }
        void SetTargetRefreshRate(float rateInSeconds) { m_targetRefreshRate = rateInSeconds; }
        void SetMarkerThreshold(size_t index, float value) { HE_ASSERT(index < MarkerCount); m_markers[index] = value; }

        void Update(float deltaSeconds);
        void Show();

    private:
        size_t GetBin(float time);
        void PlotRefreshLines(float total = 0.0f, float* values = nullptr);
        void CalcHistogramSize(size_t shownCount);

    private:
        ImVec2 m_size{ 3.0f * HistorySize, 40.0f };
        float m_lastDeltaTime{ 0 };
        float m_timesTotal{ 0 };
        float m_countsTotal{ 0 };
        float m_times[HistorySize]{};
        float m_counts[HistorySize]{};
        float m_hitchTimes[HistorySize]{};
        float m_hitchCounts[HistorySize]{};
        float m_frameTimes[HistorySize]{};

        float m_unitDivisor{ 0.001f };
        float m_targetRefreshRate{ 1.0f / 60.0f };
        float m_markers[MarkerCount]{ 0.99f, 0.999f };
    };
}
