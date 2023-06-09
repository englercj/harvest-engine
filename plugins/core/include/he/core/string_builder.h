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

        // ----------------------------------------------------------------------------------------
        // Construction

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
            : StringWriter(m_buf)
            , m_buf(x.m_buf, allocator)
        {
            m_indentDepth = x.m_indentDepth;
        }

        /// Construct a builder by copying `x`, using `allocator` for all allocations.
        ///
        /// \param x The writer to move from.
        StringBuilder(StringBuilder&& x, Allocator& allocator) noexcept
            : StringWriter(m_buf)
            , m_buf(Move(x.m_buf), allocator)
        {
            m_indentDepth = Exchange(x.m_indentDepth, 0);
        }

        /// Construct a builder by copying `x`, using the allocator from `x`.
        ///
        /// \param x The writer to move from.
        StringBuilder(const StringBuilder& x) noexcept
            : StringWriter(m_buf)
            , m_buf(x.m_buf)
        {
            m_indentDepth = x.m_indentDepth;
        }

        /// Construct a builder by moving `x`, using the allocator from `x`.
        ///
        /// \param x The builder to move from.
        StringBuilder(StringBuilder&& x) noexcept
            : StringWriter(m_buf)
            , m_buf(Move(x.m_buf))
        {
            m_indentDepth = Exchange(x.m_indentDepth, 0);
        }

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the builder `x` into this builder.
        ///
        /// \param x The builder to copy from.
        StringBuilder& operator=(const StringBuilder& x) noexcept
        {
            m_buf = x.m_buf;
            m_indentDepth = x.m_indentDepth;
            return *this;
        }

        /// Move the builder `x` into this builder.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The builder to move from.
        StringBuilder& operator=(StringBuilder&& x) noexcept
        {
            m_buf = Move(x.m_buf);
            m_indentDepth = Exchange(x.m_indentDepth, 0);
            return *this;
        }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Returns a reference to the allocator object used by the builder.
        ///
        /// \return The allocator object this builder uses.
        Allocator& GetAllocator() const { return m_buf.GetAllocator(); }

    private:
        String m_buf;
    };
}
