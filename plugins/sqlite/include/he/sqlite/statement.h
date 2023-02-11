// Copyright Chad Engler

#pragma once

#include "he/core/enum_ops.h"
#include "he/core/span.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/sqlite/column_reader.h"

struct sqlite3;
struct sqlite3_stmt;

namespace he::sqlite
{
    enum class StepResult : uint8_t
    {
        Done,       ///< The statement has completed and no rows remain
        Row,        ///< The statement has a resulting row
        Error,      ///< The statement has encountered an error
    };

    enum class PrepareFlags : uint32_t
    {
        None = 0,
        Temporary = 1 << 0,     ///< This statement is expected to be short-lived.
        NoVTables = 1 << 1,     ///< Prevents the statement from using any virtual tables.
    };
    HE_ENUM_FLAGS(PrepareFlags);

    class Statement final
    {
    public:
        Statement() = default;
        ~Statement() noexcept;

        Statement(Statement&& o) noexcept;
        Statement& operator=(Statement&& o) noexcept;

        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;

        bool IsPrepared() const { return m_db != nullptr && m_stmt != nullptr; }

        bool Prepare(sqlite3* db, const char* query, PrepareFlags flags = PrepareFlags::None);
        bool Finalize();

        bool Reset() const;

        bool Bind(int32_t index, double value) const;
        bool Bind(int32_t index, int32_t value) const;
        bool Bind(int32_t index, int64_t value) const;
        bool Bind(int32_t index, uint32_t value) const;
        bool Bind(int32_t index, Span<const uint8_t> value) const;
        bool Bind(int32_t index, StringView value) const;
        bool Bind(int32_t index, const char* value) const;
        bool Bind(int32_t index, decltype(nullptr)) const;
        bool BindNull(int32_t index) const;

        bool Bind(const char* paramName, double value) const;
        bool Bind(const char* paramName, int32_t value) const;
        bool Bind(const char* paramName, int64_t value) const;
        bool Bind(const char* paramName, uint32_t value) const;
        bool Bind(const char* paramName, Span<const uint8_t> value) const;
        bool Bind(const char* paramName, StringView value) const;
        bool Bind(const char* paramName, const char* value) const;
        bool Bind(const char* paramName, decltype(nullptr)) const;
        bool BindNull(const char* paramName) const;

        StepResult Step() const;

        template <typename F>
        bool EachRow(F&& func) const;

        ColumnReader GetColumn(int32_t index) const;

    private:
        sqlite3* m_db{ nullptr };
        sqlite3_stmt* m_stmt{ nullptr };
    };

    class ScopedStatement final
    {
    public:
        ScopedStatement(const Statement& stmt) noexcept : m_stmt(stmt) {}
        ~ScopedStatement() noexcept { m_stmt.Reset(); }

        ScopedStatement(const ScopedStatement&) = delete;
        ScopedStatement(ScopedStatement&&) = delete;

        ScopedStatement& operator=(const ScopedStatement&) = delete;
        ScopedStatement& operator=(ScopedStatement&&) = delete;

        const Statement& operator*() const { return m_stmt; }
        const Statement* operator->() const { return &m_stmt; }

    private:
        const Statement& m_stmt;
    };

    template <typename F>
    bool Statement::EachRow(F&& func) const
    {
        StepResult r;
        while ((r = Step()) == StepResult::Row)
            func(*this);
        return r == StepResult::Done;
    }
}
