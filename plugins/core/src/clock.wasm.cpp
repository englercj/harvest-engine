// Copyright Chad Engler

#include "he/core/clock.h"

#if defined(HE_PLATFORM_WASM)

#include "wasm_core.js.h"

namespace he
{
    template <>
    SystemTime SystemClock::Now()
    {
        const uint32_t nowMs = heWASM_GetDateNow();
        return { nowMs * 1000000ull };
    }

    template <>
    MonotonicTime MonotonicClock::Now()
    {
        const double nowMs = heWASM_GetPerformanceNow();
        return { static_cast<uint64_t>(nowMs * 1000000ull) };
    }

    bool IsDaylightSavingTimeActive()
    {
        return heWASM_IsDaylightSavingTimeActive();
    }

    Duration GetLocalTimezoneOffset()
    {
        // Offset is expressed in minutes and is negative for time zones west of UTC and positive
        // for time zones east of UTC.
        const uint32_t offsetMinutes = heWASM_GetDateTzOffset();
        return FromPeriod<Minutes>(-offsetMinutes);
    }
}

#endif
