// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/type_info.h"
#include "he/core/type_traits.h"
#include "he/core/vector.h"

#include <unordered_map>

#if defined(HE_CFG_MODULE_DYNAMIC)
    #define HE_MODULE_EXPORT(Impl) \
        extern "C" HE_DLL_EXPORT he::Module& GetHarvestModule() { static Impl s_module; return s_module; }
#elif defined(HE_CFG_MODULE_STATIC)
    #define HE_MODULE_EXPORT(Impl) \
        struct Impl ## _RegisterModule { \
            Impl ## _RegisterModule() { \
                he::ModuleRegistry::Get().RegisterStaticModule(m_instance); \
            } \
            Impl m_instance; \
        }; \
        static Impl ## _RegisterModule s_ ## Impl ## _RegisterModuleInstance{}
#endif

namespace he
{
    class ModuleRegistry;

    class Module
    {
    public:
        /// Called to allow the module to contribute all the APIs it plans to register.
        virtual bool Register(ModuleRegistry& registry) const = 0;

        /// Called after the registration pass so this module can perform startup work.
        virtual bool Startup() { return true; }

        /// Called when the module is going to be unloaded and must be shut down.
        virtual bool Shutdown() { return true; }

        /// Called when the module is be started again after a hot-reload operation. By
        /// default this just forwards to Startup().
        virtual bool StartupFromReload() { return Startup(); }

        /// Called when the module is being unloaded for the purposed of hot-reload. By
        /// default this just forwards to Shutdown().
        virtual bool ShutdownForReload() { return Shutdown(); }
    };

    class ModuleRegistry
    {
    public:
        static ModuleRegistry& Get();

    public:
        ~ModuleRegistry();

        bool LoadAllModules();

        void RegisterStaticModule(Module& m);

        template <typename I, typename T, HE_REQUIRES(IsBaseOf<I, T>)>
        void RegisterAPI();

        template <typename I>
        I* GetAPI();

    private:
        struct APIEntry
        {
            APIEntry(TypeId concrete) : concreteTypeId(concrete) {}

            TypeId concreteTypeId;

            void* api{ nullptr };
            void*(*create)(){ nullptr };
            void(*destroy)(void*){ nullptr };
        };

    private:
        Vector<Module*> m_staticModules{ CrtAllocator::Get() };
        std::unordered_map<TypeId, APIEntry> m_apis;
    };
}

#include "he/core/inline/module_registry.inl"
