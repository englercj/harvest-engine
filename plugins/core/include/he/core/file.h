// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/enum_ops.h"
#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/vector.h"

namespace he
{
    /// Normalization of OS results for specific file system results.
    enum class FileResult : uint32_t
    {
        Success = 0,
        Failure,

        AccessDenied,
        AlreadyExists,
        DiskFull,
        NotFound,
        NoData,
    };

    /// Returns the enum as a string.
    ///
    /// \param[in] x The value to get the string representation of.
    /// \return The string representation of the enum value.
    const char* AsString(FileResult x);

    /// Possible modes a file can be opened in.
    enum class FileOpenMode : uint32_t
    {
        Write,
        ReadWrite,
        ReadExisting,
        ReadWriteExisting,
        WriteTruncate,
        ReadWriteTruncate,
        WriteAppend,
        ReadWriteAppend
    };

    /// Flags for behavior of a file once opened.
    enum class FileOpenFlag : uint32_t
    {
        None            = 0,
        NoBuffering     = 1 << 1, ///< The file or device is being opened with no system caching for data reads and writes.
        RandomAccess    = 1 << 2, ///< Access is intended to be random. The system can use this as a hint to optimize file caching. Mutually exclusive with `SequentialScan`.
        SequentialScan  = 1 << 3, ///< Access is intended to be sequential from beginning to end. The system can use this as a hint to optimize file caching. Mutually exclusive with `RandomAccess`.
    };
    HE_ENUM_FLAGS(FileOpenFlag);

    /// The attribute flags for a file.
    enum class FileAttributeFlag : uint32_t
    {
        None            = 0,
        Directory       = 1 << 0, ///< This path refers to a directory.
        Hidden          = 1 << 1, ///< This path refers to a hidden file or directory.
        ReadOnly        = 1 << 2, ///< This path refers to a read-only file or directory.
    };
    HE_ENUM_FLAGS(FileAttributeFlag);

    /// Structure that represents the attributes of a file.
    struct FileAttributes
    {
        /// The attribute flags for this file.
        FileAttributeFlag flags;

        /// The size of the file; always `0` for directories.
        uint64_t size;

        /// The time at which the file was created. Only supported on Windows and is set to `0` on
        /// all other platforms. This value is based on system time, so the user's configuration can
        /// cause unexpected results. The timing resolution is file system-dependent.
        SystemTime createTime;

        /// The last time this file was accessed (read or written). Generally unreliable on most of
        /// the platforms and file systems  supports. May mirror `writeTime`, be empty, or any
        /// other value the OS decides depending on the user's configuration. Use with caution.
        SystemTime accessTime;

        /// The last time this file was written to. This value is based on system time and therefore
        /// can yield unexpected results due to the user's configuration.
        /// The timing resolution is file system-dependent.
        SystemTime writeTime;
    };

    /// Translates a result structure into a file result.
    ///
    /// \param[in] result The result object to translate.
    /// \return The file result representing the OS result.
    FileResult GetFileResult(Result result);

    /// Class that is used to represent a file on disk.
    class File
    {
    public:
        /// Checks if a file exists at `path`.
        ///
        /// \param[in] path The filesystem path to check.
        /// \return True if the path exists and is a regular file, false otherwise.
        static bool Exists(const char* path);

        // Removes the file at `path`.
        static Result Remove(const char* path);

        // Renames a file, moving it from `oldPath` to `newPath`.
        static Result Rename(const char* oldPath, const char* newPath);

        // Copies a file from the `oldPath` to `newPath`. This performs a deep copy of the data.
        static Result Copy(const char* oldPath, const char* newPath, bool clobber = true);

        // Fills `outAttributes` with the attributes of the file at `path`. If this returns a
        // non-successful result then `outAttributes` will not contain valid values.
        static Result GetAttributes(const char* path, FileAttributes& outAttributes);

    public:
        File();
        File(const File&) = delete;
        File(File&& x);
        ~File();

        File& operator=(const File&) = delete;
        File& operator=(File&& x);

        // Opens the file from the `path` in the given `mode`.
        Result Open(const char* path, FileOpenMode mode, FileOpenFlag flags = FileOpenFlag::None);

        // Closes the file.
        void Close();

        // Returns true if the file is currently open.
        bool IsOpen() const;

        // Returns the size of the file.
        uint64_t GetSize() const;

        // Set the size of the file to the given `size`.
        Result SetSize(uint64_t size);

        // Returns the position of the file pointer in the specified file.
        uint64_t GetPos() const;

        // Sets the file pointer to the `offset` position in the specified file.
        Result SetPos(uint64_t offset);

        // Reads `bytesToRead` bytes from the file into `dst` and stores the count of `bytesRead`.
        Result Read(void* dst, uint32_t bytesToRead, uint32_t* bytesRead = nullptr);

        // Reads `bytesToRead` bytes from the file at `offset` into `dst` and stores the count of
        // `bytesRead`.
        // Note: The position of the file handle after calling this function is undefined.
        Result ReadAt(void* dst, uint32_t bytesToRead, uint64_t offset, uint32_t* bytesRead = nullptr);

        // Writes the `bytesToWrite` bytes from `src` to the given file and stores the `bytesWritten`.
        Result Write(const void* src, uint32_t bytesToWrite, uint32_t* bytesWritten = nullptr);

        // Writes `bytesToWrite` bytes from `src` at `offset` and stores the count of `bytesWritten`.
        // Note: The position of the file handle after calling this function is undefined.
        Result WriteAt(const void* src, uint32_t bytesToWrite, uint64_t offset, uint32_t* bytesWritten = nullptr);

        // Flushes the buffers from the given file.
        Result Flush();

        // Fills `attributes` with the attributes of the file at `path`. If this returns a
        // non-successful result then `attributes` will not contain valid values.
        Result GetAttributes(FileAttributes& attributes) const;

        // Returns the full path to file
        Result GetPath(String& path) const;

        // Sets the file's creation time, access time, and last written time, according to the
        // values passed in `createTime`, `accessTime`, and `writeTime`, respectively. If any
        // value is `nullptr`, the corresponding file time attribute remains unchanged.
        Result SetTimes(const SystemTime* accessTime, const SystemTime* writeTime);

    public:
        intptr_t m_fd;
    };
}
