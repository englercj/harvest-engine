// Copyright Chad Engler

#include "di.h"
#include "editor_app.h"
#include "editor_data.h"

#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/macros.h"
#include "he/core/main.inl"
#include "he/window/view.h"

int he::AppMain(int argc, char* argv[])
{
    // Initialize logging and add the debug sink as early as possible.
    // We'll add the file sync later after we prepare the directories for writing logs to.
    he::AddLogSink(DebuggerSink);

    editor::EditorData& data = editor::DICreate<editor::EditorData&>();
    data.argc = argc;
    data.argv = argv;
    data.device = window::Device::Create();
    if (!data.device)
        return -1;

    window::ViewDesc desc{};
    desc.title = "Harvest";
    desc.flags = window::ViewFlag::Default | window::ViewFlag::Borderless;

    editor::EditorApp& app = editor::DICreate<editor::EditorApp&>();
    int rc = data.device->Run(app, desc);

    window::Device::Destroy(data.device);

    return rc;
}
