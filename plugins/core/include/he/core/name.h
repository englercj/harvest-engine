// Copyright Chad Engler

#pragma once

#include "he/core/string_pool.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he
{
    /// A small and fast to compare identifier. Names are interned in a global StringPool and
    /// store only the unique identifier of the string. They are best used to store commonly
    /// used string identifiers, which would otherwise be slow to copy around and compare.
    ///
    /// Names are immutable and cannot be changed once created, nor have their strings removed from
    /// the global StringPool they're stored in. They are also case-sensitive so "foo" and "Foo"
    /// are different names that will compare as unequal, and store two entries in the StringPool.
    ///
    /// \note The string pool identifier used for a Name is not stable across processes. If you
    /// want to store a Name in a file or database, you should use the String() method to get the
    /// string and store that instead.
    class Name
    {
    public:
        Name() = default;
        explicit Name(const char* str) : m_id(StringPool::GetDefault().Add(str)) {}
        explicit Name(StringView str) : m_id(StringPool::GetDefault().Add(str)) {}
        explicit Name(StringPoolId id) : m_id(id) {}

        [[nodiscard]] const char* String() const { return StringPool::GetDefault().Get(m_id); }
        [[nodiscard]] StringPoolId Id() const { return m_id; }

        explicit operator bool() const { return static_cast<bool>(m_id); }
        [[nodiscard]] bool operator==(const Name& x) const { return m_id == x.m_id; }
        [[nodiscard]] bool operator!=(const Name& x) const { return m_id != x.m_id; }
        [[nodiscard]] bool operator<(const Name& x) const { return m_id < x.m_id; }

        [[nodiscard]] uint64_t HashCode() const noexcept;

    private:
        StringPoolId m_id{};
    };
}
