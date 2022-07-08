// Copyright Chad Engler

#pragma once

#include "he/core/string.h"

namespace he::editor
{
    class DirectoryService
    {
    public:
        enum class DirType
        {
            Root,
            Data,
            Logs,
            Projects,

            _Count,
        };

    public:
        String GetAppDirectory(DirType type);

        bool CreateAll();

    private:
        String m_root{};
    };
}
