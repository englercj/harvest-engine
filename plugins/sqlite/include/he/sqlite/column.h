// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

struct sqlite3_stmt;

namespace he::sqlite
{
    class Column final
    {
    public:
        enum class Type
        {
            Int = 1,    // SQLITE_INTEGER (1)
            Float = 2,  // SQLITE_FLOAT (2)
            Text = 3,   // SQLITE_TEXT (3)
            Blob = 4,   // SQLITE_BLOB (4)
            Null = 5,   // SQLITE_NULL (5)
        };

    public:
        Column(sqlite3_stmt* stmt, int32_t index) noexcept;

        Type GetType() const;

        bool IsNull() const;

        double GetDouble() const;
        int32_t GetInt() const;
        int64_t GetInt64() const;
        uint32_t GetUint() const;
        Span<const uint8_t> GetBlob() const;
        StringView GetText() const;

        void ReadBlob(Span<uint8_t> blob) const;
        void ReadText(Span<char> text) const;

    private:
        sqlite3_stmt* m_stmt{ nullptr };
        const int32_t m_index{ 0 };
    };
}
