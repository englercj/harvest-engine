// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/macros.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include <new>

namespace he
{
    /// A dynamically sized array of contiguous elements.
    template <typename T>
    class Vector
    {
    public:
        /// The minimum number of elements to allocate when the vector resizes.
        static constexpr uint32_t MinElements = 8;

        /// The maximum number of elements that can be allocated.
        static constexpr uint32_t MaxElements = 0xffffffff;

        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty vector.
        ///
        /// \param allocator The allocator to use for any allocations.
        Vector(Allocator& allocator);

        /// Construct a vector by copying `x`, and using `allocator` for this vector's allocations.
        ///
        /// \param allocator The allocator to use for any allocations.
        /// \param x The vector to copy from.
        Vector(Allocator& allocator, const Vector& x);

        /// Construct a vector by moving `x`, and using `allocator` for this vector's allocations.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param allocator The allocator to use for any allocations.
        /// \param x The vector to move from.
        Vector(Allocator& allocator, Vector&& x);

        /// Construct a vector by copying `x`, using the allocator from x.
        ///
        /// \param x The vector to copy from.
        Vector(const Vector& x);

        /// Construct a vector by moving `x`, using the allocator from x.
        ///
        /// \param x The vector to move from.
        Vector(Vector&& x);

        /// Destructs the vector and all elements therein. All memory allocations are freed.
        ~Vector();

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the vector `x` into this vector.
        ///
        /// \param x The vector to copy from.
        Vector& operator=(const Vector& x);

        /// Move the vector `x` into this vector.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The vector to move from.
        Vector& operator=(Vector&& x);

        /// Gets a reference to the element at `index`. Asserts if `index` is not less than
        /// \see Size().
        ///
        /// \param index The index of the element to return.
        /// \return A reference to the element at `index`.
        T& operator[](uint32_t index);

        /// \copydoc operator[](uint32_t)
        const T& operator[](uint32_t index) const { return const_cast<const T&>(const_cast<Vector&>(*this)[index]); }

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

        /// Shrinks the memory allocation to fit the current size of the vector.
        void ShrinkToFit();

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets a pointer to the vector's buffer.
        ///
        /// \return A pointer to the data buffer.
        T* Data();

        /// \copydoc Data()
        const T* Data() const { return const_cast<const T*>(const_cast<Vector*>(this)->Data()); }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first element in the vector.
        ///
        /// \return A pointer to the first element.
        T* Begin();

        /// \copydoc Begin()
        const T* Begin() const { return const_cast<const T*>(const_cast<Vector*>(this)->Begin()); }

        /// Gets a pointer to one past the last element in the vector.
        ///
        /// \return A pointer to one past the last element.
        T* End();

        /// \copydoc End()
        const T* End() const { return const_cast<const T*>(const_cast<Vector*>(this)->End()); }

        /// \copydoc Begin()
        char* begin() { return Begin(); }

        /// \copydoc Begin()
        const char* begin() const { return Begin(); }

        /// \copydoc End()
        char* end() { return End(); }

        /// \copydoc End()
        const char* end() const { return End(); }

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

        /// Erases `count` elements from the vector starting at `index`.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the vector to insert at.
        /// \param count The number of elements to remove.
        void Erase(uint32_t index, uint32_t count);

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

        /// Removes the last element from the end of the vector and destructs it.
        void PopBack();

        /// Creates a new element at the end of the vector and returns a reference to it.
        /// The element is default initialized.
        ///
        /// \return The newly created element.
        T& EmplaceBack(DefaultInitTag);

        /// Creates a new element at the end of the vector and returns a reference to it.
        /// Any arguments passed are forwarded to the new element's constructor.
        ///
        /// \return The newly created element.
        template <typename... Args>
        T& EmplaceBack(Args&&... args);

    private:
        // Grows the internal capacity to make space for `n` elements.
        void GrowBy(uint32_t n);

        // Calculate geometric growth that will be necessary to include `n` additional elements.
        uint32_t CalculateGrowth(uint32_t n);

        // Copy the given string.
        void CopyFrom(const Vector& x);

        // Move from the given string into this object.
        void MoveFrom(Vector&& x);

    private:
        Allocator& m_allocator;

        T* m_data{ nullptr };
        uint32_t m_size{ 0 };
        uint32_t m_capacity{ 0 };
    };
}

#include "he/core/inline/vector.inl"
