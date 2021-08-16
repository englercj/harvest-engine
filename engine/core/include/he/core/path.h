// Copyright Chad Engler

#pragma once

#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Path Queries

    /// Checks if the path is absolute. That is, is it relative to a filesystem root.
    /// For example, "/home/human/file" and "C:\\Users\\human\\file" are absolute paths.
    ///
    /// \param[in] path The path to check.
    bool IsAbsolutePath(const StringView& path);

    /// Finds and returns the extension of the path including the leading dot.
    /// For example, the extension of "/home/human/file.cpp" is ".cpp".
    ///
    /// \param[in] path The path to search.
    StringView GetExtension(const StringView& path);

    /// Finds and returns the directory name of the path excluding the trailing slash.
    /// For example, the dir name of "/home/human/file.cpp" is "/home/human".
    ///
    /// \param[in] path The path to search.
    StringView GetDirName(const StringView& path);

    /// Finds and returns the final component of the path.
    /// For example, the base name of "/home/human/file.cpp" is "file.cpp".
    ///
    /// \param[in] path The path to search.
    StringView GetBaseName(const StringView& path);

    // --------------------------------------------------------------------------------------------
    // Path Modifiers

    /// Normalizes the path by flattening directory separators, and resolving special directory
    /// specifiers (e.g. "../" & "./").
    /// For example, the normalized form of "/home/human/..///human//file.cpp" is "/home/human/file.cpp".
    ///
    /// \param[in] path The path to normalize.
    void NormalizePath(String& path);

    /// Concatenates the components onto the path, adding directory separators as necessary.
    /// For example, concatenating "/home/human" and "file.cpp" results in "/home/human/file.cpp".
    ///
    /// \param[in] path The path to concatenate to.
    /// \param[in] components The new path components to concatenate onto `root`.
    void ConcatPath(String& path, const StringView& components);

    /// Removes the extension from the end of the path.
    /// For example, removing the extension of "/home/human/file.cpp" results in "/home/human/file".
    ///
    /// \param[in] path The path to remove the extension of.
    // Remove the extension from `path`. Returns the new size of the path.
    void RemoveExtension(String& path);
}
