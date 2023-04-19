// Copyright Chad Engler

#include "he/sqlite/transaction.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/fmt.h"
#include "he/core/memory_ops.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/utils.h"
#include "he/core/random.h"
#include "he/sqlite/database.h"
#include "he/sqlite/statement.h"

#include "sqlite3.h"

namespace he::sqlite
{
    Transaction::Transaction(sqlite3* db) noexcept
        : m_db(db)
        , m_finalized(false)
        , m_id()
    {
        uint8_t idBytes[8];
        const bool idResult = GetSecureRandomBytes(idBytes);
        HE_ASSERT(idResult);
        HE_UNUSED(idResult);

        FormatTo(m_id, "{:02x}", FmtJoin(idBytes, ""));
        HE_ASSERT(m_id.Size() == 16);

        m_id[0] = '_'; // must start with a non-numeric character

        HE_VERIFY(Execute("SAVEPOINT"));
    }

    Transaction::~Transaction() noexcept
    {
        Rollback();
    }

    Transaction::Transaction(Transaction&& x) noexcept
        : m_db(Exchange(x.m_db, nullptr))
        , m_finalized(Exchange(x.m_finalized, false))
        , m_id(Move(x.m_id))
    {}

    Transaction& Transaction::operator=(Transaction&& x) noexcept
    {
        Rollback();

        m_db = Exchange(x.m_db, nullptr);
        m_finalized = Exchange(x.m_finalized, false);
        m_id = Move(x.m_id);
        return *this;
    }

    bool Transaction::Commit()
    {
        if (m_finalized)
            return true;

        if (!Execute("RELEASE"))
            return false;

        m_finalized = true;
        return true;
    }

    bool Transaction::Rollback()
    {
        if (m_finalized)
            return true;

        return Execute("ROLLBACK TO") && Commit();
    }

    bool Transaction::Execute(const char* cmd)
    {
        constexpr uint32_t BufferLen = 32;
        constexpr uint32_t QueryMaxLen = BufferLen - (16 + 1); // 16 for id bytes, +1 for the ' ' character
        HE_ASSERT(m_id.Size() == 16);

        const uint32_t len = String::Length(cmd);
        HE_ASSERT(len <= QueryMaxLen);
        HE_UNUSED(QueryMaxLen);

        char query[BufferLen];
        MemCopy(query, cmd, len);
        query[len] = ' ';
        MemCopy(query + len + 1, m_id.Data(), m_id.Size() + 1); // +1 to include null terminator

        return Database::Execute(m_db, query);
    }
}
