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
