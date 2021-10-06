// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

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

        explicit Appender(T& container) noexcept
            : m_container(&container) {}

        Appender& operator=(char c) { m_container->PushBack(c); return *this; }
        [[nodiscard]] constexpr Appender& operator*() noexcept { return *this; }

        constexpr Appender& operator++() noexcept { return *this; }
        constexpr Appender operator++(int) noexcept { return *this; }

    private:
        T* m_container;
    };
}
