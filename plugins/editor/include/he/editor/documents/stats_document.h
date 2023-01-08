// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/types.h"
#include "he/editor/documents/document.h"

namespace he::editor
{
    // --------------------------------------------------------------------------------------------
    class FpsTracker
    {
    public:
        static constexpr uint32_t TimesCount = 256;

    public:
        void Update(float dt);

        float Min() const { return m_min; }
        float Max() const { return m_max; }
        float Avg() const { return static_cast<float>(m_total / m_count); }

        Span<const float> Times() const { return { m_times, m_count }; }
        uint32_t Offset() const { return m_offset; }

    private:
        void ScanForMinMax();

    private:
        uint32_t m_count{ 0 };
        uint32_t m_offset{ 0 };
        double m_total{ 0 };

        float m_times[TimesCount]{};
        float m_min{ FLT_MAX };
        float m_max{ FLT_MIN };
    };

    // --------------------------------------------------------------------------------------------
    class StatsDocument : public Document
    {
    public:
        StatsDocument() noexcept;

        void Show() override;

    private:
        FpsTracker m_fps;
    };
}
