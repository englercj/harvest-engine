// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    /// A selection of special file system directories that are useful for applications to query.
    enum class SpecialDirectory
    {
        Documents,      ///< Windows: `%UserProfile%/Documents`. Linux: `$XDG_DOCUMENTS_DIR` > `$HOME/Documents`
        LocalAppData,   ///< Windows: `%LocalAppData%`. Linux: `$XDG_DATA_HOME` > `$HOME/.local/share`
        SharedAppData,  ///< Windows: `%AppData%`. Linux: `$XDG_DATA_DIRS` > `/usr/local/share/` > `/usr/share/`
        Temp,           ///< Windows: System temp directory. Linux: `$TMPDIR` > `/tmp`
    };

    /// Helper utility for recursively iterating through the contents of a directory.
    class DirectoryScanner final
    {
    public:
        /// Structure representing an entry in a recursive directory scan.
        struct Entry
        {
            Entry(Allocator& allocator = Allocator::GetDefault()) : name(allocator) {}

            /// The name of the entry.
            String name;

            /// True if the entry is another directory.
            bool isDirectory{ false };
        };

    public:
        /// Construct a new scanner.
        ///
        /// \param[in] allocator The allocator to use.
        DirectoryScanner(Allocator& allocator = Allocator::GetDefault()) noexcept
            : m_allocator(allocator)
            , m_impl(nullptr)
        {}

        /// Constructs a new scanner by moving the other scanner into this one.
        ///
        /// \param[in] x The scanner to move from.
        DirectoryScanner(DirectoryScanner&& x) noexcept
            : m_allocator(x.m_allocator)
            , m_impl(Exchange(x.m_impl, nullptr))
        {}

        /// Destructs a scanner.
        ~DirectoryScanner() noexcept { Close(); }

        DirectoryScanner(const DirectoryScanner&) = delete;
        DirectoryScanner& operator=(const DirectoryScanner&) = delete;
        DirectoryScanner& operator=(DirectoryScanner&&) = delete;

        /// Begins a recursive directory walk from `path`.
        ///
        /// \param[in] path The path to a directory to walk.
        Result Open(const char* path);

        /// Closes the directory scanner.
        void Close();

        /// Gets the next entry in the directory being scanned. The values in `outEntry` are only
        /// valid to read if this function returns true. This function will never emit entries
        /// for the '.' and '..' entries.
        ///
        /// \param[out] outEntry The entry structure to fill with the next entry's information.
        /// \return Returns true when an entry is found, or false if the scan has completed.
        bool NextEntry(Entry& outEntry);

    public:
        Allocator& m_allocator;
        void* m_impl;
    };

    class Directory
    {
    public:
        /// Gets the path of a special OS directory from the environment and/or OS configuration.
        ///
        /// \param[out] dst The string to write the directory path to.
        /// \param[in] dir The special directory to retreive.
        /// \return The result of trying to read the special directory's path.
        static Result GetSpecial(String& dst, SpecialDirectory dir);

        /// Gets the current working directory of the process.
        ///
        /// \note Accessing the CWD is not thread-safe.
        ///
        /// \param[out] dst The string to write the directory path to.
        /// \return The result of trying to read the current working directory's path.
        static Result GetCurrent(String& dst);

        /// Sets the current working directory of the process.
        ///
        /// \note Accessing the CWD is not thread-safe.
        ///
        /// \param[in] path The path to set the CWD to.
        /// \return The result of trying to write the current working directory's path.
        static Result SetCurrent(const char* path);

        /// Renames a directory from `path` to `newPath`.
        ///
        /// \param[in] path The current path of the directory to rename.
        /// \param[in] newPath The path to rename the directory to.
        /// \return The result of the rename operation.
        static Result Rename(const char* path, const char* newPath);

        /// Returns true if a directory exists.
        ///
        /// \note In general you should prefer to attempt to use a directory and handle any errors
        /// rather than checking for existence.
        ///
        /// \param[in] path The path to a directory to check for existence.
        /// \return True if the path exists and is a directory, false otherwise.
        static bool Exists(const char* path);

        /// Creates a directory at `path`, optionally creating any necessary parent directories
        /// if `parents` is set. If the directory already exists, \ref Result::Success is returned.
        ///
        /// \param[in] path The path to create the directory at.
        /// \param[in] parents Optional. If true will also create any missing parent directories.
        /// \return The result of the create operation.
        static Result Create(const char* path, bool parents = false);

        /// Removes a directory at `path`.
        ///
        /// \param[in] path The path of the directory to remove.
        /// \return The result of the remove operation.
        static Result Remove(const char* path);

        /// Removes all contents of a directory recursively. The directory itself is not deleted.
        /// If any error is encountered the operation stops early and returns that error.
        ///
        /// \param[in] path The path to a directory to remove all contents within.
        /// \return The result of the remote operation.
        static Result RemoveContents(const char* path, Allocator& allocator = Allocator::GetDefault());
    };
}
