// Copyright Chad Engler

#pragma once

#include "he/sqlite/orm.h"

#include "he/core/concepts.h"
#include "he/core/enum_ops.h"
#include "he/core/fmt.h"
#include "he/core/string_fmt.h"
#include "he/core/string_builder.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/uuid.h"
#include "he/sqlite/column_reader.h"
#include "he/sqlite/statement.h"

namespace he::sqlite
{
    // --------------------------------------------------------------------------------------------
    // Data Type Traits

    template <typename T> struct SqlDataTypeTraits;

    // INTEGER
    template <Integral T>
    struct SqlDataTypeTraits<T>
    {
        static constexpr StringView Sql = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, T& value) { value = BitCast<T>(column.AsInt64()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", value); }
    };

    template <>
    struct SqlDataTypeTraits<char>
    {
        static constexpr StringView Sql = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, char value) { stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, char& value) { value = static_cast<char>(column.AsInt64()); }
        static void Write(StringBuilder& sql, char value) { sql.Write("'{}'", value); }
    };

    template <Enum T>
    struct SqlDataTypeTraits<T>
    {
        static constexpr StringView Sql = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, AsUnderlyingType(value)); }
        static void Read(const ColumnReader& column, T& value) { value = BitCast<T>(column.AsInt64()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", AsUnderlyingType(value)); }
    };

    template <typename T>
    struct SqlDataTypeTraits<Time<T>>
    {
        static constexpr StringView Sql = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, BitCast<int64_t>(value.val)); }
        static void Read(const ColumnReader& column, T& value) { value.val = BitCast<uint64_t>(column.AsInt64()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", BitCast<int64_t>(value.val)); }
    };

    template <>
    struct SqlDataTypeTraits<Duration>
    {
        static constexpr StringView Sql = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const Duration& value) { stmt.Bind(index, value.val); }
        static void Read(const ColumnReader& column, Duration& value) { value.val = column.AsInt64(); }
        static void Write(StringBuilder& sql, const Duration& value) { sql.Write("{}", value.val); }
    };

    // REAL
    template <FloatingPoint T>
    struct SqlDataTypeTraits<T>
    {
        static constexpr StringView Sql = "REAL";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, T& value) { value = static_cast<T>(column.AsDouble()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{:.15}", value); }
    };

    // TEXT
    template <>
    struct SqlDataTypeTraits<String>
    {
        static constexpr StringView Sql = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const String& value) { stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, String& value) { value = column.AsText(); }
        static void Write(StringBuilder& sql, const String& value) { sql.Write("'{}'", value); }
    };

    template <>
    struct SqlDataTypeTraits<Vector<char>>
    {
        static constexpr StringView Sql = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const Vector<char>& value) { stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, Vector<char>& value) { value = column.AsText(); }
        static void Write(StringBuilder& sql, const Vector<char>& value) { sql.Write("'{}'", FmtJoin(value.Begin(), value.End(), "")); }
    };

    template <>
    struct SqlDataTypeTraits<StringView>
    {
        static constexpr StringView Sql = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const StringView& value) { stmt.Bind(index, value); }
        static void Write(StringBuilder& sql, const StringView& value) { sql.Write("'{}'", value); }
        // Note: No Read() because `StringView` is non-owning, and reading a value into it would be unsafe.
    };

    template <>
    struct SqlDataTypeTraits<Span<char>>
    {
        static constexpr StringView Sql = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const Span<char>& value) { stmt.Bind(index, value); }
        static void Write(StringBuilder& sql, const Span<char>& value) { sql.Write("'{}'", FmtJoin(value.Begin(), value.End(), "")); }
        // Note: No Read() because `Span<char>` is non-owning, and reading a value into it would be unsafe.
    };

    template <>
    struct SqlDataTypeTraits<const char*>
    {
        static constexpr StringView Sql = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const char* value) { stmt.Bind(index, value); }
        static void Write(StringBuilder& sql, const char* value) { sql.Write("'{}'", value); }
        // Note: No Read() because `const char*` is non-owning, and reading a value into it would be unsafe.
    };

    // BLOB
    template <>
    struct SqlDataTypeTraits<Vector<uint8_t>>
    {
        static constexpr StringView Sql = "BLOB";
        static bool Bind(Statement& stmt, int32_t index, const Vector<uint8_t>& value) { stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, Vector<uint8_t>& value) { value = column.AsBlob(); }
        static void Write(StringBuilder& sql, const Vector<uint8_t>& value) { sql.Write("X'{:02x}'", FmtJoin(value.Begin(), value.End(), "")); }
    };

    template <>
    struct SqlDataTypeTraits<Uuid>
    {
        static constexpr StringView Sql = "BLOB(16)";
        static bool Bind(Statement& stmt, int32_t index, const Uuid& value) { stmt.Bind(index, value.m_bytes); }
        static void Read(const ColumnReader& column, Uuid& value) { column.ReadBlob(value.m_bytes); }
        static void Write(StringBuilder& sql, const Uuid& value) { sql.Write("X'{:02x}'", FmtJoin(value.m_bytes, value.m_bytes + sizeof(value.m_bytes), "")); }
    };

    template <>
    struct SqlDataTypeTraits<Span<uint8_t>>
    {
        static constexpr StringView Sql = "BLOB";
        static bool Bind(Statement& stmt, int32_t index, const Span<uint8_t>& value) { stmt.Bind(index, value); }
        static void Write(StringBuilder& sql, const Span<uint8_t>& value) { sql.Write("X'{:02x}'", FmtJoin(value.Begin(), value.End(), "")); }
        // Note: No Read() because `Span<uint8_t>` is non-owning, and reading a value into it would be unsafe.
    };

    // --------------------------------------------------------------------------------------------
    // ToSql

    template <typename T>
    struct SqlWriter;

    struct SqlWriterContextBase
    {
        bool writeRawValues{ false };   ///< When true writes values directly into the query. Default is to use a placeholder (`?`).
        bool ddlIfNotExists{ false };   ///< When true adds `IF NOT EXISTS` to CREATE statements
    };

    template <SpecializationOf<SchemaDef> T>
    struct SqlWriterContext : SqlWriterContextBase
    {
        using SchemaType = T;

        constexpr explicit SqlWriterContext(const T& schema) : schema(schema) {}

        template <typename T>
        constexpr decltype(auto) GetTable() const
        {
            return schema.TableFor<T>();
        }

        template <typename T>
        constexpr StringView GetTableName() const
        {
            return schema.TableFor<T>().Name();
        }

        template <typename T> requires(IsMemberObjectPointer<T>)
        constexpr StringView GetColumnName(T column) const
        {
            return schema.TableFor<MemberPointerObjectType<T>>().GetColumnName(column);
        }

        const SchemaType& schema;
    };

    template <typename T, typename U>
    void ToSql(StringBuilder& sql, const T& t, const SqlWriterContext<U>& ctx)
    {
        SqlWriter<T> writer;
        return writer.Write(sql, t, ctx);
    }

    // --------------------------------------------------------------------------------------------
    // BindSql

    template <typename T>
    struct SqlBinder;

    struct SqlBinderContext
    {
        int32_t index{ 1 };
    };

    template <typename T>
    void BindSql(Statement& stmt, const T& t, SqlBinderContext& ctx)
    {
        SqlBinder<T> binder;
        return binder.Bind(stmt, t, ctx);
    }

    template <typename T>
    void BindSql(Statement& stmt, const T& t)
    {
        SqlBinderContext ctx;
        return BindSql(stmt, t, ctx);
    }

    // --------------------------------------------------------------------------------------------
    // SqlWriter implementations

    template <typename T, typename Ctx>
    static void WriteColumnNames(StringBuilder& sql, const T& columns, const Ctx& ctx)
    {
        uint32_t index = 0;
        TupleForEach(columns, [&](const auto& column)
        {
            const StringView name = ctx.GetColumnName(column);
            sql.Write("{}{}", index++ > 0 ? ", " : "", name);
        });
    }

    template <typename T>
    struct SqlWriter
    {
        using Type = T;
        using Traits = SqlDataTypeTraits<T>;

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
                case OnConflictKind::Rollback: sql.Write("ROLLBACK"); break;
                case OnConflictKind::Abort: sql.Write("ABORT"); break;
                case OnConflictKind::Fail: sql.Write("FAIL"); break;
                case OnConflictKind::Ignore: sql.Write("IGNORE"); break;
                case OnConflictKind::Replace: sql.Write("REPLACE"); break;
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
                case OrderByKind::Asc: sql.Write("ASC"); break;
                case OrderByKind::Desc: sql.Write("DESC"); break;
            }
        }
    };

    template <>
    struct SqlWriter<OrderNullsByKind>
    {
        using Type = OrderNullsByKind;

        template <typename Ctx>
        void Write(StringBuilder& sql, const OrderNullsByKind& value, const Ctx& ctx) const
        {
            HE_UNUSED(ctx);
            switch (value)
            {
                case OrderNullsByKind::None: break;
                case OrderNullsByKind::NullsFirst: sql.Write("NULLS FIRST"); break;
                case OrderNullsByKind::NullsLast: sql.Write("NULLS LAST"); break;
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

            if constexpr (Type::ColumnsType::Size > 0)
            {
                sql.Write(" (");
                WriteColumnNames(sql, value.columns, ctx);
                sql.Write(')');

                if (value.onConflict != OnConflictKind::None)
                {
                    sql.Write(" ON CONFLICT ");
                    ToSql(sql, value.onConflict, ctx);
                }
            }
            else
            {
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

                if (value.autoIncrement)
                {
                    sql.Write(" AUTOINCREMENT");
                }
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

            if constexpr (Type::ColumnsType::Size > 0)
            {
                sql.Write(" (");
                WriteColumnNames(sql, value.columns, ctx);
                sql.Write(')');
            }

            if (value.onConflict != OnConflictKind::None)
            {
                sql.Write(" ON CONFLICT ");
                ToSql(sql, value.onConflict, ctx);
            }
        }
    };

    template <typename... Columns, typename... References>
    struct SqlWriter<ForeignKeyConstraint<Tuple<Columns...>, Tuple<References...>>>
    {
        using Type = ForeignKeyConstraint<Tuple<Columns...>, Tuple<References...>>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("FOREIGN KEY (");
            WriteColumnNames(sql, value.columns, ctx);
            sql.Write(") REFERENCES {} (", ctx.GetTableName<typename Type::ReferencedObjectType>());
            WriteColumnNames(sql, value.references, ctx);
            sql.Write(')');

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
        using Traits = SqlDataTypeTraits<typename Type::ValueType>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("{} {}", value.name, Traits::Sql);
            TupleForEach(value.constraints, [&](const auto& constraint)
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
            const StringView tableName = ctx.GetTableName<typename Type::ObjectType>();
            const StringView uniqueStr = value.unique ? "UNIQUE " : "";
            const StringView ddlINEStr = ctx.ddlIfNotExists ? "IF NOT EXISTS " : "";

            sql.Write("CREATE {}INDEX {}{} ON {} (", uniqueStr, ddlINEStr, value.name, tableName);
            WriteColumnNames(sql, value.columns, ctx);
            sql.Write(')');
        }
    };

    template <typename T>
    struct SqlWriter<PragmaDef<T>>
    {
        using Type = PragmaDef<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("PRAGMA {} = ", value.name);
            ToSql(sql, value.value, ctx);
        }
    };

    template <typename T, typename U>
    struct SqlWriter<ColumnRef<T, U>>
    {
        using Type = ColumnRef<T, U>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const StringView name = ctx.GetColumnName(value.member);
            sql.Write(name);
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

            if constexpr (Type::HasOffset)
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
            sql.Write(')');
        }
    };

    template <typename Ctx>
    static void WriteOrderByExprBase(StringBuilder& sql, const OrderByExprBase& item, const Ctx& ctx)
    {
        if (!item.collateName.IsEmpty())
            sql.Write(" COLLATE {}", item.collateName);

        if (item.orderBy != OrderByKind::None)
        {
            sql.Write(' ');
            ToSql(sql, item.orderBy, ctx);
        }

        if (item.orderNullsBy != OrderNullsByKind::None)
        {
            sql.Write(' ');
            ToSql(sql, item.orderNullsBy, ctx);
        }
    }

    template <typename T>
    struct SqlWriter<OrderByExpr<T>>
    {
        using Type = OrderByExpr<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("ORDER BY (");

            sql.Write('(');
            ToSql(sql, value.value, ctx);
            sql.Write(')');

            WriteOrderByExprBase(sql, value, ctx);

            sql.Write(')');
        }
    };

    template <typename... Args>
    struct SqlWriter<MultiOrderByExpr<Args...>>
    {
        using Type = MultiOrderByExpr<Args...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("ORDER BY (");
            uint32_t index = 0;
            TupleForEach(value.args, [&](const auto& item)
            {
                if (index++ > 0)
                    sql.Write(", ");
                sql.Write('(');
                if constexpr (IsSpecialization<Decay<decltype(item)>, OrderByExpr>)
                {
                    sql.Write('(');
                    ToSql(sql, item.value, ctx);
                    sql.Write(')');

                    WriteOrderByExprBase(sql, item, ctx);
                }
                else
                {
                    ToSql(sql, item, ctx);
                }
                sql.Write(')');
            });
            sql.Write(')');
        }
    };

    template <typename... Args>
    struct SqlWriter<GroupByExpr<Args...>>
    {
        using Type = GroupByExpr<Args...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("GROUP BY (");
            uint32_t index = 0;
            TupleForEach(value.args, [&](const auto& item)
            {
                if (index++ > 0)
                    sql.Write(", ");
                sql.Write('(');
                ToSql(sql, item, ctx);
                sql.Write(')');
            });
            sql.Write(')');
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
            sql.Write(')');
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
            ToSql(sql, value.lhs, ctx);
            sql.Write(") {} (", Type::Sql);
            ToSql(sql, value.rhs, ctx);
            sql.Write(')');
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
            ToSql(sql, value.lhs, ctx);
            sql.Write(") {} (", Type::Sql);
            ToSql(sql, value.rhs, ctx);
            sql.Write(')');
        }
    };

    template <typename T>
    struct SqlWriter<InsertObjectQuery<T>>
    {
        using Type = InsertObjectQuery<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const auto& table = ctx.GetTable<typename Type::ObjectType>();
            const StringView tableName = table.Name();
            sql.Write("INSERT INTO {} (", tableName);

            uint32_t index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                const StringView name = ctx.GetColumnName(column);
                sql.Write("{}{}", index++ > 0 ? ", " : "", name);
            });
            sql.Write(") VALUES (");

            index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (index++ > 0)
                    sql.Write(", ");
                ToSql(sql, value.value.*column, ctx);
            });
            sql.Write(")");
        }
    };

    template <typename T, typename C, typename V>
    struct SqlWriter<InsertQuery<T, C, V>>
    {
        using Type = InsertQuery<T, C, V>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const StringView tableName = ctx.GetTableName<typename Type::ObjectType>();
            sql.Write("INSERT INTO {} (", tableName);

            uint32_t index = 0;
            TupleForEach(value.columns, [&](const auto& column)
            {
                const StringView name = ctx.GetColumnName(column);
                sql.Write("{}{}", index++ > 0 ? ", " : "", name);
            });
            sql.Write(") VALUES (");

            index = 0;
            if constexpr (IsSpecialization<typename Type::ValuesType, Tuple>)
            {
                TupleForEach(value.values, [&](const auto& item)
                {
                    if (index++ > 0)
                        sql.Write(", ");
                    ToSql(sql, item, ctx);
                });
            }
            else
            {
                TupleForEach(value.columns, [&](const auto& column)
                {
                    if (index++ > 0)
                        sql.Write(", ");
                    ToSql(sql, value.values.*column, ctx);
                });
            }
            sql.Write(")");
        }
    };

    template <typename T, typename... Elements>
    struct SqlWriter<TableDef<T, Elements...>>
    {
        using Type = TableDef<T, Elements...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const StringView ddlINEStr = ctx.ddlIfNotExists ? "IF NOT EXISTS " : "";

            sql.WriteLine("CREATE TABLE {}{} (", ddlINEStr, value.Name());
            sql.IncreaseIndent();

            uint32_t index = 0;
            value.ForEachColumn([&](const auto& item)
            {
                if (index++ > 0)
                    sql.Write(",\n");
                sql.WriteIndent();
                ToSql(sql, item, ctx);
            });

            value.ForEachConstraint([&](const auto& item)
            {
                if (index++ > 0)
                    sql.Write(",\n");
                sql.WriteIndent();
                ToSql(sql, item, ctx);
            });

            sql.Write('\n');
            sql.DecreaseIndent();
            sql.Write(')');
        }
    };

    template <typename... Elements>
    struct SqlWriter<SchemaDef<Elements...>>
    {
        using Type = SchemaDef<Elements...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            TupleForEach(value.Elements(), [&](const auto& item)
            {
                sql.WriteIndent();
                ToSql(sql, item, ctx);
                sql.WriteLine(";");
            });
        }
    };

    // --------------------------------------------------------------------------------------------
    // SqlBinder implementations

    template <typename T>
    struct SqlBinder
    {
        using Type = T;
        using Traits = SqlDataTypeTraits<T>;

        bool Bind(Statement& stmt, const T& value, SqlBinderContext& ctx) const
        {
            return Traits::Bind(stmt, ctx.index++, value);
        }
    };

    template <typename T, typename U>
    struct SqlBinder<LimitExpr<T, U>>
    {
        using Type = LimitExpr<T, U>;

        bool Bind(Statement& stmt, const Type& value, SqlBinderContext& ctx) const
        {
            if (!BindSql(stmt, value.limit, ctx))
                return false;

            if (!BindSql(stmt, value.offset, ctx))
                return false;

            return true;
        }
    };

    template <typename T>
    struct SqlBinder<_UnaryExpr<T>>
    {
        using Type = _UnaryExpr<T>;

        bool Bind(Statement& stmt, const Type& value, SqlBinderContext& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <typename... Args>
    struct SqlBinder<MultiOrderByExpr<Args...>>
    {
        using Type = MultiOrderByExpr<Args...>;

        bool Bind(Statement& stmt, const Type& value, SqlBinderContext& ctx) const
        {
            bool result = true;
            TupleForEach(value.args, [&](const auto& arg)
            {
                result &= BindSql(stmt, arg, ctx);
            });
            return result;
        }
    };

    template <typename... Args>
    struct SqlBinder<GroupByExpr<Args...>>
    {
        using Type = GroupByExpr<Args...>;

        bool Bind(Statement& stmt, const Type& value, SqlBinderContext& ctx) const
        {
            bool result = true;
            TupleForEach(value.args, [&](const auto& arg)
            {
                result &= BindSql(stmt, arg, ctx);
            });
            return result;
        }
    };

    template <typename T>
    struct SqlBinder<NotExpr<T>>
    {
        using Type = NotExpr<T>;

        bool Bind(Statement& stmt, const Type& value, SqlBinderContext& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <typename L, typename R, typename S>
    struct SqlBinder<_BinaryCondition<L, R, S>>
    {
        using Type = _BinaryCondition<L, R, S>;

        bool Bind(Statement& stmt, const Type& value, SqlBinderContext& ctx) const
        {
            if (!BindSql(stmt, value.lhs, ctx))
                return false;

            if (!BindSql(stmt, value.rhs, ctx))
                return false;

            return true;
        }
    };

    template <typename L, typename R, typename S>
    struct SqlBinder<_BinaryOperator<L, R, S>>
    {
        using Type = _BinaryOperator<L, R, S>;

        bool Bind(Statement& stmt, const Type& value, SqlBinderContext& ctx) const
        {
            if (!BindSql(stmt, value.lhs, ctx))
                return false;

            if (!BindSql(stmt, value.rhs, ctx))
                return false;

            return true;
        }
    };

    template <typename T>
    struct SqlBinder<InsertObjectQuery<T>>
    {
        using Type = InsertObjectQuery<T>;

        bool Bind(Statement& stmt, const Type& value, SqlBinderContext& ctx) const
        {
            const auto& table = ctx.GetTable<typename Type::ObjectType>();

            bool result = true;
            table.ForEachColumn([&](const auto& column)
            {
                result &= BindSql(stmt, value.value.*column, ctx);
            });

            return result;
        }
    };

    template <typename T, typename C, typename V>
    struct SqlBinder<InsertQuery<T, C, V>>
    {
        using Type = InsertQuery<T, C, V>;

        bool Bind(Statement& stmt, const Type& value, SqlBinderContext& ctx) const
        {
            bool result = true;
            if constexpr (IsSpecialization<typename Type::ValuesType, Tuple>)
            {
                TupleForEach(value.values, [&](const auto& item)
                {
                    result &= BindSql(stmt, item, ctx);
                });
            }
            else
            {
                TupleForEach(value.columns, [&](const auto& column)
                {
                    result &= BindSql(stmt, value.values.*column, ctx);
                });
            }

            return result;
        }
    };
}
