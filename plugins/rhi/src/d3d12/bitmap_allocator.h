// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/types.h"
#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    // Utility for "allocating" individual bits. Manages a list of bytes and returns offsets
    // to contiguous sections of bits, tracking which offsets are being used.
    class BitmapAllocator
    {
    public:
        static constexpr uint32_t InvalidOffset = 0xffffffff;

        void Create(Allocator& allocator, uint32_t capacity)
        {
            const uint32_t bytes = (capacity + 7) >> 3;
            m_capacity = capacity;
            m_bitmap = static_cast<uint8_t*>(allocator.Malloc(bytes));
            MemZero(m_bitmap, bytes);
        }

        void Destroy(Allocator& allocator)
        {
            HE_ASSERT(m_allocCount == 0);
            allocator.Free(m_bitmap);
            m_bitmap = nullptr;
        }

        uint32_t Alloc(uint32_t size)
        {
            uint32_t base = 0;
            while (base + size < m_capacity)
            {
                uint32_t free = 0;
                for (uint32_t i = 0; i < size && !Test(base + i); ++i)
                    ++free;

                if (free == size)
                {
                    for (uint32_t i = 0; i < size; ++i)
                        Set(base + i);

                #if HE_ENABLE_ASSERTIONS
                    m_allocCount += size;
                #endif
                    return base;
                }
                else
                {
                    base += free + 1;
                }
            }

            return InvalidOffset;
        }

        void Free(uint32_t base, uint32_t size)
        {
            HE_ASSERT(base + size <= m_capacity);
            for (uint32_t i = 0; i < size; ++i)
                Clear(base + i);
        #if HE_ENABLE_ASSERTIONS
            m_allocCount -= size;
        #endif
        }

    private:
        uint32_t Test(uint32_t i)
        {
            return m_bitmap[i >> 3] & (1 << (i & 7));
        }

        void Set(uint32_t i)
        {
            m_bitmap[i >> 3] |= 1 << (i & 7);
        }

        void Clear(uint32_t i)
        {
            m_bitmap[i >> 3] &= ~(1 << (i & 7));
        }

        uint8_t* m_bitmap{ nullptr };
        uint32_t m_capacity{ 0 };
    #if HE_ENABLE_ASSERTIONS
        uint32_t m_allocCount{ 0 };
    #endif
    };
}

#endif
