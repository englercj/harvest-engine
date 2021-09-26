// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/buffer_writer.h"
#include "he/core/span.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/types.h"
#include "he/core/vector.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    struct Trivial { uint32_t a; };

    struct NonTrivial
    {
        int* p;
        NonTrivial() { p = new int; }
        ~NonTrivial() { delete p; }

        NonTrivial(const NonTrivial&) { p = new int; }
        NonTrivial& operator=(const NonTrivial&) { delete p; p = new int; return *this; }

        NonTrivial(NonTrivial&& x) { p = x.p; x.p = nullptr; }
        NonTrivial& operator=(NonTrivial&& x) { delete p; p = x.p; x.p = nullptr; return *this; }
    };

    struct VirtualDestructor { virtual ~VirtualDestructor() {} };

    struct CopyOnly
    {
        CopyOnly() = default;
        CopyOnly(const CopyOnly&) { copyConstructed = true; }
        CopyOnly& operator=(const CopyOnly&) { copyAssigned = true; return *this; }
        CopyOnly(CopyOnly&&) = delete;
        CopyOnly& operator=(CopyOnly&&) = delete;

        bool copyConstructed{ false };
        bool copyAssigned{ false };
    };

    struct MoveOnly
    {
        MoveOnly() = default;
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) { moveConstructed = true; }
        MoveOnly& operator=(MoveOnly&&) { moveAssigned = true; return *this; }

        bool moveConstructed{ false };
        bool moveAssigned{ false };
    };

    struct CopyAndMove
    {
        CopyAndMove() = default;
        CopyAndMove(const CopyAndMove&) { copyConstructed = true; }
        CopyAndMove& operator=(const CopyAndMove&) { copyAssigned = true; return *this; }
        CopyAndMove(CopyAndMove&&) { moveConstructed = true; }
        CopyAndMove& operator=(CopyAndMove&&) { moveAssigned = true; return *this; }

        bool copyConstructed{ false };
        bool copyAssigned{ false };
        bool moveConstructed{ false };
        bool moveAssigned{ false };
    };

    // --------------------------------------------------------------------------------------------
    class AnotherAllocator : public Allocator
    {
    public:
        void* Malloc(size_t size, size_t alignment = DefaultAlignment) override { return CrtAllocator::Get().Malloc(size, alignment); }
        void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) override { return CrtAllocator::Get().Realloc(ptr, newSize, alignment); }
        void Free(void* ptr) override { CrtAllocator::Get().Free(ptr); }
    };

    // --------------------------------------------------------------------------------------------
    class BufferWriterTestAttorney
    {
    public:
        static uint8_t* GetPtr(BufferWriter& b) { return b.m_data; }
        static BufferWriter::GrowthStrategy GetStrategy(const BufferWriter& b) { return b.m_strategy; }
        static float GetGrowth(const BufferWriter& b) { return b.m_growth; }
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

    // --------------------------------------------------------------------------------------------
    class VectorTestAttorney
    {
    public:
        template <typename T>
        static T* GetPtr(Vector<T>& v) { return v.m_data; }
    };
}
