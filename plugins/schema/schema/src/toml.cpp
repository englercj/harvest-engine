// Copyright Chad Engler

#include "he/schema/toml.h"

#include "he/core/assert.h"
#include "he/core/enum_fmt.h"
#include "he/core/log.h"
#include "he/core/span_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_builder.h"
#include "he/core/string_view_fmt.h"

#include "toml++/toml.h"

namespace he::schema
{
    template <typename T> struct TomlValueHelper;
    template <> struct TomlValueHelper<bool> { static bool Get(const toml::node& value) { return value.as_boolean()->get(); } };
    template <> struct TomlValueHelper<uint64_t> { static uint64_t Get(const toml::node& value) { return ::he::String::ToInteger<uint64_t>(value.as_string()->get().c_str(), nullptr, 16); } };
    template <std::integral T> struct TomlValueHelper<T> { static T Get(const toml::node& value) { return static_cast<T>(value.as_integer()->get()); } };
    template <std::floating_point T> struct TomlValueHelper<T> { static T Get(const toml::node& value) { return static_cast<T>(value.as_floating_point()->get()); } };

    template <typename ReaderType>
    struct SchemaValueHelper;

    template <>
    struct SchemaValueHelper<StructReader>
    {
        template <typename T>
        static T GetData(const StructReader& data, uint32_t index, uint32_t dataOffset)
        {
            HE_UNUSED(index);
            return data.GetDataField<T>(dataOffset);
        }

        static PointerReader GetPointer(const StructReader& data, uint32_t index)
        {
            return data.GetPointerField(static_cast<uint16_t>(index));
        }

        static StructReader GetComposite(const StructReader& data, uint32_t index)
        {
            return data.GetPointerField(static_cast<uint16_t>(index)).TryGetStruct();
        }
    };

    template <>
    struct SchemaValueHelper<ListReader>
    {
        template <typename T>
        static T GetData(const ListReader& data, uint32_t index, uint32_t dataOffset)
        {
            HE_UNUSED(dataOffset);
            return data.GetDataElement<T>(index);
        }

        static PointerReader GetPointer(const ListReader& data, uint32_t index)
        {
            return data.GetPointerElement(index);
        }

        static StructReader GetComposite(const ListReader& data, uint32_t index)
        {
            return data.GetCompositeElement(index);
        }
    };

    static const char* GetNodeTypeString(toml::node_type t)
    {
        switch (t)
        {
            case toml::node_type::none: return "none";
            case toml::node_type::table: return "table";
            case toml::node_type::array: return "array";
            case toml::node_type::string: return "string";
            case toml::node_type::integer: return "integer";
            case toml::node_type::floating_point: return "floating_point";
            case toml::node_type::boolean: return "boolean";
            case toml::node_type::date: return "date";
            case toml::node_type::time: return "time";
            case toml::node_type::date_time: return "date_time";
        }

        return "<unknown>";
    }

    static bool ParseBlobString(Vector<uint8_t>& out, Field::Reader field, const toml::node& value)
    {
        HE_ASSERT(value.is_string(), HE_KV(value_type, GetNodeTypeString(value.type())));

        const std::string& str = value.as_string()->get();
        const char* begin = str.data();
        const char* end = begin + str.size();

        // Reserve the maximum possible byte size. Because we allow spaces this may over-allocate
        // but likely not by much, and that's better than multiple reallocs in the loop.
        const uint32_t len = static_cast<uint32_t>(end - begin);
        out.Reserve(len / 2);

        const char* s = begin;

        char first = 0;
        while (s < end)
        {
            const char c = *s++;

            if (c == ' ')
                continue;

            if (!HE_VERIFY(IsHex(c)))
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Encountered invalid character for blob value. Skipping deserialization of field."),
                    HE_KV(field_name, field.GetName().AsView()),
                    HE_KV(field_type, field.GetMeta().GetNormal().GetType().GetData().GetTag()),
                    HE_KV(toml_type, GetNodeTypeString(value.type())),
                    HE_KV(bad_char, c),
                    HE_KV(char_offset, (s - begin)));
                return false;
            }

            if (first == 0)
            {
                first = c;
            }
            else
            {
                const uint8_t byte = HexPairToByte(first, c);
                out.PushBack(byte);
                first = 0;
            }
        }

        if (first != 0)
        {
            HE_LOG_ERROR(he_schema,
                HE_MSG("Encountered invalid blob hex string, there is a trailing nibble. Skipping deserialization of field."),
                HE_KV(field_name, field.GetName().AsView()),
                HE_KV(field_type, field.GetMeta().GetNormal().GetType().GetData().GetTag()),
                HE_KV(toml_type, GetNodeTypeString(value.type())));
            return false;
        }

        return true;
    }

    static bool CheckTypeMatch(const Type::Data::Tag typeDataTag, const toml::node& value)
    {
        switch (typeDataTag)
        {
            case Type::Data::Tag::Void: return false;
            case Type::Data::Tag::Bool: return value.is_boolean();
            case Type::Data::Tag::Int8: return value.is_integer();
            case Type::Data::Tag::Int16: return value.is_integer();
            case Type::Data::Tag::Int32: return value.is_integer();
            case Type::Data::Tag::Int64: return value.is_integer();
            case Type::Data::Tag::Uint8: return value.is_integer();
            case Type::Data::Tag::Uint16: return value.is_integer();
            case Type::Data::Tag::Uint32: return value.is_integer();
            case Type::Data::Tag::Uint64: return value.is_string();
            case Type::Data::Tag::Float32: return value.is_floating_point();
            case Type::Data::Tag::Float64: return value.is_floating_point();
            case Type::Data::Tag::Array: return value.is_array();
            case Type::Data::Tag::Blob: return value.is_string();
            case Type::Data::Tag::String: return value.is_string();
            case Type::Data::Tag::List: return value.is_array();
            case Type::Data::Tag::Enum: return value.is_string();
            case Type::Data::Tag::Struct: return value.is_table();
            case Type::Data::Tag::Interface: return false;
            case Type::Data::Tag::AnyPointer: return false;
        }
        return false;
    }

    // --------------------------------------------------------------------------------------------
    // TODO: Remove toml++ usage and just parse the toml text directly. Format is simple enough and
    // we really don't need 75% of what toml++ provides. It would also make error detection and
    // handling much easier. Additionally, toml++ doesn't support uint64 and I want it so bad.
    class TomlReader
    {
    public:
        TomlReader(Builder& dst)
            : m_dst(dst)
        {}

        bool Read(const char* data, const DeclInfo& info)
        {
            const toml::parse_result& result = toml::parse(data);
            if (!result)
            {
                const toml::parse_error& err = result.error();
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Failed to parse TOML string."),
                    HE_KV(error, err.description()),
                    HE_KV(line, err.source().begin.line),
                    HE_KV(column, err.source().begin.column));
                 return false;
            }

            PushGroup(&info);
            SetStruct(result.table());
            m_dst.SetRoot(m_stack.Back().builder);
            PopGroup();

            return true;
        }

    private:
        bool SetStruct(const toml::table& table)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Declaration::Data::Struct::Reader st = decl.GetData().GetStruct();

            for (auto&& [key, value] : table)
            {
                const Field::Reader field = FindField(key.str(), st);

                if (field.IsValid())
                {
                    SetField(field, value);
                }
                else
                {
                    // TODO: Handle unknown fields
                    HE_LOG_WARN(he_schema,
                        HE_MSG("Encountered unknown field name. Skipping deserialization of field."),
                        HE_KV(field_name, key.str()),
                        HE_KV(decl_name, decl.GetName()),
                        HE_KV(decl_id, decl.GetId()));
                }
            }

            return true;
        }

        void SetField(Field::Reader field, const toml::node& value)
        {
            switch (field.GetMeta().GetTag())
            {
                case Field::Meta::Tag::Normal: SetNormalField(field, value); break;
                case Field::Meta::Tag::Group: SetGroupField(field, value); break;
                case Field::Meta::Tag::Union: SetUnionField(field, value); break;
            }
        }

        void SetNormalField(Field::Reader field, const toml::node& value)
        {
            const StructBuilder builder = m_stack.Back().builder;
            SetValue(builder, field, value);
        }

        void SetGroupField(Field::Reader field, const toml::node& value)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Field::Meta::Group::Reader groupField = field.GetMeta().GetGroup();

            Declaration::Reader groupChild;
            for (Declaration::Reader child : decl.GetChildren())
            {
                if (child.GetId() == groupField.GetTypeId())
                {
                    if (!HE_VERIFY(child.GetData().IsStruct() && child.GetData().GetStruct().GetIsGroup()))
                        return;

                    groupChild = child;
                    break;
                }
            }

            if (!HE_VERIFY(groupChild.IsValid(),
                HE_KV(field_name, field.GetName().AsView()),
                HE_KV(group_id, groupField.GetTypeId())))
            {
                return;
            }

            PushGroup(groupChild.GetId(), m_stack.Back().builder);
            SetStruct(*value.as_table());
            PopGroup();
        }

        void SetUnionField(Field::Reader field, const toml::node& value)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Field::Meta::Union::Reader unionField = field.GetMeta().GetUnion();

            Declaration::Reader unionChild;
            for (Declaration::Reader child : decl.GetChildren())
            {
                if (child.GetId() == unionField.GetTypeId())
                {
                    if (!HE_VERIFY(child.GetData().IsStruct() && child.GetData().GetStruct().GetIsUnion()))
                        return;

                    unionChild = child;
                    break;
                }
            }

            if (!HE_VERIFY(unionChild.IsValid(),
                HE_KV(field_name, field.GetName().AsView()),
                HE_KV(group_id, unionField.GetTypeId())))
            {
                return;
            }

            Field::Reader activeField;
            Declaration::Data::Struct::Reader unionStruct = unionChild.GetData().GetStruct();

            const toml::table* data = value.as_table();

            if (data->empty())
                return;

            const toml::node* tagNode = data->get("_he_union_tag");

            if (tagNode && tagNode->is_integer())
            {
                const uint16_t tag = static_cast<uint16_t>(tagNode->as_integer()->get());

                for (Field::Reader f : unionStruct.GetFields())
                {
                    if (f.GetUnionTag() == tag)
                    {
                        activeField = f;
                        break;
                    }
                }
            }
            else
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Encountered TOML table without the _he_union_tag key. Guessing active field based on the first name encountered."),
                    HE_KV(union_field_name, field.GetName().AsView()),
                    HE_KV(union_type_id, unionChild.GetId()));

                const toml::key& firstFieldName = data->begin()->first;

                for (Field::Reader f : unionStruct.GetFields())
                {
                    if (f.GetName().AsView() == firstFieldName.str())
                    {
                        activeField = f;
                        break;
                    }
                }
            }

            if (!activeField.IsValid())
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Unable to determine active field of union from TOML data. Skipping deserialization of field."),
                    HE_KV(field_name, field.GetName().AsView()),
                    HE_KV(union_type_id, unionChild.GetId()),
                    HE_KV(toml_table_size, data->size()));
                return;
            }

            StructBuilder builder = m_stack.Back().builder;
            PushGroup(unionChild.GetId(), builder);

            builder.SetDataField(unionStruct.GetUnionTagOffset(), activeField.GetUnionTag());

            if (!activeField.GetMeta().IsNormal() || !activeField.GetMeta().GetNormal().GetType().GetData().IsVoid())
            {
                const StringView activeFieldName = activeField.GetName().AsView();
                const toml::node* fieldValue = data->get({ activeFieldName.begin(), activeFieldName.end() });

                if (fieldValue)
                {
                    SetField(activeField, *fieldValue);
                }
                else
                {
                    HE_LOG_ERROR(he_schema,
                        HE_MSG("Active union field name not found in TOML data. Skipping deserialization of field."),
                        HE_KV(active_field_name, activeFieldName),
                        HE_KV(union_field_name, field.GetName().AsView()),
                        HE_KV(union_type_id, unionChild.GetId()),
                        HE_KV(toml_table_size, data->size()));
                }
            }

            PopGroup();
        }

        template <typename T>
        void SetArrayDataElement(StructBuilder& builder, uint16_t fieldIndex, uint32_t fieldDataOffset, uint16_t arrSize, uint16_t arrIndex, const toml::node& value)
        {
            using Container = typename _ReadDataArrayReturnType<T>::Type;
            Container data = builder.GetAndMarkDataArrayField<T>(fieldIndex, fieldDataOffset, arrSize);
            data[arrIndex] = TomlValueHelper<T>::Get(value);
        }

        void SetValue(StructBuilder builder, const Field::Reader field, const toml::node& value)
        {
            const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
            const Type::Reader type = norm.GetType();
            const Type::Data::Reader typeData = type.GetData();
            const Type::Data::Tag typeDataTag = typeData.GetTag();

            if (!CheckTypeMatch(typeDataTag, value))
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Mismatch between toml data and field type. Skipping deserialization of field."),
                    HE_KV(field_name, field.GetName().AsView()),
                    HE_KV(field_type, typeDataTag),
                    HE_KV(toml_type, GetNodeTypeString(value.type())));
                return;
            }

            const uint32_t dataOffset = norm.GetDataOffset();
            const uint16_t index = norm.GetIndex();

            switch (typeDataTag)
            {
                case Type::Data::Tag::Void: break;
                case Type::Data::Tag::Bool: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<bool>::Get(value)); break;
                case Type::Data::Tag::Int8: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<int8_t>::Get(value)); break;
                case Type::Data::Tag::Int16: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<int16_t>::Get(value)); break;
                case Type::Data::Tag::Int32: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<int32_t>::Get(value)); break;
                case Type::Data::Tag::Int64: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<int64_t>::Get(value)); break;
                case Type::Data::Tag::Uint8: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<uint8_t>::Get(value)); break;
                case Type::Data::Tag::Uint16: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<uint16_t>::Get(value)); break;
                case Type::Data::Tag::Uint32: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<uint32_t>::Get(value)); break;
                case Type::Data::Tag::Uint64: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<uint64_t>::Get(value)); break;
                case Type::Data::Tag::Float32: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<float>::Get(value)); break;
                case Type::Data::Tag::Float64: builder.SetAndMarkDataField(index, dataOffset, TomlValueHelper<double>::Get(value)); break;
                case Type::Data::Tag::Array:
                {
                    SetArrayValue(builder, field, value);
                    break;
                }
                case Type::Data::Tag::Blob:
                {
                    Vector<uint8_t> bytes{ Allocator::GetTemp() };
                    if (!ParseBlobString(bytes, field, value))
                        return;

                    List<uint8_t>::Builder bytesBuilder = m_dst.AddList<uint8_t>(bytes.Size());
                    MemCopy(bytesBuilder.Data(), bytes.Data(), bytes.Size());
                    builder.GetPointerField(index).Set(bytesBuilder);
                    break;
                }
                case Type::Data::Tag::String:
                {
                    const std::string& str = value.as_string()->get();
                    String::Builder strBuilder = m_dst.AddString(str);
                    builder.GetPointerField(index).Set(strBuilder);
                    break;
                }
                case Type::Data::Tag::List:
                {
                    SetListValue(builder, field, value);
                    break;
                }
                case Type::Data::Tag::Enum:
                {
                    const StringView enumName = value.as_string()->get();
                    const Type::Data::Enum::Reader enumType = typeData.GetEnum();

                    PushGroup(enumType.GetId());
                    Declaration::Data::Enum::Reader enumDecl = m_stack.Back().decl.GetData().GetEnum();

                    for (Enumerator::Reader e : enumDecl.GetEnumerators())
                    {
                        if (e.GetName() == enumName)
                        {
                            builder.SetAndMarkDataField(index, dataOffset, e.GetOrdinal());
                            break;
                        }
                    }

                    PopGroup();

                    if (!builder.HasDataField(index))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Cannot find enum value for field. No such enumerator exists by that name."),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                            HE_KV(field_name, field.GetName().AsView()),
                            HE_KV(enum_name, enumName),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        return;
                    }

                    break;
                }
                case Type::Data::Tag::Struct:
                {
                    const Type::Data::Struct::Reader structType = typeData.GetStruct();

                    PushGroup(structType.GetId());
                    SetStruct(*value.as_table());
                    builder.GetPointerField(index).Set(m_stack.Back().builder);
                    PopGroup();

                    break;
                }
                case Type::Data::Tag::Interface:
                {
                    HE_LOG_ERROR(he_schema,
                        HE_MSG("Skipping Interface field when parsing from TOML."),
                        HE_KV(parent_id, m_stack.Back().decl.GetId()),
                        HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                        HE_KV(interface_type_id, typeData.GetInterface().GetId()),
                        HE_KV(field_name, field.GetName().AsView()),
                        HE_KV(index, index),
                        HE_KV(data_offset, dataOffset));
                    break;
                }
                case Type::Data::Tag::AnyPointer:
                {
                    HE_LOG_ERROR(he_schema,
                        HE_MSG("Skipping AnyPointer field when parsing from TOML."),
                        HE_KV(parent_id, m_stack.Back().decl.GetId()),
                        HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                        HE_KV(field_name, field.GetName().AsView()),
                        HE_KV(index, index),
                        HE_KV(data_offset, dataOffset));
                    break;
                }
            }
        }

        // TODO: Combine the 3 set value functions using the template technique I used in the TomlWriter.
        void SetArrayValue(StructBuilder builder, const Field::Reader field, const toml::node& value)
        {
            const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
            const Type::Reader type = norm.GetType();
            const Type::Data::Reader typeData = type.GetData();
            const Type::Data::Tag typeDataTag = typeData.GetTag();

            const uint32_t dataOffset = norm.GetDataOffset();
            const uint16_t index = norm.GetIndex();

            const Type::Data::Array::Reader arrayType = type.GetData().GetArray();
            const Type::Reader elementType = arrayType.GetElementType();
            const Type::Data::Tag elementTypeDataTag = elementType.GetData().GetTag();
            const uint16_t size = arrayType.GetSize();
            const toml::array* arr = value.as_array();
            const toml::node* first = arr->get(0);

            // empty data, just skip
            if (!first)
                return;

            // Check for a mixed-type array, which we cannot handle
            if (!arr->is_homogeneous())
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Toml array elements are not homogeneous, he_schema does not support mixed-type arrays. Skipping deserialization of field."),
                    HE_KV(field_name, field.GetName().AsView()),
                    HE_KV(field_type, typeDataTag),
                    HE_KV(field_element_type, elementTypeDataTag),
                    HE_KV(toml_type, GetNodeTypeString(value.type())));
                return;
            }

            // Check that the element types match
            if (!CheckTypeMatch(elementTypeDataTag, *first))
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Mismatch between toml array element type and array field element type. Skipping deserialization of field."),
                    HE_KV(field_name, field.GetName()),
                    HE_KV(field_type, typeDataTag),
                    HE_KV(field_element_type, elementTypeDataTag),
                    HE_KV(toml_type, GetNodeTypeString(value.type())),
                    HE_KV(toml_element_type, GetNodeTypeString(first->type())));
                return;
            }

            // Check if the array size is smaller than the data we have. We can still
            // parse, but emit a warning that we will be dropping data.
            if (size < arr->size())
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Toml array contains more elements than the array size. Trailing elements will be skipped."),
                    HE_KV(field_name, field.GetName().AsView()),
                    HE_KV(field_type, typeDataTag),
                    HE_KV(field_element_type, elementTypeDataTag),
                    HE_KV(field_array_size, size),
                    HE_KV(toml_type, GetNodeTypeString(value.type())),
                    HE_KV(toml_array_size, arr->size()));
            }

            for (uint16_t i = 0; i < size; ++i)
            {
                const toml::node& elmValue = *arr->get(i);

                switch (elementTypeDataTag)
                {
                    case Type::Data::Tag::Void: break;
                    case Type::Data::Tag::Bool: SetArrayDataElement<bool>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Int8: SetArrayDataElement<int8_t>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Int16: SetArrayDataElement<int16_t>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Int32: SetArrayDataElement<int32_t>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Int64: SetArrayDataElement<int64_t>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Uint8: SetArrayDataElement<uint8_t>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Uint16: SetArrayDataElement<uint16_t>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Uint32: SetArrayDataElement<uint32_t>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Uint64: SetArrayDataElement<uint64_t>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Float32: SetArrayDataElement<float>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Float64: SetArrayDataElement<double>(builder, index, dataOffset, size, i, elmValue); break;
                    case Type::Data::Tag::Blob:
                    {
                        Vector<uint8_t> bytes{ Allocator::GetTemp() };
                        if (!ParseBlobString(bytes, field, elmValue))
                            return;

                        List<uint8_t>::Builder bytesBuilder = m_dst.AddList<uint8_t>(bytes.Size());
                        MemCopy(bytesBuilder.Data(), bytes.Data(), bytes.Size());

                        builder.GetPointerArrayField(index, size).SetPointerElement(i, bytesBuilder);
                        break;
                    }
                    case Type::Data::Tag::String:
                    {
                        const std::string& str = elmValue.as_string()->get();
                        String::Builder strBuilder = m_dst.AddString(str);
                        builder.GetPointerArrayField(index, size).SetPointerElement(i, strBuilder);
                        break;
                    }
                    case Type::Data::Tag::Array:
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping array field when parsing TOML. Arrays of arrays are not supported."),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                            HE_KV(interface_type_id, elementType.GetData().GetInterface().GetId()),
                            HE_KV(field_name, field.GetName().AsView()),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        break;
                    }
                    case Type::Data::Tag::List:
                    {
                        SetListValue(builder, field, elmValue);
                        break;
                    }
                    case Type::Data::Tag::Enum:
                    {
                        const StringView enumName = elmValue.as_string()->get();
                        const Type::Data::Enum::Reader enumType = elementType.GetData().GetEnum();
                        PushGroup(enumType.GetId());
                        Declaration::Data::Enum::Reader enumDecl = m_stack.Back().decl.GetData().GetEnum();

                        for (Enumerator::Reader e : enumDecl.GetEnumerators())
                        {
                            if (e.GetName() == enumName)
                            {
                                Span<uint16_t> values = builder.GetAndMarkDataArrayField<uint16_t>(index, dataOffset, size);
                                values[i] = e.GetOrdinal();
                                break;
                            }
                        }

                        PopGroup();

                        if (!builder.HasDataField(index))
                        {
                            HE_LOG_ERROR(he_schema,
                                HE_MSG("Cannot find enum value for field. No such enumerator exists by that name."),
                                HE_KV(parent_id, m_stack.Back().decl.GetId()),
                                HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                                HE_KV(field_name, field.GetName().AsView()),
                                HE_KV(enum_name, enumName),
                                HE_KV(index, index),
                                HE_KV(data_offset, dataOffset));
                            return;
                        }

                        break;
                    }
                    case Type::Data::Tag::Struct:
                    {
                        const Type::Data::Struct::Reader structType = elementType.GetData().GetStruct();

                        PushGroup(structType.GetId());
                        SetStruct(*elmValue.as_table());
                        builder.GetPointerArrayField(index, size).SetPointerElement(i, m_stack.Back().builder);
                        PopGroup();

                        break;
                    }
                    case Type::Data::Tag::Interface:
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping Interface field when serializing."),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                            HE_KV(interface_type_id, elementType.GetData().GetInterface().GetId()),
                            HE_KV(field_name, field.GetName().AsView()),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        break;
                    }
                    case Type::Data::Tag::AnyPointer:
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping AnyPointer type when serializing to TOML."),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                            HE_KV(field_name, field.GetName().AsView()),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        break;
                    }
                    default:
                        HE_ASSERT(false, HE_MSG("Bug with IsPointer(), should never reach here."));
                }
            }
        }

        void SetListValue(StructBuilder builder, const Field::Reader field, const toml::node& value)
        {
            const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
            const Type::Reader type = norm.GetType();
            const Type::Data::Reader typeData = type.GetData();
            const Type::Data::Tag typeDataTag = typeData.GetTag();

            const uint32_t dataOffset = norm.GetDataOffset();
            const uint16_t index = norm.GetIndex();

            const Type::Data::List::Reader listType = type.GetData().GetList();
            const Type::Reader elementType = listType.GetElementType();
            const Type::Data::Tag elementTypeDataTag = elementType.GetData().GetTag();
            const toml::array* arr = value.as_array();
            const size_t arrSize = arr->size();
            const toml::node* first = arr->get(0);

            // empty data, just skip
            if (!first)
                return;

            // Check for a mixed-type array, which we cannot handle
            if (!arr->is_homogeneous())
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Toml array elements are not homogeneous, he_schema does not support mixed-type arrays. Skipping deserialization of field."),
                    HE_KV(field_name, field.GetName().AsView()),
                    HE_KV(field_type, typeDataTag),
                    HE_KV(field_element_type, elementTypeDataTag),
                    HE_KV(toml_type, GetNodeTypeString(value.type())));
                return;
            }

            // Check that the element types match
            if (!CheckTypeMatch(elementTypeDataTag, *first))
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Mismatch between toml array element type and array field element type. Skipping deserialization of field."),
                    HE_KV(field_name, field.GetName()),
                    HE_KV(field_type, typeDataTag),
                    HE_KV(field_element_type, elementTypeDataTag),
                    HE_KV(toml_type, GetNodeTypeString(value.type())),
                    HE_KV(toml_element_type, GetNodeTypeString(first->type())));
                return;
            }

            if (arrSize > std::numeric_limits<uint16_t>::max())
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Toml array length is too long to fit into a list. Skipping deserialization of field."),
                    HE_KV(field_name, field.GetName()),
                    HE_KV(field_type, typeDataTag),
                    HE_KV(field_element_type, elementTypeDataTag),
                    HE_KV(toml_type, GetNodeTypeString(value.type())),
                    HE_KV(toml_element_type, GetNodeTypeString(first->type())),
                    HE_KV(toml_array_size, arrSize));
                return;
            }

            const uint16_t size = static_cast<uint16_t>(arrSize);

            ListBuilder list;

            if (elementType.GetData().IsStruct())
            {
                const Type::Data::Struct::Reader structType = elementType.GetData().GetStruct();
                PushGroup(structType.GetId());
                const DeclInfo* info = m_stack.Back().info;
                list = m_dst.AddStructList(size, info->dataFieldCount, info->dataWordSize, info->pointerCount);
                PopGroup();
            }
            else
            {
                list = m_dst.AddList(GetTypeElementSize(elementType), size);
            }
            builder.GetPointerField(index).Set(list);

            for (uint16_t i = 0; i < size; ++i)
            {
                const toml::node& elmValue = *arr->get(i);

                switch (elementTypeDataTag)
                {
                    case Type::Data::Tag::Void: break;
                    case Type::Data::Tag::Bool: list.SetDataElement(i, TomlValueHelper<bool>::Get(elmValue)); break;
                    case Type::Data::Tag::Int8: list.SetDataElement(i, TomlValueHelper<int8_t>::Get(elmValue)); break;
                    case Type::Data::Tag::Int16: list.SetDataElement(i, TomlValueHelper<int16_t>::Get(elmValue)); break;
                    case Type::Data::Tag::Int32: list.SetDataElement(i, TomlValueHelper<int32_t>::Get(elmValue)); break;
                    case Type::Data::Tag::Int64: list.SetDataElement(i, TomlValueHelper<int64_t>::Get(elmValue)); break;
                    case Type::Data::Tag::Uint8: list.SetDataElement(i, TomlValueHelper<uint8_t>::Get(elmValue)); break;
                    case Type::Data::Tag::Uint16: list.SetDataElement(i, TomlValueHelper<uint16_t>::Get(elmValue)); break;
                    case Type::Data::Tag::Uint32: list.SetDataElement(i, TomlValueHelper<uint32_t>::Get(elmValue)); break;
                    case Type::Data::Tag::Uint64: list.SetDataElement(i, TomlValueHelper<uint64_t>::Get(elmValue)); break;
                    case Type::Data::Tag::Float32: list.SetDataElement(i, TomlValueHelper<float>::Get(elmValue)); break;
                    case Type::Data::Tag::Float64: list.SetDataElement(i, TomlValueHelper<double>::Get(elmValue)); break;
                    case Type::Data::Tag::Blob:
                    {
                        Vector<uint8_t> bytes{ Allocator::GetTemp() };
                        if (!ParseBlobString(bytes, field, elmValue))
                            return;

                        List<uint8_t>::Builder bytesBuilder = m_dst.AddList<uint8_t>(bytes.Size());
                        MemCopy(bytesBuilder.Data(), bytes.Data(), bytes.Size());
                        list.SetPointerElement(i, bytesBuilder);
                        break;
                    }
                    case Type::Data::Tag::String:
                    {
                        const std::string& str = elmValue.as_string()->get();
                        String::Builder strBuilder = m_dst.AddString(str);
                        list.SetPointerElement(i, strBuilder);
                        break;
                    }
                    case Type::Data::Tag::Array:
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping array field when parsing TOML. Lists of arrays are not supported."),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                            HE_KV(interface_type_id, elementType.GetData().GetInterface().GetId()),
                            HE_KV(field_name, field.GetName().AsView()),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        break;
                    }
                    case Type::Data::Tag::List:
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping list field when parsing TOML. Lists of lists are not supported, yet."),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                            HE_KV(field_name, field.GetName().AsView()),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        break;
                    }
                    case Type::Data::Tag::Enum:
                    {
                        const StringView enumName = elmValue.as_string()->get();
                        const Type::Data::Enum::Reader enumType = elementType.GetData().GetEnum();
                        PushGroup(enumType.GetId());
                        Declaration::Data::Enum::Reader enumDecl = m_stack.Back().decl.GetData().GetEnum();

                        bool found = false;
                        for (Enumerator::Reader e : enumDecl.GetEnumerators())
                        {
                            if (e.GetName() == enumName)
                            {
                                found = true;
                                list.SetDataElement(i, e.GetOrdinal());
                                break;
                            }
                        }

                        PopGroup();

                        if (!found)
                        {
                            HE_LOG_ERROR(he_schema,
                                HE_MSG("Cannot find enum value for field. No such enumerator exists by that name."),
                                HE_KV(parent_id, m_stack.Back().decl.GetId()),
                                HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                                HE_KV(field_name, field.GetName().AsView()),
                                HE_KV(enum_name, enumName),
                                HE_KV(index, index),
                                HE_KV(data_offset, dataOffset));
                            return;
                        }

                        break;
                    }
                    case Type::Data::Tag::Struct:
                    {
                        const Type::Data::Struct::Reader structType = elementType.GetData().GetStruct();

                        StructBuilder elementBuilder = list.GetCompositeElement(i);
                        PushGroup(structType.GetId(), elementBuilder);
                        SetStruct(*elmValue.as_table());
                        PopGroup();

                        break;
                    }
                    case Type::Data::Tag::Interface:
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping Interface field when serializing."),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                            HE_KV(interface_type_id, elementType.GetData().GetInterface().GetId()),
                            HE_KV(field_name, field.GetName().AsView()),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        break;
                    }
                    case Type::Data::Tag::AnyPointer:
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping AnyPointer type when serializing to TOML."),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                            HE_KV(field_name, field.GetName().AsView()),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        break;
                    }
                    default:
                        HE_ASSERT(false, HE_MSG("Bug with IsPointer(), should never reach here."));
                }
            }
        }

        void PushGroup(TypeId id)
        {
            const DeclInfo* parent = m_stack.Back().info;
            const DeclInfo* info = FindDependency(*parent, id);
            if (!HE_VERIFY(info))
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Failed to find dependent type, unable to properly deserialize data."),
                    HE_KV(parent_id, m_stack.Back().decl.GetId()),
                    HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                    HE_KV(id, id));
            }
            PushGroup(info);
        }

        void PushGroup(TypeId id, StructBuilder builder)
        {
            const DeclInfo* parent = m_stack.Back().info;
            const DeclInfo* info = FindDependency(*parent, id);
            if (!HE_VERIFY(info))
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Failed to find dependent type, unable to properly deserialize data."),
                    HE_KV(parent_id, m_stack.Back().decl.GetId()),
                    HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                    HE_KV(id, id));
            }

            Context& ctx = m_stack.EmplaceBack();
            ctx.info = info;
            ctx.decl = GetSchema(*info);
            ctx.builder = builder;
        }

        void PushGroup(const DeclInfo* info)
        {
            Context& ctx = m_stack.EmplaceBack();
            ctx.info = info;
            ctx.decl = GetSchema(*info);

            if (info->kind == DeclKind::Struct)
                ctx.builder = m_dst.AddStruct(info->dataFieldCount, info->dataWordSize, info->pointerCount);
        }

        void PopGroup()
        {
            m_stack.PopBack();
        }

    private:
        struct Context
        {
            const DeclInfo* info;
            Declaration::Reader decl;
            StructBuilder builder;
        };

    private:
        Builder& m_dst;
        Vector<Context> m_stack;
    };

    // --------------------------------------------------------------------------------------------
    class TomlWriter
    {
    public:
        TomlWriter(StringBuilder& dst)
            : m_writer(dst)
        {
            m_writer.Reserve(8192); // 8 KB
        }

        bool Write(StructReader data, const DeclInfo& info)
        {
            PushGroup(&info, "");
            WriteStruct(data);
            PopGroup();
            return true; // TODO: Actually return failures
        }

    private:
        void WriteStruct(StructReader data)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();

            // Write the basic fields first
            for (Field::Reader field : structDecl.GetFields())
            {
                const Field::Meta::Tag tag = field.GetMeta().GetTag();

                if (tag == Field::Meta::Tag::Normal)
                {
                    const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                    if (!norm.GetType().GetData().IsStruct())
                    {
                        WriteField(data, field);
                    }
                }
            }

            // Then write structure fields
            for (Field::Reader field : structDecl.GetFields())
            {
                const Field::Meta::Tag tag = field.GetMeta().GetTag();

                if (tag == Field::Meta::Tag::Normal)
                {
                    const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                    if (norm.GetType().GetData().IsStruct())
                    {
                        WriteField(data, field);
                    }
                }
                else
                {
                    WriteField(data, field);
                }
            }
        }

        void WriteField(StructReader data, Field::Reader field)
        {
            switch (field.GetMeta().GetTag())
            {
                case Field::Meta::Tag::Normal: WriteNormalField(data, field); break;
                case Field::Meta::Tag::Group: WriteGroupField(data, field); break;
                case Field::Meta::Tag::Union: WriteUnionField(data, field); break;
            }
        }

        void WriteNormalField(StructReader data, Field::Reader field)
        {
            const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
            const Type::Reader fieldType = norm.GetType();

            if (fieldType.GetData().IsVoid())
                return;

            if (IsPointer(fieldType))
            {
                if (!data.HasPointerField(norm.GetIndex()))
                    return;
            }
            else
            {
                if (!data.HasDataField(norm.GetIndex()))
                    return;
            }

            const bool asHex = FindAttribute(field.GetAttributes(), Toml::Hex::Id).IsValid();
            WriteValue(data, field.GetName(), fieldType, norm.GetIndex(), norm.GetDataOffset(), asHex);

            if (m_arrayStack <= 1)
                m_writer.Write('\n');
        }

        void WriteGroupField(StructReader data, Field::Reader field)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Field::Meta::Group::Reader groupField = field.GetMeta().GetGroup();

            Declaration::Reader groupChild;
            for (Declaration::Reader child : decl.GetChildren())
            {
                if (child.GetId() == groupField.GetTypeId())
                {
                    if (!HE_VERIFY(child.GetData().IsStruct() && child.GetData().GetStruct().GetIsGroup()))
                        return;

                    groupChild = child;
                    break;
                }
            }

            if (!HE_VERIFY(groupChild.IsValid()))
                return;

            PushGroup(groupChild.GetId(), field.GetName());
            m_writer.WriteLine("[{}]", m_pathName);
            m_writer.IncreaseIndent();
            WriteStruct(data);
            m_writer.DecreaseIndent();
            PopGroup();
        }

        void WriteUnionField(StructReader data, Field::Reader field)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Field::Meta::Union::Reader unionField = field.GetMeta().GetUnion();

            Declaration::Reader unionChild;
            for (Declaration::Reader child : decl.GetChildren())
            {
                if (child.GetId() == unionField.GetTypeId())
                {
                    if (!HE_VERIFY(child.GetData().IsStruct() && child.GetData().GetStruct().GetIsUnion()))
                        return;

                    unionChild = child;
                    break;
                }
            }

            if (!HE_VERIFY(unionChild.IsValid()))
                return;

            Declaration::Data::Struct::Reader unionStruct = unionChild.GetData().GetStruct();

            const uint16_t tag = data.GetDataField<uint16_t>(unionStruct.GetUnionTagOffset());

            Field::Reader activeField;
            for (Field::Reader f : unionStruct.GetFields())
            {
                if (f.GetUnionTag() == tag)
                {
                    activeField = f;
                    break;
                }
            }

            if (!HE_VERIFY(activeField.IsValid()))
                return;

            PushGroup(unionChild.GetId(), field.GetName());
            m_writer.WriteLine("[{}]", m_pathName);
            m_writer.IncreaseIndent();
            m_writer.WriteLine("_he_union_tag = {} # {}", tag, activeField.GetName().AsView());
            WriteField(data, activeField);
            m_writer.DecreaseIndent();
            PopGroup();
        }

        template <typename ReaderType>
        void WriteArrayValue(ReaderType data, StringView name, Type::Reader elementType, uint32_t index, uint32_t dataOffset, uint32_t size, bool asHex)
        {
            using Helper = SchemaValueHelper<ReaderType>;

            const bool isArrayOfStructs = elementType.GetData().IsStruct();

            if (isArrayOfStructs)
                PushGroup(elementType.GetData().GetStruct().GetId(), name);
            else
                m_writer.Write('[');

            for (uint32_t i = 0; i < size; ++i)
            {
                if (isArrayOfStructs)
                {
                    m_writer.WriteLine("[[{}]]", m_pathName);
                    m_writer.IncreaseIndent();
                    WriteStruct(Helper::GetComposite(data, index + i));
                    m_writer.DecreaseIndent();
                }
                else
                {
                    WriteValue(data, "", elementType, index + i, dataOffset + i, asHex);

                    if (i != (size - 1))
                        m_writer.Write(", ");
                }
            }

            if (isArrayOfStructs)
                PopGroup();
            else
                m_writer.Write(']');
        }

        template <typename ReaderType>
        void WriteValue(ReaderType data, StringView name, Type::Reader type, uint32_t index, uint32_t dataOffset, bool asHex)
        {
            using Helper = SchemaValueHelper<ReaderType>;

            const auto dataValueFmt = fmt::runtime(asHex ? "{:#x}" : "{}");

            const Type::Data::Reader typeData = type.GetData();
            const Type::Data::Tag typeDataTag = typeData.GetTag();

            if (!name.IsEmpty()
                && typeDataTag != Type::Data::Tag::Array
                && typeDataTag != Type::Data::Tag::List
                && typeDataTag != Type::Data::Tag::Struct)
            {
                m_writer.WriteIndent();
                m_writer.Write("{} = ", name);
            }

            switch (typeDataTag)
            {
                case Type::Data::Tag::Void: break;
                case Type::Data::Tag::Bool: m_writer.Write(dataValueFmt, Helper::template GetData<bool>(data, index, dataOffset)); break;
                case Type::Data::Tag::Int8: m_writer.Write(dataValueFmt, Helper::template GetData<int8_t>(data, index, dataOffset)); break;
                case Type::Data::Tag::Int16: m_writer.Write(dataValueFmt, Helper::template GetData<int16_t>(data, index, dataOffset)); break;
                case Type::Data::Tag::Int32: m_writer.Write(dataValueFmt, Helper::template GetData<int32_t>(data, index, dataOffset)); break;
                case Type::Data::Tag::Int64: m_writer.Write(dataValueFmt, Helper::template GetData<int64_t>(data, index, dataOffset)); break;
                case Type::Data::Tag::Uint8: m_writer.Write(dataValueFmt, Helper::template GetData<uint8_t>(data, index, dataOffset)); break;
                case Type::Data::Tag::Uint16: m_writer.Write(dataValueFmt, Helper::template GetData<uint16_t>(data, index, dataOffset)); break;
                case Type::Data::Tag::Uint32: m_writer.Write(dataValueFmt, Helper::template GetData<uint32_t>(data, index, dataOffset)); break;
                case Type::Data::Tag::Uint64: m_writer.Write("\"{:#x}\"", Helper::template GetData<uint64_t>(data, index, dataOffset)); break;
                case Type::Data::Tag::Float32: m_writer.Write(dataValueFmt, Helper::template GetData<float>(data, index, dataOffset)); break;
                case Type::Data::Tag::Float64: m_writer.Write(dataValueFmt, Helper::template GetData<double>(data, index, dataOffset)); break;
                case Type::Data::Tag::Array:
                {
                    const Type::Data::Array::Reader arrayType = type.GetData().GetArray();
                    const Type::Reader elementType = arrayType.GetElementType();
                    const uint32_t size = arrayType.GetSize();
                    WriteArrayValue(data, name, elementType, index, dataOffset, size, asHex);
                    break;
                }
                case Type::Data::Tag::Blob:
                {
                    const List<uint8_t>::Reader byteList = Helper::GetPointer(data, index).template TryGetList<uint8_t>();
                    const Span<const uint8_t> bytes{ byteList.Data(), byteList.Size() };
                    m_writer.Write("\"{}\"", bytes); // TODO: Base64 if attribute is set
                    break;
                }
                case Type::Data::Tag::String:
                {
                    const String::Reader str = Helper::GetPointer(data, index).TryGetString();
                    m_writer.Write("\"{}\"", str.AsView());
                    break;
                }
                case Type::Data::Tag::List:
                {
                    const Type::Data::List::Reader listType = typeData.GetList();
                    const Type::Reader elementType = listType.GetElementType();
                    const ElementSize elementSize = GetTypeElementSize(elementType);
                    const ListReader list = Helper::GetPointer(data, index).TryGetList(elementSize);
                    WriteArrayValue(list, name, elementType, 0, 0, list.Size(), asHex);
                    break;
                }
                case Type::Data::Tag::Enum:
                {
                    const Type::Data::Enum::Reader enumType = typeData.GetEnum();
                    PushGroup(enumType.GetId(), "");
                    Declaration::Data::Enum::Reader enumDecl = m_stack.Back().decl.GetData().GetEnum();

                    bool found = false;
                    const uint16_t enumValue = Helper::template GetData<uint16_t>(data, index, dataOffset);
                    for (Enumerator::Reader e : enumDecl.GetEnumerators())
                    {
                        if (e.GetOrdinal() == enumValue)
                        {
                            found = true;
                            m_writer.Write("\"{}\"", e.GetName().AsView());
                            break;
                        }
                    }
                    PopGroup();

                    if (!HE_VERIFY(found))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Cannot find enum value for field. No enumerator exists in the schema with that value."),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                            HE_KV(enum_value, enumValue),
                            HE_KV(path, m_pathName),
                            HE_KV(name, name),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        return;
                    }

                    break;
                }
                case Type::Data::Tag::Struct:
                {
                    const Type::Data::Struct::Reader structType = typeData.GetStruct();
                    const StructReader value = Helper::GetComposite(data, index);

                    if (m_arrayStack <= 1)
                        PushGroup(structType.GetId(), name);
                    else
                        m_writer.Write("{ ");

                    if (!name.IsEmpty() && m_arrayStack == 0)
                    {
                        m_writer.WriteLine("[{}]", m_pathName);
                        m_writer.IncreaseIndent();
                    }

                    WriteStruct(value);

                    if (m_arrayStack <= 1)
                        PopGroup();
                    else
                        m_writer.Write(" }");

                    if (!name.IsEmpty() && m_arrayStack == 0)
                        m_writer.DecreaseIndent();

                    break;
                }
                case Type::Data::Tag::Interface:
                {
                    HE_LOG_ERROR(he_schema,
                        HE_MSG("Skipping Interface type when serializing."),
                        HE_KV(parent_id, m_stack.Back().decl.GetId()),
                        HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                        HE_KV(interface_type_id, typeData.GetInterface().GetId()),
                        HE_KV(path, m_pathName),
                        HE_KV(name, name),
                        HE_KV(index, index),
                        HE_KV(data_offset, dataOffset));
                    break;
                }
                case Type::Data::Tag::AnyPointer:
                {
                    HE_LOG_ERROR(he_schema,
                        HE_MSG("Skipping AnyPointer type when serializing to TOML."),
                        HE_KV(parent_id, m_stack.Back().decl.GetId()),
                        HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                        HE_KV(path, m_pathName),
                        HE_KV(name, name),
                        HE_KV(index, index),
                        HE_KV(data_offset, dataOffset));
                    break;
                }
            }
        }

        void PushGroup(TypeId id, StringView name, bool isArray = false)
        {
            const DeclInfo* parent = m_stack.Back().info;
            const DeclInfo* info = FindDependency(*parent, id);
            if (!HE_VERIFY(info))
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Failed to find dependent type, unable to properly serialize data."),
                    HE_KV(parent_id, m_stack.Back().decl.GetId()),
                    HE_KV(parent_name, m_stack.Back().decl.GetName().AsView()),
                    HE_KV(path, m_pathName),
                    HE_KV(id, id),
                    HE_KV(name, name));
            }
            PushGroup(info, name, isArray);
        }

        void PushGroup(const DeclInfo* info, StringView name, bool isArray = false)
        {
            if (isArray)
                ++m_arrayStack;

            Context& ctx = m_stack.EmplaceBack();
            ctx.pathIndex = m_pathName.Size();
            ctx.info = info;
            ctx.decl = GetSchema(*info);

            if (!name.IsEmpty())
            {
                if (!m_pathName.IsEmpty())
                    m_pathName.PushBack('.');

                m_pathName += name;
            }
        }

        void PopGroup(bool isArray = false)
        {
            if (isArray)
                --m_arrayStack;

            const uint32_t index = m_stack.Back().pathIndex;

            m_stack.PopBack();
            m_pathName.Erase(index, m_pathName.Size() - index);
        }

    private:
        struct Context
        {
            uint32_t pathIndex;
            const DeclInfo* info;
            Declaration::Reader decl;
        };

    private:
        StringBuilder& m_writer;
        uint32_t m_arrayStack{ 0 };
        ::he::String m_pathName;
        Vector<Context> m_stack;
    };

    // --------------------------------------------------------------------------------------------
    bool ToToml(StringBuilder& dst, StructReader data, const DeclInfo& info)
    {
       TomlWriter writer(dst);
       return writer.Write(data, info);
    }

     bool FromToml(Builder& dst, const char* data, const DeclInfo& info)
     {
        TomlReader reader(dst);
        return reader.Read(data, info);
     }
}
