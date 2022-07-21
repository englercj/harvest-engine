// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/delegate.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/vector.h"

#define HE_MODULE_TYPE_CONSOLE_APP      1
#define HE_MODULE_TYPE_WINDOWED_APP     2
#define HE_MODULE_TYPE_SHARED_LIB       3
#define HE_MODULE_TYPE_STATIC_LIB       4

#if HE_CFG_MODULE_TYPE == HE_MODULE_TYPE_STATIC_LIB
    #define HE_EXPORT_MODULE(Impl) \
        static ::he::StaticModuleRegistrar<Impl> HE_UNIQUE_NAME(StaticModuleRegistrar){ HE_CFG_MODULE_NAME }
#else
    #define HE_EXPORT_MODULE(Impl, Name) \
        extern "C" HE_DLL_EXPORT ::he::Module* CreateHarvestModule(::he::Allocator& allocator) { return allocator.New<Impl>(); }
#endif

namespace he
{
    class Module;
    class ModuleRegistry;

    /// Helper to register static modules
    template <typename T>
    struct StaticModuleRegistrar final
    {
        explicit StaticModuleRegistrar(const char* name)
        {
            auto createDelegate = ModuleRegistry::CreateModuleDelegate::Make<&StaticModuleRegistrar::CreateModule>(this);
            ModuleRegistry::Get().RegisterStaticModule(name, createDelegate);
        }

        Module* CreateModule(::he::Allocator& allocator)
        {
            return allocator.New<T>();
        }
    };

    /// Interface for a module.
    class Module
    {
    public:
        virtual ~Module() = default;

        /// Called after the registration pass so this module can perform startup work.
        virtual bool Startup() { return true; }

        /// Called when the module is going to be unloaded and must be shut down.
        virtual bool Shutdown() { return true; }
    };

    /// Registry of modules loaded by Harvest.
    class ModuleRegistry final
    {
    public:
        using CreateModuleDelegate = Delegate<Module*(Allocator&)>;

    public:
        static ModuleRegistry& Get();

    public:
        bool LoadAllModules();
        bool UnloadAllModules();

        void RegisterStaticModule(const char* name, CreateModuleDelegate create);

    private:
        struct StaticModule
        {
            const char* name;
            CreateModuleDelegate create;
            Module* instance;
        };

        Vector<StaticModule> m_staticModules{};
    };
}
