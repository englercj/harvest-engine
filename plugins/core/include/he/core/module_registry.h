// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/delegate.h"
#include "he/core/string.h"
#include "he/core/system.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

#include <unordered_map>

#define HE_MODULE_TYPE_CONSOLE_APP      1
#define HE_MODULE_TYPE_WINDOWED_APP     2
#define HE_MODULE_TYPE_SHARED_LIB       3
#define HE_MODULE_TYPE_STATIC_LIB       4

/// \def HE_EXPORT_MODULE
/// Exports a Harvest module to be usable by the module system.
#if HE_CFG_MODULE_TYPE == HE_MODULE_TYPE_SHARED_LIB
    #define HE_EXPORT_MODULE(Impl) \
        extern "C" HE_DLL_EXPORT const char* GetHarvestModuleName() { return HE_STRINGIFY(HE_CFG_MODULE_NAME); } \
        extern "C" HE_DLL_EXPORT ::he::TypeInfo GetHarvestModuleTypeInfo() { return ::he::TypeInfo::Get<Impl>(); } \
        extern "C" HE_DLL_EXPORT ::he::UniquePtr<::he::Module> CreateHarvestModule() { return ::he::MakeUnique<Impl>(); }
#else
    #define HE_EXPORT_MODULE(Impl) \
        HE_RETAIN ::he::StaticModuleRegistrar<Impl> HE_PP_JOIN(HE_CFG_MODULE_NAME, _ModuleRegistrar){ HE_STRINGIFY(HE_CFG_MODULE_NAME) }
#endif

namespace he
{
    class Module;
    class ModuleRegistry;

    // --------------------------------------------------------------------------------------------

    /// Helper to register static modules. Not intended to be used directly.
    ///
    /// \internal
    /// \see HE_EXPORT_MODULE
    template <typename T>
    struct StaticModuleRegistrar final
    {
        explicit StaticModuleRegistrar(const char* name);

        static UniquePtr<Module> CreateModule(const void*)
        {
            return MakeUnique<T>();
        }
    };

    // --------------------------------------------------------------------------------------------

    /// Interface for a module.
    class HE_DLL_EXPORT Module
    {
    public:
        virtual ~Module() = default;

        /// Called just after the module is created. During the register pass the module may
        /// register any APIs that it wishes to provide.
        virtual void Register() {}

        /// Called just before the module is destroyed.
        virtual void Unregister() {}

        /// Called after the registration pass so this module can perform startup work, including
        /// using the APIs provided by other modules.
        ///
        /// \return True if startup was successful, or false if the module was unable to startup.
        virtual bool Startup() { return true; }

        /// Called when the module is going to be unloaded and must be shut down.
        virtual void Shutdown() {}

    public:
        /// Accessor for the registry this module is created in. Can be used to access other
        /// modules and their provided APIs.
        ///
        /// \return The registry that owns this module.
        ModuleRegistry& Registry() { return *m_owner; }

    private:
        friend ModuleRegistry;
        ModuleRegistry* m_owner{ nullptr };
    };

    // --------------------------------------------------------------------------------------------

    /// Registry of modules loaded by Harvest.
    class ModuleRegistry final
    {
    public:
        using CreateModuleDelegate = Delegate<UniquePtr<Module>()>;

    public:
        ModuleRegistry() = default;
        ModuleRegistry(const ModuleRegistry&) = delete;
        ModuleRegistry(ModuleRegistry&&) = delete;
        ~ModuleRegistry() noexcept;

        ModuleRegistry& operator=(const ModuleRegistry&) = delete;
        ModuleRegistry& operator=(ModuleRegistry&&) = delete;

        void LoadStaticModules();
        bool LoadDynamicModule(const char* path);

        void UnloadAllModules();

        bool StartupAllModules();
        void ShutdownAllModules();

        // Register a pre-existing instance. Lifetime ownership is not transferred so this instance
        // must not be destroyed until after it is unregistered.
        template <typename T> void RegisterApi(T& instance);
        template <typename T> void RegisterApi();

        template <typename T> void UnregisterApi();

        template <typename T> T* FindApi();
        template <typename T> T& GetApi();

    private:
        template <typename T>
        friend struct StaticModuleRegistrar;

        static void RegisterStaticModule(const char* name, TypeInfo type, CreateModuleDelegate create);

    private:
        using Pfn_GetHarvestModuleName = const char*(*)();
        using Pfn_GetHarvestModuleTypeInfo = TypeInfo(*)();
        using Pfn_CreateHarvestModule = UniquePtr<Module>(*)();

        struct StaticModule
        {
            const char* name{ nullptr };
            TypeInfo type{};
            CreateModuleDelegate create{};
        };

        struct ModuleEntry
        {
            const char* name{ nullptr };
            TypeInfo type{};
            UniquePtr<Module> instance{};
            DynamicLib dl{};
        };

        struct ApiEntry
        {
            void* instance{ nullptr };
            void(*destroy)(const void*){ nullptr };
        };

        static Vector<ModuleRegistry::StaticModule>& StaticModules();

    private:
        Vector<ModuleEntry> m_modules{};
        std::unordered_map<TypeInfo, ApiEntry> m_apis{};
    #if HE_ENABLE_ASSERTIONS
        bool m_startupOrLater{ false };
    #endif
    };

    // --------------------------------------------------------------------------------------------
    // Inline implementations

    template <typename T>
    StaticModuleRegistrar<T>::StaticModuleRegistrar(const char* name)
    {
        const auto cb = ModuleRegistry::CreateModuleDelegate::Make(&StaticModuleRegistrar::CreateModule);
        ModuleRegistry::RegisterStaticModule(name, TypeInfo::Get<T>(), cb);
    }

    template <typename T>
    inline void ModuleRegistry::RegisterApi(T& instance)
    {
        constexpr TypeInfo Info = TypeInfo::Get<T>();

        const auto pair = m_apis.try_emplace(Info);
        if (!HE_VERIFY(pair.second,
            HE_MSG("API has already been registered"),
            HE_KV(name, Info.Name()),
            HE_KV(hash, Info.Hash())))
        {
            return;
        }

        ApiEntry& entry = pair.first->second;
        entry.instance = &instance;
        entry.destroy = nullptr;
    }

    template <typename T>
    inline void ModuleRegistry::RegisterApi()
    {
        constexpr TypeInfo Info = TypeInfo::Get<T>();

        const auto pair = m_apis.try_emplace(Info);
        if (!HE_VERIFY(pair.second,
            HE_MSG("API has already been registered"),
            HE_KV(name, Info.Name()),
            HE_KV(hash, Info.Hash())))
        {
            return;
        }

        ApiEntry& entry = pair.first->second;
        entry.instance = Allocator::GetDefault().New<T>();
        entry.destroy = [](const void* api) { Allocator::GetDefault().Delete(static_cast<const T*>(api)); };
    }

    template <typename T>
    inline void ModuleRegistry::UnregisterApi()
    {
        constexpr TypeInfo Info = TypeInfo::Get<T>();
        const auto it = m_apis.find(Info);

        if (it != m_apis.end())
        {
            ApiEntry& entry = it->second;
            entry.destroy(entry.instance);
        }

        m_apis.erase(it);
    }

    template <typename T>
    inline T* ModuleRegistry::FindApi()
    {
        HE_ASSERT(m_startupOrLater, HE_MSG("You can only get a registered API after the registration pass has completed."));

        constexpr TypeInfo Info = TypeInfo::Get<T>();

        const auto it = m_apis.find(Info);
        if (it == m_apis.end())
            return nullptr;

        return static_cast<T*>(it->second.instance);
    }

    template <typename T>
    inline T& ModuleRegistry::GetApi()
    {
        T* api = FindApi<T>();
        HE_ASSERT(api);
        return *api;
    }
}
