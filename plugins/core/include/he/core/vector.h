// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/concepts.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    /// A dynamically sized array of contiguous elements.
    template <typename T>
    class Vector final
    {
    public:
        /// The type of elements in the vector.
        using ElementType = T;

        /// The minimum number of elements to allocate when the vector resizes.
        static constexpr uint32_t MinElements = 8;

        /// The maximum number of elements that can be allocated.
        static constexpr uint32_t MaxElements = 0xffffffff;

        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty vector.
        ///
        /// \param allocator Optional. The allocator to use.
        explicit Vector(Allocator& allocator = Allocator::GetDefault()) noexcept;

        /// Construct a vector by copying `x`, and using `allocator` for this vector's allocations.
        ///
        /// \param x The vector to copy from.
        /// \param allocator The allocator to use.
        Vector(const Vector& x, Allocator& allocator) noexcept;

        /// Construct a vector by moving `x`, and using `allocator` for this vector's allocations.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The vector to move from.
        /// \param allocator The allocator to use.
        Vector(Vector&& x, Allocator& allocator) noexcept;

        /// Construct a vector by copying `x`, using the allocator from `x`.
        ///
        /// \param x The vector to copy from.
        Vector(const Vector& x) noexcept;

        /// Construct a vector by moving `x`, using the allocator from `x`.
        ///
        /// \param x The vector to move from.
        Vector(Vector&& x) noexcept;

        /// Destructs the vector and all elements therein. All memory allocations are freed.
        ~Vector() noexcept;

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the vector `x` into this vector.
        ///
        /// \param x The vector to copy from.
        Vector& operator=(const Vector& x) noexcept;

        /// Move the vector `x` into this vector.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The vector to move from.
        Vector& operator=(Vector&& x) noexcept;

        /// Gets a reference to the element at `index`. Asserts if `index` is not less than
        /// \see Size().
        ///
        /// \param index The index of the element to return.
        /// \return A reference to the element at `index`.
        const T& operator[](uint32_t index) const;

        /// \copydoc operator[](uint32_t)
        T& operator[](uint32_t index) { return const_cast<T&>(const_cast<const Vector&>(*this)[index]); }

        /// Checks if this vector is equal to another vector.
        ///
        /// \param x The vector to check against.
        /// \return True if the vectors are equal, false otherwise.
        template <typename U> requires(IsConvertible<U(*)[], T(*)[]>)
        bool operator==(const Vector<U>& x) const;

        /// Checks if this vector is not equal to another vector.
        ///
        /// \param x The vector to check against.
        /// \return True if the vectors are not equal, false otherwise.
        template <typename U> requires(IsConvertible<U(*)[], T(*)[]>)
        bool operator!=(const Vector<U>& x) const { return !this->operator==(x); }

        /// Replaces the contents of this string with a copy of the characters in `range`.
        ///
        /// \param str The string source to copy from.
        template <typename R> requires(!IsSame<R, Vector<T>> && ContiguousRange<R, const T>)
        Vector& operator=(const R& range) { Clear(); Insert(0, range.Data(), range.Size()); return *this; }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this vector is empty.
        ///
        /// \return Returns true if this is an empty vector.
        bool IsEmpty() const { return Size() == 0; }

        /// The capacity the vector has for elements.
        ///
        /// Note: This is not the size of the allocation, but rather the number of total
        /// elements the vector can hold before having to reallocate.
        ///
        /// \return Number of total elements this vector can store.
        uint32_t Capacity() const;

        /// The number of elements the vector is currently storing.
        ///
        /// \return Number of elements stored by the vector.
        uint32_t Size() const;

        /// Reserves capacity for `len` elements. \see Capacity() is garuanteed to return
        /// at least `len` after this operation.
        ///
        /// \param len The number of elements to reserve capacity for.
        void Reserve(uint32_t len);

        /// Resizes the vector to be `len` elements. If `len` is larger than the
        /// current size, then the new elements are default initialized. If `len`
        /// is smaller than the current size, then the extra elements are destructed.
        ///
        /// \param len The number of elements the vector will contain after the operation.
        void Resize(uint32_t len, DefaultInitTag);

        /// Resizes the vector to be `len` elements. If `len` is larger than the
        /// current size, then the new elements are constructed with `args`. If `len`
        /// is smaller than the current size, then the extra elements are destructed.
        ///
        /// \note Moving any arguments into this function will likely yield undesired results,
        /// since they will be moved into the first new element and then be in an unspecified
        /// state for the others.
        ///
        /// \param len The number of elements the vector will contain after the operation.
        /// \param args The arguments to pass to the constructor of new elements.
        template <typename... Args>
        void Resize(uint32_t len, Args&&... args);

        /// Resizes the vector to be `len` elements larger. The new elements are default
        /// initialized.
        ///
        /// \note This is different than `Resize(Size() + len)` in that it will use normal growth
        /// rules to expand the storage. There will likely be slack in the capacity after calling
        /// `Expand(len)`, a property that `Resize(Size() + len)` does not have.
        ///
        /// \param len The number of elements to expand the vector size by.
        void Expand(uint32_t len, DefaultInitTag);

        /// Resizes the vector to be `len` elements larger. The new elements are constructed with
        /// `args`.
        ///
        /// \note This is different than `Resize(Size() + len)` in that it will use normal growth
        /// rules to expand the storage. There will likely be slack in the capacity after calling
        /// `Expand(len)`, a property that `Resize(Size() + len)` does not have.
        ///
        /// \note Moving any arguments into this function will likely yield undesired results,
        /// since they will be moved into the first new element and then be in an unspecified
        /// state for the others.
        ///
        /// \param len The number of elements to expand the vector size by.
        /// \param args The arguments to pass to the constructor of new elements.
        template <typename... Args>
        void Expand(uint32_t len, Args&&... args);

        /// Shrinks the memory allocation to fit the current size of the vector.
        void ShrinkToFit();

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets a pointer to the vector's buffer.
        ///
        /// \return A pointer to the data buffer.
        const T* Data() const;

        /// \copydoc Data()
        T* Data() { return const_cast<T*>(const_cast<const Vector*>(this)->Data()); }

        /// Adopts the preallocated memory and takes ownership of its lifetime. The adopted
        /// memory must have been allocated by the same allocator this vector was constructed
        /// with.
        ///
        /// Any memory already allocated by the vector is destroyed when calling this function.
        ///
        /// \param data A pointer to the memory to adopt.
        /// \param size Number of valid elements in the adopted memory.
        /// \param capcity Number of allocated elements in the adopted memory.
        void Adopt(T* data, uint32_t size, uint32_t capacity);

        /// Releases control of the vector's allocated memory and returns ownership to the caller.
        /// The returned memory must be freed by calling \see Allocator::Free using the same
        /// allocator that the vector was constructed with.
        ///
        /// After calling this method the vector is reset to a valid empty state and can be
        /// used again, which creates a new allocation of memory.
        ///
        /// \return The vector's allocated memory.
        T* Release();

        /// Gets a reference to the vector's first element. The vector must not be empty.
        ///
        /// \return A reference to the first element.
        const T& Front() const { HE_ASSERT(!IsEmpty()); return m_data[0]; }

        /// \copydoc Front()
        T& Front() { return const_cast<T&>(const_cast<const Vector*>(this)->Front()); }

        /// Gets a reference to the vector's last element. The vector must not be empty.
        ///
        /// \return A reference to the last element.
        const T& Back() const { HE_ASSERT(!IsEmpty()); return m_data[m_size - 1]; }

        /// \copydoc Back()
        T& Back() { return const_cast<T&>(const_cast<const Vector*>(this)->Back()); }

        /// Returns a reference to the allocator object used by the vector.
        ///
        /// \return The allocator object this vector uses.
        Allocator& GetAllocator() const { return m_allocator; }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first element in the vector.
        ///
        /// \return A pointer to the first element.
        const T* Begin() const;

        /// \copydoc Begin()
        T* Begin() { return const_cast<T*>(const_cast<const Vector*>(this)->Begin()); }

        /// Gets a pointer to one past the last element in the vector.
        ///
        /// \return A pointer to one past the last element.
        const T* End() const;

        /// \copydoc End()
        T* End() { return const_cast<T*>(const_cast<const Vector*>(this)->End()); }

        /// \copydoc Begin()
        const T* begin() const { return Begin(); }

        /// \copydoc Begin()
        T* begin() { return Begin(); }

        /// \copydoc End()
        const T* end() const { return End(); }

        /// \copydoc End()
        T* end() { return End(); }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets the size of the vector to zero, and destructs elements as necessary.
        /// Does not affect memory allocation.
        void Clear();

        /// Copies the element into the vector at `index`.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the vector to insert at.
        /// \param element The element to copy.
        void Insert(uint32_t index, const T& element);

        /// Moves the element into the vector at `index`.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the vector to insert at.
        /// \param element The element to move.
        void Insert(uint32_t index, T&& element);

        /// Copies the range of elements from `[begin, end)` into the vector at `index`.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the vector to insert at.
        /// \param begin The start of the range to copy.
        /// \param end The end of the range to copy.
        void Insert(uint32_t index, const T* begin, const T* end);

        /// Copies the range of elements from `[begin, end)` into the vector at `index`.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the vector to insert at.
        /// \param begin The start of the range to copy.
        /// \param count The number of items in the range to copy.
        void Insert(uint32_t index, const T* begin, uint32_t count) { Insert(index, begin, begin + count); }

        /// Erases `count` elements from the vector starting at `index`. Order of elements
        /// is preserved. Asserts if `index + count` is out of range.
        ///
        /// \param index The index of the first element to erase.
        /// \param count Optional. The number of elements to erase. Default is one.
        void Erase(uint32_t index, uint32_t count = 1);

        /// Erases `count` elements from the vector starting at `index`. Order of elements
        /// is *not* preserved. Asserts if `index + count` is out of range.
        ///
        /// \note This function may result in less element moves and may therefore be faster than
        /// the Erase function in some cases. However, ordering of elements after the operation is
        /// not defined.
        ///
        /// \param index The index of the first element to erase.
        /// \param count Optional. The number of elements to erase. Default is one.
        void EraseUnordered(uint32_t index, uint32_t count = 1);

        /// Copies the element into the end of the vector.
        ///
        /// \param element The element to copy.
        /// \return The newly constructed element.
        T& PushBack(const T& element);

        /// Moves the element into the end of the vector.
        ///
        /// \param element The element to move.
        /// \return The newly constructed element.
        T& PushBack(T&& element);

        /// Copies the element into the front of the vector.
        ///
        /// \param element The element to copy.
        /// \return The newly constructed element.
        T& PushFront(const T& element);

        /// Moves the element into the front of the vector.
        ///
        /// \param element The element to move.
        /// \return The newly constructed element.
        T& PushFront(T&& element);

        /// Removes the last element from the end of the vector and destructs it.
        void PopBack();

        /// Removes the first element from the start of the vector and destructs it.
        void PopFront();

        /// Creates a new element at the end of the vector and returns a reference to it.
        /// The element is default initialized.
        ///
        /// \return A reference to the newly created element.
        T& EmplaceBack(DefaultInitTag);

        /// Creates a new element at the end of the vector and returns a reference to it.
        /// The element is default constructed.
        ///
        /// \return A reference to the newly created element.
        T& EmplaceBack();

        /// Creates a new element at the end of the vector and returns a reference to it.
        /// Any arguments passed are forwarded to the new element's constructor.
        ///
        /// \param args The arguments to pass to the constructor.
        /// \return A reference to the newly created element.
        template <typename... Args>
        T& EmplaceBack(Args&&... args);

    private:
        // Grows the internal capacity to make space for `len` elements.
        void GrowBy(uint32_t len);

        // Calculate geometric growth that will be necessary to include `len` additional elements.
        uint32_t CalculateGrowth(uint32_t len) const;

        // Copy the given vector.
        void CopyFrom(const Vector& x);

        // Move from the given vector into this object.
        void MoveFrom(Vector&& x);

        // Destroys the vector and frees any associated memory.
        void Destroy();

    private:
        friend class VectorTestAttorney;

        Allocator& m_allocator;

        T* m_data{ nullptr };
        uint32_t m_size{ 0 };
        uint32_t m_capacity{ 0 };
    };
}

#include "he/core/inline/vector.inl"
