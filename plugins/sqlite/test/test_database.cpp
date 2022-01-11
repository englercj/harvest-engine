// Copyright Chad Engler

#include "he/sqlite/database.h"

#include "he/core/test.h"

using namespace he::sqlite;

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, Database, Basic)
{
    Database db;
    HE_EXPECT(db.Open(":memory:"));
    HE_EXPECT(db.Execute("CREATE TABLE test(col0 INTEGER);"));
    HE_EXPECT(db.Execute("INSERT INTO test VALUES (10);"));
    HE_EXPECT(db.Execute("SELECT col0 FROM test;"));
    HE_EXPECT(db.Execute("DROP TABLE test;"));
    HE_EXPECT(db.Close());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, Database, Negative)
{
    Database db;

    he::ScopedErrorHandler err([](he::ErrorType, const char*, const uint32_t, const char*, const char*, const char*) { return false; });

    // Execute can't work without an open db
    HE_EXPECT(!db.Execute("CREATE TABLE test(col0 INTEGER);"));

    // Invalid SQL should fail on an open db
    HE_EXPECT(db.Open(":memory:"));
    HE_EXPECT(!db.Execute("THIS IS NOT A QUERY"));
    HE_EXPECT(!db.Execute("SELECT * FROM not_a_table;"));
    HE_EXPECT(db.Execute("CREATE TABLE test(col0 INTEGER);"));
    HE_EXPECT(!db.Execute("SELECT col1 FROM test;"));
    HE_EXPECT(db.Close());

    // Calling close multiple times is OK
    HE_EXPECT(db.Close());
    HE_EXPECT(db.Close());
    HE_EXPECT(db.Close());
}
