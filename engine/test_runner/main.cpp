// Copyright Chad Engler

#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/core/main.h"
#include "he/core/test.h"

int he::AppMain(int argc, char* argv[])
{
    HE_UNUSED(argc, argv);
    AddLogSink(DebugOutputSink);
    return RunAllTests();
}
