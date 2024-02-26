// Copyright Chad Engler

#include "sched.h"

#include "he/core/thread.h"

extern "C"
{
    int sched_yield()
    {
        he::Thread::Yield();
        return 0;
    }
}
