// Copyright Chad Engler

#pragma once

#include "he/core/appender.h"
#include "he/core/string.h"
#include "he/core/string_view.h"

#include "fmt/core.h"

namespace he
{
    /// Helper class to make building code files easier. Acts as a wrapper around BufferWriter
    /// to provide some type casting helpers.
    class StringBuilder
    {
    public:
        /// Constructs a new writer.
        ///
        /// \param[in] allocator The allocator to use.
        StringBuilder(Allocator& allocator = Allocator::GetDefault())
            : m_str(allocator)
        {}

        /// Sets the size of the buffer to zero.
        /// Does not affect memory allocation.
        void Clear() { m_str.Clear(); }

        /// Reserves capacity for `len` bytes.
        ///
        /// \param len The number of bytes to reserve capacity for.
        void Reserve(uint32_t len) { m_str.Reserve(len); }

        /// Copies a character into the buffer.
        ///
        /// \param[in] value The value to copy.
        void Write(char c) { m_str.PushBack(c); }

        /// Copies the null terminated string into the buffer. This will not copy the null
        /// terminator into the buffer.
        ///
        /// \param[in] str The null terminated string to copy.
        void Write(const char* str) { m_str.Append(str); }

        /// Copies the null terminated string into the buffer. This will not copy the null
        /// terminator into the buffer.
        ///
        /// \param[in] str The null terminated string to copy.
        void WriteLine(const char* str)
        {
            WriteIndent();
            m_str.Append(str);
            m_str.PushBack('\n');
        }

        /// Copies the string view into the buffer.
        ///
        /// \param[in] str The string view to copy.
        void Write(StringView str) { m_str.Append(str.Data(), str.Size()); }

        /// Increases the indent level of the writer.
        void IncreaseIndent() { HE_ASSERT(m_indentDepth < 0xffffffff); ++m_indentDepth; }

        /// Decreases the indent level of the writer.
        void DecreaseIndent() { HE_ASSERT(m_indentDepth > 0); --m_indentDepth; }

        /// Writes indentation to the code buffer.
        void WriteIndent()
        {
            constexpr StringView Indent = "    ";
            for (uint32_t i = 0; i < m_indentDepth; ++i)
            {
                Write(Indent);
            }
        }

        /// Formats the string and copies the result into the buffer.
        ///
        /// \param[in] fmt The string format specifier.
        /// \param[in] args The arguments to use in formatting.
        template <typename... Args>
        void Write(fmt::format_string<Args...> fmt, Args&&... args)
        {
            fmt::format_to(he::Appender(m_str), fmt, Forward<Args>(args)...);
        }

        /// Formats the string and copies the result into the buffer.
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

        /// Returns the underlying string of the builder.
        ///
        /// \return The string that has been built.
        const String& Str() const { return m_str; }

        /// Returns the underlying string of the builder.
        ///
        /// \return The string that has been built.
        String& Str() { return m_str; }

        /// Returns a reference to the allocator object used by the buffer.
        ///
        /// \return The allocator object this buffer uses.
        Allocator& GetAllocator() const { return m_str.GetAllocator(); }

    private:
        String m_str;
        uint32_t m_indentDepth{ 0 };
    };
}
