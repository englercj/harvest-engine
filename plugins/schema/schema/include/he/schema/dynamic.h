// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/schema/schema.h"

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    class DynamicEnum;

    struct DynamicValue
    {
        DynamicValue() = delete;

        enum class Kind
        {
            /// Unsure how to represent the value, so we don't know what it is. This is commonly
            /// a result of using an older schema that doesn't know about a new type.
            Unknown,
            Void,
            Bool,
            Int,
            Uint,
            Float,
            Blob,
            String,
            List,
            Enum,
            Struct,
            AnyPointer,
        };

        class Reader;
        class Builder;
    };

    struct DynamicStruct
    {
        DynamicStruct() = delete;

        class Reader;
        class Builder;
    };

    struct DynamicList
    {
        DynamicList() = delete;

        class Reader;
        class Builder;
    };

    template <> struct TypeHelper<DynamicEnum> { using Reader = DynamicEnum; using Builder = DynamicEnum; };

    // --------------------------------------------------------------------------------------------
    class DynamicEnum
    {
    public:
        DynamicEnum() = default;

        DynamicEnum(const DeclInfo& info, uint16_t value) noexcept
            : m_info(&info)
            , m_value(value)
        {}

    public:
        const DeclInfo& Decl() const { HE_ASSERT(m_info); return *m_info; }
        Declaration::Reader Schema() const { return GetSchema(Decl()); }
        Declaration::Data::Enum::Reader EnumSchema() const { return Schema().GetData().GetEnum(); }

        uint16_t Value() const { return m_value; }

        Enumerator::Reader Enumerator() const
        {
            auto enumerators = EnumSchema().GetEnumerators();
            return m_value < enumerators.Size() ? enumerators[m_value] : Enumerator::Reader{};
        }

        template <Enum T>
        T As() const
        {
            HE_VERIFY(EnumInfo<T>::Id = Schema().GetId(),
                HE_MSG("DynamicEnum requested as a type that doesn't match the schema."),
                HE_KV(requested_type_name, GetSchema(EnumInfo<T>::DeclInfo).GetName()),
                HE_KV(requested_type_id, EnumInfo<T>::Id),
                HE_KV(actual_type_name, Schema().GetName()),
                HE_KV(actual_type_id, Schema().GetId()));
            return static_cast<T>(m_value);
        }

    private:
        const DeclInfo* m_info{ nullptr };
        uint16_t m_value{};
    };

    // --------------------------------------------------------------------------------------------
    class DynamicStruct::Reader
    {
    public:
        Reader() = default;

        Reader(const DeclInfo& info, StructReader reader) noexcept
            : m_info(&info)
            , m_reader(reader)
        {}

        template <typename T> requires(T::StructType::Kind == DeclKind::Struct)
        Reader(T&& reader) noexcept : Reader(T::StructType::DeclInfo, reader) {}

    public:
        const DeclInfo& Decl() const { HE_ASSERT(m_info); return *m_info; }
        Declaration::Reader Schema() const { return GetSchema(Decl()); }
        Declaration::Data::Struct::Reader StructSchema() const { return Schema().GetData().GetStruct(); }

        DynamicValue::Reader GetField(Field::Reader field) const;
        DynamicValue::Reader GetField(StringView fieldName) const;

        bool HasField(Field::Reader field) const;
        bool HasField(StringView fieldName) const;

        bool IsActiveInUnion(Field::Reader field) const;

        Field::Reader ActiveUnionField() const;

        template <typename T> requires(T::Kind == DeclKind::Struct)
        typename T::Reader As() const
        {
            const bool valid = HE_VERIFY(&T::DeclInfo = m_info,
                HE_MSG("DynamicStruct requested as a type that doesn't match the schema."),
                HE_KV(requested_type_name, GetSchema(T::DeclInfo).GetName()),
                HE_KV(requested_type_id, T::Id),
                HE_KV(actual_type_name, Schema().GetName()),
                HE_KV(actual_type_id, Schema().GetId()));
            return valid ? typename T::Reader(m_reader) : typename T::Reader{};
        }

        StructReader Struct() const { return m_reader; }

    private:
        bool IsActiveInUnion(const DeclInfo& info, Field::Reader field) const;

    private:
        const DeclInfo* m_info{ nullptr };
        StructReader m_reader{};
    };

    class DynamicStruct::Builder
    {
    public:
        Builder() = default;

        Builder(const DeclInfo& info, StructBuilder builder) noexcept
            : m_info(&info)
            , m_builder(builder)
        {}

        template <typename T> requires(T::StructType::Kind == DeclKind::Struct)
        Builder(T&& builder) noexcept : Builder(T::StructType::DeclInfo, builder) {}

    public:
        const DeclInfo& Decl() const { HE_ASSERT(m_info); return *m_info; }
        Declaration::Reader Schema() const { return GetSchema(Decl()); }
        Declaration::Data::Struct::Reader StructSchema() const { return Schema().GetData().GetStruct(); }

        DynamicValue::Builder GetField(Field::Reader field) const;
        DynamicValue::Builder GetField(StringView fieldName) const;

        void SetField(Field::Reader field, const DynamicValue::Reader& value);
        void SetField(StringView fieldName, const DynamicValue::Reader& value);

        DynamicValue::Builder InitField(Field::Reader field);
        DynamicValue::Builder InitField(StringView fieldName);

        DynamicValue::Builder InitField(Field::Reader field, uint32_t size);
        DynamicValue::Builder InitField(StringView fieldName, uint32_t size);

        void ClearField(Field::Reader field);
        void ClearField(StringView fieldName);

        bool HasField(Field::Reader field) const;
        bool HasField(StringView fieldName) const;

        bool IsActiveInUnion(Field::Reader field) const;
        Field::Reader ActiveUnionField() const;

        template <typename T> requires(T::Kind == DeclKind::Struct)
        typename T::Builder As() const
        {
            const bool valid = HE_VERIFY(&T::DeclInfo = m_info,
                HE_MSG("DynamicStruct requested as a type that doesn't match the schema."),
                HE_KV(requested_type_name, GetSchema(T::DeclInfo).GetName()),
                HE_KV(requested_type_id, T::Id),
                HE_KV(actual_type_name, Schema().GetName()),
                HE_KV(actual_type_id, Schema().GetId()));
            return valid ? typename T::Builder(m_builder) : typename T::Builder{};
        }

        Reader AsReader() const { return Reader(*m_info, m_builder.AsReader()); }

        StructBuilder Struct() const { return m_builder; }

    private:
        void SetInUnion(Field::Reader field);
        bool IsActiveInUnion(const DeclInfo& info, Field::Reader field) const;

    private:
        const DeclInfo* m_info{ nullptr };
        StructBuilder m_builder{};
    };

    // --------------------------------------------------------------------------------------------
    class DynamicList::Reader
    {
    public:
        using IteratorType = ListIterator<DynamicList::Reader>;

    public:
        Reader() = default;

        Reader(const DeclInfo& parentInfo, Type::Reader type, ListReader reader) noexcept
            : m_parentInfo(&parentInfo)
            , m_type(type)
            , m_reader(reader)
        {
            HE_ASSERT(GetSchema(parentInfo).GetData().IsStruct());
            HE_ASSERT(type.GetData().IsList());
        }

    public:
        Type::Reader Type() const { return m_type; }
        // const DeclInfo& ElementDecl() const { HE_ASSERT(m_elementInfo); return *m_elementInfo; }
        // Declaration::Reader ElementSchema() const { return GetSchema(ElementDecl()); }

        uint32_t Size() const { return m_reader.Size(); }

        DynamicValue::Reader Get(uint32_t index) const;

        DynamicValue::Reader operator[](uint32_t index) const { return Get(index); }

        template <typename T>
        typename List<T>::Reader AsListOf() const
        {
            // TODO!
            // const bool valid = HE_VERIFY(&T::DeclInfo = m_elementInfo,
            //     HE_MSG("DynamicList requested as an element type that doesn't match the schema."),
            //     HE_KV(requested_type_name, GetSchema(T::DeclInfo).GetName()),
            //     HE_KV(requested_type_id, T::Id),
            //     HE_KV(actual_type_name, ElementSchema().GetName()),
            //     HE_KV(actual_type_id, ElementSchema().GetId()));
            return valid ? typename List<T>::Reader(m_reader) : typename List<T>::Reader{};
        }

        ListReader List() const { return m_reader; }

        IteratorType begin() const { return IteratorType(this, 0); }
        IteratorType end() const { return IteratorType(this, m_reader.Size()); }

    private:
        const DeclInfo* m_parentInfo{ nullptr };
        Type::Reader m_type{};
        ListReader m_reader{};
    };

    class DynamicList::Builder
    {
    public:
        using IteratorType = ListIterator<DynamicList::Builder>;

    public:
        Builder() = default;

        Builder(const DeclInfo& parentInfo, Type::Reader type, ListBuilder builder) noexcept
            : m_parentInfo(&parentInfo)
            , m_type(type)
            , m_builder(builder)
        {
            HE_ASSERT(GetSchema(parentInfo).GetData().IsStruct());
            HE_ASSERT(type.GetData().IsList());
        }

    public:
        // const DeclInfo& ElementDecl() const { HE_ASSERT(m_elementInfo); return *m_elementInfo; }
        // Declaration::Reader ElementSchema() const { return GetSchema(ElementDecl()); }

        uint32_t Size() const { return m_builder.Size(); }

        DynamicValue::Builder Get(uint32_t index) const;
        void Set(uint32_t index, const DynamicValue::Reader& value);
        DynamicValue::Builder Init(uint32_t index, uint32_t size);

        DynamicValue::Builder operator[](uint32_t index) const { return Get(index); }

        template <typename T>
        typename List<T>::Builder AsListOf() const
        {
            // TODO!
            // const bool valid = HE_VERIFY(&T::DeclInfo = m_elementInfo,
            //     HE_MSG("DynamicList requested as an element type that doesn't match the schema."),
            //     HE_KV(requested_type_name, GetSchema(T::DeclInfo).GetName()),
            //     HE_KV(requested_type_id, T::Id),
            //     HE_KV(actual_type_name, ElementSchema().GetName()),
            //     HE_KV(actual_type_id, ElementSchema().GetId()));
            return valid ? typename List<T>::Builder(m_builder) : typename List<T>::Builder{};
        }

        Reader AsReader() const { return Reader(m_type, m_builder.AsReader()); }

        ListBuilder List() const { return m_builder; }

        IteratorType begin() const { return IteratorType(this, 0); }
        IteratorType end() const { return IteratorType(this, Size()); }

    private:
        const DeclInfo* m_parentInfo{ nullptr };
        Type::Reader m_type{};
        ListBuilder m_builder{};
    };

    // --------------------------------------------------------------------------------------------
    class DynamicValue::Reader
    {
    public:
        // These constructors are intentionally not marked as `explicit` to allow implicit conversions.

        Reader(decltype(nullptr) value = nullptr) noexcept : m_kind(Kind::Unknown) {}
        Reader(Void value) noexcept : m_kind(Kind::Void), m_void(value) {}
        Reader(bool value) noexcept : m_kind(Kind::Bool), m_bool(value) {}
        Reader(char value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Reader(signed char value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Reader(short value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Reader(int value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Reader(long value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Reader(long long value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Reader(unsigned char value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Reader(unsigned short value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Reader(unsigned int value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Reader(unsigned long value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Reader(unsigned long long value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Reader(float value) noexcept : m_kind(Kind::Float), m_float(value) {}
        Reader(double value) noexcept : m_kind(Kind::Float), m_float(value) {}
        Reader(const String::Reader& value) noexcept : m_kind(Kind::String), m_string(value) {}
        Reader(const Blob::Reader& value) noexcept : m_kind(Kind::Blob), m_blob(value) {}
        Reader(const DynamicList::Reader& value) noexcept : m_kind(Kind::List), m_list(value) {}
        Reader(DynamicEnum value) noexcept : m_kind(Kind::Enum), m_enum(value) {}
        Reader(const DynamicStruct::Reader& value) noexcept : m_kind(Kind::Struct), m_struct(value) {}
        Reader(const PointerReader& value) noexcept : m_kind(Kind::AnyPointer), m_anyPointer(value) {}

    public:
        Kind GetKind() const { return m_kind; }

        template <typename T>
        typename TypeHelper<T>::Reader As() const;

    private:
        Kind m_kind;

        union
        {
            Void m_void;
            bool m_bool;
            int64_t m_int;
            uint64_t m_uint;
            double m_float;
            String::Reader m_string;
            Blob::Reader m_blob;
            DynamicList::Reader m_list;
            DynamicEnum m_enum;
            DynamicStruct::Reader m_struct;
            PointerReader m_anyPointer;
        };
    };

    class DynamicValue::Builder
    {
    public:
        // These constructors are intentionally not marked as `explicit` to allow implicit conversions.

        Builder(decltype(nullptr) value = nullptr) noexcept : m_kind(Kind::Unknown) {}
        Builder(Void value) noexcept : m_kind(Kind::Void), m_void(value) {}
        Builder(bool value) noexcept : m_kind(Kind::Bool), m_bool(value) {}
        Builder(char value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Builder(signed char value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Builder(short value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Builder(int value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Builder(long value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Builder(long long value) noexcept : m_kind(Kind::Int), m_int(value) {}
        Builder(unsigned char value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Builder(unsigned short value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Builder(unsigned int value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Builder(unsigned long value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Builder(unsigned long long value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
        Builder(float value) noexcept : m_kind(Kind::Float), m_float(value) {}
        Builder(double value) noexcept : m_kind(Kind::Float), m_float(value) {}
        Builder(const String::Builder& value) noexcept : m_kind(Kind::String), m_string(value) {}
        Builder(const Blob::Builder& value) noexcept : m_kind(Kind::Blob), m_blob(value) {}
        Builder(const DynamicList::Builder& value) noexcept : m_kind(Kind::List), m_list(value) {}
        Builder(DynamicEnum value) noexcept : m_kind(Kind::Enum), m_enum(value) {}
        Builder(const DynamicStruct::Builder& value) noexcept : m_kind(Kind::Struct), m_struct(value) {}
        Builder(const PointerBuilder& value) noexcept : m_kind(Kind::AnyPointer), m_anyPointer(value) {}

    public:
        Kind GetKind() const { return m_kind; }

        template <typename T>
        typename TypeHelper<T>::Builder As() const;

        Reader AsReader() const;

    private:
        Kind m_kind;

        union
        {
            Void m_void;
            bool m_bool;
            int64_t m_int;
            uint64_t m_uint;
            double m_float;
            String::Builder m_string;
            Blob::Builder m_blob;
            DynamicList::Builder m_list;
            DynamicEnum m_enum;
            DynamicStruct::Builder m_struct;
            PointerBuilder m_anyPointer;
        };
    };
}
