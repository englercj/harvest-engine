// Copyright Chad Engler

#include "he/core/string_pool.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/string_ops.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    StringPool::Iterator::Iterator(const Block* strings) noexcept
        : m_strings(strings)
    {
        // go to first element, or end if the pool is empty
        Next();
    }

    const char* StringPool::Iterator::String() const
    {
        if (m_id == 0)
            return {};

        const void* page = m_strings->GetPage(m_page);
        return reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(page) + m_offset);
    }

    void StringPool::Iterator::Next()
    {
        if (!m_strings)
            return;

        if (m_id > 0)
        {
            const char* value = String();
            m_offset += StrLen(value) + 1;
            if (m_offset >= m_strings->GetOffset(m_page))
            {
                m_page++;
                m_offset = 0;
            }
        }

        if (m_page >= m_strings->Size() || m_offset >= m_strings->GetOffset(m_page))
        {
            m_strings = nullptr;
            m_id = 0;
        }
        else
        {
            ++m_id;
        }
    }

    // --------------------------------------------------------------------------------------------
    StringPool::Block::Block(Allocator& allocator, uint32_t pageSize) noexcept
        : m_allocator(allocator)
        , m_pageSize(pageSize)
    {
        Grow(1);
        AddPage();
    }

    StringPool::Block::~Block() noexcept
    {
        for (uint32_t i = 0; i < m_size; ++i)
        {
            m_allocator.Free(m_pages[i]);
        }

        m_allocator.Free(m_pages);
        m_allocator.Free(m_offsets);
    }

    void* StringPool::Block::Alloc(uint32_t size)
    {
        HE_ASSERT(m_size > 0);
        if (!HE_VERIFY(size <= m_pageSize,
            HE_MSG("Strings can only be interned if they are smaller than the pool's page size"),
            HE_KV(string_size, size),
            HE_KV(page_size, m_pageSize)))
        {
            return nullptr;
        }

        uint32_t offset = m_offsets[m_size - 1];
        void* page = nullptr;

        if (m_pageSize - offset < size)
        {
            page = AddPage();
            offset = 0;
        }
        else
        {
            page = m_pages[m_size - 1];
        }

        m_offsets[m_size - 1] += size;
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(page) + offset);
    }

    uint32_t StringPool::Block::AllocatedBytes() const
    {
        constexpr uint32_t EntrySize = sizeof(uint32_t) + sizeof(void*);
        return (m_size * m_pageSize) + (m_capacity * EntrySize);
    }

    void* StringPool::Block::AddPage()
    {
        if (m_size == m_capacity)
        {
            const uint32_t newCapacity = m_capacity * 2;
            Grow(newCapacity);
        }

        void* page = m_allocator.Malloc(m_pageSize);

        m_pages[m_size] = page;
        m_offsets[m_size] = 0;
        ++m_size;
        return page;
    }

    void StringPool::Block::Grow(uint32_t newCapacity)
    {
        m_pages = static_cast<void**>(m_allocator.Realloc(m_pages, sizeof(void*) * newCapacity));
        m_offsets = static_cast<uint32_t*>(m_allocator.Realloc(m_offsets, sizeof(uint32_t) * newCapacity));
        m_capacity = newCapacity;
    }

    // --------------------------------------------------------------------------------------------
    alignas(StringPool) static uint8_t s_defaultStringPoolMem[sizeof(StringPool)];
    StringPool& StringPool::GetDefault()
    {
        static StringPool* s_pool = ::new(s_defaultStringPoolMem) StringPool();
        return *s_pool;
    }

    StringPool::StringPool(Allocator& allocator, uint32_t pageSize) noexcept
        : m_allocator(allocator)
        , m_lock()
        , m_pageSize(pageSize)
        , m_total(0)
        , m_hashes(allocator, pageSize)
        , m_strings(allocator, pageSize)
        , m_index(allocator, pageSize)
        , m_map()
    {
    }

    StringPoolId StringPool::Add(const char* str)
    {
        if (StrEmpty(str))
            return {};

        const uint32_t hash = Hash(str);

        LockGuard lock(m_lock);

        Entry* entry = m_map.Find(hash);

        // No existing entry, create a new one and insert it
        if (!entry)
        {
            entry = AllocEntry(hash, { str, StrLen(str) });

            if (!entry) [[unlikely]]
                return {};

            m_map.Insert(entry);
            return { entry->id };
        }

        // There is an existing entry, we need to check for a hash collision.
        // In the unlikely case that the hash collides, each entry has a linked-list.
        while (!StrEqual(str, entry->value)) [[unlikely]]
        {
            if (!entry->next) [[likely]]
            {
                entry->next = AllocEntry(hash, { str, StrLen(str) });
                entry = entry->next;

                if (!entry) [[unlikely]]
                    return {};

                break;
            }

            entry = entry->next;
        }

        return { entry->id };
    }

    StringPoolId StringPool::Add(StringView str)
    {
        if (str.IsEmpty())
            return {};

        const uint32_t hash = Hash(str);

        LockGuard lock(m_lock);

        Entry* entry = m_map.Find(hash);

        // No existing entry, create a new one and insert it
        if (!entry)
        {
            entry = AllocEntry(hash, str);

            if (!entry) [[unlikely]]
                return {};

            m_map.Insert(entry);
            return { entry->id };
        }

        // There is an existing entry, we need to check for a hash collision.
        // In the unlikely case that the hash collides, each entry has a linked-list.
        while (str != entry->value) [[unlikely]]
        {
            if (!entry->next) [[likely]]
            {
                entry->next = AllocEntry(hash, str);
                entry = entry->next;

                if (!entry) [[unlikely]]
                    return {};

                break;
            }

            entry = entry->next;
        }

        return { entry->id };
    }

    StringPoolId StringPool::Find(const char* str) const
    {
        const uint32_t hash = Hash(str);

        ReadLockGuard lock(m_lock);

        const Entry* entry = m_map.Find(hash);
        while (entry)
        {
            if (StrEqual(str, entry->value)) [[likely]]
                return { entry->id };

            entry = entry->next;
        }

        return {};
    }

    StringPoolId StringPool::Find(StringView str) const
    {
        const uint32_t hash = Hash(str);

        ReadLockGuard lock(m_lock);

        const Entry* entry = m_map.Find(hash);
        while (entry)
        {
            if (str == entry->value) [[likely]]
                return { entry->id };

            entry = entry->next;
        }

        return {};
    }

    const char* StringPool::Get(StringPoolId id) const
    {
        ReadLockGuard lock(m_lock);

        if (id.val == 0 || id.val > m_total) [[unlikely]]
            return nullptr;

        const uint32_t hashesPerPage = m_pageSize / sizeof(uint32_t);
        const uint32_t offset = (id.val - 1);
        const uint32_t pageIndex = offset / hashesPerPage;
        const uint32_t pageOffset = offset % hashesPerPage;
        const uintptr_t page = reinterpret_cast<uintptr_t>(m_hashes.GetPage(pageIndex));
        const uint32_t hash = *reinterpret_cast<const uint32_t*>(page + (pageOffset * sizeof(uint32_t)));

        const Entry* entry = m_map.Find(hash);
        while (entry)
        {
            if (entry->id == id.val) [[likely]]
                return entry->value;

            entry = entry->next;
        }

        return nullptr;
    }

    uint32_t StringPool::AllocatedBytes() const
    {
        return m_strings.AllocatedBytes() + m_hashes.AllocatedBytes() + m_index.AllocatedBytes();
    }

    uint32_t StringPool::Hash(const char* str)
    {
        uint32_t hash = 5381;
        while (const char c = *str++)
        {
            hash = ((hash << 5) + hash) + (uint32_t)c;
        }
        return hash;
    }

    uint32_t StringPool::Hash(StringView str)
    {
        uint32_t hash = 5381;
        for (const char c : str)
        {
            hash = ((hash << 5) + hash) + (uint32_t)c;
        }
        return hash;
    }

    StringPool::Entry* StringPool::AllocEntry(uint32_t hash, StringView str)
    {
        Entry* entry = m_index.Alloc<Entry>();
        if (!entry) [[unlikely]]
            return nullptr;

        entry->hash = hash;
        entry->id = m_total + 1;
        entry->next = nullptr;

        // Check for overflow
        if (entry->id == 0) [[unlikely]]
            return nullptr;

        char* strMem = static_cast<char*>(m_strings.Alloc(str.Size() + 1));
        if (!strMem) [[unlikely]]
            return nullptr;

        MemCopy(strMem, str.Data(), str.Size());
        strMem[str.Size()] = '\0';
        entry->value = strMem;

        uint32_t* hashMem = m_hashes.Alloc<uint32_t>();
        if (!hashMem) [[unlikely]]
            return nullptr;

        *hashMem = hash;
        ++m_total;
        return entry;
    }
}
