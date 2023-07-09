// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/array.h"
#include "he/core/concepts.h"
#include "he/core/index_iterator.h"
#include "he/core/string.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/utils.h"
#include "he/core/vector.h"
#include "he/schema/schema.h"
#include "he/schema/types.h"

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    class DynamicValue2
    {
    public:
        virtual ~DynamicValue2() = default;

    public:
        /// Clears the value. Basic types this will reset to zero, pointers will be set to nullptr,
        /// lists will be cleared of elements, and arrays will clear each element.
        virtual void Clear(void* value) = 0;

        /// Allocate a new instance of this value.
        /// \note Only valid for pointer values.
        virtual void Alloc(void* value) = 0;

        /// Get a dynamic value for an element of the value.
        /// \note Only valid for list and array values.
        virtual UniquePtr<DynamicValue2> GetElement(void* value, uint32_t index) const = 0;

        /// Add a new element to the end of the value and return a dynamic value for it.
        /// \note Only valid for list values.
        virtual UniquePtr<DynamicValue2> AddElement(void* value) = 0;

        /// Remove an element from the value.
        /// \note Only valid for list values.
        virtual void RemoveElement(void* value, uint32_t index) = 0;

    public:
        /// Copy assign the value.
        template <typename T>
        void CopyAssign(void* value, const T& src) { CopyAssign(value, &src, TypeInfo::Get<T>()); }

        /// Move assign the value.
        template <typename T>
        void MoveAssign(void* value, T&& src) { MoveAssign(value, &src, TypeInfo::Get<T>()); }

        /// Copy assign an element of this value.
        /// \note Only valid for list and array values.
        template <typename T>
        void CopyAssignElement(void* value, uint32_t index, const T& src) { CopyAssignElement(value, &src, TypeInfo::Get<T>()); }

        /// Move assign an element of this value.
        /// \note Only valid for list and array values.
        template <typename T>
        void MoveAssignElement(void* value, uint32_t index, T&& src) { MoveAssignElement(vallue, &src, TypeInfo::Get<T>()); }

    protected:
        virtual void CopyAssign(void* value, const void* src, const TypeInfo& srcType) = 0;
        virtual void MoveAssign(void* value, void* src, const TypeInfo& srcType) = 0;
        virtual void CopyAssignElement(void* value, uint32_t index, const void* src, const TypeInfo& srcType) = 0;
        virtual void MoveAssignElement(void* value, uint32_t index, void* src, const TypeInfo& srcType) = 0;
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class DynamicValueImpl : public DynamicValue2
    {
    public:
        static_assert(T::Kind == DeclKind::Struct, "This specialization is intended only for schema generated structs.");

        using ValueType = T;

        static constexpr TypeInfo ValueTypeInfo = TypeInfo::Get<T>();

    protected:
        void Clear(void* value) override
        {
            *static_cast<ValueType*>(value) = ValueType{};
        }

        void Alloc(void* value) override
        {
            HE_UNUSED(value);
        }

        UniquePtr<DynamicValue2> GetElement(void* value, uint32_t index) const override
        {
            HE_UNUSED(value, index);
            return nullptr;
        }

        UniquePtr<DynamicValue2> AddElement(void* value) override
        {
            HE_UNUSED(value);
            return nullptr;
        }

        void RemoveElement(void* value, uint32_t index) override
        {
            HE_UNUSED(value, index);
        }

        void CopyAssign(void* value, const void* src, const TypeInfo& srcType) override
        {
            if (HE_VERIFY(srcType == ValueTypeInfo))
                *static_cast<ValueType*>(value) = *static_cast<const ValueType*>(src);
        }

        void MoveAssign(void* value, void* src, const TypeInfo& srcType) override
        {
            if (HE_VERIFY(srcType == ValueTypeInfo))
                *static_cast<ValueType*>(value) = Move(*static_cast<ValueType*>(src));
        }

        void CopyAssignElement(void* value, uint32_t index, const void* src, const TypeInfo& srcType) override
        {
            HE_UNUSED(value, index, src, srcType);
        }

        void MoveAssignElement(void* value, uint32_t index, void* src, const TypeInfo& srcType) override
        {
            HE_UNUSED(value, index, src, srcType);
        }
    };

    // --------------------------------------------------------------------------------------------
    template <DataType T>
    class DynamicValueImpl<T> : public DynamicValue2
    {
    public:
        using ValueType = T;

        static constexpr TypeInfo ValueTypeInfo = TypeInfo::Get<T>();

    protected:
        void Clear(void* value) override
        {
            *static_cast<ValueType*>(value) = ValueType{};
        }

        void Alloc(void* value) override
        {
            HE_UNUSED(value);
        }

        UniquePtr<DynamicValue2> GetElement(void* value, uint32_t index) const override
        {
            HE_UNUSED(value, index);
            return nullptr;
        }

        UniquePtr<DynamicValue2> AddElement(void* value) override
        {
            HE_UNUSED(value);
            return nullptr;
        }

        void RemoveElement(void* value, uint32_t index) override
        {
            HE_UNUSED(value, index);
        }

        void CopyAssign(void* value, const void* src, const TypeInfo& srcType) override
        {
            if (HE_VERIFY(srcType == ValueTypeInfo))
                *static_cast<ValueType*>(value) = *static_cast<const ValueType*>(src);
        }

        void MoveAssign(void* value, void* src, const TypeInfo& srcType) override
        {
            if (HE_VERIFY(srcType == ValueTypeInfo))
                *static_cast<ValueType*>(value) = Move(*static_cast<ValueType*>(src));
        }

        void CopyAssignElement(void* value, uint32_t index, const void* src, const TypeInfo& srcType) override
        {
            HE_UNUSED(value, index, src, srcType);
        }

        void MoveAssignElement(void* value, uint32_t index, void* src, const TypeInfo& srcType) override
        {
            HE_UNUSED(value, index, src, srcType);
        }
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class DynamicValueImpl<UniquePtr<T>> : public DynamicValue2
    {
    public:
        using ValueType = UniquePtr<T>;
        using ElementType = T;

        static constexpr TypeInfo ValueTypeInfo = TypeInfo::Get<ValueType>();
        static constexpr TypeInfo ElementTypeInfo = TypeInfo::Get<ElementType>();

    protected:
        void Clear(void* value) override
        {
            static_cast<ValueType*>(value)->Reset();
        }

        void Alloc(void* value) override
        {
            *static_cast<ValueType*>(value) = MakeUnique<T>();
        }

        UniquePtr<DynamicValue2> GetElement(void* value, uint32_t index) const override
        {
            HE_UNUSED(value, index);
            return nullptr;
        }

        UniquePtr<DynamicValue2> AddElement(void* value) override
        {
            HE_UNUSED(value);
            return nullptr;
        }

        void RemoveElement(void* value, uint32_t index) override
        {
            HE_UNUSED(value, index);
        }

        void CopyAssign(void* value, const void* src, const TypeInfo& srcType) override
        {
            HE_UNUSED(value, src, srcType);
            return;
        }

        void MoveAssign(void* value, void* src, const TypeInfo& srcType) override
        {
            if (HE_VERIFY(srcType == ValueTypeInfo))
                *static_cast<ValueType*>(value) = Move(*static_cast<ValueType*>(src));
        }

        void CopyAssignElement(void* value, uint32_t index, const void* src, const TypeInfo& srcType) override
        {
            HE_UNUSED(value, index, src, srcType);
        }

        void MoveAssignElement(void* value, uint32_t index, void* src, const TypeInfo& srcType) override
        {
            HE_UNUSED(value, index, src, srcType);
        }
    };

    // --------------------------------------------------------------------------------------------
    template <typename T, uint32_t N>
    class DynamicValueImpl<Array<T, N>> : public DynamicValue2
    {
    public:
        using ValueType = Array<T, N>;
        using ElementType = T;

        static constexpr TypeInfo ValueTypeInfo = TypeInfo::Get<ValueType>();
        static constexpr TypeInfo ElementTypeInfo = TypeInfo::Get<ElementType>();

    protected:
        void Clear(void* value) override
        {
            for (auto&& v : *static_cast<ValueType*>(value))
            {
                DynamicValueImpl<ElementType>(&v).Clear();
            }
        }

        void Alloc(void* value) override
        {
            HE_UNUSED(value);
        }

        UniquePtr<DynamicValue2> GetElement(void* value, uint32_t index) const override
        {
            ValueType& arr = *static_cast<ValueType*>(value);
            ElementType* element = HE_VERIFY(index < N) ? &arr[index] : nullptr;
            return MakeUnique<DynamicValueImpl<ElementType>>(element);
        }

        UniquePtr<DynamicValue2> AddElement(void* value) override
        {
            HE_UNUSED(value);
            return nullptr;
        }

        void RemoveElement(void* value, uint32_t index) override
        {
            if (HE_VERIFY(index < N))
                (*static_cast<ValueType*>(value))[index] = {};
        }

        void CopyAssign(void* value, const void* src, const TypeInfo& srcType) override
        {
            if (HE_VERIFY(srcType == ValueTypeInfo))
                *static_cast<ValueType*>(value) = *static_cast<const ValueType*>(src);
        }

        void MoveAssign(void* value, void* src, const TypeInfo& srcType) override
        {
            if (HE_VERIFY(srcType == ValueTypeInfo))
                *static_cast<ValueType*>(value) = Move(*static_cast<ValueType*>(src));
        }

        void CopyAssignElement(void* value, uint32_t index, const void* src, const TypeInfo& srcType) override
        {
            if (HE_VERIFY(index < N) && HE_VERIFY(srcType == ElementTypeInfo))
                (*static_cast<ValueType*>(value))[index] = *static_cast<const ElementType*>(src);
        }

        void MoveAssignElement(void* value, uint32_t index, void* src, const TypeInfo& srcType) override
        {
            if (HE_VERIFY(index < N) && HE_VERIFY(srcType == ElementTypeInfo))
                (*static_cast<ValueType*>(value))[index] = Move(*static_cast<ElementType*>(src));
        }
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class DynamicValueImpl<Vector<T>> : public DynamicValue2
    {
    public:
        using ValueType = Vector<T>;
        using ElementType = T;

        static constexpr TypeInfo ValueTypeInfo = TypeInfo::Get<ValueType>();
        static constexpr TypeInfo ElementTypeInfo = TypeInfo::Get<ElementType>();

    protected:
        void Clear(void* value) override
        {
            static_cast<ValueType*>(value)->Clear();
        }

        void Alloc(void* value) override
        {
            HE_UNUSED(value);
        }

        UniquePtr<DynamicValue2> GetElement(void* value, uint32_t index) const override
        {
            ValueType& vec = *static_cast<ValueType*>(value);
            ElementType* element = HE_VERIFY(index < vec.Size()) ? &vec[index] : nullptr;
            return MakeUnique<DynamicValueImpl<ElementType>>(element);
        }

        UniquePtr<DynamicValue2> AddElement(void* value) override
        {
            const ValueType& vec = *static_cast<ValueType*>(value);
            ElementType* element = &vec.EmplaceBack();
            return MakeUnique<DynamicValueImpl<ElementType>>(element);
        }

        void RemoveElement(void* value, uint32_t index) override
        {
            const ValueType& vec = *static_cast<ValueType*>(value);
            if (HE_VERIFY(index < vec.Size()))
                vec.Erase(index);
        }

        void CopyAssign(void* value, const void* src, const TypeInfo& srcType) override
        {
            const ValueType& vec = *static_cast<ValueType*>(value);
            if (HE_VERIFY(srcType == ValueTypeInfo))
                vec = *static_cast<const ValueType*>(src);
        }

        void MoveAssign(void* value, void* src, const TypeInfo& srcType) override
        {
            const ValueType& vec = *static_cast<ValueType*>(value);
            if (HE_VERIFY(srcType == ValueTypeInfo))
                vec = Move(*static_cast<ValueType*>(src));
        }

        void CopyAssignElement(void* value, uint32_t index, const void* src, const TypeInfo& srcType) override
        {
            if constexpr (IsCopyAssignable<ElementType>)
            {
                const ValueType& vec = *static_cast<ValueType*>(value);
                if (HE_VERIFY(index < vec.Size()) && HE_VERIFY(srcType == ElementTypeInfo))
                    vec[index] = *static_cast<const ElementType*>(src);
            }
        }

        void MoveAssignElement(void* value, uint32_t index, void* src, const TypeInfo& srcType) override
        {
            if constexpr (IsMoveAssignable<ElementType>)
            {
                const ValueType& vec = *static_cast<ValueType*>(value);
                if (HE_VERIFY(index < vec.Size()) && HE_VERIFY(srcType == ElementTypeInfo))
                    vec[index] = Move(*static_cast<ElementType*>(src));
            }
        }
    };

    // --------------------------------------------------------------------------------------------
    template <>
    class DynamicValueImpl<he::String> : public DynamicValue2
    {
    public:
        using ValueType = he::String;
        using ElementType = char;

        static constexpr TypeInfo ValueTypeInfo = TypeInfo::Get<ValueType>();
        static constexpr TypeInfo ElementTypeInfo = TypeInfo::Get<ElementType>();

    protected:
        void Clear(void* value) override
        {
            static_cast<ValueType*>(value)->Clear();
        }

        void Alloc(void* value) override
        {
            HE_UNUSED(value);
        }

        UniquePtr<DynamicValue2> GetElement(void* value, uint32_t index) const override
        {
            ValueType& str = *static_cast<ValueType*>(value);
            ElementType* element = HE_VERIFY(index < str.Size()) ? &str[index] : nullptr;
            return MakeUnique<DynamicValueImpl<ElementType>>(element);
        }

        UniquePtr<DynamicValue2> AddElement(void* value) override
        {
            ValueType& str = *static_cast<ValueType*>(value);
            str.PushBack('\0');
            return MakeUnique<DynamicValueImpl<ElementType>>(&str.Back());
        }

        void RemoveElement(void* value, uint32_t index) override
        {
            ValueType& str = *static_cast<ValueType*>(value);
            if (HE_VERIFY(index < str.Size()))
                str.Erase(index, 1);
        }

        void CopyAssign(void* value, const void* src, const TypeInfo& srcType) override
        {
            ValueType& str = *static_cast<ValueType*>(value);
            if (srcType == ValueTypeInfo)
                str = *static_cast<const ValueType*>(src);
            else if (srcType == TypeInfo::Get<char>())
                str.Assign(static_cast<const char*>(src), 1);
            else if (srcType == TypeInfo::Get<char*>())
                str.Assign(static_cast<const char*>(src));
            else
            {
                HE_VERIFY(false, HE_MSG("Unknown param type for string assignment."));
            }
        }

        void MoveAssign(void* value, void* src, const TypeInfo& srcType) override
        {
            ValueType& str = *static_cast<ValueType*>(value);
            if (srcType == ValueTypeInfo)
                str = Move(*static_cast<ValueType*>(src));
            else if (srcType == TypeInfo::Get<char>())
                str.Assign(static_cast<const char*>(src), 1);
            else if (srcType == TypeInfo::Get<char*>())
                str.Assign(static_cast<const char*>(src));
            else
            {
                HE_VERIFY(false, HE_MSG("Unknown param type for string assignment."));
            }
        }

        void CopyAssignElement(void* value, uint32_t index, const void* src, const TypeInfo& srcType) override
        {
            ValueType& str = *static_cast<ValueType*>(value);
            if (HE_VERIFY(index < str.Size()) && HE_VERIFY(srcType == ElementTypeInfo))
                str[index] = *static_cast<const ElementType*>(value);
        }

        void MoveAssignElement(void* value, uint32_t index, void* src, const TypeInfo& srcType) override
        {
            ValueType& str = *static_cast<ValueType*>(value);
            if (HE_VERIFY(index < str.Size()) && HE_VERIFY(srcType == ElementTypeInfo))
                str[index] = *static_cast<const ElementType*>(value);
        }
    };

    // --------------------------------------------------------------------------------------------
    //template <typename T>
    //DynamicValueImpl<T> MakeDynamicValue(T* value)
    //{
    //    return DynamicValueImpl<T>(value);
    //}

    //// --------------------------------------------------------------------------------------------
    //class DynamicEnum3
    //{
    //public:
    //    DynamicEnum3() = default;
    //    DynamicEnum3(const DeclInfo& info, uint16_t* value) noexcept
    //        : m_info(&info)
    //        , m_value(value)
    //    {}

    //public:
    //    const DeclInfo& Decl() const { HE_ASSERT(m_info); return *m_info; }
    //    Declaration::Reader Schema() const { return GetSchema(Decl()); }
    //    Declaration::Data::Enum::Reader EnumSchema() const { return Schema().GetData().GetEnum(); }

    //    schema::Enumerator::Reader Enumerator() const
    //    {
    //        HE_ASSERT(m_value);
    //        auto enumerators = EnumSchema().GetEnumerators();
    //        return *m_value < enumerators.Size() ? enumerators[*m_value] : schema::Enumerator::Reader{};
    //    }

    //    uint16_t Get() const { HE_ASSERT(m_value); return *m_value; }
    //    void Set(uint16_t value) { HE_ASSERT(m_value); *m_value = value; }

    //    template <he::Enum T>
    //    T As() const
    //    {
    //        const bool valid = HE_VERIFY(EnumDeclInfo<T>::Id = Schema().GetId(),
    //            HE_MSG("DynamicEnum requested as a type that doesn't match the schema."),
    //            HE_KV(requested_type_name, GetSchema(EnumDeclInfo<T>::DeclInfo).GetName()),
    //            HE_KV(requested_type_id, EnumDeclInfo<T>::Id),
    //            HE_KV(actual_type_name, Schema().GetName()),
    //            HE_KV(actual_type_id, Schema().GetId()));
    //        return valid ? static_cast<T>(*m_value) : T{};
    //    }

    //private:
    //    const DeclInfo* m_info{ nullptr };
    //    uint16_t* m_value{ nullptr };
    //};

    //// --------------------------------------------------------------------------------------------
    //class DynamicStruct3
    //{
    //public:
    //    DynamicStruct3() = default;

    //    DynamicStruct3(const DeclInfo& info, void* value) noexcept
    //        : m_info(&info)
    //        , m_value(value)
    //    {}

    //    template <typename T> requires(T::Kind == DeclKind::Struct)
    //    DynamicStruct3(T* value) noexcept : DynamicStruct3(T::DeclInfo, value) {}

    //public:
    //    const DeclInfo& Decl() const { HE_ASSERT(m_info); return *m_info; }
    //    Declaration::Reader Schema() const { return GetSchema(Decl()); }
    //    Declaration::Data::Struct::Reader StructSchema() const { return Schema().GetData().GetStruct(); }

    //    DynamicValue3 Get(Field::Reader field) const;
    //    DynamicValue3 Get(uint16_t fieldIndex) const;
    //    DynamicValue3 Get(StringView fieldName) const;

    //    void Set(Field::Reader field, const DynamicValue3& value);
    //    void Set(uint16_t fieldIndex, const DynamicValue3& value);
    //    void Set(StringView fieldName, const DynamicValue3& value);

    //    DynamicValue3 Init(Field::Reader field);
    //    DynamicValue3 Init(uint16_t fieldIndex);
    //    DynamicValue3 Init(StringView fieldName);

    //    DynamicValue3 Init(Field::Reader field, uint32_t size);
    //    DynamicValue3 Init(uint16_t fieldIndex, uint32_t size);
    //    DynamicValue3 Init(StringView fieldName, uint32_t size);

    //    void Clear(Field::Reader field);
    //    void Clear(uint16_t fieldIndex);
    //    void Clear(StringView fieldName);

    //    bool Has(Field::Reader field) const;
    //    bool Has(uint16_t fieldIndex) const;
    //    bool Has(StringView fieldName) const;

    //    Field::Reader ActiveUnionField() const;

    //    template <typename T> requires(T::Kind == DeclKind::Struct)
    //    T* As() const
    //    {
    //        const bool valid = HE_VERIFY(&T::DeclInfo == m_info,
    //            HE_MSG("DynamicStruct requested as a type that doesn't match the schema."),
    //            HE_KV(requested_type_name, GetSchema(T::DeclInfo).GetName()),
    //            HE_KV(requested_type_id, T::Id),
    //            HE_KV(actual_type_name, Schema().GetName()),
    //            HE_KV(actual_type_id, Schema().GetId()));
    //        return valid ? static_cast<T*>(m_value) : nullptr;
    //    }

    //private:
    //    void SetInUnion(Field::Reader field);
    //    bool IsActiveInUnion(Field::Reader field) const;

    //private:
    //    const DeclInfo* m_info{ nullptr };
    //    void* m_value{ nullptr };
    //};

    //// --------------------------------------------------------------------------------------------
    //class DynamicArray3
    //{
    //public:
    //    using ElementType = DynamicValue3;
    //    using IteratorType = IndexIterator<DynamicArray3, false>;
    //    using ConstIteratorType = IndexIterator<DynamicArray3, true>;

    //public:
    //    DynamicArray3() = default;

    //    DynamicArray3(const DeclInfo& scope, Type::Reader type, void* data) noexcept
    //        : m_scope(&scope)
    //        , m_type(type)
    //        , m_data(data)
    //    {
    //        HE_ASSERT(GetSchema(scope).GetData().IsStruct());
    //        HE_ASSERT(type.GetData().IsArray());
    //    }

    //public:
    //    const DeclInfo& Scope() const { HE_ASSERT(m_scope); return *m_scope; }
    //    Type::Reader GetType() const { return m_type; }
    //    Type::Data::Array::Reader ArrayType() const { return m_type.GetData().GetArray(); }

    //    uint16_t Size() const { return ArrayType().GetSize(); }

    //    DynamicValue3 Get(uint16_t index) const;
    //    void Set(uint16_t index, const DynamicValue3& value);
    //    DynamicValue3 Init(uint16_t index);
    //    DynamicValue3 Init(uint16_t index, uint32_t size);
    //    void Clear(uint16_t index);
    //    bool Has(uint16_t index) const;

    //    DynamicValue3 operator[](uint16_t index) const;

    //    IteratorType begin() { return IteratorType(this, 0); }
    //    IteratorType end() { return IteratorType(this, Size()); }

    //    ConstIteratorType begin() const { return ConstIteratorType(this, 0); }
    //    ConstIteratorType end() const { return ConstIteratorType(this, Size()); }

    //private:
    //    Declaration::Reader ScopeSchema() const { HE_ASSERT(m_scope); return GetSchema(*m_scope); }
    //    Declaration::Data::Struct::Reader ScopeStruct() const { return ScopeSchema().GetData().GetStruct(); }

    //private:
    //    const DeclInfo* m_scope{ nullptr };
    //    Type::Reader m_type{};
    //    void* m_data{ nullptr };
    //};

    //// --------------------------------------------------------------------------------------------
    //class DynamicList3
    //{
    //public:
    //    using ElementType = DynamicValue3;
    //    using IteratorType = IndexIterator<DynamicList3, false>;
    //    using ConstIteratorType = IndexIterator<DynamicList3, true>;

    //public:
    //    DynamicList3() = default;

    //    DynamicList3(const DeclInfo& scope, Type::Reader type, void* data, uint32_t size) noexcept
    //        : m_scope(&scope)
    //        , m_type(type)
    //        , m_data(data)
    //        , m_size(size)
    //    {
    //        HE_ASSERT(GetSchema(scope).GetData().IsStruct());
    //        HE_ASSERT(type.GetData().IsList());
    //    }

    //public:
    //    const DeclInfo& Scope() const { HE_ASSERT(m_scope); return *m_scope; }
    //    Type::Reader GetType() const { return m_type; }
    //    Type::Data::List::Reader ListType() const { return m_type.GetData().GetList(); }

    //    uint32_t Size() const { return m_size; }

    //    DynamicValue3 Get(uint32_t index) const;
    //    void Set(uint32_t index, const DynamicValue3& value);
    //    DynamicValue3 Init(uint32_t index, uint32_t size);
    //    void Clear(uint32_t index);
    //    bool Has(uint32_t index) const;

    //    DynamicValue3 operator[](uint32_t index) const;

    //    IteratorType begin() { return IteratorType(this, 0); }
    //    IteratorType end() { return IteratorType(this, Size()); }

    //    ConstIteratorType begin() const { return ConstIteratorType(this, 0); }
    //    ConstIteratorType end() const { return ConstIteratorType(this, Size()); }

    //private:
    //    const DeclInfo* m_scope{ nullptr };
    //    Type::Reader m_type{};
    //    void* m_data{ nullptr };
    //    uint32_t m_size{ 0 };
    //};

    //// --------------------------------------------------------------------------------------------
    //class DynamicValue3
    //{
    //public:
    //    enum class Kind
    //    {
    //        /// Unsure how to represent the value, so we don't know what it is. This is commonly
    //        /// a result of using an older schema that doesn't know about a new type.
    //        Unknown,
    //        Void,
    //        Bool,
    //        Int,
    //        Uint,
    //        Float,
    //        Blob,
    //        String,
    //        Array,
    //        List,
    //        Enum,
    //        Struct,
    //        AnyPointer,
    //    };

    //public:
    //    DynamicValue3(decltype(nullptr) = nullptr) noexcept : m_kind(Kind::Unknown), m_void() {}
    //    DynamicValue3(Void* value) noexcept : m_kind(Kind::Void), m_void(value) {}
    //    DynamicValue3(bool* value) noexcept : m_kind(Kind::Bool), m_bool(value) {}
    //    DynamicValue3(char* value) noexcept : m_kind(Kind::Int), m_int(value) {}
    //    DynamicValue3(signed char* value) noexcept : m_kind(Kind::Int), m_int(value) {}
    //    DynamicValue3(short* value) noexcept : m_kind(Kind::Int), m_int(value) {}
    //    DynamicValue3(int* value) noexcept : m_kind(Kind::Int), m_int(value) {}
    //    DynamicValue3(long* value) noexcept : m_kind(Kind::Int), m_int(value) {}
    //    DynamicValue3(long long* value) noexcept : m_kind(Kind::Int), m_int(value) {}
    //    DynamicValue3(unsigned char* value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
    //    DynamicValue3(unsigned short* value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
    //    DynamicValue3(unsigned int* value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
    //    DynamicValue3(unsigned long* value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
    //    DynamicValue3(unsigned long long* value) noexcept : m_kind(Kind::Uint), m_uint(value) {}
    //    DynamicValue3(float* value) noexcept : m_kind(Kind::Float), m_float(value) {}
    //    DynamicValue3(double* value) noexcept : m_kind(Kind::Float), m_float(value) {}
    //    DynamicValue3(he::String* value) noexcept : m_kind(Kind::String), m_string(value) {}
    //    DynamicValue3(he::Vector<uint8_t>* value) noexcept : m_kind(Kind::Blob), m_blob(value) {}
    //    DynamicValue3(const DynamicArray::Builder& value) noexcept : m_kind(Kind::Array), m_array(value) {}
    //    DynamicValue3(const DynamicList::Builder& value) noexcept : m_kind(Kind::List), m_list(value) {}
    //    DynamicValue3(DynamicEnum value) noexcept : m_kind(Kind::Enum), m_enum(value) {}
    //    DynamicValue3(const DynamicStruct::Builder& value) noexcept : m_kind(Kind::Struct), m_struct(value) {}
    //    DynamicValue3(const PointerBuilder& value) noexcept : m_kind(Kind::AnyPointer), m_anyPointer(value) {}

    //private:
    //    Kind m_kind;

    //    union
    //    {
    //        DynamicDataValue3 m_void;
    //        DynamicDataValue3 m_bool;
    //        DynamicDataValue3 m_int;
    //        DynamicDataValue3 m_uint;
    //        DynamicDataValue3 m_float;
    //        DynamicList3 m_blob;
    //        DynamicString3 m_string;
    //        DynamicArray3 m_array;
    //        DynamicList3 m_list;
    //        DynamicEnum3 m_enum;
    //        DynamicStruct3 m_struct;
    //        DynamicPointer3 m_anyPointer;
    //    };
    //};
}
