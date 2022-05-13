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
    // --------------------------------------------------------------------------------------------
    // TODO: Remove toml++ usage and just parse the toml text directly. Format is simple enough and
    // we really don't need 75% of what toml++ provides. It would also make error detection and handling
    // much easier.
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
            return SetStruct(result.table());
        }

    private:
        bool SetStruct(const toml::table& table)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Declaration::Data::Struct::Reader st = decl.Data().Struct();

            for (auto&& [key, value] : table)
            {
                const Field::Reader field = FindField(key.str(), st);

                if (field.IsValid())
                {
                    SetField(field, value);
                }
                else
                {
                    UnknownField& uf = m_unknownFields.EmplaceBack();
                    uf.key = key.str();
                    uf.value = value;
                }
            }

            return true;
        }

        void SetField(Field::Reader field, const toml::node& value)
        {
            switch (field.Meta().Tag())
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
            // TODO
        }

        void SetUnionField(Field::Reader field, const toml::node& value)
        {
            // TODO
        }

        const char* GetNodeTypeString(toml::node_type t)
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

        bool CheckTypeMatch(const Type::Data::Tag typeDataTag, const toml::node& value)
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
                case Type::Data::Tag::Uint64: return value.is_integer();
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

        void SetValue(StructBuilder builder, const Field::Reader field, const toml::node& value)
        {
            const Field::Meta::Normal::Reader norm = field.Meta().Normal();
            const Type::Reader type = norm.Type();
            const Type::Data::Reader typeData = type.Data();
            const Type::Data::Tag typeDataTag = typeData.Tag();

            if (!CheckTypeMatch(typeDataTag, value))
            {
                HE_LOG_ERROR(he_schema,
                    HE_MSG("Mismatch between toml data and field type. Skipping deserialization of field."),
                    HE_KV(field_name, field.Name()),
                    HE_KV(field_type, typeDataTag),
                    HE_KV(toml_type, GetNodeTypeString(value.type())));
                return;
            }

            const uint32_t dataOffset = norm.DataOffset();
            const uint32_t index = norm.Index();

            switch (typeDataTag)
            {
                case Type::Data::Tag::Void: break;
                case Type::Data::Tag::Bool: builder.SetAndMarkDataField(index, dataOffset, value.as_boolean()->get()); break;
                case Type::Data::Tag::Int8: builder.SetAndMarkDataField(index, dataOffset, value.as_integer()->get()); break;
                case Type::Data::Tag::Int16: builder.SetAndMarkDataField(index, dataOffset, value.as_integer()->get()); break;
                case Type::Data::Tag::Int32: builder.SetAndMarkDataField(index, dataOffset, value.as_integer()->get()); break;
                case Type::Data::Tag::Int64: builder.SetAndMarkDataField(index, dataOffset, value.as_integer()->get()); break;
                case Type::Data::Tag::Uint8: builder.SetAndMarkDataField(index, dataOffset, value.as_integer()->get()); break;
                case Type::Data::Tag::Uint16: builder.SetAndMarkDataField(index, dataOffset, value.as_integer()->get()); break;
                case Type::Data::Tag::Uint32: builder.SetAndMarkDataField(index, dataOffset, value.as_integer()->get()); break;
                case Type::Data::Tag::Uint64: builder.SetAndMarkDataField(index, dataOffset, value.as_integer()->get()); break;
                case Type::Data::Tag::Float32: builder.SetAndMarkDataField(index, dataOffset, value.as_floating_point()->get()); break;
                case Type::Data::Tag::Float64: builder.SetAndMarkDataField(index, dataOffset, value.as_floating_point()->get()); break;
                case Type::Data::Tag::Array:
                {
                    // TODO
                    const Type::Data::Array::Reader arrayType = type.Data().Array();
                    const Type::Reader elementType = arrayType.ElementType();
                    const uint32_t size = arrayType.Size();
                    WriteArrayValue(data, name, elementType, index, dataOffset, size, asHex);
                    break;
                }
                case Type::Data::Tag::Blob:
                {
                    const std::string& str = value.as_string()->get();
                    // TODO: hex (or b64) str -> bytes
                    break;
                }
                case Type::Data::Tag::String:
                {
                    const std::string& str = value.as_string()->get();
                    String::Builder strBuilder = m_dst.AddString(str);
                    builder.GetPointerField(norm.Index()).Set(strBuilder);
                    break;
                }
                case Type::Data::Tag::List:
                {
                    // TODO
                    const Type::Data::List::Reader listType = typeData.List();
                    const Type::Reader elementType = listType.ElementType();
                    const ElementSize elementSize = GetTypeElementSize(elementType);
                    const ListReader list = Helper::GetPointer(data, index).TryGetList(elementSize);
                    WriteArrayValue(list, name, elementType, 0, 0, list.Size(), asHex);
                    break;
                }
                case Type::Data::Tag::Enum:
                {
                    const StringView enumName = value.as_string()->get();
                    const Type::Data::Enum::Reader enumType = typeData.Enum();
                    PushGroup(enumType.Id());
                    Declaration::Data::Enum::Reader enumDecl = m_stack.Back().decl.Data().Enum();

                    for (Enumerator::Reader e : enumDecl.Enumerators())
                    {
                        if (e.Name() == enumName)
                        {
                            builder.SetAndMarkDataField(index, dataOffset, e.Ordinal());
                            break;
                        }
                    }

                    PopGroup();

                    if (!HE_VERIFY(builder.HasDataField(index)))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Cannot find enum value for field. No such enumerator exists by that name."),
                            HE_KV(parent_id, m_stack.Back().decl.Id()),
                            HE_KV(parent_name, m_stack.Back().decl.Name()),
                            HE_KV(field_name, field.Name()),
                            HE_KV(enum_name, enumName),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                        return;
                    }

                    break;
                }
                case Type::Data::Tag::Struct:
                {
                    const Type::Data::Struct::Reader structType = typeData.Struct();
                    const StructReader value = Helper::GetComposite(data, index);

                    if (m_arrayStack <= 1)
                        PushGroup(structType.Id(), name);
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
                    if (!HE_VERIFY(false, "Interface types cannot have values."))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping Interface type when serializing."),
                            HE_KV(parent_id, m_stack.Back().decl.Id()),
                            HE_KV(parent_name, m_stack.Back().decl.Name().AsView()),
                            HE_KV(interface_type_id, typeData.Interface().Id()),
                            HE_KV(path, m_pathName),
                            HE_KV(name, name),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                    }
                    break;
                }
                case Type::Data::Tag::AnyPointer:
                {
                    if (!HE_VERIFY(false, "Serialization of AnyPointer types is not supported."))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping AnyPointer type when serializing to TOML."),
                            HE_KV(parent_id, m_stack.Back().decl.Id()),
                            HE_KV(parent_name, m_stack.Back().decl.Name().AsView()),
                            HE_KV(path, m_pathName),
                            HE_KV(name, name),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                    }
                    break;
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
                    HE_KV(parent_id, m_stack.Back().decl.Id()),
                    HE_KV(parent_name, m_stack.Back().decl.Name().AsView()),
                    HE_KV(id, id));
            }
            PushGroup(info);
        }

        void PushGroup(const DeclInfo* info)
        {
            Context& ctx = m_stack.EmplaceBack();
            ctx.info = info;
            ctx.decl = GetSchema(*info);
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

        struct UnknownField
        {
            ::he::String key;
            toml::node value;
        };

    private:
        Builder& m_dst;
        Vector<Context> m_stack;
        Vector<UnknownField> m_unknownFields; // TODO: Maybe these should be in Builder?
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
            const Declaration::Data::Struct::Reader structDecl = decl.Data().Struct();

            for (Field::Reader field : structDecl.Fields())
            {
                WriteField(data, field);
            }
        }

        void WriteField(StructReader data, Field::Reader field)
        {
            switch (field.Meta().Tag())
            {
                case Field::Meta::Tag::Normal: WriteNormalField(data, field); break;
                case Field::Meta::Tag::Group: WriteGroupField(data, field); break;
                case Field::Meta::Tag::Union: WriteUnionField(data, field); break;
            }
        }

        void WriteNormalField(StructReader data, Field::Reader field)
        {
            const Field::Meta::Normal::Reader norm = field.Meta().Normal();
            const Type::Reader fieldType = norm.Type();

            if (fieldType.Data().IsVoid())
                return;

            if (IsPointer(fieldType))
            {
                if (!data.HasPointerField(norm.Index()))
                    return;
            }
            else
            {
                if (!data.HasDataField(norm.Index()))
                    return;
            }

            const bool asHex = FindAttribute(field.Attributes(), Toml::Hex::Id).IsValid();
            WriteValue(data, field.Name(), fieldType, norm.Index(), norm.DataOffset(), asHex);

            if (m_arrayStack <= 1)
                m_writer.Write('\n');
        }

        void WriteGroupField(StructReader data, Field::Reader field)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Field::Meta::Group::Reader groupField = field.Meta().Group();

            Declaration::Reader groupChild;
            for (Declaration::Reader child : decl.Children())
            {
                if (child.Id() == groupField.TypeId())
                {
                    if (!HE_VERIFY(child.Data().IsStruct() && child.Data().Struct().IsGroup()))
                        return;

                    groupChild = child;
                    break;
                }
            }

            if (!HE_VERIFY(groupChild.IsValid()))
                return;

            PushGroup(groupChild.Id(), field.Name());
            m_writer.WriteLine("[{}]", m_pathName);
            m_writer.IncreaseIndent();
            WriteStruct(data);
            m_writer.DecreaseIndent();
            PopGroup();
        }

        void WriteUnionField(StructReader data, Field::Reader field)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Field::Meta::Union::Reader unionField = field.Meta().Union();

            Declaration::Reader unionChild;
            for (Declaration::Reader child : decl.Children())
            {
                if (child.Id() == unionField.TypeId())
                {
                    if (!HE_VERIFY(child.Data().IsStruct() && child.Data().Struct().IsUnion()))
                        return;

                    unionChild = child;
                    break;
                }
            }

            if (!HE_VERIFY(unionChild.IsValid()))
                return;

            Declaration::Data::Struct::Reader unionStruct = unionChild.Data().Struct();

            const uint16_t tag = data.GetDataField<uint16_t>(unionStruct.UnionTagOffset());

            Field::Reader activeField;
            for (Field::Reader f : unionStruct.Fields())
            {
                if (f.UnionTag() == tag)
                {
                    activeField = f;
                    break;
                }
            }

            if (!HE_VERIFY(activeField.IsValid()))
                return;

            PushGroup(unionChild.Id(), field.Name());
            m_writer.WriteLine("[{}]", m_pathName);
            m_writer.IncreaseIndent();
            m_writer.WriteLine("_he_union_tag = {} # {}", tag, activeField.Name().AsView());
            WriteField(data, activeField);
            m_writer.DecreaseIndent();
            PopGroup();
        }

        template <typename ReaderType>
        struct ValueHelper;

        template <>
        struct ValueHelper<StructReader>
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
        struct ValueHelper<ListReader>
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

        template <typename ReaderType>
        void WriteArrayValue(ReaderType data, StringView name, Type::Reader elementType, uint32_t index, uint32_t dataOffset, uint32_t size, bool asHex)
        {
            using Helper = ValueHelper<ReaderType>;

            const bool isArrayOfStructs = elementType.Data().IsStruct();

            if (isArrayOfStructs)
                PushGroup(elementType.Data().Struct().Id(), name);
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
            using Helper = ValueHelper<ReaderType>;

            const auto dataValueFmt = asHex ? "{:#x}" : "{}";

            const Type::Data::Reader typeData = type.Data();
            const Type::Data::Tag typeDataTag = typeData.Tag();

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
                case Type::Data::Tag::Uint64: m_writer.Write(dataValueFmt, Helper::template GetData<uint64_t>(data, index, dataOffset)); break;
                case Type::Data::Tag::Float32: m_writer.Write(dataValueFmt, Helper::template GetData<float>(data, index, dataOffset)); break;
                case Type::Data::Tag::Float64: m_writer.Write(dataValueFmt, Helper::template GetData<double>(data, index, dataOffset)); break;
                case Type::Data::Tag::Array:
                {
                    const Type::Data::Array::Reader arrayType = type.Data().Array();
                    const Type::Reader elementType = arrayType.ElementType();
                    const uint32_t size = arrayType.Size();
                    WriteArrayValue(data, name, elementType, index, dataOffset, size, asHex);
                    break;
                }
                case Type::Data::Tag::Blob:
                {
                    const List<uint8_t>::Reader byteList = Helper::GetPointer(data, index).TryGetList<uint8_t>();
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
                    const Type::Data::List::Reader listType = typeData.List();
                    const Type::Reader elementType = listType.ElementType();
                    const ElementSize elementSize = GetTypeElementSize(elementType);
                    const ListReader list = Helper::GetPointer(data, index).TryGetList(elementSize);
                    WriteArrayValue(list, name, elementType, 0, 0, list.Size(), asHex);
                    break;
                }
                case Type::Data::Tag::Enum:
                {
                    const Type::Data::Enum::Reader enumType = typeData.Enum();
                    PushGroup(enumType.Id(), "");
                    Declaration::Data::Enum::Reader enumDecl = m_stack.Back().decl.Data().Enum();

                    bool found = false;
                    const uint16_t enumValue = Helper::template GetData<uint16_t>(data, index, dataOffset);
                    for (Enumerator::Reader e : enumDecl.Enumerators())
                    {
                        if (e.Ordinal() == enumValue)
                        {
                            found = true;
                            m_writer.Write("\"{}\"", e.Name());
                            break;
                        }
                    }
                    PopGroup();

                    if (!HE_VERIFY(found))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Cannot find enum value for field. No enumerator exists in the schema with that value."),
                            HE_KV(parent_id, m_stack.Back().decl.Id()),
                            HE_KV(parent_name, m_stack.Back().decl.Name()),
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
                    const Type::Data::Struct::Reader structType = typeData.Struct();
                    const StructReader value = Helper::GetComposite(data, index);

                    if (m_arrayStack <= 1)
                        PushGroup(structType.Id(), name);
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
                    if (!HE_VERIFY(false, "Interface types cannot have values."))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping Interface type when serializing."),
                            HE_KV(parent_id, m_stack.Back().decl.Id()),
                            HE_KV(parent_name, m_stack.Back().decl.Name().AsView()),
                            HE_KV(interface_type_id, typeData.Interface().Id()),
                            HE_KV(path, m_pathName),
                            HE_KV(name, name),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                    }
                    break;
                }
                case Type::Data::Tag::AnyPointer:
                {
                    if (!HE_VERIFY(false, "Serialization of AnyPointer types is not supported."))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping AnyPointer type when serializing to TOML."),
                            HE_KV(parent_id, m_stack.Back().decl.Id()),
                            HE_KV(parent_name, m_stack.Back().decl.Name().AsView()),
                            HE_KV(path, m_pathName),
                            HE_KV(name, name),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                    }
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
                    HE_KV(parent_id, m_stack.Back().decl.Id()),
                    HE_KV(parent_name, m_stack.Back().decl.Name().AsView()),
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
