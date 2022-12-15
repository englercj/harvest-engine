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
    enum class FileResult : uint8_t
    {
        Success,
        Failure,

        AccessDenied,
        AlreadyExists,
        DiskFull,
        NotFound,
        NoData,
    };

    /// Possible modes a file can be opened in.
    enum class FileOpenMode : uint8_t
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
        /// No special behavior.
        None            = 0,

        /// The file is being opened with no system caching for data reads and writes. When this
        /// flag is set there are a number of restrictions that must be followed:
        /// - Read/write offsets must be aligned to 4096 bytes
        /// - Read/write sizes must be aligned to 4096 bytes
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
        NonBlocking     = 1 << 1, ///< Do not block until the lock is acquired.
    };
    HE_ENUM_FLAGS(FileLockFlag);

    /// Flags for behavior of a memory mapped file once opened.
    enum class MemoryMapMode : uint8_t
    {
        Read,       ///< Map the file into read-only memory.
        ReadWrite,  ///< Map the file into read-write memory.
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

        /// Renames a file, moving it from `oldPath` to `newPath`.
        ///
        /// \param[in] oldPath The path to the file to be moved.
        /// \param[in] newPath The path to the location for the file to be moved to.
        /// \return The result of the rename operation.
        static Result Rename(const char* oldPath, const char* newPath);

        /// Copies a file from the `oldPath` to `newPath`. This performs a deep copy of the data.
        ///
        /// \param[in] oldPath The path to the file to be copied.
        /// \param[in] newPath The path to the location for the file to be copied to.
        /// \param[in] clobber Optional. Overwrite the destination file if it exists. Default is true.
        /// \return The result of the copy operation.
        static Result Copy(const char* oldPath, const char* newPath, bool clobber = true);

        /// Fills `outAttributes` with the attributes of the file at `path`. If this returns a
        /// non-successful result then `outAttributes` will not contain valid values.
        ///
        /// \param[in] path The path to the file to get the attribute of.
        /// \param[out] outAttributes The attributes structure to fill with data.
        /// \return The result of the operation.
        static Result GetAttributes(const char* path, FileAttributes& outAttributes);

        /// Read a file's contents into a container buffer. The container will be resized to
        /// hold the read data.
        ///
        /// \param[out] dst The destination vector to write data into.
        /// \param[in] path The path to the file to read. The file is expected to exist.
        /// \param[out] bytesRead Optional. The resulting number of bytes read.
        /// \return The result of the operation.
        template <typename T>
        static Result ReadAll(T& dst, const char* path,  uint32_t* bytesRead = nullptr);

        /// Write the contents of a buffer to a file.
        ///
        /// \param[in] src The buffer to write to the file.
        /// \param[in] size The number of bytes to write to the file.
        /// \param[in] path The path of the write to write to. Existing files will be overwritten.
        /// \param[out] bytesWritten Optional. The resulting number of bytes written.
        /// \return The result of the operation.
        static Result WriteAll(const void* src, uint32_t size, const char* path, uint32_t* bytesWritten = nullptr);

    public:
        /// Constructs a file.
        File() noexcept;

        /// Moves a file object (not the file on disk).
        File(File&& x) noexcept;

        /// Destructs a file and closes the filesystem handle.
        ~File() noexcept;

        /// Moves a file object (not the file on disk).
        File& operator=(File&& x) noexcept;

        // Copy operations are not allowed
        File(const File&) = delete;
        File& operator=(const File&) = delete;

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

        /// Reads `size` bytes from the file into `dst` and stores the count of `bytesRead`.
        ///
        /// \param[out] dst The destination buffer to fill with data from the file.
        /// \param[in] size The number of bytes to read. The `dst` buffer is expected to be large
        ///     enough to contain `size` bytes.
        /// \param[out] bytesRead Optional. The number of bytes that were actually read from the file.
        /// \return The result of the read operation.
        Result Read(void* dst, uint32_t size, uint32_t* bytesRead = nullptr);

        /// Reads `size` bytes from the file at `offset` into `dst` and stores the count of
        /// `bytesRead`.
        ///
        /// \note The position of the file handle after calling this function is undefined.
        ///
        /// \param[out] dst The destination buffer to fill with data from the file.
        /// \param[in] offset The byte offset into the file to begin the read.
        /// \param[in] size The number of bytes to read. The `dst` buffer is expected to be large
        ///     enough to contain `size` bytes.
        /// \param[out] bytesRead Optional. The number of bytes that were actually read from the file.
        /// \return The result of the read operation.
        Result ReadAt(void* dst, uint64_t offset, uint32_t size, uint32_t* bytesRead = nullptr);

        /// Writes the `size` bytes from `src` to the given file and stores the `bytesWritten`.
        ///
        /// \param[out] src The source buffer to write into the file.
        /// \param[in] size The number of bytes to write. The `src` buffer is expected to be large
        ///     enough to contain `size` bytes.
        /// \param[out] bytesWritten Optional. The number of bytes that were actually written to the file.
        /// \return The result of the write operation.
        Result Write(const void* src, uint32_t size, uint32_t* bytesWritten = nullptr);

        /// Writes `size` bytes from `src` at `offset` and stores the count of `bytesWritten`.
        ///
        /// \note The position of the file handle after calling this function is undefined.
        ///
        /// \param[out] src The source buffer to write into the file.
        /// \param[in] offset The byte offset into the file to begin the write.
        /// \param[in] size The number of bytes to write. The `src` buffer is expected to be large
        ///     enough to contain `size` bytes.
        /// \param[out] bytesWritten Optional. The number of bytes that were actually written to the file.
        /// \return The result of the write operation.
        Result WriteAt(const void* src, uint64_t offset, uint32_t size, uint32_t* bytesWritten = nullptr);

        /// Flushes the buffers from the given file.
        ///
        /// \return The result of the flush operation.
        Result Flush();

        /// Locks the file for exclusive or shared access by the calling process. A lock can extend
        /// beyond the end of the file.
        ///
        /// \param[in] offset The byte offset from the start of the file to begin the lock section.
        /// \param[in] size The number of bytes to lock in the file. Passing zero (0) means to
        ///     lock from offset to the end of the file, no matter how large the file grows.
        /// \param[in] flags Flags to control the behavior of the lock.
        /// \return The result of the lock operation.
        Result Lock(uint64_t offset, uint64_t size, FileLockFlag flags = FileLockFlag::None);

        /// Unlocks the file access held by this process.
        ///
        /// \param[in] offset The byte offset from the start of the file to begin the unlock section.
        /// \param[in] size The number of bytes to unlock in the file. Passing zero (0) means to
        ///     unlock from offset to the end of the file, no matter how large the file grows.
        /// \return The result of the unlock operation.
        Result Unlock(uint64_t offset, uint64_t size);

        /// Fills `attributes` with the attributes of the file at `path`. If this returns a
        /// non-successful result then `attributes` will not contain valid values.
        ///
        /// \param[out] outAttributes The attributes structure to fill with data.
        /// \return The result of the operation.
        Result GetAttributes(FileAttributes& outAttributes) const;

        /// Gets the full path to the file and writes it into `outPath`.
        ///
        /// \param[out] outPath The string to fill with the absolute path to this file.
        /// \return The result of the operation.
        Result GetPath(String& outPath) const;

        /// Sets the file's access time and/ore last write time. If any time is `nullptr`, the
        /// corresponding file time attribute remains unchanged.
        ///
        /// \param[in] accessTime Time value to set the file's last access time to, or nullptr to
        ///     leave it unchanged.
        /// \param[in] writeTime Time value to set the file's last write time to, or nullptr to
        ///     leave it unchanged.
        Result SetTimes(const SystemTime* accessTime, const SystemTime* writeTime);

    public:
        intptr_t m_fd;
    };

    /// Represents a memory-mapped file.
    class MemoryMap
    {
    public:
        /// Constructs a memory map object.
        MemoryMap() noexcept;

        /// Moves a memory map object.
        MemoryMap(MemoryMap&& x) noexcept;

        /// Destructs a memory map object, and closes filesystem handles.
        ~MemoryMap() noexcept;

        /// Moves a memory map object.
        MemoryMap& operator=(MemoryMap&& x) noexcept;

        // Copy operations are not allowed
        MemoryMap(const MemoryMap&) = delete;
        MemoryMap& operator=(const MemoryMap&) = delete;

        /// Maps a file's content into a memory buffer.
        ///
        /// \param[in] file The open file object to map into memory.
        /// \param[in] mode The access mode for the memory map.
        /// \param[in] offset Optional. Offset in bytes from the start of the file to begin the
        ///     map section. This value must be aligned to the system allocation granularity.
        /// \param[in] size Optional. Number of bytes to map. Zero will map the entire file.
        /// \return The result of the mapping operation.
        Result Map(const File& file, MemoryMapMode mode, uint64_t offset = 0, uint32_t size = 0);

        /// Checks if a file is currently memory mapped.
        ///
        /// \return True if a file is memory mapped, false otherwise.
        bool IsMapped() const;

        /// Unmaps the file from memory.
        void Unmap();

        /// Flushes a section of the memory mapped data to the underlying file.
        ///
        /// \param[in] offset Number of bytes from the start of the mapped buffer to begin the flush.
        /// \param[in] size Number of bytes to flush to the file.
        /// \param[in] async Optional. Set to true to not block while the data is being flushed
        ///     to disk. The default value is false, which blocks until the data has been flushed.
        /// \return The result of the flush operation.
        Result Flush(uint64_t offset, uint32_t size, bool async = false);

        /// Flushes the entire memory mapped region of a file to the underlying file.
        ///
        /// \param[in] async Optional. Set to true to not block while the data is being flushed
        ///     to disk. The default value is false, which blocks until the data has been flushed.
        /// \return The result of the flush operation.
        Result Flush(bool async = false) { return Flush(0, m_size, async); }

    public:
        void* m_data;
        uint32_t m_size;

    #if defined(HE_PLATFORM_API_WIN32)
        void* m_handle;
        void* m_fileHandle;
    #endif
    };

    // --------------------------------------------------------------------------------------------
    // Inline definitions

    template <typename T>
    inline Result File::ReadAll(T& dst, const char* path, uint32_t* outBytesRead)
    {
        constexpr uint32_t ElementSize = sizeof(typename T::ElementType);

        File f;
        Result r = f.Open(path, FileOpenMode::ReadExisting, FileOpenFlag::SequentialScan);
        if (!r)
            return r;

        const uint32_t size = static_cast<uint32_t>(f.GetSize());
        const uint32_t containerSize = ElementSize == 1 ? size : (size + (ElementSize - 1)) / ElementSize;
        dst.Resize(containerSize, DefaultInit);

        uint32_t bytesRead = 0;
        r = f.ReadAt(dst.Data(), 0, size, &bytesRead);

        if (outBytesRead)
            *outBytesRead = bytesRead;

        if (!r)
            return r;

        if (bytesRead < size)
        {
            const uint32_t containerReadSize = ElementSize == 1 ? bytesRead : (bytesRead + (ElementSize - 1)) / ElementSize;
            dst.Resize(containerReadSize);
        }

        return Result::Success;
    }
}
