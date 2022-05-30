// Copyright Chad Engler

#include "platform_service.h"

#include "he/core/log.h"

#if defined(HE_PLATFORM_LINUX)

namespace he::editor
{
    bool PlatformService::OpenFileDialog(const FileDialogConfig& config, Vector<String>& paths)
    {
        HE_UNUSED(config, paths);
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented on this platform!"));
        return false;
    }

    bool PlatformService::SaveFileDialog(const FileDialogConfig& config, String& path)
    {
        HE_UNUSED(config, path);
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented on this platform!"));
        return false;
    }

    bool PlatformService::OpenInFileExplorer(const char* directory, const char* selectItem)
    {
        HE_UNUSED(directory, selectItem);
        HE_LOG_WARN(editor, HE_MSG("File dialogs are not implemented on this platform!"));
        return false;
    }
}

#endif
