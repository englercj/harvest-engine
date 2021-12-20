// Copyright Chad Engler

#include "he/sqlite/database.h"

#include "sqlite_internal.h"

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/hash.h"
#include "he/core/scope_guard.h"

#include "sqlite3.h"

static const char StartupSql[] = R"(
    PRAGMA encoding = 'UTF-8';
    PRAGMA automatic_index = true;
    PRAGMA recursive_triggers = true;
    PRAGMA foreign_keys = true;
    PRAGMA journal_mode = WAL;
    VACUUM;
)";

static const char SchemaVersionTableSql[] = R"(
    CREATE TABLE IF NOT EXIST schema_version (
        version         INTEGER NOT NULL,
        description     TEXT,
        installed_on    INTEGER NOT NULL,
        execution_time  INTEGER NOT NULL,
        checksum        INTEGER NOT NULL,
        success         INTEGER NOT NULL,
    );
)";

static const char GetLatestSchemaVersionSql[] = R"(
    SELECT version FROM schema_version WHERE sucess = 1
    ORDER BY version DESC LIMIT 1;
)";

static const char InsertSchemaVersionSql[] = R"(
    INSERT INTO schema_version (version, description, installed_on, execution_time, checksum, success)
    VALUES (?, ?, ?, ?, ?, ?)
)";

namespace he::sqlite
{
    bool Database::Execute(sqlite3* m_db, const char* query)
    {
        HE_SQLITE_OK(sqlite3_exec(m_db, query, nullptr, nullptr, nullptr));
        return true;
    }

    Database::~Database()
    {
        Close();
    }

    bool Database::Open(const char* path)
    {
        HE_SQLITE_OK(sqlite3_open(path, &m_db));

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

            for (auto&& pair : m_statementCache)
            {
                pair.second.Finalize();
            }

            HE_SQLITE_OK(sqlite3_close(m_db));
        }

        return true;
    }

    const Statement& Database::StatementLiteral(const char* sql)
    {
        Statement& stmt = m_statementCache[sql];
        if (!stmt.IsPrepared())
        {
            HE_ASSERT(stmt.Prepare(m_db, sql));
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
            if (!HE_VERIFY(stmt.Prepare(m_db, GetLatestSchemaVersionSql, PrepareFlags::Temporary)))
                return false;

            const StepResult r = stmt.Step();
            if (r == StepResult::Error)
                return false;

            if (r == StepResult::Row)
                latestVersion = stmt.GetColumn(0).GetInt();
        }

        latestVersion++;

        Statement stmt;
        if (!HE_VERIFY(stmt.Prepare(m_db, InsertSchemaVersionSql, PrepareFlags::Temporary)))
            return false;

        const int32_t size = int32_t(migrations.Size());
        for (int32_t i = latestVersion; i < size; ++i)
        {
            const SchemaMigration& migration = migrations[i];
            const HighResolutionTime start = ReadHighResolutionClock();
            const bool success = Execute(migration.sql);
            const Duration time = ReadHighResolutionClock() - start;

            stmt.Reset();
            stmt.Bind(1, i);
            stmt.Bind(2, migration.description);
            stmt.Bind(3, int64_t(ReadSystemClock().ns));
            stmt.Bind(4, time.ns);
            stmt.Bind(5, int64_t(FnvHashString32(migration.sql)));
            stmt.Bind(6, success ? 1 : 0);

            if (!HE_VERIFY(stmt.Step() == StepResult::Done))
                return false;

            if (!HE_VERIFY(success, "Migration failed"))
                return false;
        }

        return true;
    }
}
