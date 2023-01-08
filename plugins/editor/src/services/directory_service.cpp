// Copyright Chad Engler

#include "he/editor/services/directory_service.h"

#include "he/core/directory.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"

namespace he::editor
{
    constexpr const char* DirectoryNames[] =
    {
        "Harvest",      // Root
        "data",         // Data
        "logs",         // Logs
        "projects",     // Projects
    };
    static_assert(HE_LENGTH_OF(DirectoryNames) == static_cast<uint32_t>(DirectoryService::DirType::_Count), "");

    String DirectoryService::GetAppDirectory(DirType type)
    {
        if (m_root.IsEmpty())
        {
            Result r = Directory::GetSpecial(m_root, SpecialDirectory::LocalAppData);
            if (!HE_VERIFY(r))
            {
                m_root.Clear();
                HE_LOG_ERROR(editor,
                    HE_MSG("Failed to read local app data directory. Things may not work as expected."),
                    HE_KV(result, r));
                return m_root;
            }

            const uint32_t appRootIndex = static_cast<uint32_t>(DirType::Root);
            ConcatPath(m_root, DirectoryNames[appRootIndex]);
        }

        if (type == DirType::Root)
            return m_root;

        const uint32_t typeIndex = static_cast<uint32_t>(type);
        String path = m_root;
        ConcatPath(path, DirectoryNames[typeIndex]);
        return path;
    }

    bool DirectoryService::CreateAll()
    {
        bool result = true;

        String path;
        for (uint32_t i = 0; i < static_cast<uint32_t>(DirType::_Count); ++i)
        {
            path = GetAppDirectory(static_cast<DirType>(i));
            if (path.IsEmpty())
                continue;

            Result r = Directory::Create(path.Data(), true);
            if (!HE_VERIFY(r))
            {
                HE_LOG_ERROR(editor,
                    HE_MSG("Failed to create app data directory. Things may not work as expected."),
                    HE_KV(result, r),
                    HE_KV(path, path));
                result = false;
            }
        }

        return result;
    }
}
