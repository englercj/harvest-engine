// Copyright Chad Engler

#pragma once

#include "he/sqlite/database.h"

namespace he::assets
{
    class AssetDatabase final
    {
    public:
        ~AssetDatabase() { Terminate(); }

        bool Initialize(const char* dbPath);
        bool Terminate();

        sqlite::Transaction BeginTransaction() const
        {
            return m_db.BeginTransaction();
        }

        const sqlite::Statement& StatementLiteral(const char* sql)
        {
            return m_db.StatementLiteral(sql);
        }

    private:
        sqlite::Database m_db;
    };
}
