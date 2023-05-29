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

        enum class Kind : uint8_t
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
        TableType& SetTable(Allocator& allocator = Allocator::GetDefault()) { return m_value.Emplace<AsUnderlyingType(Kind::Table)>(allocator); }
        ArrayType& SetArray(Allocator& allocator = Allocator::GetDefault()) { return m_value.Emplace<AsUnderlyingType(Kind::Array)>(allocator); }

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

        TomlValue& operator=(bool v) { SetBool(v); return *this; }
        TomlValue& operator=(signed char v) { SetInt(v); return *this; }
        TomlValue& operator=(short v) { SetInt(v); return *this; }
        TomlValue& operator=(int v) { SetInt(v); return *this; }
        TomlValue& operator=(long v) { SetInt(v); return *this; }
        TomlValue& operator=(long long v) { SetInt(v); return *this; }
        TomlValue& operator=(unsigned char v) { SetUint(v); return *this; }
        TomlValue& operator=(unsigned short v) { SetUint(v); return *this; }
        TomlValue& operator=(unsigned int v) { SetUint(v); return *this; }
        TomlValue& operator=(unsigned long v) { SetUint(v); return *this; }
        TomlValue& operator=(unsigned long long v) { SetUint(v); return *this; }
        TomlValue& operator=(double v) { SetFloat(v); return *this; }
        TomlValue& operator=(const char* v) { SetString(v); return *this; }
        TomlValue& operator=(StringView v) { SetString(v); return *this; }
        TomlValue& operator=(SystemTime v) { SetDateTime(v); return *this; }
        TomlValue& operator=(Duration v) { SetTime(v); return *this; }

        TomlValue& operator[](StringView key) { HE_ASSERT(IsTable()); return Table()[key]; }
        TomlValue& operator[](uint32_t index) { HE_ASSERT(IsArray()); return Array()[index]; }

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
        void Write(String& dst) const;

        String ToString() const;

        const TomlValue& Root() const { return m_root; }
        TomlValue& Root() { return m_root; }

        Allocator& GetAllocator() const { return m_allocator; }

        TomlValue& operator[](StringView key) { return Root()[key]; }

    private:
        Allocator& m_allocator;
        TomlValue m_root;
    };
}
