// Copyright Chad Engler

#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/macros.h"
#include "he/core/module_registry.h"
#include "he/editor/di.h"
#include "he/editor/editor_app.h"
#include "he/editor/editor_data.h"
#include "he/editor/services/directory_service.h"
#include "he/editor/services/log_service.h"
#include "he/window/view.h"

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    // Initialize logging and add the debug sink as early as possible.
    // The application will add the file sink later in LogService.
    AddLogSink(DebuggerSink);

    // Initialize the directory service first so that the log service has existing directories
    // to write data into.
    editor::DirectoryService& directoryService = editor::DICreate<editor::DirectoryService&>();
    if (!directoryService.CreateAll())
        return -1;

    // Initialize the log service as early as possible, so we can capture as many logs as possible.
    // In particular we want to capture any failed module loads to the log file.
    editor::LogService& logService = editor::DICreate<editor::LogService&>();
    if (!logService.Initialize())
        return -1;

    // Create and startup the static modules that were registered by any linked libraries.
    ModuleRegistry& moduleRegistry = editor::DICreate<ModuleRegistry&>();
    moduleRegistry.LoadStaticModules();
    // TODO: load dynamic modules

    if (!moduleRegistry.StartupAllModules())
        return -1;

    // Now that modules are started Create the editor data necessary to run the application and kick off the app.
    editor::EditorData& data = editor::DICreate<editor::EditorData&>();
    data.argc = argc;
    data.argv = argv;
    data.device = window::Device::Create();
    if (!data.device)
        return -1;

    window::ViewDesc desc{};
    desc.title = "Harvest Editor";
    desc.flags = window::ViewFlag::Default | window::ViewFlag::Borderless | window::ViewFlag::AcceptFiles;

    editor::EditorApp& app = editor::DICreate<editor::EditorApp&>();
    const int rc = data.device->Run(app, desc);

    // Destroy all the things in reverse order that we initialized them in.
    window::Device::Destroy(data.device);
    logService.Terminate();
    moduleRegistry.ShutdownAllModules();
    moduleRegistry.UnloadAllModules();

    return rc;
}
