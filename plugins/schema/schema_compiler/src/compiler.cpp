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
                HE_ASSERT(false, HE_MSG("Invalid method params type. Verify should've caught this."));
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
                    HE_ASSERT(false, HE_MSG("Invalid type node. Verify should've checked this."));
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
                HE_ASSERT(ast.kind == AstExpression::Kind::List);
                Type::Builder elementType = CreateType(*ast.list.elementType, scope);
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

    template <typename T> requires(std::is_same_v<T, uint64_t> || std::is_same_v<T, int64_t>)
    bool Compiler::SetInt(const AstFileLocation& location, T value, Type::Data::Reader type, Value::Data::Builder data)
    {
        if (type.IsInt8())
        {
            const bool fitsMin = std::is_signed_v<T> ? value >= std::numeric_limits<int8_t>::min() : true;
            if (fitsMin && value <= std::numeric_limits<int8_t>::max())
            {
                data.SetInt8(static_cast<int8_t>(value));
                return true;
            }

            m_context->AddError(location, "Integer value out of range for type 'int8'");
            return false;
        }

        if (type.IsInt16())
        {
            const bool fitsMin = std::is_signed_v<T> ? value >= std::numeric_limits<int16_t>::min() : true;
            if (fitsMin && value <= std::numeric_limits<int16_t>::max())
            {
                data.SetInt16(static_cast<int16_t>(value));
                return true;
            }

            m_context->AddError(location, "Integer value out of range for type 'int16'");
            return false;
        }

        if (type.IsInt32())
        {
            const bool fitsMin = std::is_signed_v<T> ? value >= std::numeric_limits<int32_t>::min() : true;
            if (fitsMin && value <= std::numeric_limits<int32_t>::max())
            {
                data.SetInt32(static_cast<int32_t>(value));
                return true;
            }

            m_context->AddError(location, "Integer value out of range for type 'int32'");
            return false;
        }

        if (type.IsInt64())
        {
            if (value <= static_cast<T>(std::numeric_limits<int64_t>::max()))
            {
                data.SetInt64(static_cast<int64_t>(value));
                return true;
            }

            m_context->AddError(location, "Integer value out of range for type 'int64'");
            return false;
        }

        if (type.IsUint8())
        {
            if (value == -1 || (value >= 0 && value <= std::numeric_limits<uint8_t>::max()))
            {
                data.SetUint8(static_cast<int8_t>(value));
                return true;
            }

            m_context->AddError(location, "Integer value out of range for type 'uint8'");
            return false;
        }

        if (type.IsUint16())
        {
            if (value == -1 || (value >= 0 && value <= std::numeric_limits<uint16_t>::max()))
            {
                data.SetUint16(static_cast<int16_t>(value));
                return true;
            }

            m_context->AddError(location, "Integer value out of range for type 'uint16'");
            return false;
        }

        if (type.IsUint32())
        {
            if (value == -1 || (value >= 0 && value <= std::numeric_limits<uint32_t>::max()))
            {
                data.SetUint32(static_cast<uint32_t>(value));
                return true;
            }

            m_context->AddError(location, "Integer value out of range for type 'uint32'");
            return false;
        }

        if (type.IsUint64())
        {
            if (value == -1 || value >= 0)
            {
                data.SetUint64(static_cast<uint64_t>(value));
                return true;
            }

            m_context->AddError(location, "Integer value out of range for type 'uint64'");
            return false;
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
            case AstExpression::Kind::Sequence:
            {
                Type::Reader elementType = typeData.IsArray() ? typeData.Array().ElementType() : typeData.List().ElementType();
                List<Value>::Builder list = data.InitList(ast.sequence.Size());

                uint16_t i = 0;
                for (const AstExpression& item : ast.sequence)
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

                // Shouldn't be possible to not find this because if the ID is set on typeData,
                // then the verifier must have found a valid node.
                const AstNode* structDeclNode = m_context->FindNode(typeData.Struct().Id());
                HE_ASSERT(structDeclNode && structDeclNode->kind == AstNode::Kind::Struct);

                List<Value::TupleValue>::Builder list = data.InitTuple(ast.tuple.Size());
                uint16_t i = 0;
                for (const AstTupleParam& item : ast.tuple)
                {
                    for (const AstNode& child : structDeclNode->children)
                    {
                        if (child.kind == AstNode::Kind::Field && child.name == item.name)
                        {
                            Type::Reader fieldType = CreateType(child.field.type, *structDeclNode);
                            Value::Builder itemValue = CreateValue(fieldType, item.value, scope);

                            Value::TupleValue::Builder v = m_builder.AddStruct<Value::TupleValue>();
                            v.InitName(item.name);
                            v.SetValue(itemValue);
                            list.Set(i++, v);
                            break;
                        }
                    }
                }
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
            case AstExpression::Kind::List:
            case AstExpression::Kind::Generic:
            case AstExpression::Kind::Identifier:
            case AstExpression::Kind::Namespace:
            case AstExpression::Kind::Unknown:
                HE_ASSERT(false, HE_MSG("Invalid value type. Verify should've caught this."));
                return {};
        }

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
            const AstNode* attrDeclNode = m_context->FindNode(astAttr.name, scope);
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
                HE_ASSERT(false, HE_MSG("Encountered invalid array size value type. This should've been verified already."));
                return 0;
        }
    }
}
