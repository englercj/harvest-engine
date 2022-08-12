// Copyright Chad Engler

#pragma once

#include "document.h"
#include "widgets/frame_time_histogram.h"

namespace he::editor
{
    class StatsDocument : public Document
    {
    public:
        StatsDocument() noexcept;

        void Show() override;

    private:
        FrameTimeHistogram m_frameTime{};
    };
}
