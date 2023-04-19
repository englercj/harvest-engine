// Copyright Chad Engler

#include "compiler.h"

#include "compile_context.h"
#include "keywords.h"
#include "struct_layout.h"

#include "he/core/ascii.h"
#include "he/core/enum_ops.h"
#include "he/core/file.h"
#include "he/core/limits.h"
#include "he/core/log.h"
#include "he/core/string_fmt.h"
#include "he/core/type_traits.h"
#include "he/core/vector.h"

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

        // Struct values are delayed until after the rest of the schema is created. This is so that
        // we can look up the struct declaration in the schema and use it to create the value here.
        // Lists are similarly delayed because they may contain structures.
        // All other values are created as they are encountered in the schema.
        for (const PendingValue& pending : m_pendingValues)
        {
            const Type::Data::Builder typeData = pending.type.GetData();

            if (pending.ast->kind == AstExpression::Kind::Tuple)
            {
                // Shouldn't be possible to not find this because if the ID is set on typeData,
                // then the verifier must have found a valid node.
                const AstNode* structDeclNode = m_context->FindNodeById(typeData.GetStruct().GetId());
                HE_ASSERT(structDeclNode && structDeclNode->kind == AstNode::Kind::Struct);
                HE_UNUSED(structDeclNode);

                const Type::Data::Struct::Builder structType = typeData.GetStruct();
                StructBuilder st = CreateStructValue(structType, *pending.ast, *pending.scope);
                pending.value.GetData().InitStruct().Set(st);
            }
            else
            {
                HE_ASSERT(pending.ast->kind == AstExpression::Kind::Sequence);

                const Type::Builder elementType = typeData.IsArray() ? typeData.GetArray().GetElementType() : typeData.GetList().GetElementType();
                ListBuilder list = CreateListValue(elementType, *pending.ast, *pending.scope);
                pending.value.GetData().InitList().Set(list);
            }
        }

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

        TrackDecl(node.location, decl);
    }

    void Compiler::CompileAttribute(const AstNode& node, Declaration::Builder decl)
    {
        Declaration::Data::Attribute::Builder data = decl.GetData().InitAttribute();
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
        Declaration::Data::Constant::Builder data = decl.GetData().InitConstant();
        data.SetType(CreateType(node.constant.type, *node.parent));
        data.SetValue(CreateValue(data.GetType(), node.constant.value, *node.parent));
    }

    void Compiler::CompileEnum(const AstNode& node, Declaration::Builder decl)
    {
        Declaration::Data::Enum::Builder data = decl.GetData().InitEnum();
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
        Declaration::Data::File::Builder fileDecl = decl.GetData().InitFile();

        he::String buf;
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
        Declaration::Data::Interface::Builder data = decl.GetData().InitInterface();

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
        Declaration::Data::Struct::Builder data = decl.GetData().InitStruct();
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
                    he::String name;
                    name = child.name;
                    name[0] = ToUpper(name[0]);
                    groupStruct.InitName(name);
                    groupStruct.SetId(MakeTypeId(name, decl.GetId()));
                    groupStruct.SetParentId(decl.GetId());
                    CompileStruct(child, groupStruct);
                    TrackDecl(child.location, groupStruct);

                    Field::Meta::Group::Builder group = field.GetMeta().InitGroup();
                    group.SetTypeId(groupStruct.GetId());

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
                    he::String name;
                    name = child.name;
                    name[0] = ToUpper(name[0]);
                    unionStruct.InitName(name);
                    unionStruct.SetId(MakeTypeId(name, decl.GetId()));
                    unionStruct.SetParentId(decl.GetId());
                    CompileStruct(child, unionStruct);
                    TrackDecl(child.location, unionStruct);

                    Field::Meta::Union::Builder unionBuilder = field.GetMeta().InitUnion();
                    unionBuilder.SetTypeId(unionStruct.GetId());

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

        if (!data.GetIsGroup() && !data.GetIsUnion())
        {
            StructLayout layout(decl);
            layout.CalculateLayout();
        }
    }

    void Compiler::CompileField(const AstNode& node, Field::Builder field, uint16_t index)
    {
        const List<Attribute>::Builder attributes = CreateAttributes(node.attributes, node);
        const Type::Builder type = CreateType(node.field.type, *node.parent);
        const Value::Builder value = CreateValue(type, node.field.defaultValue, node);

        HE_ASSERT(node.kind == AstNode::Kind::Field);
        field.InitName(node.name);
        field.SetDeclOrder(index);
        field.SetUnionTag(0); // Set during struct layout
        field.SetAttributes(attributes);

        Field::Meta::Normal::Builder normal = field.GetMeta().InitNormal();
        normal.SetType(type);
        normal.SetOrdinal(static_cast<uint16_t>(node.id));
        normal.SetIndex(0); // Set during struct layout
        normal.SetDefaultValue(value);
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
                he::String name;
                name = child.name;
                name += suffix;
                paramStruct.InitName(name);
                paramStruct.SetId(MakeTypeId(name, node.id));
                paramStruct.SetParentId(node.id);
                paramStruct.SetTypeParams(CreateTypeParams(child.typeParams));

                Declaration::Data::Struct::Builder paramData = paramStruct.GetData().InitStruct();
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
                return paramStruct.GetId();
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
                const AstNode* v = m_context->FindNodeByName(params.type, node);
                HE_ASSERT(v && v->kind == AstNode::Kind::Struct);
                return v->id;
            }
            default:
                HE_ASSERT(false, HE_MSG("Invalid method params type. Verify should've caught this."));
                return 0;
        }
    }

    Type::Builder Compiler::CreateType(const AstExpression& ast, const AstNode& scope)
    {
        const TypeValue& info = m_context->GetType({ &ast, &scope });
        Type::Builder type = m_builder.AddStruct<Type>();
        Type::Data::Builder data = type.GetData();

        if (info.type)
        {
            HE_ASSERT(ast.kind == AstExpression::Kind::QualifiedName);
            HE_ASSERT(info.tag == Type::Data::UnionTag::AnyPointer);
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
                    HE_ASSERT(false, HE_MSG("Invalid type node. Verify should've checked this."));
                    break;
            }
            return type;
        }

        switch (info.tag)
        {
            case Type::Data::UnionTag::Array:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::Array);
                Type::Builder elementType = CreateType(*ast.array.elementType, scope);
                Type::Data::Array::Builder arr = data.InitArray();
                arr.SetElementType(elementType);
                arr.SetSize(GetArraySize(*ast.array.size, scope));
                break;
            }
            case Type::Data::UnionTag::List:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::List);
                Type::Builder elementType = CreateType(*ast.list.elementType, scope);
                Type::Data::List::Builder list = data.InitList();
                list.SetElementType(elementType);
                break;
            }
            case Type::Data::UnionTag::AnyPointer:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::QualifiedName);
                HE_ASSERT(ast.qualified.names.Size() == 1);
                HE_ASSERT(ast.qualified.names.Front()->kind == AstExpression::Kind::Identifier);
                HE_ASSERT(ast.qualified.names.Front()->identifier == KW_AnyPointer);
                data.SetAnyPointer();
                break;
            }
            case Type::Data::UnionTag::AnyStruct:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::QualifiedName);
                HE_ASSERT(ast.qualified.names.Size() == 1);
                HE_ASSERT(ast.qualified.names.Front()->kind == AstExpression::Kind::Identifier);
                HE_ASSERT(ast.qualified.names.Front()->identifier == KW_AnyStruct);
                data.SetAnyStruct();
                break;
            }
            case Type::Data::UnionTag::AnyList:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::QualifiedName);
                HE_ASSERT(ast.qualified.names.Size() == 1);
                HE_ASSERT(ast.qualified.names.Front()->kind == AstExpression::Kind::Identifier);
                HE_ASSERT(ast.qualified.names.Front()->identifier == KW_AnyList);
                data.SetAnyList();
                break;
            }
            case Type::Data::UnionTag::Parameter:
            {
                Type::Data::Parameter::Builder param = data.InitParameter();

                // check if this is a generic parameter, if not we assume it is the keyword
                const AstTypeParamRef& ref = info.typeParamRef;
                HE_ASSERT(ref.scope);
                param.SetScopeId(ref.scope->id);
                param.SetIndex(ref.index);
                break;
            }
            default:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::QualifiedName);
                HE_ASSERT(ast.qualified.names.Size() == 1 && ast.qualified.names.Front()->kind == AstExpression::Kind::Identifier);
                HE_ASSERT(info.tag != Type::Data::UnionTag::Enum && info.tag != Type::Data::UnionTag::Struct && info.tag != Type::Data::UnionTag::Interface);
                data.SetUnionTag(info.tag);
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
        uint16_t scopeIndex = 0;
        Brand::Builder brand = m_builder.AddStruct<Brand>();
        List<Brand::Scope>::Builder brandScopes = brand.InitScopes(scopeCount);

        const AstNode* scope = &scope_;
        AstListIterator<AstExpression> startIt = name.qualified.names.begin();

        for (AstListIterator<AstExpression> it = name.qualified.names.begin(); it != name.qualified.names.end(); ++it)
        {
            HE_ASSERT(it->kind == AstExpression::Kind::Identifier || it->kind == AstExpression::Kind::Generic);

            if (it->kind == AstExpression::Kind::Generic)
            {
                scope = m_context->FindNodeByName(startIt, (it + 1), *scope);
                HE_ASSERT(scope);

                Brand::Scope::Builder brandScope = brandScopes[scopeIndex++];
                brandScope.SetScopeId(scope->id);

                uint16_t paramIndex = 0;
                List<Type>::Builder brandParams = brandScope.InitParams(it->generic.params.Size());
                for (const AstExpression& astType : it->generic.params)
                {
                    Type::Builder type = CreateType(astType, scope_); // use original scope
                    brandParams.Set(paramIndex++, type);
                }

                startIt = it + 1;
            }
        }

        return brand;
    }

    template <typename T> requires(IsSame<T, uint64_t> || IsSame<T, int64_t>)
    void Compiler::SetInt(const AstFileLocation& location, T value, Type::Data::Builder type, Value::Data::Builder data)
    {
        if (type.IsInt8())
        {
            data.SetInt8(ReadIntValue<int8_t>(location, value));
        }
        else if (type.IsInt16())
        {
            data.SetInt16(ReadIntValue<int16_t>(location, value));
        }
        else if (type.IsInt32())
        {
            data.SetInt32(ReadIntValue<int32_t>(location, value));
        }
        else if (type.IsInt64())
        {
            data.SetInt64(ReadIntValue<int64_t>(location, value));
        }
        else if (type.IsUint8())
        {
            data.SetUint8(ReadIntValue<uint8_t>(location, value));
        }
        else if (type.IsUint16())
        {
            data.SetUint16(ReadIntValue<uint16_t>(location, value));
        }
        else if (type.IsUint32())
        {
            data.SetUint32(ReadIntValue<uint32_t>(location, value));
        }
        else if (type.IsUint64())
        {
            data.SetUint64(ReadIntValue<uint64_t>(location, value));
        }
        else
        {
            m_context->AddError(location, "Expected {} value, but encountered integer expression", type.GetUnionTag());
            m_valid = false;
        }
    }

    Value::Builder Compiler::CreateValue(Type::Builder type, const AstExpression& ast, const AstNode& scope)
    {
        if (ast.kind == AstExpression::Kind::Unknown)
            return {};

        Value::Builder value = m_builder.AddStruct<Value>();

        FillValue(value, type, ast, scope);

        return value;
    }

    List<Attribute>::Builder Compiler::CreateAttributes(const AstList<AstAttribute>& ast, const AstNode& scope)
    {
        if (ast.IsEmpty())
            return {};

        uint16_t i = 0;
        List<Attribute>::Builder list = m_builder.AddList<Attribute>(ast.Size());
        for (const AstAttribute& astAttr : ast)
        {
            const AstNode* attrDeclNode = m_context->FindNodeByName(astAttr.name, scope);
            HE_ASSERT(attrDeclNode && attrDeclNode->kind == AstNode::Kind::Attribute);

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
            m_context->AddError(ast.location, "Invalid blob byte string, there is a trailing nibble");
            m_valid = false;
            return false;
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
                const AstNode* constant = m_context->FindNodeByName(ast, scope);
                HE_ASSERT(constant && constant->kind == AstNode::Kind::Constant);
                return GetArraySize(constant->constant.value, *constant->parent);
            }

            default:
                HE_ASSERT(false, HE_MSG("Encountered invalid array size value type. This should've been verified already."));
                return 0;
        }
    }

    ListBuilder Compiler::CreateListValue(const Type::Builder elementType, const AstExpression& ast, const AstNode& scope)
    {
        HE_ASSERT(ast.kind == AstExpression::Kind::Sequence);

        const Type::Data::UnionTag elementTag = elementType.GetData().GetUnionTag();

        ElementSize elementSize = ElementSize::Void;
        switch (elementTag)
        {
            case Type::Data::UnionTag::Bool: elementSize = ElementSize::Bit; break;
            case Type::Data::UnionTag::Int8: elementSize = ElementSize::Byte; break;
            case Type::Data::UnionTag::Int16: elementSize = ElementSize::TwoBytes; break;
            case Type::Data::UnionTag::Int32: elementSize = ElementSize::FourBytes; break;
            case Type::Data::UnionTag::Int64: elementSize = ElementSize::EightBytes; break;
            case Type::Data::UnionTag::Uint8: elementSize = ElementSize::Byte; break;
            case Type::Data::UnionTag::Uint16: elementSize = ElementSize::TwoBytes; break;
            case Type::Data::UnionTag::Uint32: elementSize = ElementSize::FourBytes; break;
            case Type::Data::UnionTag::Uint64: elementSize = ElementSize::EightBytes; break;
            case Type::Data::UnionTag::Float32: elementSize = ElementSize::FourBytes; break;
            case Type::Data::UnionTag::Float64: elementSize = ElementSize::EightBytes; break;
            case Type::Data::UnionTag::Blob: elementSize = ElementSize::Pointer; break;
            case Type::Data::UnionTag::String: elementSize = ElementSize::Pointer; break;
            case Type::Data::UnionTag::List: elementSize = ElementSize::Pointer; break;
            case Type::Data::UnionTag::Enum: elementSize = ElementSize::TwoBytes; break;
            case Type::Data::UnionTag::Struct: elementSize = ElementSize::Composite; break;
            case Type::Data::UnionTag::Array:
                HE_ASSERT(false, HE_MSG("Arrays cannot be nested in lists or other arrays"));
                break;
            case Type::Data::UnionTag::Interface:
            case Type::Data::UnionTag::AnyPointer:
            case Type::Data::UnionTag::AnyStruct:
            case Type::Data::UnionTag::AnyList:
            case Type::Data::UnionTag::Parameter:
            case Type::Data::UnionTag::Void:
                HE_ASSERT(false, HE_MSG("{:s} cannot have a default value", elementTag));
                break;
        }

        const uint32_t size = ast.sequence.Size();
        ListBuilder list = m_builder.AddList(elementSize, size);

        uint16_t i = 0;
        for (const AstExpression& value : ast.sequence)
        {
            switch (elementTag)
            {
                case Type::Data::UnionTag::Bool: list.SetDataElement<bool>(i, ReadBoolValue(value, scope)); break;
                case Type::Data::UnionTag::Int8: list.SetDataElement<int8_t>(i, ReadIntValue<int8_t>(value, scope)); break;
                case Type::Data::UnionTag::Int16: list.SetDataElement<int16_t>(i, ReadIntValue<int16_t>(value, scope)); break;
                case Type::Data::UnionTag::Int32: list.SetDataElement<int32_t>(i, ReadIntValue<int32_t>(value, scope)); break;
                case Type::Data::UnionTag::Int64: list.SetDataElement<int64_t>(i, ReadIntValue<int64_t>(value, scope)); break;
                case Type::Data::UnionTag::Uint8: list.SetDataElement<uint8_t>(i, ReadIntValue<uint8_t>(value, scope)); break;
                case Type::Data::UnionTag::Uint16: list.SetDataElement<uint16_t>(i, ReadIntValue<uint16_t>(value, scope)); break;
                case Type::Data::UnionTag::Uint32: list.SetDataElement<uint32_t>(i, ReadIntValue<uint32_t>(value, scope)); break;
                case Type::Data::UnionTag::Uint64: list.SetDataElement<uint64_t>(i, ReadIntValue<uint64_t>(value, scope)); break;
                case Type::Data::UnionTag::Float32: list.SetDataElement<float>(i, ReadFloatValue<float>(value, scope)); break;
                case Type::Data::UnionTag::Float64: list.SetDataElement<double>(i, ReadFloatValue<double>(value, scope)); break;
                case Type::Data::UnionTag::Enum: list.SetDataElement<uint16_t>(i, ReadEnumValue(value, scope)); break;
                case Type::Data::UnionTag::Blob:
                {
                    Vector<uint8_t> bytes;
                    if (DecodeBlob(value, bytes))
                    {
                        List<uint8_t>::Builder blob = m_builder.AddBlob(bytes);
                        list.SetPointerElement(i, blob);
                    }
                    break;
                }
                case Type::Data::UnionTag::String:
                {
                    he::String str;
                    if (m_context->DecodeString(ast, str))
                    {
                        String::Builder strBuilder = m_builder.AddString(str);
                        list.SetPointerElement(i, strBuilder);
                    }
                    else
                    {
                        m_valid = false;
                    }
                    break;
                }
                case Type::Data::UnionTag::List:
                {
                    const Type::Builder subElementType = elementType.GetData().GetList().GetElementType();
                    ListBuilder v = CreateListValue(subElementType, value, scope);
                    list.SetPointerElement(i, v);
                    break;
                }
                case Type::Data::UnionTag::Struct:
                {
                    const Declaration::Builder decl = m_context->GetDecl(elementType.GetData().GetStruct().GetId());
                    const Declaration::Data::Struct::Builder structDecl = decl.GetData().GetStruct();
                    StructBuilder v = list.GetCompositeElement(i);
                    FillStructValue(v, structDecl, value, scope);
                    break;
                }
                case Type::Data::UnionTag::Array:
                    HE_ASSERT(false, HE_MSG("Arrays cannot be nested in lists or other arrays"));
                    break;
                case Type::Data::UnionTag::Interface:
                case Type::Data::UnionTag::AnyPointer:
                case Type::Data::UnionTag::AnyStruct:
                case Type::Data::UnionTag::AnyList:
                case Type::Data::UnionTag::Parameter:
                case Type::Data::UnionTag::Void:
                    HE_ASSERT(false, HE_MSG("{} types cannot have default values", elementTag));
                    break;
            }

            ++i;
        }

        return list;
    }

    StructBuilder Compiler::CreateStructValue(const Type::Data::Struct::Builder structType, const AstExpression& ast, const AstNode& scope)
    {
        const Declaration::Builder decl = m_context->GetDecl(structType.GetId());
        const Declaration::Data::Struct::Builder structDecl = decl.GetData().GetStruct();

        StructBuilder st = m_builder.AddStruct(structDecl.GetDataFieldCount(), structDecl.GetDataWordSize(), structDecl.GetPointerCount());
        FillStructValue(st, structDecl, ast, scope);
        return st;
    }

    void Compiler::FillValue(Value::Builder value, Type::Builder type, const AstExpression& ast, const AstNode& scope)
    {
        Value::Data::Builder data = value.GetData();
        Type::Data::Builder typeData = type.GetData();

        switch (ast.kind)
        {
            case AstExpression::Kind::Blob:
            {
                HE_ASSERT(typeData.IsBlob());

                Vector<uint8_t> bytes;
                if (DecodeBlob(ast, bytes))
                {
                    List<uint8_t>::Builder list = data.InitBlob(bytes.Size());
                    MemCopy(list.Data(), bytes.Data(), bytes.Size());
                }

                break;
            }
            case AstExpression::Kind::Float:
            {
                HE_ASSERT(typeData.IsFloat32() || typeData.IsFloat64());

                if (typeData.IsFloat32())
                    data.SetFloat32(ReadFloatValue<float>(ast, scope));
                else
                    data.SetFloat64(ReadFloatValue<double>(ast, scope));

                break;
            }
            case AstExpression::Kind::Sequence:
            {
                HE_ASSERT(typeData.IsArray() || typeData.IsList());

                PendingValue& v = m_pendingValues.EmplaceBack();
                v.ast = &ast;
                v.scope = &scope;
                v.type = type;
                v.value = value;
                break;
            }
            case AstExpression::Kind::QualifiedName:
            {
                //HE_ASSERT(typeData.IsEnum() || typeData.IsBool());

                if (typeData.IsEnum())
                    data.SetEnum(ReadEnumValue(ast, scope));
                else if (typeData.IsBool())
                    data.SetBool(ReadBoolValue(ast, scope));
                else
                {
                    const AstNode* constant = m_context->FindNodeByName(ast, scope);
                    HE_ASSERT(constant && constant->kind == AstNode::Kind::Constant);
                    return FillValue(value, type, constant->constant.value, *constant->parent);
                }

                break;
            }
            case AstExpression::Kind::SignedInt:
            {
                HE_ASSERT(typeData.IsInt8() || typeData.IsInt16() || typeData.IsInt32() || typeData.IsInt64()
                    || typeData.IsUint8() || typeData.IsUint16() || typeData.IsUint32() || typeData.IsUint64());

                SetInt(ast.location, ast.signedInt, typeData, data);
                break;
            }
            case AstExpression::Kind::UnsignedInt:
            {
                HE_ASSERT(typeData.IsInt8() || typeData.IsInt16() || typeData.IsInt32() || typeData.IsInt64()
                    || typeData.IsUint8() || typeData.IsUint16() || typeData.IsUint32() || typeData.IsUint64());

                SetInt(ast.location, ast.unsignedInt, typeData, data);
                break;
            }
            case AstExpression::Kind::String:
            {
                HE_ASSERT(typeData.IsString());

                he::String str;
                if (m_context->DecodeString(ast, str))
                {
                    data.InitString(str);
                }
                break;
            }
            case AstExpression::Kind::Tuple:
            {
                HE_ASSERT(typeData.IsStruct());

                PendingValue& v = m_pendingValues.EmplaceBack();
                v.ast = &ast;
                v.scope = &scope;
                v.type = type;
                v.value = value;
                break;
            }

            // These are used in types, not in values. Any of this is a syntax error.
            case AstExpression::Kind::Array:
            case AstExpression::Kind::List:
            case AstExpression::Kind::Generic:
            case AstExpression::Kind::Identifier:
            case AstExpression::Kind::Namespace:
            case AstExpression::Kind::Unknown:
                HE_ASSERT(false, HE_MSG("Invalid value type. This is a verifier bug."));
                m_valid = false;
                break;
        }
    }

    void Compiler::FillStructValue(StructBuilder dst, const Declaration::Data::Struct::Builder structDecl, const AstExpression& ast, const AstNode& scope)
    {
        HE_ASSERT(ast.kind == AstExpression::Kind::Tuple);

        for (const AstTupleParam& item : ast.tuple)
        {
            for (const Field::Builder& field : structDecl.GetFields())
            {
                if (item.name != field.GetName())
                    continue;

                const Field::Meta::Builder fieldMeta = field.GetMeta();

                if (fieldMeta.IsGroup())
                {
                    HE_ASSERT(item.value.kind == AstExpression::Kind::Tuple);
                    const Declaration::Builder groupDecl = m_context->GetDecl(fieldMeta.GetGroup().GetTypeId());
                    const Declaration::Data::Struct::Builder groupStructDecl = groupDecl.GetData().GetStruct();
                    FillStructValue(dst, groupStructDecl, item.value, scope);
                }
                else if (fieldMeta.IsUnion())
                {
                    HE_ASSERT(item.value.kind == AstExpression::Kind::Tuple);
                    HE_ASSERT(item.value.tuple.Size() == 1);
                    const Declaration::Builder unionDecl = m_context->GetDecl(fieldMeta.GetUnion().GetTypeId());
                    const Declaration::Data::Struct::Builder unionStructDecl = unionDecl.GetData().GetStruct();
                    FillUnionValue(dst, unionStructDecl, item.value, scope);
                }
                else
                {
                    const Field::Meta::Normal::Builder norm = fieldMeta.GetNormal();
                    FillStructField(dst, norm.GetType().GetData(), norm.GetIndex(), norm.GetDataOffset(), item.value, scope, true);
                }
            }
        }
    }

    void Compiler::FillUnionValue(StructBuilder dst, const Declaration::Data::Struct::Builder structDecl, const AstExpression& ast, const AstNode& scope)
    {
        HE_ASSERT(ast.kind == AstExpression::Kind::Tuple);
        HE_ASSERT(structDecl.GetIsUnion());

        const AstTupleParam* param = ast.tuple.Front();

        Field::Builder activeField;
        for (const Field::Builder& field : structDecl.GetFields())
        {
            if (param->name == field.GetName())
            {
                activeField = field;
                break;
            }
        }
        HE_ASSERT(activeField.IsValid());

        dst.SetDataField(structDecl.GetUnionTagOffset(), activeField.GetUnionTag());

        const Field::Meta::Builder meta = activeField.GetMeta();

        if (meta.IsGroup())
        {
            const Declaration::Builder groupDecl = m_context->GetDecl(meta.GetGroup().GetTypeId());
            const Declaration::Data::Struct::Builder groupStructDecl = groupDecl.GetData().GetStruct();
            FillStructValue(dst, groupStructDecl, param->value, scope);
        }
        else if (meta.IsUnion())
        {
            const Declaration::Builder unionDecl = m_context->GetDecl(meta.GetUnion().GetTypeId());
            const Declaration::Data::Struct::Builder unionStructDecl = unionDecl.GetData().GetStruct();
            FillStructValue(dst, unionStructDecl, param->value, scope);
            return;
        }
        else
        {
            const Field::Meta::Normal::Builder norm = meta.GetNormal();
            const Type::Data::Builder type = norm.GetType().GetData();
            const uint16_t index = norm.GetIndex();
            const uint32_t dataOffset = norm.GetDataOffset();
            FillStructField(dst, type, index, dataOffset, param->value, scope, false);
        }
    }

    void Compiler::FillStructField(StructBuilder dst, const Type::Data::Builder type, uint16_t index, uint32_t dataOffset, const AstExpression& ast, const AstNode& scope, bool markDataField)
    {
        if (markDataField && !IsPointer(type.GetUnionTag()))
        {
            dst.MarkDataField(index);
        }

        switch (type.GetUnionTag())
        {
            case Type::Data::UnionTag::Bool: dst.SetDataField<bool>(dataOffset, ReadBoolValue(ast, scope)); break;
            case Type::Data::UnionTag::Int8: dst.SetDataField<int8_t>(dataOffset, ReadIntValue<int8_t>(ast, scope)); break;
            case Type::Data::UnionTag::Int16: dst.SetDataField<int16_t>(dataOffset, ReadIntValue<int16_t>(ast, scope)); break;
            case Type::Data::UnionTag::Int32: dst.SetDataField<int32_t>(dataOffset, ReadIntValue<int32_t>(ast, scope)); break;
            case Type::Data::UnionTag::Int64: dst.SetDataField<int64_t>(dataOffset, ReadIntValue<int64_t>(ast, scope)); break;
            case Type::Data::UnionTag::Uint8: dst.SetDataField<uint8_t>(dataOffset, ReadIntValue<uint8_t>(ast, scope)); break;
            case Type::Data::UnionTag::Uint16: dst.SetDataField<uint16_t>(dataOffset, ReadIntValue<uint16_t>(ast, scope)); break;
            case Type::Data::UnionTag::Uint32: dst.SetDataField<uint32_t>(dataOffset, ReadIntValue<uint32_t>(ast, scope)); break;
            case Type::Data::UnionTag::Uint64: dst.SetDataField<uint64_t>(dataOffset, ReadIntValue<uint64_t>(ast, scope)); break;
            case Type::Data::UnionTag::Float32: dst.SetDataField<float>(dataOffset, ReadFloatValue<float>(ast, scope)); break;
            case Type::Data::UnionTag::Float64: dst.SetDataField<double>(dataOffset, ReadFloatValue<double>(ast, scope)); break;
            case Type::Data::UnionTag::Enum: dst.SetDataField<uint16_t>(dataOffset, ReadEnumValue(ast, scope)); break;
            case Type::Data::UnionTag::Blob:
            {
                Vector<uint8_t> bytes;
                if (DecodeBlob(ast, bytes))
                {
                    List<uint8_t>::Builder blob = m_builder.AddBlob(bytes);
                    dst.GetPointerField(index).Set(blob);
                }
                break;
            }
            case Type::Data::UnionTag::String:
            {
                he::String str;
                if (m_context->DecodeString(ast, str))
                {
                    String::Builder strBuilder = m_builder.AddString(str);
                    dst.GetPointerField(index).Set(strBuilder);
                }
                else
                {
                    m_valid = false;
                }
                break;
            }
            case Type::Data::UnionTag::Array:
            {
                HE_ASSERT(ast.kind == AstExpression::Kind::Sequence);

                const Type::Data::Array::Builder arrayType = type.GetArray();
                const Type::Builder elementType = arrayType.GetElementType();
                const uint16_t size = arrayType.GetSize();
                const bool elementIsPointer = IsPointer(elementType);

                uint16_t i = 0;
                for (const AstExpression& item : ast.sequence)
                {
                    if (!HE_VERIFY(i < size, HE_MSG("Sequence longer than array size. This is a verifier bug.")))
                    {
                        m_valid = false;
                        break;
                    }

                    if (elementIsPointer)
                        FillStructField(dst, elementType.GetData(), index + i, 0, item, scope, false);
                    else
                        FillStructField(dst, elementType.GetData(), index, dataOffset + i, item, scope, false);

                    ++i;
                }
                break;
            }
            case Type::Data::UnionTag::List:
            {
                const Type::Builder elementType = type.GetList().GetElementType();
                ListBuilder list = CreateListValue(elementType, ast, scope);
                dst.GetPointerField(index).Set(list);
                break;
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Builder structType = type.GetStruct();
                StructBuilder st = CreateStructValue(structType, ast, scope);
                dst.GetPointerField(index).Set(st);
                break;
            }
            case Type::Data::UnionTag::Interface:
            case Type::Data::UnionTag::AnyPointer:
            case Type::Data::UnionTag::AnyStruct:
            case Type::Data::UnionTag::AnyList:
            case Type::Data::UnionTag::Parameter:
            case Type::Data::UnionTag::Void:
                HE_ASSERT(false, HE_MSG("{:s} types cannot have default values", type.GetUnionTag()));
                break;
        }
    }

    bool Compiler::ReadBoolValue(const AstExpression& ast, const AstNode& scope) const
    {
        if (ast.qualified.names.Size() == 1 && ast.qualified.names.Front()->kind == AstExpression::Kind::Identifier)
        {
            const StringView identifier = ast.qualified.names.Front()->identifier;
            if (identifier == KW_True)
                return true;
            if (identifier == KW_False)
                return false;
        }

        const AstNode* valueNode = m_context->FindNodeByName(ast, scope);
        HE_ASSERT(valueNode && valueNode->kind == AstNode::Kind::Constant);
        return ReadBoolValue(valueNode->constant.value, *valueNode->parent);
    }

    uint16_t Compiler::ReadEnumValue(const AstExpression& ast, const AstNode& scope) const
    {
        const AstNode* valueNode = m_context->FindNodeByName(ast, scope);
        HE_ASSERT(valueNode);
        HE_ASSERT(valueNode->kind == AstNode::Kind::Enumerator || valueNode->kind == AstNode::Kind::Constant);

        if (valueNode->kind == AstNode::Kind::Constant)
        {
            return ReadEnumValue(valueNode->constant.value, *valueNode->parent);
        }

        return static_cast<uint16_t>(valueNode->id);
    }

    template <typename OutType, typename InType>
    OutType Compiler::ReadIntValue(const AstFileLocation& location, InType value)
    {
        OutType out = static_cast<OutType>(value);

        if (static_cast<InType>(out) != value)
        {
            m_context->AddError(location, "Integer value out of range for type.");
            m_valid = false;
        }

        return out;
    }

    template <typename T>
    T Compiler::ReadIntValue(const AstExpression& ast, const AstNode& scope)
    {
        HE_ASSERT(ast.kind == AstExpression::Kind::SignedInt || ast.kind == AstExpression::Kind::UnsignedInt || ast.kind == AstExpression::Kind::QualifiedName);

        if (ast.kind == AstExpression::Kind::QualifiedName)
        {
            const AstNode* valueNode = m_context->FindNodeByName(ast, scope);
            HE_ASSERT(valueNode && valueNode->kind == AstNode::Kind::Constant);
            return ReadIntValue<T>(valueNode->constant.value, *valueNode->parent);
        }

        T value = 0;

        if (ast.kind == AstExpression::Kind::SignedInt)
            value = ReadIntValue<T>(ast.location, ast.signedInt);
        else
            value = ReadIntValue<T>(ast.location, ast.unsignedInt);

        return value;
    }

    template <typename T>
    T Compiler::ReadFloatValue(const AstExpression& ast, const AstNode& scope)
    {
        HE_ASSERT(ast.kind == AstExpression::Kind::Float || ast.kind == AstExpression::Kind::QualifiedName);

        if (ast.kind == AstExpression::Kind::QualifiedName)
        {
            const AstNode* valueNode = m_context->FindNodeByName(ast, scope);
            HE_ASSERT(valueNode && valueNode->kind == AstNode::Kind::Constant);
            return ReadFloatValue<T>(valueNode->constant.value, *valueNode->parent);
        }

        if constexpr (IsSame<T, float>)
        {
            if (ast.floatingPoint < Limits<float>::Min || ast.floatingPoint > Limits<float>::Max)
            {
                m_context->AddError(ast.location, "Floating-pointer value out of range for type.");
                m_valid = false;
            }
        }

        return static_cast<T>(ast.floatingPoint);
    }

    void Compiler::TrackDecl(const AstFileLocation& location, Declaration::Builder decl)
    {
        if (!m_context->TrackDecl(decl))
        {
            const Declaration::Builder otherDecl = m_context->GetDecl(decl.GetId());
            m_context->AddError(location, "Duplicate ID detected, this declaration collides with '{}' defined elsewhere.", otherDecl.GetName().AsView());
            m_valid = false;
        }
    }
}
