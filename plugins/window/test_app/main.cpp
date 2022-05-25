#include "test_app.h"

#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/macros.h"
#include "he/window/device.h"

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    HE_UNUSED(argc, argv);

    AddLogSink(DebuggerSink);

    window::Device* device = window::CreateDevice();
    if (!device)
        return -1;

    window::ViewDesc desc;
    desc.title = "HE Window Test App";

    TestApp app(device);
    int rc = device->Run(app, desc);

    window::DestroyDevice(device);

    return rc;
}
