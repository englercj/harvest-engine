// Copyright Chad Engler

#include "he/sqlite/database.h"

#include "sqlite_internal.h"

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/hash.h"
#include "he/core/log.h"
#include "he/core/scope_guard.h"

#include "sqlite3.h"

static const char StartupSql[] = R"(
    PRAGMA automatic_index = true;
    PRAGMA encoding = 'UTF-8';
    PRAGMA foreign_keys = true;
    PRAGMA journal_mode = WAL;
    PRAGMA page_size = 4096;
    PRAGMA recursive_triggers = true;
    PRAGMA synchronous = normal;
    PRAGMA temp_store = memory;
)";

static const char SchemaVersionTableSql[] = R"(
    CREATE TABLE IF NOT EXISTS schema_version (
        version         INTEGER NOT NULL,
        description     TEXT,
        installed_on    INTEGER NOT NULL,
        execution_time  INTEGER NOT NULL,
        checksum        INTEGER NOT NULL,
        success         INTEGER NOT NULL
    );
)";

static const char GetLatestSchemaVersionSql[] = R"(
    SELECT version FROM schema_version WHERE success = 1
    ORDER BY version DESC LIMIT 1;
)";

static const char InsertSchemaVersionSql[] = R"(
    INSERT INTO schema_version (version, description, installed_on, execution_time, checksum, success)
    VALUES (?, ?, ?, ?, ?, ?)
)";

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

        if (!Execute(StartupSql))
            return false;

        return true;
    }

    bool Database::Close()
    {
        if (m_db)
        {
            HE_AT_SCOPE_EXIT([&]()
            {
                m_db = nullptr;
            });

            for (auto&& entry : m_statementCache)
            {
                entry.value.Finalize();
            }

            Execute("pragma optimize;");
            HE_SQLITE_CHECK(OK, sqlite3_close(m_db));
        }

        return true;
    }

    const Statement& Database::StatementLiteral(const char* sql)
    {
        Statement& stmt = m_statementCache[sql];
        if (!stmt.IsPrepared())
        {
            const bool r = stmt.Prepare(m_db, sql, PrepareFlags::Persistent);
            HE_ASSERT(r);
            HE_UNUSED(r);
        }
        return stmt;
    }

    bool Database::Execute(const char* query) const
    {
        return Database::Execute(m_db, query);
    }

    Transaction Database::BeginTransaction() const
    {
        return Transaction(m_db);
    }

    Statement Database::PrepareStatement(const char* query, PrepareFlags flags) const
    {
        Statement s;
        HE_VERIFY(s.Prepare(m_db, query, flags));
        return s;
    }

    bool Database::MigrateSchema(Span<const SchemaMigration> migrations) const
    {
        if (!HE_VERIFY(Execute(SchemaVersionTableSql)))
            return false;

        int32_t latestVersion = -1;

        {
            Statement stmt;
            if (!HE_VERIFY(stmt.Prepare(m_db, GetLatestSchemaVersionSql)))
                return false;

            const StepResult r = stmt.Step();
            if (r == StepResult::Error)
                return false;

            if (r == StepResult::Row)
                latestVersion = stmt.GetColumn(0).AsInt();
        }

        latestVersion++;

        Statement stmt;
        if (!HE_VERIFY(stmt.Prepare(m_db, InsertSchemaVersionSql)))
            return false;

        const int32_t size = static_cast<int32_t>(migrations.Size());
        for (int32_t i = latestVersion; i < size; ++i)
        {
            const SchemaMigration& migration = migrations[i];
            const MonotonicTime start = MonotonicClock::Now();
            const bool success = Execute(migration.sql);
            const Duration time = MonotonicClock::Now() - start;

            stmt.Reset();
            stmt.Bind(1, i);
            stmt.Bind(2, migration.description);
            stmt.Bind(3, static_cast<int64_t>(SystemClock::Now().val));
            stmt.Bind(4, time.val);
            stmt.Bind(5, static_cast<int64_t>(FNV32::String(migration.sql)));
            stmt.Bind(6, success ? 1 : 0);

            if (!HE_VERIFY(stmt.Step() == StepResult::Done))
                return false;

            if (!HE_VERIFY(success, HE_MSG("Migration failed")))
                return false;
        }

        return true;
    }
}
