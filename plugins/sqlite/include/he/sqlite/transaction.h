// Copyright Chad Engler

#pragma once

struct sqlite3;

namespace he::sqlite
{
    class Statement;

    class Transaction final
    {
    public:
        explicit Transaction(sqlite3* db);

        ~Transaction();

        bool Commit();
        bool Rollback();

    private:
        bool Execute(const char* cmd);

    private:
        sqlite3* m_db = nullptr;
        bool m_finalized = false;
        char m_id[16];
    };
}
