// Copyright Chad Engler

#pragma once

#include "he/core/string.h"
#include "he/core/string_writer.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    /// Helper class to make building complex strings easier.
    /// String builder is basically a \ref StringWriter that owns the string it builds.
    class StringBuilder : public StringWriter
    {
    public:
        using ElementType = String::ElementType;

        /// Construct a builder using `allocator` for all string allocations.
        ///
        /// \param allocator The allocator to use.
        StringBuilder(Allocator& allocator = Allocator::GetDefault()) noexcept
            : StringWriter(m_buf)
            , m_buf(allocator)
        {}

        /// Construct a builder by copying `x`, using `allocator` for all allocations.
        ///
        /// \param x The writer to move from.
        StringBuilder(const StringBuilder& x, Allocator& allocator) noexcept
            : StringWriter(x)
            , m_buf(x.m_buf, allocator)
        {}

        /// Construct a builder by copying `x`, using `allocator` for all allocations.
        ///
        /// \param x The writer to move from.
        StringBuilder(StringBuilder&& x, Allocator& allocator) noexcept
            : StringWriter(Move(x))
            , m_buf(Move(x.m_buf), allocator)
        {}

        /// Construct a builder by copying `x`, using the allocator from `x`.
        ///
        /// \param x The writer to move from.
        StringBuilder(const StringBuilder& x) noexcept
            : StringWriter(x)
            , m_buf(x.m_buf)
        {}

        /// Construct a builder by moving `x`, using the allocator from `x`.
        ///
        /// \param x The builder to move from.
        StringBuilder(StringBuilder&& x) noexcept
            : StringWriter(Move(x))
            , m_buf(Move(x.m_buf))
        {}

        /// Copy the builder `x` into this builder.
        ///
        /// \param x The builder to copy from.
        StringBuilder& operator=(const StringBuilder& x) noexcept
        {
            StringWriter::operator=(x);
            m_buf = x.m_buf;
            return *this;
        }

        /// Move the builder `x` into this builder.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The builder to move from.
        StringBuilder& operator=(StringBuilder&& x) noexcept
        {
            StringWriter::operator=(Move(x));
            m_buf = Move(x.m_buf);
            return *this;
        }

        Allocator& GetAllocator() { return m_buf.GetAllocator(); }

    private:
        String m_buf;
    };
}
