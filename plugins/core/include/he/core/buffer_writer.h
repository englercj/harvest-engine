// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"

#include <type_traits>

namespace he
{
    /// Helper class to make building a byte buffer easier.
    class BufferWriter
    {
    public:
        /// Strategy for growing the buffer when needed.
        enum class GrowthStrategy
        {
            Factor,     ///< Grows by a factor of the current size
            Fixed,      ///< Grows by a fixed amount when resizing
        };

        /// The minimum number of bytes to allocate when the buffer resizes.
        static constexpr uint32_t MinBytes = 1024;

        /// The maximum number of bytes that can be allocated.
        static constexpr uint32_t MaxBytes = 0xffffffff;

    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Constructs a new buffer.
        ///
        /// \param[in] allocator Optional. The allocator to use.
        explicit BufferWriter(Allocator& allocator = Allocator::GetDefault());

        /// Constructs a new buffer.
        ///
        /// \param[in] strategy Optional. The strategy for growth when resizing is required.
        /// \param[in] growth Optional. The amount of growth during a resize. This is either a
        ///     fixed amount, or a factor of the size depending on `strategy`.
        /// \param[in] allocator Optional. The allocator to use.
        BufferWriter(GrowthStrategy strategy, float growth, Allocator& allocator = Allocator::GetDefault());

        /// Construct a buffer by copying `x`, and using `allocator` for this
        /// buffer's allocations.
        ///
        /// \param x The buffer to copy from.
        /// \param allocator The allocator to use for any allocations.
        BufferWriter(const BufferWriter& x, Allocator& allocator);

        /// Construct a buffer by moving `x`, and using `allocator` for this buffer
        /// writer's allocations. If the allocators do not match then a copy operation will
        /// be performed.
        ///
        /// \param x The buffer to move from.
        /// \param allocator The allocator to use for any allocations.
        BufferWriter(BufferWriter&& x, Allocator& allocator);

        /// Construct a buffer by copying `x`, using the allocator from `x`.
        ///
        /// \param x The buffer to copy from.
        BufferWriter(const BufferWriter& x);

        /// Construct a buffer by moving `x`, using the allocator from `x`.
        ///
        /// \param x The buffer to move from.
        BufferWriter(BufferWriter&& x);

        /// Destructs the buffer and all elements therein. All memory allocations are freed.
        ~BufferWriter();

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the buffer `x` into this buffer.
        ///
        /// \param x The buffer to copy from.
        BufferWriter& operator=(const BufferWriter& x);

        /// Move the buffer `x` into this buffer.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The buffer to move from.
        BufferWriter& operator=(BufferWriter&& x);

        /// Gets a reference to the element at `index`. Asserts if `index` is not less than
        /// \see Size().
        ///
        /// \param index The index of the element to return.
        /// \return A reference to the element at `index`.
        const uint8_t& operator[](uint32_t index) const;

        /// \copydoc operator[](uint32_t)
        uint8_t& operator[](uint32_t index) { return const_cast<uint8_t&>(const_cast<const BufferWriter&>(*this)[index]); }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this buffer is empty.
        ///
        /// \return Returns true if this is an empty buffer.
        bool IsEmpty() const { return Size() == 0; }

        /// The capacity of the buffer in bytes.
        ///
        /// \return Number of total bytes this buffer has allocated.
        uint32_t Capacity() const;

        /// The number of bytes written to in the buffer.
        ///
        /// \return Number of bytes written to in the buffer.
        uint32_t Size() const;

        /// Reserves capacity for `len` bytes. \see Capacity() is garuanteed to return
        /// at least `len` after this operation.
        ///
        /// \param len The number of bytes to reserve capacity for.
        void Reserve(uint32_t len);

        /// Resizes the buffer to be `len` bytes. If `len` is larger than the current capacity,
        /// then the buffer's allocation is extended to fit `len`.
        ///
        /// \param len The number of bytes the buffer will contain after the operation.
        void Resize(uint32_t len);

        /// Shrinks the memory allocation to fit the current size of the buffer.
        void ShrinkToFit();

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets a pointer to the buffer's allocated memory.
        ///
        /// \return A pointer to the buffer allocated memory.
        const uint8_t* Data() const;

        /// \copydoc Data()
        uint8_t* Data() { return const_cast<uint8_t*>(const_cast<const BufferWriter*>(this)->Data()); }

        /// Releases control of the buffer's allocation and returns ownership to the caller.
        /// The returned buffer must be freed by calling \see Allocator::Free using the same
        /// allocator that the buffer was constructed with.
        ///
        /// After calling this method the buffer is reset to a valid empty state and can be
        /// used again, which creates a new allocation of memory.
        ///
        /// \return The buffer's allocated memory.
        uint8_t* Release();

        /// Returns a reference to the allocator object used by the buffer.
        ///
        /// \return The allocator object this buffer uses.
        Allocator& GetAllocator() const { return m_allocator; }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets the size of the buffer to zero.
        /// Does not affect memory allocation.
        void Clear();

        /// Copies a trivially copyable type into the buffer.
        ///
        /// \note This is the same as \ref Write(const T&), this alias exists for generic
        /// programming that expects the interface of a container.
        ///
        /// \param[in] value The value to copy.
        template <typename T> requires(std::is_trivially_copyable_v<T>)
        void PushBack(const T& value) { Write(&value, sizeof(T)); }

        /// Copies `len` bytes from `data` into the buffer.
        ///
        /// \param[in] data The buffer to copy from.
        /// \param[in] len The number of bytes to copy.
        void Write(const void* data, uint32_t len);

        /// Copies `len` bytes from `data` into the buffer at a previously allocated offset.
        ///
        /// \note The buffer size and capacity are not affected by this function.
        ///
        /// \param[in] offset The offset within the buffer to write to. Must fit within Size().
        /// \param[in] data The buffer to copy from.
        /// \param[in] len The number of bytes to copy.
        void WriteAt(uint32_t offset, const void* data, uint32_t len);

        /// Copies the null terminated string into the buffer, excluding the null terminating
        /// character itself.
        ///
        /// \note This is equivalent to `Write(str, String::Length(str));`
        ///
        /// \param[in] str The null terminated string to copy.
        void Write(const char* str);

        /// Copies the null terminated string into the buffer at a previously allocated offset,
        /// excluding the null terminating character itself.
        ///
        /// \note This is equivalent to `WriteAt(offset, str, String::Length(str));`
        ///
        /// \note The buffer size and capacity are not affected by this function.
        ///
        /// \param[in] offset The offset within the buffer to write to. Must fit within Size().
        /// \param[in] str The null terminated string to copy.
        void WriteAt(uint32_t offset, const char* str);

        /// Copies a trivially copyable type into the buffer.
        ///
        /// \param[in] value The value to copy.
        template <typename T> requires(std::is_trivially_copyable_v<T>)
        void Write(const T& value) { Write(&value, sizeof(T)); }

        /// Copies a trivially copyable type into the buffer at a previously allocated offset.
        ///
        /// \note The buffer size and capacity are not affected by this function.
        ///
        /// \param[in] offset The offset within the buffer to write to. Must fit within Size().
        /// \param[in] value The value to copy.
        template <typename T> requires(std::is_trivially_copyable_v<T>)
        void WriteAt(uint32_t offset, const T& value) { WriteAt(offset, &value, sizeof(T)); }

        /// Writes a byte repeatedly to the buffer. This is equivalent to growing the buffer
        /// and then calling memset. This can be faster than calling Write() repeatedly.
        ///
        /// \param[in] byte The byte to write repeatedly to the buffer.
        /// \param[in] count The number of times the byte should be written.
        void WriteRepeat(uint8_t byte, uint32_t count);

        /// Writes a byte repeatedly to the buffer at a previously allocated offset. This is
        /// equivalent to calling memset. This can be faster than calling Write() repeatedly.
        ///
        /// \note The buffer size and capacity are not affected by this function.
        ///
        /// \param[in] offset The offset within the buffer to write to. Must fit within Size().
        /// \param[in] byte The byte to write repeatedly to the buffer.
        /// \param[in] count The number of times the byte should be written.
        void WriteRepeatAt(uint32_t offset, uint8_t byte, uint32_t count);

    private:
        friend class BufferWriterTestAttorney;

        // Grows the internal capacity to make space for `len` byes.
        void GrowBy(uint32_t len);

        // Calculate geometric growth that will be necessary to include `len` additional bytes.
        uint32_t CalculateGrowth(uint32_t len) const;

        // Copy the given buffer.
        void CopyFrom(const BufferWriter& x);

        // Move from the given buffer into this object.
        void MoveFrom(BufferWriter&& x);

    private:
        Allocator& m_allocator;

        uint8_t* m_data{ nullptr };
        uint32_t m_size{ 0 };
        uint32_t m_capacity{ 0 };

        GrowthStrategy m_strategy{ GrowthStrategy::Factor };
        float m_growth{ 0.5f };
    };
}
