// Copyright Chad Engler

#include "he/sqlite/statement.h"
#include "he/sqlite/database.h"

#include "he/core/test.h"

using namespace he::sqlite;

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, Statement, Basic)
{
    Database db;
    HE_EXPECT(db.Open(":memory:"));
    HE_EXPECT(db.Execute("CREATE TABLE test (i INTEGER, b BLOB);"));
    HE_EXPECT(db.Execute("INSERT INTO test VALUES (NULL, NULL);"));
    HE_EXPECT(db.Execute("INSERT INTO test VALUES (1, X'AB');"));
    HE_EXPECT(db.Execute("INSERT INTO test VALUES (50, X'CD');"));
    HE_EXPECT(db.Execute("INSERT INTO test VALUES (50, X'EF');"));

    {
        Statement s = db.PrepareStatement("SELECT * FROM test WHERE rowid = ?");

        HE_EXPECT(s.Bind(1, 1));
        HE_EXPECT_EQ(s.Step(), StepResult::Row);
        HE_EXPECT(s.GetColumn(0).IsNull());
        HE_EXPECT(s.GetColumn(1).IsNull());
        HE_EXPECT_EQ(s.Step(), StepResult::Done);
        HE_EXPECT(s.Reset());

        HE_EXPECT(s.Bind(1, 2));
        HE_EXPECT_EQ(s.Step(), StepResult::Row);
        HE_EXPECT_EQ(s.GetColumn(0).GetInt(), 1);
        HE_EXPECT(!s.GetColumn(1).IsNull());
        HE_EXPECT_EQ(s.Step(), StepResult::Done);
    }

    {
        Statement s = db.PrepareStatement("SELECT * FROM test WHERE i = ?");

        HE_EXPECT(s.Bind(1, 50));
        HE_EXPECT_EQ(s.Step(), StepResult::Row);
        HE_EXPECT_EQ(s.GetColumn(0).GetInt(), 50);
        HE_EXPECT_EQ(s.GetColumn(1).GetBlob().Size(), 1);
        HE_EXPECT_EQ(*s.GetColumn(1).GetBlob().Data(), 0xCD);
        HE_EXPECT_EQ(s.Step(), StepResult::Row);
        HE_EXPECT_EQ(s.GetColumn(0).GetInt(), 50);
        HE_EXPECT_EQ(s.GetColumn(1).GetBlob().Size(), 1);
        HE_EXPECT_EQ(*s.GetColumn(1).GetBlob().Data(), 0xEF);
        HE_EXPECT_EQ(s.Step(), StepResult::Done);
        HE_EXPECT(s.Reset());

        HE_EXPECT(s.Bind(1, 50));
        uint32_t rowCount = 0;
        const bool eachRowResult = s.EachRow([&](const Statement& st)
        {
            ++rowCount;
            HE_EXPECT_EQ(st.GetColumn(0).GetInt(), 50);
            HE_EXPECT_EQ(s.GetColumn(1).GetBlob().Size(), 1);
        });
        HE_EXPECT(eachRowResult);
        HE_EXPECT_EQ(rowCount, 2);
        HE_EXPECT(s.Reset());
    }
}
