// Copyright Chad Engler

// This file contains implementations of various libc functions for wasm.
// These exist to satisfy contrib libraries which use libc functions.

#pragma once

#include "he/core/clock.h"
#include "he/core/limits.h"
#include "he/core/thread.h"
#include "he/core/utils.h"

#if HE_PLATFORM_WASM

#include "wasm_core.js.h"

#include <sched.h>
#include <unistd.h>

extern "C"
{
    // https://man7.org/linux/man-pages/man2/sched_yield.2.html
    int sched_yield()
    {
        // wasm doesn't have an explicit yield.
        return 0;
    }
}

#endif
