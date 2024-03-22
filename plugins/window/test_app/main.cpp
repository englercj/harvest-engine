#include "test_app.h"

#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/macros.h"
#include "he/window/device.h"

#include "he/core/main.inl"
int he::AppMain([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    AddLogSink(DebuggerSink);

    window::Device* device = window::Device::Create();
    if (!device)
        return -1;

    TestApp app(device);
    int rc = device->Run(app);

    window::Device::Destroy(device);

    return rc;
}
