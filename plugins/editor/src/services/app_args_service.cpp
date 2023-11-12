// Copyright Chad Engler

#include "he/editor/services/app_args_service.h"

#include "he/core/log.h"
#include "he/core/string_ops.h"

namespace he::editor
{
    AppArgsService::AppArgsService() noexcept
    {
        m_descs.EmplaceBack(m_flags.help, 'h', "help", "Prints this help message");
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

    bool AppArgsService::RegisterArg(ArgDesc&& desc)
    {
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

        m_descs.PushBack(Move(desc));
        return true;
    }

    const ArgDesc* AppArgsService::FindArg(const char* longName) const
    {
        const uint32_t* index = m_descsByLongName.Find(longName);
        return index ? &m_descs[*index] : nullptr;
    }

    const ArgDesc* AppArgsService::FindArg(char shortName) const
    {
        const uint32_t* index = m_descsByLongName.Find(shortName);
        return index ? &m_descs[*index] : nullptr;
    }
}
