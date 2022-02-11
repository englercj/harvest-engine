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
    const bool attached = IsDebuggerAttached();
    HE_UNUSED(attached);
}
