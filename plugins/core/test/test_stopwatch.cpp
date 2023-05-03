// Copyright Chad Engler

#include "he/core/stopwatch.h"

#include "he/core/clock_fmt.h"
#include "he/core/thread.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, stopwatch, Test)
{
    Stopwatch timer;
    SleepCurrentThread(FromPeriod<Milliseconds>(10));
    HE_EXPECT_GT(timer.Elapsed(), FromPeriod<Milliseconds>(10));

    timer.Restart();
    SleepCurrentThread(FromPeriod<Milliseconds>(50));
    HE_EXPECT_GT(timer.Elapsed(), FromPeriod<Milliseconds>(50));
}
