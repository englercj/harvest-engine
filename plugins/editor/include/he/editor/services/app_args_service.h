// Copyright Chad Engler

#pragma once

#include "he/core/args.h"
#include "he/core/assert.h"
#include "he/core/hash_table.h"
#include "he/core/types.h"
#include "he/core/type_info.h"
#include "he/core/utils.h"

namespace he::editor
{
    struct EditorFlags
    {
        bool help{ false };
    };

    class AppArgsService
    {
    public:
        AppArgsService() noexcept;

        const EditorFlags& Flags() const { return m_flags; }
        Span<const char* const> Values() const { return m_args.values; }
        String Help() const;

        bool Initialize(int argc, char** argv);

        template <typename T>
        bool RegisterArg(char shortName, const char* longName, const char* description = nullptr, ArgFlag flags = ArgFlag::None);

        template <typename T>
        bool RegisterArg(const char* longName, const char* description = nullptr, ArgFlag flags = ArgFlag::None);

        bool UnregisterArg(const char* longName);
        bool UnregisterArg(char shortName);

        template <typename T>
        const T* FindArgValue(const char* longName) const;

        template <typename T>
        const T* FindArgValue(char shortName) const;

        template <typename T>
        const T& GetArgValue(const char* longName) const;

        template <typename T>
        const T& GetArgValue(char shortName) const;

    private:
        struct ValueEntry
        {
            ~ValueEntry() noexcept;
            TypeInfo valueType;
            void* valueMem{ nullptr };
            void (*valueDeleteFunc)(void*){ nullptr };
        };
        bool RegisterArg(ArgDesc&& desc, ValueEntry&& value);

    private:
        int m_argc{ 0 };
        char** m_argv{ nullptr };

        ArgResult m_args{};
        Vector<ArgDesc> m_descs{};
        Vector<ValueEntry> m_values{};
        HashMap<char, uint32_t> m_descsByShortName{};
        HashMap<const char*, uint32_t> m_descsByLongName{};

        EditorFlags m_flags{};
    };

    template <typename T>
    bool AppArgsService::RegisterArg(char shortArg, const char* longArg, const char* description, ArgFlag flags)
    {
        T* value = Allocator::GetDefault().New<T>();
        auto deleter = [](void* mem) { Allocator::GetDefault().Delete(static_cast<const T*>(mem)); };

        ArgDesc arg(*value, shortArg, longArg, description, flags);
        ValueEntry entry{ TypeInfo::Get<T>(), value, deleter };
        return RegisterArg(Move(arg), Move(entry));
    }

    template <typename T>
    bool AppArgsService::RegisterArg(const char* longArg, const char* description, ArgFlag flags)
    {
        T* value = Allocator::GetDefault().New<T>();
        auto deleter = [](void* mem) { Allocator::GetDefault().Delete(static_cast<const T*>(mem)); };

        ArgDesc arg(*value, longArg, description, flags);
        ValueEntry entry{ TypeInfo::Get<T>(), value, deleter };
        return RegisterArg(Move(arg), Move(entry));
    }

    template <typename T>
    const T* AppArgsService::FindArgValue(const char* longName) const
    {
        const uint32_t* index = m_descsByLongName.Find(longName);
        if (index)
        {
            HE_ASSERT(TypeInfo::Get<T>() == m_values[*index].valueType);
            return static_cast<const T*>(m_values[*index].valueMem);
        }
        return nullptr;
    }

    template <typename T>
    const T* AppArgsService::FindArgValue(char shortName) const
    {

        const uint32_t* index = m_descsByShortName.Find(shortName);
        if (index)
        {
            HE_ASSERT(TypeInfo::Get<T>() == m_values[*index].valueType);
            return static_cast<const T*>(m_values[*index].valueMem);
        }
        return nullptr;
    }

    template <typename T>
    const T& AppArgsService::GetArgValue(const char* longName) const
    {
        const T* value = FindArgValue<T>(longName);
        HE_ASSERT(value);
        return *value;
    }

    template <typename T>
    const T& AppArgsService::GetArgValue(char shortName) const
    {
        const T* value = FindArgValue<T>(shortName);
        HE_ASSERT(value);
        return *value;
    }
}
