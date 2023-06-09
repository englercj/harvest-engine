// Copyright Chad Engler

namespace he::sqlite
{
    // --------------------------------------------------------------------------------------------
    // StorageBase

    inline bool StorageBase::DropColumn(StringView tableName, StringView columnName)
    {
        StringBuilder sql;
        sql.Write("ALTER TABLE '{}' DROP COLUMN '{}'", tableName, columnName);
        return m_db.Execute(sql.Str().Data());
    }

    template <typename T, typename U> requires(IsTableDef<T>::Value && IsColumnDef<U>::Value)
    StorageBase::ColumnInfo StorageBase::ColumnInfoFromDef(const T& table, const U& column)
    {
        using TableConstraints = typename T::ElementsType;
        using ColumnConstraints = typename U::ConstraintsType;
        using Traits = SqlDataTypeTraits<typename U::ValueType>;

        ColumnInfo info;
        info.cid = -1;
        info.name = column.name;
        info.type = Traits::SqlType;
        info.pk = table.GetPrimaryKeyIndex(column.member) + 1;
        info.notnull = TupleHas<NotNullConstraint, ColumnConstraints>();
        info.hidden = false;

        if constexpr (TupleHas<DefaultConstraint, ColumnConstraints>())
        {
            const auto& value = TupleGet<DefaultConstraint>(column.constraints).value;
            StringBuilder valueStr;
            Traits::Write(valueStr, value);
            info.defaultValue = valueStr.Str();
        }

        return info;
    }

    inline bool StorageBase::QueryTableExists(StringView tableName)
    {
        StringBuilder sql;
        sql.Write("SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name = '{}'", tableName);

        Statement stmt;
        if (!HE_VERIFY(stmt.Prepare(m_db.Handle(), sql.Str().Data())))
            return false;

        if (!HE_VERIFY(stmt.Step() == sqlite::StepResult::Row))
            return false;

        const bool exists = stmt.GetColumn(0).AsInt() > 0;

        if (!HE_VERIFY(stmt.Step() == sqlite::StepResult::Done))
            return false;

        return exists;
    }

    inline bool StorageBase::QueryTableInfo(StringView tableName, TableInfo& info)
    {
        info.exists = QueryTableExists(tableName);

        if (info.exists)
        {
            StringBuilder sql;
            sql.Write("PRAGMA table_xinfo('{}');", tableName);

            Statement stmt;
            if (!HE_VERIFY(stmt.Prepare(m_db.Handle(), sql.Str().Data())))
                return false;

            return stmt.EachRow([&](const Statement& stmt)
            {
                ColumnInfo& cinfo = info.columns.EmplaceBack();
                cinfo.cid = stmt.GetColumn(0).AsInt();
                cinfo.name = stmt.GetColumn(1).AsText();
                cinfo.type = stmt.GetColumn(2).AsText();
                cinfo.notnull = stmt.GetColumn(3).AsInt() != 0;
                cinfo.defaultValue = stmt.GetColumn(4).AsText();
                cinfo.pk = stmt.GetColumn(5).AsInt();
                cinfo.hidden = stmt.GetColumn(6).AsInt() != 0;
            });
        }

        return true;
    }

    // --------------------------------------------------------------------------------------------
    // Storage

    template <typename S>
    template <typename T>
    bool Storage<S>::Execute(const T& query)
    {
        Statement stmt;
        if (!PrepareQuery(stmt, query))
            return false;

        return stmt.Step() != StepResult::Error;
    }

    template <typename S>
    template <typename T, typename F>
    bool Storage<S>::Execute(const T& query, F&& rowIterator)
    {
        Statement stmt;
        if (!PrepareQuery(stmt, query))
            return false;

        return stmt.EachRow(rowIterator);
    }

    template <typename S>
    template <typename T>
    bool Storage<S>::CreateTable()
    {
        const auto& table = m_schema.TableFor<T>();
        SqlWriterContext ctx(m_schema);
        StringBuilder sql;
        ToSql(sql, table, ctx);
        return m_db.Execute(sql.Str().Data());
    }

    template <typename S>
    template <typename T>
    bool Storage<S>::DropTable()
    {
        const auto& table = m_schema.TableFor<T>();
        String sql = "DROP TABLE IF EXISTS ";
        sql += table.Name();
        return m_db.Execute(sql.Data());
    }

    template <typename S>
    bool Storage<S>::Sync()
    {
        // Check if foreign key constraints are enabled.
        int32_t fk = 0;
        {
            Statement stmt;
            if (!HE_VERIFY(stmt.Prepare(m_db.Handle(), "PRAGMA foreign_keys;")))
                return false;

            if (!HE_VERIFY(stmt.Step() == sqlite::StepResult::Row))
                return false;

            fk = stmt.GetColumn(0).AsInt();
        }

        // Disable FK constraints during our migration and restore the previous state afterwards.
        m_db.Execute("PRAGMA foreign_keys = 0;");
        HE_AT_SCOPE_EXIT([&]
        {
            if (fk > 0)
                m_db.Execute("PRAGMA foreign_keys = 1;");
        });

        // Begin the transaction for our migration.
        Transaction t(m_db.Handle());

        bool result = true;
        TupleForEach(m_schema.Elements(), [&](const auto& item)
        {
            using ItemType = Decay<decltype(item)>;

            if constexpr (IsTableDef<ItemType>::Value)
            {
                HE_LOG_DEBUG(he_sqlite, HE_MSG("Starting sync of table"), HE_KV(table_name, item.Name()));

                result &= SyncTable(item);
            }
            else if constexpr (IsSame<ItemType, RawSqlQuery>)
            {
                result &= m_db.Execute(item.query);
            }
        });

        if (result)
            t.Commit();

        return result;
    }

    template <typename S>
    template <typename T>
    bool Storage<S>::Create(const T& obj)
    {
        const auto query = InsertObj(obj);
        return Execute(query);
    }

    template <typename S>
    template <typename T, typename U>
    bool Storage<S>::Destroy(WhereExpr<U>&& cond)
    {
        const auto query = Delete<T>(Move(cond));
        return Execute(query);
    }

    template <typename S>
    template <typename T, SelectQueryArg... U>
    bool Storage<S>::FindAll(Vector<T>& out, U&&... conditions)
    {
        const auto& table = m_schema.TableFor<T>();
        const auto query = SelectObj<T>(Forward<U>(conditions)...);
        return Execute(query, [&](const Statement& stmt)
        {
            int i = 0;
            T& obj = out.EmplaceBack();
            table.ForEachColumn([&](const auto& col)
            {
                const ColumnReader reader = stmt.GetColumn(i++);
                ReadSql(reader, obj.*(col.member));
            });
        });
    }

    template <typename S>
    template <typename T, SelectQueryArg... U>
    bool Storage<S>::FindOne(T& out, U&&... conditions)
    {
        const auto& table = m_schema.TableFor<T>();
        const auto query = SelectObj<T>(Forward<U>(conditions)..., Limit(1));
        return Execute(query, [&](const Statement& stmt)
        {
            int i = 0;
            table.ForEachColumn([&](const auto& col)
            {
                const ColumnReader reader = stmt.GetColumn(i++);
                ReadSql(reader, out.*(col.member));
            });
        });
    }

    template <typename S>
    template <typename T>
    bool Storage<S>::Update(const T& obj)
    {
        const auto query = Update(obj);
        return Execute(query);
    }

    template <typename S>
    template <typename T>
    bool Storage<S>::CreateOrUpdate(const T& obj)
    {
        // TODO
    }

    template <typename S>
    template <typename T>
    inline bool Storage<S>::PrepareQuery(Statement& stmt, const T& query)
    {
        SqlWriterContext writeCtx(m_schema);
        StringBuilder sql;
        ToSql(sql, query, writeCtx);

        if (!HE_VERIFY(stmt.Prepare(m_db.Handle(), sql.Str().Data())))
            return false;

        SqlBinderContext bindCtx(m_schema);
        if (!HE_VERIFY(BindSql(stmt, query, bindCtx)))
            return false;

        return true;
    }

    template <typename S>
    template <typename T, typename... Elements>
    bool Storage<S>::SyncTable(const TableDef<T, Elements...>& table)
    {
        TableInfo tableInfo;
        if (!QueryTableInfo(table.Name(), tableInfo))
            return false;

        // Create the table, if it does not exist
        if (!tableInfo.exists)
        {
            HE_LOG_INFO(he_sqlite,
                HE_MSG("Adding missing table to schema."),
                HE_KV(table_name, table.Name()));
            SqlWriterContext ctx(m_schema);
            StringBuilder sql;
            ToSql(sql, table, ctx);
            return m_db.Execute(sql.Str().Data());
        }

        // Create a lookup table for the existing columns based on index of new columns
        Vector<const ColumnInfo*> existingColumns;
        existingColumns.Reserve(TupleSize(table.Elements()));

        const uint32_t bitsetElementCount = BitSpan::RequiredElements(tableInfo.columns.Size());
        BitSpan::ElementType* columnsBitsetData = HE_ALLOCA(BitSpan::ElementType, bitsetElementCount);
        BitSpan visitedColumnsBitset(columnsBitsetData, bitsetElementCount);

        uint32_t colIndex = 0;
        table.ForEachColumn([&](const auto& col)
        {
            for (uint32_t i = 0; i < tableInfo.columns.Size(); ++i)
            {
                const ColumnInfo& candidate = tableInfo.columns[i];
                if (candidate.name == col.name)
                {
                    existingColumns[colIndex++] = &candidate;
                    visitedColumnsBitset.Set(i, true);
                    break;
                }
            }
        });

        bool result = true;
        bool columnModified = false;
        colIndex = 0;
        table.ForEachColumn([&](const auto& col)
        {
            const ColumnInfo* columnInfo = existingColumns[colIndex++];

            if (!columnModified)
            {
                result &= SyncColumn(table, col, columnInfo, columnModified);
            }
        });

        if (!result)
            return false;

        // At least one column has been modified, so we need to rebuild the table.
        if (columnModified)
        {
            SqlWriterContext ctx(m_schema);
            StringBuilder ddl;
            ToSql(ddl, table, ctx);

            const char* ddlStart = String::FindN(ddl.Str().Data(), ddl.Str().Size(), '\n');

            StringBuilder sql;
            sql.Write(R"(
                CREATE TABLE __he_temp__{0}{1};
                INSERT INTO __he_temp__{0}(
            )", table.Name(), ddlStart);

            // Write the column names we're inserting into
            colIndex = 0;
            uint32_t i = 0;
            table.ForEachColumn([&](const auto& col)
            {
                const ColumnInfo* columnInfo = existingColumns[colIndex++];
                if (!columnInfo)
                    return;

                if (i++ > 0)
                    sql.Write(", ");

                sql.Write(col.name);
            });
            sql.Write(") SELECT (");

            // Write the cast expressions from previous columns to the new types
            colIndex = 0;
            i = 0;
            table.ForEachColumn([&](const auto& col)
            {
                const ColumnInfo* columnInfo = existingColumns[colIndex++];
                if (!columnInfo)
                    return;

                if (i++ > 0)
                    sql.Write(", ");

                using Traits = SqlDataTypeTraits<Decay<decltype(col)>::ValueType>;
                sql.Write("CAST({} AS {})", col.name, Traits::SqlType);
            });
            sql.Write(") FROM {};\n", table.Name());

            sql.Write(R"(
                DROP TABLE {0};
                ALTER TABLE __he_temp__{0} RENAME TO {0};
            )", table.Name());

            return m_db.Execute(sql.Str().Data());
        }

        // If no existing columns were modified then we can drop any columns that were removed.
        for (uint32_t i = 0; i < tableInfo.columns.Size(); ++i)
        {
            if (visitedColumnsBitset.IsSet(i))
                continue;

            const ColumnInfo& col = tableInfo.columns[i];

            HE_LOG_INFO(he_sqlite,
                HE_MSG("Dropping removed column from table."),
                HE_KV(column_name, col.name),
                HE_KV(table_name, table.Name()));

            if (!DropColumn(table.Name(), col.name))
                return false;
        }

        return true;
    }

    template <typename S>
    template <typename T, typename U> requires(IsTableDef<T>::Value && IsColumnDef<U>::Value)
    bool Storage<S>::SyncColumn(const T& table, const U& column, const ColumnInfo* columnInfo, bool& columnModified)
    {
        if (!columnInfo)
        {
            HE_LOG_INFO(he_sqlite,
                HE_MSG("Adding missing column to table."),
                HE_KV(column_name, column.name),
                HE_KV(table_name, table.Name()));

            return AddColumn(table.Name(), column);
        }

        const ColumnInfo newColumnInfo = ColumnInfoFromDef(table, column);

        if (columnInfo->type == newColumnInfo.type
            && columnInfo->notnull == newColumnInfo.notnull
            && columnInfo->defaultValue == newColumnInfo.defaultValue
            && columnInfo->pk == newColumnInfo.pk
            && columnInfo->hidden == newColumnInfo.hidden)
        {
            HE_LOG_TRACE(he_sqlite,
                HE_MSG("Column is already in sync, no need for modifications."),
                HE_KV(column_name, column.name),
                HE_KV(table_name, table.Name()));

            return true;
        }

        // It would be nice if we could do something like this:
        // ```sql
        // ALTER TABLE table ADD COLUMN __temp__new_col new_type;
        // UPDATE table SET __temp__new_col = CAST(old_col as new_type);
        // ALTER TABLE table DROP COLUMN old_col;
        // ALTER TABLE table RENAME COLUMN __temp__new_col TO old_col;
        // ```
        // But unfortunately SQLite has many restrictions around dropping columns that means
        // this won't work if the column is indexed in any way.

        HE_LOG_INFO(he_sqlite,
            HE_MSG("Column is out of sync, table must be recreated."),
            HE_KV(column_name, column.name),
            HE_KV(table_name, table.Name()));

        columnModified = true;
        return true;
    }

    template <typename S>
    template <typename T, typename U, typename... Constraints>
    bool Storage<S>::AddColumn(StringView tableName, const ColumnDef<T, U, Constraints...>& column)
    {
        StringBuilder sql;
        sql.Write("ALTER TABLE '{}' ADD COLUMN ", tableName);

        SqlWriterContext ctx(m_schema);
        SqlWriter<ColumnDef<T, U, Constraints...>> writer;
        writer.Write(sql, column, ctx);

        return m_db.Execute(sql.Str().Data());
    }
}
