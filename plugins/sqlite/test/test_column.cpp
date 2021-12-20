// Copyright Chad Engler

#include "he/sqlite/column.h"
#include "he/sqlite/database.h"

#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/test.h"

using namespace he::sqlite;

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, Column, Basic)
{
    Database db;
    HE_EXPECT(db.Open(":memory:"));
    HE_EXPECT(db.Execute("CREATE TABLE test (i INTEGER, r REAL, t TEXT, b BLOB);"));
    HE_EXPECT(db.Execute("INSERT INTO test VALUES (NULL, NULL, NULL, NULL);"));
    HE_EXPECT(db.Execute("INSERT INTO test VALUES (10, 1.25, 'This is text', X'ABCDEF0123456789');"));
    HE_EXPECT(db.Execute("INSERT INTO test VALUES (123456789012345, 54321.12345, NULL, NULL);"));

    {
        Statement s = db.PrepareStatement("SELECT * FROM test");

        HE_EXPECT_EQ(s.Step(), StepResult::Row);
        HE_EXPECT(s.GetColumn(0).IsNull());
        HE_EXPECT(s.GetColumn(1).IsNull());
        HE_EXPECT(s.GetColumn(2).IsNull());
        HE_EXPECT(s.GetColumn(3).IsNull());
        HE_EXPECT_EQ(s.GetColumn(0).GetInt(), 0);
        HE_EXPECT_EQ(s.GetColumn(0).GetInt64(), 0);
        HE_EXPECT_EQ(s.GetColumn(1).GetDouble(), 0);
        HE_EXPECT_EQ(s.GetColumn(2).GetText().Size(), 0);
        HE_EXPECT_EQ(s.GetColumn(3).GetBlob().Size(), 0);

        HE_EXPECT_EQ(s.Step(), StepResult::Row);
        HE_EXPECT(!s.GetColumn(0).IsNull());
        HE_EXPECT(!s.GetColumn(1).IsNull());
        HE_EXPECT(!s.GetColumn(2).IsNull());
        HE_EXPECT(!s.GetColumn(3).IsNull());
        HE_EXPECT_EQ(s.GetColumn(0).GetInt(), 10);
        HE_EXPECT_EQ(s.GetColumn(0).GetInt64(), 10);
        HE_EXPECT_EQ(s.GetColumn(1).GetDouble(), 1.25);
        HE_EXPECT_EQ_STR(s.GetColumn(2).GetText().Data(), "This is text");
        HE_EXPECT_EQ(s.GetColumn(2).GetText().Size(), String::Length("This is text"));
        uint8_t bytes[] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89 };
        HE_EXPECT_EQ(s.GetColumn(3).GetBlob().Size(), HE_LENGTH_OF(bytes));
        HE_EXPECT(MemEqual(s.GetColumn(3).GetBlob().Data(), bytes, HE_LENGTH_OF(bytes)));

        HE_EXPECT_EQ(s.Step(), StepResult::Row);
        HE_EXPECT(!s.GetColumn(0).IsNull());
        HE_EXPECT(!s.GetColumn(1).IsNull());
        HE_EXPECT(s.GetColumn(2).IsNull());
        HE_EXPECT(s.GetColumn(3).IsNull());
        HE_EXPECT_EQ(s.GetColumn(0).GetInt(), -2045911175);
        HE_EXPECT_EQ(s.GetColumn(0).GetInt64(), 123456789012345ll);
        HE_EXPECT_EQ(s.GetColumn(1).GetDouble(), 54321.12345);
        HE_EXPECT_EQ(s.GetColumn(2).GetText().Size(), 0);
        HE_EXPECT_EQ(s.GetColumn(3).GetBlob().Size(), 0);

        HE_EXPECT_EQ(s.Step(), StepResult::Done);
    }
}
