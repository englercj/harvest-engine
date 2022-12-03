// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/core/types.h"

namespace he
{
    /// Enumerations of the types of memory access that are allowed.
    enum class MemoryAccess : uint8_t
    {
        None,               ///< Memory cannot be accessed.
        Read,               ///< Memory can only be read.
        ReadWrite,          ///< Memory can be read or written.
        Execute,            ///< Memory can only be executed.
        ReadExecute,        ///< Memory can be read or executed.
        ReadWriteExecute,   ///< Memory can be read, written, or executed.
    };

    /// The virtual memory class tracks a block of reserved virtual pages and provides mechanisms
    /// for interacting with those pages. Virtual pages that are reserved do not consume any
    /// physical memory, only virtual address space, until \ref VirtualMemory::Commit is called.
    class VirtualMemory
    {
    public:
        /// Returns the size of a virtual memory page.
        ///
        /// \return Size of a page in bytes.
        static size_t GetPageSize();

        /// Aligns up a number of bytes to a count of pages.
        ///
        /// \param[in] size The number of bytes to align up to pages.
        /// \return Number of pages necessary to fit `size` bytes.
        static size_t BytesToPages(size_t size);

        /// Calculates the byte size of a count of pages.
        ///
        /// \param[in] count The number of pages to calculate the size of.
        /// \return Number of bytes available across `count` pages.
        static size_t PagesToBytes(size_t count);

    public:
        /// Constructs a virtual memory block with no reserved pages.
        VirtualMemory() = default;

        /// Move constructs a virtual memory block from `x`.
        VirtualMemory(VirtualMemory&& x) noexcept;

        /// Destructs a virtual memory block and releases all reserved or committed pages.
        ~VirtualMemory() noexcept { Release(); }

        /// Move assigns a virtual memory block.
        VirtualMemory& operator=(VirtualMemory&& x) noexcept;

        /// Reserves `count` pages of virtual address space, but does not commit any actual pages
        /// to physical memory.
        ///
        /// \param[in] count Number of pages to reserve.
        /// \return The result of the operation.
        Result Reserve(size_t count);

        /// Releases all reserved, or comitted, pages in the block. The block cannot be used until
        /// \ref Reserve is called again to reserve a new block of pages.
        ///
        /// \return The result of the operation.
        Result Release();

        /// Commits a range of pages in a reserved block to physical memory. Committing pages that are
        /// already committed will not cause this function to fail.
        ///
        /// \param[in] index The starting index of the page(s) to commit.
        /// \param[in] count The number of pages to commit.
        /// \param[in] access The access rights to set on the committed pages.
        /// \return A pointer to the first page that was committed.
        void* Commit(size_t index, size_t count, MemoryAccess access = MemoryAccess::ReadWrite);

        /// Decommits a range of pages from the block, returning them to the reserved state.
        /// Decommitting pages that are not committed will not cause this function to fail.
        ///
        /// \param[in] index The starting index of the page(s) to decommit.
        /// \param[in] count The number of pages to decommit.
        /// \return The result of the operation.
        Result Decommit(size_t index, size_t count);

        /// Sets the memory protections for a range of committed pages. If the state of any page in
        /// the range is not committed, the function fails and returns without modifying the access
        /// protection of any pages in the specified region.
        ///
        /// \param[in] index The starting index of the page(s) to protect.
        /// \param[in] count The number of pages to protect.
        /// \param[in] access The access rights to set on the committed pages.
        /// \return The result of the operation.
        Result Protect(size_t index, size_t count, MemoryAccess access);

        /// Gets the number of pages reserved in this block.
        ///
        /// \return The number of pages this block has reserved.
        size_t Size() const { return m_size; }

        /// Gets a pointer to the underlying block of virtual pages.
        ///
        /// \return pointer to the start of the virtual address space.
        void* Block() { return m_block; }

        /// \copydoc Block
        const void* Block() const { return m_block; }

        /// Gets a pointer to the page at `index`.
        ///
        /// \param[in] index The index of the page to get a pointer to.
        const void* GetPage(size_t index) const;

        /// \copydoc GetPage
        void* GetPage(size_t index) { return const_cast<void*>(const_cast<const VirtualMemory*>(this)->GetPage(index)); }

    private:
        VirtualMemory(const VirtualMemory&) = delete;
        VirtualMemory& operator=(const VirtualMemory&) = delete;

    private:
        void* m_block{ nullptr };
        size_t m_size{ 0 };
    };
}
