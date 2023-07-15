// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/schema/layout.h"
#include "he/schema/schema.h"

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    template <typename T>
    bool IsValidElementType(Type::Reader elementType);

    // --------------------------------------------------------------------------------------------
    class DynamicEnum;

    struct DynamicStruct
    {
        DynamicStruct() = delete;

        class Reader;
        class Builder;
    };

    struct DynamicArray
    {
        DynamicArray() = delete;

        class Reader;
        class Builder;
    };

    struct DynamicList
    {
        DynamicList() = delete;

        class Reader;
        class Builder;
    };

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
            Array,
            List,
            Enum,
            Struct,
            AnyPointer,
        };

        class Reader;
        class Builder;
    };

    template <>
    struct LayoutTraits<DynamicEnum>
    {
        using Reader = DynamicEnum;
        using Builder = DynamicEnum;
        static constexpr bool IsList = false;
    };

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

        Enumerator::Reader Enum() const
        {
            auto enumerators = EnumSchema().GetEnumerators();
            return m_value < enumerators.Size() ? enumerators[m_value] : Enumerator::Reader{};
        }

        template <he::Enum T>
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

        DynamicValue::Reader Get(Field::Reader field) const;
        DynamicValue::Reader Get(StringView fieldName) const;

        bool Has(Field::Reader field) const;
        bool Has(StringView fieldName) const;

        Field::Reader ActiveUnionField() const;

        template <typename T> requires(T::Kind == DeclKind::Struct)
        typename T::Reader As() const
        {
            const bool valid = HE_VERIFY(&T::DeclInfo == m_info,
                HE_MSG("DynamicStruct requested as a type that doesn't match the schema."),
                HE_KV(requested_type_name, GetSchema(T::DeclInfo).GetName()),
                HE_KV(requested_type_id, T::Id),
                HE_KV(actual_type_name, Schema().GetName()),
                HE_KV(actual_type_id, Schema().GetId()));
            return valid ? typename T::Reader(m_reader) : typename T::Reader{};
        }

        StructReader Struct() const { return m_reader; }

    private:
        bool IsActiveInUnion(Field::Reader field) const;

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
        Builder(const T& builder) noexcept : Builder(T::StructType::DeclInfo, builder) {}

    public:
        const DeclInfo& Decl() const { HE_ASSERT(m_info); return *m_info; }
        Declaration::Reader Schema() const { return GetSchema(Decl()); }
        Declaration::Data::Struct::Reader StructSchema() const { return Schema().GetData().GetStruct(); }

        DynamicValue::Builder Get(Field::Reader field) const;
        DynamicValue::Builder Get(StringView fieldName) const;

        void Set(Field::Reader field, const DynamicValue::Reader& value);
        void Set(StringView fieldName, const DynamicValue::Reader& value);

        DynamicValue::Builder Init(Field::Reader field);
        DynamicValue::Builder Init(StringView fieldName);

        DynamicValue::Builder Init(Field::Reader field, uint32_t size);
        DynamicValue::Builder Init(StringView fieldName, uint32_t size);

        void Clear(Field::Reader field);
        void Clear(StringView fieldName);

        bool Has(Field::Reader field) const;
        bool Has(StringView fieldName) const;

        Field::Reader ActiveUnionField() const;

        template <typename T> requires(T::Kind == DeclKind::Struct)
        typename T::Builder As() const
        {
            const bool valid = HE_VERIFY(&T::DeclInfo == m_info,
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
        bool IsActiveInUnion(Field::Reader field) const;

    private:
        const DeclInfo* m_info{ nullptr };
        StructBuilder m_builder{};
    };

    // --------------------------------------------------------------------------------------------
    class DynamicArray::Reader
    {
    public:
        using IteratorType = ListIterator<DynamicArray::Reader>;
        using ElementType = DynamicValue::Reader;

    public:
        Reader() noexcept
            : m_scope(nullptr)
            , m_type()
            , m_array{ nullptr, 0 }
        {}

        Reader(const DeclInfo& scope, Type::Reader type, ListReader reader) noexcept
            : m_scope(&scope)
            , m_type(type)
            , m_list(reader)
        {
            HE_ASSERT(GetSchema(scope).GetData().IsStruct());
            HE_ASSERT(type.GetData().IsArray());
            HE_ASSERT(IsPointer(type.GetData().GetArray().GetElementType()));
        }

        Reader(const DeclInfo& scope, Type::Reader type, const Word* data, uint32_t offset) noexcept
            : m_scope(&scope)
            , m_type(type)
            , m_array{ data, offset }
        {
            HE_ASSERT(GetSchema(scope).GetData().IsStruct());
            HE_ASSERT(type.GetData().IsArray());
            HE_ASSERT(!IsPointer(type.GetData().GetArray().GetElementType()));
        }

    public:
        const DeclInfo& Scope() const { HE_ASSERT(m_scope); return *m_scope; }
        Type::Reader GetType() const { return m_type; }
        Type::Data::Array::Reader ArrayType() const { return m_type.GetData().GetArray(); }

        uint16_t Size() const { return ArrayType().GetSize(); }

        DynamicValue::Reader Get(uint16_t index) const;
        bool Has(uint16_t index) const;

        DynamicValue::Reader operator[](uint16_t index) const;

        template <typename T>
        typename List<T>::Reader AsListOf() const
        {
            using ListReaderType = typename schema::List<T>::Reader;
            const bool valid = HE_VERIFY(IsPointer(ArrayType().GetElementType()), HE_MSG("DynamicArray requested as a List with a non-pointer element type."))
                && HE_VERIFY(IsValidElementType<T>(ArrayType().GetElementType()), HE_MSG("DynamicArray requested as a List with an element type that doesn't match the schema."));
            return valid ? ListReaderType(m_list) : ListReaderType{};
        }

        template <DataType T>
        Span<const T> AsSpanOf() const
        {
            const bool valid = HE_VERIFY(!IsPointer(ArrayType().GetElementType()), HE_MSG("DynamicArray requested as a Span with a pointer element type."))
                && HE_VERIFY(IsValidElementType<T>(ArrayType().GetElementType()), HE_MSG("DynamicArray requested as a Span with an element type that doesn't match the schema."));
            return valid ? Span<const T>(reinterpret_cast<const T*>(m_array.data), Size()) : Span<const T>{};
        }

        IteratorType begin() const { return IteratorType(this, 0); }
        IteratorType end() const { return IteratorType(this, Size()); }

    private:
        Declaration::Reader ScopeSchema() const { HE_ASSERT(m_scope); return GetSchema(*m_scope); }
        Declaration::Data::Struct::Reader ScopeStruct() const { return ScopeSchema().GetData().GetStruct(); }

    private:
        const DeclInfo* m_scope{ nullptr };
        Type::Reader m_type{};

        union
        {
            ListReader m_list;
            struct
            {
                const Word* data;
                uint32_t dataOffset;
            } m_array;
        };
    };

    class DynamicArray::Builder
    {
    public:
        using IteratorType = ListIterator<DynamicArray::Builder>;
        using ElementType = DynamicValue::Builder;

    public:
        Builder() noexcept
            : m_scope(nullptr)
            , m_type()
            , m_array{ nullptr, 0 }
        {}

        Builder(const DeclInfo& scope, Type::Reader type, ListBuilder builder) noexcept
            : m_scope(&scope)
            , m_type(type)
            , m_list(builder)
        {
            HE_ASSERT(GetSchema(scope).GetData().IsStruct());
            HE_ASSERT(type.GetData().IsArray());
            HE_ASSERT(IsPointer(type.GetData().GetArray().GetElementType()));
        }

        Builder(const DeclInfo& scope, Type::Reader type, Word* data, uint32_t offset) noexcept
            : m_scope(&scope)
            , m_type(type)
            , m_array{ data, offset }
        {
            HE_ASSERT(GetSchema(scope).GetData().IsStruct());
            HE_ASSERT(type.GetData().IsArray());
            HE_ASSERT(!IsPointer(type.GetData().GetArray().GetElementType()));
        }

    public:
        const DeclInfo& Scope() const { HE_ASSERT(m_scope); return *m_scope; }
        Type::Reader GetType() const { return m_type; }
        Type::Data::Array::Reader ArrayType() const { return m_type.GetData().GetArray(); }

        uint16_t Size() const { return ArrayType().GetSize(); }

        DynamicValue::Builder Get(uint16_t index) const;
        void Set(uint16_t index, const DynamicValue::Reader& value);
        DynamicValue::Builder Init(uint16_t index);
        DynamicValue::Builder Init(uint16_t index, uint32_t size);
        void Clear(uint16_t index);
        bool Has(uint16_t index) const;

        DynamicValue::Builder operator[](uint16_t index) const;

        template <typename T>
        typename List<T>::Builder AsListOf() const
        {
            using ListBuilderType = typename schema::List<T>::Builder;
            const bool valid = HE_VERIFY(IsPointer(ArrayType().GetElementType()), HE_MSG("DynamicArray requested as a List with a non-pointer element type."))
                && HE_VERIFY(IsValidElementType<T>(ArrayType().GetElementType()), HE_MSG("DynamicArray requested as a List with an element type that doesn't match the schema."));
            return valid ? ListBuilderType(m_list) : ListBuilderType{};
        }

        template <DataType T>
        Span<T> AsSpanOf() const
        {
            const bool valid = HE_VERIFY(!IsPointer(ArrayType().GetElementType()), HE_MSG("DynamicArray requested as a Span with a pointer element type."))
                && HE_VERIFY(IsValidElementType<T>(ArrayType().GetElementType()), HE_MSG("DynamicArray requested as a Span with an element type that doesn't match the schema."));
            return valid ? Span<T>(static_cast<T*>(m_array), Size()) : Span<T>{};
        }

        Reader AsReader() const { return IsPointer(m_type) ? Reader(*m_scope, m_type, m_list) : Reader(*m_scope, m_type, m_array.data, m_array.dataOffset); }

        IteratorType begin() const { return IteratorType(this, 0); }
        IteratorType end() const { return IteratorType(this, Size()); }

    private:
        Declaration::Reader ScopeSchema() const { HE_ASSERT(m_scope); return GetSchema(*m_scope); }
        Declaration::Data::Struct::Reader ScopeStruct() const { return ScopeSchema().GetData().GetStruct(); }

    private:
        const DeclInfo* m_scope{ nullptr };
        Type::Reader m_type{};

        union
        {
            ListBuilder m_list;
            struct
            {
                Word* data;
                uint32_t dataOffset;
            } m_array;
        };
    };

    // --------------------------------------------------------------------------------------------
    class DynamicList::Reader
    {
    public:
        using IteratorType = ListIterator<DynamicList::Reader>;
        using ElementType = DynamicValue::Reader;

    public:
        Reader() = default;

        Reader(const DeclInfo& scope, Type::Reader type, ListReader reader) noexcept
            : m_scope(&scope)
            , m_type(type)
            , m_reader(reader)
        {
            HE_ASSERT(GetSchema(scope).GetData().IsStruct());
            HE_ASSERT(type.GetData().IsList());
            HE_ASSERT(type.GetData().GetList().GetElementType().GetData().IsArray() == false); // Lists of Arrays are not supported.
        }

    public:
        const DeclInfo& Scope() const { HE_ASSERT(m_scope); return *m_scope; }
        Type::Reader GetType() const { return m_type; }
        Type::Data::List::Reader ListType() const { return m_type.GetData().GetList(); }

        uint32_t Size() const { return m_reader.Size(); }

        DynamicValue::Reader Get(uint32_t index) const;
        bool Has(uint32_t index) const;

        DynamicValue::Reader operator[](uint32_t index) const;

        template <typename T>
        typename List<T>::Reader AsListOf() const
        {
            using ListReaderType = typename schema::List<T>::Reader;
            const bool valid = HE_VERIFY(IsValidElementType<T>(ListType().GetElementType()),
                HE_MSG("DynamicList requested as an element type that doesn't match the schema."));
            return valid ? ListReaderType(m_reader) : ListReaderType{};
        }

        ListReader List() const { return m_reader; }

        IteratorType begin() const { return IteratorType(this, 0); }
        IteratorType end() const { return IteratorType(this, m_reader.Size()); }

    private:
        const DeclInfo* m_scope{ nullptr };
        Type::Reader m_type{};
        ListReader m_reader{};
    };

    class DynamicList::Builder
    {
    public:
        using IteratorType = ListIterator<DynamicList::Builder>;
        using ElementType = DynamicValue::Builder;

    public:
        Builder() = default;

        Builder(const DeclInfo& scope, Type::Reader type, ListBuilder builder) noexcept
            : m_scope(&scope)
            , m_type(type)
            , m_builder(builder)
        {
            HE_ASSERT(GetSchema(scope).GetData().IsStruct());
            HE_ASSERT(type.GetData().IsList());
            HE_ASSERT(type.GetData().GetList().GetElementType().GetData().IsArray() == false); // Lists of Arrays are not supported.
        }

    public:
        const DeclInfo& Scope() const { HE_ASSERT(m_scope); return *m_scope; }
        Type::Reader GetType() const { return m_type; }
        Type::Data::List::Reader ListType() const { return m_type.GetData().GetList(); }

        uint32_t Size() const { return m_builder.Size(); }

        DynamicValue::Builder Get(uint32_t index) const;
        void Set(uint32_t index, const DynamicValue::Reader& value);
        DynamicValue::Builder Init(uint32_t index, uint32_t size);
        void Clear(uint32_t index);
        bool Has(uint32_t index) const;

        /// Creates a copy of this list at the new size.
        ///
        /// \param[in] count The size of the new list.
        /// \return A copy of this list at the new size.
        DynamicList::Builder Resize(uint32_t count);

        /// Creates a copy of this list with `value` inserted at `index`.
        ///
        /// \param[in] index The index at which to insert the element.
        /// \param[in] value The value to insert.
        /// \return A copy of this list with `value` inserted at `index`.
        DynamicList::Builder Insert(uint32_t index, const DynamicValue::Reader& value);

        /// Creates a copy of this list with `count` elements removed starting at `index`.
        ///
        /// \param[in] index The index at which to start removing elements.
        /// \param[in] count The number of elements to remove.
        /// \return A copy of this list with `count` elements removed starting at `index`.
        DynamicList::Builder Erase(uint32_t index, uint32_t count);

        DynamicValue::Builder operator[](uint32_t index) const;

        template <typename T>
        typename List<T>::Builder AsListOf() const
        {
            using ListBuilderType = typename schema::List<T>::Builder;
            const bool valid = HE_VERIFY(IsValidElementType<T>(ListType().GetElementType()),
                HE_MSG("DynamicList requested as an element type that doesn't match the schema."));
            return valid ? ListBuilderType(m_builder) : ListBuilderType{};
        }

        Reader AsReader() const { return Reader(*m_scope, m_type, m_builder.AsReader()); }

        ListBuilder List() const { return m_builder; }

        IteratorType begin() const { return IteratorType(this, 0); }
        IteratorType end() const { return IteratorType(this, Size()); }

    private:
        ListBuilder MakeResizedList(uint32_t count);

    private:
        const DeclInfo* m_scope{ nullptr };
        Type::Reader m_type{};
        ListBuilder m_builder{};
    };

    // --------------------------------------------------------------------------------------------
    class DynamicValue::Reader
    {
    public:
        // These constructors are intentionally not marked as `explicit` to allow implicit conversions.

        Reader(decltype(nullptr) = nullptr) noexcept : m_kind(Kind::Unknown), m_void() {}
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
        Reader(const DynamicArray::Reader& value) noexcept : m_kind(Kind::Array), m_array(value) {}
        Reader(const DynamicList::Reader& value) noexcept : m_kind(Kind::List), m_list(value) {}
        Reader(DynamicEnum value) noexcept : m_kind(Kind::Enum), m_enum(value) {}
        Reader(const DynamicStruct::Reader& value) noexcept : m_kind(Kind::Struct), m_struct(value) {}
        Reader(const PointerReader& value) noexcept : m_kind(Kind::AnyPointer), m_anyPointer(value) {}

    public:
        Kind GetKind() const { return m_kind; }

        template <typename T>
        typename LayoutTraits<T>::Reader As() const;

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
            DynamicArray::Reader m_array;
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

        Builder(decltype(nullptr) = nullptr) noexcept : m_kind(Kind::Unknown), m_void() {}
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
        Builder(const DynamicArray::Builder& value) noexcept : m_kind(Kind::Array), m_array(value) {}
        Builder(const DynamicList::Builder& value) noexcept : m_kind(Kind::List), m_list(value) {}
        Builder(DynamicEnum value) noexcept : m_kind(Kind::Enum), m_enum(value) {}
        Builder(const DynamicStruct::Builder& value) noexcept : m_kind(Kind::Struct), m_struct(value) {}
        Builder(const PointerBuilder& value) noexcept : m_kind(Kind::AnyPointer), m_anyPointer(value) {}

    public:
        Kind GetKind() const { return m_kind; }

        template <typename T>
        typename LayoutTraits<T>::Builder As() const;

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
            Blob::Builder m_blob;
            String::Builder m_string;
            DynamicArray::Builder m_array;
            DynamicList::Builder m_list;
            DynamicEnum m_enum;
            DynamicStruct::Builder m_struct;
            PointerBuilder m_anyPointer;
        };
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class DynamicStructVisitorT
    {
    public:
        static constexpr bool IsReader = IsSame<T, DynamicStruct::Reader>;
        static constexpr bool IsBuilder = IsSame<T, DynamicStruct::Builder>;

        static_assert(IsReader || IsBuilder, "DynamicStructVisitor is intended for DynamicStruct::Reader and DynamicStruct::Builder types.");

        template <typename U>
        using Access = Conditional<IsReader, const typename U::Reader, typename U::Builder>;

    public:
        virtual ~DynamicStructVisitorT() = default;

        void Visit(Access<DynamicStruct>& data) { VisitStruct(data); }

    public:
        // This section has functions that child classes are likely to override.

        virtual void VisitNormalField(Access<DynamicStruct>& data, Field::Reader field)
        {
            VisitValue(data.Get(field));
        }

        virtual void VisitGroupField(Access<DynamicStruct>& data, Field::Reader field)
        {
            T st = data.Get(field).template As<DynamicStruct>();
            VisitStruct(st);
        }

        virtual void VisitUnionField(Access<DynamicStruct>& data, Field::Reader field)
        {
            T st = data.Get(field).template As<DynamicStruct>();
            VisitStruct(st);
        }

        virtual void VisitValue(const Access<DynamicValue>& value) { HE_UNUSED(value); }

    protected:
        // This section has functions that child classes are less likely to override.
        // However, in some advanced cases it may be useful to do so.

        virtual void VisitStruct(Access<DynamicStruct>& data);
        virtual void VisitField(Access<DynamicStruct>& data, Field::Reader field);

        virtual bool ShouldVisitNormalField(const Access<DynamicStruct>& data, Field::Reader field) { return data.Has(field); }
        virtual bool ShouldVisitGroupField(const Access<DynamicStruct>& data, Field::Reader field) { return data.Has(field); }
        virtual bool ShouldVisitUnionField(const Access<DynamicStruct>& data, Field::Reader field) { return data.Has(field); }
    };

    struct DynamicStructVisitor
    {
        using Reader = DynamicStructVisitorT<DynamicStruct::Reader>;
        using Builder = DynamicStructVisitorT<DynamicStruct::Builder>;
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    void DynamicStructVisitorT<T>::VisitStruct(Access<DynamicStruct>& data)
    {
        const Declaration::Data::Struct::Reader structDecl = data.StructSchema();

        if (structDecl.GetIsUnion())
        {
            const Field::Reader activeField = data.ActiveUnionField();
            VisitField(data, activeField);
        }
        else
        {
            for (Field::Reader field : structDecl.GetFields())
            {
                VisitField(data, field);
            }
        }
    }

    template <typename T>
    void DynamicStructVisitorT<T>::VisitField(Access<DynamicStruct>& data, Field::Reader field)
    {
        switch (field.GetMeta().GetUnionTag())
        {
            case Field::Meta::UnionTag::Normal:
                if (ShouldVisitNormalField(data, field))
                    VisitNormalField(data, field);
                break;

            case Field::Meta::UnionTag::Group:
                if (ShouldVisitGroupField(data, field))
                    VisitGroupField(data, field);
                break;

            case Field::Meta::UnionTag::Union:
                if (ShouldVisitUnionField(data, field))
                    VisitUnionField(data, field);
                break;
        }
    }

    // --------------------------------------------------------------------------------------------
    template <typename T>
    inline bool IsValidElementType(Type::Reader elementType)
    {
        const Type::Data::Reader elementTypeData = elementType.GetData();
        switch (elementTypeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: return IsSame<T, Void>;
            case Type::Data::UnionTag::Bool: return IsSame<T, bool>;
            case Type::Data::UnionTag::Int8: return IsSame<T, int8_t>;
            case Type::Data::UnionTag::Int16: return IsSame<T, int16_t>;
            case Type::Data::UnionTag::Int32: return IsSame<T, int32_t>;
            case Type::Data::UnionTag::Int64: return IsSame<T, int64_t>;
            case Type::Data::UnionTag::Uint8: return IsSame<T, uint8_t>;
            case Type::Data::UnionTag::Uint16: return IsSame<T, uint16_t>;
            case Type::Data::UnionTag::Uint32: return IsSame<T, uint32_t>;
            case Type::Data::UnionTag::Uint64: return IsSame<T, uint64_t>;
            case Type::Data::UnionTag::Float32: return IsSame<T, float>;
            case Type::Data::UnionTag::Float64: return IsSame<T, double>;
            case Type::Data::UnionTag::Blob: return IsSame<T, schema::Blob>;
            case Type::Data::UnionTag::String: return IsSame<T, schema::String>;
            case Type::Data::UnionTag::AnyPointer: return IsSame<T, schema::AnyPointer>;
            case Type::Data::UnionTag::AnyStruct: return IsSame<T, schema::AnyStruct>;
            case Type::Data::UnionTag::AnyList: return IsSame<T, schema::AnyList>;
            case Type::Data::UnionTag::Enum: return IsEnum<T>;
            case Type::Data::UnionTag::Interface: return false;
            case Type::Data::UnionTag::Parameter: return false;

            case Type::Data::UnionTag::Struct:
            {
                if constexpr (LayoutTraits<T>::IsStruct)
                    return T::Kind == DeclKind::Struct;
                else
                    return false;
            }

            case Type::Data::UnionTag::Array:
            {
                if constexpr (LayoutTraits<T>::IsList)
                {
                    return IsValidElementType<T::ElementType>(elementTypeData.GetArray().GetElementType());
                }
                else
                {
                    return false;
                }
            }

            case Type::Data::UnionTag::List:
            {
                if constexpr (LayoutTraits<T>::IsList)
                {
                    return IsValidElementType<T::ElementType>(elementTypeData.GetList().GetElementType());
                }
                else
                {
                    return false;
                }
            }
        }
        return false;
    }
}
