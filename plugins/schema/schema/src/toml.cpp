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
    // class TomlReader
    // {
    // public:
    //     TomlReader(Builder& dst) : m_dst(dst) {}

    // private:
    //     Builder& m_dst;
    // };

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

            m_writer.WriteLine("_he_type_id = {}", decl.Id());
            m_writer.WriteLine("_he_type_name = {}", decl.Name().AsView());
            for (Field::Reader field : structDecl.Fields())
            {
                WriteField(data, field);

                if (m_arrayStack <= 1)
                    m_writer.Write('\n');
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
            m_writer.Write("[{}]\n", m_pathName);
            WriteStruct(data);
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

            PushGroup(unionChild.Id(), "");
            WriteField(data, activeField);
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
                PushGroup(elementType.Data().Struct().Id(), name, true);
            else
                m_writer.Write('[');

            for (uint32_t i = 0; i < size; ++i)
            {
                if (isArrayOfStructs)
                {
                    m_writer.Write("[[{}]]\n", m_pathName);
                    WriteStruct(Helper::GetComposite(data, index + i));
                }
                else
                {
                    WriteValue(data, "", elementType, index + i, dataOffset + i, asHex);

                    if (i != (size - 1))
                        m_writer.Write(", ");
                }
            }

            if (isArrayOfStructs)
                PopGroup(true);
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
                    const List<uint8_t>::Reader bytes = Helper::GetPointer(data, index).TryGetList<uint8_t>();
                    const Span<const uint8_t> byteSpan{ bytes.Data(), bytes.Size() };
                    m_writer.WriteLine("\"{}\"", byteSpan); // TODO: Base64 if attribute is set
                    break;
                }
                case Type::Data::Tag::String:
                {
                    const String::Reader str = Helper::GetPointer(data, index).TryGetString();
                    m_writer.WriteLine("\"{}\"", str.AsView());
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

                    const uint16_t enumValue = Helper::template GetData<uint16_t>(data, index, dataOffset);
                    for (Enumerator::Reader e : enumDecl.Enumerators())
                    {
                        if (e.Ordinal() == enumValue)
                        {
                            m_writer.Write(e.Name());
                            break;
                        }
                    }
                    PopGroup();
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
                        m_writer.Write("[{}]\n", name);

                    WriteStruct(value);

                    if (m_arrayStack <= 1)
                        PopGroup();
                    else
                        m_writer.Write(" }");

                    break;
                }
                case Type::Data::Tag::Interface:
                    if (!HE_VERIFY(false, "Interface types cannot have values."))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping Interface type when serializing."),
                            HE_KV(parent_id, m_stack.Back().decl.Id()),
                            HE_KV(parent_name, m_stack.Back().decl.Name().AsView()),
                            HE_KV(interface_type_id, typeData.Interface().Id()),
                            HE_KV(name, name),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                    }
                    break;
                case Type::Data::Tag::AnyPointer:
                    // TODO: Should be able to serialize these though...
                    if (!HE_VERIFY(false, "AnyPointer types cannot have values."))
                    {
                        HE_LOG_ERROR(he_schema,
                            HE_MSG("Skipping AnyPointer type when serializing."),
                            HE_KV(parent_id, m_stack.Back().decl.Id()),
                            HE_KV(parent_name, m_stack.Back().decl.Name().AsView()),
                            HE_KV(name, name),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                    }
                    break;
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

            if (!m_pathName.IsEmpty())
                m_pathName.PushBack('.');

            m_pathName += name;
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
        const DeclInfo* m_info;
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

    // bool FromToml(Builder& dst, const DeclInfo& info)
    // {
    //    TomlReader reader(dst);
    //    return reader.Visit(schema);
    // }
}
