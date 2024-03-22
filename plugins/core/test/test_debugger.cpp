// Copyright Chad Engler

#include "he/core/debugger.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, debugger, PrintToDebugger)
{
    PrintToDebugger("Debug print test.");
    PrintToDebugger("Debug print test: {}, {}.", 1, false);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, debugger, IsDebuggerAttached)
{
    [[maybe_unused]] const bool attached = IsDebuggerAttached();
    attached;
}
