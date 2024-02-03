// Copyright Chad Engler

// This file contains implementations of various libc functions for wasm.
// These exist to satisfy contrib libraries which use libc functions.

#pragma once

#include "he/core/thread.h"

#if HE_PLATFORM_WASM

#include <sched.h>
#include <unistd.h>

extern "C"
{
    // https://man7.org/linux/man-pages/man2/sched_yield.2.html
    int sched_yield()
    {
        he::Thread::Yield();
        return 0;
    }
}

#endif
