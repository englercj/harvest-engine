// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/compiler.h"
#include "he/core/rb_tree.h"
#include "he/core/string_view.h"
#include "he/core/sync.h"
#include "he/core/types.h"

HE_BEGIN_NAMESPACE_STD
struct forward_iterator_tag;
HE_END_NAMESPACE_STD

namespace he
{
    /// A unique identifier of a string that has been interned in a StringPool.
    struct StringPoolId final
    {
        uint32_t val{ 0 };

        constexpr explicit operator bool() const { return val != 0; }
        constexpr bool operator==(const StringPoolId& x) const { return val == x.val; }
        constexpr bool operator!=(const StringPoolId& x) const { return val != x.val; }
        constexpr bool operator<(const StringPoolId& x) const { return val < x.val; }
    };

    /// A threadsafe pool of interned strings.
    ///
    /// Strings interned in the pool are immutable and cannot be removed once added.
    class StringPool final
    {
    public:
        static constexpr uint32_t DefaultPageSize = 4096;

        static StringPool& GetDefault();

    private:
        class Block;

    public:
        class Iterator final
        {
        public:
            using ElementType = const char*;

            using difference_type = uint32_t;
            using value_type = ElementType;
            using container_type = StringPool;
            using iterator_category = std::forward_iterator_tag;
            using _Unchecked_type = Iterator; // Mark iterator as checked.

        public:
            Iterator() = default;
            Iterator(const Block* strings) noexcept;

            const char* String() const;
            StringPoolId Id() const { return { m_id }; }

            const char* operator*() const { return String(); }

            Iterator& operator++() { Next(); return *this; }
            Iterator operator++(int) { Iterator x(*this); Next(); return x; }

            [[nodiscard]] Iterator operator+(uint32_t n) const
            {
                Iterator x(*this);
                for (uint32_t i = 0; i < n; ++i)
                    x.Next();
                return x;
            }

            [[nodiscard]] bool operator==(const Iterator& x) const { return m_strings == x.m_strings && m_id == x.m_id; }
            [[nodiscard]] bool operator!=(const Iterator& x) const { return m_strings != x.m_strings || m_id != x.m_id; }

            [[nodiscard]] explicit operator bool() const { return m_strings != nullptr; }

        private:
            void Next();

        private:
            const Block* m_strings{ nullptr };
            uint32_t m_page{ 0 };
            uint32_t m_offset{ 0 };
            uint32_t m_id{ 0 };
        };

    public:
        StringPool(Allocator& allocator = Allocator::GetDefault(), uint32_t pageSize = DefaultPageSize) noexcept;

        StringPool(const StringPool&) = delete;
        StringPool(StringPool&&) = delete;

        StringPool& operator=(const StringPool&) = delete;
        StringPool& operator=(StringPool&&) = delete;

        [[nodiscard]] Allocator& GetAllocator() const { return m_allocator; }

        StringPoolId Add(const char* str);
        StringPoolId Add(StringView str);
        StringPoolId Find(const char* str) const;
        StringPoolId Find(StringView str) const;

        const char* Get(StringPoolId id) const;

        uint32_t PageSize() const { return m_pageSize; }
        uint32_t Size() const { return m_total; }

        uint32_t AllocatedBytes() const;

    public:
        Iterator begin() const { return Iterator(&m_strings); }
        Iterator end() const { return Iterator(); }

    private:
        class Block final
        {
        public:
            Block(Allocator& allocator, uint32_t pageSize) noexcept;
            ~Block() noexcept;

            void* Alloc(uint32_t size);
            template <typename T> T* Alloc() { return static_cast<T*>(Alloc(sizeof(T))); }

            uint32_t AllocatedBytes() const;

            void* GetPage(uint32_t index) const { return m_pages[index]; }
            uint32_t GetOffset(uint32_t page) const { return m_offsets[page]; }

            uint32_t Size() const { return m_size; }
            uint32_t Capacity() const { return m_capacity; }

        private:
            void* AddPage();
            void Grow(uint32_t newCapacity);

        private:
            Allocator& m_allocator;
            void** m_pages{ nullptr };
            uint32_t* m_offsets{ nullptr };
            uint32_t m_pageSize{ 0 };
            uint32_t m_size{ 0 };
            uint32_t m_capacity{ 0 };
        };

        struct Entry final
        {
            uint32_t hash;
            uint32_t id;
            const char* value;

            RBTreeLink<Entry> link;
            Entry* next;
        };

        using EntryMap = RBTree<Entry, &Entry::link, uint32_t, &Entry::hash>;

    private:
        static uint32_t Hash(const char* str);
        static uint32_t Hash(StringView str);

        Entry* AllocEntry(uint32_t hash, StringView str);

    private:
        Allocator& m_allocator;

        mutable RWLock m_lock;
        uint32_t m_pageSize;
        uint32_t m_total;

        Block m_hashes;
        Block m_strings;
        Block m_index;
        EntryMap m_map;
    };
}
