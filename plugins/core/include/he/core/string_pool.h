// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/string_view.h"
#include "he/core/sync.h"
#include "he/core/types.h"

namespace he
{
    /// A reference to a string in a StringPool.
    class StringPoolRef final
    {
    public:
        constexpr StringPoolRef() = default;
        constexpr StringPoolRef(uint32_t number, uint32_t index) noexcept : m_value(index < IndexMax ? ((number << IndexBits) | index) : InvalidValue) {}
        constexpr StringPoolRef(const StringPoolRef& x) noexcept : m_value(x.m_value) {}

        constexpr uint32_t Number() const { return m_value >> IndexBits; }
        constexpr uint32_t Index() const { return m_value & IndexMask; }
        constexpr bool IsValid() const { return m_value != InvalidValue; }

        constexpr explicit operator bool() const { return m_value != InvalidValue; }

        constexpr StringPoolRef& operator=(const StringPoolRef& x) noexcept { m_value = x.m_value; return *this; };
        constexpr bool operator==(const StringPoolRef& x) const { return m_value == x.m_value; }
        constexpr bool operator!=(const StringPoolRef& x) const { return m_value != x.m_value; }
        constexpr bool operator<(const StringPoolRef& x) const { return m_value < x.m_value; }

    private:
        static constexpr uint32_t IndexBits = 8;
        static constexpr uint32_t IndexMax = 1 << IndexBits;
        static constexpr uint32_t IndexMask = (1 << IndexBits) - 1;

        //static constexpr uint32_t NumberBits = sizeof(uint32_t) - IndexBits;
        //static constexpr uint32_t NumberMax = 1 << NumberBits;

        static constexpr uint32_t InvalidValue = static_cast<uint32_t>(-1);

    private:
        uint32_t m_value{ InvalidValue };
    };

    /// A thread safe, and lock free, pool of interned strings.
    ///
    /// Strings interned in the pool are immutable and cannot be removed once added.
    class StringPool final
    {
    public:
        static constexpr uint32_t PageSize = 2 * 1024 * 1024; // 2MB

        static StringPool& GetDefault();

    public:
        StringPool(Allocator& allocator = Allocator::GetDefault()) noexcept : m_allocator(allocator) {}
        ~StringPool() noexcept;

        StringPool(const StringPool&) = delete;
        StringPool(StringPool&&) = delete;

        StringPool& operator=(const StringPool&) = delete;
        StringPool& operator=(StringPool&&) = delete;

        StringPoolRef Add(StringView str);

        StringView Get(StringPoolRef ref) const;

        //uint32_t Size() const { return m_entryCount; }

    private:
        StringPoolRef Add(StringView str, uint64_t hash);

    private:
        Allocator& m_allocator;

        RWLock m_lock;
        class Node* m_head{ nullptr };
        uint32_t m_noItems{ 0 };
        uint32_t m_noBuckets{ 0 };
        uint32_t m_nextResize{ 0 };
        double m_maxLoadFactor{ 0.0 };
    };
}
