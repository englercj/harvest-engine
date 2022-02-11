// Copyright Chad Engler

#include "he/sqlite/column.h"

#include "he/core/assert.h"

#include "sqlite3.h"

namespace he::sqlite
{
    Column::Column(sqlite3_stmt* stmt, int32_t index)
        : m_stmt(stmt)
        , m_index(index)
    {}

    Column::Type Column::GetType() const
    {
        return Type(sqlite3_column_type(m_stmt, m_index));
    }

    bool Column::IsNull() const
    {
        return GetType() == Type::Null;
    }

    double Column::GetDouble() const
    {
        const Type t = GetType();
        if (!HE_VERIFY(t == Type::Float || t == Type::Null))
            return 0.0;

        return sqlite3_column_double(m_stmt, m_index);
    }

    int32_t Column::GetInt() const
    {
        const Type t = GetType();
        if (!HE_VERIFY(t == Type::Int || t == Type::Null))
            return 0;

        return sqlite3_column_int(m_stmt, m_index);
    }

    int64_t Column::GetInt64() const
    {
        const Type t = GetType();
        if (!HE_VERIFY(t == Type::Int || t == Type::Null))
            return 0;

        return sqlite3_column_int64(m_stmt, m_index);
    }

    uint32_t Column::GetUint() const
    {
        return static_cast<uint32_t>(GetInt64());
    }

    Span<const uint8_t> Column::GetBlob() const
    {
        const Type t = GetType();
        if (!HE_VERIFY(t == Type::Blob || t == Type::Null))
            return { nullptr, 0 };

        const void* p = sqlite3_column_blob(m_stmt, m_index);
        const int32_t s = sqlite3_column_bytes(m_stmt, m_index);

        return { static_cast<const uint8_t*>(p), static_cast<uint32_t>(s) };
    }

    Span<const char> Column::GetText() const
    {
        const Type t = GetType();
        if (!HE_VERIFY(t == Type::Text || t == Type::Null))
            return { "", 0 };

        const uint8_t* p = sqlite3_column_text(m_stmt, m_index);
        const int32_t s = sqlite3_column_bytes(m_stmt, m_index);

        return { reinterpret_cast<const char*>(p), static_cast<uint32_t>(s) };
    }

    void Column::ReadBlob(Span<uint8_t> blob) const
    {
        Span<const uint8_t> data = GetBlob();
        MemCopy(blob.Data(), data.Data(), Min(blob.Size(), data.Size()));
    }

    void Column::ReadText(Span<char> text) const
    {
        Span<const char> data = GetText();
        MemCopy(text.Data(), data.Data(), Min(text.Size(), data.Size()));
    }
}
