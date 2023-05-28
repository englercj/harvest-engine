// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/fmt.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    /// Helper class to make writing complex strings easier.
    class StringWriter
    {
    public:
        using ElementType = String::ElementType;

        // ----------------------------------------------------------------------------------------
        // Construction

        /// Constructs a new writer.
        ///
        /// \param[in] allocator Optional. The allocator to use.
        explicit StringWriter(String& dst) noexcept
            : m_str(&dst)
            , m_indentDepth(0)
        {}

        /// Construct a writer by copying `x`.
        ///
        /// \param x The writer to copy from.
        StringWriter(const StringWriter& x) noexcept
            : m_str(x.m_str)
            , m_indentDepth(x.m_indentDepth)
        {}

        /// Construct a writer by moving `x`.
        ///
        /// \param x The writer to move from.
        StringWriter(StringWriter&& x) noexcept
            : m_str(Exchange(x.m_str, nullptr))
            , m_indentDepth(Exchange(x.m_indentDepth, 0))
        {}

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the writer `x` into this writer.
        ///
        /// \param x The writer to copy from.
        StringWriter& operator=(const StringWriter& x) noexcept
        {
            m_str = x.m_str;
            m_indentDepth = x.m_indentDepth;
            return *this;
        }

        /// Move the writer `x` into this writer.
        ///
        /// \param x The writer to move from.
        StringWriter& operator=(StringWriter&& x) noexcept
        {
            m_str = Exchange(x.m_str, nullptr);
            m_indentDepth = Exchange(x.m_indentDepth, 0);
            return *this;
        }

        /// Gets a reference to the element at `index`. Asserts if `index` is not less than
        /// \see Size().
        ///
        /// \param index The index of the element to return.
        /// \return A reference to the element at `index`.
        const char& operator[](uint32_t index) const { return (*m_str)[index]; }

        /// \copydoc operator[](uint32_t)
        char& operator[](uint32_t index) { return (*m_str)[index]; }

        /// Appends the null terminated string to the end of this writer.
        ///
        /// \param str The string to append.
        StringWriter& operator+=(const char* str) { *m_str += str; return *this; }

        /// Appends the character to the end of this writer.
        ///
        /// \param c The character to append.
        StringWriter& operator+=(char c) { *m_str += c; return *this; }

        /// Appends a series of characters from an object that provides a Harvest-style
        /// contiguous range. That is, it has `.Data()` and `.Size()` members.
        ///
        /// \param range The object that provides the range.
        template <typename R> requires(ContiguousRange<R, const char>)
        StringWriter& operator+=(const R& range) { *m_str += range; return *this; }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this writer is empty.
        ///
        /// \return Returns true if this is an empty string.
        bool IsEmpty() const { return m_str == nullptr || m_str->IsEmpty(); }

        /// The capacity the writer has for characters.
        ///
        /// Note: This is not the size of the allocation, but rather the number of total
        /// characters the writer can hold before having to reallocate.
        ///
        /// \return Number of total chracters this writer can store.
        uint32_t Capacity() const { return m_str ? m_str->Capacity() : 0; }

        /// The length of the string that is currently stored.
        ///
        /// \return Number of characters in the string, not including the null terminator.
        uint32_t Size() const { return m_str ? m_str->Size() : 0; }

        /// Reserves capacity for `len` bytes.
        ///
        /// \param len The number of bytes to reserve capacity for.
        void Reserve(uint32_t len) { m_str->Reserve(len); }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Returns the underlying string of the writer.
        ///
        /// \return The string that has been built.
        const String& Str() const { return *m_str; }

        /// Returns the underlying string of the writer.
        ///
        /// \return The string that has been built.
        String& Str() { return *m_str; }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets the size of the writer to zero. Does not affect memory allocation.
        void Clear() { m_str->Clear(); }

        /// Decreases the indent level of the writer.
        void DecreaseIndent() { HE_ASSERT(m_indentDepth > 0); --m_indentDepth; }

        /// Increases the indent level of the writer.
        void IncreaseIndent() { HE_ASSERT(m_indentDepth < 0xffffffff); ++m_indentDepth; }

        /// Copies a character into the writer. This is the same as calling Write,
        /// but exists to support generic programming on containers.
        ///
        /// \param[in] value The value to copy.
        void PushBack(char c) { m_str->PushBack(c); }

        /// Copies a character into the writer.
        ///
        /// \param[in] value The value to copy.
        void Write(char c) { m_str->PushBack(c); }

        /// Copies the null terminated string into the writer. This will not copy the null
        /// terminator into the writer.
        ///
        /// \param[in] str The null terminated string to copy.
        void Write(const char* str) { m_str->Append(str); }

        /// Copies the string view into the writer.
        ///
        /// \param[in] str The string view to copy.
        void Write(StringView str) { m_str->Append(str.Data(), str.Size()); }

        /// Formats the string and copies the result into the writer.
        ///
        /// \param[in] fmt The string format specifier.
        /// \param[in] args The arguments to use in formatting.
        template <typename... Args>
        void Write(FmtString<Args...> fmt, Args&&... args)
        {
            FormatTo(*m_str, fmt, Forward<Args>(args)...);
        }

        /// Writes the current indentation to the code writer.
        void WriteIndent()
        {
            if (m_indentDepth > 0)
            {
                const uint32_t growth = m_indentDepth * 4;
                m_str->Resize(m_str->Size() + growth, ' ');
            }
        }

        /// Copies the null terminated string into the writer. This will not copy the null
        /// terminator into the writer.
        ///
        /// \param[in] str The null terminated string to copy.
        void WriteLine(const char* str)
        {
            WriteIndent();
            m_str->Append(str);
            m_str->PushBack('\n');
        }

        /// Formats the string and copies the result into the writer.
        ///
        /// \param[in] fmt The string format specifier.
        /// \param[in] args The arguments to use in formatting.
        template <typename... Args>
        void WriteLine(FmtString<Args...> fmt, Args&&... args)
        {
            WriteIndent();
            FormatTo(*m_str, fmt, Forward<Args>(args)...);
            m_str->PushBack('\n');
        }

    protected:
        String* m_str{ nullptr };
        uint32_t m_indentDepth{ 0 };
    };
}
