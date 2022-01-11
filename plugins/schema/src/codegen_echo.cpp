// Copyright Chad Engler

#include "codegen_echo.h"

#include "he/core/span_fmt.h"
#include "he/core/string_fmt.h"

#include <iostream>

namespace he::schema
{
    #define HE_ID_FMT "@{:#018x}"

    CodeGenEcho::CodeGenEcho(const CodeGenRequest& request)
        : m_request(request)
    {}

    bool CodeGenEcho::Generate()
    {
        m_writer.Reserve(8192);

        m_writer.WriteLine(HE_ID_FMT, m_request.schema.root.id);

        for (const Import& im : m_request.schema.imports)
        {
            m_writer.WriteLine("import \"{}\"", im.path);
        }

        if (!m_request.schema.root.name.IsEmpty())
        {
            m_writer.WriteLine("namespace {}", m_request.schema.root.name);
        }

        for (const Attribute& attr : m_request.schema.attributes)
        {
            WriteAttribute(attr, m_request.schema.root);
            m_writer.Write(";\n");
        }

        for (const Declaration& decl : m_request.schema.root.children)
        {
            WriteDecl(decl, m_request.schema.root);
        }

        m_writer.Write('\0');
        std::cout << m_writer.Str().Data() << std::endl;

        return true;
    }

    void CodeGenEcho::WriteDecl(const Declaration& decl, const Declaration& scope)
    {
        switch (decl.kind)
        {
            case DeclKind::None:
                break;
            case DeclKind::Attribute:
                WriteAttributeDecl(decl, scope);
                break;
            case DeclKind::Const:
                WriteConstDecl(decl, scope);
                break;
            case DeclKind::Enum:
                WriteEnumDecl(decl, scope);
                break;
            case DeclKind::Interface:
                WriteInterfaceDecl(decl, scope);
                break;
            case DeclKind::Struct:
                WriteStructDecl(decl, scope);
                break;
        }
    }

    void CodeGenEcho::WriteAttributeDecl(const Declaration& decl, const Declaration& scope)
    {
        String targets(Allocator::GetTemp());

        if (decl.attribute_.targetsAttribute)
            targets += "attribute,";
        if (decl.attribute_.targetsConst)
            targets += "const,";
        if (decl.attribute_.targetsEnum)
            targets += "enum,";
        if (decl.attribute_.targetsEnumerator)
            targets += "enumerator,";
        if (decl.attribute_.targetsField)
            targets += "field,";
        if (decl.attribute_.targetsFile)
            targets += "file,";
        if (decl.attribute_.targetsInterface)
            targets += "interface,";
        if (decl.attribute_.targetsMethod)
            targets += "method,";
        if (decl.attribute_.targetsParameter)
            targets += "parameter,";
        if (decl.attribute_.targetsStruct)
            targets += "struct,";

        if (!targets.IsEmpty())
            targets.PopBack();

        m_writer.WriteIndent();
        m_writer.Write("attribute {} " HE_ID_FMT " ({}) :", decl.name, decl.id, targets);
        WriteType(decl.attribute_.type, scope);
        WriteAttributes(decl.attributes, scope);
        m_writer.Write(";\n");
    }

    void CodeGenEcho::WriteConstDecl(const Declaration& decl, const Declaration& scope)
    {
        m_writer.WriteIndent();
        m_writer.Write("const {} " HE_ID_FMT " :", decl.name, decl.id);
        WriteType(decl.const_.type, scope);
        m_writer.Write(" = ");
        WriteValue(decl.const_.type, scope, decl.const_.value);
        WriteAttributes(decl.attributes, scope);
        m_writer.Write(";\n");
    }

    void CodeGenEcho::WriteEnumDecl(const Declaration& decl, const Declaration& scope)
    {
        m_writer.WriteIndent();
        m_writer.Write("enum {} " HE_ID_FMT, decl.name, decl.id);
        WriteAttributes(decl.attributes, scope);
        m_writer.Write('\n');

        m_writer.WriteLine("{");
        m_writer.IncreaseIndent();

        for (const Enumerator& e : decl.enum_.enumerators)
        {
            m_writer.WriteIndent();
            m_writer.Write("{} @{}", e.name, e.ordinal);
            if (!e.attributes.IsEmpty())
            {
                m_writer.Write(' ');
                WriteAttributes(e.attributes, scope);
            }
            m_writer.Write(";\n");
        }

        m_writer.DecreaseIndent();
        m_writer.WriteLine("}");
    }

    void CodeGenEcho::WriteInterfaceDecl(const Declaration& decl, const Declaration& scope)
    {
        m_writer.WriteIndent();
        m_writer.Write("interface {}", decl.name);
        WriteTypeParams(decl.typeParams);
        m_writer.Write(" " HE_ID_FMT, decl.id);
        if (decl.interface_.super.kind == TypeKind::Interface)
        {
            m_writer.Write(" extends ");
            WriteType(decl.interface_.super, scope);
        }
        WriteAttributes(decl.attributes, scope);
        m_writer.Write('\n');

        m_writer.WriteLine("{");
        m_writer.IncreaseIndent();

        for (const Declaration& child : decl.children)
        {
            if (child.kind != DeclKind::Struct || !child.struct_.isAutoGenerated)
            {
                WriteDecl(child, decl);
            }
        }

        for (const Method& method : decl.interface_.methods)
        {
            m_writer.WriteIndent();
            m_writer.Write("{} @{} ", method.name, method.ordinal);

            const Declaration& paramStruct = m_request.GetDecl(method.paramStruct);
            WriteTuple(paramStruct);

            m_writer.Write(" -> ");

            if (method.resultStruct != 0)
            {
                const Declaration& resultStruct = m_request.GetDecl(method.resultStruct);
                WriteTuple(resultStruct);
            }
            else
            {
                m_writer.Write("void");
            }
            m_writer.Write(";\n");
        }

        m_writer.DecreaseIndent();
        m_writer.WriteLine("}");
    }

    void CodeGenEcho::WriteStructDecl(const Declaration& decl, const Declaration& scope)
    {
        if (!decl.struct_.isGroup && !decl.struct_.isUnion)
        {
            m_writer.WriteIndent();
            m_writer.Write("struct {}", decl.name);
            WriteTypeParams(decl.typeParams);
            m_writer.Write(" " HE_ID_FMT, decl.id);
            WriteAttributes(decl.attributes, scope);
            m_writer.Write(" // {} bytes, {} pointers", decl.struct_.dataWordSize * 8, decl.struct_.pointerCount);
        }

        m_writer.Write('\n');
        m_writer.WriteLine("{");
        m_writer.IncreaseIndent();

        for (const Declaration& child : decl.children)
        {
            if (child.kind != DeclKind::Struct || !child.struct_.isAutoGenerated)
            {
                WriteDecl(child, decl);
            }
        }

        for (const Field& field : decl.struct_.fields)
        {
            if (field.isGroup || field.isUnion)
            {
                HE_ASSERT(field.type.kind == TypeKind::Struct);
                const Declaration& group = m_request.GetDecl(field.type.struct_.id);
                m_writer.WriteIndent();
                if (group.struct_.isGroup)
                {
                    m_writer.Write("{} :group", field.name);
                    if (decl.struct_.isUnion)
                    {
                        m_writer.Write(" // union tag = {}", field.unionTag);
                    }
                }
                else if (group.struct_.isUnion)
                {
                    const uint32_t tagSize = 16;
                    const uint32_t begin = decl.struct_.unionTagOffset * tagSize;
                    const uint32_t end = begin + tagSize;
                    m_writer.Write("{} :union // tag bits[{}, {})", field.name, begin, end);
                    if (decl.struct_.isUnion)
                    {
                        m_writer.Write(", union tag = {}", field.unionTag);
                    }
                }
                WriteAttributes(group.attributes, scope);
                WriteStructDecl(group, decl);
                continue;
            }

            if (IsPointer(field.type))
            {
                m_writer.WriteIndent();
                WriteField(field, decl);
                m_writer.Write("; // ptr[{}]", field.index);
            }
            else
            {
                const uint32_t fieldSize = GetTypeSize(field.type);
                const uint32_t begin = field.dataOffset * fieldSize;
                const uint32_t end = begin + fieldSize;
                m_writer.WriteIndent();
                WriteField(field, decl);
                m_writer.Write("; // bits[{}, {})", begin, end);
            }

            if (decl.struct_.isUnion)
            {
                m_writer.Write(", union tag = {}", field.unionTag);
            }
            m_writer.Write('\n');
        }

        m_writer.DecreaseIndent();
        m_writer.WriteLine("}");
    }

    void CodeGenEcho::WriteAttribute(const Attribute& attribute, const Declaration& scope)
    {
        const Declaration& decl = m_request.GetDecl(attribute.id);
        HE_ASSERT(decl.kind == DeclKind::Attribute);
        m_writer.Write("${}", decl.name);

        if (attribute.value.kind != TypeKind::Void)
        {
            m_writer.Write('(');
            WriteValue(decl.attribute_.type, scope, attribute.value);
            m_writer.Write(')');
        }
    }

    void CodeGenEcho::WriteAttributes(Span<const Attribute> attributes, const Declaration& scope)
    {
        for (uint32_t i = 0; i < attributes.Size(); ++i)
        {
            m_writer.Write(' ');
            const Attribute& attr = attributes[i];
            WriteAttribute(attr, scope);
        }
    }

    void CodeGenEcho::WriteField(const Field& field, const Declaration& scope)
    {
        m_writer.Write("{} @{} :", field.name, field.ordinal);
        WriteType(field.type, scope);

        if (field.defaultValue.kind != TypeKind::Void)
        {
            m_writer.Write(" = ");
            WriteValue(field.type, scope, field.defaultValue);
        }

        WriteAttributes(field.attributes, scope);
    }

    void CodeGenEcho::WriteName(const Declaration& decl, const Declaration& scope, const Brand& brand)
    {
        if (decl.parentId != scope.id && decl.parentId != scope.parentId && decl.parentId != m_request.schema.root.id)
        {
            const Declaration& parent = m_request.GetDecl(decl.parentId);
            WriteName(parent, scope, brand);
            m_writer.Write('.');
        }

        m_writer.Write(decl.name);

        for (const Brand::Scope& brandScope : brand.scopes)
        {
            if (brandScope.scopeId == decl.id)
            {
                m_writer.Write("<");
                for (uint32_t i = 0; i < brandScope.params.Size(); ++i)
                {
                    const Type* t = brandScope.params[i];
                    WriteType(*t, scope);
                    if (i < (brandScope.params.Size() - 1))
                        m_writer.Write(", ");
                }
                m_writer.Write(">");
                break;
            }
        }
    }

    void CodeGenEcho::WriteTuple(const Declaration& decl)
    {
        m_writer.Write('(');

        for (uint32_t i = 0; i < decl.struct_.fields.Size(); ++i)
        {
            const Field& field = decl.struct_.fields[i];
            WriteField(field, decl);

            if (i != (decl.struct_.fields.Size() - 1))
                m_writer.Write(", ");
        }

        m_writer.Write(')');
    }


    void CodeGenEcho::WriteTypeParams(Span<const String> typeParams)
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

    void CodeGenEcho::WriteType(const Type& type, const Declaration& scope)
    {
        switch (type.kind)
        {
            case TypeKind::Void: m_writer.Write("void"); break;
            case TypeKind::Bool: m_writer.Write("bool"); break;
            case TypeKind::Int8: m_writer.Write("int8"); break;
            case TypeKind::Int16: m_writer.Write("int16"); break;
            case TypeKind::Int32: m_writer.Write("int32"); break;
            case TypeKind::Int64: m_writer.Write("int64"); break;
            case TypeKind::Uint8: m_writer.Write("uint8"); break;
            case TypeKind::Uint16: m_writer.Write("uint16"); break;
            case TypeKind::Uint32: m_writer.Write("uint32"); break;
            case TypeKind::Uint64: m_writer.Write("uint64"); break;
            case TypeKind::Float32: m_writer.Write("float32"); break;
            case TypeKind::Float64: m_writer.Write("float64"); break;
            case TypeKind::Array:
                WriteType(*type.array_.elementType, scope);
                m_writer.Write("[{}]", type.array_.size);
                break;
            case TypeKind::Blob:
                m_writer.Write("Blob");
                break;
            case TypeKind::String:
                m_writer.Write("String");
                break;
            case TypeKind::List:
                m_writer.Write("List<");
                WriteType(*type.list_.elementType, scope);
                m_writer.Write(">");
                break;
            case TypeKind::Enum:
            {
                const Declaration& decl = m_request.GetDecl(type.enum_.id);
                HE_ASSERT(decl.kind == DeclKind::Enum);
                WriteName(decl, scope, type.enum_.brand);
                break;
            }
            case TypeKind::Struct:
            {
                const Declaration& decl = m_request.GetDecl(type.struct_.id);
                HE_ASSERT(decl.kind == DeclKind::Struct);
                WriteName(decl, scope, type.struct_.brand);
                break;
            }
            case TypeKind::Interface:
            {
                const Declaration& decl = m_request.GetDecl(type.interface_.id);
                HE_ASSERT(decl.kind == DeclKind::Interface);
                WriteName(decl, scope, type.interface_.brand);
                break;
            }
            case TypeKind::AnyPointer:
            {
                if (type.any_.paramScopeId == 0)
                {
                    m_writer.Write("AnyPointer");
                }
                else
                {
                    const Declaration& decl = m_request.GetDecl(type.any_.paramScopeId);
                    m_writer.Write(decl.typeParams[type.any_.paramIndex]);
                }
                break;
            }
        }
    }

    void CodeGenEcho::WriteValue(const Type& type, const Declaration& scope, const Value& value)
    {
        HE_ASSERT(type.kind == value.kind);

        switch (value.kind)
        {
            case TypeKind::Void: break;
            case TypeKind::Bool: m_writer.Write("{}", value.b); break;
            case TypeKind::Int8: m_writer.Write("{}", value.i8); break;
            case TypeKind::Int16: m_writer.Write("{}", value.i16); break;
            case TypeKind::Int32: m_writer.Write("{}", value.i32); break;
            case TypeKind::Int64: m_writer.Write("{}", value.i64); break;
            case TypeKind::Uint8: m_writer.Write("{}", value.u8); break;
            case TypeKind::Uint16: m_writer.Write("{}", value.u16); break;
            case TypeKind::Uint32: m_writer.Write("{}", value.u32); break;
            case TypeKind::Uint64: m_writer.Write("{}", value.u64); break;
            case TypeKind::Float32: m_writer.Write("{}", value.f32); break;
            case TypeKind::Float64: m_writer.Write("{}", value.f64); break;
            case TypeKind::Array:
                m_writer.Write('[');
                for (uint32_t i = 0; i < value.array.Size(); ++i)
                {
                    WriteValue(*type.array_.elementType, scope, *value.array[i]);

                    if (i != (value.array.Size() - 1))
                        m_writer.Write(", ");
                }
                m_writer.Write(']');
                break;
            case TypeKind::Blob:
            {
                Span<const uint8_t> bytes = value.blob;
                m_writer.Write("0x\"{}\"", bytes);
                break;
            }
            case TypeKind::String:
                m_writer.Write("\"{}\"", value.str);
                break;
            case TypeKind::List:
                m_writer.Write('[');
                for (uint32_t i = 0; i < value.list.Size(); ++i)
                {
                    WriteValue(*type.list_.elementType, scope, *value.list[i]);

                    if (i != (value.list.Size() - 1))
                        m_writer.Write(", ");
                }
                m_writer.Write(']');
                break;
            case TypeKind::Enum:
            {
                const Declaration& decl = m_request.GetDecl(type.enum_.id);
                HE_ASSERT(decl.kind == DeclKind::Enum);
                WriteName(decl, scope, type.enum_.brand);
                m_writer.Write('.');
                for (const Enumerator& e : decl.enum_.enumerators)
                {
                    if (e.ordinal == value.enum_)
                    {
                        m_writer.Write(e.name);
                        break;
                    }
                }
                break;
            }
            case TypeKind::Struct:
            {
                const Declaration& decl = m_request.GetDecl(type.struct_.id);
                HE_ASSERT(decl.kind == DeclKind::Struct);

                m_writer.Write("{ ");
                for (uint32_t i = 0; i < value.struct_.Size(); ++i)
                {
                    const Value::StructValue& v = value.struct_[i];
                    const Field* field = nullptr;
                    for (uint32_t j = 0; j < decl.struct_.fields.Size(); ++j)
                    {
                        if (decl.struct_.fields[j].name == v.fieldName)
                        {
                            field = &decl.struct_.fields[j];
                            break;
                        }
                    }
                    HE_ASSERT(field);
                    m_writer.Write("{} = ", v.fieldName);
                    WriteValue(field->type, scope, *v.value);

                    if (i != (value.list.Size() - 1))
                        m_writer.Write(", ");
                }
                m_writer.Write(" }");
                break;
            }
            case TypeKind::Interface:
                break;
            case TypeKind::AnyPointer:
                break;
        }
    }

    bool GenerateEcho(const CodeGenRequest& request)
    {
        CodeGenEcho generator(request);
        if (!generator.Generate())
            return false;

        return true;
    }
}
