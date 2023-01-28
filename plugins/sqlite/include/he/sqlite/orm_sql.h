// Copyright Chad Engler

#pragma once

#include "he/sqlite/orm.h"

#include "he/core/string_builder.h"
#include "he/core/string_view_fmt.h"
#include "he/core/types.h"

#include "fmt/core.h"

namespace he::sqlite
{
    // --------------------------------------------------------------------------------------------
    // Forward declarations

    template <typename T>
    struct SqlWriter;

    template <typename T>
    struct SqlBinder;

    // --------------------------------------------------------------------------------------------
    // Useful type traits

    template <typename T, size_t Index, typename M>
    struct EnableFoundModel : std::enable_if<std::is_same_v<T, typename M::ObjectType>, M> {};

    template <typename T, typename Seq, typename MTuple>
    struct _PickModel;

    template <typename T, size_t... I, typename... M>
    struct _PickModel<T, std::index_sequence<I...>, std::tuple<M...>> : EnableFoundModel<T, I, M>... {};

    template <typename T, typename MTuple>
    using PickModel = typename _PickModel<T, std::make_index_sequence<std::tuple_size_v<MTuple>,, std::remove_const_t<MTuple>>::type;

    // --------------------------------------------------------------------------------------------
    // ToSql public API

    struct SqlWriterContextBase
    {
        bool writeRawValues{ false };
    };

    template <typename Models>
    struct SqlWriterContext
    {
        using ModelsType = Models;

        constexpr SqlWriterContext(const ModelsType& models) : models(models) {}

        template <typename T>
        constexpr StringView GetModelName() const
        {
            using ModelType = PickModel<T, ModelsType>;
            return std::get<ModelType>(models).Name();
        }

        template <typename T> requires(std::is_member_object_pointer_v<T>)
        constexpr StringView GetColumnName(T column) const
        {
            using ModelType = PickModel<MemberObjectType<T>, ModelsType>;
            return std::get<ModelType>(models).GetColumnName(column);
        }

        const ModelsType& models;
    };

    template <typename T, typename U>
    void ToSql(StringBuilder& sql, const T& t, const SqlWriterContext<U>& ctx)
    {
        SqlWriter<T> writer;
        return writer.Write(sql, t, ctx);
    }

    template <typename T>
    void BindSql(Statement& stmt, int32_t& index, const T& t)
    {
        SqlBinder<T> binder;
        return binder.Bind(stmt, index, t);
    }

    template <typename T>
    void BindSql(Statement& stmt, const T& t)
    {
        int32_t index = 1;
        return BindSql(stmt, index, t);
    }

    // --------------------------------------------------------------------------------------------
    // SqlWriter implementations

    template <typename T, typename Ctx>
    static void WriteColumnNames(StringBuilder& sql, const T& columns, const Ctx& ctx)
    {
        IterateTuple(columns, [&](const auto& column, uint32_t index)
        {
            const StringView name = ctx.GetColumnName(column);
            sql.Write("{}{}", index > 0 ? ", " : "", name);
        });
    }

    template <DataType T>
    struct SqlWriter<T>
    {
        using Type = T;
        using Traits = DataTypeTraits<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const T& value, const Ctx& ctx) const
        {
            if (ctx.writeRawValues)
            {
                Traits::Write(sql, value);
            }
            else
            {
                sql.Write('?');
            }
        }
    };

    template <>
    struct SqlWriter<OnConflictKind>
    {
        using Type = OnConflictKind;

        template <typename Ctx>
        void Write(StringBuilder& sql, const OnConflictKind& value, const Ctx& ctx) const
        {
            HE_UNUSED(ctx);
            switch (value)
            {
                case OnConflictKind::None: break;
                case OnConflictKind::Rollback: sql.Write("ROLLBACK") break;
                case OnConflictKind::Abort: sql.Write("ABORT") break;
                case OnConflictKind::Fail: sql.Write("FAIL") break;
                case OnConflictKind::Ignore: sql.Write("IGNORE") break;
                case OnConflictKind::Replace: sql.Write("REPLACE") break;
            }
        }
    };

    template <>
    struct SqlWriter<OrderByKind>
    {
        using Type = OrderByKind;

        template <typename Ctx>
        void Write(StringBuilder& sql, const OrderByKind& value, const Ctx& ctx) const
        {
            HE_UNUSED(ctx);
            switch (value)
            {
                case OrderByKind::None: break;
                case OrderByKind::Asc: sql.Write("ASC") break;
                case OrderByKind::Desc: sql.Write("DESC") break;
            }
        }
    };

    template <>
    struct SqlWriter<FkActionKind>
    {
        using Type = FkActionKind;

        template <typename Ctx>
        void Write(StringBuilder& sql, const FkActionKind& value, const Ctx& ctx) const
        {
            HE_UNUSED(ctx);
            switch (value)
            {
                case FkActionKind::None: break;
                case FkActionKind::Cascade: sql.Write("CASCADE"); break;
                case FkActionKind::NoAction: sql.Write("NO ACTION"); break;
                case FkActionKind::Restrict: sql.Write("RESTRICT"); break;
                case FkActionKind::SetDefault: sql.Write("SET DEFAULT"); break;
                case FkActionKind::SetNull: sql.Write("SET NULL"); break;
            }
        }
    };

    template <typename... Columns>
    struct SqlWriter<PrimaryKeyConstraint<Columns...>>
    {
        using Type = PrimaryKeyConstraint<Columns...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("PRIMARY KEY");

            if (value.orderBy != OrderByKind::None)
            {
                sql.Write(' ');
                ToSql(sql, value.orderBy, ctx);
            }

            if (value.onConflict != OnConflictKind::None)
            {
                sql.Write(" ON CONFLICT ");
                ToSql(sql, value.onConflict, ctx);
            }

            if constexpr (std::tuple_size_v<Type::ColumnsType> > 0)
            {
                sql.Write(" (");
                WriteColumnNames(sql, value.columns, ctx);
                sql.Write(")");
            }

            if (value.autoIncrement)
            {
                sql.Write(" AUTOINCREMENT");
            }
        }
    };

    template <typename... Columns>
    struct SqlWriter<UniqueConstraint<Columns...>>
    {
        using Type = UniqueConstraint<Columns...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("UNIQUE");

            if constexpr (std::tuple_size_v<Type::ColumnsType> > 0)
            {
                sql.Write(" (");
                WriteColumnNames(sql, value.columns, ctx);
                sql.Write(")");
            }
        }
    };

    template <typename... Columns, typename... References>
    struct SqlWriter<ForeignKeyConstraint<std::tuple<Columns...>, std::tuple<References...>>>
    {
        using Type = ForeignKeyConstraint<std::tuple<Columns...>, std::tuple<References...>>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("FOREIGN KEY (");
            WriteColumnNames(sql, value.columns, ctx);
            sql.Write(") REFERENCES {} (", ctx.GetModelName<Type::ReferencedObjectType>());
            WriteColumnNames(sql, value.references, ctx);
            sql.Write(")");

            if (value.onDelete != FkActionKind::None)
            {
                sql.Write(" ON DELETE ");
                ToSql(sql, value.onDelete, ctx);
            }

            if (value.onUpdate != FkActionKind::None)
            {
                sql.Write(" ON UPDATE ");
                ToSql(sql, value.onDelete, ctx);
            }
        }
    };

    template <typename T>
    struct SqlWriter<DefaultConstraint<T>>
    {
        using Type = DefaultConstraint<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("DEFAULT ");
            ToSql(sql, value.value, ctx);
        }
    };

    template <>
    struct SqlWriter<NotNullConstraint>
    {
        using Type = NotNullConstraint;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("NOT NULL");

            if (value.onConflict != OnConflictKind::None)
            {
                sql.Write(" ON CONFLICT ");
                ToSql(sql, value.onConflict, ctx);
            }
        }
    };

    template <typename T, typename U, typename... Constraints>
    struct SqlWriter<ColumnDef<T, U, Constraints...>>
    {
        using Type = ColumnDef<T, U, Constraints...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("{} {}", name, Type::Traits::Sql);
            IterateTuple(value.constraints, [&](const auto& constraint, uint32_t)
            {
                sql.Write(' ');
                ToSql(sql, constraint, ctx);
            });
        }
    };

    template <typename... Columns>
    struct SqlWriter<IndexDef<Columns...>>
    {
        using Type = IndexDef<Columns...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            constexpr StringView tableName = ctx.GetModelName<Type::ObjectType>();
            const StringView uniqueStr = value.unique ? "UNIQUE " : "";

            sql.Write("CREATE {}INDEX {} ON {} (", uniqueStr, value.name, tableName);
            WriteColumnNames(sql, value.columns, ctx);
            sql.Write(")");
        }
    };

    template <typename T, typename U>
    struct SqlWriter<LimitExpr<T, U>>
    {
        using Type = LimitExpr<T, U>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("LIMIT ");
            ToSql(sql, value.limit, ctx);

            if constexpr (HasOffset)
            {
                sql.Write(" OFFSET ");
                ToSql(sql, value.offset, ctx);
            }
        }
    };

    template <typename T>
    struct SqlWriter<IsNullExpr<T>>
    {
        using Type = IsNullExpr<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("(");
            ToSql(sql, value.value, ctx);
            sql.Write(") IS NULL");
        }
    };

    template <typename T>
    struct SqlWriter<IsNotNullExpr<T>>
    {
        using Type = IsNotNullExpr<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("(");
            ToSql(sql, value.value, ctx);
            sql.Write(") IS NOT NULL");
        }
    };

    template <typename T>
    struct SqlWriter<WhereExpr<T>>
    {
        using Type = WhereExpr<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("WHERE (");
            ToSql(sql, value.value, ctx);
            sql.Write(")");
        }
    };

    template <typename T>
    struct SqlWriter<NotExpr<T>>
    {
        using Type = NotExpr<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("NOT (");
            ToSql(sql, value.cond, ctx);
            sql.Write(")");
        }
    };

    template <typename L, typename R, typename S>
    struct SqlWriter<_BinaryCondition<L, R, S>>
    {
        using Type = _BinaryCondition<L, R, S>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("(");
            ToSql(lhs);
            sql.Write(") {} (", Type::Sql);
            ToSql(rhs);
            sql.Write(")");
        }
    };

    template <typename L, typename R, typename S>
    struct SqlWriter<_BinaryOperator<L, R, S>>
    {
        using Type = _BinaryOperator<L, R, S>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("(");
            ToSql(lhs);
            sql.Write(") {} (", Type::Sql);
            ToSql(rhs);
            sql.Write(")");
        }
    };

    template <typename T, typename... Elements>
    struct SqlWriter<Model<T, Elements...>>
    {
        using Type = Model<T, Elements...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.WriteLine("CREATE TABLE {} (", value.Name());
            sql.IncreaseIndent();

            IterateTuple<IsColumnDef>(value.Elements(), [&](const auto& column, uint32_t index)
            {
                if (index > 0)
                    sql.Write(",\n");
                sql.WriteIndent();
                ToSql(sql, column, ctx);
            });

            IterateTuple<IsTableConstraint>(value.Elements(), [&](const auto& constraint, uint32_t index)
            {
                if (Type::ColumnsCount > 0 || index > 0)
                    sql.Write(",\n");
                sql.WriteIndent();
                ToSql(sql, constraint, ctx);
            });

            sql.Write('\n');
            sql.DecreaseIndent();
            sql.WriteLine(")");
        }
    };

    // --------------------------------------------------------------------------------------------
    // SqlBinder implementations

    template <DataType T>
    struct SqlBinder<T>
    {
        using Type = T;
        using Traits = DataTypeTraits<T>;

        bool Bind(Statement& stmt, int32_t& index, const T& value) const
        {
            return Traits::Bind(stmt, index++, value);
        }
    };

    template <typename T, typename U>
    struct SqlBinder<LimitExpr<T, U>>
    {
        using Type = LimitExpr<T, U>;

        bool Bind(Statement& stmt, int32_t& index, const Type& value) const
        {
            if (!BindSql(stmt, index, value.limit))
                return false;

            if (!BindSql(stmt, index, value.offset))
                return false;

            return true;
        }
    };

    template <typename T>
    struct SqlBinder<_UnaryExpr<T>>
    {
        using Type = _UnaryExpr<T>;

        bool Bind(Statement& stmt, int32_t& index, const Type& value) const
        {
            return BindSql(stmt, index, value.value);
        }
    };

    template <typename T>
    struct SqlBinder<NotExpr<T>>
    {
        using Type = NotExpr<T>;

        bool Bind(Statement& stmt, int32_t& index, const Type& value) const
        {
            return BindSql(stmt, index, value.value);
        }
    };

    template <typename L, typename R, typename S>
    struct SqlBinder<_BinaryCondition<L, R, S>>
    {
        using Type = _BinaryCondition<L, R, S>;

        bool Bind(Statement& stmt, int32_t& index, const Type& value) const
        {
            if (!BindSql(stmt, index, value.lhs))
                return false;

            if (!BindSql(stmt, index, value.rhs))
                return false;

            return true;
        }
    };

    template <typename L, typename R, typename S>
    struct SqlBinder<_BinaryOperator<L, R, S>>
    {
        using Type = _BinaryOperator<L, R, S>;

        bool Bind(Statement& stmt, int32_t& index, const Type& value) const
        {
            if (!BindSql(stmt, index, value.lhs))
                return false;

            if (!BindSql(stmt, index, value.rhs))
                return false;

            return true;
        }
    };
}
