// Copyright Chad Engler

#include "fixtures.h"

#include "he/sqlite/orm_sql.h"

#include "he/core/test.h"

using namespace he;
using namespace he::sqlite;

// ------------------------------------------------------------------------------------------------
template <typename T, typename U>
static void TestToSql(const SqlWriterContext<U>& ctx, const T& obj, StringView expected)
{
    StringBuilder sql;
    ToSql(sql, obj, ctx);
    HE_EXPECT_EQ(sql.Str(), expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm_sql, SqlDataTypeTraits)
{
    enum class TestEnum {};

    static_assert(SqlDataTypeTraits<signed char>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<short>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<int>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<long>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<long long>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<unsigned char>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<unsigned short>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<unsigned int>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<unsigned long>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<unsigned long long>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<char>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<TestEnum>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<SystemTime>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<MonotonicTime>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<CycleCount>::SqlType == StringView("INTEGER"));
    static_assert(SqlDataTypeTraits<Duration>::SqlType == StringView("INTEGER"));

    static_assert(SqlDataTypeTraits<float>::SqlType == StringView("REAL"));
    static_assert(SqlDataTypeTraits<double>::SqlType == StringView("REAL"));

    static_assert(SqlDataTypeTraits<String>::SqlType == StringView("TEXT"));
    static_assert(SqlDataTypeTraits<Vector<char>>::SqlType == StringView("TEXT"));
    static_assert(SqlDataTypeTraits<StringView>::SqlType == StringView("TEXT"));
    static_assert(SqlDataTypeTraits<Span<char>>::SqlType == StringView("TEXT"));
    static_assert(SqlDataTypeTraits<const char*>::SqlType == StringView("TEXT"));
    static_assert(SqlDataTypeTraits<char[5]>::SqlType == StringView("TEXT"));

    static_assert(SqlDataTypeTraits<Vector<uint8_t>>::SqlType == StringView("BLOB"));
    static_assert(SqlDataTypeTraits<Uuid>::SqlType == StringView("BLOB(16)"));
    static_assert(SqlDataTypeTraits<Span<uint8_t>>::SqlType == StringView("BLOB"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm_sql, ToSql)
{
    constexpr auto TestSchema = DefineSchema(
        Table("parent",
            Column("id", &OrmTestParent::id, PrimaryKey()),
            Column("file_path", &OrmTestParent::filePath, Unique()),
            Column("file_path_depth", &OrmTestParent::filePathDepth, Default(0)),
            Column("file_write_time", &OrmTestParent::fileWriteTime),
            Column("file_size", &OrmTestParent::fileSize),
            Column("source_path", &OrmTestParent::sourcePath),
            Column("source_write_time", &OrmTestParent::sourceWriteTime),
            Column("source_size", &OrmTestParent::sourceSize),
            Column("scan_token", &OrmTestParent::scanToken)),
        Table("child",
            Column("id", &OrmTestChild::id, PrimaryKey()),
            Column("parent_id", &OrmTestChild::parentId),
            Column("name", &OrmTestChild::name),
            Unique(&OrmTestChild::name),
            ForeignKey(&OrmTestChild::parentId).References(&OrmTestParent::id)),
        Index("idx_test", &OrmTestParent::sourcePath, &OrmTestParent::sourceSize));

    SqlWriterContext ctx(TestSchema);
    ctx.writeRawValues = true;

    // PrimaryKeyConstraint
    TestToSql(ctx, PrimaryKey(), "PRIMARY KEY");
    TestToSql(ctx, PrimaryKey().Asc(), "PRIMARY KEY ASC");
    TestToSql(ctx, PrimaryKey().Desc(), "PRIMARY KEY DESC");
    TestToSql(ctx, PrimaryKey().AutoIncrement(), "PRIMARY KEY AUTOINCREMENT");
    TestToSql(ctx, PrimaryKey().OnConflictAbort(), "PRIMARY KEY ON CONFLICT ABORT");
    TestToSql(ctx, PrimaryKey().OnConflictRollback(), "PRIMARY KEY ON CONFLICT ROLLBACK");
    TestToSql(ctx, PrimaryKey(&OrmTestParent::id, &OrmTestParent::filePath), "PRIMARY KEY (id, file_path)");
    TestToSql(ctx, PrimaryKey(&OrmTestParent::id, &OrmTestParent::filePath).OnConflictAbort(), "PRIMARY KEY (id, file_path) ON CONFLICT ABORT");
    TestToSql(ctx, PrimaryKey(&OrmTestParent::id, &OrmTestParent::filePath).OnConflictRollback(), "PRIMARY KEY (id, file_path) ON CONFLICT ROLLBACK");

    // UniqueConstraint
    TestToSql(ctx, Unique(), "UNIQUE");
    TestToSql(ctx, Unique(&OrmTestParent::id, &OrmTestParent::filePath), "UNIQUE (id, file_path)");
    TestToSql(ctx, Unique(&OrmTestParent::id, &OrmTestParent::filePath).OnConflictAbort(), "UNIQUE (id, file_path) ON CONFLICT ABORT");
    TestToSql(ctx, Unique(&OrmTestParent::id, &OrmTestParent::filePath).OnConflictRollback(), "UNIQUE (id, file_path) ON CONFLICT ROLLBACK");

    // ForeignKeyConstraint, TODO: Actions
    TestToSql(ctx, ForeignKey(&OrmTestChild::parentId).References(&OrmTestParent::id), "FOREIGN KEY (parent_id) REFERENCES parent (id)");

    // DefaultConstraint
    TestToSql(ctx, Default("abc"), "DEFAULT 'abc'");
    TestToSql(ctx, Default(10), "DEFAULT 10");

    // NotNullConstraint, TODO: on conflict
    TestToSql(ctx, NotNull(), "NOT NULL");

    // ColumnDef
    TestToSql(ctx, Column("id", &OrmTestParent::id, Unique(), NotNull()), "id INTEGER UNIQUE NOT NULL");
    TestToSql(ctx, Column("custom", &OrmTestParent::custom), "custom FAKE_TYPE");

    // IndexDef
    TestToSql(ctx, Index("idx_test", &OrmTestParent::sourcePath, &OrmTestParent::sourceSize), "CREATE INDEX idx_test ON parent (source_path, source_size)");
    TestToSql(ctx, UniqueIndex("idx_test", &OrmTestParent::sourcePath, &OrmTestParent::sourceSize), "CREATE UNIQUE INDEX idx_test ON parent (source_path, source_size)");

    // ColumnRef
    TestToSql(ctx, Col(&OrmTestParent::id), "id");
    TestToSql(ctx, Col(&OrmTestParent::filePath), "file_path");
    TestToSql(ctx, Col(&OrmTestChild::parentId), "parent_id");

    // LimitExpr
    TestToSql(ctx, Limit(10), "LIMIT 10");
    TestToSql(ctx, Limit(10, 20), "LIMIT 10 OFFSET 20");

    // IsNullExpr
    TestToSql(ctx, IsNull(Col(&OrmTestParent::id)), "(id) IS NULL");

    // IsNotNullExpr
    TestToSql(ctx, IsNotNull(Col(&OrmTestParent::id)), "(id) IS NOT NULL");

    // GroupByExpr
    TestToSql(ctx, GroupBy(Col(&OrmTestParent::id)), "GROUP BY ((id))");

    // OrderByExpr
    TestToSql(ctx, OrderBy(Col(&OrmTestParent::id)), "ORDER BY ((id))");

    // MultiOrderByExpr
    TestToSql(ctx, MultiOrderBy(OrderBy(Col(&OrmTestParent::id)).Collate("ccc"), OrderBy(Col(&OrmTestParent::filePath)).Asc()), "ORDER BY (((id) COLLATE ccc), ((file_path) ASC))");

    // WhereExpr
    TestToSql(ctx, Where(Col(&OrmTestParent::id) == 15), "WHERE ((id) = (15))");

    // NotExpr
    TestToSql(ctx, !(Col(&OrmTestParent::id) == 15), "NOT ((id) = (15))");

    // AndExpr
    TestToSql(ctx, Col(&OrmTestParent::id) == 15 && IsNotNull(Col(&OrmTestParent::filePath)), "((id) = (15)) AND ((file_path) IS NOT NULL)");

    // OrExpr
    TestToSql(ctx, Col(&OrmTestParent::id) == 15 || IsNotNull(Col(&OrmTestParent::filePath)), "((id) = (15)) OR ((file_path) IS NOT NULL)");

    // EqualExpr
    TestToSql(ctx, Col(&OrmTestParent::id) == 15, "(id) = (15)");

    // NotEqualExpr
    TestToSql(ctx, Col(&OrmTestParent::id) != 15, "(id) != (15)");

    // GreaterThanExpr
    TestToSql(ctx, Col(&OrmTestParent::id) > 15, "(id) > (15)");

    // GreaterThanOrEqualExpr
    TestToSql(ctx, Col(&OrmTestParent::id) >= 15, "(id) >= (15)");

    // LesserThanExpr
    TestToSql(ctx, Col(&OrmTestParent::id) < 15, "(id) < (15)");

    // LesserThanOrEqualExpr
    TestToSql(ctx, Col(&OrmTestParent::id) <= 15, "(id) <= (15)");

    // ConcatExpr
    TestToSql(ctx, Concat(Col(&OrmTestParent::id), '%'), "(id) || ('%')");

    // AddExpr
    TestToSql(ctx, Col(&OrmTestParent::id) + 15, "(id) + (15)");

    // SubExpr
    TestToSql(ctx, Col(&OrmTestParent::id) - 15, "(id) - (15)");

    // MulExpr
    TestToSql(ctx, Col(&OrmTestParent::id) * 15, "(id) * (15)");

    // DivExpr
    TestToSql(ctx, Col(&OrmTestParent::id) / 15, "(id) / (15)");

    // ModExpr
    TestToSql(ctx, Col(&OrmTestParent::id) % 15, "(id) % (15)");

    // SelectObjectQuery
    TestToSql(ctx, SelectObj<OrmTestParent>(), "SELECT * FROM parent");

    // SelectQuery
    TestToSql(ctx, Select<OrmTestParent>(Cols(&OrmTestParent::id, &OrmTestParent::filePath)), "SELECT id, file_path FROM parent");
    TestToSql(ctx, Select<OrmTestParent>(Cols(&OrmTestParent::id, 15)), "SELECT id, 15 FROM parent");

    // DeleteQuery
    TestToSql(ctx, Delete<OrmTestParent>(), "DELETE FROM parent WHERE ((1) = (1))");
    TestToSql(ctx, Delete<OrmTestParent>(Where(Col(&OrmTestParent::id) == 15)), "DELETE FROM parent WHERE ((id) = (15))");

    // InsertObjectQuery
    TestToSql(ctx, InsertObj(OrmTestParent{}), "INSERT INTO parent (file_path, file_path_depth, file_write_time, file_size, source_path, source_write_time, source_size, scan_token) VALUES ('', 0, 0, 0, '', 0, 0, 0)");

    // UpsertObjectQuery
    TestToSql(ctx, UpsertObj(OrmTestParent{}), "INSERT INTO parent (file_path, file_path_depth, file_write_time, file_size, source_path, source_write_time, source_size, scan_token) VALUES ('', 0, 0, 0, '', 0, 0, 0) ON CONFLICT (id) DO UPDATE SET file_path = excluded.file_path, file_path_depth = excluded.file_path_depth, file_write_time = excluded.file_write_time, file_size = excluded.file_size, source_path = excluded.source_path, source_write_time = excluded.source_write_time, source_size = excluded.source_size, scan_token = excluded.scan_token");

    // InsertQuery
    TestToSql(ctx, Insert<OrmTestParent>(Cols(&OrmTestParent::id, &OrmTestParent::filePath), Values(10, "hello")), "INSERT INTO parent (id, file_path) VALUES (10, 'hello')");
    TestToSql(ctx, Insert<OrmTestParent>(Cols(&OrmTestParent::id, &OrmTestParent::filePath), DefaultValues()), "INSERT INTO parent (id, file_path) DEFAULT VALUES");

    TestToSql(ctx, Insert<OrmTestParent>(
        Cols(&OrmTestParent::id, &OrmTestParent::filePath),
        Values(10, "hello"),
        OnConflict(&OrmTestParent::id).DoNothing()),
        "INSERT INTO parent (id, file_path) VALUES (10, 'hello') ON CONFLICT (id) DO NOTHING");

    TestToSql(ctx, Insert<OrmTestParent>(
        Cols(&OrmTestParent::id, &OrmTestParent::filePath),
        Values(10, "hello"),
        OnConflict(&OrmTestParent::id).DoUpdate(Set(&OrmTestParent::filePath, "excl"), Set(&OrmTestParent::filePathDepth, Excluded(&OrmTestParent::filePathDepth)))),
        "INSERT INTO parent (id, file_path) VALUES (10, 'hello') ON CONFLICT (id) DO UPDATE SET file_path = 'excl', file_path_depth = excluded.file_path_depth");

    TestToSql(ctx, Insert<OrmTestChild>(
        Cols(&OrmTestChild::parentId, &OrmTestChild::name),
        Select<OrmTestParent>(
            Cols(&OrmTestParent::id, "child_name"),
            Limit(1)),
        OnConflict(&OrmTestParent::id).DoNothing()),
        "INSERT INTO child (parent_id, name) SELECT parent.id, 'child_name' FROM parent LIMIT 1 ON CONFLICT (id) DO NOTHING");

    // UpdateObjectQuery
    TestToSql(ctx, UpdateObj(OrmTestParent{}), "UPDATE parent SET file_path = '', file_path_depth = 0, file_write_time = 0, file_size = 0, source_path = '', source_write_time = 0, source_size = 0, scan_token = 0 WHERE id = 0");

    // UpdateQuery
    TestToSql(ctx, Update(Set(&OrmTestParent::id, 10), Set(&OrmTestParent::filePath, "hello")), "UPDATE parent SET id = 10, file_path = 'hello' WHERE ((1) = (1))");

    // Table
    TestToSql(ctx, Table("table", Column("id", &OrmTestParent::id, PrimaryKey())), R"(CREATE TABLE table (
    id INTEGER PRIMARY KEY
))");

    // Schema
    constexpr StringView ExpectedSchemaDDL = R"(CREATE TABLE parent (
    id INTEGER PRIMARY KEY,
    file_path TEXT UNIQUE,
    file_path_depth INTEGER DEFAULT 0,
    file_write_time INTEGER,
    file_size INTEGER,
    source_path TEXT,
    source_write_time INTEGER,
    source_size INTEGER,
    scan_token INTEGER
);
CREATE TABLE child (
    id INTEGER PRIMARY KEY,
    parent_id INTEGER,
    name TEXT,
    UNIQUE (name),
    FOREIGN KEY (parent_id) REFERENCES parent (id)
);
CREATE INDEX idx_test ON parent (source_path, source_size);
)";
    TestToSql(ctx, TestSchema, ExpectedSchemaDDL);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm_sql, ToSql_Queries)
{
    // TODO: more complex queries
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm_sql, BindSql)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm_sql, ReadSql)
{
    // TODO
}
