// Copyright Chad Engler

#include "he/schema/dynamic.h"

#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/log.h"
#include "he/core/type_info.h"

#include <algorithm>
#include <concepts>
#include <limits>

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    #define HE_DYNAMIC_VALUE_CLASS_AS_TYPE(member, kind, Class, Type) \
        template <> typename LayoutTraits<Type>::Class DynamicValue::Class::As<Type>() const \
        { \
            const bool valid = HE_VERIFY(m_kind == Kind::kind, \
                HE_MSG("DynamicValue requested as a type it doesn't hold."), \
                HE_KV(requested_type_name, TypeInfo::Get<Type>().Name()), \
                HE_KV(actual_kind, m_kind)); \
            return valid ? member : typename LayoutTraits<Type>::Class{}; \
        }

    #define HE_DYNAMIC_VALUE_AS_TYPE(member, kind, Type) \
        HE_DYNAMIC_VALUE_CLASS_AS_TYPE(member, kind, Reader, Type) \
        HE_DYNAMIC_VALUE_CLASS_AS_TYPE(member, kind, Builder, Type)

    HE_DYNAMIC_VALUE_AS_TYPE(m_void, Void, Void)
    HE_DYNAMIC_VALUE_AS_TYPE(m_bool, Bool, bool)
    HE_DYNAMIC_VALUE_AS_TYPE(m_string, String, String)
    HE_DYNAMIC_VALUE_AS_TYPE(m_blob, Blob, Blob)
    HE_DYNAMIC_VALUE_AS_TYPE(m_list, List, DynamicList)
    HE_DYNAMIC_VALUE_AS_TYPE(m_enum, Enum, DynamicEnum)
    HE_DYNAMIC_VALUE_AS_TYPE(m_struct, Struct, DynamicStruct)
    HE_DYNAMIC_VALUE_AS_TYPE(m_anyPointer, AnyPointer, AnyPointer)

    #undef HE_DYNAMIC_VALUE_AS_TYPE
    #undef HE_DYNAMIC_VALUE_CLASS_AS_TYPE

    // --------------------------------------------------------------------------------------------
    template <typename T, typename U>
    T CoerceInteger(U value)
    {
        const T result = static_cast<T>(value);
        HE_VERIFY(static_cast<U>(result) == value,
            HE_MSG("Value out of range for requested type."),
            HE_KV(requested_type, TypeInfo::Get<T>().Name()),
            HE_KV(value, value));
        return result;
    }

    template <typename T>
    T CoerceSignedToUnsigned(long long value)
    {
        HE_VERIFY(value >= 0 && static_cast<T>(value) == value,
            HE_MSG("Value out of range for requested type."),
            HE_KV(requested_type, TypeInfo::Get<T>().Name()),
            HE_KV(value, value));
        return static_cast<T>(value);
    }

    template <>
    uint64_t CoerceSignedToUnsigned<uint64_t>(long long value)
    {
        HE_VERIFY(value >= 0,
            HE_MSG("Value out of range for requested type."),
            HE_KV(requested_type, TypeInfo::Get<uint64_t>().Name()),
            HE_KV(value, value));
        return static_cast<uint64_t>(value);
    }

    template <typename T>
    T CoerceUnsignedToSigned(unsigned long long value)
    {
        const T result = static_cast<T>(value);
        HE_VERIFY(result >= 0 && static_cast<unsigned long long>(result) == value,
            HE_MSG("Value out of range for requested type."),
            HE_KV(requested_type, TypeInfo::Get<T>().Name()),
            HE_KV(value, value));
        return result;
    }

    template <>
    int64_t CoerceUnsignedToSigned<int64_t>(unsigned long long value)
    {
        const int64_t result = static_cast<int64_t>(value);
        HE_VERIFY(result >= 0,
            HE_MSG("Value out of range for requested type."),
            HE_KV(requested_type, TypeInfo::Get<int64_t>().Name()),
            HE_KV(value, value));
        return result;
    }

    template <typename T, std::floating_point U>
    T CoerceFloat(U value)
    {
        constexpr T min = std::numeric_limits<T>::lowest();
        constexpr T max = std::numeric_limits<T>::max();
        HE_VERIFY(value >= static_cast<U>(min) && value <= static_cast<U>(max),
            HE_MSG("Value out of range for requested type."),
            HE_KV(requested_type, TypeInfo::Get<T>().Name()),
            HE_KV(value, value));

        const T result = static_cast<T>(value);
        HE_VERIFY(static_cast<U>(result) == value,
            HE_MSG("Value out of range for requested type."),
            HE_KV(requested_type, TypeInfo::Get<T>().Name()),
            HE_KV(value, value));
        return result;
    }

    template <typename T, typename U>
    T ImplicitCast(U&& value) { return static_cast<T>(Forward<U>(value)); }

    #define HE_DYNAMIC_VALUE_CLASS_AS_NUMERIC_TYPE(Class, Type, coerceInt, coerceUint, coerceFloat) \
        template <> Type DynamicValue::Class::As<Type>() const \
        { \
            switch (m_kind) \
            { \
                case Kind::Int: return coerceInt<Type>(m_int); \
                case Kind::Uint: return coerceUint<Type>(m_uint); \
                case Kind::Float: return coerceFloat<Type>(m_float); \
                default: \
                    HE_VERIFY(false, \
                        HE_MSG("DynamicValue requested as a numeric type, but it doesn't hold a numeric type."), \
                        HE_KV(requested_type_name, TypeInfo::Get<Type>().Name()), \
                        HE_KV(actual_kind, m_kind)); \
                    return Type{}; \
            } \
        }

    #define HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(Type, coerceInt, coerceUint, coerceFloat) \
        HE_DYNAMIC_VALUE_CLASS_AS_NUMERIC_TYPE(Reader, Type, coerceInt, coerceUint, coerceFloat) \
        HE_DYNAMIC_VALUE_CLASS_AS_NUMERIC_TYPE(Builder, Type, coerceInt, coerceUint, coerceFloat)

    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(int8_t, CoerceInteger, CoerceUnsignedToSigned, CoerceFloat)
    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(int16_t, CoerceInteger, CoerceUnsignedToSigned, CoerceFloat)
    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(int32_t, CoerceInteger, CoerceUnsignedToSigned, CoerceFloat)
    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(int64_t, ImplicitCast, CoerceUnsignedToSigned, CoerceFloat)
    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(uint8_t, CoerceSignedToUnsigned, CoerceInteger, CoerceFloat)
    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(uint16_t, CoerceSignedToUnsigned, CoerceInteger, CoerceFloat)
    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(uint32_t, CoerceSignedToUnsigned, CoerceInteger, CoerceFloat)
    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(uint64_t, CoerceSignedToUnsigned, ImplicitCast, CoerceFloat)
    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(float, ImplicitCast, ImplicitCast, ImplicitCast)
    HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE(double, ImplicitCast, ImplicitCast, ImplicitCast)

    #undef HE_DYNAMIC_VALUE_AS_NUMERIC_TYPE
    #undef HE_DYNAMIC_VALUE_CLASS_AS_NUMERIC_TYPE

    // --------------------------------------------------------------------------------------------
    static uint16_t GetEnumValue(const DeclInfo& parentInfo, Type::Reader type, const DynamicValue::Reader& value)
    {
        const Type::Data::Enum::Reader enumType = type.GetData().GetEnum();
        const DeclInfo* info = FindDependency(parentInfo, enumType.GetId());
        if (!HE_VERIFY(info,
            HE_MSG("Type is an enum that is not listed in the parent dependencies."),
            HE_KV(struct_name, GetSchema(parentInfo).GetName()),
            HE_KV(requested_type_id, enumType.GetId())))
        {
            return 0;
        }

        const Declaration::Reader decl = GetSchema(*info);
        if (!HE_VERIFY(decl.GetData().IsEnum()))
            return 0;

        const Declaration::Data::Enum::Reader enumDecl = decl.GetData().GetEnum();

        switch (value.GetKind())
        {
            case DynamicValue::Kind::Int:
            case DynamicValue::Kind::Uint:
            {
                return value.As<uint16_t>();
            }
            case DynamicValue::Kind::String:
            {
                const String::Reader str = value.As<String>();
                for (const Enumerator::Reader e : enumDecl.GetEnumerators())
                {
                    if (e.GetName() == str)
                    {
                        return e.GetOrdinal();
                    }
                }
                HE_VERIFY(false,
                    HE_MSG("Unknown enum string, no such value found in the schema"),
                    HE_KV(enum_name, decl.GetName()),
                    HE_KV(requested_enumerator, str));
                return 0;
            }
            case DynamicValue::Kind::Enum:
            {
                const DynamicEnum e = value.As<DynamicEnum>();
                return HE_VERIFY(&e.Decl() == info) ? e.Value() : 0;
            }
            default:
                HE_VERIFY(false,
                    HE_MSG("Unsupported DynamicValue kind for enum"),
                    HE_KV(enum_name, decl.GetName()),
                    HE_KV(value_kind, value.GetKind()));
                return 0;
        }
    }

    static const void* GetDataArrayPointer(StructReader reader, Type::Reader elementType, uint16_t index, uint32_t dataOffset, uint16_t arraySize, const Word* defaultValue)
    {
        switch (elementType.GetData().GetUnionTag())
        {
            case Type::Data::UnionTag::Void: return reader.TryGetDataArrayField<Void>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Bool: return reader.TryGetDataArrayField<bool>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Int8: return reader.TryGetDataArrayField<int8_t>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Int16: return reader.TryGetDataArrayField<int16_t>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Int32: return reader.TryGetDataArrayField<int32_t>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Int64: return reader.TryGetDataArrayField<int64_t>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Uint8: return reader.TryGetDataArrayField<uint8_t>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Uint16: return reader.TryGetDataArrayField<uint16_t>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Uint32: return reader.TryGetDataArrayField<uint32_t>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Uint64: return reader.TryGetDataArrayField<uint64_t>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Float32: return reader.TryGetDataArrayField<float>(index, dataOffset, arraySize, defaultValue).Data();
            case Type::Data::UnionTag::Float64: return reader.TryGetDataArrayField<double>(index, dataOffset, arraySize, defaultValue).Data();
            default:
                HE_VERIFY(false, HE_MSG("Only works for arrays with data element types."));
                return nullptr;
        }
    }

    // --------------------------------------------------------------------------------------------
    DynamicValue::Reader DynamicStruct::Reader::Get(Field::Reader field) const
    {
        if (!field.IsValid())
            return DynamicValue::Reader{};

        const Declaration::Data::Struct::Reader structDecl = StructSchema();

        const List<Field>::Reader fields = structDecl.GetFields();
        const auto it = std::find(fields.begin(), fields.end(), field);
        if (!HE_VERIFY(it != fields.end(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return DynamicValue::Reader{};
        }

        if (!HE_VERIFY(!structDecl.GetIsUnion() || IsActiveInUnion(field),
            HE_MSG("Field requested from DynamicStruct that is not active in the union."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return DynamicValue::Reader{};
        }

        const Field::Meta::Reader meta = field.GetMeta();

        switch (meta.GetUnionTag())
        {
            case Field::Meta::UnionTag::Normal:
            {
                const Field::Meta::Normal::Reader norm = meta.GetNormal();
                const Value::Data::Reader defaultValue = norm.GetDefaultValue().GetData();
                const Type::Reader type = norm.GetType();
                const Type::Data::Reader typeData = type.GetData();

                const uint16_t index = norm.GetIndex();
                const uint32_t dataOffset = norm.GetDataOffset();

                switch (typeData.GetUnionTag())
                {
                    case Type::Data::UnionTag::Void: return m_reader.TryGetDataField<Void>(index, dataOffset);
                    case Type::Data::UnionTag::Bool: return m_reader.TryGetDataField<bool>(index, dataOffset, defaultValue.IsBool() ? defaultValue.GetBool() : false);
                    case Type::Data::UnionTag::Int8: return m_reader.TryGetDataField<int8_t>(index, dataOffset, defaultValue.IsInt8() ? defaultValue.GetInt8() : 0);
                    case Type::Data::UnionTag::Int16: return m_reader.TryGetDataField<int16_t>(index, dataOffset, defaultValue.IsInt16() ? defaultValue.GetInt16() : 0);
                    case Type::Data::UnionTag::Int32: return m_reader.TryGetDataField<int32_t>(index, dataOffset, defaultValue.IsInt32() ? defaultValue.GetInt32() : 0);
                    case Type::Data::UnionTag::Int64: return m_reader.TryGetDataField<int64_t>(index, dataOffset, defaultValue.IsInt64() ? defaultValue.GetInt64() : 0);
                    case Type::Data::UnionTag::Uint8: return m_reader.TryGetDataField<uint8_t>(index, dataOffset, defaultValue.IsUint8() ? defaultValue.GetUint8() : 0);
                    case Type::Data::UnionTag::Uint16: return m_reader.TryGetDataField<uint16_t>(index, dataOffset, defaultValue.IsUint16() ? defaultValue.GetUint16() : 0);
                    case Type::Data::UnionTag::Uint32: return m_reader.TryGetDataField<uint32_t>(index, dataOffset, defaultValue.IsUint32() ? defaultValue.GetUint32() : 0);
                    case Type::Data::UnionTag::Uint64: return m_reader.TryGetDataField<uint64_t>(index, dataOffset, defaultValue.IsUint64() ? defaultValue.GetUint64() : 0);
                    case Type::Data::UnionTag::Float32: return m_reader.TryGetDataField<float>(index, dataOffset, defaultValue.IsFloat32() ? defaultValue.GetFloat32() : 0.0f);
                    case Type::Data::UnionTag::Float64: return m_reader.TryGetDataField<double>(index, dataOffset, defaultValue.IsFloat64() ? defaultValue.GetFloat64() : 0.0);
                    case Type::Data::UnionTag::Blob:
                    {
                        const uint8_t* value = defaultValue.IsBlob() ? defaultValue.GetBlob().Data() : nullptr;
                        return m_reader.GetPointerField(index).TryGetBlob(reinterpret_cast<const Word*>(value));
                    }
                    case Type::Data::UnionTag::String:
                    {
                        const char* value = defaultValue.IsString() ? defaultValue.GetString().Data() : nullptr;
                        return m_reader.GetPointerField(index).TryGetString(reinterpret_cast<const Word*>(value));
                    }
                    case Type::Data::UnionTag::AnyPointer:
                    {
                        return AnyPointer::Reader(m_reader.GetPointerField(index));
                    }
                    case Type::Data::UnionTag::AnyStruct:
                    {
                        return AnyStruct::Reader(m_reader.GetPointerField(index));
                    }
                    case Type::Data::UnionTag::AnyList:
                    {
                        return AnyList::Reader(m_reader.GetPointerField(index));
                    }
                    case Type::Data::UnionTag::Array:
                    {
                        const Type::Data::Array::Reader arrayType = typeData.GetArray();
                        const uint16_t arraySize = arrayType.GetSize();
                        const Type::Reader elementType = arrayType.GetElementType();

                        const Word* value = defaultValue.IsList() ? defaultValue.GetList().Data() : nullptr;

                        if (IsPointer(elementType))
                        {
                            const ListReader reader = m_reader.TryGetPointerArrayField(index, arraySize, value);
                            return DynamicArray::Reader(*m_info, type, reader);
                        }
                        else
                        {
                            const void* ptr = GetDataArrayPointer(m_reader, elementType, index, dataOffset, arraySize, value);
                            return DynamicArray::Reader(*m_info, type, ptr);
                        }
                        break;
                    }
                    case Type::Data::UnionTag::List:
                    {
                        const Type::Data::List::Reader listType = typeData.GetList();
                        const Type::Reader elementType = listType.GetElementType();
                        const ElementSize elementSize = GetTypeElementSize(elementType);

                        const Word* value = defaultValue.IsList() ? defaultValue.GetList().Data() : nullptr;
                        const ListReader reader = m_reader.GetPointerField(index).TryGetList(elementSize, value);

                        return DynamicList::Reader(*m_info, type, reader);
                    }
                    case Type::Data::UnionTag::Enum:
                    {
                        const Type::Data::Enum::Reader enumType = typeData.GetEnum();
                        const DeclInfo* info = FindDependency(*m_info, enumType.GetId());
                        const uint16_t value = defaultValue.IsEnum() ? defaultValue.GetEnum() : 0;
                        const bool valid = HE_VERIFY(info,
                            HE_MSG("Field requested from DynamicStruct is an enum that has a missing type."),
                            HE_KV(struct_name, Schema().GetName()),
                            HE_KV(requested_field_name, field.GetName()),
                            HE_KV(requested_field_type_id, enumType.GetId()));
                        return valid ? DynamicEnum(*info, value) : DynamicEnum{};
                    }
                    case Type::Data::UnionTag::Struct:
                    {
                        const Type::Data::Struct::Reader structType = typeData.GetStruct();
                        const DeclInfo* info = FindDependency(*m_info, structType.GetId());
                        const bool valid = HE_VERIFY(info,
                            HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                            HE_KV(struct_name, Schema().GetName()),
                            HE_KV(requested_field_name, field.GetName()),
                            HE_KV(requested_field_type_id, structType.GetId()));
                        return valid ? DynamicStruct::Reader(*info, m_reader) : DynamicStruct::Reader{};
                    }
                    case Type::Data::UnionTag::Interface:
                    {
                        return DynamicValue::Reader{}; // Interfaces do not currently have values.
                    }
                    case Type::Data::UnionTag::Parameter:
                    {
                        return AnyPointer::Reader(m_reader.GetPointerField(index));
                    }
                }

                return DynamicValue::Reader{};
            }
            case Field::Meta::UnionTag::Group:
            {
                const Field::Meta::Group::Reader group = meta.GetGroup();
                const DeclInfo* info = FindDependency(*m_info, group.GetTypeId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a group that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, group.GetTypeId()));
                return valid ? DynamicStruct::Reader(*info, m_reader) : DynamicValue::Reader{};
            }
            case Field::Meta::UnionTag::Union:
            {
                const Field::Meta::Union::Reader group = meta.GetUnion();
                const DeclInfo* info = FindDependency(*m_info, group.GetTypeId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a union that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, group.GetTypeId()));
                return valid ? DynamicStruct::Reader(*info, m_reader) : DynamicValue::Reader{};
            }
        }

        return DynamicValue::Reader{};
    }

    DynamicValue::Reader DynamicStruct::Reader::Get(StringView fieldName) const
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? Get(field) : DynamicValue::Reader{};
    }

    bool DynamicStruct::Reader::Has(Field::Reader field) const
    {
        if (!field.IsValid())
            return false;

        const Declaration::Data::Struct::Reader structDecl = StructSchema();

        const List<Field>::Reader fields = structDecl.GetFields();
        const auto it = std::find(fields.begin(), fields.end(), field);
        if (!HE_VERIFY(it != fields.end(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return false;
        }

        if (structDecl.GetIsUnion())
        {
            return IsActiveInUnion(field);
        }

        const Field::Meta::Reader meta = field.GetMeta();

        switch (meta.GetUnionTag())
        {
            case Field::Meta::UnionTag::Normal:
            {
                const Field::Meta::Normal::Reader norm = meta.GetNormal();
                const bool isPointer = IsPointer(norm.GetType());
                const uint16_t index = norm.GetIndex();
                return isPointer ? m_reader.HasPointerField(index) : m_reader.HasDataField(index);
            }
            case Field::Meta::UnionTag::Group:
            case Field::Meta::UnionTag::Union:
            {
                // groups are always available
                return true;
            }
        }

        return false;
    }

    bool DynamicStruct::Reader::Has(StringView fieldName) const
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? Has(field) : false;
    }

    bool DynamicStruct::Reader::IsActiveInUnion(Field::Reader field) const
    {
        const Declaration::Reader decl = GetSchema(*m_info);

        if (!decl.IsValid() || !decl.GetData().IsStruct() || !decl.GetData().GetStruct().GetIsUnion())
            return false;

        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
        const uint16_t activeUnionTag = m_reader.GetDataField<uint16_t>(structDecl.GetUnionTagOffset());
        return field.GetUnionTag() == activeUnionTag;
    }

    Field::Reader DynamicStruct::Reader::ActiveUnionField() const
    {
        const Declaration::Data::Struct::Reader structDecl = StructSchema();
        if (!structDecl.GetIsUnion())
            return {};

        const uint16_t tag = m_reader.GetDataField<uint16_t>(structDecl.GetUnionTagOffset());
        return FindFieldByUnionTag(tag, structDecl);
    }

    // --------------------------------------------------------------------------------------------
    DynamicValue::Builder DynamicStruct::Builder::Get(Field::Reader field) const
    {
        if (!field.IsValid())
            return DynamicValue::Builder{};

        const Declaration::Data::Struct::Reader structDecl = StructSchema();

        const List<Field>::Reader fields = structDecl.GetFields();
        const auto it = std::find(fields.begin(), fields.end(), field);
        if (!HE_VERIFY(it != fields.end(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return DynamicValue::Builder{};
        }

        if (!HE_VERIFY(!structDecl.GetIsUnion() || IsActiveInUnion(field),
            HE_MSG("Field requested from DynamicStruct that is not active in the union."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return DynamicValue::Builder{};
        }

        switch (field.GetMeta().GetUnionTag())
        {
            case Field::Meta::UnionTag::Normal:
            {
                const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                const Value::Data::Reader defaultValue = norm.GetDefaultValue().GetData();
                const Type::Reader type = norm.GetType();
                const Type::Data::Reader typeData = type.GetData();

                const uint16_t index = norm.GetIndex();
                const uint32_t dataOffset = norm.GetDataOffset();

                switch (typeData.GetUnionTag())
                {
                    case Type::Data::UnionTag::Void: return m_builder.TryGetDataField<Void>(index, dataOffset);
                    case Type::Data::UnionTag::Bool: return m_builder.TryGetDataField<bool>(index, dataOffset, defaultValue.IsBool() ? defaultValue.GetBool() : false);
                    case Type::Data::UnionTag::Int8: return m_builder.TryGetDataField<int8_t>(index, dataOffset, defaultValue.IsInt8() ? defaultValue.GetInt8() : 0);
                    case Type::Data::UnionTag::Int16: return m_builder.TryGetDataField<int16_t>(index, dataOffset, defaultValue.IsInt16() ? defaultValue.GetInt16() : 0);
                    case Type::Data::UnionTag::Int32: return m_builder.TryGetDataField<int32_t>(index, dataOffset, defaultValue.IsInt32() ? defaultValue.GetInt32() : 0);
                    case Type::Data::UnionTag::Int64: return m_builder.TryGetDataField<int64_t>(index, dataOffset, defaultValue.IsInt64() ? defaultValue.GetInt64() : 0);
                    case Type::Data::UnionTag::Uint8: return m_builder.TryGetDataField<uint8_t>(index, dataOffset, defaultValue.IsUint8() ? defaultValue.GetUint8() : 0);
                    case Type::Data::UnionTag::Uint16: return m_builder.TryGetDataField<uint16_t>(index, dataOffset, defaultValue.IsUint16() ? defaultValue.GetUint16() : 0);
                    case Type::Data::UnionTag::Uint32: return m_builder.TryGetDataField<uint32_t>(index, dataOffset, defaultValue.IsUint32() ? defaultValue.GetUint32() : 0);
                    case Type::Data::UnionTag::Uint64: return m_builder.TryGetDataField<uint64_t>(index, dataOffset, defaultValue.IsUint64() ? defaultValue.GetUint64() : 0);
                    case Type::Data::UnionTag::Float32: return m_builder.TryGetDataField<float>(index, dataOffset, defaultValue.IsFloat32() ? defaultValue.GetFloat32() : 0.0f);
                    case Type::Data::UnionTag::Float64: return m_builder.TryGetDataField<double>(index, dataOffset, defaultValue.IsFloat64() ? defaultValue.GetFloat64() : 0.0);
                    case Type::Data::UnionTag::Blob: return m_builder.GetPointerField(index).TryGetBlob();
                    case Type::Data::UnionTag::String: return m_builder.GetPointerField(index).TryGetString();
                    case Type::Data::UnionTag::AnyPointer: return AnyPointer::Builder(m_builder.GetPointerField(index));
                    case Type::Data::UnionTag::AnyStruct: return AnyStruct::Builder(m_builder.GetPointerField(index));
                    case Type::Data::UnionTag::AnyList: return AnyList::Builder(m_builder.GetPointerField(index));
                    case Type::Data::UnionTag::Array:
                    {
                        const Type::Data::Array::Reader arrayType = typeData.GetArray();
                        const uint16_t arraySize = arrayType.GetSize();
                        const Type::Reader elementType = arrayType.GetElementType();

                        if (IsPointer(elementType))
                        {
                            const ListBuilder builder = m_builder.GetPointerArrayField(index, arraySize);
                            return DynamicArray::Builder(*m_info, type, builder);
                        }
                        else
                        {
                            const void* ptr = GetDataArrayPointer(m_builder, elementType, index, dataOffset, arraySize, nullptr);
                            return DynamicArray::Builder(*m_info, type, const_cast<void*>(ptr));
                        }
                        break;
                    }
                    case Type::Data::UnionTag::List:
                    {
                        const Type::Data::List::Reader listType = typeData.GetList();
                        const Type::Reader elementType = listType.GetElementType();
                        const ElementSize elementSize = GetTypeElementSize(elementType);
                        const ListBuilder reader = m_builder.GetPointerField(index).TryGetList(elementSize);

                        return DynamicList::Builder(*m_info, type, reader);
                    }
                    case Type::Data::UnionTag::Enum:
                    {
                        const Type::Data::Enum::Reader enumType = typeData.GetEnum();
                        const DeclInfo* info = FindDependency(*m_info, enumType.GetId());
                        const uint16_t value = defaultValue.IsEnum() ? defaultValue.GetEnum() : 0;
                        const bool valid = HE_VERIFY(info,
                            HE_MSG("Field requested from DynamicStruct is an enum that has a missing type."),
                            HE_KV(struct_name, Schema().GetName()),
                            HE_KV(requested_field_name, field.GetName()),
                            HE_KV(requested_field_type_id, enumType.GetId()));
                        return valid ? DynamicEnum(*info, value) : DynamicEnum{};
                    }
                    case Type::Data::UnionTag::Struct:
                    {
                        const Type::Data::Struct::Reader structType = typeData.GetStruct();
                        const DeclInfo* info = FindDependency(*m_info, structType.GetId());
                        const bool valid = HE_VERIFY(info,
                            HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                            HE_KV(struct_name, Schema().GetName()),
                            HE_KV(requested_field_name, field.GetName()),
                            HE_KV(requested_field_type_id, structType.GetId()));
                        return valid ? DynamicStruct::Builder(*info, m_builder) : DynamicStruct::Builder{};
                    }
                    case Type::Data::UnionTag::Interface:
                    {
                        return DynamicValue::Builder{}; // Interfaces do not currently have values.
                    }
                    case Type::Data::UnionTag::Parameter:
                    {
                        return AnyPointer::Builder(m_builder.GetPointerField(index));
                    }
                }

                return DynamicValue::Builder{};
            }
            case Field::Meta::UnionTag::Group:
            {
                const Field::Meta::Group::Reader group = field.GetMeta().GetGroup();
                const DeclInfo* info = FindDependency(*m_info, group.GetTypeId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a group that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, group.GetTypeId()));
                return valid ? DynamicStruct::Builder(*info, m_builder) : DynamicValue::Builder{};
            }
            case Field::Meta::UnionTag::Union:
            {
                const Field::Meta::Union::Reader group = field.GetMeta().GetUnion();
                const DeclInfo* info = FindDependency(*m_info, group.GetTypeId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a union that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, group.GetTypeId()));
                return valid ? DynamicStruct::Builder(*info, m_builder) : DynamicValue::Builder{};
            }
        }

        return DynamicValue::Builder{};
    }

    DynamicValue::Builder DynamicStruct::Builder::Get(StringView fieldName) const
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? Get(field) : DynamicValue::Builder{};
    }

    void DynamicStruct::Builder::Set(Field::Reader field, const DynamicValue::Reader& value)
    {
        if (!field.IsValid())
            return;

        const Declaration::Data::Struct::Reader structDecl = StructSchema();

        const List<Field>::Reader fields = structDecl.GetFields();
        const auto it = std::find(fields.begin(), fields.end(), field);
        if (!HE_VERIFY(it != fields.end(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return;
        }

        SetInUnion(field);

        switch (field.GetMeta().GetUnionTag())
        {
            case Field::Meta::UnionTag::Normal:
            {
                const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                const Value::Data::Reader defaultValue = norm.GetDefaultValue().GetData();
                const Type::Reader type = norm.GetType();
                const Type::Data::Reader typeData = type.GetData();

                const uint16_t index = norm.GetIndex();
                const uint32_t dataOffset = norm.GetDataOffset();

                switch (typeData.GetUnionTag())
                {
                    case Type::Data::UnionTag::Void: m_builder.SetAndMarkDataField(index, dataOffset, value.As<Void>()); break;
                    case Type::Data::UnionTag::Bool: m_builder.SetAndMarkDataField(index, dataOffset, value.As<bool>()); break;
                    case Type::Data::UnionTag::Int8: m_builder.SetAndMarkDataField(index, dataOffset, value.As<int8_t>()); break;
                    case Type::Data::UnionTag::Int16: m_builder.SetAndMarkDataField(index, dataOffset, value.As<int16_t>()); break;
                    case Type::Data::UnionTag::Int32: m_builder.SetAndMarkDataField(index, dataOffset, value.As<int32_t>()); break;
                    case Type::Data::UnionTag::Int64: m_builder.SetAndMarkDataField(index, dataOffset, value.As<int64_t>()); break;
                    case Type::Data::UnionTag::Uint8: m_builder.SetAndMarkDataField(index, dataOffset, value.As<uint8_t>()); break;
                    case Type::Data::UnionTag::Uint16: m_builder.SetAndMarkDataField(index, dataOffset, value.As<uint16_t>()); break;
                    case Type::Data::UnionTag::Uint32: m_builder.SetAndMarkDataField(index, dataOffset, value.As<uint32_t>()); break;
                    case Type::Data::UnionTag::Uint64: m_builder.SetAndMarkDataField(index, dataOffset, value.As<uint64_t>()); break;
                    case Type::Data::UnionTag::Float32: m_builder.SetAndMarkDataField(index, dataOffset, value.As<float>()); break;
                    case Type::Data::UnionTag::Float64: m_builder.SetAndMarkDataField(index, dataOffset, value.As<double>()); break;
                    case Type::Data::UnionTag::Blob: m_builder.GetPointerField(index).Set(value.As<Blob>()); break;
                    case Type::Data::UnionTag::String: m_builder.GetPointerField(index).Set(value.As<String>()); break;
                    case Type::Data::UnionTag::AnyPointer:
                    case Type::Data::UnionTag::Parameter:
                    {
                        switch (value.GetKind())
                        {
                            case DynamicValue::Kind::Blob: m_builder.GetPointerField(index).Set(value.As<Blob>()); break;
                            case DynamicValue::Kind::String: m_builder.GetPointerField(index).Set(value.As<String>()); break;
                            case DynamicValue::Kind::List: m_builder.GetPointerField(index).Set(value.As<DynamicList>().List()); break;
                            case DynamicValue::Kind::Struct: m_builder.GetPointerField(index).Set(value.As<DynamicStruct>().Struct()); break;
                            case DynamicValue::Kind::AnyPointer: m_builder.GetPointerField(index).Set(value.As<AnyPointer>()); break;
                            default:
                                HE_VERIFY(false, HE_MSG("Expected a pointer value."), HE_KV(value_kind, value.GetKind()));
                        }
                        break;
                    }
                    case Type::Data::UnionTag::AnyStruct:
                    {
                        const DynamicStruct::Reader src = value.As<DynamicStruct>();
                        m_builder.GetPointerField(index).Set(src.Struct());
                        break;
                    }
                    case Type::Data::UnionTag::AnyList:
                    {
                        const DynamicList::Reader src = value.As<DynamicList>();
                        m_builder.GetPointerField(index).Set(src.List());
                        break;
                    }
                    case Type::Data::UnionTag::Array:
                    {
                        HE_VERIFY(false, HE_MSG("Setting an array field via DynamicStruct::Builder::Set is not supported. Use DynamicStruct::Builder::Get() to get a DynamicArray::Builder object instead."));
                        break;
                    }
                    case Type::Data::UnionTag::List:
                    {
                        const DynamicList::Reader src = value.As<DynamicList>();
                        if (HE_VERIFY(src.Type() == type))
                        {
                            m_builder.GetPointerField(index).Set(src.List());
                        }
                        break;
                    }
                    case Type::Data::UnionTag::Enum:
                    {
                        const uint16_t enumValue = GetEnumValue(*m_info, type, value);
                        m_builder.SetAndMarkDataField(index, dataOffset, enumValue);
                        break;
                    }
                    case Type::Data::UnionTag::Struct:
                    {
                        const Type::Data::Struct::Reader structType = typeData.GetStruct();
                        const DeclInfo* info = FindDependency(*m_info, structType.GetId());
                        if (!HE_VERIFY(info,
                            HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                            HE_KV(struct_name, Schema().GetName()),
                            HE_KV(requested_field_name, field.GetName()),
                            HE_KV(requested_field_type_id, structType.GetId())))
                        {
                            return;
                        }

                        const DynamicStruct::Reader src = value.As<DynamicStruct>();

                        if (HE_VERIFY(&src.Decl() == info))
                        {
                            m_builder.GetPointerField(index).Set(src.Struct());
                        }
                        break;
                    }
                    case Type::Data::UnionTag::Interface:
                    {
                        // Interfaces do not currently have values.
                        break;
                    }
                }
                break;
            }
            case Field::Meta::UnionTag::Group:
            {
                const DynamicStruct::Reader src = value.As<DynamicStruct>();
                DynamicStruct::Builder dst = Init(field).As<DynamicStruct>();

                if (!HE_VERIFY(&src.Decl() == &dst.Decl()))
                    return;

                for (const Field::Reader groupField : src.StructSchema().GetFields())
                {
                    if (src.Has(groupField))
                    {
                        dst.Set(groupField, src.Get(groupField));
                    }
                }
                break;
            }
            case Field::Meta::UnionTag::Union:
            {
                const DynamicStruct::Reader src = value.As<DynamicStruct>();
                DynamicStruct::Builder dst = Init(field).As<DynamicStruct>();

                if (!HE_VERIFY(&src.Decl() == &dst.Decl()))
                    return;

                const Field::Reader unionField = src.ActiveUnionField();
                if (HE_VERIFY(unionField.IsValid()))
                {
                    dst.Set(unionField, src.Get(unionField));
                }
                break;
            }
        }
    }

    void DynamicStruct::Builder::Set(StringView fieldName, const DynamicValue::Reader& value)
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        if (HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName)))
        {
            Set(field, value);
        }
    }

    DynamicValue::Builder DynamicStruct::Builder::Init(Field::Reader field)
    {
        if (!field.IsValid())
            return DynamicValue::Builder{};

        const Declaration::Data::Struct::Reader structDecl = StructSchema();

        const List<Field>::Reader fields = structDecl.GetFields();
        const auto it = std::find(fields.begin(), fields.end(), field);
        if (!HE_VERIFY(it != fields.end(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return DynamicValue::Builder{};
        }

        SetInUnion(field);

        switch (field.GetMeta().GetUnionTag())
        {
            case Field::Meta::UnionTag::Normal:
            {
                const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                const Value::Data::Reader defaultValue = norm.GetDefaultValue().GetData();
                const Type::Reader type = norm.GetType();
                const Type::Data::Reader typeData = type.GetData();

                const uint16_t index = norm.GetIndex();

                switch (typeData.GetUnionTag())
                {
                    case Type::Data::UnionTag::AnyPointer:
                    case Type::Data::UnionTag::AnyStruct:
                    case Type::Data::UnionTag::Parameter:
                    {
                        PointerBuilder ptr = m_builder.GetPointerField(index);
                        ptr.SetNull();
                        return AnyPointer::Builder(ptr);
                    }
                    case Type::Data::UnionTag::Struct:
                    {
                        const Type::Data::Struct::Reader structType = typeData.GetStruct();
                        const DeclInfo* info = FindDependency(*m_info, structType.GetId());
                        if (!HE_VERIFY(info,
                            HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                            HE_KV(struct_name, Schema().GetName()),
                            HE_KV(requested_field_name, field.GetName()),
                            HE_KV(requested_field_type_id, structType.GetId())))
                        {
                            return DynamicStruct::Builder{};
                        }

                        const Declaration::Reader decl = GetSchema(*info);
                        const Declaration::Data::Struct::Reader fieldStructDecl = decl.GetData().GetStruct();
                        const StructBuilder builder = m_builder.GetBuilder()->AddStruct(fieldStructDecl.GetDataFieldCount(), fieldStructDecl.GetDataWordSize(), fieldStructDecl.GetPointerCount());
                        m_builder.GetPointerField(index).Set(builder);
                        return DynamicStruct::Builder(*info, builder);
                    }
                    default:
                        HE_VERIFY(false, HE_MSG("Init without a size is only valid for struct fields"), HE_KV(field_type, typeData.GetUnionTag()));
                        return DynamicStruct::Builder{};
                }

                break;
            }
            case Field::Meta::UnionTag::Group:
            {
                Clear(field);
                const Field::Meta::Group::Reader group = field.GetMeta().GetGroup();
                const DeclInfo* info = FindDependency(*m_info, group.GetTypeId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a group that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, group.GetTypeId()));
                return valid ? DynamicStruct::Builder(*info, m_builder) : DynamicValue::Builder{};
            }
            case Field::Meta::UnionTag::Union:
            {
                Clear(field);
                const Field::Meta::Union::Reader group = field.GetMeta().GetUnion();
                const DeclInfo* info = FindDependency(*m_info, group.GetTypeId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a union that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, group.GetTypeId()));
                return valid ? DynamicStruct::Builder(*info, m_builder) : DynamicValue::Builder{};
            }
        }

        return DynamicValue::Builder{};
    }

    DynamicValue::Builder DynamicStruct::Builder::Init(StringView fieldName)
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? Init(field) : DynamicValue::Builder{};
    }

    DynamicValue::Builder DynamicStruct::Builder::Init(Field::Reader field, uint32_t size)
    {
        if (!field.IsValid())
            return DynamicValue::Builder{};

        const Declaration::Data::Struct::Reader structDecl = StructSchema();

        const List<Field>::Reader fields = structDecl.GetFields();
        const auto it = std::find(fields.begin(), fields.end(), field);
        if (!HE_VERIFY(it != fields.end(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return DynamicValue::Builder{};
        }

        SetInUnion(field);

        if (!HE_VERIFY(field.GetMeta().IsNormal(),
            HE_MSG("Init with a size is only valid for blob, string, and list fields"),
            HE_KV(field_kind, field.GetMeta().GetUnionTag())))
        {
            return DynamicStruct::Builder{};
        }

        const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
        const Value::Data::Reader defaultValue = norm.GetDefaultValue().GetData();
        const Type::Reader type = norm.GetType();
        const Type::Data::Reader typeData = type.GetData();

        const uint16_t index = norm.GetIndex();

        switch (typeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Blob:
            {
                Blob::Builder blob = m_builder.GetBuilder()->AddList<uint8_t>(size);
                m_builder.GetPointerField(index).Set(blob);
                return DynamicList::Builder(*m_info, type, blob);
            }
            case Type::Data::UnionTag::String:
            {
                String::Builder str = String::Builder(m_builder.GetBuilder()->AddList<char>(size));
                m_builder.GetPointerField(index).Set(str);
                return DynamicList::Builder(*m_info, type, str);
            }
            case Type::Data::UnionTag::List:
            {
                const Type::Data::List::Reader listType = typeData.GetList();
                const Type::Reader elementType = listType.GetElementType();
                const ElementSize elementSize = GetTypeElementSize(elementType);

                if (elementSize != ElementSize::Composite)
                {
                    ListBuilder list = m_builder.GetBuilder()->AddList(elementSize, size);
                    m_builder.GetPointerField(index).Set(list);
                    return DynamicList::Builder(*m_info, type, list);
                }

                const Type::Data::Struct::Reader structType = elementType.GetData().GetStruct();
                const DeclInfo* info = FindDependency(*m_info, structType.GetId());
                if (!HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a list of structs that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, structType.GetId())))
                {
                    return DynamicStruct::Builder{};
                }

                const Declaration::Reader decl = GetSchema(*info);
                const Declaration::Data::Struct::Reader fieldStructDecl = decl.GetData().GetStruct();
                ListBuilder list = m_builder.GetBuilder()->AddStructList(size, fieldStructDecl.GetDataFieldCount(), fieldStructDecl.GetDataWordSize(), fieldStructDecl.GetPointerCount());
                m_builder.GetPointerField(index).Set(list);
                return DynamicList::Builder(*m_info, type, list);
            }
            default:
                HE_VERIFY(false, HE_MSG("Init with a size is only valid for blob, string, and list fields"), HE_KV(field_type, typeData.GetUnionTag()));
                return DynamicStruct::Builder{};
        }
    }

    DynamicValue::Builder DynamicStruct::Builder::Init(StringView fieldName, uint32_t size)
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? Init(field, size) : DynamicValue::Builder{};
    }

    void DynamicStruct::Builder::Clear(Field::Reader field)
    {
        if (!field.IsValid())
            return;

        const Declaration::Data::Struct::Reader structDecl = StructSchema();

        const List<Field>::Reader fields = structDecl.GetFields();
        const auto it = std::find(fields.begin(), fields.end(), field);
        if (!HE_VERIFY(it != fields.end(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return;
        }

        SetInUnion(field);

        switch (field.GetMeta().GetUnionTag())
        {
            case Field::Meta::UnionTag::Normal:
            {
                const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                const Type::Reader type = norm.GetType();
                const uint16_t index = norm.GetIndex();

                if (IsPointer(type))
                    m_builder.ClearPointerField(index);
                else
                    m_builder.ClearDataField(index);
                break;
            }
            case Field::Meta::UnionTag::Group:
            {
                const Field::Meta::Group::Reader group = field.GetMeta().GetGroup();
                const DeclInfo* info = FindDependency(*m_info, group.GetTypeId());
                if (!HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a group that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, group.GetTypeId())))
                {
                    return;
                }

                DynamicStruct::Builder builder(*info, m_builder);
                for (const Field::Reader groupField : builder.StructSchema().GetFields())
                {
                    builder.Clear(groupField);
                }
            }
            case Field::Meta::UnionTag::Union:
            {
                const Field::Meta::Union::Reader group = field.GetMeta().GetUnion();
                const DeclInfo* info = FindDependency(*m_info, group.GetTypeId());
                if (!HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a union that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, group.GetTypeId())))
                {
                    return;
                }

                DynamicStruct::Builder builder(*info, m_builder);
                const Field::Reader unionField = builder.ActiveUnionField();
                builder.Clear(unionField);
            }
        }
    }

    void DynamicStruct::Builder::Clear(StringView fieldName)
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        if (HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName)))
        {
            Clear(field);
        }
    }

    bool DynamicStruct::Builder::Has(Field::Reader field) const
    {
        return AsReader().Has(field);
    }

    bool DynamicStruct::Builder::Has(StringView fieldName) const
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? Has(field) : false;
    }

    Field::Reader DynamicStruct::Builder::ActiveUnionField() const
    {
        const Declaration::Data::Struct::Reader structDecl = StructSchema();
        if (!structDecl.GetIsUnion())
            return {};

        const uint16_t tag = m_builder.GetDataField<uint16_t>(structDecl.GetUnionTagOffset());
        return FindFieldByUnionTag(tag, structDecl);
    }

    void DynamicStruct::Builder::SetInUnion(Field::Reader field)
    {
        const Declaration::Data::Struct::Reader structDecl = StructSchema();
        if (!structDecl.GetIsUnion())
            return;

        m_builder.SetDataField<uint16_t>(structDecl.GetUnionTagOffset(), field.GetUnionTag());
    }

    bool DynamicStruct::Builder::IsActiveInUnion(Field::Reader field) const
    {
        const Declaration::Reader decl = GetSchema(*m_info);

        if (!decl.IsValid() || !decl.GetData().IsStruct() || !decl.GetData().GetStruct().GetIsUnion())
            return false;

        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
        const uint16_t activeUnionTag = m_builder.GetDataField<uint16_t>(structDecl.GetUnionTagOffset());
        return field.GetUnionTag() == activeUnionTag;
    }

    // --------------------------------------------------------------------------------------------
    DynamicValue::Reader DynamicArray::Reader::Get(uint16_t index) const
    {
        if (!HE_VERIFY(index < Size()))
            return DynamicValue::Reader{};

        const Type::Reader elementType = ArrayType().GetElementType();
        const Type::Data::Reader elementTypeData = elementType.GetData();

        switch (elementTypeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: return Void{};
            case Type::Data::UnionTag::Bool: return static_cast<const bool*>(m_array)[index];
            case Type::Data::UnionTag::Int8: return static_cast<const int8_t*>(m_array)[index];
            case Type::Data::UnionTag::Int16: return static_cast<const int16_t*>(m_array)[index];
            case Type::Data::UnionTag::Int32: return static_cast<const int32_t*>(m_array)[index];
            case Type::Data::UnionTag::Int64: return static_cast<const int64_t*>(m_array)[index];
            case Type::Data::UnionTag::Uint8: return static_cast<const uint8_t*>(m_array)[index];
            case Type::Data::UnionTag::Uint16: return static_cast<const uint16_t*>(m_array)[index];
            case Type::Data::UnionTag::Uint32: return static_cast<const uint32_t*>(m_array)[index];
            case Type::Data::UnionTag::Uint64: return static_cast<const uint64_t*>(m_array)[index];
            case Type::Data::UnionTag::Float32: return static_cast<const float*>(m_array)[index];
            case Type::Data::UnionTag::Float64: return static_cast<const double*>(m_array)[index];
            case Type::Data::UnionTag::Blob: return m_list.GetPointerElement(index).TryGetBlob();
            case Type::Data::UnionTag::String: return m_list.GetPointerElement(index).TryGetString();
            case Type::Data::UnionTag::AnyPointer: return AnyPointer::Reader(m_list.GetPointerElement(index));
            case Type::Data::UnionTag::AnyStruct: return AnyStruct::Reader(m_list.GetPointerElement(index));
            case Type::Data::UnionTag::AnyList: return AnyList::Reader(m_list.GetPointerElement(index));
            case Type::Data::UnionTag::Array:
            {
                HE_VERIFY(false, HE_MSG("Arrays of arrays are not supported."));
                return DynamicValue::Reader{};
            }
            case Type::Data::UnionTag::List:
            {
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListReader list = m_list.GetPointerElement(index).TryGetList(elementSize);
                return DynamicList::Reader(*m_scope, elementType, list);
            }
            case Type::Data::UnionTag::Enum:
            {
                const Type::Data::Enum::Reader enumType = elementTypeData.GetEnum();
                const DeclInfo* info = FindDependency(*m_scope, enumType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is an enum that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, enumType.GetId()));
                const uint16_t value = static_cast<const uint16_t*>(m_array)[index];
                return valid ? DynamicEnum(*info, value) : DynamicEnum{};
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = elementTypeData.GetStruct();
                const DeclInfo* info = FindDependency(*m_scope, structType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, structType.GetId()));
                const StructReader reader = m_list.GetPointerElement(index).TryGetStruct();
                return valid ? DynamicStruct::Reader(*info, reader) : DynamicStruct::Reader{};
            }
            case Type::Data::UnionTag::Interface:
            {
                return DynamicValue::Reader{}; // Interfaces do not currently have values.
            }
            case Type::Data::UnionTag::Parameter:
            {
                return AnyPointer::Reader(m_list.GetPointerElement(index));
            }
        }

        return DynamicValue::Reader{};
    }

    DynamicValue::Reader DynamicArray::Reader::operator[](uint16_t index) const
    {
        return Get(index);
    }

    // --------------------------------------------------------------------------------------------
    DynamicValue::Builder DynamicArray::Builder::Get(uint16_t index) const
    {
        if (!HE_VERIFY(index < Size()))
            return DynamicValue::Builder{};

        const Type::Reader elementType = ArrayType().GetElementType();
        const Type::Data::Reader elementTypeData = elementType.GetData();

        switch (elementTypeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: return Void{};
            case Type::Data::UnionTag::Bool: return static_cast<const bool*>(m_array)[index];
            case Type::Data::UnionTag::Int8: return static_cast<const int8_t*>(m_array)[index];
            case Type::Data::UnionTag::Int16: return static_cast<const int16_t*>(m_array)[index];
            case Type::Data::UnionTag::Int32: return static_cast<const int32_t*>(m_array)[index];
            case Type::Data::UnionTag::Int64: return static_cast<const int64_t*>(m_array)[index];
            case Type::Data::UnionTag::Uint8: return static_cast<const uint8_t*>(m_array)[index];
            case Type::Data::UnionTag::Uint16: return static_cast<const uint16_t*>(m_array)[index];
            case Type::Data::UnionTag::Uint32: return static_cast<const uint32_t*>(m_array)[index];
            case Type::Data::UnionTag::Uint64: return static_cast<const uint64_t*>(m_array)[index];
            case Type::Data::UnionTag::Float32: return static_cast<const float*>(m_array)[index];
            case Type::Data::UnionTag::Float64: return static_cast<const double*>(m_array)[index];
            case Type::Data::UnionTag::Blob: return m_list.GetPointerElement(index).TryGetBlob();
            case Type::Data::UnionTag::String: return m_list.GetPointerElement(index).TryGetString();
            case Type::Data::UnionTag::AnyPointer: return AnyPointer::Builder(m_list.GetPointerElement(index));
            case Type::Data::UnionTag::AnyStruct: return AnyStruct::Builder(m_list.GetPointerElement(index));
            case Type::Data::UnionTag::AnyList: return AnyList::Builder(m_list.GetPointerElement(index));
            case Type::Data::UnionTag::Array:
            {
                HE_VERIFY(false, HE_MSG("Arrays of arrays are not supported."));
                return DynamicValue::Builder{};
            }
            case Type::Data::UnionTag::List:
            {
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListBuilder list = m_list.GetPointerElement(index).TryGetList(elementSize);
                return DynamicList::Builder(*m_scope, elementType, list);
            }
            case Type::Data::UnionTag::Enum:
            {
                const Type::Data::Enum::Reader enumType = elementTypeData.GetEnum();
                const DeclInfo* info = FindDependency(*m_scope, enumType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is an enum that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, enumType.GetId()));
                const uint16_t value = static_cast<const uint16_t*>(m_array)[index];
                return valid ? DynamicEnum(*info, value) : DynamicEnum{};
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = elementTypeData.GetStruct();
                const DeclInfo* info = FindDependency(*m_scope, structType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, structType.GetId()));
                const StructBuilder builder = m_list.GetPointerElement(index).TryGetStruct();
                return valid ? DynamicStruct::Builder(*info, builder) : DynamicStruct::Builder{};
            }
            case Type::Data::UnionTag::Interface:
            {
                return DynamicValue::Builder{}; // Interfaces do not currently have values.
            }
            case Type::Data::UnionTag::Parameter:
            {
                return AnyPointer::Builder(m_list.GetPointerElement(index));
            }
        }

        return DynamicValue::Builder{};
    }

    DynamicValue::Builder DynamicArray::Builder::operator[](uint16_t index) const
    {
        return Get(index);
    }

    // --------------------------------------------------------------------------------------------
    DynamicValue::Reader DynamicList::Reader::Get(uint32_t index) const
    {
        if (!HE_VERIFY(index < Size()))
            return DynamicValue::Reader{};

        const Type::Reader elementType = ListType().GetElementType();
        const Type::Data::Reader elementTypeData = elementType.GetData();

        switch (elementTypeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: return m_reader.GetDataElement<Void>(index);
            case Type::Data::UnionTag::Bool: return m_reader.GetDataElement<bool>(index);
            case Type::Data::UnionTag::Int8: return m_reader.GetDataElement<int8_t>(index);
            case Type::Data::UnionTag::Int16: return m_reader.GetDataElement<int16_t>(index);
            case Type::Data::UnionTag::Int32: return m_reader.GetDataElement<int32_t>(index);
            case Type::Data::UnionTag::Int64: return m_reader.GetDataElement<int64_t>(index);
            case Type::Data::UnionTag::Uint8: return m_reader.GetDataElement<uint8_t>(index);
            case Type::Data::UnionTag::Uint16: return m_reader.GetDataElement<uint16_t>(index);
            case Type::Data::UnionTag::Uint32: return m_reader.GetDataElement<uint32_t>(index);
            case Type::Data::UnionTag::Uint64: return m_reader.GetDataElement<uint64_t>(index);
            case Type::Data::UnionTag::Float32: return m_reader.GetDataElement<float>(index);
            case Type::Data::UnionTag::Float64: return m_reader.GetDataElement<double>(index);
            case Type::Data::UnionTag::Blob: return m_reader.GetPointerElement(index).TryGetBlob();
            case Type::Data::UnionTag::String: return m_reader.GetPointerElement(index).TryGetString();
            case Type::Data::UnionTag::AnyPointer: return AnyPointer::Reader(m_reader.GetPointerElement(index));
            case Type::Data::UnionTag::AnyStruct: return AnyStruct::Reader(m_reader.GetPointerElement(index));
            case Type::Data::UnionTag::AnyList: return AnyList::Reader(m_reader.GetPointerElement(index));
            case Type::Data::UnionTag::Array:
            {
                HE_VERIFY(false, HE_MSG("Lists of arrays are not supported."));
                return DynamicValue::Reader{};
            }
            case Type::Data::UnionTag::List:
            {
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListReader list = m_reader.GetPointerElement(index).TryGetList(elementSize);
                return DynamicList::Reader(*m_scope, elementType, list);
            }
            case Type::Data::UnionTag::Enum:
            {
                const Type::Data::Enum::Reader enumType = elementTypeData.GetEnum();
                const DeclInfo* info = FindDependency(*m_scope, enumType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is an enum that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, enumType.GetId()));
                const uint16_t value = m_reader.GetDataElement<uint16_t>(index);
                return valid ? DynamicEnum(*info, value) : DynamicEnum{};
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = elementTypeData.GetStruct();
                const DeclInfo* info = FindDependency(*m_scope, structType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, structType.GetId()));
                const StructReader reader = m_reader.GetCompositeElement(index);
                return valid ? DynamicStruct::Reader(*info, reader) : DynamicStruct::Reader{};
            }
            case Type::Data::UnionTag::Interface:
            {
                return DynamicValue::Reader{}; // Interfaces do not currently have values.
            }
            case Type::Data::UnionTag::Parameter:
            {
                return AnyPointer::Reader(m_reader.GetPointerElement(index));
            }
        }

        return DynamicValue::Reader{};
    }

    DynamicValue::Reader DynamicList::Reader::operator[](uint32_t index) const
    {
        return Get(index);
    }

    // --------------------------------------------------------------------------------------------
    DynamicValue::Builder DynamicList::Builder::Get(uint32_t index) const
    {
        if (!HE_VERIFY(index < Size()))
            return DynamicValue::Builder{};

        const Type::Reader elementType = ListType().GetElementType();
        const Type::Data::Reader elementTypeData = elementType.GetData();

        switch (elementTypeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: return m_builder.GetDataElement<Void>(index);
            case Type::Data::UnionTag::Bool: return m_builder.GetDataElement<bool>(index);
            case Type::Data::UnionTag::Int8: return m_builder.GetDataElement<int8_t>(index);
            case Type::Data::UnionTag::Int16: return m_builder.GetDataElement<int16_t>(index);
            case Type::Data::UnionTag::Int32: return m_builder.GetDataElement<int32_t>(index);
            case Type::Data::UnionTag::Int64: return m_builder.GetDataElement<int64_t>(index);
            case Type::Data::UnionTag::Uint8: return m_builder.GetDataElement<uint8_t>(index);
            case Type::Data::UnionTag::Uint16: return m_builder.GetDataElement<uint16_t>(index);
            case Type::Data::UnionTag::Uint32: return m_builder.GetDataElement<uint32_t>(index);
            case Type::Data::UnionTag::Uint64: return m_builder.GetDataElement<uint64_t>(index);
            case Type::Data::UnionTag::Float32: return m_builder.GetDataElement<float>(index);
            case Type::Data::UnionTag::Float64: return m_builder.GetDataElement<double>(index);
            case Type::Data::UnionTag::Blob: return m_builder.GetPointerElement(index).TryGetBlob();
            case Type::Data::UnionTag::String: return m_builder.GetPointerElement(index).TryGetString();
            case Type::Data::UnionTag::AnyPointer: return AnyPointer::Builder(m_builder.GetPointerElement(index));
            case Type::Data::UnionTag::AnyStruct: return AnyStruct::Builder(m_builder.GetPointerElement(index));
            case Type::Data::UnionTag::AnyList: return AnyList::Builder(m_builder.GetPointerElement(index));
            case Type::Data::UnionTag::Array:
            {
                HE_VERIFY(false, HE_MSG("Lists of arrays are not supported."));
                return DynamicValue::Builder{};
            }
            case Type::Data::UnionTag::List:
            {
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListBuilder list = m_builder.GetPointerElement(index).TryGetList(elementSize);
                return DynamicList::Builder(*m_scope, elementType, list);
            }
            case Type::Data::UnionTag::Enum:
            {
                const Type::Data::Enum::Reader enumType = elementTypeData.GetEnum();
                const DeclInfo* info = FindDependency(*m_scope, enumType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is an enum that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, enumType.GetId()));
                const uint16_t value = m_builder.GetDataElement<uint16_t>(index);
                return valid ? DynamicEnum(*info, value) : DynamicEnum{};
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = elementTypeData.GetStruct();
                const DeclInfo* info = FindDependency(*m_scope, structType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, structType.GetId()));
                const StructBuilder builder = m_builder.GetCompositeElement(index);
                return valid ? DynamicStruct::Builder(*info, builder) : DynamicStruct::Builder{};
            }
            case Type::Data::UnionTag::Interface:
            {
                return DynamicValue::Builder{}; // Interfaces do not currently have values.
            }
            case Type::Data::UnionTag::Parameter:
            {
                return AnyPointer::Builder(m_builder.GetPointerElement(index));
            }
        }

        return DynamicValue::Builder{};
    }

    void DynamicList::Builder::Set(uint32_t index, const DynamicValue::Reader& value)
    {
        if (!HE_VERIFY(index < Size()))
            return;

        const Type::Data::List::Reader listType = m_type.GetData().GetList();
        const Type::Reader elementType = listType.GetElementType();
        const Type::Data::Reader elementTypeData = elementType.GetData();

        switch (elementTypeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: m_builder.SetDataElement(index, value.As<Void>()); break;
            case Type::Data::UnionTag::Bool: m_builder.SetDataElement(index, value.As<bool>()); break;
            case Type::Data::UnionTag::Int8: m_builder.SetDataElement(index, value.As<int8_t>()); break;
            case Type::Data::UnionTag::Int16: m_builder.SetDataElement(index, value.As<int16_t>()); break;
            case Type::Data::UnionTag::Int32: m_builder.SetDataElement(index, value.As<int32_t>()); break;
            case Type::Data::UnionTag::Int64: m_builder.SetDataElement(index, value.As<int64_t>()); break;
            case Type::Data::UnionTag::Uint8: m_builder.SetDataElement(index, value.As<uint8_t>()); break;
            case Type::Data::UnionTag::Uint16: m_builder.SetDataElement(index, value.As<uint16_t>()); break;
            case Type::Data::UnionTag::Uint32: m_builder.SetDataElement(index, value.As<uint32_t>()); break;
            case Type::Data::UnionTag::Uint64: m_builder.SetDataElement(index, value.As<uint64_t>()); break;
            case Type::Data::UnionTag::Float32: m_builder.SetDataElement(index, value.As<float>()); break;
            case Type::Data::UnionTag::Float64: m_builder.SetDataElement(index, value.As<double>()); break;
            case Type::Data::UnionTag::Blob: m_builder.GetPointerElement(index).Set(value.As<Blob>()); break;
            case Type::Data::UnionTag::String: m_builder.GetPointerElement(index).Set(value.As<String>()); break;
            case Type::Data::UnionTag::AnyPointer:
            case Type::Data::UnionTag::AnyStruct:
            case Type::Data::UnionTag::AnyList:
            case Type::Data::UnionTag::Parameter:
            {
                // TODO: Support these better
                HE_VERIFY(false, HE_MSG("Lists of AnyPointer types are not supported by DynamicList::Builder::Set(), yet."));
                break;
            }
            case Type::Data::UnionTag::Array:
            {
                HE_VERIFY(false, HE_MSG("Lists of arrays are not supported."));
                break;;
            }
            case Type::Data::UnionTag::List:
            {
                const DynamicList::Reader src = value.As<DynamicList>();

                if (!HE_VERIFY(src.Type() == elementType))
                    return;

                m_builder.GetPointerElement(index).Set(src.List());
                break;
            }
            case Type::Data::UnionTag::Enum:
            {
                const uint16_t enumValue = GetEnumValue(*m_scope, elementType, value);
                m_builder.SetDataElement(index, enumValue);
                break;
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = elementTypeData.GetStruct();
                const DeclInfo* info = FindDependency(*m_scope, structType.GetId());
                if (!HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, structType.GetId())))
                {
                    return;
                }

                const DynamicStruct::Reader src = value.As<DynamicStruct>();

                if (HE_VERIFY(&src.Decl() == info))
                {
                    m_builder.GetCompositeElement(index).Copy(src.Struct());
                }
                break;
            }
            case Type::Data::UnionTag::Interface:
            {
                // Interfaces do not currently have values.
                break;
            }
        }
    }

    DynamicValue::Builder DynamicList::Builder::Init(uint32_t index, uint32_t size)
    {
        if (!HE_VERIFY(index < Size()))
            return DynamicValue::Builder{};

        const Type::Data::List::Reader listType = m_type.GetData().GetList();
        const Type::Reader elementType = listType.GetElementType();
        const Type::Data::Reader elementTypeData = elementType.GetData();

        switch (elementTypeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Blob:
            {
                Blob::Builder blob = m_builder.GetBuilder()->AddList<uint8_t>(size);
                m_builder.GetPointerElement(index).Set(blob);
                return DynamicList::Builder(*m_scope, elementType, blob);
            }
            case Type::Data::UnionTag::String:
            {
                String::Builder str = String::Builder(m_builder.GetBuilder()->AddList<char>(size));
                m_builder.GetPointerElement(index).Set(str);
                return DynamicList::Builder(*m_scope, elementType, str);
            }
            case Type::Data::UnionTag::List:
            {
                const Type::Data::List::Reader subListType = elementTypeData.GetList();
                const Type::Reader subElementType = subListType.GetElementType();
                const ElementSize subElementSize = GetTypeElementSize(subElementType);

                if (subElementSize != ElementSize::Composite)
                {
                    ListBuilder list = m_builder.GetBuilder()->AddList(subElementSize, size);
                    m_builder.GetPointerElement(index).Set(list);
                    return DynamicList::Builder(*m_scope, subElementType, list);
                }

                const Type::Data::Struct::Reader structType = subElementType.GetData().GetStruct();
                const DeclInfo* info = FindDependency(*m_scope, structType.GetId());
                if (!HE_VERIFY(info,
                    HE_MSG("Element requested from DynamicList is a list of structs that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_scope).GetName()),
                    HE_KV(requested_type_id, structType.GetId())))
                {
                    return DynamicStruct::Builder{};
                }

                const Declaration::Reader decl = GetSchema(*info);
                const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
                ListBuilder list = m_builder.GetBuilder()->AddStructList(size, structDecl.GetDataFieldCount(), structDecl.GetDataWordSize(), structDecl.GetPointerCount());
                m_builder.GetPointerElement(index).Set(list);
                return DynamicList::Builder(*m_scope, subElementType, list);
            }
            default:
                HE_VERIFY(false, HE_MSG("Init with a size is only valid for blob, string, and list elements"), HE_KV(element_type, elementTypeData.GetUnionTag()));
                return DynamicStruct::Builder{};
        }
    }

    DynamicValue::Builder DynamicList::Builder::operator[](uint32_t index) const
    {
        return Get(index);
    }
}

namespace he
{
    template <>
    const char* AsString(schema::DynamicValue::Kind x)
    {
        switch (x)
        {
            case schema::DynamicValue::Kind::Unknown: return "Unknown";
            case schema::DynamicValue::Kind::Void: return "Void";
            case schema::DynamicValue::Kind::Bool: return "Bool";
            case schema::DynamicValue::Kind::Int: return "Int";
            case schema::DynamicValue::Kind::Uint: return "Uint";
            case schema::DynamicValue::Kind::Float: return "Float";
            case schema::DynamicValue::Kind::Blob: return "Blob";
            case schema::DynamicValue::Kind::String: return "String";
            case schema::DynamicValue::Kind::Array: return "Array";
            case schema::DynamicValue::Kind::List: return "List";
            case schema::DynamicValue::Kind::Enum: return "Enum";
            case schema::DynamicValue::Kind::Struct: return "Struct";
            case schema::DynamicValue::Kind::AnyPointer: return "AnyPointer";
        }
        return "<unknown>";
    }
}
