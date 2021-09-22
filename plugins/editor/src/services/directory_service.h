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
            AppDataRoot,
            Logs,
            Resources,
            Settings,

            _Count,
        };

    public:
        DirectoryService(Allocator& allocator);

        String GetAppDirectory(DirType type);

        bool CreateAll();

    private:
        String m_appDataRoot;
    };
}
