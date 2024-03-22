// Copyright Chad Engler

#include "he/editor/services/platform_service.h"

#include "he/core/log.h"

#if defined(HE_PLATFORM_LINUX)

namespace he::editor
{
    bool PlatformService::OpenFilesDialog([[maybe_unused]] Vector<String>& paths, [[maybe_unused]] const FileDialogConfig& config)
    {
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }

    bool PlatformService::OpenFileDialog([[maybe_unused]] String& path, [[maybe_unused]] const FileDialogConfig& config)
    {
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }

    bool PlatformService::OpenFolderDialog([[maybe_unused]] String& path, [[maybe_unused]] const FileDialogConfig& config)
    {
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }

    bool PlatformService::SaveFileDialog([[maybe_unused]] String& path, [[maybe_unused]] const FileDialogConfig& config)
    {
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }

    bool PlatformService::OpenInFileExplorer([[maybe_unused]] const char* directory, [[maybe_unused]] const char* selectItem)
    {
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }
}

#endif
