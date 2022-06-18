// Copyright Chad Engler

#pragma once

#include "he/sqlite/schema_migration.h"
#include "he/sqlite/statement.h"
#include "he/sqlite/transaction.h"

#include <unordered_map>

struct sqlite3;

namespace he::sqlite
{
    class Database final
    {
    public:
        static bool Execute(sqlite3* db, const char* query);

    public:
        ~Database() noexcept;

        bool Open(const char* path);
        bool Close();

        const Statement& StatementLiteral(const char* sql);

        bool Execute(const char* query) const;

        Transaction BeginTransaction() const;
        Statement PrepareStatement(const char* query, PrepareFlags flags = PrepareFlags::None) const;

        bool MigrateSchema(Span<const SchemaMigration> migrations) const;

    private:
        sqlite3* m_db{ nullptr };
        std::unordered_map<const char*, Statement> m_statementCache{};
    };
}
