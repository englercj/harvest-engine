// Copyright Chad Engler

#include "codegen_echo.h"

#include "he/core/span_fmt.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view_fmt.h"

#include <iostream>

namespace he::schema
{
    #define HE_ID_FMT "@{:#018x}"

    CodeGenEcho::CodeGenEcho(const CodeGenRequest& request)
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
        WriteValue(constDecl.GetType(), scope, constDecl.GetValue());
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
        m_writer.Write("${}", decl.GetName().AsView());

        if (!attribute.GetValue().GetData().IsVoid())
        {
            m_writer.Write('(');
            WriteValue(decl.GetData().GetAttribute().GetType(), scope, attribute.GetValue());
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

        if (norm.HasDefaultValue() && !norm.GetDefaultValue().GetData().IsVoid())
        {
            m_writer.Write(" = ");
            WriteValue(norm.GetType(), scope, norm.GetDefaultValue());
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

    void CodeGenEcho::WriteName(Declaration::Reader decl, Declaration::Reader scope, Brand::Reader brand)
    {
        if (decl.GetParentId() != scope.GetId() && decl.GetParentId() != scope.GetParentId())
        {
            Declaration::Reader parent = m_request.GetDecl(decl.GetParentId());
            if (!parent.GetData().IsFile())
            {
                WriteName(parent, scope, brand);
                m_writer.Write('.');
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
        switch (type.GetData().GetTag())
        {
            case Type::Data::Tag::Void: m_writer.Write("void"); break;
            case Type::Data::Tag::Bool: m_writer.Write("bool"); break;
            case Type::Data::Tag::Int8: m_writer.Write("int8"); break;
            case Type::Data::Tag::Int16: m_writer.Write("int16"); break;
            case Type::Data::Tag::Int32: m_writer.Write("int32"); break;
            case Type::Data::Tag::Int64: m_writer.Write("int64"); break;
            case Type::Data::Tag::Uint8: m_writer.Write("uint8"); break;
            case Type::Data::Tag::Uint16: m_writer.Write("uint16"); break;
            case Type::Data::Tag::Uint32: m_writer.Write("uint32"); break;
            case Type::Data::Tag::Uint64: m_writer.Write("uint64"); break;
            case Type::Data::Tag::Float32: m_writer.Write("float32"); break;
            case Type::Data::Tag::Float64: m_writer.Write("float64"); break;
            case Type::Data::Tag::Array:
            {
                Type::Data::Array::Reader arrayType = type.GetData().GetArray();
                WriteType(arrayType.GetElementType(), scope);
                m_writer.Write("[{}]", arrayType.GetSize());
                break;
            }
            case Type::Data::Tag::Blob:
                m_writer.Write("Blob");
                break;
            case Type::Data::Tag::String:
                m_writer.Write("String");
                break;
            case Type::Data::Tag::List:
            {
                Type::Data::List::Reader listType = type.GetData().GetList();
                WriteType(listType.GetElementType(), scope);
                m_writer.Write("[]");
                break;
            }
            case Type::Data::Tag::Enum:
            {
                Type::Data::Enum::Reader enumType = type.GetData().GetEnum();
                Declaration::Reader decl = m_request.GetDecl(enumType.GetId());
                HE_ASSERT(decl.GetData().IsEnum());
                WriteName(decl, scope, enumType.GetBrand());
                break;
            }
            case Type::Data::Tag::Struct:
            {
                Type::Data::Struct::Reader structType = type.GetData().GetStruct();
                Declaration::Reader decl = m_request.GetDecl(structType.GetId());
                HE_ASSERT(decl.GetData().IsStruct());
                WriteName(decl, scope, structType.GetBrand());
                break;
            }
            case Type::Data::Tag::Interface:
            {
                Type::Data::Interface::Reader interfaceType = type.GetData().GetInterface();
                Declaration::Reader decl = m_request.GetDecl(interfaceType.GetId());
                HE_ASSERT(decl.GetData().IsInterface());
                WriteName(decl, scope, interfaceType.GetBrand());
                break;
            }
            case Type::Data::Tag::AnyPointer:
            {
                Type::Data::AnyPointer::Reader anyType = type.GetData().GetAnyPointer();

                if (anyType.GetParamScopeId() == 0)
                {
                    m_writer.Write("AnyPointer");
                }
                else
                {
                    Declaration::Reader decl = m_request.GetDecl(anyType.GetParamScopeId());
                    m_writer.Write(decl.GetTypeParams()[anyType.GetParamIndex()]);
                }
                break;
            }
        }
    }

    void CodeGenEcho::WriteValue(Type::Reader type, Declaration::Reader scope, Value::Reader value)
    {
        switch (value.GetData().GetTag())
        {
            case Value::Data::Tag::Void: break;
            case Value::Data::Tag::Bool: m_writer.Write("{}", value.GetData().GetBool()); break;
            case Value::Data::Tag::Int8: m_writer.Write("{}", value.GetData().GetInt8()); break;
            case Value::Data::Tag::Int16: m_writer.Write("{}", value.GetData().GetInt16()); break;
            case Value::Data::Tag::Int32: m_writer.Write("{}", value.GetData().GetInt32()); break;
            case Value::Data::Tag::Int64: m_writer.Write("{}", value.GetData().GetInt64()); break;
            case Value::Data::Tag::Uint8: m_writer.Write("{}", value.GetData().GetUint8()); break;
            case Value::Data::Tag::Uint16: m_writer.Write("{}", value.GetData().GetUint16()); break;
            case Value::Data::Tag::Uint32: m_writer.Write("{}", value.GetData().GetUint32()); break;
            case Value::Data::Tag::Uint64: m_writer.Write("{}", value.GetData().GetUint64()); break;
            case Value::Data::Tag::Float32: m_writer.Write("{}", value.GetData().GetFloat32()); break;
            case Value::Data::Tag::Float64: m_writer.Write("{}", value.GetData().GetFloat64()); break;
            case Value::Data::Tag::Array:
            {
                HE_ASSERT(type.GetData().IsArray());
                const Type::Reader elementType = type.GetData().GetArray().GetElementType();
                const List<Value>::Reader arrayValues = value.GetData().GetArray();
                WriteValueList(elementType, scope, arrayValues);
                break;
            }
            case Value::Data::Tag::Blob:
            {
                HE_ASSERT(type.GetData().IsBlob());
                List<uint8_t>::Reader bytes = value.GetData().GetBlob();
                Span<const uint8_t> byteSpan{ bytes.Data(), bytes.Size() };
                m_writer.Write("0x\"{}\"", byteSpan);
                break;
            }
            case Value::Data::Tag::String:
            {
                HE_ASSERT(type.GetData().IsString());
                String::Reader str = value.GetData().GetString();
                m_writer.Write("\"{}\"", str.AsView());
                break;
            }
            case Value::Data::Tag::List:
            {
                HE_ASSERT(type.GetData().IsList());
                const Type::Reader elementType = type.GetData().GetList().GetElementType();
                const List<Value>::Reader listValues = value.GetData().GetList();
                WriteValueList(elementType, scope, listValues);
                break;
            }
            case Value::Data::Tag::Enum:
            {
                HE_ASSERT(type.GetData().IsEnum());
                const Type::Data::Enum::Reader enumType = type.GetData().GetEnum();
                const Declaration::Reader decl = m_request.GetDecl(enumType.GetId());
                const uint16_t enumValue = value.GetData().GetEnum();

                HE_ASSERT(decl.GetData().IsEnum());
                Declaration::Data::Enum::Reader enumDecl = decl.GetData().GetEnum();

                WriteName(decl, scope, enumType.GetBrand());
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
            case Value::Data::Tag::Tuple:
            {
                HE_ASSERT(type.GetData().IsStruct());
                const Type::Data::Struct::Reader structType = type.GetData().GetStruct();
                const Declaration::Reader decl = m_request.GetDecl(structType.GetId());

                HE_ASSERT(decl.GetData().IsStruct());
                const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
                const List<Field>::Reader fields = structDecl.GetFields();

                const List<Value::TupleValue>::Reader tupleValue = value.GetData().GetTuple();
                m_writer.Write("{ ");
                for (uint32_t i = 0; i < tupleValue.Size(); ++i)
                {
                    const Value::TupleValue::Reader v = tupleValue[i];
                    uint32_t fieldIndex = ~0u;
                    for (uint32_t j = 0; j < fields.Size(); ++j)
                    {
                        if (fields[j].GetName() == v.GetName())
                        {
                            fieldIndex = j;
                            break;
                        }
                    }
                    HE_ASSERT(fieldIndex != ~0u);
                    m_writer.Write("{} = ", v.GetName().AsView());

                    Field::Reader field = fields[fieldIndex];
                    Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                    WriteValue(norm.GetType(), scope, v.GetValue());

                    if (i != (tupleValue.Size() - 1))
                        m_writer.Write(", ");
                }
                m_writer.Write(" }");
                break;
            }
            case Value::Data::Tag::Interface:
            case Value::Data::Tag::AnyPointer:
                std::cerr << "Interface and AnyPointer types cannot have explicit values. Verifier should've caught this.";
                break;
        }
    }

    void CodeGenEcho::WriteValueList(Type::Reader elementType, Declaration::Reader scope, List<Value>::Reader values)
    {
        const uint32_t size = values.Size();

        m_writer.Write('[');
        for (uint32_t i = 0; i < size; ++i)
        {
            WriteValue(elementType, scope, values[i]);

            if (i != (size - 1))
                m_writer.Write(", ");
        }
        m_writer.Write(']');
    }

    bool GenerateEcho(const CodeGenRequest& request)
    {
        CodeGenEcho generator(request);
        if (!generator.Generate())
            return false;

        return true;
    }
}
