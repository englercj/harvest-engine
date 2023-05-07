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
HE_TEST(sqlite, orm_sql, _PickTable)
{
    constexpr auto c = DefineSchema(
        Table("parent",
            Column("id", &OrmTestParent::id, PrimaryKey()),
            Column("file_path", &OrmTestParent::filePath, Unique())),
        Table("child",
            Column("id", &OrmTestChild::id, PrimaryKey()),
            Column("parent_id", &OrmTestChild::parentId)),
        Index("idx_test", &OrmTestParent::sourcePath, &OrmTestParent::sourceSize));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, SchemaDef>);
    static_assert(T::ElementsType::Size == 3);

    using Parent = typename _PickTable<OrmTestParent, T::ElementsType>::Type;
    static_assert(IsSame<Parent, TypeListElement<0, T::ElementsType::ElementList>>);
    static_assert(IsSame<Parent, T::TableTypeFor<OrmTestParent>>);

    using Child = typename _PickTable<OrmTestChild, T::ElementsType>::Type;
    static_assert(IsSame<Child, TypeListElement<1, T::ElementsType::ElementList>>);
    static_assert(IsSame<Child, T::TableTypeFor<OrmTestChild>>);
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
HE_TEST(sqlite, orm, PragmaDef)
{
    constexpr auto c = Pragma("foreign_keys", true);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, PragmaExpr>);
    static_assert(IsSame<T::ValueType, bool>);
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
HE_TEST(sqlite, orm, ColumnRefCollection)
{
    constexpr auto c = Cols(&OrmTestParent::id, &OrmTestParent::sourcePath);

    using T = Decay<decltype(c)>;

    static_assert(IsSame<T::ColumnsType, Tuple<uint32_t OrmTestParent::*, String OrmTestParent::*>>);
    static_assert(T::ColumnsType::Size == 2);
    static_assert(TupleGet<0>(c.columns) == &OrmTestParent::id);
    static_assert(TupleGet<1>(c.columns) == &OrmTestParent::sourcePath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ColumnRefAndValue)
{
    constexpr auto c = Set(&OrmTestParent::id, 10);

    using T = Decay<decltype(c)>;

    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSame<T::ValueType, int>);
    static_assert(c.member == &OrmTestParent::id);
    static_assert(c.value == 10);
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
    constexpr auto c = IsNull(Col(&OrmTestParent::id));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, IsNullExpr>);
    static_assert(IsSame<T::ValueType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(c.value.member == &OrmTestParent::id);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, IsNotNullExpr)
{
    constexpr auto c = IsNotNull(Col(&OrmTestParent::id));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, IsNotNullExpr>);
    static_assert(IsSame<T::ValueType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(c.value.member == &OrmTestParent::id);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, WhereExpr)
{
    constexpr auto c = Where(Col(&OrmTestParent::id) == 10);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, WhereExpr>);
    static_assert(IsSame<T::ValueType, EqualExpr<ColumnRef<OrmTestParent, uint32_t>, int>>);
    static_assert(c.value.lhs.member == &OrmTestParent::id);
    static_assert(c.value.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, OrderByExpr)
{
    constexpr auto c = OrderBy(Col(&OrmTestParent::id));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, OrderByExpr>);
    static_assert(IsSame<T::ValueType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(c.value.member == &OrmTestParent::id);
    static_assert(c.orderBy == OrderByKind::None);
    static_assert(c.orderNullsBy == OrderNullsByKind::None);
    static_assert(c.collateName.IsEmpty());

    constexpr auto ca = c.Asc().NullsFirst();

    static_assert(IsSpecialization<T, OrderByExpr>);
    static_assert(IsSame<T::ValueType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(ca.value.member == &OrmTestParent::id);
    static_assert(ca.orderBy == OrderByKind::Asc);
    static_assert(ca.orderNullsBy == OrderNullsByKind::NullsFirst);
    static_assert(ca.collateName.IsEmpty());

    constexpr auto cd = c.Desc().NullsLast().Collate("test");

    static_assert(IsSpecialization<T, OrderByExpr>);
    static_assert(IsSame<T::ValueType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(cd.value.member == &OrmTestParent::id);
    static_assert(cd.orderBy == OrderByKind::Desc);
    static_assert(cd.orderNullsBy == OrderNullsByKind::NullsLast);
    static_assert(cd.collateName == StringView("test"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, MultiOrderByExpr)
{
    constexpr auto c = MultiOrderBy(OrderBy(Col(&OrmTestParent::id)).Collate("ccc"), OrderBy(Col(&OrmTestParent::filePath)).Asc());

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, MultiOrderByExpr>);
    static_assert(TupleGet<0>(c.args).value.member == &OrmTestParent::id);
    static_assert(TupleGet<0>(c.args).orderBy == OrderByKind::None);
    static_assert(TupleGet<0>(c.args).orderNullsBy == OrderNullsByKind::None);
    static_assert(TupleGet<0>(c.args).collateName == StringView("ccc"));

    static_assert(IsSpecialization<T, MultiOrderByExpr>);
    static_assert(TupleGet<1>(c.args).value.member == &OrmTestParent::filePath);
    static_assert(TupleGet<1>(c.args).orderBy == OrderByKind::Asc);
    static_assert(TupleGet<1>(c.args).orderNullsBy == OrderNullsByKind::None);
    static_assert(TupleGet<1>(c.args).collateName.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, GroupByExpr)
{
    constexpr auto c = GroupBy(Col(&OrmTestParent::id));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, GroupByExpr>);
    static_assert(IsSame<T::ArgsType, Tuple<ColumnRef<OrmTestParent, uint32_t>>>);
    static_assert(TupleGet<0>(c.args).member == &OrmTestParent::id);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, NotExpr)
{
    constexpr auto c = !(Col(&OrmTestParent::id) == 10);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, NotExpr>);
    static_assert(IsSame<T::ValueType, EqualExpr<ColumnRef<OrmTestParent, uint32_t>, int>>);
    static_assert(c.cond.lhs.member == &OrmTestParent::id);
    static_assert(c.cond.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, AndExpr)
{
    constexpr auto c = ((Col(&OrmTestParent::id) == 10) && (Col(&OrmTestParent::filePath) == "test"));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryCondition>);
    static_assert(T::Sql == StringView("AND"));
    static_assert(IsSame<T::LhsType, EqualExpr<ColumnRef<OrmTestParent, uint32_t>, int>>);
    static_assert(IsSame<T::RhsType, EqualExpr<ColumnRef<OrmTestParent, String>, const char*>>);
    static_assert(c.lhs.lhs.member == &OrmTestParent::id);
    static_assert(c.lhs.rhs == 10);
    static_assert(c.rhs.lhs.member == &OrmTestParent::filePath);
    static_assert(StringView(c.rhs.rhs) == StringView("test"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, OrExpr)
{
    constexpr auto c = ((Col(&OrmTestParent::id) == 10) || (Col(&OrmTestParent::filePath) == "test"));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryCondition>);
    static_assert(T::Sql == StringView("OR"));
    static_assert(IsSame<T::LhsType, EqualExpr<ColumnRef<OrmTestParent, uint32_t>, int>>);
    static_assert(IsSame<T::RhsType, EqualExpr<ColumnRef<OrmTestParent, String>, const char*>>);
    static_assert(c.lhs.lhs.member == &OrmTestParent::id);
    static_assert(c.lhs.rhs == 10);
    static_assert(c.rhs.lhs.member == &OrmTestParent::filePath);
    static_assert(StringView(c.rhs.rhs) == StringView("test"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, EqualExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) == 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryCondition>);
    static_assert(T::Sql == StringView("="));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, NotEqualExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) != 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryCondition>);
    static_assert(T::Sql == StringView("!="));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, GreaterThanExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) > 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryCondition>);
    static_assert(T::Sql == StringView(">"));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, GreaterThanOrEqualExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) >= 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryCondition>);
    static_assert(T::Sql == StringView(">="));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LesserThanExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) < 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryCondition>);
    static_assert(T::Sql == StringView("<"));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, LesserThanOrEqualExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) <= 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryCondition>);
    static_assert(T::Sql == StringView("<="));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ConcatExpr)
{
    constexpr auto c = Concat(Col(&OrmTestParent::id), 10);

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryOperator>);
    static_assert(T::Sql == StringView("||"));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, AddExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) + 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryOperator>);
    static_assert(T::Sql == StringView("+"));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, SubExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) - 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryOperator>);
    static_assert(T::Sql == StringView("-"));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, MulExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) * 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryOperator>);
    static_assert(T::Sql == StringView("*"));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, DivExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) / 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryOperator>);
    static_assert(T::Sql == StringView("/"));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, ModExpr)
{
    constexpr auto c = Col(&OrmTestParent::id) % 10;

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, _BinaryOperator>);
    static_assert(T::Sql == StringView("%"));
    static_assert(IsSame<T::LhsType, ColumnRef<OrmTestParent, uint32_t>>);
    static_assert(IsSame<T::RhsType, int>);
    static_assert(c.lhs.member == &OrmTestParent::id);
    static_assert(c.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, SelectObjectQuery)
{
    constexpr auto c = SelectObj<OrmTestParent>(Where(Col(&OrmTestParent::id) < 10));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, SelectObjectQuery>);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSame<T::ArgsType, Tuple<WhereExpr<LesserThanExpr<ColumnRef<OrmTestParent, uint32_t>, int>>>>);
    static_assert(TupleGet<0>(c.args).value.lhs.member == &OrmTestParent::id);
    static_assert(TupleGet<0>(c.args).value.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, SelectQuery)
{
    constexpr auto c = Select<OrmTestParent>(
        Cols(&OrmTestParent::id, &OrmTestParent::filePath),
        Where(Col(&OrmTestParent::id) < 10));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, SelectQuery>);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSame<T::ColumnsType, Tuple<uint32_t OrmTestParent::*, String OrmTestParent::*>>);
    static_assert(IsSame<T::ArgsType, Tuple<WhereExpr<LesserThanExpr<ColumnRef<OrmTestParent, uint32_t>, int>>>>);
    static_assert(TupleGet<0>(c.columns) == &OrmTestParent::id);
    static_assert(TupleGet<1>(c.columns) == &OrmTestParent::filePath);
    static_assert(TupleGet<0>(c.args).value.lhs.member == &OrmTestParent::id);
    static_assert(TupleGet<0>(c.args).value.rhs == 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, DeleteQuery)
{
    constexpr auto c = Delete<OrmTestParent>(Where(Col(&OrmTestParent::id) < 10));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, DeleteQuery>);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSame<T::WhereType, WhereExpr<LesserThanExpr<ColumnRef<OrmTestParent, uint32_t>, int>>>);
    static_assert(c.where.value.lhs.member == &OrmTestParent::id);
    static_assert(c.where.value.rhs == 10);

    constexpr auto c2 = Delete<OrmTestParent>();

    using T2 = Decay<decltype(c2)>;

    static_assert(IsSpecialization<T2, DeleteQuery>);
    static_assert(IsSame<T2::ObjectType, OrmTestParent>);
    static_assert(IsSame<T2::WhereType, WhereExpr<EqualExpr<int, int>>>);
    static_assert(c2.where.value.lhs == 1);
    static_assert(c2.where.value.rhs == 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, InsertObjectQuery)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, InsertQuery)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, UpdateObjectQuery)
{
    const auto c = UpdateObj(OrmTestParent{});

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, UpdateObjectQuery>);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, UpdateQuery)
{
    constexpr auto c = Update(Set(&OrmTestParent::id, 10), Set(&OrmTestParent::filePath, "test"));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, UpdateQuery>);
    static_assert(IsSame<T::ObjectType, OrmTestParent>);
    static_assert(IsSame<T::SettersType, Tuple<ColumnRefAndValue<OrmTestParent, uint32_t, int>, ColumnRefAndValue<OrmTestParent, String, const char*>>>);
    static_assert(IsSame<T::WhereType, WhereExpr<EqualExpr<int, int>>>);
    static_assert(TupleGet<0>(c.setters).member == &OrmTestParent::id);
    static_assert(TupleGet<0>(c.setters).value == 10);
    static_assert(TupleGet<1>(c.setters).member == &OrmTestParent::filePath);
    static_assert(StringView(TupleGet<1>(c.setters).value) == StringView("test"));
    static_assert(c.where.value.lhs == 1);
    static_assert(c.where.value.rhs == 1);

    constexpr auto c2 = Update(Where(Col(&OrmTestParent::id) == 5), Set(&OrmTestParent::id, 10), Set(&OrmTestParent::filePath, "test"));

    using T2 = Decay<decltype(c2)>;

    static_assert(IsSpecialization<T2, UpdateQuery>);
    static_assert(IsSame<T2::ObjectType, OrmTestParent>);
    static_assert(IsSame<T2::SettersType, Tuple<ColumnRefAndValue<OrmTestParent, uint32_t, int>, ColumnRefAndValue<OrmTestParent, String, const char*>>>);
    static_assert(IsSame<T2::WhereType, WhereExpr<EqualExpr<ColumnRef<OrmTestParent, uint32_t>, int>>>);
    static_assert(TupleGet<0>(c2.setters).member == &OrmTestParent::id);
    static_assert(TupleGet<0>(c2.setters).value == 10);
    static_assert(TupleGet<1>(c2.setters).member == &OrmTestParent::filePath);
    static_assert(StringView(TupleGet<1>(c2.setters).value) == StringView("test"));
    static_assert(c2.where.value.lhs.member == &OrmTestParent::id);
    static_assert(c2.where.value.rhs == 5);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, RawSqlQuery)
{
    constexpr auto c = RawSql("SELECT * FROM test");

    using T = Decay<decltype(c)>;

    static_assert(IsSame<T, RawSqlQuery>);
    static_assert(c.query == StringView("SELECT * FROM test"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, TableDefBase)
{
    TableDefBase m{ "test" };
    HE_UNUSED(m);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, TableDef)
{
    constexpr auto c = Table("child",
            Column("id", &OrmTestChild::id, PrimaryKey()),
            Column("parent_id", &OrmTestChild::parentId),
            Column("name", &OrmTestChild::name),
            Unique(&OrmTestChild::name),
            ForeignKey(&OrmTestChild::parentId).References(&OrmTestParent::id));

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, TableDef>);
    static_assert(IsSame<T::ObjectType, OrmTestChild>);
    static_assert(IsSame<T::ElementsType, Tuple<
        ColumnDef<OrmTestChild, uint32_t, PrimaryKeyConstraint<>>,
        ColumnDef<OrmTestChild, uint32_t>,
        ColumnDef<OrmTestChild, String>,
        UniqueConstraint<String OrmTestChild::*>,
        ForeignKeyConstraint<Tuple<uint32_t OrmTestChild::*>, Tuple<uint32_t OrmTestParent::*>>>>);

    static_assert(c.Name() == StringView("child"));
    static_assert(TupleGet<0>(c.Elements()).name == StringView("id"));
    static_assert(TupleGet<0>(c.Elements()).member == &OrmTestChild::id);
    static_assert(TupleGet<1>(c.Elements()).name == StringView("parent_id"));
    static_assert(TupleGet<1>(c.Elements()).member == &OrmTestChild::parentId);
    static_assert(TupleGet<2>(c.Elements()).name == StringView("name"));
    static_assert(TupleGet<2>(c.Elements()).member == &OrmTestChild::name);
    static_assert(TupleGet<0>(TupleGet<3>(c.Elements()).columns) == &OrmTestChild::name);
    static_assert(TupleGet<0>(TupleGet<4>(c.Elements()).columns) == &OrmTestChild::parentId);
    static_assert(TupleGet<0>(TupleGet<4>(c.Elements()).references) == &OrmTestParent::id);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(sqlite, orm, SchemaDef)
{
    constexpr auto c = DefineSchema(
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

    using T = Decay<decltype(c)>;

    static_assert(IsSpecialization<T, SchemaDef>);
    static_assert(IsSame<T::ElementsType, Tuple<
        TableDef<OrmTestParent,
            ColumnDef<OrmTestParent, uint32_t, PrimaryKeyConstraint<>>,
            ColumnDef<OrmTestParent, String, UniqueConstraint<>>,
            ColumnDef<OrmTestParent, uint32_t, DefaultConstraint<int>>,
            ColumnDef<OrmTestParent, SystemTime>,
            ColumnDef<OrmTestParent, uint32_t>,
            ColumnDef<OrmTestParent, String>,
            ColumnDef<OrmTestParent, SystemTime>,
            ColumnDef<OrmTestParent, uint32_t>,
            ColumnDef<OrmTestParent, uint32_t>>,
        TableDef<OrmTestChild,
            ColumnDef<OrmTestChild, uint32_t, PrimaryKeyConstraint<>>,
            ColumnDef<OrmTestChild, uint32_t>,
            ColumnDef<OrmTestChild, String>,
            UniqueConstraint<String OrmTestChild::*>,
            ForeignKeyConstraint<Tuple<uint32_t OrmTestChild::*>, Tuple<uint32_t OrmTestParent::*>>>,
        IndexDef<String OrmTestParent::*, uint32_t OrmTestParent::*>>>);

    static_assert(TupleGet<0>(c.Elements()).Name() == StringView("parent"));
    static_assert(TupleGet<1>(c.Elements()).Name() == StringView("child"));
    static_assert(TupleGet<2>(c.Elements()).name == StringView("idx_test"));

    static_assert(TupleGet<0>(TupleGet<0>(c.Elements()).Elements()).name == StringView("id"));
    static_assert(TupleGet<0>(TupleGet<0>(c.Elements()).Elements()).member == &OrmTestParent::id);
    static_assert(TupleGet<1>(TupleGet<0>(c.Elements()).Elements()).name == StringView("file_path"));
    static_assert(TupleGet<1>(TupleGet<0>(c.Elements()).Elements()).member == &OrmTestParent::filePath);
    static_assert(TupleGet<2>(TupleGet<0>(c.Elements()).Elements()).name == StringView("file_path_depth"));
    static_assert(TupleGet<2>(TupleGet<0>(c.Elements()).Elements()).member == &OrmTestParent::filePathDepth);
    static_assert(TupleGet<3>(TupleGet<0>(c.Elements()).Elements()).name == StringView("file_write_time"));
    static_assert(TupleGet<3>(TupleGet<0>(c.Elements()).Elements()).member == &OrmTestParent::fileWriteTime);
    static_assert(TupleGet<4>(TupleGet<0>(c.Elements()).Elements()).name == StringView("file_size"));
    static_assert(TupleGet<4>(TupleGet<0>(c.Elements()).Elements()).member == &OrmTestParent::fileSize);
    static_assert(TupleGet<5>(TupleGet<0>(c.Elements()).Elements()).name == StringView("source_path"));
    static_assert(TupleGet<5>(TupleGet<0>(c.Elements()).Elements()).member == &OrmTestParent::sourcePath);
    static_assert(TupleGet<6>(TupleGet<0>(c.Elements()).Elements()).name == StringView("source_write_time"));
    static_assert(TupleGet<6>(TupleGet<0>(c.Elements()).Elements()).member == &OrmTestParent::sourceWriteTime);
    static_assert(TupleGet<7>(TupleGet<0>(c.Elements()).Elements()).name == StringView("source_size"));
    static_assert(TupleGet<7>(TupleGet<0>(c.Elements()).Elements()).member == &OrmTestParent::sourceSize);
    static_assert(TupleGet<8>(TupleGet<0>(c.Elements()).Elements()).name == StringView("scan_token"));
    static_assert(TupleGet<8>(TupleGet<0>(c.Elements()).Elements()).member == &OrmTestParent::scanToken);

    static_assert(TupleGet<0>(TupleGet<1>(c.Elements()).Elements()).name == StringView("id"));
    static_assert(TupleGet<0>(TupleGet<1>(c.Elements()).Elements()).member == &OrmTestChild::id);
    static_assert(TupleGet<1>(TupleGet<1>(c.Elements()).Elements()).name == StringView("parent_id"));
    static_assert(TupleGet<1>(TupleGet<1>(c.Elements()).Elements()).member == &OrmTestChild::parentId);
    static_assert(TupleGet<2>(TupleGet<1>(c.Elements()).Elements()).name == StringView("name"));
    static_assert(TupleGet<2>(TupleGet<1>(c.Elements()).Elements()).member == &OrmTestChild::name);
}
