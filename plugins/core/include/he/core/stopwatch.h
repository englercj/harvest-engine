// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/types.h"

namespace he
{
    template <typename T = MonotonicClock>
    class Stopwatch final
    {
    public:
        using ClockType = T;

    public:
        Timer() noexcept { Restart(); }

        void Restart() { m_start = T::Now(); }
        [[nodiscard]] Duration Elapsed() const { return T::Now() - m_start; }

    private:
        T::Time m_start{};
    };
}
