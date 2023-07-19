// Copyright Chad Engler

#include "he/schema/toml.h"

#include "he/core/assert.h"
#include "he/core/base64.h"
#include "he/core/clock.h"
#include "he/core/clock_fmt.h"
#include "he/core/toml_document.h"
#include "he/core/fmt.h"
#include "he/core/limits.h"
#include "he/core/log.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/core/vector_fmt.h"

#include "zstd.h"

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    constexpr StringView SchemaUnionTagName = "__schema_union_tag";

    static bool ParseBlobHexString(Vector<uint8_t>& out, const Field::Reader& field, const StringView& value)
    {
        // Reserve the maximum possible byte size. Because we allow spaces this may over-allocate
        // but likely not by much, and that's better than multiple reallocs in the loop.
        const uint32_t len = value.Size();
        out.Reserve(len / 2);

        const char* s = value.Begin();

        char first = 0;
        while (s < value.End())
        {
            const char c = *s++;

            if (c == ' ')
                continue;

            if (!IsHex(c))
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Invalid character found in blob string. Skipping deserialization of field."),
                    HE_KV(field_name, field.GetName()),
                    HE_KV(field_type, field.GetMeta().GetNormal().GetType().GetData().GetUnionTag()),
                    HE_KV(bad_char, c),
                    HE_KV(char_offset, (s - value.Begin())));
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
            HE_LOG_WARN(he_schema,
                HE_MSG("Invalid hex string used for blob, there is a trailing nibble. Skipping deserialization of field."),
                HE_KV(field_name, field.GetName()),
                HE_KV(field_type, field.GetMeta().GetNormal().GetType().GetData().GetUnionTag()));
            return false;
        }

        return true;
    }

    static bool ParseBlobBase64String(Vector<uint8_t>& out, const Field::Reader& field, bool useZstd, const StringView& value)
    {
        if (!Base64Decode(out, value))
        {
            HE_LOG_WARN(he_schema,
                HE_MSG("Skipping blob field when parsing TOML. The value is not valid Base64."),
                HE_KV(field_name, field.GetName()),
                HE_KV(field_type, field.GetMeta().GetNormal().GetType().GetData().GetUnionTag()));
            return false;
        }

        if (useZstd && out.Size() >= ZSTD_STATIC_LINKING_ONLY)
        {
            const uint64_t maxSize = ZSTD_getFrameContentSize(out.Data(), out.Size());

            if (maxSize == ZSTD_CONTENTSIZE_UNKNOWN || maxSize == ZSTD_CONTENTSIZE_ERROR || maxSize >= Limits<uint32_t>::Max)
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Blob field is marked as zstd compressed, but the compression header is invalid. Treating bytes as uncompressed."),
                    HE_KV(field_name, field.GetName()),
                    HE_KV(field_type, field.GetMeta().GetNormal().GetType().GetData().GetUnionTag()));
            }
            else if (maxSize == 0)
            {
                out.Clear();
            }
            else
            {
                Vector<uint8_t> decompressed;
                decompressed.Resize(static_cast<uint32_t>(maxSize), DefaultInit);

                const size_t result = ZSTD_decompress(decompressed.Data(), decompressed.Size(), out.Data(), out.Size());
                if (ZSTD_isError(result) || result > maxSize)
                {
                    HE_LOG_WARN(he_schema,
                        HE_MSG("Blob field is marked as zstd compressed, but the decompression routine failed. Treating bytes as uncompressed."),
                        HE_KV(field_name, field.GetName()),
                        HE_KV(field_type, field.GetMeta().GetNormal().GetType().GetData().GetUnionTag()),
                        HE_KV(zstd_result, result));
                }
                else
                {
                    decompressed.Resize(static_cast<uint32_t>(result));
                    out = Move(decompressed);
                }
            }
        }

        return true;
    }

    static bool ParseBlobString(Vector<uint8_t>& out, const Field::Reader& field, const StringView& value)
    {
        const Attribute::Reader b64Attr = FindAttribute<Toml::Base64>(field.GetAttributes());
        const Attribute::Reader hexAttr = FindAttribute<Toml::Hex>(field.GetAttributes());

        if (hexAttr.IsValid() && !b64Attr.IsValid())
        {
            Vector<uint8_t> bytes;
            return ParseBlobHexString(bytes, field, value);
        }

        const bool useZstd = b64Attr.IsValid() ? b64Attr.GetValue().GetData().GetEnum() == AsUnderlyingType(Toml::Compression::Zstd) : false;
        return ParseBlobBase64String(out, field, useZstd, value);
    }

    // StructBuilder setters and getters
    template <typename T>
    static void SetDataValue(StructBuilder& builder, uint32_t index, uint32_t dataOffset, T value)
    {
        builder.SetAndMarkDataField(static_cast<uint16_t>(index), dataOffset, value);
    }

    template <typename T>
    static void SetPointerValue(StructBuilder& builder, uint32_t index, const T& value)
    {
        builder.GetPointerField(static_cast<uint16_t>(index)).Set(value);
    }

    // ListBuilder setters and getters
    template <typename T>
    static void SetDataValue(ListBuilder& builder, uint32_t index, uint32_t dataOffset, T value)
    {
        HE_UNUSED(dataOffset);
        builder.SetDataElement(index, value);
    }

    template <typename T>
    static void SetPointerValue(ListBuilder& builder, uint32_t index, const T& value)
    {
        builder.SetPointerElement(index, value);
    }

    // ArrayBuilder setters and getters
    struct ArrayBuilder { StructBuilder& st; uint16_t index; uint16_t size; };

    template <typename T>
    static void SetDataValue(ArrayBuilder& builder, uint32_t index, uint32_t dataOffset, T value)
    {
        using Container = typename _ReadDataArrayReturnType<T>::Type;
        Container data = builder.st.GetAndMarkDataArrayField<T>(static_cast<uint16_t>(index), dataOffset, builder.size);
        data[builder.index] = value;
    }

    template <typename T>
    static void SetPointerValue(ArrayBuilder& builder, uint32_t index, const T& value)
    {
        builder.st.GetPointerArrayField(static_cast<uint16_t>(index), builder.size).SetPointerElement(builder.index, value);
    }

    // --------------------------------------------------------------------------------------------
    class SchemaTomlReader final
    {
    public:
        SchemaTomlReader(Builder& dst)
            : m_dst(dst)
            , m_reader(dst.GetAllocator())
        {}

        bool Read(StringView data, const DeclInfo& info)
        {
            TomlDocument doc;
            const TomlReadResult result = doc.Read(data);
            if (!result)
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Failed to parse TOML document. Deserialization is not possible."),
                    HE_KV(error, result.error),
                    HE_KV(line, result.line),
                    HE_KV(column, result.column));
                return false;
            }

            if (!doc.Root().IsTable())
            {
                HE_LOGF_WARN(he_schema, "Root of the TOML document is not a table. Deserialization is not possible.");
                return false;
            }

            PushGroup(&info);
            StructBuilder rootBuilder = m_stack.Back().builder;
            SetStructValue(rootBuilder, doc.Root());
            m_dst.SetRoot(rootBuilder);
            PopGroup();
            return true;
        }

    private:
        bool PushField(const he::String& name)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();

            m_currentField = {};
            for (const Field::Reader field : structDecl.GetFields())
            {
                const Attribute::Reader nameAttr = FindAttribute<Toml::Name>(field.GetAttributes());
                const String::Reader fieldName = nameAttr.IsValid() ? nameAttr.GetValue().GetData().GetString() : field.GetName();

                if (fieldName == name)
                {
                    m_currentField = field;
                    break;
                }
            }

            if (!m_currentField.IsValid())
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Encountered unknown field name. Deserialization of field is not possible."),
                    HE_KV(field_name, name),
                    HE_KV(decl_name, decl.GetName()),
                    HE_KV(decl_id, decl.GetId()));
                m_currentField = {};
                return false;
            }

            const Field::Meta::UnionTag fieldUnionTag = m_currentField.GetMeta().GetUnionTag();
            switch (fieldUnionTag)
            {
                case Field::Meta::UnionTag::Normal:
                    return PushNormalField(name);

                case Field::Meta::UnionTag::Group:
                    PushGroup(m_currentField.GetMeta().GetGroup());
                    return true;

                case Field::Meta::UnionTag::Union:
                    PushGroup(m_currentField.GetMeta().GetUnion());
                    return true;
            }

            HE_LOG_WARN(he_schema,
                HE_MSG("Encountered unknown field tag. Deserialization of this field is not possible."),
                HE_KV(field_name, name),
                HE_KV(field_tag, AsUnderlyingType(fieldUnionTag)),
                HE_KV(decl_name, decl.GetName()),
                HE_KV(decl_id, decl.GetId()));
            m_currentField = {};
            return false;
        }

        bool PushNormalField(const he::String& name)
        {
            const Declaration::Reader decl = m_stack.Back().decl;
            const Field::Meta::Normal::Reader normalField = m_currentField.GetMeta().GetNormal();
            const Type::Reader type = normalField.GetType();
            const Type::Data::Reader typeData = type.GetData();
            const Type::Data::UnionTag typeDataTag = typeData.GetUnionTag();

            switch (typeDataTag)
            {
                case Type::Data::UnionTag::Bool:
                case Type::Data::UnionTag::Int8:
                case Type::Data::UnionTag::Int16:
                case Type::Data::UnionTag::Int32:
                case Type::Data::UnionTag::Int64:
                case Type::Data::UnionTag::Uint8:
                case Type::Data::UnionTag::Uint16:
                case Type::Data::UnionTag::Uint32:
                case Type::Data::UnionTag::Uint64:
                case Type::Data::UnionTag::Float32:
                case Type::Data::UnionTag::Float64:
                case Type::Data::UnionTag::Blob:
                case Type::Data::UnionTag::String:
                case Type::Data::UnionTag::Enum:
                    // Normal primitive fields do not push a new group on the stack.
                    return false;

                case Type::Data::UnionTag::Array:
                case Type::Data::UnionTag::List:
                    // List and array fields do not push a new group on the stack.
                    return false;

                case Type::Data::UnionTag::Void:
                case Type::Data::UnionTag::Interface:
                case Type::Data::UnionTag::Parameter:
                {
                    HE_LOG_WARN(he_schema,
                        HE_MSG("Encountered unexpected field type. Deserialization of this field is not possible."),
                        HE_KV(field_name, name),
                        HE_KV(field_type, typeDataTag),
                        HE_KV(decl_name, decl.GetName()),
                        HE_KV(decl_id, decl.GetId()));
                    m_currentField = {};
                    return false;
                }

                case Type::Data::UnionTag::Struct:
                {
                    PushGroup(typeData.GetStruct().GetId());
                    return true;
                }

                case Type::Data::UnionTag::AnyPointer:
                case Type::Data::UnionTag::AnyStruct:
                case Type::Data::UnionTag::AnyList:
                {
                    HE_LOG_WARN(he_schema,
                        HE_MSG("Encountered any-pointer field type. Deserialization of this field is not possible."),
                        HE_KV(field_name, name),
                        HE_KV(field_type, typeDataTag),
                        HE_KV(decl_name, decl.GetName()),
                        HE_KV(decl_id, decl.GetId()));
                    m_currentField = {};
                    return false;
                }
            }

            HE_LOG_WARN(he_schema,
                HE_MSG("Encountered unknown normal field type. Deserialization of this field is not possible."),
                HE_KV(field_name, name),
                HE_KV(field_type, AsUnderlyingType(typeDataTag)),
                HE_KV(decl_name, decl.GetName()),
                HE_KV(decl_id, decl.GetId()));
            m_currentField = {};
            return false;
        }

        void PushGroup(const Field::Meta::Group::Reader groupField)
        {
            const Declaration::Reader decl = m_stack.Back().decl;

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
                HE_KV(field_name, m_currentField.GetName()),
                HE_KV(group_id, groupField.GetTypeId())))
            {
                return;
            }

            PushGroup(groupChild.GetId(), m_stack.Back().builder);
        }

        void PushGroup(const Field::Meta::Union::Reader unionField)
        {
            const Declaration::Reader decl = m_stack.Back().decl;

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
                HE_KV(field_name, m_currentField.GetName()),
                HE_KV(group_id, unionField.GetTypeId())))
            {
                return;
            }

            PushGroup(unionChild.GetId(), m_stack.Back().builder);
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
                    HE_KV(parent_name, m_stack.Back().decl.GetName()),
                    HE_KV(id, id));
                return;
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
                    HE_KV(parent_name, m_stack.Back().decl.GetName()),
                    HE_KV(type_id, id));
                return;
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

        static bool CanConvertTomlValue(const Type::Reader type, const TomlValue& value)
        {
            switch (type.GetData().GetUnionTag())
            {
                case Type::Data::UnionTag::Void:
                    return false;

                case Type::Data::UnionTag::Bool:
                    return value.IsBool() || value.IsInt() || value.IsUint();

                case Type::Data::UnionTag::Int8:
                case Type::Data::UnionTag::Int16:
                case Type::Data::UnionTag::Int32:
                case Type::Data::UnionTag::Int64:
                case Type::Data::UnionTag::Uint8:
                case Type::Data::UnionTag::Uint16:
                case Type::Data::UnionTag::Uint32:
                case Type::Data::UnionTag::Uint64:
                case Type::Data::UnionTag::Float32:
                case Type::Data::UnionTag::Float64:
                    return value.IsBool() || value.IsInt() || value.IsUint() || value.IsFloat() || value.IsDateTime() || value.IsTime();

                case Type::Data::UnionTag::Blob:
                    return value.IsString();

                case Type::Data::UnionTag::String:
                    return value.IsBool() || value.IsInt() || value.IsUint() || value.IsFloat() || value.IsDateTime() || value.IsTime() || value.IsString();

                case Type::Data::UnionTag::Array:
                case Type::Data::UnionTag::List:
                    // We can convert strings to lists of bytes
                    return value.IsString() || value.IsArray();

                case Type::Data::UnionTag::Enum:
                    return value.IsString() || value.IsUint();

                case Type::Data::UnionTag::Struct:
                    return value.IsTable();

                case Type::Data::UnionTag::AnyPointer:
                case Type::Data::UnionTag::AnyStruct:
                case Type::Data::UnionTag::AnyList:
                case Type::Data::UnionTag::Interface:
                case Type::Data::UnionTag::Parameter:
                    return false;
            }

            return false;
        }

        static bool ConvertToBool(const TomlValue& value)
        {
            switch (value.GetKind())
            {
                case TomlValue::Kind::Bool: return value.Bool();
                case TomlValue::Kind::Int: return static_cast<bool>(value.Int());
                case TomlValue::Kind::Uint: return static_cast<bool>(value.Uint());
                default: return false;
            }
        }

        template <typename T>
        static T ConvertToNumber(const TomlValue& value)
        {
            switch (value.GetKind())
            {
                case TomlValue::Kind::Bool: return static_cast<T>(value.Bool());
                case TomlValue::Kind::Int: return static_cast<T>(value.Int());
                case TomlValue::Kind::Uint: return static_cast<T>(value.Uint());
                case TomlValue::Kind::Float: return static_cast<T>(value.Float());
                case TomlValue::Kind::DateTime: return static_cast<T>(value.DateTime().val);
                case TomlValue::Kind::Time: return static_cast<T>(value.Time().val);
                default: return T{};
            }
        }

        static String::Builder ConvertToString(Builder& dst, const TomlValue& value)
        {
            if (value.IsString())
                return dst.AddString(value.String());

            switch (value.GetKind())
            {
                case TomlValue::Kind::Bool: return dst.AddString(Format("{}", value.Bool()));
                case TomlValue::Kind::Int: return dst.AddString(Format("{}", value.Int()));
                case TomlValue::Kind::Uint: return dst.AddString(Format("{}", value.Uint()));
                case TomlValue::Kind::Float: return dst.AddString(Format("{}", value.Float()));
                case TomlValue::Kind::DateTime: return dst.AddString(Format("{}", value.DateTime()));
                case TomlValue::Kind::Time: return dst.AddString(Format("{}", value.Time()));
                default: return {};
            }
        }

        void SetValue(StructBuilder& builder, const TomlValue& value)
        {
            if (!m_currentField.IsValid())
                return;

            switch (m_currentField.GetMeta().GetUnionTag())
            {
                case Field::Meta::UnionTag::Normal:
                {
                    const Field::Meta::Normal::Reader norm = m_currentField.GetMeta().GetNormal();
                    const Type::Reader type = norm.GetType();
                    const Type::Data::Reader typeData = type.GetData();
                    const Type::Data::UnionTag typeDataTag = typeData.GetUnionTag();

                    if (CanConvertTomlValue(type, value))
                    {
                        SetNormalValue(builder, type, norm.GetIndex(), norm.GetDataOffset(), value);
                    }
                    else
                    {
                        HE_LOG_WARN(he_schema,
                            HE_MSG("Skipping field when parsing TOML because the value type cannot be converted to field type."),
                            HE_KV(value_type, value.GetKind()),
                            HE_KV(field_name, m_currentField.GetName()),
                            HE_KV(field_type, typeDataTag),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName()));
                    }
                    break;
                }
                case Field::Meta::UnionTag::Group:
                {
                    SetStructValue(builder, value);
                    break;
                }
                case Field::Meta::UnionTag::Union:
                {
                    SetUnionValue(builder, value);
                    break;
                }
            }
        }

        template <typename B>
        void SetNormalValue(B& builder, const Type::Reader fieldType, uint32_t index, uint32_t dataOffset, const TomlValue& value)
        {
            HE_ASSERT(m_currentField.IsValid());
            const Type::Data::Reader typeData = fieldType.GetData();
            const Type::Data::UnionTag typeDataTag = typeData.GetUnionTag();

            switch (typeDataTag)
            {
                case Type::Data::UnionTag::Void: break;
                case Type::Data::UnionTag::Bool: SetDataValue(builder, index, dataOffset, ConvertToBool(value)); break;
                case Type::Data::UnionTag::Int8: SetDataValue(builder, index, dataOffset, ConvertToNumber<int8_t>(value)); break;
                case Type::Data::UnionTag::Int16: SetDataValue(builder, index, dataOffset, ConvertToNumber<int16_t>(value)); break;
                case Type::Data::UnionTag::Int32: SetDataValue(builder, index, dataOffset, ConvertToNumber<int32_t>(value)); break;
                case Type::Data::UnionTag::Int64: SetDataValue(builder, index, dataOffset, ConvertToNumber<int64_t>(value)); break;
                case Type::Data::UnionTag::Uint8: SetDataValue(builder, index, dataOffset, ConvertToNumber<uint8_t>(value)); break;
                case Type::Data::UnionTag::Uint16: SetDataValue(builder, index, dataOffset, ConvertToNumber<uint16_t>(value)); break;
                case Type::Data::UnionTag::Uint32: SetDataValue(builder, index, dataOffset, ConvertToNumber<uint32_t>(value)); break;
                case Type::Data::UnionTag::Uint64: SetDataValue(builder, index, dataOffset, ConvertToNumber<uint64_t>(value)); break;
                case Type::Data::UnionTag::Float32: SetDataValue(builder, index, dataOffset, ConvertToNumber<float>(value)); break;
                case Type::Data::UnionTag::Float64: SetDataValue(builder, index, dataOffset, ConvertToNumber<double>(value)); break;
                case Type::Data::UnionTag::String: SetPointerValue(builder, index, ConvertToString(m_dst, value)); break;
                case Type::Data::UnionTag::Array:
                {
                    if constexpr (!IsSame<Decay<B>, StructBuilder>)
                    {
                        HE_LOG_WARN(he_schema,
                            HE_MSG("Skipping array field when parsing TOML. Arrays cannot be element types."),
                            HE_KV(field_name, m_currentField.GetName()),
                            HE_KV(field_type, typeDataTag),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName()),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                    }
                    else if (value.IsString())
                    {
                        SetArrayValueFromString(builder, value.String());
                    }
                    else
                    {
                        SetArrayValue(builder, value.Array());
                    }
                    break;
                }

                case Type::Data::UnionTag::List:
                {
                    if constexpr (IsSame<Decay<B>, ListBuilder>)
                    {
                        HE_LOG_WARN(he_schema,
                            HE_MSG("Skipping list field when parsing TOML. Lists of lists are not supported."),
                            HE_KV(field_name, m_currentField.GetName()),
                            HE_KV(field_type, typeDataTag),
                            HE_KV(parent_id, m_stack.Back().decl.GetId()),
                            HE_KV(parent_name, m_stack.Back().decl.GetName()),
                            HE_KV(index, index),
                            HE_KV(data_offset, dataOffset));
                    }
                    else if (value.IsString())
                    {
                        SetListValueFromString(builder, value.String());
                    }
                    else
                    {
                        SetListValue(builder, value.Array());
                    }
                    break;
                }
                case Type::Data::UnionTag::Enum:
                {
                    SetEnumValue(builder, index, dataOffset, value);
                    break;
                }
                case Type::Data::UnionTag::Blob:
                {
                    Vector<uint8_t> bytes;
                    if (ParseBlobString(bytes, m_currentField, value.String()))
                    {
                        Blob::Builder blob = m_dst.AddBlob(bytes);
                        SetPointerValue(builder, index, blob);
                    }
                    break;
                }
                case Type::Data::UnionTag::Struct:
                {
                    const Type::Data::Struct::Reader structType = typeData.GetStruct();

                    StructBuilder structBuilder;
                    if constexpr (IsSame<Decay<B>, ListBuilder>)
                    {
                        structBuilder = builder.GetCompositeElement(index);
                        PushGroup(structType.GetId(), structBuilder);
                    }
                    else
                    {
                        PushGroup(structType.GetId());
                        structBuilder = m_stack.Back().builder;
                    }

                    SetStructValue(structBuilder, value);

                    if constexpr (!IsSame<Decay<B>, ListBuilder>)
                    {
                        SetPointerValue(builder, index, structBuilder);
                    }

                    PopGroup();
                    break;
                }
                case Type::Data::UnionTag::Interface:
                case Type::Data::UnionTag::AnyPointer:
                case Type::Data::UnionTag::AnyStruct:
                case Type::Data::UnionTag::AnyList:
                case Type::Data::UnionTag::Parameter:
                {
                    HE_LOG_WARN(he_schema,
                        HE_MSG("Skipping field when parsing TOML. The field's type cannot be represented in TOML."),
                        HE_KV(field_name, m_currentField.GetName()),
                        HE_KV(field_type, typeDataTag),
                        HE_KV(parent_id, m_stack.Back().decl.GetId()),
                        HE_KV(parent_name, m_stack.Back().decl.GetName()),
                        HE_KV(index, index),
                        HE_KV(data_offset, dataOffset));
                    break;
                }
            }
        }

        void SetUnionValue(StructBuilder& builder, const TomlValue& value)
        {
            HE_ASSERT(m_currentField.IsValid());
            const Declaration::Reader decl = m_stack.Back().decl;
            const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();

            if (!value.IsTable())
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Encountered union field that isn't a table. Deserialization of field is not possible."),
                    HE_KV(field_name, m_currentField.GetName()),
                    HE_KV(decl_name, decl.GetName()),
                    HE_KV(decl_id, decl.GetId()));
                return;
            }

            const TomlValue* unionTagValue = value.Table().Find(SchemaUnionTagName);
            if (!unionTagValue || !unionTagValue->IsUint())
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Encountered union field without a schema tag. Deserialization of field is not possible."),
                    HE_KV(field_name, m_currentField.GetName()),
                    HE_KV(decl_name, decl.GetName()),
                    HE_KV(decl_id, decl.GetId()));
                return;
            }

            const uint16_t unionTag = static_cast<uint16_t>(unionTagValue->Uint());
            builder.SetDataField<uint16_t>(structDecl.GetUnionTagOffset(), unionTag);

            for (Field::Reader field : structDecl.GetFields())
            {
                if (field.GetUnionTag() == unionTag)
                {
                    const StringView name = field.GetName();
                    const TomlValue* fieldValue = value.Table().Find(name);

                    if (fieldValue)
                    {
                        const bool newGroup = PushField(name);

                        if (m_currentField.IsValid())
                            SetValue(builder, *fieldValue);

                        if (newGroup)
                            PopGroup();
                    }
                    break;
                }
            }
        }

        void SetArrayValueFromString(StructBuilder& builder, StringView str)
        {
            const Field::Meta::Normal::Reader norm = m_currentField.GetMeta().GetNormal();
            const Type::Reader type = norm.GetType();
            const Type::Data::Array::Reader arrayType = type.GetData().GetArray();
            const Type::Reader elementType = arrayType.GetElementType();
            const Type::Data::UnionTag elementTypeDataTag = elementType.GetData().GetUnionTag();

            const uint16_t index = norm.GetIndex();
            const uint32_t dataOffset = norm.GetDataOffset();

            if (elementTypeDataTag != Type::Data::UnionTag::Uint8)
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Arrays can only be deserialized from strings if their element type is uint8. Skipping deserialization of field."),
                    HE_KV(field_name, m_currentField.GetName()),
                    HE_KV(field_type, Type::Data::UnionTag::Array),
                    HE_KV(field_element_type, elementTypeDataTag),
                    HE_KV(parent_id, m_stack.Back().decl.GetId()),
                    HE_KV(parent_name, m_stack.Back().decl.GetName()));
                return;
            }

            Vector<uint8_t> bytes;
            if (ParseBlobString(bytes, m_currentField, str))
            {
                const uint16_t size = arrayType.GetSize();
                Span<uint8_t> data = builder.GetAndMarkDataArrayField<uint8_t>(index, dataOffset, size);
                MemCopy(data.Data(), bytes.Data(), Min(data.Size(), bytes.Size()));
            }
        }

        void SetArrayValue(StructBuilder& builder, const TomlValue::ArrayType& array)
        {
            const Field::Meta::Normal::Reader norm = m_currentField.GetMeta().GetNormal();
            const Type::Reader type = norm.GetType();
            const Type::Data::Array::Reader arrayType = type.GetData().GetArray();
            const Type::Reader elementType = arrayType.GetElementType();
            const Type::Data::UnionTag elementTypeDataTag = elementType.GetData().GetUnionTag();

            const uint16_t index = norm.GetIndex();
            const uint32_t dataOffset = norm.GetDataOffset();
            const uint16_t size = arrayType.GetSize();

            const bool isPointer = IsPointer(elementType);

            for (uint32_t i = 0; i < array.Size(); ++i)
            {
                const TomlValue& value = array[i];

                if (!CanConvertTomlValue(elementType, value))
                {
                    HE_LOG_WARN(he_schema,
                        HE_MSG("Skipping array element when parsing TOML because the value type cannot be converted to element type."),
                        HE_KV(value_type, value.GetKind()),
                        HE_KV(field_name, m_currentField.GetName()),
                        HE_KV(field_type, Type::Data::UnionTag::List),
                        HE_KV(field_element_type, elementTypeDataTag),
                        HE_KV(parent_id, m_stack.Back().decl.GetId()),
                        HE_KV(parent_name, m_stack.Back().decl.GetName()),
                        HE_KV(list_index, i));
                    continue;
                }

                ArrayBuilder b{ builder, static_cast<uint16_t>(i), size };

                if (isPointer)
                    SetNormalValue(b, elementType, index + i, dataOffset, value);
                else
                    SetNormalValue(b, elementType, index, dataOffset + i, value);
            }
        }

        template <typename B>
        void SetListValueFromString(B& builder, StringView str)
        {
            const Field::Meta::Normal::Reader norm = m_currentField.GetMeta().GetNormal();
            const Type::Reader type = norm.GetType();
            const Type::Data::List::Reader listType = type.GetData().GetList();
            const Type::Reader elementType = listType.GetElementType();
            const Type::Data::UnionTag elementTypeDataTag = elementType.GetData().GetUnionTag();

            const uint16_t index = norm.GetIndex();

            if (elementTypeDataTag != Type::Data::UnionTag::Uint8)
            {
                HE_LOG_WARN(he_schema,
                    HE_MSG("Lists can only be deserialized from strings if their element type is uint8. Skipping deserialization of field."),
                    HE_KV(field_name, m_currentField.GetName()),
                    HE_KV(field_type, Type::Data::UnionTag::List),
                    HE_KV(field_element_type, elementTypeDataTag),
                    HE_KV(parent_id, m_stack.Back().decl.GetId()),
                    HE_KV(parent_name, m_stack.Back().decl.GetName()));
                return;
            }

            Vector<uint8_t> bytes;
            if (ParseBlobString(bytes, m_currentField, str))
            {
                ListBuilder list = m_dst.AddList<uint8_t>(bytes.Size());
                MemCopy(list.Data(), bytes.Data(), bytes.Size());
                SetPointerValue(builder, index, list);
            }
        }

        template <typename B>
        void SetListValue(B& builder, const TomlValue::ArrayType& array)
        {
            const Field::Meta::Normal::Reader norm = m_currentField.GetMeta().GetNormal();
            const Type::Reader type = norm.GetType();
            const Type::Data::List::Reader listType = type.GetData().GetList();
            const Type::Reader elementType = listType.GetElementType();
            const Type::Data::UnionTag elementTypeDataTag = elementType.GetData().GetUnionTag();

            ListBuilder list;

            if (elementTypeDataTag == Type::Data::UnionTag::Struct)
            {
                const Type::Data::Struct::Reader structType = elementType.GetData().GetStruct();
                PushGroup(structType.GetId());
                const DeclInfo* info = m_stack.Back().info;
                list = m_dst.AddStructList(array.Size(), info->dataFieldCount, info->dataWordSize, info->pointerCount);
                PopGroup();
            }
            else
            {
                list = m_dst.AddList(GetTypeElementSize(elementType), array.Size());
            }
            SetPointerValue(builder, norm.GetIndex(), list);

            for (uint32_t i = 0; i < array.Size(); ++i)
            {
                const TomlValue& value = array[i];

                if (!CanConvertTomlValue(elementType, value))
                {
                    HE_LOG_WARN(he_schema,
                        HE_MSG("Skipping list element when parsing TOML because the value type cannot be converted to element type."),
                        HE_KV(value_type, value.GetKind()),
                        HE_KV(field_name, m_currentField.GetName()),
                        HE_KV(field_type, Type::Data::UnionTag::List),
                        HE_KV(field_element_type, elementTypeDataTag),
                        HE_KV(parent_id, m_stack.Back().decl.GetId()),
                        HE_KV(parent_name, m_stack.Back().decl.GetName()),
                        HE_KV(list_index, i));
                    continue;
                }

                SetNormalValue(list, elementType, i, 0, value);
            }
        }

        template <typename B>
        void SetEnumValue(B& builder, uint32_t index, uint32_t dataOffset, const TomlValue& value)
        {
            const Field::Meta::Normal::Reader norm = m_currentField.GetMeta().GetNormal();
            const Type::Reader type = norm.GetType();
            const Type::Data::Enum::Reader enumType = type.GetData().GetEnum();

            PushGroup(enumType.GetId());
            const Declaration::Data::Enum::Reader enumDecl = m_stack.Back().decl.GetData().GetEnum();

            if (value.IsUint())
            {
                if (value.Uint() < enumDecl.GetEnumerators().Size())
                {
                    SetDataValue(builder, index, dataOffset, static_cast<uint16_t>(value.Uint()));
                }
                else
                {
                    HE_LOG_WARN(he_schema,
                        HE_MSG("Skipping enum field when parsing TOML. The value does not map to a valid enumerator."),
                        HE_KV(value, value.Uint()),
                        HE_KV(field_name, m_currentField.GetName()),
                        HE_KV(field_type, Type::Data::UnionTag::Enum),
                        HE_KV(parent_id, m_stack.Back().decl.GetId()),
                        HE_KV(parent_name, m_stack.Back().decl.GetName()),
                        HE_KV(max_enumerator, enumDecl.GetEnumerators().Size() - 1),
                        HE_KV(index, index),
                        HE_KV(data_offset, dataOffset));
                }
            }
            else
            {
                const StringView& enumName = value.String();
                bool found = false;
                for (Enumerator::Reader e : enumDecl.GetEnumerators())
                {
                    const Attribute::Reader nameAttr = FindAttribute<Toml::Name>(e.GetAttributes());
                    const String::Reader name = nameAttr.IsValid() ? nameAttr.GetValue().GetData().GetString() : e.GetName();

                    if (name == enumName)
                    {
                        SetDataValue(builder, index, dataOffset, e.GetOrdinal());
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    HE_LOG_WARN(he_schema,
                        HE_MSG("Skipping enum field when parsing TOML. The value does not map to a valid enumerator."),
                        HE_KV(value, enumName),
                        HE_KV(field_name, m_currentField.GetName()),
                        HE_KV(field_type, Type::Data::UnionTag::Enum),
                        HE_KV(parent_id, m_stack.Back().decl.GetId()),
                        HE_KV(parent_name, m_stack.Back().decl.GetName()),
                        HE_KV(index, index),
                        HE_KV(data_offset, dataOffset));
                }
            }

            PopGroup();
        }

        void SetStructValue(StructBuilder& builder, const TomlValue& value)
        {
            HE_ASSERT(value.IsTable());

            for (auto&& it : value.Table())
            {
                const he::String& key = it.key;
                const TomlValue& val = it.value;
                const bool newGroup = PushField(key);

                if (m_currentField.IsValid())
                    SetValue(builder, val);

                if (newGroup)
                    PopGroup();
            }
        }

    private:
        struct Context
        {
            const DeclInfo* info{ nullptr };
            Declaration::Reader decl{};
            StructBuilder builder{};
        };

    private:
        Builder& m_dst;
        TomlReader m_reader;

        Field::Reader m_currentField{};

        Vector<Context> m_stack;
        he::String m_buffer;
    };

    // --------------------------------------------------------------------------------------------
    class SchemaTomlWriter final : private StructVisitor
    {
    public:
        SchemaTomlWriter(he::String& dst)
            : m_writer(dst)
        {
            m_writer.Reserve(8192); // 8 KB
        }

        void Write(StructReader data, const DeclInfo& info)
        {
            // Call parent visit, so we don't write a header for the root struct
            StructVisitor::VisitStruct(data, info);
        }

        void Write(DynamicValue::Reader data)
        {
            WriteDynamicValue(data);
        }

    private:
        void BeginTable()
        {
            const Attribute::Reader tomlName = FindAttribute<Toml::Name>(m_currentField.GetAttributes());
            const StringView name = tomlName.IsValid() ? tomlName.GetValue().GetData().GetString() : m_currentField.GetName();
            m_keyStack.PushBack(name);

            if (m_arrayDepth > 0)
            {
                m_writer.StartInlineTable();
            }
            else
            {
                m_writer.Table(m_keyStack, m_nextTableIsArray);
                m_writer.IncreaseIndent();
            }

            m_nextTableIsArray = false;
        }

        void EndTable()
        {
            if (m_arrayDepth > 0)
                m_writer.EndInlineTable();
            else
                m_writer.DecreaseIndent();

            m_keyStack.PopBack();
        }

        void VisitStruct(StructReader data, const DeclInfo& info)
        {
            const Declaration::Reader decl = GetSchema(info);

            BeginTable();
            StructVisitor::VisitStruct(data, info);
            EndTable();
        }

        void VisitNormalField(StructReader data, Field::Reader field, const DeclInfo& scope) override
        {
            const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
            Type::Data::Reader typeData = norm.GetType().GetData();

            if (typeData.IsArray() || typeData.IsList())
                typeData = typeData.IsArray() ? typeData.GetArray().GetElementType().GetData() : typeData.GetList().GetElementType().GetData();

            if (!typeData.IsStruct())
            {
                const Attribute::Reader nameAttr = FindAttribute<Toml::Name>(field.GetAttributes());
                const StringView name = nameAttr.IsValid() ? nameAttr.GetValue().GetData().GetString() : field.GetName();
                m_writer.Key(name);
            }

            m_currentField = field;
            StructVisitor::VisitNormalField(data, field, scope);
        }

        void VisitGroupField(StructReader data, Field::Reader field, const DeclInfo& scope) override
        {
            m_currentField = field;
            StructVisitor::VisitGroupField(data, field, scope);
        }

        void VisitUnionField(StructReader data, Field::Reader field, const DeclInfo& scope) override
        {
            const DeclInfo* info = FindGroupOrUnionInfo(field, scope);
            if (!info)
                return;

            const Declaration::Reader decl = GetSchema(*info);
            const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
            const List<Field>::Reader fields = structDecl.GetFields();

            const uint16_t activeFieldTag = data.GetDataField<uint16_t>(structDecl.GetUnionTagOffset());

            m_currentField = field;
            BeginTable();
            m_writer.Key(SchemaUnionTagName);
            m_writer.Uint(activeFieldTag);

            for (const Field::Reader unionField : fields)
            {
                if (unionField.GetUnionTag() == activeFieldTag)
                {
                    m_currentField = unionField;
                    VisitField(data, unionField, *info);
                    break;
                }
            }

            EndTable();
        }

        void VisitValue(bool value, Type::Reader, const DeclInfo&) override { m_writer.Bool(value); }
        void VisitValue(int8_t value, Type::Reader, const DeclInfo&) override { WriteIntValue(value); }
        void VisitValue(int16_t value, Type::Reader, const DeclInfo&) override { WriteIntValue(value); }
        void VisitValue(int32_t value, Type::Reader, const DeclInfo&) override { WriteIntValue(value); }
        void VisitValue(int64_t value, Type::Reader, const DeclInfo&) override { WriteIntValue(value); }
        void VisitValue(uint8_t value, Type::Reader, const DeclInfo&) override { WriteUintValue(value); }
        void VisitValue(uint16_t value, Type::Reader, const DeclInfo&) override { WriteUintValue(value); }
        void VisitValue(uint32_t value, Type::Reader, const DeclInfo&) override { WriteUintValue(value); }
        void VisitValue(uint64_t value, Type::Reader, const DeclInfo&) override { WriteUintValue(value); }
        void VisitValue(float value, Type::Reader, const DeclInfo&) override { WriteFloatValue(value); }
        void VisitValue(double value, Type::Reader, const DeclInfo&) override { WriteFloatValue(value); }
        void VisitValue(Blob::Reader value, Type::Reader, const DeclInfo&) override { WriteBlobValue(value); }

        void VisitValue(String::Reader value, Type::Reader type, const DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);
            const bool useLiteral = HasAttribute<Toml::Literal>(m_currentField.GetAttributes());
            const TomlStringFormat format = useLiteral ? TomlStringFormat::Literal : TomlStringFormat::Basic;
            m_writer.String(value, format);
        }

        void VisitValue(EnumValueTag, uint16_t value, Type::Reader type, const DeclInfo& scope) override
        {
            const Declaration::Reader decl = GetSchema(scope);
            const Type::Data::Enum::Reader enumType = type.GetData().GetEnum();
            const DeclInfo* enumInfo = FindDependency(scope, enumType.GetId());

            if (!HE_VERIFY(enumInfo,
                HE_MSG("Invalid schema. No dependency of the parent scope matches the enum's type id."),
                HE_KV(field_name, m_currentField.GetName()),
                HE_KV(field_type, m_currentField.GetMeta().GetNormal().GetType().GetData().GetUnionTag()),
                HE_KV(parent_id, decl.GetId()),
                HE_KV(parent_name, decl.GetName()),
                HE_KV(key_stack, m_keyStack),
                HE_KV(enum_type_id, enumType.GetId())))
            {
                return;
            }

            const bool found = WriteEnumValue(value, *enumInfo);

            // Should never happen
            if (!HE_VERIFY(found,
                HE_MSG("Invalid enum value. No enumerator exists in the schema with that value."),
                HE_KV(field_name, m_currentField.GetName()),
                HE_KV(field_type, m_currentField.GetMeta().GetNormal().GetType().GetData().GetUnionTag()),
                HE_KV(parent_id, decl.GetId()),
                HE_KV(parent_name, decl.GetName()),
                HE_KV(key_stack, m_keyStack),
                HE_KV(enum_value, value)))
            {
                m_writer.String("");
            }
        }

        void VisitArrayValue(StructReader data, Type::Reader elementType, uint16_t index, uint32_t dataOffset, uint16_t size, const DeclInfo& scope) override
        {
            if (elementType.GetData().IsUint8())
            {
                const List<Attribute>::Reader attributes = m_currentField.GetAttributes();
                if (HasAttribute<Toml::Base64>(attributes) || HasAttribute<Toml::Hex>(attributes))
                {
                    const Span<const uint8_t> value = data.TryGetDataArrayField<uint8_t>(index, dataOffset, size);
                    WriteBlobValue(value);
                    return;
                }
            }

            const bool isPointer = IsPointer(elementType);
            const bool isStruct = elementType.GetData().IsStruct();

            if (!isStruct)
            {
                m_writer.StartArray();
                ++m_arrayDepth;
            }

            for (uint16_t i = 0; i < size; ++i)
            {
                Field::Reader oldField = m_currentField;
                m_nextTableIsArray = true;
                if (isPointer)
                {
                    StructVisitor::VisitValue(data, elementType, index + i, dataOffset, scope);
                }
                else
                {
                    StructVisitor::VisitValue(data, elementType, index, dataOffset + i, scope);
                }
                m_nextTableIsArray = false;
                m_currentField = oldField;
            }

            if (!isStruct)
            {
                --m_arrayDepth;
                m_writer.EndArray();
            }

        }

        void VisitListValue(ListReader data, Type::Reader elementType, const DeclInfo& scope) override
        {
            if (elementType.GetData().IsUint8())
            {
                const List<Attribute>::Reader attributes = m_currentField.GetAttributes();
                if (HasAttribute<Toml::Base64>(attributes) || HasAttribute<Toml::Hex>(attributes))
                {
                    const uint8_t* value = reinterpret_cast<const uint8_t*>(data.Data());
                    WriteBlobValue({ value, data.Size() });
                    return;
                }
            }

            const uint32_t size = data.Size();

            if (!elementType.GetData().IsStruct())
            {
                m_writer.StartArray();
                ++m_arrayDepth;
            }

            for (uint32_t i = 0; i < size; ++i)
            {
                Field::Reader oldField = m_currentField;
                m_nextTableIsArray = true;
                StructVisitor::VisitValue(data, elementType, i, scope);
                m_nextTableIsArray = false;
                m_currentField = oldField;
            }

            if (!elementType.GetData().IsStruct())
            {
                --m_arrayDepth;
                m_writer.EndArray();
            }
        }

        void VisitAnyPointer(PointerReader ptr, Type::Reader type, const DeclInfo& scope) override
        {
            HE_UNUSED(ptr, type, scope);
            const Declaration::Reader decl = GetSchema(scope);
            HE_LOG_WARN(he_schema,
                HE_MSG("Skipping AnyPointer field when serializing to TOML."),
                HE_KV(field_name, m_currentField.GetName()),
                HE_KV(field_type, m_currentField.GetMeta().GetNormal().GetType().GetData().GetUnionTag()),
                HE_KV(parent_id, decl.GetId()),
                HE_KV(parent_name, decl.GetName()),
                HE_KV(key_stack, m_keyStack));
        }

        void VisitAnyStruct(PointerReader ptr, Type::Reader type, const DeclInfo& scope) override
        {
            HE_UNUSED(ptr, type, scope);
            const Declaration::Reader decl = GetSchema(scope);
            HE_LOG_WARN(he_schema,
                HE_MSG("Skipping AnyStruct field when serializing to TOML."),
                HE_KV(field_name, m_currentField.GetName()),
                HE_KV(field_type, m_currentField.GetMeta().GetNormal().GetType().GetData().GetUnionTag()),
                HE_KV(parent_id, decl.GetId()),
                HE_KV(parent_name, decl.GetName()),
                HE_KV(key_stack, m_keyStack));
        }

        void VisitAnyList(PointerReader ptr, Type::Reader type, const DeclInfo& scope) override
        {
            HE_UNUSED(ptr, type, scope);
            const Declaration::Reader decl = GetSchema(scope);
            HE_LOG_WARN(he_schema,
                HE_MSG("Skipping AnyList field when serializing to TOML."),
                HE_KV(field_name, m_currentField.GetName()),
                HE_KV(field_type, m_currentField.GetMeta().GetNormal().GetType().GetData().GetUnionTag()),
                HE_KV(parent_id, decl.GetId()),
                HE_KV(parent_name, decl.GetName()),
                HE_KV(key_stack, m_keyStack));
        }

    private:
        void WriteIntValue(int64_t value)
        {
            m_writer.Int(value);
        }

        void WriteUintValue(uint64_t value)
        {
            const List<Attribute>::Reader attributes = m_currentField.GetAttributes();

            TomlUintFormat format = TomlUintFormat::Decimal;
            if (HasAttribute<Toml::Binary>(attributes))
                format = TomlUintFormat::Binary;
            else if (HasAttribute<Toml::Hex>(attributes))
                format = TomlUintFormat::Hex;
            else if (HasAttribute<Toml::Octal>(attributes))
                format = TomlUintFormat::Octal;

            m_writer.Uint(value, format);
        }

        void WriteFloatValue(double value)
        {
            const List<Attribute>::Reader attributes = m_currentField.GetAttributes();

            TomlFloatFormat format = TomlFloatFormat::General;
            if (HasAttribute<Toml::Fixed>(attributes))
                format = TomlFloatFormat::Fixed;
            else if (HasAttribute<Toml::Exponent>(attributes))
                format = TomlFloatFormat::Exponent;

            const Attribute::Reader precisionAttr = FindAttribute<Toml::Precision>(attributes);
            const int32_t precision = precisionAttr.IsValid() ? precisionAttr.GetValue().GetData().GetInt32() : -1;

            m_writer.Float(value, format, precision);
        }

        void WriteBlobValue(Span<const uint8_t> value)
        {
            const Attribute::Reader b64Attr = FindAttribute<Toml::Base64>(m_currentField.GetAttributes());
            const Attribute::Reader hexAttr = FindAttribute<Toml::Hex>(m_currentField.GetAttributes());

            if (hexAttr.IsValid() && !b64Attr.IsValid())
            {
                he::String str = Format("{:02x}", FmtJoin(value, ""));
                m_writer.String(str);
                return;
            }
            const bool useZstd = b64Attr.IsValid() ? b64Attr.GetValue().GetData().GetEnum() == AsUnderlyingType(Toml::Compression::Zstd) : false;

            if (!useZstd)
            {
                he::String str = Base64Encode(value);
                m_writer.String(str);
                return;
            }

            const size_t maxSize = ZSTD_compressBound(value.Size());

            Vector<uint8_t> compressed;
            compressed.Resize(static_cast<uint32_t>(maxSize), DefaultInit);

            const size_t result = ZSTD_compress(compressed.Data(), compressed.Size(), value.Data(), value.Size(), ZSTD_CLEVEL_DEFAULT);
            if (!HE_VERIFY(!ZSTD_isError(result),
                HE_MSG("Failed to compress blob bytes using zstd"),
                HE_KV(field_name, m_currentField.GetName()),
                HE_KV(field_type, m_currentField.GetMeta().GetNormal().GetType().GetData().GetUnionTag()),
                HE_KV(zstd_result, result)))
            {
                m_writer.String("");
                return;
            }

            he::String str = Base64Encode(compressed);
            m_writer.String(str);
        }

        bool WriteEnumValue(uint16_t value, const DeclInfo& enumInfo)
        {
            const Declaration::Data::Enum::Reader enumDecl = GetSchema(enumInfo).GetData().GetEnum();

            for (Enumerator::Reader e : enumDecl.GetEnumerators())
            {
                if (e.GetOrdinal() == value)
                {
                    const Attribute::Reader nameAttr = FindAttribute<Toml::Name>(e.GetAttributes());
                    if (nameAttr.IsValid())
                        m_writer.String(nameAttr.GetValue().GetData().GetString());
                    else
                        m_writer.String(e.GetName());
                    return true;
                }
            }
            return false;
        }

        void WriteDynamicValue(DynamicValue::Reader data)
        {
            switch (data.GetKind())
            {
                case DynamicValue::Kind::Unknown: break;
                case DynamicValue::Kind::Void: break;
                case DynamicValue::Kind::Bool: m_writer.Bool(data.As<bool>()); break;
                case DynamicValue::Kind::Int: WriteIntValue(data.As<int64_t>()); break;
                case DynamicValue::Kind::Uint: WriteUintValue(data.As<uint64_t>()); break;
                case DynamicValue::Kind::Float: WriteFloatValue(data.As<double>()); break;
                case DynamicValue::Kind::String: m_writer.String(data.As<String>(), TomlStringFormat::Basic); break;
                case DynamicValue::Kind::Blob:
                {
                    const Blob::Reader value = data.As<Blob>();
                    WriteBlobValue({ value.Data(), value.Size() });
                    break;
                }
                case DynamicValue::Kind::Array:
                {

                    const DynamicArray::Reader value = data.As<DynamicArray>();
                    const Type::Data::Array::Reader arrayType = value.ArrayType();
                    const Type::Reader elementType = arrayType.GetElementType();

                    if (elementType.GetData().IsUint8())
                    {
                        const Span<const uint8_t> bytes = value.AsSpanOf<uint8_t>();
                        WriteBlobValue(bytes);
                    }
                    else
                    {
                        const bool isStruct = elementType.GetData().IsStruct();

                        if (!isStruct)
                        {
                            m_writer.StartArray();
                            ++m_arrayDepth;
                        }

                        for (uint16_t i = 0; i < arrayType.GetSize(); ++i)
                        {
                            Field::Reader oldField = m_currentField;
                            m_nextTableIsArray = true;
                            WriteDynamicValue(value.Get(i));
                            m_nextTableIsArray = false;
                            m_currentField = oldField;
                        }

                        if (!isStruct)
                        {
                            --m_arrayDepth;
                            m_writer.EndArray();
                        }
                    }
                    break;
                }
                case DynamicValue::Kind::List:
                {
                    const DynamicList::Reader value = data.As<DynamicList>();
                    const Type::Data::List::Reader listType = value.ListType();
                    const Type::Reader elementType = listType.GetElementType();

                    if (elementType.GetData().IsUint8())
                    {
                        const List<const uint8_t>::Reader bytes = value.AsListOf<const uint8_t>();
                        WriteBlobValue(bytes);
                    }
                    else
                    {
                        const uint32_t size = value.Size();
                        const bool isStruct = elementType.GetData().IsStruct();

                        if (!isStruct)
                        {
                            m_writer.StartArray();
                            ++m_arrayDepth;
                        }

                        for (uint32_t i = 0; i < size; ++i)
                        {
                            Field::Reader oldField = m_currentField;
                            m_nextTableIsArray = true;
                            StructVisitor::VisitValue(value.List(), elementType, i, value.Scope());
                            m_nextTableIsArray = false;
                            m_currentField = oldField;
                        }

                        if (!isStruct)
                        {
                            --m_arrayDepth;
                            m_writer.EndArray();
                        }
                    }
                    break;
                }
                case DynamicValue::Kind::Enum:
                {
                    const DynamicEnum value = data.As<DynamicEnum>();
                    WriteEnumValue(value.Value(), value.Decl());
                    break;
                }
                case DynamicValue::Kind::Struct:
                {
                    const DynamicStruct::Reader value = data.As<DynamicStruct>();
                    StructVisitor::VisitStruct(value.Struct(), value.Decl());
                    break;
                }
                case DynamicValue::Kind::AnyPointer:
                {
                    // TODO!
                    const AnyPointer::Reader value = data.As<AnyPointer>();
                    VisitAnyPointer(value, {}, {});
                    break;
                }
            }
        }

    private:
        TomlWriter m_writer;
        Vector<StringView> m_keyStack;

        Field::Reader m_currentField{};
        uint32_t m_arrayDepth{ 0 };
        bool m_nextTableIsArray{ false };
    };

    // --------------------------------------------------------------------------------------------
    void ToToml(he::String& dst, StructReader data, const DeclInfo& info)
    {
       SchemaTomlWriter writer(dst);
       return writer.Write(data, info);
    }

    void ToToml(he::String& dst, DynamicValue::Reader data)
    {
        SchemaTomlWriter writer(dst);
        return writer.Write(data);
    }

     bool FromToml(Builder& dst, StringView data, const DeclInfo& info)
     {
        SchemaTomlReader reader(dst);
        return reader.Read(data, info);
     }
}
