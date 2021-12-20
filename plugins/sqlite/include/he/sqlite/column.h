// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/types.h"

struct sqlite3_stmt;

namespace he::sqlite
{
    class Column final
    {
    public:
        enum class Type
        {
            Int = 1,
            Float = 2,
            Text = 3,
            Blob = 4,
            Null = 5,
        };

    public:
        Column(sqlite3_stmt* stmt, int32_t index);

        Type GetType() const;

        bool IsNull() const;

        double GetDouble() const;
        int32_t GetInt() const;
        int64_t GetInt64() const;
        uint32_t GetUint() const;
        Span<const uint8_t> GetBlob() const;
        Span<const char> GetText() const;

        void ReadBlob(Span<uint8_t> blob) const;
        void ReadText(Span<char> text) const;

    private:
        sqlite3_stmt* m_stmt = nullptr;
        const int32_t m_index = 0;
    };
}
