// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/core/types.h"

#include <thread>

namespace he
{
    using ThreadHandle = uintptr_t;

    ThreadHandle GetCurrentThreadHandle();
    Result SetThreadAffinity(ThreadHandle thread, uint64_t mask);
    void SetCurrentThreadName(const char* name);
}
