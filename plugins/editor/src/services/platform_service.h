// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/string.h"
#include "he/core/vector.h"

namespace he::editor
{
    struct FileDialogFilter
    {
        /// Name of this filter.
        const char* name{ nullptr };

        /// Spec string for this filter. Most common is filtering by extension with something like "*.cpp".
        /// To allow any file you can use a spec like "*.*".
        const char* spec{ nullptr };
    };

    struct FileDialogConfig
    {
        /// The default path to open the dialog with.
        const char* defaultPath{ nullptr };

        /// When set the user can select multiple items. This is ignored for save dialogs.
        bool allowMultiSelect{ false };

        /// When set the user can only select folder, not files. This is ignored for save dialogs.
        bool folderSelect{ false };

        /// An array of filters to use in the dialog. The user can select from this list in the UI.
        const FileDialogFilter* filters{ nullptr };

        /// Number of filter elements in the `filter` array.
        uint32_t filterCount{ 0 };
    };

    class PlatformService
    {
    public:
        /// Opens a native file dialog for selecting files or folders to be opened.
        ///
        /// @param config Configuration for the dialog
        /// @param paths Vector of selected paths. If config.allowMultiSelect is false this will contain a single entry.
        /// @return Returns true if the user made a selection, or false if the user cancelled or there was an error.
        bool OpenFileDialog(const FileDialogConfig& config, Vector<String>& paths);

        /// Opens a native file dialog for selecting a file save location.
        ///
        /// @param config Configuration for the dialog
        /// @param path The path the user selected.
        /// @return Returns true if the user made a selection, or false if the user cancelled or there was an error.
        bool SaveFileDialog(const FileDialogConfig& config, String& path);

        /// Opens the file explorer to a particular directory, and optionally selects an item within it.
        ///
        /// @param directory The directory to open the file explorer to.
        /// @param selectItem Optional. A file or directory name to select that exists in `directory`.
        /// @return Returns true if the file explorer was opened, or false others.
        bool OpenInFileExplorer(const char* directory, const char* selectItem = nullptr);
    };
}
