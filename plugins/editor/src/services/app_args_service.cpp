// Copyright Chad Engler

#include "he/editor/services/app_args_service.h"

#include "he/core/log.h"
#include "he/core/string_ops.h"
#include "he/core/utils.h"

namespace he::editor
{
    AppArgsService::ValueEntry::ValueEntry(ValueEntry&& x) noexcept
        : valueType(Exchange(x.valueType, {}))
        , valueMem(Exchange(x.valueMem, nullptr))
        , valueDeleter(Exchange(x.valueDeleter, nullptr))
    {}

    AppArgsService::ValueEntry::ValueEntry(const TypeInfo& type, void* mem, Pfn_Deleter deleter) noexcept
        : valueType(type)
        , valueMem(mem)
        , valueDeleter(deleter)
    {}

    AppArgsService::ValueEntry::~ValueEntry() noexcept
    {
        if (valueDeleter)
        {
            valueDeleter(valueMem);
        }
    }

    AppArgsService::AppArgsService() noexcept
    {
        RegisterArg<bool>('h', "help", "Prints this help message");
    }

    String AppArgsService::Help() const
    {
        return MakeHelpString(m_descs, m_argv[0], &m_args);
    }

    bool AppArgsService::Initialize(int argc, char** argv)
    {
        m_argc = argc;
        m_argv = argv;
        m_args = ParseArgs(m_descs, m_argc, m_argv);
        return m_args.code == ArgResult::Success;
    }

    bool AppArgsService::UnregisterArg(const char* longName)
    {
        // Technically this leaks the descs and values since they stay in the vector.
        // However, since register/unregister pairs should primarily be at module load/unload
        // boundaries - which are called very rarely - this is probably fine. We'll still
        // cleanup the memory when we destruct.
        // If this becomes a problem, we can make a new strategy.
        return m_descsByLongName.Erase(longName);
    }

    bool AppArgsService::UnregisterArg(char shortName)
    {
        // Technically this leaks the descs and values since they stay in the vector.
        // However, since register/unregister pairs should primarily be at module load/unload
        // boundaries - which are called very rarely - this is probably fine. We'll still
        // cleanup the memory when we destruct.
        // If this becomes a problem, we can make a new strategy.
        return m_descsByShortName.Erase(shortName);
    }

    bool AppArgsService::RegisterArg(ArgDesc&& desc, ValueEntry&& value)
    {
        HE_ASSERT(m_descs.Size() == m_values.Size());

        const uint32_t index = m_descs.Size();

        auto resultLong = m_descsByLongName.Emplace(desc.LongName(), index);
        if (!resultLong.inserted)
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("An argument with that long name is already registered."),
                HE_KV(short_name, desc.ShortName()),
                HE_KV(long_name, desc.LongName()));
            return false;
        }

        if (desc.ShortName() != '\0')
        {
            auto resultShort = m_descsByShortName.Emplace(desc.ShortName(), index);
            if (!resultShort.inserted)
            {
                m_descsByLongName.Erase(desc.LongName());
                HE_LOG_ERROR(he_editor,
                    HE_MSG("An argument with that short name is already registered."),
                    HE_KV(short_name, desc.ShortName()),
                    HE_KV(long_name, desc.LongName()));
                return false;

            }
        }

        m_descs.PushBack(Move(desc));
        m_values.PushBack(Move(value));
        return true;
    }
}
