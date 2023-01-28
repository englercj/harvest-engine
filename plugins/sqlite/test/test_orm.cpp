// Copyright Chad Engler

#include "he/sqlite/orm.h"

#include "he/core/clock.h"
#include "he/core/string.h"
#include "he/core/string_builder.h"
#include "he/core/string_view.h"
#include "he/core/test.h"

using namespace he;
using namespace he::sqlite;

// ------------------------------------------------------------------------------------------------
struct OrmCustomValue { uint64_t val; };

template <>
struct DataTypeTraits<OrmCustomValue>
{
    static constexpr StringView Sql = "FAKE_TYPE";
    static bool Bind(sqlite::Statement& stmt, int32_t index, const OrmCustomValue& value) { stmt.Bind(index, BitCast<int64_t>(value.val)); }
    static void Read(const sqlite::Column& column, OrmCustomValue& value) { value.val = BitCast<uint64_t>(column.GetInt64()); }
};

struct OrmTestParent final
{
    uint32_t id{ 0 };

    String filePath{};
    uint32_t filePathDepth{ 0 };
    SystemTime fileWriteTime{ 0 };
    uint32_t fileSize{ 0 };

    String sourcePath{};
    SystemTime sourceWriteTime{ 0 };
    uint32_t sourceSize{ 0 };

    uint32_t scanToken{ 0 };
    OrmCustomValue custom{};
};

struct OrmTestChild final
{
    uint32_t id{ 0 };
    uint32_t parentId{ 0 };
    String name{};
};

struct OrmTestHasFunc
{
    void DoStuff() {}
};

template <typename T> struct IsIntTest { static constexpr bool Value = std::is_same_v<T, int>; };
template <typename T> struct IsFloatTest { static constexpr bool Value = std::is_same_v<T, float>; };

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, CountTuple)
{
    static_assert(CountTuple<std::tuple<int, float, int, int>, IsIntTest> == 3);
    static_assert(CountTuple<std::tuple<int, float, int, int>, IsFloatTest> == 1);
    static_assert(CountTuple<std::tuple<double, float, OrmTestParent>, IsIntTest> == 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IterateTuple)
{
    std::tuple<int, float, int, int> t;

    uint32_t count = 0;
    IterateTuple(t, [&](const auto& x, uint32_t)
    {
        static_assert(std::is_same_v<std::decay_t<decltype(x)>, int> || std::is_same_v<std::decay_t<decltype(x)>, float>);
        ++count;
    });
    HE_EXPECT_EQ(count, 4);

    count = 0;
    IterateTuple<IsIntTest>(t, [&](const auto& x, uint32_t)
    {
        static_assert(std::is_same_v< std::decay_t<decltype(x)>, int>);
        ++count;
    });
    HE_EXPECT_EQ(count, 3);

    count = 0;
    IterateTuple<IsFloatTest>(t, [&](const auto& x, uint32_t)
    {
        static_assert(std::is_same_v< std::decay_t<decltype(x)>, float>);
        ++count;
    });
    HE_EXPECT_EQ(count, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, MemberObjectType)
{
    static_assert(std::is_same_v<MemberObjectType<decltype(&OrmTestParent::id)>, OrmTestParent>);
    static_assert(std::is_same_v<MemberObjectType<decltype(&OrmTestChild::id)>, OrmTestChild>);
    static_assert(std::is_same_v<MemberObjectType<decltype(&OrmTestHasFunc::DoStuff)>, OrmTestHasFunc>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ColumnsObjectType)
{
    static_assert(std::is_same_v<ColumnsObjectType<decltype(&OrmTestParent::id), decltype(&OrmTestParent::filePath), decltype(&OrmTestParent::filePathDepth)>, OrmTestParent>);
    static_assert(std::is_same_v<ColumnsObjectType<decltype(&OrmTestChild::id), decltype(&OrmTestChild::parentId)>, OrmTestChild>);

    // Column mismatch, does not compile
    //static_assert(std::is_same_v<ColumnsObjectType<decltype(&OrmTestParent::id), decltype(&OrmTestChild::parentId)>, OrmTestParent>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, DataTypeTraits)
{

    static_assert(DataTypeTraits<char>::Sql == "INTEGER");
    static_assert(DataTypeTraits<unsigned char>::Sql == "INTEGER");
    static_assert(DataTypeTraits<int>::Sql == "INTEGER");
    static_assert(DataTypeTraits<unsigned int>::Sql == "INTEGER");

    enum class TestEnum {};
    static_assert(DataTypeTraits<TestEnum>::Sql == "INTEGER");

    static_assert(DataTypeTraits<float>::Sql == "REAL");
    static_assert(DataTypeTraits<double>::Sql == "REAL");

    static_assert(DataTypeTraits<const char*>::Sql == "TEXT");
    static_assert(DataTypeTraits<StringView>::Sql == "TEXT");
    static_assert(DataTypeTraits<StringView>::Sql == "TEXT");
    static_assert(DataTypeTraits<Span<char>>::Sql == "TEXT");
    static_assert(DataTypeTraits<Vector<char>>::Sql == "TEXT");

    static_assert(DataTypeTraits<Span<uint8_t>>::Sql == "BLOB");
    static_assert(DataTypeTraits<Vector<uint8_t>>::Sql == "BLOB");

    static_assert(DataTypeTraits<Duration>::Sql == "INTEGER");
    static_assert(DataTypeTraits<SystemTime>::Sql == "INTEGER");
    static_assert(DataTypeTraits<MonotonicTime>::Sql == "INTEGER");
    static_assert(DataTypeTraits<CycleCount>::Sql == "INTEGER");

    static_assert(DataTypeTraits<Uuid>::Sql == "BLOB(16)");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, PrimaryKeyConstraint)
{
    constexpr auto m = DefineModel("test",
        DefineColumn("id", &OrmTestParent::id),
        DefineColumn("file_path", &OrmTestParent::filePath));

    constexpr auto c = PrimaryKey(&OrmTestParent::id, &OrmTestParent::filePath);

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, PrimaryKeyConstraint>);
    static_assert(IsSpecialization<T::ColumnsType, std::tuple>);
    static_assert(std::tuple_size_v<T::ColumnsType> == 2);
    static_assert(std::is_same_v<T::ObjectType, OrmTestParent>);

    StringBuilder ddl;
    c.GetColumnDDL(ddl);
    HE_EXPECT_EQ(ddl.Str(), "PRIMARY KEY");

    ddl.Clear();
    c.GetTableDDL(ddl, m);
    HE_EXPECT_EQ(ddl.Str(), "PRIMARY KEY (id, file_path)");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, UniqueConstraint)
{
    constexpr auto m = DefineModel("test",
        DefineColumn("id", &OrmTestParent::id),
        DefineColumn("file_path", &OrmTestParent::filePath));

    constexpr auto c = Unique(&OrmTestParent::id, &OrmTestParent::filePath);

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, UniqueConstraint>);
    static_assert(IsSpecialization<T::ColumnsType, std::tuple>);
    static_assert(std::tuple_size_v<T::ColumnsType> == 2);
    static_assert(std::is_same_v<T::ObjectType, OrmTestParent>);

    StringBuilder ddl;
    c.GetColumnDDL(ddl);
    HE_EXPECT_EQ(ddl.Str(), "UNIQUE");

    ddl.Clear();
    c.GetTableDDL(ddl, m);
    HE_EXPECT_EQ(ddl.Str(), "UNIQUE (id, file_path)");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ForeignKeyConstraint)
{
    constexpr auto parent = DefineModel("parent", DefineColumn("id", &OrmTestParent::id));
    constexpr auto child = DefineModel("child",
        DefineColumn("id", &OrmTestChild::id),
        DefineColumn("parent_id", &OrmTestChild::parentId));

    constexpr auto c = ForeignKey(&OrmTestChild::parentId).References(&OrmTestParent::id);

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, ForeignKeyConstraint>);
    static_assert(IsSpecialization<T::ColumnsType, std::tuple>);
    static_assert(IsSpecialization<T::ReferencesType, std::tuple>);
    static_assert(std::tuple_size_v<T::ColumnsType> == 1);
    static_assert(std::tuple_size_v<T::ReferencesType> == 1);
    static_assert(std::is_same_v<T::ObjectType, OrmTestChild>);
    static_assert(std::is_same_v<T::ReferencedObjectType, OrmTestParent>);

    StringBuilder ddl;
    c.GetTableDDL(ddl, child);
    HE_EXPECT_EQ(ddl.Str(), "FOREIGN KEY (parent_id) REFERENCES parent (id)");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, DefaultConstraint)
{
    constexpr auto c = Default(10);

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, DefaultConstraint>);
    static_assert(std::is_same_v<T::ValueType, int>);

    StringBuilder ddl;
    c.GetColumnDDL(ddl);
    HE_EXPECT_EQ(ddl.Str(), "DEFAULT 10");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, NotNullConstraint)
{
    constexpr auto c = NotNull();

    using T = std::decay_t<decltype(c)>;

    static_assert(std::is_same_v<T, NotNullConstraint>);

    StringBuilder ddl;
    c.GetColumnDDL(ddl);
    HE_EXPECT_EQ(ddl.Str(), "NOT NULL");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ColumnDef)
{
    constexpr auto c = DefineColumn("id", &OrmTestParent::id, Unique(), NotNull());

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, ColumnDef>);
    static_assert(std::is_same_v<T::ObjectType, OrmTestParent>);
    static_assert(std::is_same_v<T::ValueType, uint32_t>);
    static_assert(std::is_same_v<T::Traits, DataTypeTraits<uint32_t>>);
    static_assert(IsSpecialization<T::ConstraintsType, std::tuple>);
    static_assert(std::tuple_size_v<T::ConstraintsType> == 2);
    static_assert(std::is_same_v<std::tuple_element_t<0, T::ConstraintsType>, UniqueConstraint<>>);
    static_assert(std::is_same_v<std::tuple_element_t<1, T::ConstraintsType>, NotNullConstraint>);

    StringBuilder ddl;
    c.GetDDL(ddl);
    HE_EXPECT_EQ(ddl.Str(), "id INTEGER UNIQUE NOT NULL");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ColumnDef_CustomDataType)
{
    constexpr auto c = DefineColumn("custom_value", &OrmTestParent::custom);

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, ColumnDef>);
    static_assert(std::is_same_v<T::ObjectType, OrmTestParent>);
    static_assert(std::is_same_v<T::ValueType, OrmCustomValue>);
    static_assert(std::is_same_v<T::Traits, DataTypeTraits<OrmCustomValue>>);
    static_assert(IsSpecialization<T::ConstraintsType, std::tuple>);
    static_assert(std::tuple_size_v<T::ConstraintsType> == 0);

    StringBuilder ddl;
    c.GetDDL(ddl);
    HE_EXPECT_EQ(ddl.Str(), "custom_value FAKE_TYPE");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IndexDef)
{
    constexpr auto m = DefineModel("parent",
        DefineColumn("source_path", &OrmTestParent::sourcePath),
        DefineColumn("source_size", &OrmTestParent::sourceSize));

    constexpr auto c = Index("idx_test", &OrmTestParent::sourcePath, &OrmTestParent::sourceSize);

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, IndexDef>);
    static_assert(std::is_same_v<T::ObjectType, OrmTestParent>);
    static_assert(IsSpecialization<T::ColumnsType, std::tuple>);
    static_assert(std::tuple_size_v<T::ColumnsType> == 2);
    static_assert(!c.unique);

    StringBuilder ddl;
    c.GetDDL(ddl, m);
    HE_EXPECT_EQ(ddl.Str(), "CREATE INDEX idx_test ON parent (source_path, source_size)");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IndexDef_Unique)
{
    constexpr auto m = DefineModel("parent",
        DefineColumn("source_path", &OrmTestParent::sourcePath),
        DefineColumn("source_size", &OrmTestParent::sourceSize));

    constexpr auto c = UniqueIndex("idx_test", &OrmTestParent::sourcePath, &OrmTestParent::sourceSize);

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, IndexDef>);
    static_assert(std::is_same_v<T::ObjectType, OrmTestParent>);
    static_assert(IsSpecialization<T::ColumnsType, std::tuple>);
    static_assert(std::tuple_size_v<T::ColumnsType> == 2);
    static_assert(c.unique);

    StringBuilder ddl;
    c.GetDDL(ddl, m);
    HE_EXPECT_EQ(ddl.Str(), "CREATE UNIQUE INDEX idx_test ON parent (source_path, source_size)");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ColumnRef)
{
    constexpr auto c = Col(&OrmTestParent::id);

    using T = std::decay_t<decltype(c)>;

    static_assert(std::is_same_v<T::ObjectType, OrmTestParent>);
    static_assert(std::is_same_v<T::ValueType, uint32_t>);
    static_assert(c.member == &OrmTestParent::id);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LimitExpr)
{
    constexpr auto c = Limit(1);

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, LimitExpr>);
    static_assert(std::is_same_v<T::LimitType, int>);
    static_assert(std::is_same_v<T::OffsetType, void>);
    static_assert(std::is_same_v<T::OffsetStorageType, int>);
    static_assert(!T::HasOffset);
    static_assert(c.limit == 1);
    static_assert(c.offset == 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LimitExpr_Offset)
{
    constexpr auto c = Limit(1, 10);

    using T = std::decay_t<decltype(c)>;

    static_assert(IsSpecialization<T, LimitExpr>);
    static_assert(std::is_same_v<T::LimitType, int>);
    static_assert(std::is_same_v<T::OffsetType, int>);
    static_assert(std::is_same_v<T::OffsetStorageType, int>);
    static_assert(T::HasOffset);
    static_assert(c.limit == 1);
    static_assert(c.offset == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IsNullExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IsNotNullExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, WhereExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, NotExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, AndExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, OrExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, EqualExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, NotEqualExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, GreaterThanExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, GreaterThanOrEqualExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LesserThanExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LesserThanOrEqualExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ConcatExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, AddExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, SubExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, MulExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, DivExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ModExpr)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ModelBase_Construct)
{
    ModelBase m{ "test" };
    HE_UNUSED(m);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, Model)
{
    auto OrmTestParentModel = DefineModel("parent",
        DefineColumn("id", &OrmTestParent::id, PrimaryKey()),
        DefineColumn("file_path", &OrmTestParent::filePath, Unique()),
        DefineColumn("file_path_depth", &OrmTestParent::filePathDepth, Default(0)),
        DefineColumn("file_write_time", &OrmTestParent::fileWriteTime),
        DefineColumn("file_size", &OrmTestParent::fileSize),
        DefineColumn("source_path", &OrmTestParent::sourcePath),
        DefineColumn("source_write_time", &OrmTestParent::sourceWriteTime),
        DefineColumn("source_size", &OrmTestParent::sourceSize),
        DefineColumn("scan_token", &OrmTestParent::scanToken),
        Index("idx_asset_file_source_path", &OrmTestParent::sourcePath));

    constexpr StringView ExpectedParentDDL = R"(CREATE TABLE parent (
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
CREATE INDEX idx_asset_file_source_path ON parent (source_path);
)";

    auto OrmTestChildModel = DefineModel("child",
        DefineColumn("id", &OrmTestChild::id, PrimaryKey()),
        DefineColumn("parent_id", &OrmTestChild::parentId),
        DefineColumn("name", &OrmTestChild::name),
        Unique(&OrmTestChild::name),
        ForeignKey(&OrmTestChild::parentId).References(OrmTestParentModel, &OrmTestParent::id));

    constexpr StringView ExpectedChildDDL = R"(CREATE TABLE child (
    id INTEGER PRIMARY KEY,
    parent_id INTEGER,
    name TEXT,
    UNIQUE (name),
    FOREIGN KEY (parent_id) REFERENCES parent (id)
);
)";

    // Test DDL statements

    StringBuilder ddl;
    OrmTestParentModel.GetDDL(ddl);
    HE_EXPECT_EQ(ddl.Str(), ExpectedParentDDL);

    ddl.Clear();
    OrmTestChildModel.GetDDL(ddl);
    HE_EXPECT_EQ(ddl.Str(), ExpectedChildDDL);

    // TODO: Test FindOne/FindAll/Update/Upsert/Delete
}
