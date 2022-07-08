// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

#include <cstddef>

namespace he
{
    struct ListLink
    {
        ListLink* next;
        ListLink* prev;
    };

    template <auto>
    class List;

    template <typename T, ListLink T::*LinkMemberPtr>
    class List<link>
    {
    public:
        using ElementType = T;
        using LinkMember = LinkMemberPtr;

    public:
        class Iterator
        {

        };

    public:
        bool IsEmpty() const { return m_size == 0; }
        void Clear() { m_head = m_tail = nullptr; m_size = 0; }

        uint32_t Size() const { return m_size; }

        T* Front() const { return ToObj(m_head); }
        T* Back() const { return ToObj(m_tail); }
        T* Next(const T* node) const { return node ? ToObj((node->*LinkMember).next) : nullptr; }
        T* Previous(const T* node) const { return node ? ToObj(node->link.prev) : nullptr; }

        Iterator begin() const { return AstListIterator<T>(this, m_head); }
        Iterator end() const { return AstListIterator<T>(this, nullptr); }

    private:
        static inline constexpr T* ToObj(ListLink* link) const
        {
            return link ? reinterpret_cast<T*>(reinterpret_cast<char*>(link) - offsetof(T, LinkMember)) : nullptr;
        }

    private:
        T* m_head{ nullptr };
        T* m_tail{ nullptr };
        uint32_t m_size{ 0 };
    };
}
