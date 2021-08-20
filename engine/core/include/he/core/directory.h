// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/types.h"

// Since File is a class with some static (and non-static) member functions we use a namespace
// here to make the usage feel the same. That is, you can do File::Remove() and Directory::Remove()
namespace he::Directory
{
    /// A selection of special file system directories that are useful for applications to query.
    enum class SpecialId
    {
        LocalAppData,   ///< Windows: `%LocalAppData%`. Linux: `$XDG_DATA_HOME` > `$HOME/.local/share`
        SharedAppData,  ///< Windows: `%AppData%`. Linux: `$XDG_DATA_DIRS` > `/usr/local/share/` > `/usr/share/`
        Documents,      ///< Windows: `%UserProfile%/Documents`. Linux: `$XDG_DOCUMENTS_DIR` > `$HOME/Documents`
        Temp,           ///< Windows: System temp directory. Linux: `$TMPDIR` > `/tmp`
    };

    /// A directory entry. Can be a file or a subdirectory.
    struct Entry
    {
        const char* name;
        bool isDirectory;
    };

    /// Helper utility for recursively iterating through the contents of a directory.
    class Scanner
    {
    public:
        /// Construct a new scanner.
        ///
        /// \param[in] allocator The allocator to use.
        Scanner(Allocator& allocator);

        /// Destructs a scanner.
        ~Scanner();

        Scanner(const Scanner&) = delete;
        Scanner& operator=(const Scanner&) = delete;

        /// Begins a recursive directory walk from `path`.
        Result Open(const char* path);

        /// Closes the directory scanner.
        void Close();

        /// Gets the name of the next entry in the directory being scanned. Also optionally checks
        /// if the entry is a directory. This check can be much faster than getting the attributes
        /// of the file, if you only want to know if it is a directory.
        ///
        /// \param[out] name The string to write the entry name into.
        /// \param[out] isDirectory Optional. If non-null set to true if the entry is a directory
        ///     and false when it isn't.
        /// \return Returns true when an entry is found, or false if the scan has completed.
        bool NextEntry(String& outName, bool* outIsDirectory = nullptr);

    public:
        Allocator& m_allocator;
        void* m_impl;
    };

    /// Reads the path of a special OS directory from the environment and/or OS configuration.
    ///
    /// \param[in] dir The special directory to retreive.
    /// \return The path to the special directory.
    Result GetSpecial(String& dst, SpecialId dir);

    Result GetCurrent(String& dst);

    Result SetCurrent(const char* path);

    Result Rename(const char* oldPath, const char* newPath);

    bool Exists(const char* path);

    Result Create(const char* path, bool parents = false);

    Result Remove(const char* path);

    Result RemoveContents(const char* path);
}
