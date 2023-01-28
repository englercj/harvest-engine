// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/enum_ops.h"
#include "he/core/span.h"
#include "he/core/string_builder.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/uuid.h"
#include "he/core/vector.h"
#include "he/sqlite/column.h"
#include "he/sqlite/database.h"
#include "he/sqlite/statement.h"

#include "fmt/format.h"

#include <concepts>
#include <initializer_list>
#include <tuple>

namespace he::sqlite
{
    // --------------------------------------------------------------------------------------------
    // Forward declarations

    template <typename T>
    struct DataTypeTraits;

    template <typename T, typename... Elements>
    class Model;

    // --------------------------------------------------------------------------------------------
    // Useful type traits

    // Tuple filtering
    template <typename T, template <typename...> typename Pred>
    constexpr auto _FilterTuple(T t)
    {
        return std::apply([](auto... ts)
        {
            return std::tuple_cat(
                std::conditional_t<Pred<decltype(ts)>::Value,
                std::tuple<decltype(ts)>,
                std::tuple<>>{}...);
        }, t);
    }

    template <typename T, template <typename...> typename Pred>
    inline constexpr uint32_t CountTuple = std::tuple_size_v<decltype(_FilterTuple<T, Pred>(std::declval<T>()))>;

    // Tuple iteration
    template <typename T, typename F>
    constexpr void IterateTuple(T&& t, F&& iterator)
    {
        std::apply([&](auto... ts)
        {
            uint32_t index = 0;
            (iterator(ts, index++), ...);
        }, t);
    }

    template <template <typename...> typename Pred, typename T, typename F>
    constexpr void IterateTuple(T&& t, F&& iterator)
    {
        std::apply([&](auto... ts)
        {
            uint32_t index = 0;
            auto func = [&](const auto& x)
            {
                if constexpr (Pred<std::decay_t<decltype(x)>>::Value)
                {
                    iterator(x, index++);
                }
            };

            (func(ts), ...);
        }, t);
    }

    // Get object type from a member pointer
    template <typename T> struct _MemberObjectType;
    template <typename T, typename U> struct _MemberObjectType<U T::*> { using Type = T; };

    template <typename T>
    using MemberObjectType = typename _MemberObjectType<T>::Type;

    // Get the object type of a series of columns
    template <typename... Columns>
    struct _ColumnsObjectType
    {
        static_assert((std::is_member_object_pointer_v<Columns> && ...), "All columns must be pointers to object members.");
        static_assert(AllSame<MemberObjectType<Columns>...>, "All columns must refer to members of the same object type.");
        using Type = MemberObjectType<std::tuple_element_t<0, std::tuple<Columns...>>>;
    };


    template <>
    struct _ColumnsObjectType<>
    {
        using Type = void;
    };

    template <typename... Columns>
    using ColumnsObjectType = typename _ColumnsObjectType<Columns...>::Type;

    // Check if a type is setup to be a proper DataTable
    template <typename T>
    concept DataType = requires(Statement& stmt, const Column& column, StringBuilder& sb, int32_t index, const T& constValue, T& value)
    {
        std::same_as<decltype(T::Sql), StringView>;
        { T::Bind(stmt, index, constValue) } -> std::same_as<bool>;
        { T::Read(column, value) } -> std::same_as<void>;
        { T::Write(sb, value) } -> std::same_as<void>;
    };

    // --------------------------------------------------------------------------------------------
    // Data Types

    // Integers
    template <std::integral T>
    struct DataTypeTraits<T>
    {
        static constexpr StringView Sql = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, value); }
        static void Read(const Column& column, T& value) { value = BitCast<T>(column.GetInt64()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", value); }
    };

    template <Enum T>
    struct DataTypeTraits<T>
    {
        static constexpr StringView Sql = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, AsUnderlyingType(value)); }
        static void Read(const Column& column, T& value) { value = BitCast<T>(column.GetInt64()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", AsUnderlyingType(value)); }
    };

    // Floats
    template <std::floating_point T>
    struct DataTypeTraits<T>
    {
        static constexpr StringView Sql = "REAL";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, value); }
        static void Read(const Column& column, T& value) { value = static_cast<T>(column.GetDouble()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{:.15}", value); }
    };

    // String
    template <ContiguousRange<const char> T>
    struct DataTypeTraits<T>
    {
        static constexpr StringView Sql = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, StringView(value)); }
        static void Read(const Column& column, T& value) { value = column.GetText(); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("\"{}\"", value); }
    };

    template <>
    struct DataTypeTraits<const char*>
    {
        static constexpr StringView Sql = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const char* value) { stmt.Bind(index, value); }
        static void Read(const Column& column, const char*& value) { value = column.GetText().Data(); }
        static void Write(StringBuilder& sql, const char* value) { sql.Write("\"{}\"", value); }
    };

    // Blob
    template <typename T> requires(ContiguousRange<T, const uint8_t>)
    struct DataTypeTraits<T>
    {
        static constexpr StringView Sql = "BLOB";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, value); }
        static void Read(const Column& column, T& value) { value = column.GetBlob(); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("X\"{:02x}\"", fmt::join(value.Data(), value.Data() + value.Size(), "")); }
    };

    // Time
    template <typename T>
    struct DataTypeTraits<Time<T>>
    {
        static constexpr StringView Sql = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, BitCast<int64_t>(value.val)); }
        static void Read(const Column& column, T& value) { value.val = BitCast<uint64_t>(column.GetInt64()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", BitCast<int64_t>(value.val)); }
    };

    // Duration
    template <>
    struct DataTypeTraits<Duration>
    {
        static constexpr StringView Sql = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const Duration& value) { stmt.Bind(index, value.val); }
        static void Read(const Column& column, Duration& value) { value.val = column.GetInt64(); }
        static void Write(StringBuilder& sql, const Duration& value) { sql.Write("{}", value.val); }
    };

    // Uuid
    template <>
    struct DataTypeTraits<Uuid>
    {
        static constexpr StringView Sql = "BLOB(16)";
        static bool Bind(Statement& stmt, int32_t index, const Uuid& value) { stmt.Bind(index, value.m_bytes); }
        static void Read(const Column& column, Uuid& value) { column.ReadBlob(value.m_bytes); }
        static void Write(StringBuilder& sql, const Uuid& value) { sql.Write("X\"{:02x}\"", fmt::join(value.m_bytes, value.m_bytes + sizeof(value.m_bytes), "")); }
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

    struct PrimaryKeyConstraintBase : _Constraint
    {
        bool autoIncrement{ false };
        OrderByKind orderBy{ OrderByKind::None };
        OnConflictKind onConflict{ OnConflictKind::None };
    };

    template <typename... Columns>
    struct PrimaryKeyConstraint : PrimaryKeyConstraintBase
    {
        using ColumnsType = std::tuple<Columns...>;
        using ObjectType = ColumnsObjectType<Columns...>;

        constexpr PrimaryKeyConstraint() = default;
        constexpr PrimaryKeyConstraint(ColumnsType columns) : columns(Move(columns)) {}

        constexpr PrimaryKeyConstraint Asc() { auto r = *this; r.orderBy = OrderByKind::Asc; return r; }
        constexpr PrimaryKeyConstraint Desc() { auto r = *this; r.orderBy = OrderByKind::Desc; return r; }
        constexpr PrimaryKeyConstraint AutoIncrement() { auto r = *this; r.autoIncrement = true; return r; }

        constexpr PrimaryKeyConstraint OnConflictRollback() { auto r = *this; r.onConflict = OnConflictKind::Rollback; return r; }
        constexpr PrimaryKeyConstraint OnConflictAbort() { auto r = *this; r.onConflict = OnConflictKind::Abort; return r; }
        constexpr PrimaryKeyConstraint OnConflictFail() { auto r = *this; r.onConflict = OnConflictKind::Fail; return r; }
        constexpr PrimaryKeyConstraint OnConflictIgnore() { auto r = *this; r.onConflict = OnConflictKind::Ignore; return r; }
        constexpr PrimaryKeyConstraint OnConflictReplace() { auto r = *this; r.onConflict = OnConflictKind::Replace; return r; }

        ColumnsType columns;
    };

    struct UniqueConstraintBase : _Constraint
    {
        OnConflictKind onConflict{ OnConflictKind::None };
    };

    template <typename... Columns>
    struct UniqueConstraint : UniqueConstraintBase
    {
        using ColumnsType = std::tuple<Columns...>;
        using ObjectType = ColumnsObjectType<Columns...>;

        constexpr UniqueConstraint() = default;
        constexpr UniqueConstraint(ColumnsType columns) : columns(Move(columns)) {}

        constexpr UniqueConstraint OnConflictRollback() { auto r = *this; r.onConflict = OnConflictKind::Rollback; return r; }
        constexpr UniqueConstraint OnConflictAbort() { auto r = *this; r.onConflict = OnConflictKind::Abort; return r; }
        constexpr UniqueConstraint OnConflictFail() { auto r = *this; r.onConflict = OnConflictKind::Fail; return r; }
        constexpr UniqueConstraint OnConflictIgnore() { auto r = *this; r.onConflict = OnConflictKind::Ignore; return r; }
        constexpr UniqueConstraint OnConflictReplace() { auto r = *this; r.onConflict = OnConflictKind::Replace; return r; }

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
    struct ForeignKeyConstraint<std::tuple<Columns...>, std::tuple<References...>> : ForeignKeyConstraintBase
    {
        using ColumnsType = std::tuple<Columns...>;
        using ReferencesType = std::tuple<References...>;

        using ObjectType = ColumnsObjectType<Columns...>;
        using ReferencedObjectType = ColumnsObjectType<References...>;

        static_assert(std::tuple_size_v<ColumnsType> == std::tuple_size_v<ReferencesType>, "The number of source columns and referenced columns must be the same.");
        static_assert(std::is_same_v<ObjectType, ReferencedObjectType>, "The referenced column types must refer to the same object type as the model.");

        constexpr ForeignKeyConstraint() = default;
        constexpr ForeignKeyConstraint(ColumnsType columns, ReferencesType references)
            : columns(Move(columns))
            , references(Move(references))
        {}

        constexpr ForeignKeyConstraint OnDeleteCascade() { auto r = *this; r.onDelete = FkActionKind::Cascade; return r; }
        constexpr ForeignKeyConstraint OnDeleteNoAction() { auto r = *this; r.onDelete = FkActionKind::NoAction; return r; }
        constexpr ForeignKeyConstraint OnDeleteRestrict() { auto r = *this; r.onDelete = FkActionKind::Restrict; return r; }
        constexpr ForeignKeyConstraint OnDeleteSetDefault() { auto r = *this; r.onDelete = FkActionKind::SetDefault; return r; }
        constexpr ForeignKeyConstraint OnDeleteSetNull() { auto r = *this; r.onDelete = FkActionKind::SetNull; return r; }

        constexpr ForeignKeyConstraint OnUpdateCascade() { auto r = *this; r.onDelete = FkActionKind::Cascade; return r; }
        constexpr ForeignKeyConstraint OnUpdateNoAction() { auto r = *this; r.onDelete = FkActionKind::NoAction; return r; }
        constexpr ForeignKeyConstraint OnUpdateRestrict() { auto r = *this; r.onDelete = FkActionKind::Restrict; return r; }
        constexpr ForeignKeyConstraint OnUpdateSetDefault() { auto r = *this; r.onDelete = FkActionKind::SetDefault; return r; }
        constexpr ForeignKeyConstraint OnUpdateSetNull() { auto r = *this; r.onDelete = FkActionKind::SetNull; return r; }

        ColumnsType columns;
        ReferencesType references;
    };

    template <typename... Columns>
    struct ForeignKeyConstraintHelper
    {
        using ColumnsType = std::tuple<Columns...>;

        template <typename... References> requires((std::is_member_object_pointer_v<References> && ...))
        constexpr ForeignKeyConstraint<ColumnsType, std::tuple<References...>> References(References... references)
        {
            return { Move(this->columns), std::make_tuple(Forward<References>(references)...) };
        }

        ColumnsType columns;
    };

    template <typename T>
    struct DefaultConstraint : _Constraint
    {
        using ValueType = T;

        constexpr DefaultConstraint() = default;
        constexpr DefaultConstraint(ValueType&& value) : value(Move(value)) {}

        ValueType value;
    };

    struct NotNullConstraint : _Constraint
    {
        constexpr NotNullConstraint() = default;

        constexpr NotNullConstraint OnConflictRollback() { auto r = *this; r.onConflict = OnConflictKind::Rollback; return r; }
        constexpr NotNullConstraint OnConflictAbort() { auto r = *this; r.onConflict = OnConflictKind::Abort; return r; }
        constexpr NotNullConstraint OnConflictFail() { auto r = *this; r.onConflict = OnConflictKind::Fail; return r; }
        constexpr NotNullConstraint OnConflictIgnore() { auto r = *this; r.onConflict = OnConflictKind::Ignore; return r; }
        constexpr NotNullConstraint OnConflictReplace() { auto r = *this; r.onConflict = OnConflictKind::Replace; return r; }

        OnConflictKind onConflict{ OnConflictKind::None };
    };

    template <typename T>
    concept ColumnConstraint = IsSpecialization<T, PrimaryKeyConstraint>
        || IsSpecialization<T, UniqueConstraint>
        || IsSpecialization<T, DefaultConstraint>
        || std::is_same_v<T, NotNullConstraint>;

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

    template <typename... Columns> requires(std::is_member_object_pointer_v<Columns> && ...)
    constexpr PrimaryKeyConstraint<Columns...> PrimaryKey(Columns... columns)
    {
        return { std::make_tuple(Forward<Columns>(columns)...) };
    }

    template <typename... Columns> requires(std::is_member_object_pointer_v<Columns> && ...)
    constexpr UniqueConstraint<Columns...> Unique(Columns... columns)
    {
        return { std::make_tuple(Forward<Columns>(columns)...) };
    }

    template <typename... Columns> requires(std::is_member_object_pointer_v<Columns> && ...)
    constexpr ForeignKeyConstraintHelper<Columns...> ForeignKey(Columns... columns)
    {
        return { std::make_tuple(Forward<Columns>(columns)...) };
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
        using Traits = DataTypeTraits<ValueType>;
        using ConstraintsType = std::tuple<Constraints...>;

        constexpr ColumnDef() = default;
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

    template <typename T, template <typename...> typename Pred>
    struct ColumnHas { static constexpr bool Value = CountTuple<typename T::ConstraintsType, Pred> > 0; };

    template <typename T>
    using ColumnHasPrimaryKey = ColumnHas<T, IsPrimaryKeyConstraint>;

    template <typename T, typename U, ColumnConstraint... Constraints>
    constexpr ColumnDef<T, U, Constraints...> DefineColumn(StringView name, U T::* member, Constraints... constraints)
    {
        return { name, member, std::make_tuple(Forward<Constraints>(constraints)...) };
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
        using ColumnsType = std::tuple<Columns...>;
        using ObjectType = ColumnsObjectType<Columns...>;

        constexpr IndexDef() = default;
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

    template <typename... Columns> requires(std::is_member_object_pointer_v<Columns> && ...)
    constexpr IndexDef<Columns...> Index(StringView name, Columns... columns)
    {
        return { name, false, std::make_tuple(Forward<Columns>(columns)...) };
    }

    template <typename... Columns> requires(std::is_member_object_pointer_v<Columns> && ...)
    constexpr IndexDef<Columns...> UniqueIndex(StringView name, Columns... columns)
    {
        return { name, true, std::make_tuple(Forward<Columns>(columns)...) };
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
        using OffsetStorageType = std::conditional_t<std::is_same_v<U, void>, int, U>;

        static constexpr bool HasOffset = !std::is_same_v<OffsetType, void>;

        constexpr LimitExpr() = default;
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

        constexpr _UnaryExpr() = default;
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

    // Condition Expressions
    struct _ConditionBase {};

    template <typename T>
    struct NotExpr : _ConditionBase
    {
        constexpr NotExpr() = default;
        constexpr NotExpr(T cond) : cond(Move(cond)) {}

        T cond;
    };

    template <typename L, typename R, typename S>
    struct _BinaryCondition : _ConditionBase
    {
        static constexpr StringView Sql = S::Sql;

        constexpr _BinaryCondition() = default;
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

        constexpr _BinaryOperator() = default;
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
    template <typename T> inline constexpr bool IsCondition = std::is_base_of_v<_ConditionBase, T>;
    template <typename T> inline constexpr bool IsOperator = IsSpecialization<_OperatorBase, T>;

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

    template <typename L, typename R>
    constexpr ConcatExpr<L, R> Concat(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename T> requires(std::is_base_of_v<_ConditionBase, T>)
    constexpr NotExpr<T> operator!(T arg) { return { Move(arg) }; }

    template <typename L, typename R> requires(std::is_base_of_v<_ConditionBase, L> || std::is_base_of_v<_ConditionBase, R>)
    constexpr AndExpr<L, R> operator&&(L lhs, R rhs) { return { Move(lhs), Move(rhs) }; }

    template <typename L, typename R> requires(std::is_base_of_v<_ConditionBase, L> || std::is_base_of_v<_ConditionBase, R>)
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
    // Model

    class ModelBase
    {
    public:
        explicit constexpr ModelBase(StringView name) : m_name(name) {}

        constexpr const StringView& Name() const { return m_name; }

    protected:
        StringView m_name;
    };

    template <typename T, typename... Elements>
    class Model : public ModelBase
    {
    public:
        using ObjectType = T;
        using ElementsType = std::tuple<Elements...>;

        static constexpr size_t ColumnsCount = CountTuple<ElementsType, IsColumnDef>;
        static constexpr size_t ConstraintsCount = CountTuple<ElementsType, IsTableConstraint>;

    public:
        template <typename U> struct ElementIsPrimaryKey;

        template <typename U> requires(IsSpecialization<U, ColumnDef>)
        struct ElementIsPrimaryKey<U> { static constexpr bool Value = ColumnHasPrimaryKey<U>::Value; };

        template <typename U> requires(!IsSpecialization<U, ColumnDef>)
        struct ElementIsPrimaryKey<U> { static constexpr bool Value = IsPrimaryKeyConstraint<U>::Value; };

        template <typename U>
        struct ElementIsObjectType { static constexpr bool Value = std::is_same_v<typename U::ObjectType, ObjectType>; };

        static_assert(CountTuple<ElementsType, ElementIsPrimaryKey> == 0 || CountTuple<ElementsType, ElementIsPrimaryKey> == 1,
            "There can only be one PrimaryKey constraint on a table.");

        static_assert(CountTuple<ElementsType, ElementIsObjectType> == std::tuple_size_v<ElementsType>,
            "All columns and constraints must refer to the same object type.");

    public:
        constexpr Model(StringView name, ElementsType elements)
            : ModelBase(name)
            , m_elements(Move(elements))
        {}

        constexpr const ElementsType& Elements() const { return m_elements; }

        template <typename U> requires(std::is_member_object_pointer_v<U>)
        constexpr StringView GetColumnName(U column) const;

        template <typename... Args>
        bool FindOne(Database& db, ObjectType& value, Args... args) const;

        template <typename... Args>
        bool FindAll(Database& db, Vector<ObjectType>& values, Args... args) const;

        template <typename... Args>
        bool Update(Database& db, const ObjectType& value, Args... args) const;

        template <typename... Args>
        bool Upsert(Database& db, const ObjectType& value, Args... args) const;

        template <typename... Args>
        bool Delete(Database& db, Args... args) const;

    private:
        ElementsType m_elements;
    };

    template <typename T>
    concept ModelArg = IsColumnDef<T>::Value || IsTableConstraint<T>::Value;

    template <ModelArg... Args, typename T = std::tuple_element_t<0, std::tuple<Args...>>::ObjectType>
    constexpr Model<T, Args...> DefineModel(StringView name, Args... args)
    {
        return { name, std::make_tuple<Args...>(Forward<Args>(args)...) };
    }

    // --------------------------------------------------------------------------------------------
    // Inline definitions

    template <typename T, typename... Elements>
    template <typename U> requires(std::is_member_object_pointer_v<U>)
    constexpr StringView Model<T, Elements...>::GetColumnName(U column) const
    {
        StringView name;
        IterateTuple<IsColumnDef>(m_elements, [&](const auto& c, uint32_t)
        {
            if constexpr (std::is_same_v<decltype(c.member), U>)
            {
                if (c.member == column)
                    name = c.name;
            }
        });
        return name;
    }

    template <typename T, typename... Elements>
    template <typename... Args>
    inline bool Model<T, Elements...>::FindOne(Database& db, ObjectType& value, Args... args) const
    {
        // TODO
        HE_UNUSED(db, value, args...);
        return false;
    }

    template <typename T, typename... Elements>
    template <typename... Args>
    inline bool Model<T, Elements...>::FindAll(Database& db, Vector<ObjectType>& values, Args... args) const
    {
        // TODO
        HE_UNUSED(db, values, args...);
        return false;
    }

    template <typename T, typename... Elements>
    template <typename... Args>
    inline bool Model<T, Elements...>::Update(Database& db, const ObjectType& value, Args... args) const
    {
        // TODO
        HE_UNUSED(db, value, args...);
        return false;
    }

    template <typename T, typename... Elements>
    template <typename... Args>
    inline bool Model<T, Elements...>::Upsert(Database& db, const ObjectType& value, Args... args) const
    {
        // TODO
        HE_UNUSED(db, value, args...);
        return false;
    }

    template <typename T, typename... Elements>
    template <typename... Args>
    inline bool Model<T, Elements...>::Delete(Database& db, Args... args) const
    {
        // TODO
        HE_UNUSED(db, args...);
        return false;
    }
}
