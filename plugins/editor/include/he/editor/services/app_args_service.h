// Copyright Chad Engler

#pragma once

#include "he/core/args.h"
#include "he/core/hash_table.h"
#include "he/core/types.h"
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

        bool RegisterArg(ArgDesc&& desc);
        const ArgDesc* FindArg(const char* longName) const;
        const ArgDesc* FindArg(char shortName) const;

    private:
        int m_argc{ 0 };
        char** m_argv{ nullptr };

        ArgResult m_args{};
        Vector<ArgDesc> m_descs{};
        HashMap<char, uint32_t> m_descsByShortName{};
        HashMap<const char*, uint32_t> m_descsByLongName{};

        EditorFlags m_flags{};
    };
}
