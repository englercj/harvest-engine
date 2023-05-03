// Copyright Chad Engler

#include "fixtures.h"

#include "he/sqlite/orm.h"

#include "he/core/clock.h"
#include "he/core/string.h"
#include "he/core/string_builder.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/test.h"

using namespace he;
using namespace he::sqlite;

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, _CountTuple)
{
    static_assert(_CountTuple<Tuple<int, float, int, int>, IsIntTest> == 3);
    static_assert(_CountTuple<Tuple<int, float, int, int>, IsFloatTest> == 1);
    static_assert(_CountTuple<Tuple<double, float, OrmTestParent>, IsIntTest> == 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ColumnsObjectType)
{
    static_assert(IsSame<ColumnsObjectType<decltype(&OrmTestParent::id), decltype(&OrmTestParent::filePath), decltype(&OrmTestParent::filePathDepth)>, OrmTestParent>);
    static_assert(IsSame<ColumnsObjectType<decltype(&OrmTestChild::id), decltype(&OrmTestChild::parentId)>, OrmTestChild>);

    // Column mismatch, does not compile
    //static_assert(IsSame<ColumnsObjectType<decltype(&OrmTestParent::id), decltype(&OrmTestChild::parentId)>, OrmTestParent>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, PrimaryKeyConstraint)
{
    constexpr auto m = Table("test",
        Column("id", &OrmTestParent::id),
        Column("file_path", &OrmTestParent::filePath));

    constexpr auto c = PrimaryKey(&OrmTestParent::id, &OrmTestParent::filePath);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, PrimaryKeyConstraint>);
    static_assert(IsSpecialization<T::ColumnsType, Tuple>);
    static_assert(T::ColumnsType::Size == 2);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, UniqueConstraint)
{
    constexpr auto m = Table("test",
        Column("id", &OrmTestParent::id),
        Column("file_path", &OrmTestParent::filePath));

    constexpr auto c = Unique(&OrmTestParent::id, &OrmTestParent::filePath);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, UniqueConstraint>);
    static_assert(IsSpecialization<T::ColumnsType, Tuple>);
    static_assert(T::ColumnsType::Size == 2);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ForeignKeyConstraint)
{
    constexpr auto parent = Table("parent", Column("id", &OrmTestParent::id));
    constexpr auto child = Table("child",
        Column("id", &OrmTestChild::id),
        Column("parent_id", &OrmTestChild::parentId));

    constexpr auto c = ForeignKey(&OrmTestChild::parentId).References(&OrmTestParent::id);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, ForeignKeyConstraint>);
    static_assert(IsSpecialization<T::ColumnsType, Tuple>);
    static_assert(IsSpecialization<T::ReferencesType, Tuple>);
    static_assert(T::ColumnsType::Size == 1);
    static_assert(T::ReferencesType::Size == 1);
    static_assert(IsSame<T::ObjectType, OrmTestChild>);
    static_assert(IsSame<T::ReferencedObjectType, OrmTestParent>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, DefaultConstraint)
{
    constexpr auto c = Default(10);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, DefaultConstraint>);
    static_assert(IsSame<T::ValueType, int>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, NotNullConstraint)
{
    constexpr auto c = NotNull();

    using T = Decay<decltype(c)>;

    static_assert(IsSame<T, NotNullConstraint>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ColumnDef)
{
    constexpr auto c = Column("id", &OrmTestParent::id, Unique(), NotNull());

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, ColumnDef>);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSame<T::ValueType, uint32_t>);
    static_assert(IsSpecialization<T::ConstraintsType, Tuple>);
    static_assert(T::ConstraintsType::Size == 2);
    static_assert(IsSame<TupleElement<0, T::ConstraintsType>, UniqueConstraint<>>);
    static_assert(IsSame<TupleElement<1, T::ConstraintsType>, NotNullConstraint>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ColumnDef_CustomDataType)
{
    constexpr auto c = Column("custom_value", &OrmTestParent::custom);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, ColumnDef>);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSame<T::ValueType, OrmCustomValue>);
    static_assert(IsSpecialization<T::ConstraintsType, Tuple>);
    static_assert(T::ConstraintsType::Size == 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IndexDef)
{
    constexpr auto m = Table("parent",
        Column("source_path", &OrmTestParent::sourcePath),
        Column("source_size", &OrmTestParent::sourceSize));

    constexpr auto c = Index("idx_test", &OrmTestParent::sourcePath, &OrmTestParent::sourceSize);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, IndexDef>);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSpecialization<T::ColumnsType, Tuple>);
    static_assert(T::ColumnsType::Size == 2);
    static_assert(!c.unique);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IndexDef_Unique)
{
    constexpr auto m = Table("parent",
        Column("source_path", &OrmTestParent::sourcePath),
        Column("source_size", &OrmTestParent::sourceSize));

    constexpr auto c = UniqueIndex("idx_test", &OrmTestParent::sourcePath, &OrmTestParent::sourceSize);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, IndexDef>);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSpecialization<T::ColumnsType, Tuple>);
    static_assert(T::ColumnsType::Size == 2);
    static_assert(c.unique);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ColumnRef)
{
    constexpr auto c = Col(&OrmTestParent::id);

    using T = Decay<decltype(c)>;

    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSame<T::ValueType, uint32_t>);
    static_assert(c.member == &OrmTestParent::id);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LimitExpr)
{
    constexpr auto c = Limit(1);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, LimitExpr>);
    static_assert(IsSame<T::LimitType, int>);
    static_assert(IsSame<T::OffsetType, void>);
    static_assert(IsSame<T::OffsetStorageType, int>);
    static_assert(!T::HasOffset);
    static_assert(c.limit == 1);
    static_assert(c.offset == 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LimitExpr_Offset)
{
    constexpr auto c = Limit(1, 10);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, LimitExpr>);
    static_assert(IsSame<T::LimitType, int>);
    static_assert(IsSame<T::OffsetType, int>);
    static_assert(IsSame<T::OffsetStorageType, int>);
    static_assert(T::HasOffset);
    static_assert(c.limit == 1);
    static_assert(c.offset == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IsNullExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IsNotNullExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, WhereExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, OrderByExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, MultiOrderByExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, GroupByExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, NotExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, AndExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, OrExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, EqualExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, NotEqualExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, GreaterThanExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, GreaterThanOrEqualExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LesserThanExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LesserThanOrEqualExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ConcatExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, AddExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, SubExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, MulExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, DivExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ModExpr)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, TableDefBase_Construct)
{
    TableDefBase m{ "test" };
    HE_UNUSED(m);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, TableDef)
{
    // TODO
}
