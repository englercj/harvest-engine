// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"

#include <unordered_map>

#define HE_MODULE_EXPORT(Impl) extern "C" HE_DLL_EXPORT he::Module& GetHarvestModule() { static Impl s_module; return s_module; }

namespace he
{
    class ModuleRegistry
    {
    public:
        static ModuleRegistry& Get();

        template <typename I, typename T>
        void RegisterAPI();

        template <typename I>
        I* GetAPI();
    };

    struct ModuleInfo
    {
        const char* name;       ///< Friendly name of the module
        const char* desc;       ///< Description of the module
        const char* version;    ///< Version string of the module
    };

    class Module
    {
    public:
        /// Returns information about this module.
        virtual const ModuleInfo& GetInfo() const = 0;

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
}

#include "he/core/inline/module_registry.inl"
