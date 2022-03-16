// Copyright Chad Engler

#include "compiler.h"

#include "compile_context.h"
#include "keywords.h"
#include "struct_layout.h"

#include "he/core/ascii.h"
#include "he/core/enum_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view_fmt.h"
#include "he/core/vector.h"

#include <concepts>
#include <set>
#include <unordered_set>

namespace he::schema
{
    bool Compiler::Compile(const AstFile& ast, CompileContext& ctx)
    {
        m_context = &ctx;
        m_builder.Clear();
        m_valid = true;

        SchemaFile::Builder schema = m_builder.AddStruct<SchemaFile>();
        m_builder.SetRoot(schema);
        CompileNode(ast.root, schema.InitRoot());

        return m_valid;
    }

    void Compiler::CompileNode(const AstNode& node, Declaration::Builder decl)
    {
        decl.InitName(node.name);
        decl.SetId(node.id);
        if (node.parent)
            decl.SetParentId(node.parent->id);
        if (m_context->Config().includeSourceInfo)
        {
            SourceInfo::Builder source = decl.InitSource();
            source.InitDocComment(node.docComment);
            source.InitFile(m_context->Path());
            source.SetLine(node.location.line);
            source.SetColumn(node.location.column);
        }
        decl.SetAttributes(CreateAttributes(node.attributes, node));
        decl.SetTypeParams(CreateTypeParams(node.typeParams));

        switch (node.kind)
        {
            case AstNode::Kind::Attribute:
                CompileAttribute(node, decl);
                break;
            case AstNode::Kind::Constant:
                CompileConstant(node, decl);
                break;
            case AstNode::Kind::Enum:
                CompileEnum(node, decl);
                break;
            case AstNode::Kind::File:
                CompileFile(node, decl);
                break;
            case AstNode::Kind::Interface:
                CompileInterface(node, decl);
                break;
            case AstNode::Kind::Struct:
                CompileStruct(node, decl);
                break;
            case AstNode::Kind::Group:
            case AstNode::Kind::Union:
            case AstNode::Kind::Alias:
            case AstNode::Kind::Field:
            case AstNode::Kind::Enumerator:
            case AstNode::Kind::Method:
                m_context->AddError(node.location, "Expected declaration, but encountered {}", node.kind);
                m_valid = false;
                break;
        }
    }

    void Compiler::CompileAttribute(const AstNode& node, Declaration::Builder decl)
    {
        Declaration::Data::Attribute::Builder data = decl.Data().InitAttribute();
        data.SetType(CreateType(node.attribute.type, *node.parent));
        data.SetTargetsAttribute(node.attribute.targetsAttribute);
        data.SetTargetsConstant(node.attribute.targetsConstant);
        data.SetTargetsEnum(node.attribute.targetsEnum);
        data.SetTargetsEnumerator(node.attribute.targetsEnumerator);
        data.SetTargetsField(node.attribute.targetsField);
        data.SetTargetsFile(node.attribute.targetsFile);
        data.SetTargetsInterface(node.attribute.targetsInterface);
        data.SetTargetsMethod(node.attribute.targetsMethod);
        data.SetTargetsParameter(node.attribute.targetsParameter);
        data.SetTargetsStruct(node.attribute.targetsStruct);
    }

    void Compiler::CompileConstant(const AstNode& node, Declaration::Builder decl)
    {
        Declaration::Data::Constant::Builder data = decl.Data().InitConstant();
        data.SetType(CreateType(node.constant.type, node));
        data.SetValue(CreateValue(data.Type(), node.constant.value, node));
    }

    void Compiler::CompileEnum(const AstNode& node, Declaration::Builder decl)
    {
        Declaration::Data::Enum::Builder data = decl.Data().InitEnum();
        List<Enumerator>::Builder members = data.InitEnumerators(node.children.Size());

        uint16_t i = 0;
        for (const AstNode& child : node.children)
        {
            Enumerator::Builder member = members[i];
            member.InitName(child.name);
            member.SetDeclOrder(i);
            member.SetOrdinal(static_cast<uint16_t>(child.id));
            member.SetAttributes(CreateAttributes(child.attributes, node));

            ++i;
        }
    }

    void Compiler::CompileFile(const AstNode& node, Declaration::Builder decl)
    {
        Declaration::Data::File::Builder fileDecl = decl.Data().InitFile();

        he::String buf(Allocator::GetTemp());
        for (const AstExpression& item : node.file.nameSpace.qualified.names)
        {
            HE_ASSERT(item.kind == AstExpression::Kind::Identifier);
            buf += item.identifier;
            buf += '.';
        }
        if (!buf.IsEmpty())
            buf.PopBack();
        decl.InitName(buf);

        uint16_t importIndex = 0;
        List<String>::Builder fileImports = fileDecl.InitImports(node.file.imports.Size());
        for (const AstExpression& import : node.file.imports)
        {
            buf.Clear();
            if (!m_context->DecodeString(import, buf))
            {
                m_valid = false;
                return;
            }

            fileImports.Set(importIndex++, m_builder.AddString(buf));
        }

        uint16_t childCount = 0;
        for (const AstNode& child : node.children)
        {
            if (child.kind != AstNode::Kind::Alias)
            {
                ++childCount;
            }
        }

        List<Declaration>::Builder decls = decl.InitChildren(childCount);

        uint16_t childIndex = 0;
        for (const AstNode& child : node.children)
        {
            if (child.kind != AstNode::Kind::Alias)
            {
                CompileNode(child, decls[childIndex++]);
            }
        }
    }

    void Compiler::CompileInterface(const AstNode& node, Declaration::Builder decl)
    {
        Declaration::Data::Interface::Builder data = decl.Data().InitInterface();

        uint16_t methodCount = 0;
        uint16_t childCount = 0;
        for (const AstNode& child : node.children)
        {
            if (child.kind == AstNode::Kind::Method)
            {
                ++methodCount;
                if (child.method.params.kind == AstMethodParams::Kind::Fields)
                    ++childCount;
                if (child.method.results.kind == AstMethodParams::Kind::Fields)
                    ++childCount;
            }
            else if (child.kind != AstNode::Kind::Alias)
            {
                ++childCount;
            }
        }

        List<Method>::Builder members = data.InitMethods(methodCount);
        List<Declaration>::Builder children = decl.InitChildren(childCount);

        uint16_t methodIndex = 0;
        uint16_t childIndex = 0;
        for (const AstNode& child : node.children)
        {
            if (child.kind == AstNode::Kind::Method)
            {
                Method::Builder method = members[methodIndex];
                method.InitName(child.name);
                method.SetDeclOrder(methodIndex);
                method.SetOrdinal(static_cast<uint16_t>(child.id));
                method.SetParamStruct(CompileMethodParams(node, child, child.method.params, children, childIndex, "_Params"));
                method.SetResultStruct(CompileMethodParams(node, child, child.method.results, children, childIndex, "_Results"));
                method.SetAttributes(CreateAttributes(child.attributes, node));
                method.SetTypeParams(CreateTypeParams(child.typeParams));
                ++methodIndex;
            }
            else if (child.kind != AstNode::Kind::Alias)
            {
                Declaration::Builder childDecl = children[childIndex];
                CompileNode(child, childDecl);
                ++childIndex;
            }
        }
    }

    void Compiler::CompileStruct(const AstNode& node, Declaration::Builder decl)
    {
        Declaration::Data::Struct::Builder data = decl.Data().InitStruct();
        data.SetIsGroup(node.kind == AstNode::Kind::Group);
        data.SetIsUnion(node.kind == AstNode::Kind::Union);

        uint16_t fieldCount = 0;
        uint16_t childCount = 0;
        for (const AstNode& child : node.children)
        {
            if (child.kind == AstNode::Kind::Field || child.kind == AstNode::Kind::Group || child.kind == AstNode::Kind::Union)
                ++fieldCount;

            if (child.kind != AstNode::Kind::Field)
                ++childCount;
        }

        List<Field>::Builder members = data.InitFields(fieldCount);
        List<Declaration>::Builder children = decl.InitChildren(childCount);

        uint16_t fieldIndex = 0;
        uint16_t childIndex = 0;
        for (const AstNode& child : node.children)
        {
            switch (child.kind)
            {
                case AstNode::Kind::Field:
                {
                    Field::Builder field = members[fieldIndex];
                    CompileField(child, field, fieldIndex);
                    ++fieldIndex;
                    break;
                }
                case AstNode::Kind::Group:
                {
                    Field::Builder field = members[fieldIndex];
                    field.InitName(child.name);
                    field.SetDeclOrder(fieldIndex);
                    field.SetUnionTag(0); // Set during struct layout
                    field.SetAttributes(CreateAttributes(child.attributes, node));

                    Declaration::Builder groupStruct = children[childIndex];
                    he::String name(Allocator::GetTemp());
                    name = child.name;
                    name[0] = ToUpper(name[0]);
                    groupStruct.InitName(name);
                    groupStruct.SetId(MakeTypeId(name, decl.Id()));
                    groupStruct.SetParentId(decl.Id());
                    CompileStruct(child, groupStruct);

                    Field::Meta::Group::Builder group = field.Meta().InitGroup();
                    group.SetTypeId(groupStruct.Id());

                    ++childIndex;
                    ++fieldIndex;
                    break;
                }
                case AstNode::Kind::Union:
                {
                    Field::Builder field = members[fieldIndex];
                    field.InitName(child.name);
                    field.SetDeclOrder(fieldIndex);
                    field.SetUnionTag(0); // Set during struct layout
                    field.SetAttributes(CreateAttributes(child.attributes, node));

                    Declaration::Builder unionStruct = children[childIndex];
                    he::String name(Allocator::GetTemp());
                    name = child.name;
                    name[0] = ToUpper(name[0]);
                    unionStruct.InitName(name);
                    unionStruct.SetId(MakeTypeId(name, decl.Id()));
                    unionStruct.SetParentId(decl.Id());
                    CompileStruct(child, unionStruct);

                    Field::Meta::Union::Builder unionBuilder = field.Meta().InitUnion();
                    unionBuilder.SetTypeId(unionStruct.Id());

                    ++childIndex;
                    ++fieldIndex;
                    break;
                }
                default:
                {
                    Declaration::Builder childDecl = children[childIndex];
                    CompileNode(child, childDecl);
                    ++childIndex;
                    break;
                }
            }
        }

        if (!data.IsGroup() && !data.IsUnion())
        {
            StructLayout layout(decl);
            layout.CalculateLayout();
        }
    }

    void Compiler::CompileField(const AstNode& node, Field::Builder field, uint16_t index)
    {
        HE_ASSERT(node.kind == AstNode::Kind::Field);
        field.InitName(node.name);
        field.SetDeclOrder(index);
        field.SetUnionTag(0); // Set during struct layout
        field.SetAttributes(CreateAttributes(node.attributes, node));
        Field::Meta::Normal::Builder normal = field.Meta().InitNormal();
        normal.SetType(CreateType(node.field.type, *node.parent));
        normal.SetOrdinal(static_cast<uint16_t>(node.id));
        normal.SetIndex(0); // Set during struct layout
        normal.SetDefaultValue(CreateValue(normal.Type(), node.field.defaultValue, node));
        normal.SetDataOffset(0); // Set during struct layout
    }

    TypeId Compiler::CompileMethodParams(
        const AstNode& node,
        const AstNode& child,
        const AstMethodParams& params,
        List<Declaration>::Builder children,
        uint16_t& childIndex,
        StringView suffix)
    {
        switch (params.kind)
        {
            case AstMethodParams::Kind::Fields:
            {
                Declaration::Builder paramStruct = children[childIndex];
                he::String name(Allocator::GetTemp());
                name = child.name;
                name += suffix;
                paramStruct.InitName(name);
                paramStruct.SetId(MakeTypeId(name, node.id));
                paramStruct.SetParentId(node.id);
                paramStruct.SetTypeParams(CreateTypeParams(child.typeParams));

                Declaration::Data::Struct::Builder paramData = paramStruct.Data().InitStruct();
                paramData.SetIsMethodParams(true);
                paramData.SetIsMethodResults(true);

                List<Field>::Builder members = paramData.InitFields(params.fields.Size());
                uint16_t fieldIndex = 0;
                for (const AstNode& fieldNode : params.fields)
                {
                    Field::Builder field = members[fieldIndex];
                    CompileField(fieldNode, field, fieldIndex);
                    ++fieldIndex;
                }

                ++childIndex;
                return paramStruct.Id();
            }
            case AstMethodParams::Kind::Type:
            {
                // implicit void type, usually from omitting the result type
                if (params.type.kind == AstExpression::Kind::Unknown)
                    return 0;

                HE_ASSERT(params.type.kind == AstExpression::Kind::QualifiedName);

                // check for explicit use of the void keyword
                if (params.type.qualified.names.Size() == 1)
                {
                    const AstExpression* name = params.type.qualified.names.Front();
                    if (name->kind == AstExpression::Kind::Identifier && name->identifier == KW_Void)
                        return 0;
                }

                // otherwise, must be an explicit structure type
                const AstNode* v = m_context->FindNode(params.type, node);
                HE_ASSERT(v && v->kind == AstNode::Kind::Struct);
                return v->id;
            }
            default:
                HE_ASSERT(false, "Invalid method params type. Verify should've caught this.");
                return 0;
        }
    }

    Type::Builder Compiler::CreateType(const AstExpression& ast, const AstNode& scope)
    {
        const TypeValue& info = m_context->GetType({ &ast, &scope });
        Type::Builder type = m_builder.AddStruct<Type>();
        Type::Data::Builder data = type.Data();

        if (info.type)
        {
            HE_ASSERT(ast.kind == AstExpression::Kind::QualifiedName);
            HE_ASSERT(info.tag == Type::Data::Tag::AnyPointer);
            switch (info.type->kind)
            {
                case AstNode::Kind::Alias:
                    return CreateType(info.type->alias.target, *info.type->parent);
                case AstNode::Kind::Enum:
                {
                    Type::Data::Enum::Builder e = data.InitEnum();
                    e.SetId(info.type->id);
                    e.SetBrand(CreateTypeBrand(ast, scope));
                    break;
                }
                case AstNode::Kind::Interface:
                {
                    Type::Data::Interface::Builder i = data.InitInterface();
                    i.SetId(info.type->id);
                    i.SetBrand(CreateTypeBrand(ast, scope));
                    break;
                }
                case AstNode::Kind::Struct:
                {
                    Type::Data::Struct::Builder s = data.InitStruct();
                    s.SetId(info.type->id);
                    s.SetBrand(CreateTypeBrand(ast, scope));
                    break;
                }
                default:
                    HE_ASSERT(false, "Invalid type node. Verify should've checked this.");
                    break;
            }
            return type;
        }

        switch (info.tag)
        {
            case Type::Data::Tag::Array:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::Array);
                Type::Builder elementType = CreateType(*ast.array.elementType, scope);
                Type::Data::Array::Builder arr = data.InitArray();
                arr.SetElementType(elementType);
                arr.SetSize(GetArraySize(*ast.array.size, scope));
                break;
            }
            case Type::Data::Tag::List:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::Array);
                Type::Builder elementType = CreateType(*ast.array.elementType, scope);
                Type::Data::List::Builder list = data.InitList();
                list.SetElementType(elementType);
                break;
            }
            case Type::Data::Tag::AnyPointer:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::QualifiedName);
                Type::Data::AnyPointer::Builder any = data.InitAnyPointer();

                // check if this is a generic parameter, if not we assume it is the keyword
                const AstTypeParamRef& ref = info.typeParamRef;
                if (ref.scope)
                {
                    any.SetParamScopeId(ref.scope->id);
                    any.SetParamIndex(ref.index);
                }
                else
                {
                    HE_ASSERT(ast.qualified.names.Size() == 1);
                    HE_ASSERT(ast.qualified.names.Front()->kind == AstExpression::Kind::Identifier);
                    HE_ASSERT(ast.qualified.names.Front()->identifier == KW_AnyPointer);
                }
                break;
            }
            default:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::QualifiedName);
                HE_ASSERT(ast.qualified.names.Size() == 1 && ast.qualified.names.Front()->kind == AstExpression::Kind::Identifier);
                HE_ASSERT(info.tag != Type::Data::Tag::Enum && info.tag != Type::Data::Tag::Struct && info.tag != Type::Data::Tag::Interface);
                data.SetTag(info.tag);
                break;
            }
        }

        return type;
    }

    Brand::Builder Compiler::CreateTypeBrand(const AstExpression& name, const AstNode& scope_)
    {
        HE_ASSERT(name.kind == AstExpression::Kind::QualifiedName);

        // Count the number of generics we have in this name
        uint16_t scopeCount = 0;
        for (const AstExpression& child : name.qualified.names)
        {
            if (child.kind == AstExpression::Kind::Generic)
                ++scopeCount;
        }

        // Go through each name of the qualified name
        const AstNode* scope = &scope_;
        uint16_t scopeIndex = 0;
        Brand::Builder brand = m_builder.AddStruct<Brand>();
        List<Brand::Scope>::Builder brandScopes = brand.InitScopes(scopeCount);
        for (const AstExpression& child : name.qualified.names)
        {
            HE_ASSERT(child.kind == AstExpression::Kind::Identifier || child.kind == AstExpression::Kind::Generic);

            if (child.kind == AstExpression::Kind::Generic)
            {
                scope = m_context->FindNode(child.identifier, *scope);
                HE_ASSERT(scope);

                Brand::Scope::Builder brandScope = brandScopes[scopeIndex++];
                brandScope.SetScopeId(scope->id);

                uint16_t paramIndex = 0;
                List<Type>::Builder brandParams = brandScope.InitParams(child.generic.params.Size());
                for (const AstExpression& astType : child.generic.params)
                {
                    Type::Builder type = CreateType(astType, scope_); // use original scope
                    brandParams.Set(paramIndex++, type);
                }
            }
        }

        return brand;
    }

    template <std::integral T>
    bool Compiler::SetInt(const AstFileLocation& location, T value, Type::Data::Reader type, Value::Data::Builder data)
    {
        if (type.IsInt8())
        {
            if (value > std::numeric_limits<int8_t>::max() || value < std::numeric_limits<int8_t>::min())
            {
                m_context->AddError(location, "Integer value out of range for type 'int8'");
                return false;
            }

            data.SetInt8(static_cast<int8_t>(value));
            return true;
        }

        if (type.IsInt16())
        {
            if (value > std::numeric_limits<int16_t>::max() || value < std::numeric_limits<int16_t>::min())
            {
                m_context->AddError(location, "Integer value out of range for type 'int16'");
                return false;
            }

            data.SetInt16(static_cast<int16_t>(value));
            return true;
        }

        if (type.IsInt32())
        {
            if (value > std::numeric_limits<int32_t>::max() || value < std::numeric_limits<int32_t>::min())
            {
                m_context->AddError(location, "Integer value out of range for type 'int32'");
                return false;
            }

            data.SetInt32(static_cast<int32_t>(value));
            return true;
        }

        if (type.IsInt64())
        {
            if (static_cast<uint64_t>(value) > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
            {
                m_context->AddError(location, "Integer value out of range for type 'int64'");
                return false;
            }

            data.SetInt64(static_cast<int64_t>(value));
            return true;
        }

        if (type.IsUint8())
        {
            if (value != -1 && (value > std::numeric_limits<uint8_t>::max() || value < std::numeric_limits<uint8_t>::min()))
            {
                m_context->AddError(location, "Integer value out of range for type 'uint8'");
                return false;
            }

            data.SetUint8(static_cast<int8_t>(value));
            return true;
        }

        if (type.IsUint16())
        {
            if (value != -1 && (value > std::numeric_limits<uint16_t>::max() || value < std::numeric_limits<uint16_t>::min()))
            {
                m_context->AddError(location, "Integer value out of range for type 'uint16'");
                return false;
            }

            data.SetUint16(static_cast<int16_t>(value));
            return true;
        }

        if (type.IsUint32())
        {
            if (value != -1 && (value > std::numeric_limits<uint32_t>::max() || value < std::numeric_limits<uint32_t>::min()))
            {
                m_context->AddError(location, "Integer value out of range for type 'uint32'");
                return false;
            }

            data.SetUint32(static_cast<uint32_t>(value));
            return true;
        }

        if (type.IsUint64())
        {
            if (value < 0 && static_cast<uint64_t>(value) != static_cast<uint64_t>(-1))
            {
                m_context->AddError(location, "Integer value out of range for type 'uint64'");
                return false;
            }

            data.SetUint64(static_cast<uint64_t>(value));
            return true;
        }

        m_context->AddError(location, "Expected {} value, but encountered integer expression", type.Tag());
        return {};
    }

    Value::Builder Compiler::CreateValue(Type::Reader type, const AstExpression& ast, const AstNode& scope)
    {
        if (ast.kind == AstExpression::Kind::Unknown)
            return {};

        Value::Builder value = m_builder.AddStruct<Value>();
        Value::Data::Builder data = value.Data();

        Type::Data::Reader typeData = type.Data();

        switch (ast.kind)
        {
            case AstExpression::Kind::Blob:
            {
                Vector<uint8_t> bytes(Allocator::GetTemp());
                if (!DecodeBlob(ast, bytes))
                    return {};

                List<uint8_t>::Builder list = data.InitBlob(bytes.Size());
                MemCopy(list.Data(), bytes.Data(), bytes.Size());
                break;
            }
            case AstExpression::Kind::Float:
            {
                const double v = ast.floatingPoint;
                if (typeData.IsFloat32())
                    data.SetFloat32(static_cast<float>(v));
                else if (typeData.IsFloat64())
                    data.SetFloat64(v);
                else
                {
                    m_context->AddError(ast.location, "Unexpected floating-point value");
                    m_valid = false;
                    return {};
                }
                break;
            }
            case AstExpression::Kind::List:
            {
                Type::Reader elementType = typeData.IsArray() ? typeData.Array().ElementType() : typeData.List().ElementType();
                List<Value>::Builder list = data.InitList(ast.list.Size());

                uint16_t i = 0;
                for (const AstExpression& item : ast.list)
                {
                    Value::Builder itemValue = CreateValue(elementType, item, scope);
                    list.Set(i++, itemValue);
                }
                break;
            }
            case AstExpression::Kind::QualifiedName:
            {
                const AstNode* valueNode = m_context->FindNode(ast, scope);
                HE_ASSERT(valueNode);
                HE_ASSERT(valueNode->kind == AstNode::Kind::Enumerator || valueNode->kind == AstNode::Kind::Constant);

                if (valueNode->kind == AstNode::Kind::Constant)
                    return CreateValue(type, valueNode->constant.value, *valueNode->parent);

                data.SetEnum(static_cast<uint16_t>(valueNode->id));
                break;
            }
            case AstExpression::Kind::SignedInt:
            {
                if (!SetInt(ast.location, ast.signedInt, typeData, data))
                {
                    m_valid = false;
                    return {};
                }
                break;
            }
            case AstExpression::Kind::String:
            {
                HE_ASSERT(typeData.IsString());

                he::String str(Allocator::GetTemp());
                if (!m_context->DecodeString(ast, str))
                {
                    m_valid = false;
                    return {};
                }
                data.InitString(str);
                break;
            }
            case AstExpression::Kind::Tuple:
            {
                HE_ASSERT(typeData.IsStruct());
                const Declaration::Reader typeDecl = m_context->GetDecl(typeData.Struct().Id());
                HE_ASSERT(typeDecl.IsValid() && typeDecl.Data().IsStruct());
                const Declaration::Data::Struct::Reader typeStructDecl = typeDecl.Data().Struct();

                StructBuilder st = m_builder.AddStruct(typeStructDecl.DataFieldCount(), typeStructDecl.DataWordSize(), typeStructDecl.PointerCount());
                SetStructValues(typeStructDecl, ast.tuple, st, scope);
                data.InitStruct().Set(st);
                break;
            }
            case AstExpression::Kind::UnsignedInt:
            {
                if (!SetInt(ast.location, ast.unsignedInt, typeData, data))
                {
                    m_valid = false;
                    return {};
                }
                break;
            }

            // These are used in types, not in values. Any of this is a syntax error.
            case AstExpression::Kind::Array:
            case AstExpression::Kind::Generic:
            case AstExpression::Kind::Identifier:
            case AstExpression::Kind::Namespace:
            case AstExpression::Kind::Unknown:
                HE_ASSERT(false, "Invalid value type. Verify should've caught this.");
                return {};
        }

        return value;
    }

    void Compiler::SetStructValues(Declaration::Data::Struct::Reader typeStructDecl, const AstList<AstTupleParam>& params, StructBuilder st, const AstNode& scope)
    {
        for (const AstTupleParam& param : params)
        {
            for (Field::Reader field : typeStructDecl.Fields())
            {
                if (param.name != field.Name())
                    continue;

                if (field.Meta().IsGroup())
                {
                    Declaration::Reader groupTypeDecl = m_context->GetDecl(field.Meta().Group().TypeId());
                    HE_ASSERT(groupTypeDecl.IsValid() && groupTypeDecl.Data().IsStruct());
                    HE_ASSERT(param.value.kind == AstExpression::Kind::Tuple);
                    SetStructValues(groupTypeDecl.Data().Struct(), param.value.tuple, st, scope);
                }
                else if (field.Meta().IsUnion())
                {
                    // TODO.
                    m_context->AddError(param.location, "Defaults for union members are not yet implemented.");
                    m_valid = false;
                    return;
                }
                else
                {
                    HE_ASSERT(field.Meta().IsNormal());
                    Field::Meta::Normal::Reader norm = field.Meta().Normal();
                    SetValue(param.value, norm.Type(), scope, st, norm.Index(), norm.DataOffset());
                }
            }
        }
    }

    template <typename BuilderType>
    struct ValueSetter;

    template <>
    struct ValueSetter<ListBuilder>
    {
        template <typename T>
        static void SetData(ListBuilder list, uint16_t index, uint32_t, T value)
        {
            list.SetDataElement(index, value);
        }

        template <typename T>
        static void SetPointer(ListBuilder list, uint16_t index, T value)
        {
            list.SetPointerElement(index, value);
        }

        static StructBuilder SetupStruct(Builder&, Declaration::Data::Struct::Reader, ListBuilder list, uint16_t index)
        {
            return list.GetCompositeElement(index);
        }
    };

    template <>
    struct ValueSetter<StructBuilder>
    {
        template <typename T>
        static void SetData(StructBuilder st, uint16_t index, uint32_t dataOffset, T value)
        {
            st.SetDataField(index, dataOffset, value);
        }

        template <typename T>
        static void SetPointer(StructBuilder st, uint16_t index, T value)
        {
            st.GetPointerField(index).Set(value);
        }

        static StructBuilder SetupStruct(Builder& builder, Declaration::Data::Struct::Reader typeStructDecl, StructBuilder st, uint16_t index)
        {
            StructBuilder value = builder.AddStruct(typeStructDecl.DataFieldCount(), typeStructDecl.DataWordSize(), typeStructDecl.PointerCount());
            st.GetPointerField(index).Set(value);
            return value;
        }
    };

    template <typename BuilderType>
    void Compiler::SetValue(
        const AstExpression& ast,
        Type::Reader type,
        const AstNode& scope,
        BuilderType obj,
        uint16_t index,
        uint32_t dataOffset)
    {
        using Setter = ValueSetter<BuilderType>;
        Type::Data::Reader typeData = type.Data();

        // Recurse for constants to resolve them
        if (ast.kind == AstExpression::Kind::QualifiedName)
        {
            const AstNode* valueNode = m_context->FindNode(ast, scope);
            if (valueNode && valueNode->kind == AstNode::Kind::Constant)
                return SetValue(valueNode->constant.value, type, *valueNode->parent, obj, index, dataOffset);
        }

        switch (typeData.Tag())
        {
            case Type::Data::Tag::Bool:
                HE_ASSERT(ast.kind == AstExpression::Kind::QualifiedName && ast.qualified.names.Size() == 1 && ast.qualified.names.Front()->kind == AstExpression::Kind::Identifier);
                Setter::SetData(obj, index, dataOffset, ast.qualified.names.Front()->identifier == KW_True);
                break;
            case Type::Data::Tag::Int8:
                HE_ASSERT(ast.kind == AstExpression::Kind::SignedInt || ast.kind == AstExpression::Kind::UnsignedInt);
                Setter::SetData(obj, index, dataOffset, static_cast<int8_t>(ast.kind == AstExpression::Kind::SignedInt ? ast.signedInt : ast.unsignedInt));
                break;
            case Type::Data::Tag::Int16:
                HE_ASSERT(ast.kind == AstExpression::Kind::SignedInt || ast.kind == AstExpression::Kind::UnsignedInt);
                Setter::SetData(obj, index, dataOffset, static_cast<int16_t>(ast.kind == AstExpression::Kind::SignedInt ? ast.signedInt : ast.unsignedInt));
                break;
            case Type::Data::Tag::Int32:
                HE_ASSERT(ast.kind == AstExpression::Kind::SignedInt || ast.kind == AstExpression::Kind::UnsignedInt);
                Setter::SetData(obj, index, dataOffset, static_cast<int32_t>(ast.kind == AstExpression::Kind::SignedInt ? ast.signedInt : ast.unsignedInt));
                break;
            case Type::Data::Tag::Int64:
                HE_ASSERT(ast.kind == AstExpression::Kind::SignedInt || ast.kind == AstExpression::Kind::UnsignedInt);
                Setter::SetData(obj, index, dataOffset, static_cast<int64_t>(ast.kind == AstExpression::Kind::SignedInt ? ast.signedInt : ast.unsignedInt));
                break;
            case Type::Data::Tag::Uint8:
                HE_ASSERT(ast.kind == AstExpression::Kind::SignedInt || ast.kind == AstExpression::Kind::UnsignedInt);
                Setter::SetData(obj, index, dataOffset, static_cast<uint8_t>(ast.kind == AstExpression::Kind::SignedInt ? ast.signedInt : ast.unsignedInt));
                break;
            case Type::Data::Tag::Uint16:
                HE_ASSERT(ast.kind == AstExpression::Kind::SignedInt || ast.kind == AstExpression::Kind::UnsignedInt);
                Setter::SetData(obj, index, dataOffset, static_cast<uint16_t>(ast.kind == AstExpression::Kind::SignedInt ? ast.signedInt : ast.unsignedInt));
                break;
            case Type::Data::Tag::Uint32:
                HE_ASSERT(ast.kind == AstExpression::Kind::SignedInt || ast.kind == AstExpression::Kind::UnsignedInt);
                Setter::SetData(obj, index, dataOffset, static_cast<uint32_t>(ast.kind == AstExpression::Kind::SignedInt ? ast.signedInt : ast.unsignedInt));
                break;
            case Type::Data::Tag::Uint64:
                HE_ASSERT(ast.kind == AstExpression::Kind::SignedInt || ast.kind == AstExpression::Kind::UnsignedInt);
                Setter::SetData(obj, index, dataOffset, static_cast<uint64_t>(ast.kind == AstExpression::Kind::SignedInt ? ast.signedInt : ast.unsignedInt));
                break;
            case Type::Data::Tag::Float32:
                HE_ASSERT(ast.kind == AstExpression::Kind::Float);
                Setter::SetData(obj, index, dataOffset, static_cast<float>(ast.floatingPoint));
                break;
            case Type::Data::Tag::Float64:
                HE_ASSERT(ast.kind == AstExpression::Kind::Float);
                Setter::SetData(obj, index, dataOffset, static_cast<double>(ast.floatingPoint));
                break;
            case Type::Data::Tag::Blob:
            {
                Vector<uint8_t> bytes(Allocator::GetTemp());
                if (!DecodeBlob(ast, bytes))
                    return;

                List<uint8_t>::Builder blob = m_builder.AddList<uint8_t>(bytes.Size());
                MemCopy(blob.Data(), bytes.Data(), bytes.Size());
                Setter::SetPointer(obj, index, blob);
                break;
            }
            case Type::Data::Tag::String:
            {
                he::String str(Allocator::GetTemp());
                if (!m_context->DecodeString(ast, str))
                {
                    m_valid = false;
                    return;
                }
                String::Reader strReader = m_builder.AddString(str);
                Setter::SetPointer(obj, index, strReader);
                break;
            }
            case Type::Data::Tag::List:
            {
                Type::Reader elementType = typeData.List().ElementType();
                ElementSize elementSize = GetTypeElementSize(elementType);
                ListBuilder list = m_builder.AddList(elementSize, ast.list.Size());

                uint16_t i = 0;
                for (const AstExpression& item : ast.list)
                {
                    SetValue(item, elementType, scope, list, i++, 0);
                }
                break;
            }
            case Type::Data::Tag::Enum:
            {
                const AstNode* valueNode = m_context->FindNode(ast, scope);
                HE_ASSERT(valueNode);
                HE_ASSERT(valueNode->kind == AstNode::Kind::Enumerator);
                Setter::SetData(obj, index, dataOffset, static_cast<uint16_t>(valueNode->id));
                break;
            }
            case Type::Data::Tag::Struct:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::Tuple);
                const Declaration::Reader typeDecl = m_context->GetDecl(typeData.Struct().Id());
                HE_ASSERT(typeDecl.IsValid() && typeDecl.Data().IsStruct());
                const Declaration::Data::Struct::Reader typeStructDecl = typeDecl.Data().Struct();
                StructBuilder st = Setter::SetupStruct(m_builder, typeStructDecl, obj, index);
                SetStructValues(typeStructDecl, ast.tuple, st, scope);
                break;
            }
            case Type::Data::Tag::Void:
                HE_ASSERT(false, "Values for lists of void is not supported. Verifier should've caught this.");
                break;
            case Type::Data::Tag::Array:
                HE_ASSERT(false, "Values for lists of arrays is not supported. Verifier should've caught this.");
                break;
            case Type::Data::Tag::Interface:
            case Type::Data::Tag::AnyPointer:
                HE_ASSERT(false, "An element type of {} cannot be assigned values. Verifier should've caught this.", typeData.Tag());
                break;
        }
    }

    List<Attribute>::Builder Compiler::CreateAttributes(const AstList<AstAttribute>& ast, const AstNode& scope)
    {
        if (ast.IsEmpty())
            return {};

        uint16_t i = 0;
        List<Attribute>::Builder list = m_builder.AddList<Attribute>(ast.Size());
        for (const AstAttribute& astAttr : ast)
        {
            const AstNode* attrDeclNode = m_context->FindNode(astAttr.name, scope);
            HE_ASSERT(attrDeclNode && attrDeclNode->kind != AstNode::Kind::Attribute);

            Type::Builder attrType = CreateType(attrDeclNode->attribute.type, *attrDeclNode->parent);
            Value::Builder attrValue = CreateValue(attrType, astAttr.value, scope);

            Attribute::Builder attr = list[i++];
            attr.SetId(attrDeclNode->id);
            attr.SetValue(attrValue);
        }

        return list;
    }

    List<String>::Builder Compiler::CreateTypeParams(const AstList<AstTypeParam>& ast)
    {
        if (ast.IsEmpty())
            return {};

        uint16_t i = 0;
        List<String>::Builder list = m_builder.AddList<String>(ast.Size());
        for (const AstTypeParam& astTypeParam : ast)
        {
            list.Set(i++, m_builder.AddString(astTypeParam.name));
        }

        return list;
    }

    bool Compiler::DecodeBlob(const AstExpression& ast, he::Vector<uint8_t>& out)
    {
        HE_ASSERT(ast.kind == AstExpression::Kind::Blob);

        const char* s = ast.blob.Begin();
        const char* end = ast.blob.End() - 1; // -1 so we point at end quote

        // Lexer should be validating this is true so we assert here to document our assumptions.
        HE_ASSERT(s[0] == '0' && s[1] == 'x' && s[2] == '"' && *end == '"');
        s += 3; // skip 0x"

        // Reserve the maximum possible byte size. Because we allow spaces this may over-allocate
        // but likely not by much, and that's better than multiple reallocs in the loop.
        out.Clear();
        out.Reserve(static_cast<uint32_t>(end - s) / 2);

        char first = 0;
        while (s < end)
        {
            const char c = *s++;

            if (c == ' ')
                continue;

            HE_ASSERT(IsHex(c));

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
            m_context->AddError(ast.location, "Invalid blob byte string, there are trailing nibbles");
            m_valid = false;
            return {};
        }

        return true;
    }

    uint16_t Compiler::GetArraySize(const AstExpression& ast, const AstNode& scope) const
    {
        switch (ast.kind)
        {
            case AstExpression::Kind::UnsignedInt:
                return static_cast<uint16_t>(ast.unsignedInt);

            case AstExpression::Kind::SignedInt:
                return static_cast<uint16_t>(ast.signedInt);

            case AstExpression::Kind::QualifiedName:
            {
                const AstNode* constant = m_context->FindNode(ast, scope);
                HE_ASSERT(constant && constant->kind == AstNode::Kind::Constant);
                return GetArraySize(constant->constant.value, *constant->parent);
            }

            default:
                HE_ASSERT(false, "Encountered invalid array size value type. This should've been verified already.");
                return 0;
        }
    }
}
