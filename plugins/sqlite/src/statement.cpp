// Copyright Chad Engler

#include "he/sqlite/statement.h"

#include "sqlite_internal.h"

#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/scope_guard.h"
#include "he/core/utils.h"

#include "sqlite3.h"

namespace he::sqlite
{
    Statement::~Statement()
    {
        Finalize();
    }

    Statement::Statement(Statement&& o)
        : m_db(Exchange(o.m_db, nullptr))
        , m_stmt(Exchange(o.m_stmt, nullptr))
    { }

    Statement& Statement::operator=(Statement&& o)
    {
        m_db = Exchange(o.m_db, nullptr);
        m_stmt = Exchange(o.m_stmt, nullptr);
        return *this;
    }

    bool Statement::Prepare(sqlite3* db, const char* query, PrepareFlags flags)
    {
        HE_ASSERT(m_stmt == nullptr && m_db == nullptr);
        m_db = db;

        uint32_t flagValues = 0;
        if (HasFlag(flags, PrepareFlags::Temporary))
            flagValues |= SQLITE_PREPARE_PERSISTENT;
        if (!HasFlag(flags, PrepareFlags::NoVTables))
            flagValues |= SQLITE_PREPARE_NO_VTAB;

        HE_SQLITE_OK(sqlite3_prepare_v3(m_db, query, -1, flagValues, &m_stmt, nullptr));
        return true;
    }

    bool Statement::Finalize()
    {
        if (m_stmt)
        {
            HE_AT_SCOPE_EXIT([&]()
            {
                m_stmt = nullptr;
                m_db = nullptr;
            });
            HE_SQLITE_OK(sqlite3_finalize(m_stmt));
        }

        return true;
    }

    bool Statement::Reset() const
    {
        HE_SQLITE_OK(sqlite3_reset(m_stmt));
        return true;
    }

    bool Statement::Bind(int32_t index, double value) const
    {
        HE_SQLITE_OK(sqlite3_bind_double(m_stmt, index, value));
        return true;
    }

    bool Statement::Bind(int32_t index, int32_t value) const
    {
        HE_SQLITE_OK(sqlite3_bind_int(m_stmt, index, value));
        return true;
    }

    bool Statement::Bind(int32_t index, int64_t value) const
    {
        HE_SQLITE_OK(sqlite3_bind_int64(m_stmt, index, value));
        return true;
    }

    bool Statement::Bind(int32_t index, uint32_t value) const
    {
        return Bind(index, static_cast<int64_t>(value));
    }

    bool Statement::Bind(int32_t index, Span<const uint8_t> value) const
    {
        HE_SQLITE_OK(sqlite3_bind_blob(m_stmt, index, value.Data(), static_cast<int32_t>(value.Size()), nullptr));
        return true;
    }

    bool Statement::Bind(int32_t index, Span<const char> value) const
    {
        HE_SQLITE_OK(sqlite3_bind_text(m_stmt, index, value.Data(), static_cast<int32_t>(value.Size()), nullptr));
        return true;
    }

    bool Statement::Bind(int32_t index, const char* value) const
    {
        HE_SQLITE_OK(sqlite3_bind_text(m_stmt, index, value, -1, nullptr));
        return true;
    }

    bool Statement::Bind(int32_t index, decltype(nullptr)) const
    {
        HE_SQLITE_OK(sqlite3_bind_null(m_stmt, index));
        return true;
    }

    bool Statement::BindNull(int32_t index) const
    {
        return Bind(index, nullptr);
    }

    bool Statement::Bind(const char* paramName, double value) const
    {
        const int32_t index = sqlite3_bind_parameter_index(m_stmt, paramName);
        return HE_VERIFY(index > 0) ? Bind(index, value) : false;
    }

    bool Statement::Bind(const char* paramName, int32_t value) const
    {
        const int32_t index = sqlite3_bind_parameter_index(m_stmt, paramName);
        return HE_VERIFY(index > 0) ? Bind(index, value) : false;
    }

    bool Statement::Bind(const char* paramName, int64_t value) const
    {
        const int32_t index = sqlite3_bind_parameter_index(m_stmt, paramName);
        return HE_VERIFY(index > 0) ? Bind(index, value) : false;
    }

    bool Statement::Bind(const char* paramName, uint32_t value) const
    {
        return Bind(paramName, static_cast<int64_t>(value));
    }

    bool Statement::Bind(const char* paramName, Span<const uint8_t> value) const
    {
        const int32_t index = sqlite3_bind_parameter_index(m_stmt, paramName);
        return HE_VERIFY(index > 0) ? Bind(index, value) : false;
    }

    bool Statement::Bind(const char* paramName, Span<const char> value) const
    {
        const int32_t index = sqlite3_bind_parameter_index(m_stmt, paramName);
        return HE_VERIFY(index > 0) ? Bind(index, value) : false;
    }

    bool Statement::Bind(const char* paramName, const char* value) const
    {
        const int32_t index = sqlite3_bind_parameter_index(m_stmt, paramName);
        return HE_VERIFY(index > 0) ? Bind(index, value) : false;
    }

    bool Statement::Bind(const char* paramName, decltype(nullptr) value) const
    {
        const int32_t index = sqlite3_bind_parameter_index(m_stmt, paramName);
        return HE_VERIFY(index > 0) ? Bind(index, value) : false;
    }

    bool Statement::BindNull(const char* paramName) const
    {
        const int32_t index = sqlite3_bind_parameter_index(m_stmt, paramName);
        return HE_VERIFY(index > 0) ? BindNull(index) : false;
    }

    StepResult Statement::Step() const
    {
        int32_t r = sqlite3_step(m_stmt);

        switch (r)
        {
            case SQLITE_ROW: return StepResult::Row;
            case SQLITE_DONE: return StepResult::Done;
            default:
                HE_SQLITE_ERROR(r, "SQLite error. Expected result: SQLITE_ROW({}) or SQLITE_DONE({})", SQLITE_ROW, SQLITE_DONE);
                return StepResult::Error;
        }
    }

    Column Statement::GetColumn(int32_t index) const
    {
        return { m_stmt, index };
    }
}

namespace he
{
    template <>
    const char* AsString(sqlite::StepResult x)
    {
        switch (x)
        {
            case sqlite::StepResult::Done: return "Done";
            case sqlite::StepResult::Row: return "Row";
            case sqlite::StepResult::Error: return "Error";
        }

        return "<unknown>";
    }
}
