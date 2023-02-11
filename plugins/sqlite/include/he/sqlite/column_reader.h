// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

struct sqlite3_stmt;

namespace he::sqlite
{
    enum class ColumnType : int
    {
        Int = 1,    // SQLITE_INTEGER (1)
        Float = 2,  // SQLITE_FLOAT (2)
        Text = 3,   // SQLITE_TEXT (3)
        Blob = 4,   // SQLITE_BLOB (4)
        Null = 5,   // SQLITE_NULL (5)
    };

    class ColumnReader final
    {
    public:
        ColumnReader(sqlite3_stmt* stmt, int32_t index) noexcept;

        ColumnType GetType() const;

        bool IsNull() const;

        double AsDouble() const;
        int32_t AsInt() const;
        int64_t AsInt64() const;
        uint32_t AsUint() const;
        Span<const uint8_t> AsBlob() const;
        StringView AsText() const;

        void ReadBlob(Span<uint8_t> blob) const;
        void ReadText(Span<char> text) const;

    private:
        sqlite3_stmt* m_stmt{ nullptr };
        const int32_t m_index{ 0 };
    };
}
