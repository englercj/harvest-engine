// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/string_builder.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/sqlite/database.h"
#include "he/sqlite/orm.h"
#include "he/sqlite/orm_sql.h"
#include "he/sqlite/statement.h"

namespace he::sqlite
{
    template <typename S>
    class Storage
    {
    public:
        using SchemaType = S;
        static_assert(IsSpecialization<S, SchemaDef>, "Storage expects a SchemaDef object.");

    public:
        explicit Storage(SchemaType&& schema)
            : m_db()
            , m_schema(Move(schema))
        {}

        // Storage state getters
    public:
        bool Open(const char* path) { return m_db.Open(path); }
        bool Close() { return m_db.Close(); }

        bool IsOpen() const { return m_db.IsOpen(); }

        // Table functions
    public:
        template <typename T>
        bool CreateTable();

        template <typename T>
        bool DropTable();

        bool Sync();

        // Query functions
    public:
        template <typename T>
        bool Query(const T& query);

        template <typename T, typename F>
        bool Query(const T& query, F&& rowIterator);

        // Model functions
    public:
        template <typename T>
        bool Create(const T& obj);

        template <typename T, QueryCondition... U>
        bool Destroy(U&&... query);

        template <typename T, QueryCondition... U>
        bool FindAll(Vector<T>& out, U&&... conditions);

        template <typename T, QueryCondition... U>
        bool FindOne(T& out, U&&... conditions);

        template <typename T>
        bool Update(const T& obj);

    private:
        template <typename T>
        bool PrepareQuery(Statement& stmt, const T& query);

    private:
        Database m_db;
        SchemaType m_schema;
    };

    // --------------------------------------------------------------------------------------------
    // Inline definitions

    template <typename S>
    template <typename T>
    inline bool Storage<S>::CreateTable()
    {
        const auto& table = m_schema.TableFor<T>();
        SqlWriterContext ctx(m_schema);
        StringBuilder sql;
        ToSql(sql, obj, ctx);
        return m_db.Execute(sql.Str().Data());
    }

    template <typename S>
    template <typename T>
    inline bool Storage<S>::DropTable()
    {
        const auto& table = m_schema.TableFor<T>();
        String sql = "DROP TABLE IF EXISTS ";
        sql += table.Name();
        return m_db.Execute(sql.Data());
    }

    template <typename S>
    inline bool Storage<S>::Sync()
    {
        // TODO
    }

    template <typename S>
    template <typename T>
    inline bool Storage<S>::Query(const T& query)
    {
        Statement stmt;
        if (!PrepareQuery(stmt, query))
            return false;

        return stmt.Step() != StepResult::Error;
    }

    template <typename S>
    template <typename T, typename F>
    inline bool Storage<S>::Query(const T& query, F&& rowIterator)
    {
        Statement stmt;
        if (!PrepareQuery(stmt, query))
            return false;

        return stmt.EachRow(rowIterator);
    }

    template <typename S>
    template <typename T>
    inline bool Storage<S>::Create(const T& obj)
    {
        const auto query = Insert(obj);
        return Query(query);
    }

    template <typename S>
    template <typename T, QueryCondition... U>
    inline bool Storage<S>::Destroy(U&&... query)
    {
        const auto query = Delete<T>(Forward<U>(query)...);
        return Query(query);
    }

    template <typename S>
    template <typename T, QueryCondition... U>
    inline bool Storage<S>::FindAll(Vector<T>& out, U&&... conditions)
    {
        const auto query = Select<T>(conditions);
        return Query(query, [&](Statement& stmt)
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
    template <typename T, QueryCondition... U>
    inline bool Storage<S>::FindOne(T& out, U&&... conditions)
    {
        const auto query = Select<T>(conditions, Limit(1));
        return Query(query, [&](Statement& stmt)
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
    inline bool Storage<S>::Update(const T& obj)
    {
        const auto query = Update(obj);
        return Query(query);
    }

    template <typename S>
    template <typename T>
    inline bool Storage<S>::PrepareQuery(Statement& stmt, const T& query)
    {
        SqlWriterContext ctx(m_schema);
        StringBuilder sql;
        ToSql(sql, query, ctx);

        if (!HE_VERIFY(stmt.Prepare(m_db, sql.Data())))
            return false;

        if (!HE_VERIFY(BindSql(stmt, query)))
            return false;

        return true;
    }
}
