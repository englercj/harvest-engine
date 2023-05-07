// Copyright Chad Engler

#include "he/sqlite/database.h"

#include "sqlite_internal.h"

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/hash.h"
#include "he/core/log.h"

#include "sqlite3.h"

namespace he::sqlite
{
    // We use ALTER TABLE DROP COLUMN, which was added in 3.35.0
    static_assert(SQLITE_VERSION_NUMBER >= 3035000, "Expected SQLite v3.35+");

    bool Database::Execute(sqlite3* m_db, const char* query)
    {
        HE_SQLITE_CHECK(OK, sqlite3_exec(m_db, query, nullptr, nullptr, nullptr));
        return true;
    }

    Database::~Database() noexcept
    {
        Close();
    }

    bool Database::Open(const char* path)
    {
        HE_SQLITE_CHECK(OK, sqlite3_open(path, &m_db));
        return true;
    }

    bool Database::Close()
    {
        if (m_db)
        {
            Execute("pragma optimize;");

            sqlite3* db = m_db;
            m_db = nullptr;

            HE_SQLITE_CHECK(OK, sqlite3_close(db));
        }

        return true;
    }

    bool Database::Execute(const char* query) const
    {
        return Database::Execute(m_db, query);
    }
}
