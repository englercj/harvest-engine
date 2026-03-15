// Copyright Chad Engler

#pragma once

#include "he/sqlite/orm.h"

#include "he/core/clock.h"
#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/enum_ops.h"
#include "he/core/fmt.h"
#include "he/core/invoke.h"
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
#if HE_SIZEOF_LONG == 4
    template <AnyOf<signed char, short, int, long> T>
#else
    template <AnyOf<signed char, short, int> T>
#endif
    struct SqlDataTypeTraits<T>
    {
        static constexpr StringView SqlType = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { return stmt.Bind(index, static_cast<int32_t>(value)); }
        static void Read(const ColumnReader& column, T& value) { value = static_cast<T>(column.AsInt()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", value); }
    };

#if HE_SIZEOF_LONG == 4
    template <AnyOf<unsigned char, unsigned short, unsigned int, unsigned long> T>
#else
    template <AnyOf<unsigned char, unsigned short, unsigned int> T>
#endif
    struct SqlDataTypeTraits<T>
    {
        static constexpr StringView SqlType = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { return stmt.Bind(index, static_cast<uint32_t>(value)); }
        static void Read(const ColumnReader& column, T& value) { value = static_cast<T>(column.AsUint()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", value); }
    };

#if HE_SIZEOF_LONG == 8
    template <AnyOf<long long, long> T>
#else
    template <AnyOf<long long> T>
#endif
    struct SqlDataTypeTraits<T>
    {
        static constexpr StringView SqlType = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { return stmt.Bind(index, static_cast<int64_t>(value)); }
        static void Read(const ColumnReader& column, T& value) { value = static_cast<T>(column.AsInt64()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", value); }
    };

#if HE_SIZEOF_LONG == 8
    template <AnyOf<unsigned long long, unsigned long> T>
#else
    template <AnyOf<unsigned long long> T>
#endif
    struct SqlDataTypeTraits<T>
    {
        static constexpr StringView SqlType = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { return stmt.Bind(index, BitCast<int64_t>(value)); }
        static void Read(const ColumnReader& column, T& value) { value = BitCast<T>(column.AsInt64()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{}", value); }
    };

    template <>
    struct SqlDataTypeTraits<char>
    {
        static constexpr StringView SqlType = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, char value) { return stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, char& value) { value = static_cast<char>(column.AsInt64()); }
        static void Write(StringBuilder& sql, char value) { sql.Write("'{}'", value); }
    };

    template <Enum T>
    struct SqlDataTypeTraits<T>
    {
        using U = UnderlyingType<T>;
        using Traits = SqlDataTypeTraits<U>;

        static constexpr StringView SqlType = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { return Traits::Bind(stmt, index, EnumToValue(value)); }
        static void Read(const ColumnReader& column, T& value) { U v; Traits::Read(column, v); value = static_cast<T>(v); }
        static void Write(StringBuilder& sql, const T& value) { Traits::Write(sql, EnumToValue(value)); }
    };

    template <typename T>
    struct SqlDataTypeTraits<Time<T>>
    {
        static constexpr StringView SqlType = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const Time<T>& value) { return stmt.Bind(index, BitCast<int64_t>(value.val)); }
        static void Read(const ColumnReader& column, Time<T>& value) { value.val = BitCast<uint64_t>(column.AsInt64()); }
        static void Write(StringBuilder& sql, const Time<T>& value) { sql.Write("{}", BitCast<int64_t>(value.val)); }
    };

    template <>
    struct SqlDataTypeTraits<Duration>
    {
        static constexpr StringView SqlType = "INTEGER";
        static bool Bind(Statement& stmt, int32_t index, const Duration& value) { return stmt.Bind(index, value.val); }
        static void Read(const ColumnReader& column, Duration& value) { value.val = column.AsInt64(); }
        static void Write(StringBuilder& sql, const Duration& value) { sql.Write("{}", value.val); }
    };

    // REAL
    template <FloatingPoint T>
    struct SqlDataTypeTraits<T>
    {
        static constexpr StringView SqlType = "REAL";
        static bool Bind(Statement& stmt, int32_t index, const T& value) { return stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, T& value) { value = static_cast<T>(column.AsDouble()); }
        static void Write(StringBuilder& sql, const T& value) { sql.Write("{:.15}", value); }
    };

    // TEXT
    template <>
    struct SqlDataTypeTraits<String>
    {
        static constexpr StringView SqlType = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const String& value) { return stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, String& value) { value = column.AsText(); }
        static void Write(StringBuilder& sql, const String& value) { sql.Write("'{}'", value); }
    };

    template <>
    struct SqlDataTypeTraits<Vector<char>>
    {
        static constexpr StringView SqlType = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const Vector<char>& value) { return stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, Vector<char>& value) { value = column.AsText(); }
        static void Write(StringBuilder& sql, const Vector<char>& value) { sql.Write("'{}'", FmtJoin(value.Begin(), value.End(), "")); }
    };

    template <>
    struct SqlDataTypeTraits<StringView>
    {
        static constexpr StringView SqlType = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const StringView& value) { return stmt.Bind(index, value); }
        static void Write(StringBuilder& sql, const StringView& value) { sql.Write("'{}'", value); }
        // Note: No Read() because `StringView` is non-owning, and reading a value into it would be unsafe.
    };

    template <>
    struct SqlDataTypeTraits<Span<char>>
    {
        static constexpr StringView SqlType = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const Span<char>& value) { return stmt.Bind(index, value); }
        static void Write(StringBuilder& sql, const Span<char>& value) { sql.Write("'{}'", FmtJoin(value.Begin(), value.End(), "")); }
        // Note: No Read() because `Span<char>` is non-owning, and reading a value into it would be unsafe.
    };

    template <>
    struct SqlDataTypeTraits<const char*>
    {
        static constexpr StringView SqlType = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const char* value) { return stmt.Bind(index, value); }
        static void Write(StringBuilder& sql, const char* value) { sql.Write("'{}'", value); }
        // Note: No Read() because `const char*` is non-owning, and reading a value into it would be unsafe.
    };

    template <size_t N>
    struct SqlDataTypeTraits<char[N]>
    {
        static constexpr StringView SqlType = "TEXT";
        static bool Bind(Statement& stmt, int32_t index, const char (&value)[N]) { return stmt.Bind(index, value); }
        static void Write(StringBuilder& sql, const char (&value)[N]) { sql.Write("'{}'", value); }
        // Note: No Read() because `const char*` is non-owning, and reading a value into it would be unsafe.
    };

    // BLOB
    template <>
    struct SqlDataTypeTraits<Vector<uint8_t>>
    {
        static constexpr StringView SqlType = "BLOB";
        static bool Bind(Statement& stmt, int32_t index, const Vector<uint8_t>& value) { return stmt.Bind(index, value); }
        static void Read(const ColumnReader& column, Vector<uint8_t>& value) { value = column.AsBlob(); }
        static void Write(StringBuilder& sql, const Vector<uint8_t>& value) { sql.Write("X'{:02x}'", FmtJoin(value.Begin(), value.End(), "")); }
    };

    template <>
    struct SqlDataTypeTraits<Uuid>
    {
        static constexpr StringView SqlType = "BLOB(16)";
        static bool Bind(Statement& stmt, int32_t index, const Uuid& value) { return stmt.Bind(index, value.m_bytes); }
        static void Read(const ColumnReader& column, Uuid& value) { column.ReadBlob(value.m_bytes); }
        static void Write(StringBuilder& sql, const Uuid& value) { sql.Write("X'{:02x}'", FmtJoin(value.m_bytes, value.m_bytes + sizeof(value.m_bytes), "")); }
    };

    template <>
    struct SqlDataTypeTraits<Span<uint8_t>>
    {
        static constexpr StringView SqlType = "BLOB";
        static bool Bind(Statement& stmt, int32_t index, const Span<uint8_t>& value) { return stmt.Bind(index, value); }
        static void Write(StringBuilder& sql, const Span<uint8_t>& value) { sql.Write("X'{:02x}'", FmtJoin(value.Begin(), value.End(), "")); }
        // Note: No Read() because `Span<uint8_t>` is non-owning, and reading a value into it would be unsafe.
    };

    // --------------------------------------------------------------------------------------------
    // ToSql

    template <typename T>
    struct SqlWriter;

    struct SqlWriterContextBase
    {
        /// When true column names will be prefixed with the table name.
        /// Useful for queries that reference multiple tables.
        bool writeColumnTableNames{ false };

        /// When true writes values directly into the query.
        /// Default (false) is to use a placeholder tokens (`?`).
        bool writeRawValues{ false };
    };

    template <SpecializationOf<SchemaDef> T>
    struct SqlWriterContext : SqlWriterContextBase
    {
        using SchemaType = T;

        constexpr explicit SqlWriterContext(const T& schema) : schema(schema) {}

        template <typename T>
        constexpr decltype(auto) GetTable() const
        {
            return schema.template TableFor<T>();
        }

        template <typename T>
        constexpr StringView GetTableName() const
        {
            return schema.template TableFor<T>().Name();
        }

        template <typename T> requires(IsMemberObjectPointer<T>)
        constexpr StringView GetColumnName(T column) const
        {
            return schema.template TableFor<MemberPointerObjectType<T>>().GetColumnName(column);
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

    template <SpecializationOf<SchemaDef> T>
    struct SqlBinderContext
    {
        using SchemaType = T;

        constexpr explicit SqlBinderContext(const T& schema) : schema(schema) {}

        template <typename T>
        constexpr decltype(auto) GetTable() const
        {
            return schema.template TableFor<T>();
        }

        const SchemaType& schema;
        int32_t index{ 1 };
    };

    template <typename T, typename U>
    bool BindSql(Statement& stmt, const T& t, SqlBinderContext<U>& ctx)
    {
        SqlBinder<T> binder;
        return binder.Bind(stmt, t, ctx);
    }

    template <typename T>
    bool BindSql(Statement& stmt, const T& t)
    {
        SqlBinderContext ctx;
        return BindSql(stmt, t, ctx);
    }

    // --------------------------------------------------------------------------------------------
    // ReadSql

    template <typename T>
    void ReadSql(const ColumnReader& column, T& value)
    {
        using Traits = SqlDataTypeTraits<T>;
        Traits::Read(column, value);
    }
}

#include "he/sqlite/inline/orm_sql_binder.inl"
#include "he/sqlite/inline/orm_sql_writer.inl"
