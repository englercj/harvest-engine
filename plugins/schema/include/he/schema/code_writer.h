// Copyright Chad Engler

#pragma once

#include "he/core/buffer_writer.h"
#include "he/core/string_view.h"

#include "fmt/format.h"

namespace he::schema
{
    /// Helper class to make building code files easier. Acts as a wrapper around BufferWriter
    /// to provide some type casting helpers.
    class CodeWriter
    {
    public:
        /// Constructs a new writer.
        ///
        /// \param[in] allocator The allocator to use.
        CodeWriter(Allocator& allocator)
            : m_buffer(allocator)
        {}

        /// Sets the size of the buffer to zero.
        /// Does not affect memory allocation.
        void Clear() { m_buffer.Clear(); }

        /// Reserves capacity for `len` bytes.
        ///
        /// \param len The number of bytes to reserve capacity for.
        void Reserve(uint32_t len) { m_buffer.Reserve(len); }

        /// Copies a character into the buffer.
        ///
        /// \param[in] value The value to copy.
        void Write(char c) { m_buffer.Write(c); }

        /// Copies the null terminated string into the buffer. This will not copy the null
        /// terminator into the buffer.
        ///
        /// \param[in] str The null terminated string to copy.
        void Write(const char* str) { m_buffer.Write(str); }

        /// Copies the string view into the buffer.
        ///
        /// \param[in] str The string view to copy.
        void Write(StringView str) { m_buffer.Write(str.Data(), str.Size()); }

        /// Increases the indent level of the writer.
        void IncreaseIndent() { ++m_indentDepth; }

        /// Decreases the indent level of the writer.
        void DecreaseIndent() { --m_indentDepth; }

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
            fmt::memory_buffer buf;
            fmt::format_to(fmt::appender(buf), fmt, Forward<Args>(args)...);
            m_buffer.Write(buf.data(), static_cast<uint32_t>(buf.size()));
        }

        /// Returns a string view pointing to code buffer.
        ///
        /// \return A string view representing the code buffer.
        StringView Str() const
        {
            const char* data = reinterpret_cast<const char*>(m_buffer.Data());
            const uint32_t size = m_buffer.Size();
            return { data, size };
        }

        /// Returns a reference to the allocator object used by the buffer.
        ///
        /// \return The allocator object this buffer uses.
        Allocator& GetAllocator() { return m_buffer.GetAllocator(); }

    private:
        BufferWriter m_buffer;
        uint32_t m_indentDepth{ 0 };
    };
}
