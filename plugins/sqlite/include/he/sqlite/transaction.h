// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

struct sqlite3;

namespace he::sqlite
{
    class Statement;

    class Transaction final
    {
    public:
        explicit Transaction(sqlite3* db) noexcept;
        ~Transaction() noexcept;

        Transaction(Transaction&& x) noexcept;
        Transaction& operator=(Transaction&& x) noexcept;

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        bool Commit();
        bool Rollback();

    private:
        bool Execute(const char* cmd);

    private:
        sqlite3* m_db;
        bool m_finalized;
        char m_id[16];
    };
}
