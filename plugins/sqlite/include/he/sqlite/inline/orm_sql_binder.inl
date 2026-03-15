// Copyright Chad Engler

namespace he::sqlite
{
    template <typename T>
    struct SqlBinder
    {
        using Type = T;
        using Traits = SqlDataTypeTraits<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const T& value, Ctx& ctx) const
        {
            return Traits::Bind(stmt, ctx.index++, value);
        }
    };

    template <AnyOf<FkActionKind, OrderNullsByKind, OrderByKind, OnConflictKind> T>
    struct SqlBinder<T>
    {
        using Type = T;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return true;
        }
    };

    template <typename... Columns>
    struct SqlBinder<PrimaryKeyConstraint<Columns...>>
    {
        using Type = PrimaryKeyConstraint<Columns...>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            if constexpr (Type::ColumnsType::Size > 0)
            {
                if (value.onConflict != OnConflictKind::None)
                {
                    result &= BindSql(stmt, value.onConflict, ctx);
                }
            }
            else
            {
                if (value.orderBy != OrderByKind::None)
                {
                    result &= BindSql(stmt, value.orderBy, ctx);
                }

                if (value.onConflict != OnConflictKind::None)
                {
                    result &= BindSql(stmt, value.onConflict, ctx);
                }
            }
            return result;
        }
    };

    template <typename... Columns>
    struct SqlBinder<UniqueConstraint<Columns...>>
    {
        using Type = UniqueConstraint<Columns...>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            if (value.onConflict != OnConflictKind::None)
            {
                result &= BindSql(stmt, value.onConflict, ctx);
            }
            return result;
        }
    };

    template <typename... Columns, typename... References>
    struct SqlBinder<ForeignKeyConstraint<Tuple<Columns...>, Tuple<References...>>>
    {
        using Type = ForeignKeyConstraint<Tuple<Columns...>, Tuple<References...>>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            if (value.onDelete != FkActionKind::None)
            {
                result &= BindSql(stmt, value.onDelete, ctx);
            }

            if (value.onUpdate != FkActionKind::None)
            {
                result &= BindSql(stmt, value.onUpdate, ctx);
            }
            return result;
        }
    };

    template <typename T>
    struct SqlBinder<DefaultConstraint<T>>
    {
        using Type = DefaultConstraint<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <>
    struct SqlBinder<NotNullConstraint>
    {
        using Type = NotNullConstraint;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.onConflict, ctx);
        }
    };

    template <typename T, typename U, typename... Constraints>
    struct SqlBinder<ColumnDef<T, U, Constraints...>>
    {
        using Type = ColumnDef<T, U, Constraints...>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            TupleForEach(value.constraints, [&](const auto& constraint)
            {
                result &= BindSql(stmt, constraint, ctx);
            });
            return result;
        }
    };

    template <typename... Columns>
    struct SqlBinder<IndexDef<Columns...>>
    {
        using Type = IndexDef<Columns...>;

        template <typename Ctx>
        bool Bind([[maybe_unused]] Statement& stmt, [[maybe_unused]] const Type& value, [[maybe_unused]] Ctx& ctx) const
        {
            return true;
        }
    };

    template <typename T>
    struct SqlBinder<PragmaExpr<T>>
    {
        using Type = PragmaExpr<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <typename T, typename U>
    struct SqlBinder<ColumnRef<T, U>>
    {
        using Type = ColumnRef<T, U>;

        template <typename Ctx>
        bool Bind([[maybe_unused]] Statement& stmt, [[maybe_unused]] const Type& value, [[maybe_unused]] Ctx& ctx) const
        {
            return true;
        }
    };

    template <typename T, typename U>
    struct SqlBinder<ExcludedColumnRef<T, U>>
    {
        using Type = ExcludedColumnRef<T, U>;

        template <typename Ctx>
        bool Bind([[maybe_unused]] Statement& stmt, [[maybe_unused]] const Type& value, [[maybe_unused]] Ctx& ctx) const
        {
            return true;
        }
    };

    template <typename T, typename U, typename V> requires(IsConvertible<V, U> || IsSpecialization<V, ExcludedColumnRef>)
    struct SqlBinder<ColumnRefAndValue<T, U, V>>
    {
        using Type = ColumnRefAndValue<T, U, V>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <typename T, typename U>
    struct SqlBinder<LimitExpr<T, U>>
    {
        using Type = LimitExpr<T, U>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            if (!BindSql(stmt, value.limit, ctx))
                return false;

            if (!BindSql(stmt, value.offset, ctx))
                return false;

            return true;
        }
    };

    template <typename T>
    struct SqlBinder<IsNullExpr<T>>
    {
        using Type = IsNullExpr<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <typename T>
    struct SqlBinder<IsNotNullExpr<T>>
    {
        using Type = IsNotNullExpr<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <typename T>
    struct SqlBinder<WhereExpr<T>>
    {
        using Type = WhereExpr<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <typename T>
    struct SqlBinder<OrderByExpr<T>>
    {
        using Type = OrderByExpr<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <typename... Args>
    struct SqlBinder<MultiOrderByExpr<Args...>>
    {
        using Type = MultiOrderByExpr<Args...>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
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

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
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

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.value, ctx);
        }
    };

    template <typename L, typename R, typename S>
    struct SqlBinder<_BinaryCondition<L, R, S>>
    {
        using Type = _BinaryCondition<L, R, S>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
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

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            if (!BindSql(stmt, value.lhs, ctx))
                return false;

            if (!BindSql(stmt, value.rhs, ctx))
                return false;

            return true;
        }
    };

    template <typename T, typename... Args>
    struct SqlBinder<SelectObjectQuery<T, Args...>>
    {
        using Type = SelectObjectQuery<T, Args...>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            TupleForEach(value.args, [&](const auto& arg)
            {
                result &= BindSql(stmt, arg, ctx);
            });
            return result;
        }
    };

    template <typename T, typename Columns, typename Args>
    struct SqlBinder<SelectQuery<T, Columns, Args>>
    {
        using Type = SelectQuery<T, Columns, Args>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            TupleForEach(value.args, [&](const auto& arg)
            {
                result &= BindSql(stmt, arg, ctx);
            });
            return result;
        }
    };

    template <typename T, typename U>
    struct SqlBinder<DeleteQuery<T, U>>
    {
        using Type = DeleteQuery<T, U>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.where, ctx);
        }
    };

    template <typename T>
    struct SqlBinder<InsertObjectQuery<T>>
    {
        using Type = InsertObjectQuery<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            const auto& table = ctx.template GetTable<typename Type::ObjectType>();

            bool result = true;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                result &= BindSql(stmt, Invoke(column.member, value.value), ctx);
            });

            return result;
        }
    };

    template <typename T>
    struct SqlBinder<UpsertObjectQuery<T>>
    {
        using Type = UpsertObjectQuery<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            const auto& table = ctx.template GetTable<typename Type::ObjectType>();

            bool result = true;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                result &= BindSql(stmt, Invoke(column.member, value.value), ctx);
            });

            return result;
        }
    };

    template <>
    struct SqlBinder<InsertOrKind>
    {
        using Type = InsertOrKind;

        template <typename Ctx>
        bool Bind([[maybe_unused]] Statement& stmt, [[maybe_unused]] const Type& value, [[maybe_unused]] Ctx& ctx) const
        {
            return true;
        }
    };

    template <>
    struct SqlBinder<InsertOrExpr>
    {
        using Type = InsertOrExpr;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            return BindSql(stmt, value.kind, ctx);
        }
    };

    template <>
    struct SqlBinder<InsertDefaultValuesExpr>
    {
        using Type = InsertDefaultValuesExpr;

        template <typename Ctx>
        bool Bind([[maybe_unused]] Statement& stmt, [[maybe_unused]] const Type& value, [[maybe_unused]] Ctx& ctx) const
        {
            return true;
        }
    };

    template <typename... Values>
    struct SqlBinder<InsertValuesExpr<Values...>>
    {
        using Type = InsertValuesExpr<Values...>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            TupleForEach(value.values, [&](const auto& val)
            {
                result &= BindSql(stmt, val, ctx);
            });
            return result;
        }
    };

    template <typename T, typename A>
    struct SqlBinder<UpsertExpr<T, A>>
    {
        using Type = UpsertExpr<T, A>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            TupleForEach(value.actions, [&](const auto& action)
            {
                result &= BindSql(stmt, action, ctx);
            });
            return result;
        }
    };

    template <typename T, typename C, typename V>
    struct SqlBinder<InsertQuery<T, C, V>>
    {
        using Type = InsertQuery<T, C, V>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            TupleForEach(value.args, [&](const auto& item)
            {
                result &= BindSql(stmt, item, ctx);
            });
            return result;
        }
    };

    template <typename T>
    struct SqlBinder<UpdateObjectQuery<T>>
    {
        using Type = UpdateObjectQuery<T>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            const auto& table = ctx.template GetTable<typename Type::ObjectType>();

            bool result = true;
            table.ForEachColumn([&](const auto& column)
            {
                if (table.IsPrimaryKeyColumn(column.member))
                    return;

                result &= BindSql(stmt, Invoke(column.member, value.value), ctx);
            });

            table.ForEachColumn([&](const auto& column)
            {
                if (!table.IsPrimaryKeyColumn(column.member))
                    return;

                result &= BindSql(stmt, Invoke(column.member, value.value), ctx);
            });

            return result;
        }
    };

    template <typename T, typename U>
    struct SqlBinder<UpdateQuery<T, U>>
    {
        using Type = UpdateQuery<T, U>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            TupleForEach(value.setters, [&](const auto& setter)
            {
                result &= BindSql(stmt, setter, ctx);
            });
            return result;
        }
    };

    template <>
    struct SqlBinder<RawSqlQuery>
    {
        using Type = RawSqlQuery;

        template <typename Ctx>
        bool Bind([[maybe_unused]] Statement& stmt, [[maybe_unused]] const Type& value, [[maybe_unused]] Ctx& ctx) const
        {
            return true;
        }
    };

    template <typename T, typename... Elements>
    struct SqlBinder<TableDef<T, Elements...>>
    {
        using Type = TableDef<T, Elements...>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            value.ForEachColumn([&](const auto& item)
            {
                result &= BindSql(stmt, item, ctx);
            });
            value.ForEachConstraint([&](const auto& item)
            {
                result &= BindSql(stmt, item, ctx);
            });
            return result;
        }
    };

    template <typename... Elements>
    struct SqlBinder<SchemaDef<Elements...>>
    {
        using Type = SchemaDef<Elements...>;

        template <typename Ctx>
        bool Bind(Statement& stmt, const Type& value, Ctx& ctx) const
        {
            bool result = true;
            TupleForEach(value.Elements(), [&](const auto& item)
            {
                result &= BindSql(stmt, item, ctx);
            });
            return result;
        }
    };
}
