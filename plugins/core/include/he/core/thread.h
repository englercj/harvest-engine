// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/result.h"
#include "he/core/types.h"

namespace he
{
    using ThreadHandle = uintptr_t;

    ThreadHandle GetCurrentThreadHandle();
    Result SetThreadAffinity(ThreadHandle thread, uint64_t mask);
    void SetCurrentThreadName(const char* name);

    void SleepCurrentThread(Duration amount);
    void YieldCurrentThread();
}
