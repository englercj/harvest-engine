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

        /// The file is being opened with no system caching for data reads and writes. When this
        /// flag is set there are a number of restrictions that must be followed:
        /// - Read/write offsets much be aligned to 4096 bytes
        /// - Read/write sizes much be aligned to 4096 bytes
        /// - Read/write buffers must have an address that is aligned to 4096 bytes
        NoBuffering     = 1 << 1,

        /// Access is intended to be random. The system may use this as a hint to optimize file
        /// caching. Mutually exclusive with `SequentialScan`.
        RandomAccess    = 1 << 2,

        /// Access is intended to be sequential from beginning to end. The system may use this as
        /// a hint to optimize file caching. Mutually exclusive with `RandomAccess`.
        SequentialScan  = 1 << 3,
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

    /// Flags for behavior of locking an open file.
    enum class FileLockFlag : uint32_t
    {
        None            = 0,
        Exclusive       = 1 << 0, ///< Lock the file for exclusive access by this process.
        NonBlocking     = 1 << 1, ///< Do not block until the lock is aquired.
    };
    HE_ENUM_FLAGS(FileLockFlag);

    /// Flags for behavior of a memory mapped file once opened.
    enum class MemoryMapMode : uint8_t
    {
        Read,   ///< Map the file into read-only memory.
        Write,  ///< Map the file into read-write memory.
    };

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

    /// Represents a file on disk that can be read from or written to.
    class File
    {
    public:
        /// Checks if a file exists at `path`.
        ///
        /// \param[in] path The filesystem path to check.
        /// \return True if the path exists and is a regular file, false otherwise.
        static bool Exists(const char* path);

        /// Removes the file at `path`.
        ///
        /// \param[in] path The filesystem path to remove.
        /// \return The result of the remove operation.
        static Result Remove(const char* path);

        // Renames a file, moving it from `oldPath` to `newPath`.
        static Result Rename(const char* oldPath, const char* newPath);

        // Copies a file from the `oldPath` to `newPath`. This performs a deep copy of the data.
        static Result Copy(const char* oldPath, const char* newPath, bool clobber = true);

        // Fills `outAttributes` with the attributes of the file at `path`. If this returns a
        // non-successful result then `outAttributes` will not contain valid values.
        static Result GetAttributes(const char* path, FileAttributes& outAttributes);

        /// Read a file's contents into a vector buffer. The vector will be resized to hold the
        /// read data.
        ///
        /// \param[out] dst The destination vector to write data into.
        /// \param[in] path The path to the file to read. The file is expected to exist.
        /// \param[out] bytesRead Optional. The resulting number of bytes read.
        template <typename T>
        static Result ReadAll(Vector<T>& dst, const char* path,  uint32_t* bytesRead = nullptr);

        /// Write the contents of a buffer to a file.
        ///
        /// \param[in] src The buffer to write to the file.
        /// \param[in] size The number of bytes to write to the file.
        /// \param[in] path The path of the write to write to. Existing files will be overwritten.
        /// \param[out] bytesWritten Optional. The resulting number of bytes written.
        static Result WriteAll(const void* src, uint32_t size, const char* path, uint32_t* bytesWritten = nullptr);

    public:
        File();
        File(const File&) = delete;
        File(File&& x);
        ~File();

        File& operator=(const File&) = delete;
        File& operator=(File&& x);

        /// Opens the file at `path` in the given `mode` using behavior defined by `flags`.
        ///
        /// \param[in] path The filesystem path to open.
        /// \param[in] mode The mode to open the file in.
        /// \param[in] flags The behavior flags for operations on this file.
        /// \return The result of the operation.
        Result Open(const char* path, FileOpenMode mode, FileOpenFlag flags = FileOpenFlag::None);

        /// Closes the file.
        void Close();

        /// Checks if the file is currently open.
        ///
        /// \return True if the file is open, false otherwise.
        bool IsOpen() const;

        /// Reads the size of the file.
        ///
        /// \return The size of the file in bytes.
        uint64_t GetSize() const;

        /// Set the size of the file to `size`.
        ///
        /// \param[in] size The size in bytes to set the file to.
        /// \return The result of the operation.
        Result SetSize(uint64_t size);

        /// Reads the position of the file pointer.
        ///
        /// \note After using \ref ReadAt or \ref WriteAt the position of the file pointer is
        /// undefined. You'll need to reset it with \ref SetPos for this function to return
        /// valid result again.
        ///
        /// \return The current position of the file pointer.
        uint64_t GetPos() const;

        /// Sets the file pointer to the `offset` position.
        ///
        /// \param[in] offset The offset in the file to move the pointer to.
        /// \return The result of the operation.
        Result SetPos(uint64_t offset);

        // Reads `size` bytes from the file into `dst` and stores the count of `bytesRead`.
        Result Read(void* dst, uint32_t size, uint32_t* bytesRead = nullptr);

        // Reads `size` bytes from the file at `offset` into `dst` and stores the count of
        // `bytesRead`.
        // Note: The position of the file handle after calling this function is undefined.
        Result ReadAt(void* dst, uint64_t offset, uint32_t size, uint32_t* bytesRead = nullptr);

        // Writes the `size` bytes from `src` to the given file and stores the `bytesWritten`.
        Result Write(const void* src, uint32_t size, uint32_t* bytesWritten = nullptr);

        // Writes `size` bytes from `src` at `offset` and stores the count of `bytesWritten`.
        // Note: The position of the file handle after calling this function is undefined.
        Result WriteAt(const void* src, uint64_t offset, uint32_t size, uint32_t* bytesWritten = nullptr);

        // Flushes the buffers from the given file.
        Result Flush();

        // Locks the file for exclusive or shared access by the calling process.
        Result Lock(uint64_t offset, uint64_t size, FileLockFlag flags = FileLockFlag::None);

        // Unlocks the file access held by this process.
        Result Unlock(uint64_t offset, uint64_t size);

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

    /// Represents a memory-mapped file.
    class MemoryMap
    {
    public:
        MemoryMap();
        MemoryMap(const MemoryMap&) = delete;
        MemoryMap(MemoryMap&& x);
        ~MemoryMap();

        MemoryMap& operator=(const MemoryMap&) = delete;
        MemoryMap& operator=(MemoryMap&& x);

        // Memory maps a file.
        Result Map(File& file, MemoryMapMode mode, uint64_t offset = 0, uint32_t size = 0);

        /// Checks if a file is currently memory mapped.
        ///
        /// \return True if a file is memory mapped, false otherwise.
        bool IsMapped() const;

        // Unmaps the file from memory.
        void Unmap();

        // Flushes the memory mapped data to the underlying file.
        Result Flush(uint64_t offset, uint32_t size, bool async = false);

        // Flushes an entire memory mapped region of a file to the underlying file.
        Result Flush(bool async = false) { return Flush(0, m_size, async); }

    public:
        void* m_data;
        uint32_t m_size;

    #if defined(HE_PLATFORM_API_WIN32)
        void* m_handle;
        void* m_fileHandle;
    #endif
    };

    template <typename T>
    inline Result File::ReadAll(Vector<T>& dst, const char* path, uint32_t* outBytesRead)
    {
        File f;
        Result r = f.Open(path, FileOpenMode::ReadExisting);
        if (!r)
            return r;

        const uint32_t size = static_cast<uint32_t>(f.GetSize());
        const uint32_t vectorSize = sizeof(T) == 1 ? size : (size + (sizeof(T) - 1)) / sizeof(T);
        dst.Resize(vectorSize, he::DefaultInit);

        uint32_t bytesRead = 0;
        r = f.ReadAt(dst.Data(), 0, size, &bytesRead);

        if (outBytesRead)
            *outBytesRead = bytesRead;

        if (!r)
            return r;

        if (bytesRead < size)
        {
            const uint32_t vectorReadSize = sizeof(T) == 1 ? bytesRead : (bytesRead + (sizeof(T) - 1)) / sizeof(T);
            dst.Resize(vectorReadSize);
        }

        return Result::Success;
    }
}
