// Copyright Chad Engler

#include "platform_service.h"

#include "he/core/log.h"

#if defined(HE_PLATFORM_LINUX)

namespace he::editor
{
    bool PlatformService::OpenFilesDialog(Vector<String>& paths, const FileDialogConfig& config)
    {
        HE_UNUSED(paths, config);
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }

    bool PlatformService::OpenFileDialog(String& path, const FileDialogConfig& config)
    {
        HE_UNUSED(path, config);
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }

    bool PlatformService::OpenFolderDialog(String& path, const FileDialogConfig& config)
    {
        HE_UNUSED(path, config);
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }

    bool PlatformService::SaveFileDialog(String& path, const FileDialogConfig& config)
    {
        HE_UNUSED(path, config);
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }

    bool PlatformService::OpenInFileExplorer(const char* directory, const char* selectItem)
    {
        HE_UNUSED(directory, selectItem);
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented for linux!"));
        return false;
    }
}

#endif
