// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/concepts.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/tuple.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

namespace he::sqlite
{
    // --------------------------------------------------------------------------------------------
    // Forward declarations

    template <typename T, typename... Elements>
    class TableDef;

    // --------------------------------------------------------------------------------------------
    // Useful type traits

    template <template <typename> typename Pred, typename>
    struct _CountTupleHelper;

    template <template <typename> typename Pred, typename... Ts>
    struct _CountTupleHelper<Pred, Tuple<Ts...>>
    {
        static constexpr uint32_t Value = (0 + ... + (Pred<Ts>::Value ? 1 : 0));
    };

    template <typename T, template <typename> typename Pred>
    inline constexpr uint32_t _CountTuple = _CountTupleHelper<Pred, T>::Value;

    // Get the object type of a series of columns
    template <typename... Columns>
    struct _ColumnsObjectType
    {
        static_assert((IsMemberObjectPointer<Columns> && ...), "All columns must be pointers to object members.");
        static_assert(IsAllSame<MemberPointerObjectType<Columns>...>, "All columns must refer to members of the same object type.");
        using Type = MemberPointerObjectType<TupleElement<0, Tuple<Columns...>>>;
    };

    template <>
    struct _ColumnsObjectType<>
    {
        using Type = void;
    };

    template <typename... Columns>
    using ColumnsObjectType = typename _ColumnsObjectType<Columns...>::Type;

    // Helpers to select table types from schema elements based on the ObjectType
    template <typename T, typename U>
    inline constexpr bool _TableIsObjectType = false;

    template <typename T, typename U, typename... Elements>
    inline constexpr bool _TableIsObjectType<T, TableDef<U, Elements...>> = IsSame<T, U>;

    template <typename T, typename... Ts>
    static constexpr uint32_t _FindTableIndexForObjectType()
    {
        constexpr uint32_t size = sizeof...(Ts);
        constexpr bool found[size] = { _TableIsObjectType<T, Ts>... };
        uint32_t n = size;
        for (uint32_t i = 0; i < size; ++i)
        {
            if (found[i])
            {
                if (n < size)
                    return size;
                n = i;
            }
        }
        return n;
    }

    template <typename T, typename MTuple>
    struct _PickTable;

    template <typename T, typename... Ts>
    struct _PickTable<T, Tuple<Ts...>>
    {
        static constexpr uint32_t TableIndex = _FindTableIndexForObjectType<T, Ts...>();
        static_assert(TableIndex != sizeof...(Ts), "No table, or multiple tables, found with T as object type.");

        using Type = TupleElement<TableIndex, Tuple<Ts...>>;
    };

    // --------------------------------------------------------------------------------------------
    // Constraints

    // TODO: CHECK, COLLATE, GENERATED ALWAYS

    struct _Constraint {};

    enum class OnConflictKind : uint8_t
    {
        None,
        Rollback,
        Abort,
        Fail,
        Ignore,
        Replace,
    };

    enum class OrderByKind : uint8_t
    {
        None,
        Asc,
        Desc,
    };

    enum class OrderNullsByKind : uint8_t
    {
        None,
        NullsFirst,
        NullsLast,
    };

    struct PrimaryKeyConstraintBase : _Constraint
    {
        bool autoIncrement{ false };
        OrderByKind orderBy{ OrderByKind::None };
        OnConflictKind onConflict{ OnConflictKind::None };
    };

    template <typename... Columns>
    struct PrimaryKeyConstraint : PrimaryKeyConstraintBase
    {
        using ColumnsType = Tuple<Columns...>;
        using ObjectType = ColumnsObjectType<Columns...>;

        constexpr PrimaryKeyConstraint(ColumnsType columns) : columns(Move(columns)) {}

        constexpr PrimaryKeyConstraint Asc() const { auto r = *this; r.orderBy = OrderByKind::Asc; return r; }
        constexpr PrimaryKeyConstraint Desc() const { auto r = *this; r.orderBy = OrderByKind::Desc; return r; }
        constexpr PrimaryKeyConstraint AutoIncrement() const { auto r = *this; r.autoIncrement = true; return r; }

        constexpr PrimaryKeyConstraint OnConflictRollback() const { auto r = *this; r.onConflict = OnConflictKind::Rollback; return r; }
        constexpr PrimaryKeyConstraint OnConflictAbort() const { auto r = *this; r.onConflict = OnConflictKind::Abort; return r; }
        constexpr PrimaryKeyConstraint OnConflictFail() const { auto r = *this; r.onConflict = OnConflictKind::Fail; return r; }
        constexpr PrimaryKeyConstraint OnConflictIgnore() const { auto r = *this; r.onConflict = OnConflictKind::Ignore; return r; }
        constexpr PrimaryKeyConstraint OnConflictReplace() const { auto r = *this; r.onConflict = OnConflictKind::Replace; return r; }

        ColumnsType columns;
    };

    struct UniqueConstraintBase : _Constraint
    {
        OnConflictKind onConflict{ OnConflictKind::None };
    };

    template <typename... Columns>
    struct UniqueConstraint : UniqueConstraintBase
    {
        using ColumnsType = Tuple<Columns...>;
        using ObjectType = ColumnsObjectType<Columns...>;

        constexpr UniqueConstraint(ColumnsType columns) : columns(Move(columns)) {}

        constexpr UniqueConstraint OnConflictRollback() const { auto r = *this; r.onConflict = OnConflictKind::Rollback; return r; }
        constexpr UniqueConstraint OnConflictAbort() const { auto r = *this; r.onConflict = OnConflictKind::Abort; return r; }
        constexpr UniqueConstraint OnConflictFail() const { auto r = *this; r.onConflict = OnConflictKind::Fail; return r; }
        constexpr UniqueConstraint OnConflictIgnore() const { auto r = *this; r.onConflict = OnConflictKind::Ignore; return r; }
        constexpr UniqueConstraint OnConflictReplace() const { auto r = *this; r.onConflict = OnConflictKind::Replace; return r; }

        ColumnsType columns;
    };

    enum class FkActionKind : uint8_t
    {
        None,
        Cascade,
        NoAction,
        Restrict,
        SetDefault,
        SetNull,
    };

    struct ForeignKeyConstraintBase : _Constraint
    {
        FkActionKind onDelete{ FkActionKind::None };
        FkActionKind onUpdate{ FkActionKind::None };
    };

    template <typename C, typename R>
    struct ForeignKeyConstraint;

    template <typename... Columns, typename... References>
    struct ForeignKeyConstraint<Tuple<Columns...>, Tuple<References...>> : ForeignKeyConstraintBase
    {
        using ColumnsType = Tuple<Columns...>;
        using ReferencesType = Tuple<References...>;

        using ObjectType = ColumnsObjectType<Columns...>;
        using ReferencedObjectType = ColumnsObjectType<References...>;

        static_assert(ColumnsType::Size == ReferencesType::Size, "The number of source columns and referenced columns must be the same.");

        constexpr ForeignKeyConstraint(ColumnsType columns, ReferencesType references)
            : columns(Move(columns))
            , references(Move(references))
        {}

        constexpr ForeignKeyConstraint OnDeleteCascade() const { auto r = *this; r.onDelete = FkActionKind::Cascade; return r; }
        constexpr ForeignKeyConstraint OnDeleteNoAction() const { auto r = *this; r.onDelete = FkActionKind::NoAction; return r; }
        constexpr ForeignKeyConstraint OnDeleteRestrict() const { auto r = *this; r.onDelete = FkActionKind::Restrict; return r; }
        constexpr ForeignKeyConstraint OnDeleteSetDefault() const { auto r = *this; r.onDelete = FkActionKind::SetDefault; return r; }
        constexpr ForeignKeyConstraint OnDeleteSetNull() const { auto r = *this; r.onDelete = FkActionKind::SetNull; return r; }

        constexpr ForeignKeyConstraint OnUpdateCascade() const { auto r = *this; r.onDelete = FkActionKind::Cascade; return r; }
        constexpr ForeignKeyConstraint OnUpdateNoAction() const { auto r = *this; r.onDelete = FkActionKind::NoAction; return r; }
        constexpr ForeignKeyConstraint OnUpdateRestrict() const { auto r = *this; r.onDelete = FkActionKind::Restrict; return r; }
        constexpr ForeignKeyConstraint OnUpdateSetDefault() const { auto r = *this; r.onDelete = FkActionKind::SetDefault; return r; }
        constexpr ForeignKeyConstraint OnUpdateSetNull() const { auto r = *this; r.onDelete = FkActionKind::SetNull; return r; }

        ColumnsType columns;
        ReferencesType references;
    };

    template <typename... Columns>
    struct ForeignKeyConstraintHelper
    {
        using ColumnsType = Tuple<Columns...>;

        template <typename... References> requires((IsMemberObjectPointer<References> && ...))
        constexpr ForeignKeyConstraint<ColumnsType, Tuple<References...>> References(References... references)
        {
            return { Move(this->columns), MakeTuple(Forward<References>(references)...) };
        }

        ColumnsType columns;
    };

    template <typename T>
    struct DefaultConstraint : _Constraint
    {
        using ValueType = T;

        constexpr DefaultConstraint(ValueType&& value) : value(Move(value)) {}

        ValueType value;
    };

    struct NotNullConstraint : _Constraint
    {
        constexpr NotNullConstraint OnConflictRollback() const { auto r = *this; r.onConflict = OnConflictKind::Rollback; return r; }
        constexpr NotNullConstraint OnConflictAbort() const { auto r = *this; r.onConflict = OnConflictKind::Abort; return r; }
        constexpr NotNullConstraint OnConflictFail() const { auto r = *this; r.onConflict = OnConflictKind::Fail; return r; }
        constexpr NotNullConstraint OnConflictIgnore() const { auto r = *this; r.onConflict = OnConflictKind::Ignore; return r; }
        constexpr NotNullConstraint OnConflictReplace() const { auto r = *this; r.onConflict = OnConflictKind::Replace; return r; }

        OnConflictKind onConflict{ OnConflictKind::None };
    };

    template <typename T>
    concept ColumnConstraint = IsSpecialization<T, PrimaryKeyConstraint>
        || IsSpecialization<T, UniqueConstraint>
        || IsSpecialization<T, DefaultConstraint>
        || IsSame<T, NotNullConstraint>;

    template <typename T>
    concept TableConstraint = IsSpecialization<T, PrimaryKeyConstraint>
        || IsSpecialization<T, UniqueConstraint>
        || IsSpecialization<T, ForeignKeyConstraint>;

    template <typename T>
    concept Constraint = ColumnConstraint<T> || TableConstraint<T>;

    template <typename T>
    struct IsColumnConstraint { static constexpr bool Value = ColumnConstraint<T>; };

    template <typename T>
    struct IsTableConstraint { static constexpr bool Value = TableConstraint<T>; };

    template <typename T>
    struct IsConstraint { static constexpr bool Value = Constraint<T>; };

    template <typename T>
    struct IsPrimaryKeyConstraint { static constexpr bool Value = IsSpecialization<T, PrimaryKeyConstraint>; };

    template <typename... Columns> requires(IsMemberObjectPointer<Columns> && ...)
    constexpr PrimaryKeyConstraint<Columns...> PrimaryKey(Columns... columns)
    {
        return { MakeTuple(Forward<Columns>(columns)...) };
    }

    template <typename... Columns> requires(IsMemberObjectPointer<Columns> && ...)
    constexpr UniqueConstraint<Columns...> Unique(Columns... columns)
    {
        return { MakeTuple(Forward<Columns>(columns)...) };
    }

    template <typename... Columns> requires(IsMemberObjectPointer<Columns> && ...)
    constexpr ForeignKeyConstraintHelper<Columns...> ForeignKey(Columns... columns)
    {
        return { MakeTuple(Forward<Columns>(columns)...) };
    }

    template <typename T>
    constexpr DefaultConstraint<T> Default(T value)
    {
        return { Move(value) };
    }

    constexpr NotNullConstraint NotNull()
    {
        return { };
    }

    // --------------------------------------------------------------------------------------------
    // Column Definition

    struct ColumnDefBase
    {
        StringView name;
    };

    template <typename T, typename U, typename... Constraints>
    struct ColumnDef : ColumnDefBase
    {
        using ObjectType = T;
        using ValueType = U;
        using ConstraintsType = Tuple<Constraints...>;

        constexpr ColumnDef(StringView name, ValueType ObjectType::* member, ConstraintsType constraints)
            : ColumnDefBase(name)
            , member(member)
            , constraints(constraints)
        {}

        ValueType ObjectType::* member{ nullptr };
        ConstraintsType constraints;
    };

    template <typename T>
    struct IsColumnDef { static constexpr bool Value = IsSpecialization<T, ColumnDef>; };

    template <typename T, typename U, ColumnConstraint... Constraints>
    constexpr ColumnDef<T, U, Constraints...> Column(StringView name, U T::* member, Constraints... constraints)
    {
        return { name, member, MakeTuple(Forward<Constraints>(constraints)...) };
    }

    // --------------------------------------------------------------------------------------------
    // Index Definition

    struct IndexDefBase
    {
        StringView name;
        bool unique{ false };
    };

    template <typename... Columns>
    struct IndexDef : IndexDefBase
    {
        using ColumnsType = Tuple<Columns...>;
        using ObjectType = ColumnsObjectType<Columns...>;

        constexpr IndexDef(StringView name, bool unique, ColumnsType columns)
            : IndexDefBase(name, unique)
            , columns(Move(columns))
        {}

        // TODO: indexed-column traits (collate, asc, desc)
        // TODO: where expr

        ColumnsType columns;
    };

    template <typename T>
    struct IsIndexDef { static constexpr bool Value = IsSpecialization<T, IndexDef>; };

    template <typename... Columns> requires(IsMemberObjectPointer<Columns> && ...)
    constexpr IndexDef<Columns...> Index(StringView name, Columns... columns)
    {
        return { name, false, MakeTuple(Forward<Columns>(columns)...) };
    }

    template <typename... Columns> requires(IsMemberObjectPointer<Columns> && ...)
    constexpr IndexDef<Columns...> UniqueIndex(StringView name, Columns... columns)
    {
        return { name, true, MakeTuple(Forward<Columns>(columns)...) };
    }

    // --------------------------------------------------------------------------------------------
    // Pragma Definition

    struct PragmaDefBase
    {
        StringView name;
    };

    template <typename T>
    struct PragmaDef : PragmaDefBase
    {
        using ValueType = T;

        constexpr PragmaDef(StringView name, T&& value) : value(Move(value)) {}

        T value;
    };

    template <typename T>
    struct IsPragmaDef { static constexpr bool Value = IsSpecialization<T, PragmaDef>; };

    template <typename T>
    constexpr PragmaDef<T> Pragma(StringView name, T&& value)
    {
        return { name, Forward<T>(value) };
    }

    // --------------------------------------------------------------------------------------------
    // Expressions

    // TODO: collate, order by, group by, like, glob, join/on
    // functions: sum/hex/lower/etc?

    // Column reference
    template <typename T, typename U>
    struct ColumnRef
    {
        using ObjectType = T;
        using ValueType = U;

        ValueType ObjectType::* member;
    };

    // Limit/Offset expression
    template <typename T, typename U = void>
    struct LimitExpr
    {
        using LimitType = T;
        using OffsetType = U;
        using OffsetStorageType = Conditional<IsSame<U, void>, int, U>;

        static constexpr bool HasOffset = !IsSame<OffsetType, void>;

        constexpr LimitExpr(LimitType limit) : limit(Move(limit)), offset() {}
        constexpr LimitExpr(LimitType limit, OffsetStorageType offset) : limit(Move(limit)), offset(Move(offset)) {}

        LimitType limit;
        OffsetStorageType offset;
    };

    // Unary expressions
    template <typename T>
    struct _UnaryExpr
    {
        using ValueType = T;

        constexpr _UnaryExpr(T value) : value(Move(value)) {}

        T value;
    };

    template <typename T>
    struct IsNullExpr : _UnaryExpr<T>
    {
        using _UnaryExpr<T>::_UnaryExpr;
    };

    template <typename T>
    struct IsNotNullExpr : _UnaryExpr<T>
    {
        using _UnaryExpr<T>::_UnaryExpr;
    };

    template <typename T>
    struct WhereExpr : _UnaryExpr<T>
    {
        using _UnaryExpr<T>::_UnaryExpr;
    };

    struct OrderByExprBase
    {
        OrderByKind orderBy{ OrderByKind::None };
        OrderNullsByKind orderNullsBy{ OrderNullsByKind::None };
        StringView collateName{};
    };

    template <typename T>
    struct OrderByExpr : OrderByExprBase, _UnaryExpr<T>
    {
        using _UnaryExpr<T>::_UnaryExpr;

        constexpr OrderByExpr Asc() const { auto r = *this; r.orderBy = OrderByKind::Asc; return r; }
        constexpr OrderByExpr Desc() const { auto r = *this; r.orderBy = OrderByKind::Desc; return r; }

        constexpr OrderByExpr NullsFirst() const { auto r = *this; r.orderNullsBy = OrderNullsByKind::NullsFirst; return r; }
        constexpr OrderByExpr NullsLast() const { auto r = *this; r.orderNullsBy = OrderNullsByKind::NullsLast; return r; }

        constexpr OrderByExpr Collate(StringView name) const { auto r = *this; r.collateName = name; return r; }
    };

    template <typename... Args>
    struct MultiOrderByExpr
    {
        using ArgsType = Tuple<Args...>;

        constexpr MultiOrderByExpr(ArgsType&& args) : args(Move(args)) {}

        ArgsType args;
    };

    template <typename... Args>
    struct GroupByExpr
    {
        using ArgsType = Tuple<Args...>;

        constexpr GroupByExpr(ArgsType&& args) : args(Move(args)) {}

        // TODO: HAVING

        ArgsType args;
    };

    // Condition Expressions
    struct _ConditionBase {};

    template <typename T>
    struct NotExpr : _ConditionBase
    {
        constexpr NotExpr(T cond) : cond(Move(cond)) {}

        T cond;
    };

    template <typename L, typename R, typename S>
    struct _BinaryCondition : _ConditionBase
    {
        static constexpr StringView Sql = S::Sql;

        constexpr _BinaryCondition(L lhs, R rhs) : lhs(Move(lhs)), rhs(Move(rhs)) {}

        L lhs;
        R rhs;
    };

    struct AndExpr_Sql { static constexpr StringView Sql = "AND"; };
    template <typename L, typename R> using AndExpr = _BinaryCondition<L, R, AndExpr_Sql>;

    struct OrExpr_Sql { static constexpr StringView Sql = "OR"; };
    template <typename L, typename R> using OrExpr = _BinaryCondition<L, R, OrExpr_Sql>;

    struct EqualExpr_Sql { static constexpr StringView Sql = "="; };
    template <typename L, typename R> using EqualExpr = _BinaryCondition<L, R, EqualExpr_Sql>;

    struct NotEqualExpr_Sql { static constexpr StringView Sql = "!="; };
    template <typename L, typename R> using NotEqualExpr = _BinaryCondition<L, R, NotEqualExpr_Sql>;

    struct GreaterThanExpr_Sql { static constexpr StringView Sql = ">"; };
    template <typename L, typename R> using GreaterThanExpr = _BinaryCondition<L, R, GreaterThanExpr_Sql>;

    struct GreaterThanOrEqualExpr_Sql { static constexpr StringView Sql = ">="; };
    template <typename L, typename R> using GreaterThanOrEqualExpr = _BinaryCondition<L, R, GreaterThanOrEqualExpr_Sql>;

    struct LesserThanExpr_Sql { static constexpr StringView Sql = "<"; };
    template <typename L, typename R> using LesserThanExpr = _BinaryCondition<L, R, LesserThanExpr_Sql>;

    struct LesserThanOrEqualExpr_Sql { static constexpr StringView Sql = "<="; };
    template <typename L, typename R> using LesserThanOrEqualExpr = _BinaryCondition<L, R, LesserThanOrEqualExpr_Sql>;

    // Operator Expressions
    struct _OperatorBase {};

    template <typename L, typename R, typename S>
    struct _BinaryOperator : _OperatorBase
    {
        static constexpr StringView Sql = S::Sql;

        constexpr _BinaryOperator(L lhs, R rhs) : lhs(Move(lhs)), rhs(Move(rhs)) {}

        L lhs;
        R rhs;
    };

    struct ConcatExpr_Sql { static constexpr StringView Sql = "||"; };
    template <typename L, typename R> using ConcatExpr = _BinaryOperator<L, R, ConcatExpr_Sql>;

    struct AddExpr_Sql { static constexpr StringView Sql = "+"; };
    template <typename L, typename R> using AddExpr = _BinaryOperator<L, R, AddExpr_Sql>;

    struct SubExpr_Sql { static constexpr StringView Sql = "-"; };
    template <typename L, typename R> using SubExpr = _BinaryOperator<L, R, SubExpr_Sql>;

    struct MulExpr_Sql { static constexpr StringView Sql = "*"; };
    template <typename L, typename R> using MulExpr = _BinaryOperator<L, R, MulExpr_Sql>;

    struct DivExpr_Sql { static constexpr StringView Sql = "/"; };
    template <typename L, typename R> using DivExpr = _BinaryOperator<L, R, DivExpr_Sql>;

    struct ModExpr_Sql { static constexpr StringView Sql = "%"; };
    template <typename L, typename R> using ModExpr = _BinaryOperator<L, R, ModExpr_Sql>;

    // Some traits to help detect types

    template <typename T> inline constexpr bool IsColumnRef = IsSpecialization<T, ColumnRef>;
    template <typename T> inline constexpr bool IsCondition = IsBaseOf<_ConditionBase, T>;
    template <typename T> inline constexpr bool IsOperator = IsSpecialization<_OperatorBase, T>;

    template <typename T>
    concept QueryCondition = IsSpecialization<T, WhereExpr>
        || IsSpecialization<T, GroupByExpr>
        || IsSpecialization<T, OrderByExpr>
        || IsSpecialization<T, LimitExpr>;

    // Helper functions and operators

    template <typename T, typename U>
    constexpr ColumnRef<T, U> Col(U T::* member) { return { member }; }

    template <typename T>
    constexpr LimitExpr<T> Limit(T value) { return { Move(value) }; }

    template <typename T, typename U>
    constexpr LimitExpr<T, U> Limit(T value, U offset) { return { Move(value), Move(offset) }; }

    template <typename T>
    constexpr IsNullExpr<T> IsNull(T expr) { return { Move(expr) }; }

    template <typename T>
    constexpr IsNotNullExpr<T> IsNotNull(T expr) { return { Move(expr) }; }

    template <typename T>
    constexpr WhereExpr<T> Where(T expr) { return { Move(expr) }; }

    template <typename T>
    constexpr OrderByExpr<T> OrderBy(T expr) { return { Move(expr) }; }

    template <typename... Args>
    constexpr MultiOrderByExpr<Args...> MultiOrderBy(Args&&... args) { return { MakeTuple(Forward<Args>(args)...) }; }

    template <typename... Args>
    constexpr GroupByExpr<Args...> GroupBy(Args&&... args) { return { MakeTuple(Forward<Args>(args)...) }; }

    template <typename L, typename R>
    constexpr ConcatExpr<L, R> Concat(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename T> requires(IsBaseOf<_ConditionBase, T>)
    constexpr NotExpr<T> operator!(T arg) { return { Move(arg) }; }

    template <typename L, typename R> requires(IsBaseOf<_ConditionBase, L> || IsBaseOf<_ConditionBase, R>)
    constexpr AndExpr<L, R> operator&&(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsBaseOf<_ConditionBase, L> || IsBaseOf<_ConditionBase, R>)
    constexpr OrExpr<L, R> operator||(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr EqualExpr<L, R> operator==(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr NotEqualExpr<L, R> operator!=(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr GreaterThanExpr<L, R> operator>(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr GreaterThanOrEqualExpr<L, R> operator>=(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr LesserThanExpr<L, R> operator<(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr LesserThanOrEqualExpr<L, R> operator<=(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr AddExpr<L, R> operator+(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr SubExpr<L, R> operator-(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr MulExpr<L, R> operator*(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr DivExpr<L, R> operator/(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(IsSpecialization<L, ColumnRef> || IsSpecialization<R, ColumnRef>)
    constexpr ModExpr<L, R> operator%(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    // --------------------------------------------------------------------------------------------
    // Queries

    struct _QueryBase {};

    template <typename T>
    struct InsertObjectQuery : _QueryBase
    {
        using ObjectType = T;

        const ObjectType& value;
    };

    template <typename T, typename Columns, typename Values>
    struct InsertQuery : _QueryBase
    {
        using ObjectType = T;
        using ColumnsType = Columns;
        using ValuesType = Values;

        static_assert(!IsSpecialization<ValuesType, Tuple> || ColumnsType::Size == ValuesType::Size, "The number of columns and values must be the same.");

        ColumnsTypes columns;
        ValuesType values;
    };

    template <typename... Columns>
    struct InsertQueryHelper
    {
        using ObjectType = ColumnsObjectType<Columns...>;
        using ColumnsType = Tuple<Columns...>;

        constexpr InsertQuery<ObjectType, ColumnsType, const ObjectType&> Values(const ObjectType& object)
        {
            return { Move(this->columns), object } };
        }

        template <typename... Values>
        constexpr InsertQuery<ObjectType, ColumnsType, Tuple<Values...>> Values(Values&&... values)
        {
            return { Move(this->columns), MakeTuple(Forward<Values>(values)...) };
        }

        ColumnsType columns;
    };

    // struct SelectQuery : _QueryBase
    // {

    // };

    // struct DeleteQuery : _QueryBase
    // {

    // };

    // struct UpdateQuery : _QueryBase
    // {

    // };

    template <typename T> inline constexpr bool IsQuery = IsBaseOf<_QueryBase, T>;

    template <typename T> concept Query = IsQuery<T>;

    template <typename T>
    constexpr InsertObjectQuery<T> Insert(const T& obj) { return { obj }; }

    template <typename... Columns> requires((IsMemberObjectPointer<Columns> && ...))
    constexpr InsertQueryHelper<Columns...> Insert(Columns... columns) { return { MakeTuple(Forward<Columns>(columns)...) }; }

    // --------------------------------------------------------------------------------------------
    // Table Definition

    class TableDefBase
    {
    public:
        constexpr explicit TableDefBase(StringView name) : m_name(name) {}

        constexpr const StringView& Name() const { return m_name; }

    protected:
        StringView m_name;
    };

    template <typename T, typename... Elements>
    class TableDef : public TableDefBase
    {
    public:
        using ObjectType = T;
        using ElementsType = Tuple<Elements...>;

        static constexpr uint32_t ColumnsCount = _CountTuple<ElementsType, IsColumnDef>;
        static constexpr uint32_t ConstraintsCount = _CountTuple<ElementsType, IsTableConstraint>;

    private:
        template <typename U>
        struct IsElementPrimaryKey { static constexpr bool Value = IsPrimaryKeyConstraint<U>::Value; };

        template <SpecializationOf<ColumnDef> U>
        struct IsElementPrimaryKey<U> { static constexpr bool Value = _CountTuple<typename U::ConstraintsType, IsPrimaryKeyConstraint> != 0; };

        static constexpr uint32_t PrimaryKeyCount = _CountTuple<ElementsType, IsElementPrimaryKey>;
        static_assert(PrimaryKeyCount == 0 || PrimaryKeyCount == 1, "There can only be one PrimaryKey constraint on a table.");

        template <typename U>
        struct IsElementSameObjectType { static constexpr bool Value = IsSame<typename U::ObjectType, ObjectType>; };

        static_assert(_CountTuple<ElementsType, IsElementSameObjectType> == ElementsType::Size,
            "All columns and constraints must refer to the same object type.");

    public:
        constexpr TableDef(StringView name, ElementsType elements)
            : TableDefBase(name)
            , m_elements(Move(elements))
        {}

        constexpr const ElementsType& Elements() const { return m_elements; }

        template <typename F>
        constexpr void ForEachColumn(F&& func) const;

        template <typename F>
        constexpr void ForEachConstraint(F&& func) const;

        template <typename U> requires(IsMemberObjectPointer<U>)
        constexpr StringView GetColumnName(U column) const;

    private:
        ElementsType m_elements;
    };

    template <typename T>
    struct IsTableDef { static constexpr bool Value = IsSpecialization<T, TableDef>; };

    template <typename T>
    concept TableElement = IsColumnDef<T>::Value || IsTableConstraint<T>::Value;

    template <TableElement... Args, typename T = TupleElement<0, Tuple<Args...>>::ObjectType>
    constexpr TableDef<T, Args...> Table(StringView name, Args... args)
    {
        return { name, MakeTuple(Forward<Args>(args)...) };
    }

    // --------------------------------------------------------------------------------------------
    // Schema Definition

    template <typename... Elements>
    class SchemaDef
    {
    public:
        using ElementsType = Tuple<Elements...>;

        template <typename T>
        using TableTypeFor = typename _PickTable<T, ElementsType>::Type;

        static constexpr uint32_t TablesCount = _CountTuple<ElementsType, IsTableDef>;
        static constexpr uint32_t IndexesCount = _CountTuple<ElementsType, IsIndexDef>;

    public:
        constexpr SchemaDef(ElementsType elements)
            : m_elements(Move(elements))
        {}

        constexpr const ElementsType& Elements() const { return m_elements; }

        template <typename T>
        constexpr const auto& TableFor() const
        {
            using TableType = TableTypeFor<T>;
            return TupleGet<TableType>(m_elements);
        }

    private:
        ElementsType m_elements;
    };

    template <typename T>
    concept SchemaElement = IsPragmaDef<T>::Value || IsIndexDef<T>::Value || IsTableDef<T>::Value;

    template <SchemaElement... Args>
    constexpr SchemaDef<Args...> DefineSchema(Args... args)
    {
        return { MakeTuple(Forward<Args>(args)...) };
    }

    // --------------------------------------------------------------------------------------------
    // Inline definitions

    template <typename T, typename... Elements>
    template <typename F>
    constexpr void TableDef<T, Elements...>::ForEachColumn(F&& func) const
    {
        TupleForEach(m_elements, [&](const auto& item)
        {
            if constexpr (IsColumnDef<Decay<decltype(item)>>::Value)
            {
                func(item);
            }
        });
    }

    template <typename T, typename... Elements>
    template <typename F>
    constexpr void TableDef<T, Elements...>::ForEachConstraint(F&& func) const
    {
        TupleForEach(m_elements, [&](const auto& item)
        {
            if constexpr (IsTableConstraint<Decay<decltype(item)>>::Value)
            {
                func(item);
            }
        });
    }

    template <typename T, typename... Elements>
    template <typename U> requires(IsMemberObjectPointer<U>)
    constexpr StringView TableDef<T, Elements...>::GetColumnName(U column) const
    {
        StringView name;
        ForEachColumn([&](const auto& c)
        {
            if constexpr (IsSame<U, decltype(c.member)>)
            {
                if (c.member == column)
                    name = c.name;
            }
        });
        return name;
    }
}
