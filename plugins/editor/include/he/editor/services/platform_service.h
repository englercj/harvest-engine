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

        /// An array of filters to use in the dialog. The user can select from this list in the UI.
        const FileDialogFilter* filters{ nullptr };

        /// Number of filter elements in the `filter` array.
        uint32_t filterCount{ 0 };
    };

    class PlatformService
    {
    public:
        /// Opens a native file dialog for selecting multiple files to be opened.
        ///
        /// @param config Configuration for the dialog
        /// @param paths Vector of selected paths.
        /// @return Returns true if the user made a selection, or false if the user cancelled or there was an error.
        bool OpenFilesDialog(Vector<String>& paths, const FileDialogConfig& config = {});

        /// Opens a native file dialog for selecting a file to be opened.
        ///
        /// @param config Configuration for the dialog
        /// @param path The path the user selected.
        /// @return Returns true if the user made a selection, or false if the user cancelled or there was an error.
        bool OpenFileDialog(String& paths, const FileDialogConfig& config = {});

        /// Opens a native file dialog for selecting a folder.
        ///
        /// @param config Configuration for the dialog
        /// @param path The path the user selected.
        /// @return Returns true if the user made a selection, or false if the user cancelled or there was an error.
        bool OpenFolderDialog(String& path, const FileDialogConfig& config = {});

        /// Opens a native file dialog for selecting a file save location.
        ///
        /// @param config Configuration for the dialog
        /// @param path The path the user selected.
        /// @return Returns true if the user made a selection, or false if the user cancelled or there was an error.
        bool SaveFileDialog(String& path, const FileDialogConfig& config = {});

        /// Opens the file explorer to a particular directory, and optionally selects an item within it.
        ///
        /// @param directory The directory to open the file explorer to.
        /// @param selectItem Optional. A file or directory name to select that exists in `directory`.
        /// @return Returns true if the file explorer was opened, or false others.
        bool OpenInFileExplorer(const char* directory, const char* selectItem = nullptr);
    };
}
