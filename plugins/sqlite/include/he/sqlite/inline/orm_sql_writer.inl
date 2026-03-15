// Copyright Chad Engler

namespace he::sqlite
{
    template <typename Ctx>
    static void WriteColumnName(StringBuilder& sql, StringView tableName, StringView columnName, const Ctx& ctx)
    {
        if (ctx.writeColumnTableNames)
        {
            sql.Write(tableName);
            sql.Write(".");
        }

        sql.Write(columnName);
    }

    template <typename T, typename Ctx>
    static void WriteColumnName(StringBuilder& sql, StringView tableName, const T& column, const Ctx& ctx)
    {
        const StringView name = ctx.GetColumnName(column);
        WriteColumnName(sql, tableName, name, ctx);
    }

    template <typename T, typename Ctx>
    static void WriteColumnNames(StringBuilder& sql, StringView tableName, const T& columns, const Ctx& ctx)
    {
        uint32_t index = 0;
        TupleForEach(columns, [&](const auto& column)
        {
            if (index++ > 0)
                sql.Write(", ");

            WriteColumnName(sql, tableName, column, ctx);
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
        void Write(StringBuilder& sql, const OnConflictKind& value, [[maybe_unused]] const Ctx& ctx) const
        {
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
        void Write(StringBuilder& sql, const OrderByKind& value, [[maybe_unused]] const Ctx& ctx) const
        {
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
        void Write(StringBuilder& sql, const OrderNullsByKind& value, [[maybe_unused]] const Ctx& ctx) const
        {
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
        void Write(StringBuilder& sql, const FkActionKind& value, [[maybe_unused]] const Ctx& ctx) const
        {
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
                const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
                sql.Write(" (");
                WriteColumnNames(sql, tableName, value.columns, ctx);
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
                const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
                sql.Write(" (");
                WriteColumnNames(sql, tableName, value.columns, ctx);
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
            const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
            const StringView refTableName = ctx.template GetTableName<typename Type::ReferencedObjectType>();

            sql.Write("FOREIGN KEY (");
            WriteColumnNames(sql, tableName, value.columns, ctx);
            sql.Write(") REFERENCES {} (", refTableName);
            WriteColumnNames(sql, refTableName, value.references, ctx);
            sql.Write(')');

            if (value.onDelete != FkActionKind::None)
            {
                sql.Write(" ON DELETE ");
                ToSql(sql, value.onDelete, ctx);
            }

            if (value.onUpdate != FkActionKind::None)
            {
                sql.Write(" ON UPDATE ");
                ToSql(sql, value.onUpdate, ctx);
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
            sql.Write("{} {}", value.name, Traits::SqlType);
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
            const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
            const StringView uniqueStr = value.unique ? "UNIQUE " : "";

            sql.Write("CREATE {}INDEX IF NOT EXISTS {} ON {} (", uniqueStr, value.name, tableName);
            WriteColumnNames(sql, tableName, value.columns, ctx);
            sql.Write(')');
        }
    };

    template <typename T>
    struct SqlWriter<PragmaExpr<T>>
    {
        using Type = PragmaExpr<T>;

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
            const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
            WriteColumnName(sql, tableName, value.member, ctx);
        }
    };

    template <typename T, typename U>
    struct SqlWriter<ExcludedColumnRef<T, U>>
    {
        using Type = ExcludedColumnRef<T, U>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("excluded.");

            const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
            WriteColumnName(sql, tableName, value.member, ctx);
        }
    };

    template <typename T, typename U, typename V> requires(IsConvertible<V, U> || IsSpecialization<V, ExcludedColumnRef>)
    struct SqlWriter<ColumnRefAndValue<T, U, V>>
    {
        using Type = ColumnRefAndValue<T, U, V>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
            WriteColumnName(sql, tableName, value.member, ctx);

            sql.Write(" = ");
            ToSql(sql, value.value, ctx);
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

    template <typename T, typename... Args>
    struct SqlWriter<SelectObjectQuery<T, Args...>>
    {
        using Type = SelectObjectQuery<T, Args...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const auto& table = ctx.template GetTable<typename Type::ObjectType>();
            const StringView tableName = table.Name();
            sql.Write("SELECT * FROM {}", tableName);
            TupleForEach(value.args, [&](const auto& arg)
            {
                sql.Write(" ", tableName);
                ToSql(sql, arg, ctx);
            });
        }
    };

    template <typename T, typename Columns, typename Args>
    struct SqlWriter<SelectQuery<T, Columns, Args>>
    {
        using Type = SelectQuery<T, Columns, Args>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const auto& table = ctx.template GetTable<typename Type::ObjectType>();
            const StringView tableName = table.Name();
            sql.Write("SELECT ");
            uint32_t index = 0;
            TupleForEach(value.columns, [&](const auto& column)
            {
                if (index++ > 0)
                    sql.Write(", ");

                if constexpr (IsMemberObjectPointer<Decay<decltype(column)>>)
                {
                    WriteColumnName(sql, tableName, column, ctx);
                }
                else
                {
                    ToSql(sql, column, ctx);
                }
            });
            sql.Write(" FROM {}", tableName);
            TupleForEach(value.args, [&](const auto& arg)
            {
                sql.Write(" ", tableName);
                ToSql(sql, arg, ctx);
            });
        }
    };

    template <typename T, typename U>
    struct SqlWriter<DeleteQuery<T, U>>
    {
        using Type = DeleteQuery<T, U>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const auto& table = ctx.template GetTable<typename Type::ObjectType>();
            const StringView tableName = table.Name();
            sql.Write("DELETE FROM {} ", tableName);
            ToSql(sql, value.where, ctx);
        }
    };

    template <typename T>
    struct SqlWriter<InsertObjectQuery<T>>
    {
        using Type = InsertObjectQuery<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const auto& table = ctx.template GetTable<typename Type::ObjectType>();
            const StringView tableName = table.Name();
            sql.Write("INSERT INTO {} ", tableName);

            uint32_t columnCount = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                ++columnCount;
            });

            if (columnCount == 0)
            {
                sql.Write("DEFAULT VALUES");
                return;
            }

            sql.Write("(");

            uint32_t index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                if (index++ > 0)
                    sql.Write(", ");

                WriteColumnName(sql, tableName, column.name, ctx);
            });

            sql.Write(") VALUES (");

            index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                if (index++ > 0)
                    sql.Write(", ");

                ToSql(sql, Invoke(column.member, value.value), ctx);
            });
            sql.Write(")");
        }
    };

    template <typename T>
    struct SqlWriter<UpsertObjectQuery<T>>
    {
        using Type = UpsertObjectQuery<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const auto& table = ctx.template GetTable<typename Type::ObjectType>();
            const StringView tableName = table.Name();
            sql.Write("INSERT INTO {} ", tableName);

            uint32_t columnCount = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                ++columnCount;
            });

            if (columnCount == 0)
            {
                sql.Write("DEFAULT VALUES");
                return;
            }

            sql.Write("(");

            uint32_t index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                if (index++ > 0)
                    sql.Write(", ");

                WriteColumnName(sql, tableName, column.name, ctx);
            });

            sql.Write(") VALUES (");

            index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                if (index++ > 0)
                    sql.Write(", ");

                ToSql(sql, Invoke(column.member, value.value), ctx);
            });
            sql.Write(") ON CONFLICT (");

            index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (!table.IsPrimaryKeyColumn(column.member))
                    return;

                if (index++ > 0)
                    sql.Write(", ");

                WriteColumnName(sql, tableName, column.name, ctx);
            });
            sql.Write(") DO UPDATE SET ");

            index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                if (index++ > 0)
                    sql.Write(", ");

                WriteColumnName(sql, tableName, column.name, ctx);
                sql.Write(" = excluded.");
                WriteColumnName(sql, tableName, column.name, ctx);
            });
        }
    };

    template <>
    struct SqlWriter<InsertOrKind>
    {
        using Type = InsertOrKind;

        template <typename Ctx>
        void Write(StringBuilder& sql, const InsertOrKind& value, [[maybe_unused]] const Ctx& ctx) const
        {
            switch (value)
            {
                case InsertOrKind::None: break;
                case InsertOrKind::Abort: sql.Write("ABORT"); break;
                case InsertOrKind::Fail: sql.Write("FAIL"); break;
                case InsertOrKind::Ignore: sql.Write("IGNORE"); break;
                case InsertOrKind::Replace: sql.Write("REPLACE"); break;
                case InsertOrKind::Rollback: sql.Write("ROLLBACK"); break;
            }
        }
    };

    template <>
    struct SqlWriter<InsertOrExpr>
    {
        using Type = InsertOrExpr;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            ToSql(sql, value.kind, ctx);
        }
    };

    template <>
    struct SqlWriter<InsertDefaultValuesExpr>
    {
        using Type = InsertDefaultValuesExpr;

        template <typename Ctx>
        void Write(StringBuilder& sql, [[maybe_unused]] const Type& value, [[maybe_unused]] const Ctx& ctx) const
        {
            sql.Write("DEFAULT VALUES");
        }
    };

    template <typename... Values>
    struct SqlWriter<InsertValuesExpr<Values...>>
    {
        using Type = InsertValuesExpr<Values...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("VALUES (");
            uint32_t index = 0;
            TupleForEach(value.values, [&](const auto& val)
            {
                if (index++ > 0)
                    sql.Write(", ");
                ToSql(sql, val, ctx);
            });
            sql.Write(")");
        }
    };

    template <typename T, typename A>
    struct SqlWriter<UpsertExpr<T, A>>
    {
        using Type = UpsertExpr<T, A>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write("ON CONFLICT (");
            const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
            WriteColumnNames(sql, tableName, value.targetArgs, ctx);
            sql.Write(") DO ");

            if constexpr (Type::ActionsType::Size == 0)
            {
                sql.Write("NOTHING");
            }
            else
            {
                sql.Write("UPDATE SET ");
                uint32_t index = 0;
                TupleForEach(value.actions, [&](const auto& action)
                {
                    if (index++ > 0)
                        sql.Write(", ");
                    ToSql(sql, action, ctx);
                });
            }
        }
    };

    template <typename T, typename C, typename V>
    struct SqlWriter<InsertQuery<T, C, V>>
    {
        using Type = InsertQuery<T, C, V>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
            sql.Write("INSERT INTO {} (", tableName);
            WriteColumnNames(sql, tableName, value.columns, ctx);
            sql.Write(")");

            TupleForEach(value.args, [&](const auto& item)
            {
                using ItemType = Decay<decltype(item)>;
                sql.Write(" ");

                if constexpr (IsSpecialization<ItemType, SelectQuery>)
                {
                    Ctx ctx2 = ctx;
                    ctx2.writeColumnTableNames = true;
                    ToSql(sql, item, ctx2);
                }
                else
                {
                    ToSql(sql, item, ctx);
                }
            });
        }
    };

    template <typename T>
    struct SqlWriter<UpdateObjectQuery<T>>
    {
        using Type = UpdateObjectQuery<T>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const auto& table = ctx.template GetTable<typename Type::ObjectType>();
            const StringView tableName = table.Name();
            sql.Write("UPDATE {} SET ", tableName);

            uint32_t index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                if (index++ > 0)
                    sql.Write(", ");

                WriteColumnName(sql, tableName, column.name, ctx);
                sql.Write(" = ");
                ToSql(sql, Invoke(column.member, value.value), ctx);
            });

            sql.Write(" WHERE ");
            index = 0;
            table.ForEachColumn([&](const auto& column)
            {
                if (!table.IsPrimaryKeyColumn(column.member))
                    return;

                if (index++ > 0)
                    sql.Write(" AND ");

                WriteColumnName(sql, tableName, column.name, ctx);
                sql.Write(" = ");
                ToSql(sql, Invoke(column.member, value.value), ctx);
            });
        }
    };

    template <typename T, typename U>
    struct SqlWriter<UpdateQuery<T, U>>
    {
        using Type = UpdateQuery<T, U>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            const StringView tableName = ctx.template GetTableName<typename Type::ObjectType>();
            sql.Write("UPDATE {} SET ", tableName);

            uint32_t index = 0;
            TupleForEach(value.setters, [&](const auto& setter)
            {
                if (index++ > 0)
                    sql.Write(", ");

                ToSql(sql, setter, ctx);
            });

            sql.Write(' ');
            ToSql(sql, value.where, ctx);
        }
    };

    template <>
    struct SqlWriter<RawSqlQuery>
    {
        using Type = RawSqlQuery;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.Write(value.query);
        }
    };

    template <typename T, typename... Elements>
    struct SqlWriter<TableDef<T, Elements...>>
    {
        using Type = TableDef<T, Elements...>;

        template <typename Ctx>
        void Write(StringBuilder& sql, const Type& value, const Ctx& ctx) const
        {
            sql.WriteLine("CREATE TABLE {} (", value.Name());
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
}
