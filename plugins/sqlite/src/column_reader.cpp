// Copyright Chad Engler

#include "he/sqlite/column_reader.h"

#include "he/core/assert.h"

#include "sqlite3.h"

namespace he::sqlite
{
    ColumnReader::ColumnReader(sqlite3_stmt* stmt, int32_t index) noexcept
        : m_stmt(stmt)
        , m_index(index)
    {}

    ColumnType ColumnReader::GetType() const
    {
        return static_cast<ColumnType>(sqlite3_column_type(m_stmt, m_index));
    }

    bool ColumnReader::IsNull() const
    {
        return GetType() == ColumnType::Null;
    }

    double ColumnReader::AsDouble() const
    {
        const ColumnType t = GetType();
        if (!HE_VERIFY(t == ColumnType::Float || t == ColumnType::Null))
            return 0.0;

        return sqlite3_column_double(m_stmt, m_index);
    }

    int32_t ColumnReader::AsInt() const
    {
        const ColumnType t = GetType();
        if (!HE_VERIFY(t == ColumnType::Int || t == ColumnType::Null))
            return 0;

        return sqlite3_column_int(m_stmt, m_index);
    }

    int64_t ColumnReader::AsInt64() const
    {
        const ColumnType t = GetType();
        if (!HE_VERIFY(t == ColumnType::Int || t == ColumnType::Null))
            return 0;

        return sqlite3_column_int64(m_stmt, m_index);
    }

    uint32_t ColumnReader::AsUint() const
    {
        return static_cast<uint32_t>(AsInt64());
    }

    Span<const uint8_t> ColumnReader::AsBlob() const
    {
        const ColumnType t = GetType();
        if (!HE_VERIFY(t == ColumnType::Blob || t == ColumnType::Null))
            return { nullptr, 0 };

        const void* p = sqlite3_column_blob(m_stmt, m_index);
        const int32_t s = sqlite3_column_bytes(m_stmt, m_index);

        return { static_cast<const uint8_t*>(p), static_cast<uint32_t>(s) };
    }

    StringView ColumnReader::AsText() const
    {
        const ColumnType t = GetType();
        if (!HE_VERIFY(t == ColumnType::Text || t == ColumnType::Null))
            return { "" };

        const uint8_t* p = sqlite3_column_text(m_stmt, m_index);
        const int32_t s = sqlite3_column_bytes(m_stmt, m_index);

        return { reinterpret_cast<const char*>(p), static_cast<uint32_t>(s) };
    }

    void ColumnReader::ReadBlob(Span<uint8_t> blob) const
    {
        Span<const uint8_t> data = AsBlob();
        MemCopy(blob.Data(), data.Data(), Min(blob.Size(), data.Size()));
    }

    void ColumnReader::ReadText(Span<char> text) const
    {
        const StringView data = AsText();
        MemCopy(text.Data(), data.Data(), Min(text.Size(), data.Size()));
    }
}
