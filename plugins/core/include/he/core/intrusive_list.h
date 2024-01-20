// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/utils.h"

namespace std { struct bidirectional_iterator_tag; }

namespace he
{
    /// A link that is used for intrusive list nodes.
    ///
    /// Example usage:
    /// ```cpp
    /// struct MyEntry
    /// {
    ///     uint32_t data;
    ///     IntrusiveListLink<MyEntry> link;
    /// };
    /// ```
    ///
    /// \tparam T The containing type this link tracks.
    template <typename T>
    struct IntrusiveListLink
    {
        T* prev;
        T* next;
    };

    /// An intrusive doubly linked list which tracks nodes that are allocated outside the container.
    /// Node structures are user-defined, but must have a \ref IntrusiveListLink member, a pointer to
    /// which is the second template parameter.
    ///
    /// Example usage:
    /// ```cpp
    /// struct MyEntry
    /// {
    ///     uint32_t data;
    ///     IntrusiveListLink<MyEntry> link;
    /// };
    /// IntrusiveList<MyEntry, &MyEntry::link> myList;
    /// ```
    ///
    /// \tparam T The node type for entries in the list.
    /// \tparam Link Pointer to member data for the link in the node type.
    template <typename T, IntrusiveListLink<T> T::*Link>
    class IntrusiveList final
    {
    public:
        using ElementType = T;
        using ListType = IntrusiveList<T, Link>;

    public:
        class Iterator
        {
        public:
            using ElementType = T;
            using ListType = IntrusiveList<T, Link>;

            using difference_type = uint32_t;
            using value_type = ElementType;
            using container_type = ListType;
            using iterator_category = std::bidirectional_iterator_tag;
            using _Unchecked_type = Iterator; // Mark iterator as checked.

        public:
            Iterator() = default;
            Iterator(const ListType* list, T* node) noexcept : m_list(list), m_node(node) {}

            T& operator*() const { return *m_node; }
            T* operator->() const { return m_node; }

            Iterator& operator++() { m_node = m_list->Next(m_node); return *this; }
            Iterator operator++(int) { Iterator x(m_list, m_node); m_node = m_list->Next(m_node); return x; }
            Iterator& operator--() { m_node = m_list->Prev(m_node); return *this; }
            Iterator operator--(int) { Iterator x(m_list, m_node); m_node = m_list->Prev(m_node); return x; }

            Iterator operator+(uint32_t n) const
            {
                Iterator x(m_list, m_node);
                for (uint32_t i = 0; i < n; ++i)
                    ++x;
                return x;
            }

            Iterator operator-(int n) const
            {
                Iterator x(m_list, m_node);
                for (int i = 0; i < n; ++i)
                    --x;
                return x;
            }

            [[nodiscard]] bool operator==(const Iterator& x) const { return m_list == x.m_list && m_node == x.m_node; }
            [[nodiscard]] bool operator!=(const Iterator& x) const { return m_list != x.m_list || m_node != x.m_node; }

            [[nodiscard]] explicit operator bool() const { return m_node != nullptr; }

        private:
            const ListType* m_list{ nullptr };
            T* m_node{ nullptr };
        };

    public:
        IntrusiveList() = default;
        IntrusiveList(const IntrusiveList&) = delete;
        IntrusiveList(IntrusiveList&& x)
            : m_head(Exchange(x.m_head, nullptr))
            , m_tail(Exchange(x.m_tail, nullptr))
            , m_size(Exchange(x.m_size, 0))
        {}

        IntrusiveList& operator=(const IntrusiveList& x) = delete;
        IntrusiveList& operator=(IntrusiveList&& x)
        {
            m_head = Exchange(x.m_head, nullptr);
            m_tail = Exchange(x.m_tail, nullptr);
            m_size = Exchange(x.m_size, 0);
        }

        bool IsEmpty() const { return m_size == 0; }
        uint32_t Size() const { return m_size; }

        void Clear()
        {
            if (m_size == 0)
                return;

            T* node = m_head;
            while (node)
            {
                T* next = node->*Link.next;
                node->*Link.prev = nullptr;
                node->*Link.next = nullptr;
                node = next;
            }

            m_head = nullptr;
            m_tail = nullptr;
            m_size = 0;
        }

        T* Front() const { return m_head; }
        T* Back() const { return m_tail; }
        T* Next(const T* node) const { return node ? node->*Link.next : nullptr; }
        T* Prev(const T* node) const { return node ? node->*Link.prev : nullptr; }

        void PushBack(T* node)
        {
            if (IsEmpty())
            {
                m_head = node;
                m_tail = node;
            }
            else
            {
                node->*Link.prev = m_tail;
                m_tail->*Link.next = node;
                m_tail = node;
            }
            ++m_size;
        }

        void PushFront(T* node)
        {
            if (IsEmpty())
            {
                m_head = node;
                m_tail = node;
            }
            else
            {
                node->*Link.next = m_head;
                m_head->*Link.prev = node;
                m_head = node;
            }
            ++m_size;
        }

        void Remove(T* node)
        {
            T* prev = node->*Link.prev;
            T* next = node->*Link.next;

            if (prev)
                prev->*Link.next = next;
            else
                m_head = next;

            if (next)
                next->*Link.prev = prev;
            else
                m_tail = prev;

            node->*Link.prev = nullptr;
            node->*Link.next = nullptr;

            --m_size;
        }

        Iterator begin() const { return Iterator(this, m_head); }
        Iterator end() const { return Iterator(this, nullptr); }

    private:
        T* m_head{ nullptr };
        T* m_tail{ nullptr };
        uint32_t m_size{ 0 };
    };
}
