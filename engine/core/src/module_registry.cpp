// Copyright Chad Engler

#include "he/core/module_registry.h"

namespace he
{
    ModuleRegistry::~ModuleRegistry()
    {
        for (auto&& pair : m_apis)
        {
            APIEntry& entry = pair.second;
            entry.destroy(entry.api);
            entry.api = nullptr;
        }
    }

    bool ModuleRegistry::LoadAllModules()
    {
        return false;
    }

    void ModuleRegistry::RegisterStaticModule(Module& m)
    {
        m_staticModules.PushBack(&m);
    }

    ModuleRegistry& ModuleRegistry::Get()
    {
        static ModuleRegistry s_registry;
        return s_registry;
    }
}
