// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
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
    /// \note This `const char*` overload avoids an unnecessary strlen.
    ///
    /// \param[in] path The path to check.
    bool IsAbsolutePath(const char* path);

    /// Checks if the path is absolute. That is, is it relative to a filesystem root.
    /// For example, "/home/human/file" and "C:\\Users\\human\\file" are absolute paths.
    ///
    /// \param[in] path The path to check.
    bool IsAbsolutePath(StringView path);

    /// Finds and returns the extension of the path including the leading dot.
    /// For example, the extension of "/home/human/file.cpp" is ".cpp".
    ///
    /// \param[in] path The path to search.
    StringView GetExtension(StringView path);

    /// Finds and returns the directory of the path excluding the trailing slash.
    /// For example, the directory of "/home/human/file.cpp" is "/home/human".
    ///
    /// \param[in] path The path to search.
    StringView GetDirectory(StringView path);

    /// Finds and returns the final component of the path.
    /// For example, the base name of "/home/human/file.cpp" is "file.cpp".
    ///
    /// \param[in] path The path to search.
    StringView GetBaseName(StringView path);

    /// Gets a path without the extension.
    /// For example, the path without extension of "/home/human/file.cpp" is "/home/human/file".
    ///
    /// \param[in] path The path from which to get the path without the extension.
    StringView GetPathWithoutExtension(StringView path);

    // --------------------------------------------------------------------------------------------
    // Path Modifiers

    /// Normalizes the path by flattening directory separators, and resolving special directory
    /// specifiers (e.g. "../" & "./").
    /// For example, the normalized form of "/home/human/..///human//file.cpp" is "/home/human/file.cpp".
    ///
    /// \param[in,out] path The path to normalize.
    void NormalizePath(String& path);

    /// Concatenates the components onto the path, adding directory separators as necessary.
    /// For example, concatenating "/home/human" and "file.cpp" results in "/home/human/file.cpp".
    ///
    /// \param[in,out] path The path to concatenate to.
    /// \param[in] components The new path components to concatenate onto `root`.
    void ConcatPath(String& path, StringView components);

    /// Removes the extension from the end of the path.
    /// For example, removing the extension of "/home/human/file.cpp" results in "/home/human/file".
    ///
    /// \param[in,out] path The path to remove the extension of.
    void RemoveExtension(String& path);

    /// Makes a path into an absolute path. This will follow symlinks and resolve relative
    /// directories using the current working directory.
    ///
    /// \note An error will be returned if the file doesn't exist, or the process is unable
    /// to read it.
    ///
    /// \param[in,out] path The path to make into an absolute path.
    /// \return The result of the operation.
    Result MakeAbsolute(String& path);
}
