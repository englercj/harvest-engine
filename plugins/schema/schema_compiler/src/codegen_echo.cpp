// Copyright Chad Engler

#include "codegen_echo.h"

#include "he/core/string_fmt.h"
#include "he/core/string_view_fmt.h"

#include "fmt/format.h"
#include "fmt/ranges.h"

#include <iostream>

namespace he::schema
{
    #define HE_ID_FMT "@{:#018x}"

    CodeGenEcho::CodeGenEcho(const CodeGenRequest& request) noexcept
        : m_request(request)
    {}

    bool CodeGenEcho::Generate()
    {
        m_writer.Reserve(8192); // 8 KB

        Declaration::Reader root = m_request.schemaFile.GetRoot();

        if (root.HasSource())
            m_writer.WriteLine("// {}", root.GetSource().GetFile().AsView());

        m_writer.WriteLine(HE_ID_FMT ";", root.GetId());

        for (String::Reader importStr : root.GetData().GetFile().GetImports())
        {
            m_writer.WriteLine("import \"{}\";", importStr.AsView());
        }

        Visit(root);

        m_writer.Write('\0');
        std::cout << m_writer.Str().Data() << std::endl;

        return true;
    }

    bool CodeGenEcho::VisitFile(Declaration::Reader decl)
    {
        if (!decl.GetName().IsEmpty())
        {
            m_writer.WriteLine("namespace {};", decl.GetName().AsView());
        }

        for (Attribute::Reader attr : decl.GetAttributes())
        {
            WriteAttribute(attr, decl);
            m_writer.Write(";\n");
        }

        return SchemaVisitor::VisitFile(decl);
    }

    bool CodeGenEcho::VisitAttribute(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_ASSERT(decl.GetData().IsAttribute());
        Declaration::Data::Attribute::Reader attrDecl = decl.GetData().GetAttribute();

        he::String targetStr(Allocator::GetTemp());

        if (attrDecl.GetTargetsAttribute())
            targetStr += "attribute,";
        if (attrDecl.GetTargetsConstant())
            targetStr += "const,";
        if (attrDecl.GetTargetsEnum())
            targetStr += "enum,";
        if (attrDecl.GetTargetsEnumerator())
            targetStr += "enumerator,";
        if (attrDecl.GetTargetsField())
            targetStr += "field,";
        if (attrDecl.GetTargetsFile())
            targetStr += "file,";
        if (attrDecl.GetTargetsInterface())
            targetStr += "interface,";
        if (attrDecl.GetTargetsMethod())
            targetStr += "method,";
        if (attrDecl.GetTargetsParameter())
            targetStr += "parameter,";
        if (attrDecl.GetTargetsStruct())
            targetStr += "struct,";

        if (!targetStr.IsEmpty())
            targetStr.PopBack();

        m_writer.WriteIndent();
        m_writer.Write("attribute {} " HE_ID_FMT " ({}) :", decl.GetName().AsView(), decl.GetId(), targetStr);
        WriteType(attrDecl.GetType(), scope);
        WriteAttributes(decl.GetAttributes(), scope);
        m_writer.Write(";\n");

        return true;
    }

    bool CodeGenEcho::VisitConstant(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_ASSERT(decl.GetData().IsConstant());
        Declaration::Data::Constant::Reader constDecl = decl.GetData().GetConstant();

        m_writer.WriteIndent();
        m_writer.Write("const {} " HE_ID_FMT " :", decl.GetName().AsView(), decl.GetId());
        WriteType(constDecl.GetType(), scope);
        m_writer.Write(" = ");
        WriteValue(constDecl.GetType(), constDecl.GetValue(), scope);
        WriteAttributes(decl.GetAttributes(), scope);
        m_writer.Write(";\n");

        return true;
    }

    bool CodeGenEcho::VisitEnum(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_ASSERT(decl.GetData().IsEnum());

        m_writer.WriteIndent();
        m_writer.Write("enum {} " HE_ID_FMT, decl.GetName().AsView(), decl.GetId());
        WriteAttributes(decl.GetAttributes(), scope);
        m_writer.Write('\n');

        m_writer.WriteLine("{");
        m_writer.IncreaseIndent();

        if (!SchemaVisitor::VisitEnum(decl, scope))
            return false;

        m_writer.DecreaseIndent();
        m_writer.WriteLine("}");

        return true;
    }

    bool CodeGenEcho::VisitEnumerator(Enumerator::Reader enumerator, Declaration::Reader scope)
    {
        m_writer.WriteIndent();
        m_writer.Write("{} @{}", enumerator.GetName().AsView(), enumerator.GetOrdinal());
        if (!enumerator.GetAttributes().IsEmpty())
        {
            m_writer.Write(' ');
            WriteAttributes(enumerator.GetAttributes(), scope);
        }
        m_writer.Write(";\n");

        return true;
    }

    bool CodeGenEcho::VisitInterface(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_ASSERT(decl.GetData().IsInterface());
        Declaration::Data::Interface::Reader interfaceDecl = decl.GetData().GetInterface();

        m_writer.WriteIndent();
        m_writer.Write("interface {}", decl.GetName().AsView());
        WriteTypeParams(decl.GetTypeParams());
        m_writer.Write(" " HE_ID_FMT, decl.GetId());
        if (interfaceDecl.HasSuper())
        {
            m_writer.Write(" extends ");
            WriteType(interfaceDecl.GetSuper(), scope);
        }
        WriteAttributes(decl.GetAttributes(), scope);
        m_writer.Write('\n');

        m_writer.WriteLine("{");
        m_writer.IncreaseIndent();

        if (!SchemaVisitor::VisitInterface(decl, scope))
            return false;

        m_writer.DecreaseIndent();
        m_writer.WriteLine("}");

        return true;
    }

    bool CodeGenEcho::VisitMethod(Method::Reader method, Declaration::Reader scope)
    {
        HE_UNUSED(scope);

        m_writer.WriteIndent();
        m_writer.Write("{} @{} ", method.GetName().AsView(), method.GetOrdinal());

        Declaration::Reader paramStruct = m_request.GetDecl(method.GetParamStruct());
        WriteTuple(paramStruct);

        m_writer.Write(" -> ");

        if (method.GetResultStruct() != 0)
        {
            Declaration::Reader resultStruct = m_request.GetDecl(method.GetResultStruct());
            WriteTuple(resultStruct);
        }
        else
        {
            m_writer.Write("void");
        }
        m_writer.Write(";\n");

        return true;
    }

    bool CodeGenEcho::VisitStruct(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_ASSERT(decl.GetData().IsStruct());
        Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();

        if (!structDecl.GetIsGroup() && !structDecl.GetIsUnion())
        {
            m_writer.WriteIndent();
            m_writer.Write("struct {}", decl.GetName().AsView());
            WriteTypeParams(decl.GetTypeParams());
            m_writer.Write(" " HE_ID_FMT, decl.GetId());
            WriteAttributes(decl.GetAttributes(), scope);
            m_writer.Write(" // {} data fields, {} data words, {} pointers", structDecl.GetDataFieldCount(), structDecl.GetDataWordSize(), structDecl.GetPointerCount());
        }

        m_writer.Write('\n');
        m_writer.WriteLine("{");
        m_writer.IncreaseIndent();

        if (!SchemaVisitor::VisitStruct(decl, scope))
            return false;

        m_writer.DecreaseIndent();
        m_writer.WriteLine("}");

        return true;
    }

    bool CodeGenEcho::VisitNormalField(Field::Reader field, Declaration::Reader scope)
    {
        HE_ASSERT(field.GetMeta().IsNormal());
        HE_ASSERT(scope.GetData().IsStruct());
        Declaration::Data::Struct::Reader structDecl = scope.GetData().GetStruct();
        Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();

        if (IsPointer(norm.GetType()))
        {
            m_writer.WriteIndent();
            WriteField(field, scope);

            if (norm.GetType().GetData().IsArray())
            {
                m_writer.Write("; // ptrs[{}, {})", norm.GetIndex(), norm.GetIndex() + norm.GetType().GetData().GetArray().GetSize());
            }
            else
            {
                m_writer.Write("; // ptr[{}]", norm.GetIndex());
            }
        }
        else
        {
            const uint32_t fieldSize = GetTypeSize(norm.GetType());
            const uint32_t begin = norm.GetDataOffset() * fieldSize;
            const uint32_t end = begin + fieldSize;
            m_writer.WriteIndent();
            WriteField(field, scope);
            m_writer.Write("; // bits[{}, {})", begin, end);
        }

        if (structDecl.GetIsUnion())
        {
            m_writer.Write(", union tag = {}", field.GetUnionTag());
        }
        m_writer.Write('\n');
        return true;
    }

    bool CodeGenEcho::VisitGroupField(Field::Reader field, Declaration::Reader scope)
    {
        return WriteGroupOrUnionField(field, scope);
    }

    bool CodeGenEcho::VisitUnionField(Field::Reader field, Declaration::Reader scope)
    {
        return WriteGroupOrUnionField(field, scope);
    }

    void CodeGenEcho::WriteAttribute(Attribute::Reader attribute, Declaration::Reader scope)
    {
        Declaration::Reader decl = m_request.GetDecl(attribute.GetId());
        HE_ASSERT(decl.GetData().IsAttribute());
        m_writer.Write('$');
        WriteName(decl, {}, scope);

        if (attribute.HasValue())
        {
            m_writer.Write('(');
            WriteValue(decl.GetData().GetAttribute().GetType(), attribute.GetValue(), scope);
            m_writer.Write(')');
        }
    }

    void CodeGenEcho::WriteAttributes(List<Attribute>::Reader attributes, Declaration::Reader scope)
    {
        for (Attribute::Reader attr : attributes)
        {
            m_writer.Write(' ');
            WriteAttribute(attr, scope);
        }
    }

    void CodeGenEcho::WriteField(Field::Reader field, Declaration::Reader scope)
    {
        HE_ASSERT(field.GetMeta().IsNormal());
        Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();

        m_writer.Write("{} @{} :", field.GetName().AsView(), norm.GetOrdinal());
        WriteType(norm.GetType(), scope);

        if (norm.HasDefaultValue())
        {
            m_writer.Write(" = ");
            WriteValue(norm.GetType(), norm.GetDefaultValue(), scope);
        }

        WriteAttributes(field.GetAttributes(), scope);
    }

    bool CodeGenEcho::WriteGroupOrUnionField(Field::Reader field, Declaration::Reader scope)
    {
        HE_ASSERT(field.GetMeta().IsGroup() || field.GetMeta().IsUnion());
        HE_ASSERT(scope.GetData().IsStruct());
        Declaration::Data::Struct::Reader structDecl = scope.GetData().GetStruct();

        TypeId typeId = field.GetMeta().IsGroup() ? field.GetMeta().GetGroup().GetTypeId() : field.GetMeta().GetUnion().GetTypeId();
        Declaration::Reader group = m_request.GetDecl(typeId);
        HE_ASSERT(group.GetData().IsStruct());
        Declaration::Data::Struct::Reader groupStruct = group.GetData().GetStruct();

        m_writer.WriteIndent();
        if (groupStruct.GetIsGroup())
        {
            m_writer.Write("{} :group", field.GetName().AsView());
            if (structDecl.GetIsUnion())
            {
                m_writer.Write(" // union tag = {}", field.GetUnionTag());
            }
        }
        else
        {
            HE_ASSERT(groupStruct.GetIsUnion());
            const uint32_t tagSize = 16;
            const uint32_t begin = groupStruct.GetUnionTagOffset() * tagSize;
            const uint32_t end = begin + tagSize;
            m_writer.Write("{} :union // tag bits[{}, {})", field.GetName().AsView(), begin, end);
            if (structDecl.GetIsUnion())
            {
                m_writer.Write(", union tag = {}", field.GetUnionTag());
            }
        }

        WriteAttributes(group.GetAttributes(), scope);
        return VisitStruct(group, scope);
    }

    void CodeGenEcho::WriteName(Declaration::Reader decl, Brand::Reader brand, Declaration::Reader scope)
    {
        if (decl.GetParentId() != scope.GetId() && decl.GetParentId() != scope.GetParentId())
        {
            Declaration::Reader parent = m_request.GetDecl(decl.GetParentId());
            if (!parent.GetData().IsFile())
            {
                WriteName(parent, brand, scope);
                m_writer.Write('.');
            }
            else
            {
                Declaration::Reader root = m_request.schemaFile.GetRoot();
                if (!scope.IsValid())
                {
                    StringView nameSpace = parent.GetId() == root.GetId() ? root.GetName() : parent.GetName();
                    m_writer.Write(".{}.", nameSpace);
                }
                else if (parent.GetId() != root.GetId() && root.GetName() != parent.GetName())
                {
                    m_writer.Write(".{}.", parent.GetName().AsView());
                }
            }
        }

        m_writer.Write(decl.GetName());

        for (Brand::Scope::Reader brandScope : brand.GetScopes())
        {
            if (brandScope.GetScopeId() == decl.GetId())
            {
                m_writer.Write("<");
                List<Type>::Reader scopeParams = brandScope.GetParams();
                for (uint32_t i = 0; i < scopeParams.Size(); ++i)
                {
                    Type::Reader t = scopeParams[i];
                    WriteType(t, scope);
                    if (i < (scopeParams.Size() - 1))
                        m_writer.Write(", ");
                }
                m_writer.Write(">");
                break;
            }
        }
    }

    void CodeGenEcho::WriteTuple(Declaration::Reader decl)
    {
        HE_ASSERT(decl.GetData().IsStruct());
        Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
        List<Field>::Reader fields = structDecl.GetFields();

        m_writer.Write('(');

        for (uint32_t i = 0; i < fields.Size(); ++i)
        {
            Field::Reader field = fields[i];
            WriteField(field, decl);

            if (i != (fields.Size() - 1))
                m_writer.Write(", ");
        }

        m_writer.Write(')');
    }


    void CodeGenEcho::WriteTypeParams(List<String>::Reader typeParams)
    {
        if (typeParams.IsEmpty())
            return;

        m_writer.Write('<');

        for (uint32_t i = 0; i < typeParams.Size(); ++i)
        {
            m_writer.Write(typeParams[i]);

            if (i != (typeParams.Size() - 1))
                m_writer.Write(", ");
        }

        m_writer.Write('>');
    }

    void CodeGenEcho::WriteType(Type::Reader type, Declaration::Reader scope)
    {
        switch (type.GetData().GetUnionTag())
        {
            case Type::Data::UnionTag::Void: m_writer.Write("void"); break;
            case Type::Data::UnionTag::Bool: m_writer.Write("bool"); break;
            case Type::Data::UnionTag::Int8: m_writer.Write("int8"); break;
            case Type::Data::UnionTag::Int16: m_writer.Write("int16"); break;
            case Type::Data::UnionTag::Int32: m_writer.Write("int32"); break;
            case Type::Data::UnionTag::Int64: m_writer.Write("int64"); break;
            case Type::Data::UnionTag::Uint8: m_writer.Write("uint8"); break;
            case Type::Data::UnionTag::Uint16: m_writer.Write("uint16"); break;
            case Type::Data::UnionTag::Uint32: m_writer.Write("uint32"); break;
            case Type::Data::UnionTag::Uint64: m_writer.Write("uint64"); break;
            case Type::Data::UnionTag::Float32: m_writer.Write("float32"); break;
            case Type::Data::UnionTag::Float64: m_writer.Write("float64"); break;
            case Type::Data::UnionTag::Array:
            {
                Type::Data::Array::Reader arrayType = type.GetData().GetArray();
                WriteType(arrayType.GetElementType(), scope);
                m_writer.Write("[{}]", arrayType.GetSize());
                break;
            }
            case Type::Data::UnionTag::Blob:
                m_writer.Write("Blob");
                break;
            case Type::Data::UnionTag::String:
                m_writer.Write("String");
                break;
            case Type::Data::UnionTag::List:
            {
                Type::Data::List::Reader listType = type.GetData().GetList();
                WriteType(listType.GetElementType(), scope);
                m_writer.Write("[]");
                break;
            }
            case Type::Data::UnionTag::Enum:
            {
                Type::Data::Enum::Reader enumType = type.GetData().GetEnum();
                Declaration::Reader decl = m_request.GetDecl(enumType.GetId());
                HE_ASSERT(decl.GetData().IsEnum());
                WriteName(decl, enumType.GetBrand(), scope);
                break;
            }
            case Type::Data::UnionTag::Struct:
            {
                Type::Data::Struct::Reader structType = type.GetData().GetStruct();
                Declaration::Reader decl = m_request.GetDecl(structType.GetId());
                HE_ASSERT(decl.GetData().IsStruct());
                WriteName(decl, structType.GetBrand(), scope);
                break;
            }
            case Type::Data::UnionTag::Interface:
            {
                Type::Data::Interface::Reader interfaceType = type.GetData().GetInterface();
                Declaration::Reader decl = m_request.GetDecl(interfaceType.GetId());
                HE_ASSERT(decl.GetData().IsInterface());
                WriteName(decl, interfaceType.GetBrand(), scope);
                break;
            }
            case Type::Data::UnionTag::AnyPointer:
            {
                m_writer.Write("AnyPointer");
                break;
            }
            case Type::Data::UnionTag::AnyStruct:
            {
                m_writer.Write("AnyStruct");
                break;
            }
            case Type::Data::UnionTag::AnyList:
            {
                m_writer.Write("AnyList");
                break;
            }
            case Type::Data::UnionTag::Parameter:
            {
                Type::Data::Parameter::Reader param = type.GetData().GetParameter();
                Declaration::Reader decl = m_request.GetDecl(param.GetScopeId());
                m_writer.Write(decl.GetTypeParams()[param.GetIndex()]);
                break;
            }
        }
    }

    void CodeGenEcho::WriteValue(Type::Reader type, Value::Reader value, Declaration::Reader scope)
    {
        const Value::Data::Reader valueData = value.GetData();

        switch (valueData.GetUnionTag())
        {
            case Value::Data::UnionTag::Void: break;
            case Value::Data::UnionTag::Bool: m_writer.Write("{}", valueData.GetBool()); break;
            case Value::Data::UnionTag::Int8: m_writer.Write("{}", valueData.GetInt8()); break;
            case Value::Data::UnionTag::Int16: m_writer.Write("{}", valueData.GetInt16()); break;
            case Value::Data::UnionTag::Int32: m_writer.Write("{}", valueData.GetInt32()); break;
            case Value::Data::UnionTag::Int64: m_writer.Write("{}", valueData.GetInt64()); break;
            case Value::Data::UnionTag::Uint8: m_writer.Write("{}", valueData.GetUint8()); break;
            case Value::Data::UnionTag::Uint16: m_writer.Write("{}", valueData.GetUint16()); break;
            case Value::Data::UnionTag::Uint32: m_writer.Write("{}", valueData.GetUint32()); break;
            case Value::Data::UnionTag::Uint64: m_writer.Write("{}", valueData.GetUint64()); break;
            case Value::Data::UnionTag::Float32: m_writer.Write("{}", valueData.GetFloat32()); break;
            case Value::Data::UnionTag::Float64: m_writer.Write("{}", valueData.GetFloat64()); break;
            case Value::Data::UnionTag::Blob:
            {
                const Blob::Reader bytes = valueData.GetBlob();
                m_writer.Write("0x\"{:02x}\"", fmt::join(bytes, ""));
                break;
            }
            case Value::Data::UnionTag::String:
            {
                const String::Reader str = valueData.GetString();
                m_writer.Write("\"{}\"", str.AsView());
                break;
            }
            case Value::Data::UnionTag::List:
            {
                const Type::Reader elementType = type.GetData().GetList().GetElementType();
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListReader listValues = valueData.GetList().TryGetList(elementSize);
                WriteListValue(elementType, listValues, scope);
                break;
            }
            case Value::Data::UnionTag::Enum:
            {
                const Type::Data::Enum::Reader enumType = type.GetData().GetEnum();
                const Declaration::Reader decl = m_request.GetDecl(enumType.GetId());
                const uint16_t enumValue = valueData.GetEnum();
                const Declaration::Data::Enum::Reader enumDecl = decl.GetData().GetEnum();

                WriteName(decl, enumType.GetBrand(), scope);
                m_writer.Write('.');
                for (Enumerator::Reader e : enumDecl.GetEnumerators())
                {
                    if (e.GetOrdinal() == enumValue)
                    {
                        m_writer.Write(e.GetName());
                        break;
                    }
                }
                break;
            }
            case Value::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = type.GetData().GetStruct();
                const StructReader structValue = valueData.GetStruct().TryGetStruct();

                WriteStructValue(structType.GetId(), structValue, scope);
                break;
            }
        }
    }

    void CodeGenEcho::WriteValue(Type::Reader fieldType, StructReader value, uint16_t index, uint32_t dataOffset, Declaration::Reader scope)
    {
        const Type::Data::Reader typeData = fieldType.GetData();

        switch (typeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: break;
            case Type::Data::UnionTag::Bool: m_writer.Write("{}", value.GetDataField<bool>(dataOffset)); break;
            case Type::Data::UnionTag::Int8: m_writer.Write("{}", value.GetDataField<int8_t>(dataOffset)); break;
            case Type::Data::UnionTag::Int16: m_writer.Write("{}", value.GetDataField<int16_t>(dataOffset)); break;
            case Type::Data::UnionTag::Int32: m_writer.Write("{}", value.GetDataField<int32_t>(dataOffset)); break;
            case Type::Data::UnionTag::Int64: m_writer.Write("{}", value.GetDataField<int64_t>(dataOffset)); break;
            case Type::Data::UnionTag::Uint8: m_writer.Write("{}", value.GetDataField<uint8_t>(dataOffset)); break;
            case Type::Data::UnionTag::Uint16: m_writer.Write("{}", value.GetDataField<uint16_t>(dataOffset)); break;
            case Type::Data::UnionTag::Uint32: m_writer.Write("{}", value.GetDataField<uint32_t>(dataOffset)); break;
            case Type::Data::UnionTag::Uint64: m_writer.Write("{}", value.GetDataField<uint64_t>(dataOffset)); break;
            case Type::Data::UnionTag::Float32: m_writer.Write("{}", value.GetDataField<float>(dataOffset)); break;
            case Type::Data::UnionTag::Float64: m_writer.Write("{}", value.GetDataField<double>(dataOffset)); break;
            case Type::Data::UnionTag::Blob:
            {
                const Blob::Reader bytes = value.GetPointerField(index).TryGetBlob();
                m_writer.Write("0x\"{:02x}\"", fmt::join(bytes, ""));
                break;
            }
            case Type::Data::UnionTag::String:
            {
                const String::Reader str = value.GetPointerField(index).TryGetString();
                m_writer.Write("\"{}\"", str.AsView());
                break;
            }
            case Type::Data::UnionTag::Array:
            {
                HE_ASSERT(!typeData.IsArray(), HE_MSG("Arrays of arrays are not supported."));

                const Type::Data::Array::Reader arrayType = typeData.GetArray();
                const Type::Reader elementType = arrayType.GetElementType();
                const uint16_t size = arrayType.GetSize();
                WriteArrayValue(elementType, value, index, dataOffset, size, scope);
                break;
            }
            case Type::Data::UnionTag::List:
            {
                const Type::Data::List::Reader listType = typeData.GetList();
                const Type::Reader elementType = listType.GetElementType();
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListReader list = value.GetPointerField(index).TryGetList(elementSize);
                WriteListValue(elementType, list, scope);
                break;
            }
            case Type::Data::UnionTag::Enum:
            {
                const Type::Data::Enum::Reader enumType = typeData.GetEnum();
                const Declaration::Reader decl = m_request.GetDecl(enumType.GetId());
                const uint16_t enumValue = value.GetDataField<uint16_t>(dataOffset);

                HE_ASSERT(decl.GetData().IsEnum());
                Declaration::Data::Enum::Reader enumDecl = decl.GetData().GetEnum();

                WriteName(decl, enumType.GetBrand(), scope);
                m_writer.Write('.');
                for (Enumerator::Reader e : enumDecl.GetEnumerators())
                {
                    if (e.GetOrdinal() == enumValue)
                    {
                        m_writer.Write(e.GetName());
                        break;
                    }
                }
                break;
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = typeData.GetStruct();
                WriteStructValue(structType.GetId(), value.GetPointerField(index).TryGetStruct(), scope);
                break;
            }

            case Type::Data::UnionTag::AnyPointer:
            case Type::Data::UnionTag::AnyStruct:
            case Type::Data::UnionTag::AnyList:
            case Type::Data::UnionTag::Interface:
            case Type::Data::UnionTag::Parameter:
                HE_ASSERT(false, HE_MSG("{} types cannot have default values."));
                break;
        }
    }

    void CodeGenEcho::WriteValue(Type::Reader elementType, ListReader value, uint32_t index, Declaration::Reader scope)
    {
        const Type::Data::Reader typeData = elementType.GetData();

        switch (typeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: break;
            case Type::Data::UnionTag::Bool: m_writer.Write("{}", value.GetDataElement<bool>(index)); break;
            case Type::Data::UnionTag::Int8: m_writer.Write("{}", value.GetDataElement<int8_t>(index)); break;
            case Type::Data::UnionTag::Int16: m_writer.Write("{}", value.GetDataElement<int16_t>(index)); break;
            case Type::Data::UnionTag::Int32: m_writer.Write("{}", value.GetDataElement<int32_t>(index)); break;
            case Type::Data::UnionTag::Int64: m_writer.Write("{}", value.GetDataElement<int64_t>(index)); break;
            case Type::Data::UnionTag::Uint8: m_writer.Write("{}", value.GetDataElement<uint8_t>(index)); break;
            case Type::Data::UnionTag::Uint16: m_writer.Write("{}", value.GetDataElement<uint16_t>(index)); break;
            case Type::Data::UnionTag::Uint32: m_writer.Write("{}", value.GetDataElement<uint32_t>(index)); break;
            case Type::Data::UnionTag::Uint64: m_writer.Write("{}", value.GetDataElement<uint64_t>(index)); break;
            case Type::Data::UnionTag::Float32: m_writer.Write("{}", value.GetDataElement<float>(index)); break;
            case Type::Data::UnionTag::Float64: m_writer.Write("{}", value.GetDataElement<double>(index)); break;
            case Type::Data::UnionTag::Blob:
            {
                const Blob::Reader bytes = value.GetPointerElement(index).TryGetBlob();
                m_writer.Write("0x\"{:02x}\"", fmt::join(bytes, ""));
                break;
            }
            case Type::Data::UnionTag::String:
            {
                const String::Reader str = value.GetPointerElement(index).TryGetString();
                m_writer.Write("\"{}\"", str.AsView());
                break;
            }
            case Type::Data::UnionTag::Array:
            {
                HE_ASSERT(!typeData.IsArray(), HE_MSG("Lists of arrays are not supported."));
                break;
            }
            case Type::Data::UnionTag::List:
            {
                const Type::Data::List::Reader listType = typeData.GetList();
                const Type::Reader subElementType = listType.GetElementType();
                const ElementSize subElementSize = GetTypeElementSize(subElementType);
                const ListReader list = value.GetPointerElement(index).TryGetList(subElementSize);
                WriteListValue(subElementType, list, scope);
                break;
            }
            case Type::Data::UnionTag::Enum:
            {
                const Type::Data::Enum::Reader enumType = typeData.GetEnum();
                const Declaration::Reader decl = m_request.GetDecl(enumType.GetId());
                const uint16_t enumValue = value.GetDataElement<uint16_t>(index);

                HE_ASSERT(decl.GetData().IsEnum());
                Declaration::Data::Enum::Reader enumDecl = decl.GetData().GetEnum();

                WriteName(decl, enumType.GetBrand(), scope);
                m_writer.Write('.');
                for (Enumerator::Reader e : enumDecl.GetEnumerators())
                {
                    if (e.GetOrdinal() == enumValue)
                    {
                        m_writer.Write(e.GetName());
                        break;
                    }
                }
                break;
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = typeData.GetStruct();
                WriteStructValue(structType.GetId(), value.GetCompositeElement(index), scope);
                break;
            }

            case Type::Data::UnionTag::AnyPointer:
            case Type::Data::UnionTag::AnyStruct:
            case Type::Data::UnionTag::AnyList:
            case Type::Data::UnionTag::Interface:
            case Type::Data::UnionTag::Parameter:
                HE_ASSERT(false, HE_MSG("{} types cannot have default values."));
                break;
        }
    }

    void CodeGenEcho::WriteArrayValue(Type::Reader elementType, StructReader value, uint16_t index, uint32_t dataOffset, uint16_t size, Declaration::Reader scope)
    {
        const bool isPointer = IsPointer(elementType);

        m_writer.Write('[');
        for (uint16_t i = 0; i < size; ++i)
        {
            if (i > 0)
                m_writer.Write(", ");

            if (isPointer)
            {
                WriteValue(elementType, value, index + i, dataOffset, scope);
            }
            else
            {
                WriteValue(elementType, value, index, dataOffset + i, scope);
            }
        }
        m_writer.Write(']');
    }

    void CodeGenEcho::WriteListValue(Type::Reader elementType, ListReader list, Declaration::Reader scope)
    {
        const uint32_t size = list.Size();

        m_writer.Write('[');
        for (uint32_t i = 0; i < size; ++i)
        {
            if (i > 0)
                m_writer.Write(", ");

            WriteValue(elementType, list, i, scope);
        }
        m_writer.Write(']');
    }

    void CodeGenEcho::WriteStructValue(TypeId typeId, StructReader value, Declaration::Reader scope)
    {
        const Declaration::Reader decl = m_request.GetDecl(typeId);
        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
        const List<Field>::Reader fields = structDecl.GetFields();

        bool first = true;
        m_writer.Write("{ ");
        for (const Field::Reader field : fields)
        {
            if (!first)
                m_writer.Write(", ");

            if (TryWriteFieldValue(field, value, scope))
                first = false;
        }
        m_writer.Write(" }");
    }

    void CodeGenEcho::WriteUnionValue(TypeId typeId, StructReader value, Declaration::Reader scope)
    {
        const Declaration::Reader decl = m_request.GetDecl(typeId);
        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
        const List<Field>::Reader fields = structDecl.GetFields();

        const uint16_t activeFieldTag = value.GetDataField<uint16_t>(structDecl.GetUnionTagOffset());

        m_writer.Write("{ ");
        for (const Field::Reader field : fields)
        {
            if (field.GetUnionTag() == activeFieldTag)
            {
                TryWriteFieldValue(field, value, scope);
                break;
            }
        }
        m_writer.Write(" }");
    }

    bool CodeGenEcho::TryWriteFieldValue(Field::Reader field, StructReader value, Declaration::Reader scope)
    {
        if (field.GetMeta().IsGroup())
        {
            if (!AnyGroupFieldSet(field, value))
                return false;

            const Field::Meta::Group::Reader group = field.GetMeta().GetGroup();

            m_writer.Write("{} = ", field.GetName().AsView());
            WriteStructValue(group.GetTypeId(), value, scope);
            return true;
        }
        else if (field.GetMeta().IsUnion())
        {
            if (!IsUnionFieldSet(field, value))
                return false;

            const Field::Meta::Union::Reader group = field.GetMeta().GetUnion();

            m_writer.Write("{} = ", field.GetName().AsView());
            WriteUnionValue(group.GetTypeId(), value, scope);
            return true;
        }
        else
        {
            if (!IsNormalFieldSet(field, value))
                return false;

            const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
            const Type::Reader fieldType = norm.GetType();
            const uint16_t index = norm.GetIndex();
            const uint32_t dataOffset = norm.GetDataOffset();

            m_writer.Write("{} = ", field.GetName().AsView());
            WriteValue(fieldType, value, index, dataOffset, scope);
            return true;
        }
    }

    bool CodeGenEcho::AnyGroupFieldSet(Field::Reader field, StructReader value)
    {
        const Field::Meta::Group::Reader group = field.GetMeta().GetGroup();
        const Declaration::Reader decl = m_request.GetDecl(group.GetTypeId());
        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
        const List<Field>::Reader fields = structDecl.GetFields();

        for (const Field::Reader f : fields)
        {
            if (f.GetMeta().IsGroup())
            {
                if (AnyGroupFieldSet(f, value))
                    return true;
            }
            else if (f.GetMeta().IsUnion())
            {
                if (IsUnionFieldSet(f, value))
                    return true;
            }
            else
            {
                if (IsNormalFieldSet(f, value))
                    return true;
            }
        }

        return false;
    }

    bool CodeGenEcho::IsUnionFieldSet(Field::Reader field, StructReader value)
    {
        // TODO: This doesn't actually work. We need to allocate a field index for the union tag to make this work.
        //const Field::Meta::Group::Reader group = field.GetMeta().GetGroup();
        //const Declaration::Reader decl = m_request.GetDecl(group.GetTypeId());
        //const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();

        //return value.HasDataField(structDecl.GetUnionTagOffset());

        HE_UNUSED(field, value);
        return false;
    }

    bool CodeGenEcho::IsNormalFieldSet(Field::Reader field, StructReader value)
    {
        const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
        const Type::Reader fieldType = norm.GetType();
        const Type::Data::Reader fieldTypeData = fieldType.GetData();

        if (fieldTypeData.IsVoid())
            return false;

        if (IsPointer(fieldType))
            return value.HasPointerField(norm.GetIndex());

        return value.HasDataField(norm.GetIndex());
    }

    bool GenerateEcho(const CodeGenRequest& request)
    {
        CodeGenEcho generator(request);
        if (!generator.Generate())
            return false;

        return true;
    }
}
