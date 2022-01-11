// Copyright Chad Engler

#include "he/sqlite/transaction.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/span.h"
#include "he/core/span_fmt.h"
#include "he/core/string.h"
#include "he/core/utils.h"
#include "he/core/uuid.h"
#include "he/sqlite/database.h"
#include "he/sqlite/statement.h"

#include "sqlite3.h"
#include "fmt/format.h"

namespace he::sqlite
{
    Transaction::Transaction(sqlite3* db)
        : m_db(db)
    {
        Uuid id = Uuid::CreateV4();
        static_assert(HE_LENGTH_OF(id.m_bytes) * 2 >= HE_LENGTH_OF(m_id), "");

        fmt::format_to_n(m_id, HE_LENGTH_OF(m_id) - 1, "{}", Span<const uint8_t>(id.m_bytes));

        m_id[0] = '_'; // must start with a non-numeric character
        m_id[14] = ToHex(id.m_bytes[7] & 0xf);
        m_id[15] = '\0';

        HE_VERIFY(Execute("SAVEPOINT"));
    }

    Transaction::~Transaction()
    {
        if (!m_finalized)
        {
            Rollback();
        }
    }

    bool Transaction::Commit()
    {
        if (!Execute("RELEASE"))
            return false;

        m_finalized = true;
        return true;
    }

    bool Transaction::Rollback()
    {
        return Execute("ROLLBACK TO") && Commit();
    }

    bool Transaction::Execute(const char* cmd)
    {
        const uint32_t len = String::Length(cmd);
        HE_ASSERT(len < 16);

        char query[32];
        MemCopy(query, cmd, len);
        query[len] = ' ';
        MemCopy(query + len + 1, m_id, HE_LENGTH_OF(m_id));

        return Database::Execute(m_db, query);
    }
}
