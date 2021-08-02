// Copyright Chad Engler

#include "he/core/module_registry.h"

namespace he
{
    ModuleRegistry& ModuleRegistry::Get()
    {
        static ModuleRegistry s_registry;
        return s_registry;
    }
}
