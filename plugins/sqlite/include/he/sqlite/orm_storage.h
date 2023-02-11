// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/string_builder.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/sqlite/database.h"
#include "he/sqlite/orm.h"
#include "he/sqlite/orm_sql.h"

namespace he::sqlite
{
    template <typename S>
    class Storage
    {
    public:
        using SchemaType = S;
        static_assert(IsSpecialization<S, SchemaDef>, "Storage expects a SchemaDef object.");

    public:
        explicit Storage(StringView path, SchemaType&& schema)
            : m_db()
            , m_path(path)
            , m_schema(Move(schema))
        {}

    public:
        bool Open() { return m_db.Open(m_path.Data()); }
        bool Close() { return m_db.Close(); }

        bool IsOpen() const { return m_db.IsOpen(); }

        const String& Path() const { return m_path; }

    public:
        template <typename T>
        bool CreateTable();

        template <typename T>
        bool DropTable();

        bool Sync();

    public:
        template <typename T, QueryCondition... U>
        bool Create(const T& obj, U&&... query);

        template <typename T, QueryCondition... U>
        bool Destroy(U&&... query);

        template <typename T, QueryCondition... U>
        bool FindAll(Vector<T>& out, U&&... query);

        template <typename T, QueryCondition... U>
        bool FindOne(T& out, U&&... query);

        template <typename T>
        bool FindByPk(T& out, uint64_t pk);

        template <typename T, QueryCondition... U>
        bool FindOrCreate(T& out, U&&... query);

        template <typename T, QueryCondition... U>
        bool Update(const T& obj, U&&... query);

        template <typename T, QueryCondition... U>
        bool Upsert(const T& obj, U&&... query);

    private:

    private:
        Database m_db;
        String m_path;
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

    }

    template <typename S>
    template <typename T, QueryCondition... U>
    bool Storage<S>::Create(const T& obj, U&&... query)
    {

    }

    template <typename S>
    template <typename T, QueryCondition... U>
    bool Storage<S>::Destroy(U&&... query)
    {

    }

    template <typename S>
    template <typename T, QueryCondition... U>
    bool Storage<S>::FindAll(Vector<T>& out, U&&... query)
    {

    }

    template <typename S>
    template <typename T, QueryCondition... U>
    bool Storage<S>::FindOne(T& out, U&&... query)
    {

    }

    template <typename S>
    template <typename T>
    bool Storage<S>::FindByPk(T& out, uint64_t pk)
    {

    }

    template <typename S>
    template <typename T, QueryCondition... U>
    bool Storage<S>::FindOrCreate(T& out, U&&... query)
    {

    }

    template <typename S>
    template <typename T, QueryCondition... U>
    bool Storage<S>::Update(const T& obj, U&&... query)
    {

    }

    template <typename S>
    template <typename T, QueryCondition... U>
    bool Storage<S>::Upsert(const T& obj, U&&... query)
    {

    }
}
