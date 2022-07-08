// Copyright Chad Engler

#include "he/core/module_registry.h"

#include "he/core/config.h"

namespace he
{
    ModuleRegistry& ModuleRegistry::Get()
    {
        static ModuleRegistry s_registry;
        return s_registry;
    }

    bool ModuleRegistry::LoadAllModules()
    {
        Allocator& allocator = Allocator::GetDefault();

        for (StaticModule& m : m_staticModules)
        {
            m.instance = m.create(allocator);

            if (!m.instance->Startup())
                return false;
        }

        // TODO: load and register dynamic modules

        return false;
    }

    bool ModuleRegistry::UnloadAllModules()
    {
        Allocator& allocator = Allocator::GetDefault();

        for (StaticModule& m : m_staticModules)
        {
            const bool r = m.instance->Shutdown();

            allocator.Delete(m.instance);
            m.instance = nullptr;

            if (!r)
                return false;
        }

        // TODO: unload registered dynamic modules

        return false;
    }

    void ModuleRegistry::RegisterStaticModule(const char* name, CreateModuleDelegate create)
    {
        m_staticModules.PushBack({ name, create });
    }
}
