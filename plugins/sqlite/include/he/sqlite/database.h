// Copyright Chad Engler

#pragma once

#include "he/sqlite/transaction.h"

struct sqlite3;

namespace he::sqlite
{
    class Database final
    {
    public:
        static bool Execute(sqlite3* db, const char* query);

    public:
        Database() = default;
        ~Database() noexcept;

        bool Open(const char* path);
        bool Close();

        bool IsOpen() const { return m_db != nullptr; }

        bool Execute(const char* query) const;

        sqlite3* Handle() const { return m_db; }

    private:
        sqlite3* m_db{ nullptr };
    };
}
