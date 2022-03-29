// Copyright Chad Engler

#pragma once

#include "he/core/appender.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/utils.h"

#include "fmt/core.h"

namespace he
{
    /// Helper class to make building complex strings easier.
    class StringBuilder final
    {
    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Constructs a new builder.
        ///
        /// \param[in] allocator Optional. The allocator to use.
        explicit StringBuilder(Allocator& allocator = Allocator::GetDefault())
            : m_str(allocator)
            , m_indentDepth(0)
        {}

        /// Construct a builder by copying `x`, and using `allocator` for this
        /// builder's allocations.
        ///
        /// \param x The builder to copy from.
        /// \param allocator The allocator to use for any allocations.
        StringBuilder(const StringBuilder& x, Allocator& allocator)
            : m_str(x.m_str, allocator)
            , m_indentDepth(x.m_indentDepth)
        {}

        /// Construct a builder by moving `x`, and using `allocator` for this builder
        /// writer's allocations. If the allocators do not match then a copy operation will
        /// be performed.
        ///
        /// \param x The builder to move from.
        /// \param allocator The allocator to use for any allocations.
        StringBuilder(StringBuilder&& x, Allocator& allocator)
            : m_str(Move(x.m_str), allocator)
            , m_indentDepth(Exchange(x.m_indentDepth, 0))
        {}

        /// Construct a builder by copying `x`, using the allocator from `x`.
        ///
        /// \param x The builder to copy from.
        StringBuilder(const StringBuilder& x)
            : m_str(x.m_str)
            , m_indentDepth(x.m_indentDepth)
        {}

        /// Construct a builder by moving `x`, using the allocator from `x`.
        ///
        /// \param x The builder to move from.
        StringBuilder(StringBuilder&& x)
            : m_str(Move(x.m_str))
            , m_indentDepth(Exchange(x.m_indentDepth, 0))
        {}

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the buffer `x` into this buffer.
        ///
        /// \param x The buffer to copy from.
        StringBuilder& operator=(const StringBuilder& x)
        {
            m_str = x.m_str;
            m_indentDepth = x.m_indentDepth;
            return *this;
        }

        /// Move the buffer `x` into this buffer.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The buffer to move from.
        StringBuilder& operator=(StringBuilder&& x)
        {
            m_str = Move(x.m_str);
            m_indentDepth = Exchange(x.m_indentDepth, 0);
            return *this;
        }

        /// Gets a reference to the element at `index`. Asserts if `index` is not less than
        /// \see Size().
        ///
        /// \param index The index of the element to return.
        /// \return A reference to the element at `index`.
        const char& operator[](uint32_t index) const { return m_str[index]; }

        /// \copydoc operator[](uint32_t)
        char& operator[](uint32_t index) { return m_str[index]; }

        /// Appends the null terminated string to the end of this builder.
        ///
        /// \param str The string to append.
        StringBuilder& operator+=(const char* str) { m_str += str; return *this; }

        /// Appends the character to the end of this builder.
        ///
        /// \param c The character to append.
        StringBuilder& operator+=(char c) { m_str += c; return *this; }

        /// Appends a series of characters from an object that provides a STL-style
        /// contiguous range. That is, it has `.data()` and `.size()` members.
        ///
        /// \param range The object that provides the range.
        template <typename R> requires(StdContiguousRange<R, const char>)
        StringBuilder& operator+=(const R& range) { m_str += range; return *this; }

        /// Appends a series of characters from an object that provides a Harvest-style
        /// contiguous range. That is, it has `.Data()` and `.Size()` members.
        ///
        /// \param range The object that provides the range.
        template <typename R> requires(ContiguousRange<R, const char>)
        StringBuilder& operator+=(const R& range) { m_str += range; return *this; }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this builder is empty.
        ///
        /// \return Returns true if this is an empty string.
        bool IsEmpty() const { return m_str.IsEmpty(); }

        /// The capacity the builder has for characters.
        ///
        /// Note: This is not the size of the allocation, but rather the number of total
        /// characters the builder can hold before having to reallocate.
        ///
        /// \return Number of total chracters this builder can store.
        uint32_t Capacity() const { return m_str.Capacity(); }

        /// The length of the string that is currently stored.
        ///
        /// \return Number of characters in the string, not including the null terminator.
        uint32_t Size() const { return m_str.Size(); }

        /// Reserves capacity for `len` bytes.
        ///
        /// \param len The number of bytes to reserve capacity for.
        void Reserve(uint32_t len) { m_str.Reserve(len); }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Returns the underlying string of the builder.
        ///
        /// \return The string that has been built.
        const String& Str() const { return m_str; }

        /// Returns the underlying string of the builder.
        ///
        /// \return The string that has been built.
        String& Str() { return m_str; }

        /// Returns a reference to the allocator object used by the builder.
        ///
        /// \return The allocator object this builder uses.
        Allocator& GetAllocator() const { return m_str.GetAllocator(); }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets the size of the builder to zero. Does not affect memory allocation.
        void Clear() { m_str.Clear(); }

        /// Decreases the indent level of the writer.
        void DecreaseIndent() { HE_ASSERT(m_indentDepth > 0); --m_indentDepth; }

        /// Increases the indent level of the writer.
        void IncreaseIndent() { HE_ASSERT(m_indentDepth < 0xffffffff); ++m_indentDepth; }

        /// Copies a character into the builder. This is the same as calling Write,
        /// but exists to support generic programming on containers.
        ///
        /// \param[in] value The value to copy.
        void PushBack(char c) { m_str.PushBack(c); }

        /// Copies a character into the builder.
        ///
        /// \param[in] value The value to copy.
        void Write(char c) { m_str.PushBack(c); }

        /// Copies the null terminated string into the builder. This will not copy the null
        /// terminator into the builder.
        ///
        /// \param[in] str The null terminated string to copy.
        void Write(const char* str) { m_str.Append(str); }

        /// Copies the string view into the builder.
        ///
        /// \param[in] str The string view to copy.
        void Write(StringView str) { m_str.Append(str.Data(), str.Size()); }

        /// Formats the string and copies the result into the builder.
        ///
        /// \param[in] fmt The string format specifier.
        /// \param[in] args The arguments to use in formatting.
        template <typename... Args>
        void Write(fmt::format_string<Args...> fmt, Args&&... args)
        {
            fmt::format_to(he::Appender(m_str), fmt, Forward<Args>(args)...);
        }

        /// Writes the current indentation to the code builder.
        void WriteIndent()
        {
            if (m_indentDepth > 0)
            {
                const uint32_t growth = m_indentDepth * 4;
                m_str.Resize(m_str.Size() + growth, ' ');
            }
        }

        /// Copies the null terminated string into the builder. This will not copy the null
        /// terminator into the builder.
        ///
        /// \param[in] str The null terminated string to copy.
        void WriteLine(const char* str)
        {
            WriteIndent();
            m_str.Append(str);
            m_str.PushBack('\n');
        }

        /// Formats the string and copies the result into the builder.
        ///
        /// \param[in] fmt The string format specifier.
        /// \param[in] args The arguments to use in formatting.
        template <typename... Args>
        void WriteLine(fmt::format_string<Args...> fmt, Args&&... args)
        {
            WriteIndent();
            fmt::format_to(he::Appender(m_str), fmt, Forward<Args>(args)...);
            m_str.PushBack('\n');
        }

    private:
        String m_str;
        uint32_t m_indentDepth;
    };
}
