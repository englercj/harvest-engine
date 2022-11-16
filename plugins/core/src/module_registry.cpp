// Copyright Chad Engler

#include "he/core/module_registry.h"

#include "he/core/config.h"
#include "he/core/log.h"
#include "he/core/system.h"

namespace he
{
    Vector<ModuleRegistry::StaticModule>& ModuleRegistry::StaticModules()
    {
        static Vector<ModuleRegistry::StaticModule> s_staticModules{};
        return s_staticModules;
    }

    ModuleRegistry::~ModuleRegistry() noexcept
    {
        UnloadAllModules();
    }

    void ModuleRegistry::LoadStaticModules()
    {
        for (StaticModule& m : StaticModules())
        {
            ModuleEntry& entry = m_modules.EmplaceBack();
            entry.name = m.name;
            entry.type = m.type;
            entry.instance = m.create();
            entry.instance->m_owner = this;
            entry.instance->Register();
        }
    }

    bool ModuleRegistry::LoadDynamicModule(const char* path)
    {
        ModuleEntry entry;
        entry.dl.Open(path);

        if (!entry.dl.IsOpen())
        {
            HE_LOG_ERROR(he_core, HE_MSG("Failed to load dynamic module."), HE_KV(path, path));
            return false;
        }

        const Pfn_GetHarvestModuleName pfnGetHarvestModuleName = entry.dl.Symbol<Pfn_GetHarvestModuleName>("GetHarvestModuleName");
        const Pfn_GetHarvestModuleTypeInfo pfnGetHarvestModuleTypeInfo = entry.dl.Symbol<Pfn_GetHarvestModuleTypeInfo>("GetHarvestModuleTypeInfo");
        const Pfn_CreateHarvestModule pfnCreateHarvestModule = entry.dl.Symbol<Pfn_CreateHarvestModule>("CreateHarvestModule");

        if (!pfnGetHarvestModuleName || !pfnGetHarvestModuleTypeInfo || !pfnCreateHarvestModule)
        {
            HE_LOG_ERROR(he_core,
                HE_MSG("Dynamic module is missing expected symbols. Does this library export a Harvest module?"),
                HE_KV(path, path));
            return false;
        }

        entry.name = pfnGetHarvestModuleName();
        entry.type = pfnGetHarvestModuleTypeInfo();
        entry.instance = pfnCreateHarvestModule();
        entry.instance->Register();

        return true;
    }

    void ModuleRegistry::UnloadAllModules()
    {
        // Delete all the APIs that were provided by modules
        for (auto&& it : m_apis)
        {
            ApiEntry& entry = it.second;

            if (entry.destroy)
                entry.destroy(entry.instance);
        }
        m_apis.clear();

        // Delete the module instances and close any dynamic libraries that were opened.
        for (ModuleEntry& entry : m_modules)
        {
            entry.instance.Reset();
            entry.dl.Close();
        }
        m_modules.Clear();
    }

    bool ModuleRegistry::StartupAllModules()
    {
    #if HE_ENABLE_ASSERTIONS
        m_startupOrLater = true;
    #endif

        for (ModuleEntry& entry : m_modules)
        {
            if (!entry.instance->Startup())
            {
                HE_LOG_ERROR(he_core, HE_MSG("Module failed to startup."), HE_KV(module_name, entry.name));
                return false;
            }
        }

        return true;
    }

    void ModuleRegistry::ShutdownAllModules()
    {
        for (ModuleEntry& entry : m_modules)
        {
            entry.instance->Shutdown();
        }
    }

    void ModuleRegistry::RegisterStaticModule(const char* name, TypeInfo type, CreateModuleDelegate create)
    {
        StaticModules().PushBack({ name, type, create });
    }
}
