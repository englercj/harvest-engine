// Copyright Chad Engler

#include "he/core/debug.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, debug, PrintToDebugger)
{
    PrintToDebugger("Debug print test.");
    PrintToDebugger("Debug print test: {}, {}.", 1, false);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, debug, IsDebuggerAttached)
{
    const bool attached = IsDebuggerAttached();
    HE_UNUSED(attached);
}
