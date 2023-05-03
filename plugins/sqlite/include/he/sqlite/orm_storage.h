// Copyright Chad Engler

#pragma once

#include "he/core/alloca.h"
#include "he/core/bitset.h"
#include "he/core/fmt.h"
#include "he/core/log.h"
#include "he/core/string_builder.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/sqlite/database.h"
#include "he/sqlite/orm.h"
#include "he/sqlite/orm_sql.h"
#include "he/sqlite/statement.h"
#include "he/sqlite/transaction.h"

namespace he::sqlite
{
    class StorageBase
    {
    public:
        StorageBase() = default;

    public:
        bool Open(const char* path) { return m_db.Open(path); }
        bool Close() { return m_db.Close(); }

        bool IsOpen() const { return m_db.IsOpen(); }

        Database& Db() { return m_db; }
        sqlite3* Handle() const { return m_db.Handle(); }

    public:
        template <typename T>
        bool Query(const T& query);

        template <typename T, typename F>
        bool Query(const T& query, F&& rowIterator);

    protected:
        bool DropColumn(StringView tableName, StringView columnName);

    protected:
        struct ColumnInfo
        {
            int32_t cid{ 0 };
            String name{};
            String type{};
            String defaultValue{};
            int32_t pk{ 0 };
            bool notnull{ false };
            bool hidden{ false };
        };

        struct TableInfo
        {
            bool exists{ false };
            Vector<ColumnInfo> columns{};
        };

        template <typename T, typename U> requires(IsTableDef<T>::Value && IsColumnDef<U>::Value)
        ColumnInfo ColumnInfoFromDef(const T& table, const U& column);

        bool QueryTableExists(StringView tableName);
        bool QueryTableInfo(StringView tableName, TableInfo& info);

    protected:
        Database m_db{};
    };

    template <typename S>
    class Storage : public StorageBase
    {
    public:
        using SchemaType = S;
        static_assert(IsSpecialization<S, SchemaDef>, "Storage expects a SchemaDef specialization as the template parameter.");

    public:
        explicit Storage(SchemaType&& schema)
            : StorageBase()
            , m_schema(Move(schema))
        {}

        // Table functions
    public:
        template <typename T>
        bool CreateTable();

        template <typename T>
        bool DropTable();

        bool Sync();

        // Model functions
    public:
        template <typename T>
        bool Create(const T& obj);

        template <typename T, QueryCondition... U>
        bool Destroy(U&&... conditions);

        template <typename T, QueryCondition... U>
        bool FindAll(Vector<T>& out, U&&... conditions);

        template <typename T, QueryCondition... U>
        bool FindOne(T& out, U&&... conditions);

        template <typename T>
        bool Update(const T& obj);

    private:
        using ColumnInfo = StorageBase::ColumnInfo;
        using TableInfo = StorageBase::TableInfo;

        template <typename T>
        bool PrepareQuery(Statement& stmt, const T& query);

        template <typename T, typename... Elements>
        bool SyncTable(const TableDef<T, Elements...>& table);

        template <typename T, typename U> requires(IsTableDef<T>::Value && IsColumnDef<U>::Value)
        bool SyncColumn(const T& table, const TableInfo& tableInfo, const U& column, const ColumnInfo* columnInfo, bool& columnModified);

        template <typename T, typename U, typename... Constraints>
        bool AddColumn(StringView tableName, const ColumnDef<T, U, Constraints...>& column);

    private:
        SchemaType m_schema;
    };
}

#include "he/sqlite/inline/orm_storage.inl"
