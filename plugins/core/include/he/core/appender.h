// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    /// An output iterator that appends to a container. Useful for formatting to a container
    /// using fmt::format_to().
    template <typename T>
    class Appender
    {
    public:
        //using iterator_category = std::output_iterator_tag;
        using value_type        = void;
        using pointer           = void;
        using reference         = void;
        using container_type    = T;
        using difference_type   = ptrdiff_t;
        using _Unchecked_type   = Appender; // Mark iterator as checked.

        constexpr explicit Appender(T& container) noexcept
            : m_container(&container) {}

        constexpr Appender& operator=(const typename T::ElementType& v) { m_container->PushBack(v); return *this; }
        constexpr Appender& operator=(typename T::ElementType&& v) { m_container->PushBack(Move(v)); return *this; }

        [[nodiscard]] constexpr Appender& operator*() noexcept { return *this; }
        constexpr Appender& operator++() noexcept { return *this; }
        constexpr Appender operator++(int) noexcept { return *this; }

    protected:
        T* m_container;
    };
}
