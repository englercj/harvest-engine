#include "scribe_test_app.h"

#include "he/core/debugger.h"
#include "he/core/error.h"
#include "he/core/file.h"
#include "he/core/key_value_fmt.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/string_fmt.h"
#include "he/window/device.h"

namespace
{
    bool ScribeTestAppErrorHandler(
        [[maybe_unused]] void* userData,
        const he::ErrorSource& source,
        const he::KeyValue* kvs,
        uint32_t count)
    {
        he::String message;
        he::FormatTo(
            message,
            "error.kind = {}\nsource.file = {}\nsource.line = {}\nsource.func = {}\n",
            static_cast<uint32_t>(source.kind),
            source.file,
            source.line,
            source.funcName);

        for (uint32_t i = 0; i < count; ++i)
        {
            he::FormatTo(message, "{}\n", kvs[i]);
        }

        he::PrintToDebugger(message.Data());
        he::File::WriteAll(message.Data(), message.Size(), "he_scribe_test_app.error.txt");
        return false;
    }
}

#include "he/core/main.inl"
int he::AppMain([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    AddLogSink(DebuggerSink);
    SetErrorHandler(&ScribeTestAppErrorHandler);

    window::Device* device = window::Device::Create();
    if (!device)
    {
        return -1;
    }

    ScribeTestApp app(device);
    const int rc = device->Run(app);

    window::Device::Destroy(device);
    return rc;
}
