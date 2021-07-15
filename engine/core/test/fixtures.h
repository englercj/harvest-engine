// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/span.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/types.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    struct Trivial { uint32_t a; };

    struct NonTrivial
    {
        int* p;
        NonTrivial() { p = new int; }
        ~NonTrivial() { delete p; }
    };

    struct VirtualDestructor { virtual ~VirtualDestructor() {} };

    // --------------------------------------------------------------------------------------------
    class AnotherAllocator : public Allocator
    {
    public:
        void* Malloc(size_t size, size_t alignment = DefaultAlignment) override { return CrtAllocator::Get().Malloc(size, alignment); }
        void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) override { return CrtAllocator::Get().Realloc(ptr, newSize, alignment); }
        void Free(void* ptr) override { CrtAllocator::Get().Free(ptr); }
    };

    // --------------------------------------------------------------------------------------------
    class SpanTestAttorney
    {
    public:
        template <typename T>
        static void Test(const Span<T>& s, const void* expectedPtr, uint32_t expectedSize)
        {
            HE_EXPECT(s.m_ptr == expectedPtr);
            HE_EXPECT_EQ(s.m_size, expectedSize);
        }
    };

    // --------------------------------------------------------------------------------------------
    class StringViewTestAttorney
    {
    public:
        static void Test(const StringView& s, const void* expectedPtr, uint32_t expectedSize)
        {
            SpanTestAttorney::Test(s.m_span, expectedPtr, expectedSize);
        }
    };

    // --------------------------------------------------------------------------------------------
    class StringTestAttorney
    {
    public:
        using Heap = String::Heap;

        static char* GetEmbed(String& s) { return s.m_embed; }
        static Heap& GetHeap(String& s) { return s.m_heap; }
    };
}
