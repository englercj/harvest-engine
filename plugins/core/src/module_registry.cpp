// Copyright Chad Engler

#include "he/core/module_registry.h"

#include "he/core/config.h"
#include "he/core/log.h"
#include "he/core/system.h"
#include "he/core/type_info_fmt.h"

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
            ModuleEntry entry;
            entry.name = m.name;
            entry.type = m.type;
            entry.instance = m.create();
            if (!entry.instance)
            {
                HE_LOG_ERROR(he_core,
                    HE_MSG("Static module load failed. Failed to instantiate the harvest module object."),
                    HE_KV(name, m.name),
                    HE_KV(type, m.type));
                continue;
            }

            entry.instance->m_owner = this;
            entry.instance->Register();
            entry.destroy = m.destroy;

            m_modules.EmplaceBack(Move(entry));
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
        const Pfn_DestroyHarvestModule pfnDestroyHarvestModule = entry.dl.Symbol<Pfn_DestroyHarvestModule>("DestroyHarvestModule");

        if (!pfnGetHarvestModuleName || !pfnGetHarvestModuleTypeInfo || !pfnCreateHarvestModule || !pfnDestroyHarvestModule)
        {
            HE_LOG_ERROR(he_core,
                HE_MSG("Dynamic module load failed. The shared library is missing expected symbols. Does this library export a Harvest module?"),
                HE_KV(path, path));
            return false;
        }

        entry.name = pfnGetHarvestModuleName();
        entry.type = pfnGetHarvestModuleTypeInfo();
        entry.instance = pfnCreateHarvestModule();

        if (!entry.instance)
        {
            HE_LOG_ERROR(he_core,
                HE_MSG("Dynamic module load failed. Failed to instantiate the harvest module object."),
                HE_KV(path, path),
                HE_KV(name, entry.name),
                HE_KV(type, entry.type));
            return false;
        }

        entry.instance->Register();
        entry.destroy = pfnDestroyHarvestModule;

        m_modules.PushBack(Move(entry));
        return true;
    }

    void ModuleRegistry::UnloadAllModules()
    {
        // Delete all the APIs that were provided by modules
        for (const auto& pair : m_apis)
        {
            const ApiEntry& entry = pair.value;
            if (entry.destroy)
                entry.destroy(entry.instance);
        }
        m_apis.Clear();

        // Delete the module instances and close any dynamic libraries that were opened.
        for (ModuleEntry& entry : m_modules)
        {
            entry.destroy(entry.instance);
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
}
