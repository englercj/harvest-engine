// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/result.h"
#include "he/core/types.h"

namespace he
{
    uintptr_t GetCurrentThreadHandle();
    uint32_t GetCurrentThreadId();

    Result SetThreadAffinity(uintptr_t thread, uint64_t mask);
    void SetCurrentThreadName(const char* name);

    void SleepCurrentThread(Duration amount);
    void YieldCurrentThread();
}
