// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/type_traits.h"

namespace std { struct random_access_iterator_tag; }

namespace he
{
    template <typename T, bool Const>
    class IndexIterator
    {
    public:
        using ContainerType     = T;
        using ContainerPtrType  = Conditional<Const, const T*, T*>;
        using ElementType       = typename T::ElementType;

        using difference_type   = uint32_t;
        using value_type        = ElementType;
        using pointer           = Conditional<Const, const value_type*, value_type*>;
        using reference         = Conditional<Const, const value_type&, value_type&>;
        using iterator_category = std::random_access_iterator_tag;
        using _Unchecked_type   = IndexIterator; // Mark iterator as checked for msvc.

    public:
        IndexIterator() = default;
        IndexIterator(ContainerPtrType container, uint32_t index) : m_container(container), m_index(index) {}

        [[nodiscard]] auto operator*() const { return m_container[m_index]; }

        [[nodiscard]] auto operator->() const
        {
            using ReturnType = decltype(m_container.operator[](DeclVal<uint32_t>()));
            if constexpr (IsReference<ReturnType>)
                return &m_container[m_index];
            else
                return m_container[m_index];
        }

        IndexIterator& operator++() { ++m_index; return *this; }
        IndexIterator operator++(int) { IndexIterator x = *this; ++m_index; return x; }
        IndexIterator& operator--() { --m_index; return *this; }
        IndexIterator operator--(int) { IndexIterator x = *this; --m_index; return x; }

        bool operator==(const IndexIterator& x) const { return m_container == x.m_container && m_index == x.m_index; }
        bool operator!=(const IndexIterator& x) const { return m_container != x.m_container || m_index != x.m_index; }

        friend difference_type operator-(const IndexIterator& lhs, const IndexIterator& rhs)
        {
            HE_ASSERT(lhs.m_container == rhs.m_container);
            return lhs.m_index - rhs.m_index;
        }

    private:
        ContainerPtrType m_container{ nullptr };
        uint32_t m_index{ 0 };
    };
}
