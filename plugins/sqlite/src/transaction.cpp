// Copyright Chad Engler

#include "he/sqlite/transaction.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/utils.h"
#include "he/core/random.h"
#include "he/sqlite/database.h"
#include "he/sqlite/statement.h"

#include "sqlite3.h"
#include "fmt/format.h"
#include "fmt/ranges.h"

namespace he::sqlite
{
    Transaction::Transaction(sqlite3* db) noexcept
        : m_db(db)
        , m_finalized(false)
        // , m_id() // Intentionally not initialized. We do it below.
    {
        uint8_t idBytes[HE_LENGTH_OF(m_id) / 2];
        const bool idResult = GetSecureRandomBytes(idBytes);
        HE_ASSERT(idResult);
        HE_UNUSED(idResult);

        fmt::format_to_n(m_id, HE_LENGTH_OF(m_id), "{:x}", fmt::join(idBytes, ""));

        m_id[0] = '_'; // must start with a non-numeric character
        m_id[15] = '\0'; // end with a null terminator to simplify Execute() logic

        HE_VERIFY(Execute("SAVEPOINT"));
    }

    Transaction::~Transaction() noexcept
    {
        Rollback();
    }

    Transaction::Transaction(Transaction&& x) noexcept
        : m_db(Exchange(x.m_db, nullptr))
        , m_finalized(Exchange(x.m_finalized, false))
    {
        MemCopy(m_id, x.m_id, sizeof(m_id));
    }

    Transaction& Transaction::operator=(Transaction&& x) noexcept
    {
        Rollback();

        m_db = Exchange(x.m_db, nullptr);
        m_finalized = Exchange(x.m_finalized, false);
        MemCopy(m_id, x.m_id, sizeof(m_id));
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
        constexpr uint32_t QueryMaxLen = BufferLen - (HE_LENGTH_OF(m_id) + 1); // +1 for the ' ' character

        // our size checks & memcpy assume m_id ends in a null terminator
        HE_ASSERT(m_id[15] == '\0');

        const uint32_t len = String::Length(cmd);
        HE_ASSERT(len <= QueryMaxLen);

        char query[BufferLen];
        MemCopy(query, cmd, len);
        query[len] = ' ';
        MemCopy(query + len + 1, m_id, HE_LENGTH_OF(m_id));

        return Database::Execute(m_db, query);
    }
}
