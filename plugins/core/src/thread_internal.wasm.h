// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he
{
    // TODO: When starting main, call with (ptr, !ENVIRONMENT_IS_WORKER, true, !ENVIRONMENT_IS_WEB)
    // TODO: When starting a worker, call with (ptr, false, false, false) and on exit call (null, false, false, true)
    // TODO: Em also calls this after create on 'run' with (ptr, false, false, true)
    //    -> This is because workers can block, main thread cannot. Uses this info in futex implementation.
    // This pthread stores info about the thread: name, id, attach state, stack size, tls, etc.
    // For main, make a static one and pass it in. For new threads alloc one on heap, and pass it in.
    // Emscripten has pthreads in a linked list as well
    void _SetThreadState(pthread* ptr, bool isMain, bool isRuntime, bool canBlock);
}
