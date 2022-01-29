// Copyright Chad Engler

#pragma once

#include "he/core/memory_ops.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include <type_traits>

namespace he
{
    class Allocator
    {
    public:
        /// The default alignment value that all allocations will use.
        static constexpr size_t DefaultAlignment = alignof(max_align_t);

        /// Gets an allocator used for general allocations throughout the engine.
        /// Most places in the engine that allocate also take an allocator parameter, which
        /// if not specified uses this allocator instead.
        ///
        /// If you wish to override the allocator used here you can define the symbol
        /// `HE_ENABLE_CUSTOM_ALLOCATORS`, and then define this function's body in your code.
        ///
        /// \return The default allocator.
        static Allocator& GetDefault();

        /// Gets an allocator used for temporary allocations throughout the engine.
        ///
        /// If you wish to override the allocator used here you can define the symbol
        /// `HE_ENABLE_CUSTOM_ALLOCATORS`, and then define this function's body in your code.
        ///
        /// \return The default allocator.
        static Allocator& GetTemp();

    public:
        virtual ~Allocator() = default;

        /// Allocates a new memory block on the heap, which can be over aligned.
        ///
        /// \param size The size in bytes of the new memory block.
        /// \param alignment Optional. Custom alignment for the pointer.
        /// \return A pointer to the newly allocated memory.
        virtual void* Malloc(size_t size, size_t alignment = DefaultAlignment) = 0;

        /// Rellocates a memory block on the heap. Tries to avoid allocate and copy by extending
        /// the existing memory block if possible.
        ///
        /// \param ptr A pointer to an existing memory allocation returned from Malloc.
        /// \param newSize The desired size in bytes of the new memory block.
        /// \param alignment Optional. Custom alignment for the pointer. Must match what was originally passed to Malloc.
        /// \return A pointer to the newly reallocated memory.
        virtual void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) = 0;

        /// Frees a memory block that was allocated with Malloc or reallocated with Realloc.
        ///
        /// \param ptr The pointer returned from Malloc or Realloc.
        virtual void Free(void* ptr) = 0;

        /// Allocates a new memory block that can fit `count` instance of a trivial type `T`.
        ///
        /// \note No constructors are run.
        ///
        /// \tparam The type to allocate enough space for, and to use as a guide for alignment.
        /// \param count The number of elements to allocate space for.
        /// \return A pointer to the newly allocated memory.
        template <typename T>
        T* Malloc(uint32_t count)
        {
            return static_cast<T*>(Malloc(sizeof(T) * count, alignof(T)));
        }

        /// Allocates and constructs a new `T`.
        ///
        /// \tparam The type to allocate and construct.
        /// \param args optional arguments for the constructor.
        template <typename T, class... Args>
        T* New(Args&&... args)
        {
            void* p = Malloc<T>(1);
            return new(p) T(Forward<Args>(args)...);
        }

        /// Allocates and constructs a new array of `T`.
        ///
        /// \tparam The type to allocate and construct.
        /// \param count The number of array elements to allocate and construct.
        /// \param args optional arguments for the constructor.
        template <typename T> requires(std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>)
        T* NewArray(uint32_t count)
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
        T* NewArray(uint32_t count, Args&&... args)
        {
            constexpr size_t Align = alignof(T);
            constexpr size_t Offset = AlignUp<size_t>(sizeof(size_t), Align);

            const size_t size = Offset + (sizeof(T) * count);

            char* ptr = static_cast<char*>(Malloc(size, Align));
            if (ptr == nullptr)
                return nullptr;

            // Store array length so we can retrieve it later
            *reinterpret_cast<size_t*>(ptr) = count;

            // Construct each array element
            T* p = reinterpret_cast<T*>(ptr + Offset);
            for (size_t i = 0; i < count; ++i)
            {
                new(p + i) T(Forward<Args>(args)...);
            }

            return p;
        }

        /// Deletes `p` which must have been allocated with New.
        ///
        /// \param p The pointer to destruct and deallocate.
        template <typename T> requires(std::is_trivially_destructible_v<T>)
        void Delete(const T* p)
        {
            Free(const_cast<T*>(p));
        }

        /// Deletes `p` which must have been allocated with New.
        ///
        /// \param p The pointer to destruct and deallocate.
        template <typename T> requires(!std::is_trivially_destructible_v<T>)
        void Delete(const T* p)
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
        template <typename T> requires(std::is_trivially_destructible_v<T>)
        void DeleteArray(const T* p)
        {
            Free(const_cast<T*>(p));
        }

        /// Deletes `p` which must have been allocated with NewArray.
        ///
        /// \param p The pointer to destruct and deallocate.
        template <typename T> requires(!std::is_trivially_destructible_v<T>)
        void DeleteArray(const T* p)
        {
            if (!p)
                return;

            constexpr size_t Align = alignof(T);
            constexpr size_t Offset = AlignUp<size_t>(sizeof(size_t), Align);

            // Read the array length from the header
            const char* ptr = reinterpret_cast<const char*>(p) - Offset;
            const size_t count = *reinterpret_cast<const size_t*>(ptr);

            // Destruct each of the array items
            for (size_t i = 0; i < count; ++i)
            {
                p[count - i - 1].~T();
            }

            // Free the memory
            Free(const_cast<char*>(ptr));
        }
    };

    /// Allocator that forwards memory requests to the CRT.
    class CrtAllocator : public Allocator
    {
    public:
        /// Returns the singleton instance of the CrtAllocator.
        static CrtAllocator& Get();

        void* Malloc(size_t size, size_t alignment = DefaultAlignment) override;
        void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) override;
        void Free(void* ptr) override;
    };

    /// Allocator that allocates pages of space and expects them all to be freed at the end of use.
    class LinearPageAllocator : public Allocator
    {
    public:
        explicit LinearPageAllocator(size_t pageSize, Allocator& allocator = Allocator::GetDefault());
        ~LinearPageAllocator();

        void* Malloc(size_t size, size_t alignment = DefaultAlignment) override;
        void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) override;
        void Free(void*) override {}

        /// Invalidates all issued pointers and resets the state for reuse. Previously allocated
        /// pages are kept for reuse.
        void Clear();

        /// Invalidates all issued pointers, frees all page memory, and resets state for reuse.
        void Reset();

    private:
        struct PageHeader
        {
            PageHeader* next;
            PageHeader* previous;
        };

        bool CanFit(size_t size, size_t alignment) const;
        void AllocPage();

    private:
        Allocator& m_allocator;
        PageHeader* m_currentPage{ nullptr };
        PageHeader* m_lastPage{ nullptr };
        size_t m_pageOffset{ 0 };
        const size_t m_pageSize{ 0 };
    };
}
