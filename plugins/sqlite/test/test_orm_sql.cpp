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
HE_TEST(sqlite, orm_sql, PickModel)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm_sql, SqlDataTypeTraits)
{
    enum class TestEnum {};

    static_assert(SqlDataTypeTraits<char>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<signed char>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<short>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<int>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<long>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<long long>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<unsigned char>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<unsigned short>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<unsigned int>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<unsigned long>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<unsigned long long>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<TestEnum>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<SystemTime>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<MonotonicTime>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<CycleCount>::Sql == "INTEGER");
    static_assert(SqlDataTypeTraits<Duration>::Sql == "INTEGER");

    static_assert(SqlDataTypeTraits<float>::Sql == "REAL");
    static_assert(SqlDataTypeTraits<double>::Sql == "REAL");

    static_assert(SqlDataTypeTraits<String>::Sql == "TEXT");
    static_assert(SqlDataTypeTraits<Vector<char>>::Sql == "TEXT");
    static_assert(SqlDataTypeTraits<StringView>::Sql == "TEXT");
    static_assert(SqlDataTypeTraits<Span<char>>::Sql == "TEXT");
    static_assert(SqlDataTypeTraits<const char*>::Sql == "TEXT");

    static_assert(SqlDataTypeTraits<Vector<uint8_t>>::Sql == "BLOB");
    static_assert(SqlDataTypeTraits<Uuid>::Sql == "BLOB(16)");
    static_assert(SqlDataTypeTraits<Span<uint8_t>>::Sql == "BLOB");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm_sql, SqlWriterContext)
{
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
    TestToSql(ctx, Select<OrmTestParent>(), "SELECT * FROM parent");

    // SelectQuery
    TestToSql(ctx, Select(&OrmTestParent::id, &OrmTestParent::filePath), "SELECT id, file_path FROM parent");

    // DeleteQuery
    TestToSql(ctx, Delete<OrmTestParent>(), "DELETE FROM parent");
    TestToSql(ctx, Delete<OrmTestParent>(Where(Col(&OrmTestParent::id) == 15)), "DELETE FROM parent WHERE id = 15");

    // InsertObjectQuery
    TestToSql(ctx, Insert(OrmTestParent{}), "INSERT INTO parent (id, file_path, file_path_depth, file_write_time, file_size, source_path, source_write_time, source_size, scan_token) VALUES (0, '', 0, 0, 0, '', 0, 0, 0, 0)");

    // InsertQuery
    TestToSql(ctx, Insert(&OrmTestParent::id, &OrmTestParent::filePath).Values(10, "hello"), "INSERT INTO parent (id, file_path) VALUES (10, 'hello')");

    // UpdateObjectQuery
    TestToSql(ctx, Update(OrmTestParent{}), "UPDATE parent SET id = 0, file_path = '', file_path_depth = 0, file_write_time = 0, file_size = 0, source_path = '', source_write_time = 0, source_size = 0, scan_token = 0");

    // UpdateQuery
    TestToSql(ctx, Update(&OrmTestParent::id, &OrmTestParent::filePath).Set(10, "hello"), "UPDATE parent SET id = 10, file_path = 'hello'");

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
