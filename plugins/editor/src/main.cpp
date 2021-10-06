// Copyright Chad Engler

#include "di.h"
#include "editor_app.h"
#include "editor_data.h"

#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/macros.h"
#include "he/core/main.inl"
#include "he/window/view.h"

namespace he::editor
{
    const AppInjectorType* g_appInjector = nullptr;
}

int he::AppMain(int argc, char* argv[])
{
    // Initialize logging and add the debug sink as early as possible.
    // We'll add the file sync later after we prepare the directories for writing logs to.
    DebuggerSink debugSink(CrtAllocator::Get());
    he::AddLogSink(debugSink);

    const auto injector = editor::MakeAppInjector();
    editor::g_appInjector = &injector;

    editor::EditorData& data = injector.create<editor::EditorData&>();
    data.argc = argc;
    data.argv = argv;
    data.device = window::CreateDevice();
    if (!data.device)
        return -1;

    window::ViewDesc desc{};
    desc.title = "Harvest";
    desc.flags = window::ViewFlag::Default | window::ViewFlag::Borderless;

    editor::EditorApp& app = injector.create<editor::EditorApp&>();
    int rc = data.device->Run(app, desc);

    window::DestroyDevice(data.device);

    he::RemoveLogSink(debugSink);

    return rc;
}
