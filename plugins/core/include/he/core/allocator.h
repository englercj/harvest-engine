// Copyright Chad Engler

#pragma once

#include "he/core/memory_ops.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

// Forward declare placement new. Doing this instead of including <new> has a major impact
// on reducing compile times, espcially since this file is included everywhere.
[[nodiscard]] inline void* __cdecl operator new(size_t, void* ptr) noexcept;

namespace he
{
    // --------------------------------------------------------------------------------------------
    /// Interface for memory allocators.
    ///
    /// All functions of a Harvest memory allocator are expected to be thread-safe when dealing
    /// with different pointers. That is, calling Free with different pointers from different
    /// threads must be safe. However, calling Free with the *same* pointer from different threads
    /// is not safe. Similarly calling two different functions with the same pointer from different
    /// threads is not safe.
    class Allocator
    {
    public:
        /// The default alignment value that all allocations will use.
        static constexpr size_t DefaultAlignment = alignof(max_align_t);

        /// Gets an allocator used for general allocations throughout the engine.
        /// Most places in the engine that allocate also take an allocator parameter, which
        /// if not specified uses this allocator instead.
        ///
        /// If you wish to define the allocator used here yourself then define the symbol
        /// \ref HE_USER_DEFINED_DEFAULT_ALLOCATOR as one (1), and then define this function's
        /// body in your code.
        ///
        /// \return The default allocator object.
        static Allocator& GetDefault();

    public:
        virtual ~Allocator() = default;

        /// Allocates a new memory block that is at least `size` bytes large and returns a pointer
        /// to the allocated memory that is aligned to `alignment`.
        ///
        /// \param size The size in bytes of the new memory block.
        /// \param alignment Optional. Custom alignment for the pointer.
        /// \return A pointer to the newly allocated memory.
        [[nodiscard]] virtual void* Malloc(size_t size, size_t alignment = DefaultAlignment) noexcept = 0;

        /// Rellocates an existing memory block. This function may be more efficient than creating
        /// a new allocation, copying data, and freeing the original allocation. Instead, this
        /// function may simply extend the existing allocation if possible.
        ///
        /// The semantics for this function are as follows:
        /// 1. If `ptr` is null, then it is equivalent to \ref Allocator::Malloc
        /// 2. If `ptr` is not null, and `newSize` is zero, then it is equivalent to
        ///     \ref Allocator::Free
        /// 3. If `ptr` is not null, and `newSize` is not zero, then the memory is reallocated to
        ///     match `newSize` and a valid pointer is returned.
        ///
        /// \param ptr A pointer to an existing memory allocation returned from Malloc. This
        ///     pointer should no longer be used after this function returns.
        /// \param newSize The desired size in bytes of the new memory block.
        /// \param alignment Optional. Custom alignment for the pointer. Must match the original
        ///     value passed to Malloc for over-aligned allocations.
        /// \return A pointer to the newly reallocated memory.
        virtual void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) noexcept = 0;

        /// Frees a memory block that was allocated with Malloc or reallocated with Realloc.
        ///
        /// \param ptr The pointer returned from Malloc or Realloc.
        virtual void Free(void* ptr) noexcept = 0;

        /// Allocates a new memory block that can fit `count` instance of a type `T`.
        ///
        /// \note Constructors are not called.
        ///
        /// \tparam The type to allocate enough space for, and to use for alignment.
        /// \param count The number of instances to allocate space for.
        /// \return A pointer to the newly allocated memory.
        template <typename T>
        [[nodiscard]] T* Malloc(uint32_t count) noexcept
        {
            return static_cast<T*>(Malloc(sizeof(T) * count, alignof(T)));
        }

        /// Allocates and constructs a new `T`.
        ///
        /// \tparam The type to allocate and construct.
        /// \param args optional arguments for the constructor.
        template <typename T, class... Args>
        [[nodiscard]] T* New(Args&&... args) noexcept
        {
            void* p = Malloc<T>(1);
            return ::new(p) T(Forward<Args>(args)...);
        }

        /// Allocates and constructs a new array of `T`.
        ///
        /// \tparam The type to allocate and construct.
        /// \param count The number of array elements to allocate and construct.
        /// \param args optional arguments for the constructor.
        template <typename T> requires(IsTriviallyConstructible<T> && IsTriviallyDestructible<T>)
        [[nodiscard]] T* NewArray(uint32_t count) noexcept
        {
            T* mem = Malloc<T>(count);
            MemZero(mem, count * sizeof(T));
            return mem;
        }

        /// Allocates and constructs a new array of `T`.
        ///
        /// \tparam The type to allocate and construct.
        /// \param count The number of array elements to allocate and construct.
        /// \param args optional arguments for the constructor.
        template <typename T, class... Args>
        [[nodiscard]] T* NewArray(uint32_t count, Args&&... args) noexcept
        {
            constexpr size_t Align = AlignUp(alignof(T), sizeof(void*));
            constexpr size_t Offset = AlignUp<size_t>(sizeof(size_t), Align);

            // We're going to put a size_t at the allocated location, so it must be size_t aligned.
            static_assert(IsAligned(Align, alignof(size_t)));

            const size_t size = Offset + (sizeof(T) * count);

            uint8_t* ptr = static_cast<uint8_t*>(Malloc(size, Align));
            if (ptr == nullptr)
                return nullptr;

            // Store array length so we can retrieve it later
            *reinterpret_cast<size_t*>(ptr) = count;

            // Construct each array element
            T* p = reinterpret_cast<T*>(ptr + Offset);
            for (size_t i = 0; i < count; ++i)
            {
                ::new(p + i) T(Forward<Args>(args)...);
            }

            return p;
        }

        /// Deletes `p` which must have been allocated with New.
        ///
        /// \param p The pointer to destruct and deallocate.
        template <typename T> requires(IsTriviallyDestructible<T>)
        void Delete(const T* p) noexcept
        {
            Free(const_cast<T*>(p));
        }

        /// Deletes `p` which must have been allocated with New.
        ///
        /// \param p The pointer to destruct and deallocate.
        template <typename T> requires(!IsTriviallyDestructible<T>)
        void Delete(const T* p) noexcept
        {
            if (p)
            {
                p->~T();
                Free(const_cast<T*>(p));
            }
        }

        /// Deletes `p` which must have been allocated with NewArray.
        ///
        /// \param p The pointer to destruct and deallocate.
        template <typename T> requires(IsTriviallyDestructible<T>)
        void DeleteArray(const T* p) noexcept
        {
            Free(const_cast<T*>(p));
        }

        /// Deletes `p` which must have been allocated with NewArray.
        ///
        /// \param p The pointer to destruct and deallocate.
        template <typename T> requires(!IsTriviallyDestructible<T>)
        void DeleteArray(const T* p) noexcept
        {
            if (!p)
                return;

            constexpr size_t Align = AlignUp(alignof(T), sizeof(void*));
            constexpr size_t Offset = AlignUp<size_t>(sizeof(size_t), Align);

            // Read the array length from the header
            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(p) - Offset;
            const size_t count = *reinterpret_cast<const size_t*>(ptr);

            // Destruct each of the array items
            for (size_t i = 0; i < count; ++i)
            {
                p[count - i - 1].~T();
            }

            // Free the memory
            Free(const_cast<uint8_t*>(ptr));
        }
    };

    // --------------------------------------------------------------------------------------------
    /// Allocator that forwards memory requests to the CRT. This is the default allocator that
    /// the engine will use if \ref HE_USER_DEFINED_DEFAULT_ALLOCATOR is defined as zero (default).
    class CrtAllocator : public Allocator
    {
    public:
        /// Returns the singleton instance of the CrtAllocator.
        static CrtAllocator& Get();

        /// \copydoc Allocator::Malloc(size_t, size_t)
        [[nodiscard]] void* Malloc(size_t size, size_t alignment = DefaultAlignment) noexcept override;

        /// \copydoc Allocator::Realloc(void*, size_t, size_t)
        void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) noexcept override;

        /// \copydoc Allocator::Free(void*)
        void Free(void* ptr) noexcept override;
    };
}
