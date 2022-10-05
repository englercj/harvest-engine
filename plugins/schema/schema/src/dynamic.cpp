// Copyright Chad Engler

// TODO: This will need to be updated if/when we support anonymous unions

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

    // --------------------------------------------------------------------------------------------
    DynamicValue::Reader DynamicStruct::Reader::GetField(Field::Reader field) const
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
                        // TODO: DynamicArray
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
                const Field::Meta::Group::Reader group = field.GetMeta().GetGroup();
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
                const Field::Meta::Union::Reader group = field.GetMeta().GetUnion();
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

    DynamicValue::Reader DynamicStruct::Reader::GetField(StringView fieldName) const
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? GetField(field) : DynamicValue::Reader{};
    }

    bool DynamicStruct::Reader::HasField(Field::Reader field) const
    {
        if (!field.IsValid())
            return false;

        const List<Field>::Reader fields = StructSchema().GetFields();
        const auto it = std::find(fields.begin(), fields.end(), field);
        if (!HE_VERIFY(it != fields.end(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, field.GetName())))
        {
            return false;
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
            {
                // groups are always available
                return true;
            }
            case Field::Meta::UnionTag::Union:
            {
                // TODO: Support 'unset' unions by storing a bitset index for the union tag

                const Field::Meta::Union::Reader group = meta.GetUnion();
                const DeclInfo* info = FindDependency(*m_info, group.GetTypeId());
                const Declaration::Reader decl = info ? GetSchema(*info) : Declaration::Reader{};
                const bool valid = HE_VERIFY(decl.IsValid() && decl.GetData().IsStruct() && decl.GetData().GetStruct().GetIsUnion(),
                    HE_MSG("Field requested from DynamicStruct is a union that has a missing type."),
                    HE_KV(struct_name, Schema().GetName()),
                    HE_KV(requested_field_name, field.GetName()),
                    HE_KV(requested_field_type_id, group.GetTypeId()));

                if (!valid)
                    return false;

                return IsActiveInUnion(*info, field);
            }
        }

        return false;
    }

    bool DynamicStruct::Reader::HasField(StringView fieldName) const
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? HasField(field) : false;
    }

    bool DynamicStruct::Reader::IsActiveInUnion(Field::Reader field) const
    {
        return IsActiveInUnion(*m_info, field);
    }

    bool DynamicStruct::Reader::IsActiveInUnion(const DeclInfo& info, Field::Reader field) const
    {
        const Declaration::Reader decl = GetSchema(info);

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
    DynamicValue::Builder DynamicStruct::Builder::GetField(Field::Reader field) const
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
                        // TODO: DynamicArray
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

    DynamicValue::Builder DynamicStruct::Builder::GetField(StringView fieldName) const
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? GetField(field) : DynamicValue::Builder{};
    }

    void DynamicStruct::Builder::SetField(Field::Reader field, const DynamicValue::Reader& value)
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
                        // TODO: DynamicArray
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
                DynamicStruct::Builder dst = InitField(field).As<DynamicStruct>();

                if (!HE_VERIFY(&src.Decl() == &dst.Decl()))
                    return;

                for (const Field::Reader field : src.StructSchema().GetFields())
                {
                    if (src.HasField(field))
                    {
                        dst.SetField(field, src.GetField(field));
                    }
                }
                break;
            }
            case Field::Meta::UnionTag::Union:
            {
                const DynamicStruct::Reader src = value.As<DynamicStruct>();
                DynamicStruct::Builder dst = InitField(field).As<DynamicStruct>();

                if (!HE_VERIFY(&src.Decl() == &dst.Decl()))
                    return;

                const Field::Reader unionField = src.ActiveUnionField();
                if (HE_VERIFY(unionField.IsValid()))
                {
                    dst.SetField(unionField, src.GetField(unionField));
                }
                break;
            }
        }
    }

    void DynamicStruct::Builder::SetField(StringView fieldName, const DynamicValue::Reader& value)
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        if (HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName)))
        {
            SetField(field, value);
        }
    }

    DynamicValue::Builder DynamicStruct::Builder::InitField(Field::Reader field)
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
                        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
                        const StructBuilder builder = m_builder.GetBuilder()->AddStruct(structDecl.GetDataFieldCount(), structDecl.GetDataWordSize(), structDecl.GetPointerCount());
                        m_builder.GetPointerField(index).Set(builder);
                        return DynamicStruct::Builder(*info, builder);
                    }
                    default:
                        HE_VERIFY(false, HE_MSG("InitField without a size is only valid for struct fields"), HE_KV(field_type, typeData.GetUnionTag()));
                        return DynamicStruct::Builder{};
                }

                break;
            }
            case Field::Meta::UnionTag::Group:
            {
                ClearField(field);
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
                ClearField(field);
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
    }

    DynamicValue::Builder DynamicStruct::Builder::InitField(StringView fieldName)
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? InitField(field) : DynamicValue::Builder{};
    }

    DynamicValue::Builder DynamicStruct::Builder::InitField(Field::Reader field, uint32_t size)
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
            HE_MSG("InitField with a size is only valid for blob, string, and list fields"),
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
                const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
                ListBuilder list = m_builder.GetBuilder()->AddStructList(size, structDecl.GetDataFieldCount(), structDecl.GetDataWordSize(), structDecl.GetPointerCount());
                m_builder.GetPointerField(index).Set(list);
                return DynamicList::Builder(*m_info, type, list);
            }
            default:
                HE_VERIFY(false, HE_MSG("InitField with a size is only valid for blob, string, and list fields"), HE_KV(field_type, typeData.GetUnionTag()));
                return DynamicStruct::Builder{};
        }
    }

    DynamicValue::Builder DynamicStruct::Builder::InitField(StringView fieldName, uint32_t size)
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? InitField(field, size) : DynamicValue::Builder{};
    }

    void DynamicStruct::Builder::ClearField(Field::Reader field)
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
                for (const Field::Reader field : builder.StructSchema().GetFields())
                {
                    builder.ClearField(field);
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
                builder.ClearField(unionField);
            }
        }
    }

    void DynamicStruct::Builder::ClearField(StringView fieldName)
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        if (HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName)))
        {
            ClearField(field);
        }
    }

    bool DynamicStruct::Builder::HasField(Field::Reader field) const
    {
        return AsReader().HasField(field);
    }

    bool DynamicStruct::Builder::HasField(StringView fieldName) const
    {
        const Field::Reader field = FindFieldByName(fieldName, StructSchema());
        const bool valid = HE_VERIFY(field.IsValid(),
            HE_MSG("Field requested from DynamicStruct that is not a member of the schema."),
            HE_KV(struct_name, Schema().GetName()),
            HE_KV(requested_field_name, fieldName));
        return valid ? HasField(field) : false;
    }

    bool DynamicStruct::Builder::IsActiveInUnion(Field::Reader field) const
    {
        return IsActiveInUnion(*m_info, field);
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

    bool DynamicStruct::Builder::IsActiveInUnion(const DeclInfo& info, Field::Reader field) const
    {
        const Declaration::Reader decl = GetSchema(info);

        if (!decl.IsValid() || !decl.GetData().IsStruct() || !decl.GetData().GetStruct().GetIsUnion())
            return false;

        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
        const uint16_t activeUnionTag = m_builder.GetDataField<uint16_t>(structDecl.GetUnionTagOffset());
        return field.GetUnionTag() == activeUnionTag;
    }

    // --------------------------------------------------------------------------------------------
    DynamicValue::Reader DynamicList::Reader::Get(uint32_t index) const
    {
        if (!HE_VERIFY(index < Size()))
            return DynamicValue::Reader{};

        const Type::Data::List::Reader listType = m_type.GetData().GetList();
        const Type::Reader elementType = listType.GetElementType();

        switch (elementType.GetData().GetUnionTag())
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
                // TODO: DynamicArray
                break;
            }
            case Type::Data::UnionTag::List:
            {
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListReader list = m_reader.GetPointerElement(index).TryGetList(elementSize);
                return DynamicList::Reader(*m_parentInfo, elementType, list);
            }
            case Type::Data::UnionTag::Enum:
            {
                const Type::Data::Enum::Reader enumType = elementType.GetData().GetEnum();
                const DeclInfo* info = FindDependency(*m_parentInfo, enumType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is an enum that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_parentInfo).GetName()),
                    HE_KV(requested_type_id, enumType.GetId()));
                const uint16_t value = m_reader.GetDataElement<uint16_t>(index);
                return valid ? DynamicEnum(*info, value) : DynamicEnum{};
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = elementType.GetData().GetStruct();
                const DeclInfo* info = FindDependency(*m_parentInfo, structType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_parentInfo).GetName()),
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

    // --------------------------------------------------------------------------------------------
    DynamicValue::Builder DynamicList::Builder::Get(uint32_t index) const
    {
        if (!HE_VERIFY(index < Size()))
            return DynamicValue::Builder{};

        const Type::Data::List::Reader listType = m_type.GetData().GetList();
        const Type::Reader elementType = listType.GetElementType();

        switch (elementType.GetData().GetUnionTag())
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
                // TODO: DynamicArray
                break;
            }
            case Type::Data::UnionTag::List:
            {
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListBuilder list = m_builder.GetPointerElement(index).TryGetList(elementSize);
                return DynamicList::Builder(*m_parentInfo, elementType, list);
            }
            case Type::Data::UnionTag::Enum:
            {
                const Type::Data::Enum::Reader enumType = elementType.GetData().GetEnum();
                const DeclInfo* info = FindDependency(*m_parentInfo, enumType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is an enum that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_parentInfo).GetName()),
                    HE_KV(requested_type_id, enumType.GetId()));
                const uint16_t value = m_builder.GetDataElement<uint16_t>(index);
                return valid ? DynamicEnum(*info, value) : DynamicEnum{};
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = elementType.GetData().GetStruct();
                const DeclInfo* info = FindDependency(*m_parentInfo, structType.GetId());
                const bool valid = HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_parentInfo).GetName()),
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

        switch (elementType.GetData().GetUnionTag())
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
                HE_LOGF_ERROR(he_schema, "Lists of AnyPointer types are not supported by DynamicList::Builder::Set()");
                break;
            }
            case Type::Data::UnionTag::Array:
            {
                // TODO: DynamicArray
                break;
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
                const uint16_t enumValue = GetEnumValue(*m_parentInfo, elementType, value);
                m_builder.SetDataElement(index, enumValue);
                break;
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = elementType.GetData().GetStruct();
                const DeclInfo* info = FindDependency(*m_parentInfo, structType.GetId());
                if (!HE_VERIFY(info,
                    HE_MSG("Field requested from DynamicStruct is a struct that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_parentInfo).GetName()),
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
            return;

        const Type::Data::List::Reader listType = m_type.GetData().GetList();
        const Type::Reader elementType = listType.GetElementType();
        const Type::Data::Reader elementTypeData = elementType.GetData();

        switch (elementTypeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Blob:
            {
                Blob::Builder blob = m_builder.GetBuilder()->AddList<uint8_t>(size);
                m_builder.GetPointerElement(index).Set(blob);
                return DynamicList::Builder(*m_parentInfo, elementType, blob);
            }
            case Type::Data::UnionTag::String:
            {
                String::Builder str = String::Builder(m_builder.GetBuilder()->AddList<char>(size));
                m_builder.GetPointerElement(index).Set(str);
                return DynamicList::Builder(*m_parentInfo, elementType, str);
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
                    return DynamicList::Builder(*m_parentInfo, subElementType, list);
                }

                const Type::Data::Struct::Reader structType = subElementType.GetData().GetStruct();
                const DeclInfo* info = FindDependency(*m_parentInfo, structType.GetId());
                if (!HE_VERIFY(info,
                    HE_MSG("Element requested from DynamicList is a list of structs that has a missing type."),
                    HE_KV(struct_name, GetSchema(*m_parentInfo).GetName()),
                    HE_KV(requested_type_id, structType.GetId())))
                {
                    return DynamicStruct::Builder{};
                }

                const Declaration::Reader decl = GetSchema(*info);
                const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
                ListBuilder list = m_builder.GetBuilder()->AddStructList(size, structDecl.GetDataFieldCount(), structDecl.GetDataWordSize(), structDecl.GetPointerCount());
                m_builder.GetPointerElement(index).Set(list);
                return DynamicList::Builder(*m_parentInfo, subElementType, list);
            }
            default:
                HE_VERIFY(false, HE_MSG("Init with a size is only valid for blob, string, and list elements"), HE_KV(element_type, elementTypeData.GetUnionTag()));
                return DynamicStruct::Builder{};
        }
    }

    // --------------------------------------------------------------------------------------------
    #define HE_DYNAMIC_VALUE_CLASS_AS_TYPE(member, kind, Class, Type) \
        template <> typename TypeHelper<Type>::Class DynamicValue::Class::As<Type>() const \
        { \
            const bool valid = HE_VERIFY(m_kind == Kind::kind, \
                HE_MSG("DynamicValue requested as a type it doesn't hold."), \
                HE_KV(requested_type_name, TypeInfo::Get<Type>().Name()), \
                HE_KV(actual_kind, m_kind)); \
            return valid ? member : typename TypeHelper<Type>::Class{}; \
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
        return value;
    }

    template <>
    uint64_t CoerceSignedToUnsigned<uint64_t>(long long value)
    {
        HE_VERIFY(value >= 0,
            HE_MSG("Value out of range for requested type."),
            HE_KV(requested_type, TypeInfo::Get<uint64_t>().Name()),
            HE_KV(value, value));
        return value;
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
    T ImplicitCast(U&& value) { return Forward<U>(value); }

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
            case schema::DynamicValue::Kind::List: return "List";
            case schema::DynamicValue::Kind::Enum: return "Enum";
            case schema::DynamicValue::Kind::Struct: return "Struct";
            case schema::DynamicValue::Kind::AnyPointer: return "AnyPointer";
        }
        return "<unknown>";
    }
}
