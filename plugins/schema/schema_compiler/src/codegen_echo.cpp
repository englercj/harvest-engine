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

        Declaration::Reader root = m_request.schemaFile.Root();

        if (root.HasSource())
            m_writer.WriteLine("// {}", root.Source().File().AsView());

        m_writer.WriteLine(HE_ID_FMT ";", root.Id());

        for (String::Reader importStr : root.Data().File().Imports())
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
        if (!decl.Name().IsEmpty())
        {
            m_writer.WriteLine("namespace {};", decl.Name().AsView());
        }

        for (Attribute::Reader attr : decl.Attributes())
        {
            WriteAttribute(attr, decl);
            m_writer.Write(";\n");
        }

        return SchemaVisitor::VisitFile(decl);
    }

    bool CodeGenEcho::VisitAttribute(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_ASSERT(decl.Data().IsAttribute());
        Declaration::Data::Attribute::Reader attrDecl = decl.Data().Attribute();

        he::String targetStr(Allocator::GetTemp());

        if (attrDecl.TargetsAttribute())
            targetStr += "attribute,";
        if (attrDecl.TargetsConstant())
            targetStr += "const,";
        if (attrDecl.TargetsEnum())
            targetStr += "enum,";
        if (attrDecl.TargetsEnumerator())
            targetStr += "enumerator,";
        if (attrDecl.TargetsField())
            targetStr += "field,";
        if (attrDecl.TargetsFile())
            targetStr += "file,";
        if (attrDecl.TargetsInterface())
            targetStr += "interface,";
        if (attrDecl.TargetsMethod())
            targetStr += "method,";
        if (attrDecl.TargetsParameter())
            targetStr += "parameter,";
        if (attrDecl.TargetsStruct())
            targetStr += "struct,";

        if (!targetStr.IsEmpty())
            targetStr.PopBack();

        m_writer.WriteIndent();
        m_writer.Write("attribute {} " HE_ID_FMT " ({}) :", decl.Name().AsView(), decl.Id(), targetStr);
        WriteType(attrDecl.Type(), scope);
        WriteAttributes(decl.Attributes(), scope);
        m_writer.Write(";\n");

        return true;
    }

    bool CodeGenEcho::VisitConstant(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_ASSERT(decl.Data().IsConstant());
        Declaration::Data::Constant::Reader constDecl = decl.Data().Constant();

        m_writer.WriteIndent();
        m_writer.Write("const {} " HE_ID_FMT " :", decl.Name().AsView(), decl.Id());
        WriteType(constDecl.Type(), scope);
        m_writer.Write(" = ");
        WriteValue(constDecl.Type(), scope, constDecl.Value());
        WriteAttributes(decl.Attributes(), scope);
        m_writer.Write(";\n");

        return true;
    }

    bool CodeGenEcho::VisitEnum(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_ASSERT(decl.Data().IsEnum());

        m_writer.WriteIndent();
        m_writer.Write("enum {} " HE_ID_FMT, decl.Name().AsView(), decl.Id());
        WriteAttributes(decl.Attributes(), scope);
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
        m_writer.Write("{} @{}", enumerator.Name().AsView(), enumerator.Ordinal());
        if (!enumerator.Attributes().IsEmpty())
        {
            m_writer.Write(' ');
            WriteAttributes(enumerator.Attributes(), scope);
        }
        m_writer.Write(";\n");

        return true;
    }

    bool CodeGenEcho::VisitInterface(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_ASSERT(decl.Data().IsInterface());
        Declaration::Data::Interface::Reader interfaceDecl = decl.Data().Interface();

        m_writer.WriteIndent();
        m_writer.Write("interface {}", decl.Name().AsView());
        WriteTypeParams(decl.TypeParams());
        m_writer.Write(" " HE_ID_FMT, decl.Id());
        if (interfaceDecl.HasSuper())
        {
            m_writer.Write(" extends ");
            WriteType(interfaceDecl.Super(), scope);
        }
        WriteAttributes(decl.Attributes(), scope);
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
        m_writer.Write("{} @{} ", method.Name().AsView(), method.Ordinal());

        Declaration::Reader paramStruct = m_request.GetDecl(method.ParamStruct());
        WriteTuple(paramStruct);

        m_writer.Write(" -> ");

        if (method.ResultStruct() != 0)
        {
            Declaration::Reader resultStruct = m_request.GetDecl(method.ResultStruct());
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
        HE_ASSERT(decl.Data().IsStruct());
        Declaration::Data::Struct::Reader structDecl = decl.Data().Struct();

        if (!structDecl.IsGroup() && !structDecl.IsUnion())
        {
            m_writer.WriteIndent();
            m_writer.Write("struct {}", decl.Name().AsView());
            WriteTypeParams(decl.TypeParams());
            m_writer.Write(" " HE_ID_FMT, decl.Id());
            WriteAttributes(decl.Attributes(), scope);
            m_writer.Write(" // {} data fields, {} data words, {} pointers", structDecl.DataFieldCount(), structDecl.DataWordSize(), structDecl.PointerCount());
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
        HE_ASSERT(field.Meta().IsNormal());
        HE_ASSERT(scope.Data().IsStruct());
        Declaration::Data::Struct::Reader structDecl = scope.Data().Struct();
        Field::Meta::Normal::Reader norm = field.Meta().Normal();

        if (IsPointer(norm.Type()))
        {
            m_writer.WriteIndent();
            WriteField(field, scope);

            if (norm.Type().Data().IsArray())
            {
                m_writer.Write("; // ptrs[{}, {})", norm.Index(), norm.Index() + norm.Type().Data().Array().Size());
            }
            else
            {
                m_writer.Write("; // ptr[{}]", norm.Index());
            }
        }
        else
        {
            const uint32_t fieldSize = GetTypeSize(norm.Type());
            const uint32_t begin = norm.DataOffset() * fieldSize;
            const uint32_t end = begin + fieldSize;
            m_writer.WriteIndent();
            WriteField(field, scope);
            m_writer.Write("; // bits[{}, {})", begin, end);
        }

        if (structDecl.IsUnion())
        {
            m_writer.Write(", union tag = {}", field.UnionTag());
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
        Declaration::Reader decl = m_request.GetDecl(attribute.Id());
        HE_ASSERT(decl.Data().IsAttribute());
        m_writer.Write("${}", decl.Name().AsView());

        if (!attribute.Value().Data().IsVoid())
        {
            m_writer.Write('(');
            WriteValue(decl.Data().Attribute().Type(), scope, attribute.Value());
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
        HE_ASSERT(field.Meta().IsNormal());
        Field::Meta::Normal::Reader norm = field.Meta().Normal();

        m_writer.Write("{} @{} :", field.Name().AsView(), norm.Ordinal());
        WriteType(norm.Type(), scope);

        if (norm.HasDefaultValue() && !norm.DefaultValue().Data().IsVoid())
        {
            m_writer.Write(" = ");
            WriteValue(norm.Type(), scope, norm.DefaultValue());
        }

        WriteAttributes(field.Attributes(), scope);
    }

    bool CodeGenEcho::WriteGroupOrUnionField(Field::Reader field, Declaration::Reader scope)
    {
        HE_ASSERT(field.Meta().IsGroup() || field.Meta().IsUnion());
        HE_ASSERT(scope.Data().IsStruct());
        Declaration::Data::Struct::Reader structDecl = scope.Data().Struct();

        TypeId typeId = field.Meta().IsGroup() ? field.Meta().Group().TypeId() : field.Meta().Union().TypeId();
        Declaration::Reader group = m_request.GetDecl(typeId);
        HE_ASSERT(group.Data().IsStruct());
        Declaration::Data::Struct::Reader groupStruct = group.Data().Struct();

        m_writer.WriteIndent();
        if (groupStruct.IsGroup())
        {
            m_writer.Write("{} :group", field.Name().AsView());
            if (structDecl.IsUnion())
            {
                m_writer.Write(" // union tag = {}", field.UnionTag());
            }
        }
        else
        {
            HE_ASSERT(groupStruct.IsUnion());
            const uint32_t tagSize = 16;
            const uint32_t begin = groupStruct.UnionTagOffset() * tagSize;
            const uint32_t end = begin + tagSize;
            m_writer.Write("{} :union // tag bits[{}, {})", field.Name().AsView(), begin, end);
            if (structDecl.IsUnion())
            {
                m_writer.Write(", union tag = {}", field.UnionTag());
            }
        }

        WriteAttributes(group.Attributes(), scope);
        return VisitStruct(group, scope);
    }

    void CodeGenEcho::WriteName(Declaration::Reader decl, Declaration::Reader scope, Brand::Reader brand)
    {
        if (decl.ParentId() != scope.Id() && decl.ParentId() != scope.ParentId())
        {
            Declaration::Reader parent = m_request.GetDecl(decl.ParentId());
            if (!parent.Data().IsFile())
            {
                WriteName(parent, scope, brand);
                m_writer.Write('.');
            }
        }

        m_writer.Write(decl.Name());

        for (Brand::Scope::Reader brandScope : brand.Scopes())
        {
            if (brandScope.ScopeId() == decl.Id())
            {
                m_writer.Write("<");
                List<Type>::Reader scopeParams = brandScope.Params();
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
        HE_ASSERT(decl.Data().IsStruct());
        Declaration::Data::Struct::Reader structDecl = decl.Data().Struct();
        List<Field>::Reader fields = structDecl.Fields();

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
        switch (type.Data().Tag())
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
                Type::Data::Array::Reader arrayType = type.Data().Array();
                WriteType(arrayType.ElementType(), scope);
                m_writer.Write("[{}]", arrayType.Size());
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
                Type::Data::List::Reader listType = type.Data().List();
                WriteType(listType.ElementType(), scope);
                m_writer.Write("[]");
                break;
            }
            case Type::Data::Tag::Enum:
            {
                Type::Data::Enum::Reader enumType = type.Data().Enum();
                Declaration::Reader decl = m_request.GetDecl(enumType.Id());
                HE_ASSERT(decl.Data().IsEnum());
                WriteName(decl, scope, enumType.Brand());
                break;
            }
            case Type::Data::Tag::Struct:
            {
                Type::Data::Struct::Reader structType = type.Data().Struct();
                Declaration::Reader decl = m_request.GetDecl(structType.Id());
                HE_ASSERT(decl.Data().IsStruct());
                WriteName(decl, scope, structType.Brand());
                break;
            }
            case Type::Data::Tag::Interface:
            {
                Type::Data::Interface::Reader interfaceType = type.Data().Interface();
                Declaration::Reader decl = m_request.GetDecl(interfaceType.Id());
                HE_ASSERT(decl.Data().IsInterface());
                WriteName(decl, scope, interfaceType.Brand());
                break;
            }
            case Type::Data::Tag::AnyPointer:
            {
                Type::Data::AnyPointer::Reader anyType = type.Data().AnyPointer();

                if (anyType.ParamScopeId() == 0)
                {
                    m_writer.Write("AnyPointer");
                }
                else
                {
                    Declaration::Reader decl = m_request.GetDecl(anyType.ParamScopeId());
                    m_writer.Write(decl.TypeParams()[anyType.ParamIndex()]);
                }
                break;
            }
        }
    }

    void CodeGenEcho::WriteValue(Type::Reader type, Declaration::Reader scope, Value::Reader value)
    {
        switch (value.Data().Tag())
        {
            case Value::Data::Tag::Void: break;
            case Value::Data::Tag::Bool: m_writer.Write("{}", value.Data().Bool()); break;
            case Value::Data::Tag::Int8: m_writer.Write("{}", value.Data().Int8()); break;
            case Value::Data::Tag::Int16: m_writer.Write("{}", value.Data().Int16()); break;
            case Value::Data::Tag::Int32: m_writer.Write("{}", value.Data().Int32()); break;
            case Value::Data::Tag::Int64: m_writer.Write("{}", value.Data().Int64()); break;
            case Value::Data::Tag::Uint8: m_writer.Write("{}", value.Data().Uint8()); break;
            case Value::Data::Tag::Uint16: m_writer.Write("{}", value.Data().Uint16()); break;
            case Value::Data::Tag::Uint32: m_writer.Write("{}", value.Data().Uint32()); break;
            case Value::Data::Tag::Uint64: m_writer.Write("{}", value.Data().Uint64()); break;
            case Value::Data::Tag::Float32: m_writer.Write("{}", value.Data().Float32()); break;
            case Value::Data::Tag::Float64: m_writer.Write("{}", value.Data().Float64()); break;
            case Value::Data::Tag::Array:
            {
                HE_ASSERT(type.Data().IsArray());
                const Type::Reader elementType = type.Data().Array().ElementType();
                const List<Value>::Reader arrayValues = value.Data().Array();
                WriteValueList(elementType, scope, arrayValues);
                break;
            }
            case Value::Data::Tag::Blob:
            {
                HE_ASSERT(type.Data().IsBlob());
                List<uint8_t>::Reader bytes = value.Data().Blob();
                Span<const uint8_t> byteSpan{ bytes.Data(), bytes.Size() };
                m_writer.Write("0x\"{}\"", byteSpan);
                break;
            }
            case Value::Data::Tag::String:
            {
                HE_ASSERT(type.Data().IsString());
                String::Reader str = value.Data().String();
                m_writer.Write("\"{}\"", str.AsView());
                break;
            }
            case Value::Data::Tag::List:
            {
                HE_ASSERT(type.Data().IsList());
                const Type::Reader elementType = type.Data().List().ElementType();
                const List<Value>::Reader listValues = value.Data().List();
                WriteValueList(elementType, scope, listValues);
                break;
            }
            case Value::Data::Tag::Enum:
            {
                HE_ASSERT(type.Data().IsEnum());
                const Type::Data::Enum::Reader enumType = type.Data().Enum();
                const Declaration::Reader decl = m_request.GetDecl(enumType.Id());
                const uint16_t enumValue = value.Data().Enum();

                HE_ASSERT(decl.Data().IsEnum());
                Declaration::Data::Enum::Reader enumDecl = decl.Data().Enum();

                WriteName(decl, scope, enumType.Brand());
                m_writer.Write('.');
                for (Enumerator::Reader e : enumDecl.Enumerators())
                {
                    if (e.Ordinal() == enumValue)
                    {
                        m_writer.Write(e.Name());
                        break;
                    }
                }
                break;
            }
            case Value::Data::Tag::Tuple:
            {
                HE_ASSERT(type.Data().IsStruct());
                const Type::Data::Struct::Reader structType = type.Data().Struct();
                const Declaration::Reader decl = m_request.GetDecl(structType.Id());

                HE_ASSERT(decl.Data().IsStruct());
                const Declaration::Data::Struct::Reader structDecl = decl.Data().Struct();
                const List<Field>::Reader fields = structDecl.Fields();

                const List<Value::TupleValue>::Reader tupleValue = value.Data().Tuple();
                m_writer.Write("{ ");
                for (uint32_t i = 0; i < tupleValue.Size(); ++i)
                {
                    const Value::TupleValue::Reader v = tupleValue[i];
                    uint32_t fieldIndex = ~0u;
                    for (uint32_t j = 0; j < fields.Size(); ++j)
                    {
                        if (fields[j].Name() == v.Name())
                        {
                            fieldIndex = j;
                            break;
                        }
                    }
                    HE_ASSERT(fieldIndex != ~0u);
                    m_writer.Write("{} = ", v.Name().AsView());

                    Field::Reader field = fields[fieldIndex];
                    Field::Meta::Normal::Reader norm = field.Meta().Normal();
                    WriteValue(norm.Type(), scope, v.Value());

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
