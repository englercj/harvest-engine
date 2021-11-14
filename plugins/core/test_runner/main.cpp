// Copyright Chad Engler

#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/macros.h"
#include "he/core/test.h"

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    HE_UNUSED(argc, argv);
    DebuggerSink sink;
    AddLogSink(sink);
    return RunAllTests();
}
