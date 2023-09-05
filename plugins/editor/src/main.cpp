// Copyright Chad Engler

#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/types.h"
#include "he/editor/di.h"
#include "he/editor/editor_app.h"

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    // Add the debug sink as early as possible.
    // A file sink is added later during LogService initialization.
    AddLogSink(DebuggerSink);

    // Create our application and run it. This starts the main loop, which continues until the
    // application decides to quit.
    editor::EditorApp& app = editor::DICreate<editor::EditorApp&>();
    if (!app.Initialize(argc, argv))
        return -1;

    const int rc = app.Run();

    app.Terminate();
    return rc;
}
