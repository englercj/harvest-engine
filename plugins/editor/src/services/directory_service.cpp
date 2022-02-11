// Copyright Chad Engler

#include "directory_service.h"

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
        "Harvest",      // AppDataRoot
        "logs",         // Logs
        "resources",    // Resources
        "settings",     // Settings
    };
    static_assert(HE_LENGTH_OF(DirectoryNames) == static_cast<uint32_t>(DirectoryService::DirType::_Count), "");

    String DirectoryService::GetAppDirectory(DirType type)
    {
        if (m_appDataRoot.IsEmpty())
        {
            Result r = Directory::GetSpecial(m_appDataRoot, SpecialDirectory::LocalAppData);
            if (!HE_VERIFY(r))
            {
                m_appDataRoot.Clear();
                HE_LOGF_ERROR(editor, "Failed to read local app data directory. Things may not work as expected. Error: {}", r);
                return m_appDataRoot;
            }

            const uint32_t appRootIndex = static_cast<uint32_t>(DirType::AppDataRoot);
            ConcatPath(m_appDataRoot, DirectoryNames[appRootIndex]);
        }

        if (type == DirType::AppDataRoot)
        {
            return m_appDataRoot;
        }

        const uint32_t typeIndex = static_cast<uint32_t>(type);
        String path = m_appDataRoot;
        ConcatPath(path, DirectoryNames[typeIndex]);
        return path;
    }

    bool DirectoryService::CreateAll()
    {
        bool result = true;

        String path(m_appDataRoot.GetAllocator());
        for (uint32_t i = 0; i < static_cast<uint32_t>(DirType::_Count); ++i)
        {
            path = GetAppDirectory(static_cast<DirType>(i));
            if (path.IsEmpty())
                continue;

            Result r = Directory::Create(path.Data(), true);
            if (!HE_VERIFY(r))
            {
                HE_LOGF_ERROR(editor, "Failed to create app data directory: '{}'. Things may not work as expected. Error: {}", path, r);
                result = false;
            }
        }

        return result;
    }
}
