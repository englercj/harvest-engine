// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/type_traits.h"

namespace he
{
    class Allocator
    {
    public:
        /// The default alignment value that all allocations will use.
        static constexpr size_t DefaultAlignment = alignof(max_align_t);

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

        /// Allocates and constructs a new `T`.
        ///
        /// \tparam The type to allocate and construct.
        /// \param args optional arguments for the constructor.
        template <typename T, class... Args>
        T* New(Args&&... args)
        {
            void* p = Malloc(sizeof(T), alignof(T));
            return new(p) T(static_cast<Args&&>(args)...);
        }

        /// Deletes `p` which must have been allocated with New.
        ///
        /// \param p The pointer to destruct and deallocate.
        template <typename T, HE_REQUIRES(std::is_trivially_destructible_v<T>)>
        void Delete(const T* p)
        {
            Free(const_cast<T*>(p));
        }

        /// Deletes `p` which must have been allocated with New.
        ///
        /// \param p The pointer to destruct and deallocate.
        template <typename T, HE_REQUIRES(!std::is_trivially_destructible_v<T>)>
        void Delete(const T* p)
        {
            if (p)
            {
                p->~T();
                Free(const_cast<T*>(p));
            }
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
}
