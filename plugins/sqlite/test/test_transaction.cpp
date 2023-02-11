// Copyright Chad Engler

#include "he/sqlite/transaction.h"
#include "he/sqlite/database.h"

#include "he/core/test.h"

using namespace he::sqlite;

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, transaction, test)
{
    Database db;
    HE_EXPECT(db.Open(":memory:"));
    HE_EXPECT(db.Execute("CREATE TABLE test (i INTEGER);"));

    Statement s = db.PrepareStatement("SELECT count(*) FROM test");

    HE_EXPECT_EQ(s.Step(), StepResult::Row);
    HE_EXPECT_EQ(s.GetColumn(0).AsInt(), 0);
    HE_EXPECT_EQ(s.Step(), StepResult::Done);
    HE_EXPECT(s.Reset());

    {
        Transaction t = db.BeginTransaction();
        db.Execute("INSERT INTO test VALUES (10);");
    }

    HE_EXPECT_EQ(s.Step(), StepResult::Row);
    HE_EXPECT_EQ(s.GetColumn(0).AsInt(), 0);
    HE_EXPECT_EQ(s.Step(), StepResult::Done);
    HE_EXPECT(s.Reset());

    {
        Transaction t = db.BeginTransaction();
        db.Execute("INSERT INTO test VALUES (10);");
        t.Rollback();
    }

    HE_EXPECT_EQ(s.Step(), StepResult::Row);
    HE_EXPECT_EQ(s.GetColumn(0).AsInt(), 0);
    HE_EXPECT_EQ(s.Step(), StepResult::Done);
    HE_EXPECT(s.Reset());

    {
        Transaction t = db.BeginTransaction();
        db.Execute("INSERT INTO test VALUES (10);");
        t.Commit();
    }

    HE_EXPECT_EQ(s.Step(), StepResult::Row);
    HE_EXPECT_EQ(s.GetColumn(0).AsInt(), 1);
    HE_EXPECT_EQ(s.Step(), StepResult::Done);
    HE_EXPECT(s.Reset());
}
