// Copyright Chad Engler

/// Path utilities
///
/// There a couple differences between these path utilities and the C++ std::filesystem utilities.
/// Some choices and compromises were made here to unify the behavior across platforms:
/// 1. All these function work with both forward and backward slashes as directory separators.
/// 2. We consider forward slashes canonical, so normalized paths use forward slashes.
/// 3. Strange Windows paths (like UNC paths) are supported, on all platforms.
/// 4. Any path starting with a slash is considered absolute.
///     On windows `/anything` is not an absolute path, it is a "root-relative" path, however
///     in Harvest we consider root-relative paths as absolute.

#pragma once

#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace std { struct forward_iterator_tag; }

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

    /// Checks if the path is referring to an object that lives within parent.
    /// For example, "/home/human" is a child of "/home" but is not a child of "/home/dir"
    ///
    /// \note Both paths are expected to be absolute, and normalized.
    ///
    /// \param[in] path The path to check.
    /// \param[in] parent The path to check against.
    bool IsChildPath(StringView path, StringView parent);

    /// Finds and returns the extension of the path including the leading dot.
    /// For example, the extension of "/home/human/file.cpp" is ".cpp".
    ///
    /// \param[in] path The path to parse.
    StringView GetExtension(StringView path);

    /// Finds and returns the directory of the path excluding the trailing slash.
    /// For example, the directory of "/home/human/file.cpp" is "/home/human".
    ///
    /// \param[in] path The path to parse.
    StringView GetDirectory(StringView path);

    /// Finds and returns the final component of the path.
    /// For example, the base name of "/home/human/file.cpp" is "file.cpp".
    ///
    /// \param[in] path The path to parse.
    StringView GetBaseName(StringView path);

    /// Gets a path without the extension.
    /// For example, the path without extension of "/home/human/file.cpp" is "/home/human/file".
    ///
    /// \param[in] path The path to parse.
    StringView GetPathWithoutExtension(StringView path);

    /// Gets the root name of the absolute path.
    /// For example, the root name of "/home/human" is "" and the root name of "C:/Windows" is "C:"
    ///
    /// \param[in] path The path to parse.
    StringView GetRootName(StringView path);

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

    /// Makes `path` relative to `parent`.
    ///
    /// \param[in,out] path The path absolute path to made relative.
    /// \param[in] base The absolute base path to be relative to.
    /// \return True if the path was made relative, or false if it couldn't be.
    bool MakeRelative(String& path, StringView base);

    /// Makes a path into an absolute path. This will follow symlinks and resolve relative
    /// directories using the current working directory.
    ///
    /// \note An error will be returned if the file doesn't exist, or the process is unable
    /// to read it.
    ///
    /// \param[in,out] path The path to make into an absolute path.
    /// \return The result of the operation.
    Result MakeAbsolute(String& path);

    // --------------------------------------------------------------------------------------------
    // Path Iteration

    /// An iterator used by \ref PathSplitter to iterate elements of a path.
    class PathIterator
    {
    public:
        using ElementType = StringView;

        using difference_type   = uint32_t;
        using value_type        = StringView;
        using pointer           = const value_type*;
        using reference         = const value_type&;
        using iterator_category = std::forward_iterator_tag;
        using _Unchecked_type   = PathIterator; // Mark iterator as checked.

    public:
        PathIterator() = default;
        explicit PathIterator(StringView path);
        PathIterator(StringView path, StringView element) : m_path(path), m_element(element) {}

        const StringView& operator*() const { return m_element; }
        const StringView* operator->() const { return &m_element; }

        PathIterator& operator++() { GoToNext(); return *this; }
        PathIterator operator++(int) { PathIterator x = *this; GoToNext(); return x; }

        PathIterator& operator+=(uint32_t num) { while (num) { GoToNext(); --num; } return *this; }
        PathIterator operator+(uint32_t num) const { PathIterator x = *this; return (x += num); }

        bool operator==(const PathIterator& x) const;
        bool operator!=(const PathIterator& x) const { return !operator==(x); }
        bool operator<(const PathIterator& x) const { return m_element.Data() < x.m_element.Data(); }

    private:
        void GoToNext();

    private:
        StringView m_path{};
        StringView m_element{};
    };

    /// Allows iteration of a path as if it were split along directory separators.
    class PathSplitter
    {
    public:
        explicit PathSplitter(StringView path) : m_path(path) {}

        PathIterator begin() const { return PathIterator(m_path); }
        PathIterator end() const { return PathIterator(m_path, { m_path.End(), m_path.End() }); }

    private:
        const StringView m_path;
    };
}
