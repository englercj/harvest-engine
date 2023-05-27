// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/clock.h"
#include "he/core/enum_ops.h"
#include "he/core/hash_table.h"
#include "he/core/string.h"
#include "he/core/toml_reader.h"
#include "he/core/toml_writer.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/variant.h"
#include "he/core/vector.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    class TomlValue
    {
    public:
        using ArrayType = Vector<TomlValue>;
        using TableType = HashMap<String, TomlValue>;
        using VariantType = Variant<bool, int64_t, uint64_t, double, String, SystemTime, Duration, TableType, ArrayType>;

        enum class Kind : uint32_t
        {
            Bool,
            Int,
            Uint,
            Float,
            String,
            DateTime,
            Time,
            Table,
            Array,
            Invalid,
        };

    public:
        TomlValue() noexcept : m_value() {}
        TomlValue(bool v) noexcept : m_value(IndexConstant<AsUnderlyingType(Kind::Bool)>{}, v) {}
        TomlValue(int64_t v) noexcept : m_value(IndexConstant<AsUnderlyingType(Kind::Int)>{}, v) {}
        TomlValue(uint64_t v) noexcept : m_value(IndexConstant<AsUnderlyingType(Kind::Uint)>{}, v) {}
        TomlValue(double v) noexcept : m_value(IndexConstant<AsUnderlyingType(Kind::Float)>{}, v) {}
        TomlValue(StringView v) noexcept : m_value(IndexConstant<AsUnderlyingType(Kind::String)>{}, v) {}
        TomlValue(SystemTime v) noexcept : m_value(IndexConstant<AsUnderlyingType(Kind::DateTime)>{}, v) {}
        TomlValue(Duration v) noexcept : m_value(IndexConstant<AsUnderlyingType(Kind::Time)>{}, v) {}

        Kind GetKind() const { return m_value.IsValid() ? static_cast<Kind>(m_value.Index()) : Kind::Invalid; }
        bool IsValid() const { return m_value.IsValid(); }
        void Clear() { m_value.Clear(); }

        template <Kind K> bool Is() const { return m_value.Index() == AsUnderlyingType(K); }
        bool IsBool() const { return Is<Kind::Bool>(); }
        bool IsInt() const { return Is<Kind::Int>(); }
        bool IsUint() const { return Is<Kind::Uint>(); }
        bool IsFloat() const { return Is<Kind::Float>(); }
        bool IsString() const { return Is<Kind::String>(); }
        bool IsDateTime() const { return Is<Kind::DateTime>(); }
        bool IsTime() const { return Is<Kind::Time>(); }
        bool IsTable() const { return Is<Kind::Table>(); }
        bool IsArray() const { return Is<Kind::Array>(); }

        template <Kind K> decltype(auto) Set() { return m_value.Emplace<AsUnderlyingType(K)>(); }
        void SetBool(bool v) { Set<Kind::Bool>() = v; }
        void SetInt(int64_t v) { Set<Kind::Int>() = v; }
        void SetUint(uint64_t v) { Set<Kind::Uint>() = v; }
        void SetFloat(double v) { Set<Kind::Float>() = v; }
        void SetString(StringView v) { Set<Kind::String>() = v; }
        void SetDateTime(SystemTime v) { Set<Kind::DateTime>() = v; }
        void SetTime(Duration v) { Set<Kind::Time>() = v; }
        void SetTable(Allocator& allocator) { m_value.Emplace<AsUnderlyingType(Kind::Table)>(allocator); }
        void SetArray(Allocator& allocator) { m_value.Emplace<AsUnderlyingType(Kind::Array)>(allocator); }

        template <Kind K> decltype(auto) Get() { return m_value.Get<AsUnderlyingType(K)>(); }
        template <Kind K> decltype(auto) Get() const { return m_value.Get<AsUnderlyingType(K)>(); }
        bool Bool() const { return Get<Kind::Bool>(); }
        int64_t Int() const { return Get<Kind::Int>(); }
        uint64_t Uint() const { return Get<Kind::Uint>(); }
        double Float() const { return Get<Kind::Float>(); }
        StringView String() const { return Get<Kind::String>(); }
        SystemTime DateTime() const { return Get<Kind::DateTime>(); }
        Duration Time() const { return Get<Kind::Time>(); }
        TableType& Table() { return Get<Kind::Table>(); }
        const TableType& Table() const { return Get<Kind::Table>(); }
        ArrayType& Array() { return Get<Kind::Array>(); }
        const ArrayType& Array() const { return Get<Kind::Array>(); }

        template <typename T>
        T As() const
        {
            switch (GetKind())
            {
                case Kind::Bool: return static_cast<T>(Bool());
                case Kind::Int: return static_cast<T>(Int());
                case Kind::Uint: return static_cast<T>(Uint());
                case Kind::Float: return static_cast<T>(Float());
                case Kind::String: return static_cast<T>(String());
                case Kind::DateTime: return static_cast<T>(DateTime());
                case Kind::Time: return static_cast<T>(Time());
                default: return T();
            }
        }

    private:
        VariantType m_value;
    };

    // --------------------------------------------------------------------------------------------
    class TomlDocument
    {
    public:
        using TableType = TomlValue::TableType;
        using ArrayType = TomlValue::ArrayType;

        explicit TomlDocument(Allocator& allocator = Allocator::GetDefault()) noexcept;

        TomlReadResult Read(StringView data);
        void Write(String& dst);

        const TomlValue& Root() const { return m_root; }
        TomlValue& Root() { return m_root; }

        Allocator& GetAllocator() const { return m_allocator; }

    private:
        class ReadHandler : public TomlReader::Handler
        {
        public:
            explicit ReadHandler(TomlDocument& doc) noexcept;

            bool StartDocument() override;
            bool EndDocument() override;
            bool Comment(StringView value) override;
            bool Bool(bool value) override;
            bool Int(int64_t value) override;
            bool Uint(uint64_t value) override;
            bool Float(double value) override;
            bool String(StringView value) override;
            bool DateTime(SystemTime value) override;
            bool Time(Duration value) override;
            bool Table(Span<const he::String> path, bool isArray) override;
            bool Key(Span<const he::String> path) override;
            bool StartInlineTable() override;
            bool EndInlineTable(uint32_t length) override;
            bool StartArray() override;
            bool EndArray(uint32_t length) override;

            bool WalkPath(Span<const he::String> path);

            TomlDocument& m_doc;
            TomlValue* m_value{ nullptr };
            TomlReadResult m_result{};
        };

        class WriteHandler
        {
        public:
            void WriteValue(TomlWriter& writer, const TomlValue& value);

        private:
            void WriteTable(TomlWriter& writer, const TomlValue& value);
            void WriteArray(TomlWriter& writer, const TomlValue& value);

        private:
            Vector<StringView> m_keys;
            uint32_t m_arrayDepth{ 0 };
            bool m_nextTableIsArray{ false };
        };

    private:
        Allocator& m_allocator;
        TomlValue m_root;
    };
}
