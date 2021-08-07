// Copyright Chad Engler

namespace he
{
    template <typename I, typename T, HE_REQUIRED(IsBaseOf<I, T>)>
    void ModuleRegistry::RegisterAPI()
    {
        constexpr TypeId InterfaceTypeId = GetTypeId<I>();
        constexpr TypeId ConcreteTypeId = GetTypeId<T>();

        APIEntry entry(ConcreteTypeId);
        entry.create = []() -> void* { return new T(); };
        entry.destroy = [](void* v) { delete static_cast<T*>(v); };

        m_apis.emplace(InterfaceTypeId, entry);
    }

    template <typename I>
    I* ModuleRegistry::GetAPI()
    {
        constexpr TypeId InterfaceTypeId = GetTypeId<I>();

        auto it = m_apis.find(InterfaceTypeId);
        if (it == m_apis.end())
            return nullptr;

        APIEntry& entry = it->second;

        if (!entry.api)
            entry.api = entry.create();

        return static_cast<I*>(entry.api);
    }
}
