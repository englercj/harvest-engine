// Copyright Chad Engler

#include "debugger_impl.h"
#include "platform_support.h"

#include "he/core/compiler.h"
#include "he/core/module_registry.h"

namespace he
{
    class PlatformModule : public Module
    {
    public:
        const ModuleInfo& GetInfo() const override
        {
            static const ModuleInfo s_info{
                "Harvest Platform",
                "Provides platform support for the base engine platforms (Windows, Linux, Emscripten).",
                "1.0.0",
            };
            return s_info;
        }

        bool Register(ModuleRegistry& registry) const override
        {
        #if HE_CAN_PROVIDE_PLATFORM
            registry.RegisterAPI<Debugger, DebuggerImpl>();
        #endif

            return true;
        }
    };
}

HE_MODULE_EXPORT(he::PlatformModule);
