// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/arena_allocator.h"
#include "he/core/buffer_writer.h"
#include "he/core/hash_table.h"
#include "he/core/span.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/core/virtual_memory.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    void TestAllocator(Allocator& alloc);
    void TouchTestFile(const char* path, const void* data = nullptr, uint32_t len = 0);
    StringView GetTestTomlDocument();

    // --------------------------------------------------------------------------------------------
    struct Trivial { uint32_t a; };

    struct NonTrivial
    {
        int* p;
        NonTrivial() noexcept { p = new int; }
        ~NonTrivial() noexcept { delete p; }

        NonTrivial(const NonTrivial&) noexcept { p = new int; }
        NonTrivial& operator=(const NonTrivial&) noexcept { delete p; p = new int; return *this; }

        NonTrivial(NonTrivial&& x) noexcept { p = x.p; x.p = nullptr; }
        NonTrivial& operator=(NonTrivial&& x) noexcept { delete p; p = x.p; x.p = nullptr; return *this; }
    };

    struct VirtualDestructor { virtual ~VirtualDestructor() {} };

    struct CopyOnly
    {
        CopyOnly() = default;
        CopyOnly(const CopyOnly&) noexcept { copyConstructed = true; }
        CopyOnly& operator=(const CopyOnly&) noexcept { copyAssigned = true; return *this; }
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
        MoveOnly(MoveOnly&&) noexcept { moveConstructed = true; }
        MoveOnly& operator=(MoveOnly&&) noexcept { moveAssigned = true; return *this; }

        bool moveConstructed{ false };
        bool moveAssigned{ false };
    };

    struct CopyAndMove
    {
        CopyAndMove() = default;
        CopyAndMove(const CopyAndMove&) noexcept { copyConstructed = true; }
        CopyAndMove& operator=(const CopyAndMove&) noexcept { copyAssigned = true; return *this; }
        CopyAndMove(CopyAndMove&&) noexcept { moveConstructed = true; }
        CopyAndMove& operator=(CopyAndMove&&) noexcept { moveAssigned = true; return *this; }

        bool copyConstructed{ false };
        bool copyAssigned{ false };
        bool moveConstructed{ false };
        bool moveAssigned{ false };
    };

    // --------------------------------------------------------------------------------------------
    class AnotherAllocator : public Allocator
    {
    public:
        void* Malloc(size_t size, size_t alignment = DefaultAlignment) noexcept override { return Allocator::GetDefault().Malloc(size, alignment); }
        void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) noexcept override { return Allocator::GetDefault().Realloc(ptr, newSize, alignment); }
        void Free(void* ptr) noexcept override { Allocator::GetDefault().Free(ptr); }
    };

    // --------------------------------------------------------------------------------------------
    class ArenaAllocatorTestAttorney
    {
    public:
        static VirtualMemory& GetVM(ArenaAllocator& a) { return a.m_arena; }
        static uint32_t GetCommittedPages(ArenaAllocator& a) { return a.m_committedPages; }
        static size_t GetAllocatedBytes(ArenaAllocator& a) { return a.m_allocatedBytes; }
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
    class HashTableTestAttorney
    {
    public:
        template <typename T>
        static Vector<typename T::EntryType>& GetVector(HashTable<T>& v) { return v.m_entries; }
    };

    // --------------------------------------------------------------------------------------------
    class SpanTestAttorney
    {
    public:
        template <typename T>
        static void Test(const Span<T>& s, const void* expectedPtr, uint32_t expectedSize)
        {
            HE_EXPECT(s.m_data == expectedPtr);
            HE_EXPECT_EQ(s.m_size, expectedSize);
        }
    };

    // --------------------------------------------------------------------------------------------
    class StringViewTestAttorney
    {
    public:
        static void Test(const StringView& s, const void* expectedPtr, uint32_t expectedSize)
        {
            HE_EXPECT(s.m_data == expectedPtr);
            HE_EXPECT_EQ(s.m_size, expectedSize);
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
