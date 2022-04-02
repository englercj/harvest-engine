// Copyright Chad Engler

#include "verifier.h"

#include "compile_context.h"
#include "keywords.h"

#include "he/core/enum_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/random.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view_fmt.h"
#include "he/schema/schema.h"

#include <set>
#include <unordered_set>

namespace he::schema
{
    struct BuiltinType { const StringView name; const Type::Data::Tag kind; };
    constexpr BuiltinType BuiltinTypes[] =
    {
        { KW_Void, Type::Data::Tag::Void },
        { KW_Bool, Type::Data::Tag::Bool },
        { KW_Int8, Type::Data::Tag::Int8 },
        { KW_Int16, Type::Data::Tag::Int16 },
        { KW_Int32, Type::Data::Tag::Int32 },
        { KW_Int64, Type::Data::Tag::Int64 },
        { KW_Uint8, Type::Data::Tag::Uint8 },
        { KW_Uint16, Type::Data::Tag::Uint16 },
        { KW_Uint32, Type::Data::Tag::Uint32 },
        { KW_Uint64, Type::Data::Tag::Uint64 },
        { KW_Float32, Type::Data::Tag::Float32 },
        { KW_Float64, Type::Data::Tag::Float64 },
        { KW_Blob, Type::Data::Tag::Blob },
        { KW_String, Type::Data::Tag::String },
        { KW_AnyPointer, Type::Data::Tag::AnyPointer },
    };

    bool Verifier::Verify(const AstFile& ast, CompileContext& ctx)
    {
        if (m_builtinTypeMap.empty())
        {
            for (const BuiltinType& t : BuiltinTypes)
            {
                m_builtinTypeMap[t.name] = t.kind;
            }
        }

        m_context = &ctx;
        if (!VerifyNode(ast.root))
            return false;

        return true;
    }

    bool Verifier::VerifyNode(const AstNode& node)
    {
        if (node.kind != AstNode::Kind::File && node.parent == nullptr)
        {
            m_context->AddError(node.location, "AstNode is missing a parent reference. This is a bug in the parser.");
            return false;
        }

        if (!VerifyAttributes(node))
            return false;

        if (node.children.Size() > std::numeric_limits<uint16_t>::max())
        {
            m_context->AddError(node.location, "{} has too many members. Max is UINT16_MAX ({})", node.kind, std::numeric_limits<uint16_t>::max());
            return false;
        }

        switch (node.kind)
        {
            case AstNode::Kind::Alias:
            {
                return VerifyTypeId(node)
                    && VerifyType(node.alias.target, *node.parent);
            }
            case AstNode::Kind::Attribute:
            {
                return VerifyTypeId(node)
                    && VerifyType(node.attribute.type, *node.parent);
            }
            case AstNode::Kind::Constant:
            {
                return VerifyTypeId(node)
                    && VerifyType(node.constant.type, *node.parent)
                    && VerifyValue(node.constant.type, *node.parent, node.constant.value, *node.parent);
            }
            case AstNode::Kind::Enum:
            {
                if (!VerifyTypeId(node))
                    return false;

                if (!VerifyMembers(node, AstNode::Kind::Enumerator))
                    return false;

                for (AstNode& child : node.children)
                {
                    if (node.kind != AstNode::Kind::Enumerator)
                    {
                        m_context->AddError(child.location, "Enums may only contain enumerators");
                        return false;
                    }

                    if (!VerifyNode(child))
                        return false;
                }
                return true;
            }
            case AstNode::Kind::Enumerator:
            {
                return VerifyOrdinal(node);
            }
            case AstNode::Kind::Field:
            {
                return VerifyOrdinal(node)
                    && VerifyType(node.field.type, *node.parent)
                    && (
                        node.field.defaultValue.kind == AstExpression::Kind::Unknown
                        || VerifyValue(node.field.type, *node.parent, node.field.defaultValue, *node.parent)
                    );
            }
            case AstNode::Kind::File:
            {
                if (!HasFlag(node.id, TypeIdFlag))
                {
                    TypeId id = 0;
                    const bool idResult = GetSecureRandomBytes(reinterpret_cast<uint8_t*>(&id), sizeof(id));
                    HE_ASSERT(idResult);
                    HE_UNUSED(idResult);

                    id |= TypeIdFlag;

                    m_context->AddError(node.location, "Invalid file unique ID. Add this line to the top of your file: @{:#018x};", id);
                    return false;
                }

                if (node.file.nameSpace.kind != AstExpression::Kind::Unknown)
                {
                    if (node.file.nameSpace.kind != AstExpression::Kind::QualifiedName)
                    {
                        m_context->AddError(node.file.nameSpace.location, "Invalid namespace expression. Expected a series of identifiers separated by dots.");
                        return false;
                    }

                    for (const AstExpression& item : node.file.nameSpace.qualified.names)
                    {
                        if (item.kind != AstExpression::Kind::Identifier)
                        {
                            m_context->AddError(node.file.nameSpace.location, "Invalid namespace expression. Expected a series of identifiers separated by dots.");
                            return false;
                        }
                    }
                }

                for (const AstExpression& importExpr : node.file.imports)
                {
                    if (importExpr.kind != AstExpression::Kind::String)
                    {
                        m_context->AddError(importExpr.location, "Expected string expression for import, but got {}", importExpr.kind);
                        return false;
                    }

                    if (importExpr.string.IsEmpty())
                    {
                        m_context->AddError(importExpr.location, "Import values cannot be empty");
                        return false;
                    }
                }

                return VerifyNodes(node.children);
            }
            case AstNode::Kind::Group:
            {
                uint32_t childCount = 0;
                for (AstNode& item : node.children)
                {
                    if (!VerifyNode(item))
                        return false;

                    if (item.kind == AstNode::Kind::Field || item.kind == AstNode::Kind::Group || item.kind == AstNode::Kind::Union)
                        ++childCount;
                }
                if (childCount < 1)
                {
                    m_context->AddError(node.location, "A group must contain at least 1 field, group, or union.");
                    return false;
                }
                return true;
            }
            case AstNode::Kind::Interface:
            {
                if (!VerifyTypeId(node))
                    return false;

                if (!VerifyMembers(node, AstNode::Kind::Method))
                    return false;

                return VerifyNodes(node.children);
            }
            case AstNode::Kind::Method:
            {
                if (!VerifyOrdinal(node))
                    return false;

                if (!VerifyMethodParams(node, node.method.params))
                    return false;

                if (!VerifyMethodParams(node, node.method.results))
                    return false;

                return true;
            }
            case AstNode::Kind::Struct:
            {
                if (!VerifyTypeId(node))
                    return false;

                if (!VerifyMembers(node, AstNode::Kind::Field))
                    return false;

                return VerifyNodes(node.children);
            }
            case AstNode::Kind::Union:
            {
                uint32_t childCount = 0;
                for (AstNode& item : node.children)
                {
                    if (!VerifyNode(item))
                        return false;

                    if (item.kind == AstNode::Kind::Field || item.kind == AstNode::Kind::Group || item.kind == AstNode::Kind::Union)
                       ++childCount;
                }
                if (childCount < 2)
                {
                    m_context->AddError(node.location, "A union must contain at least 2 fields, groups, or unions.");
                    return false;
                }
                return true;
            }
        }

        m_context->AddError(node.location, "Unknown node type ({}) in AST. This is a parser bug.", node.kind);
        return false;
    }

    bool Verifier::VerifyNodes(const AstList<AstNode>& list)
    {
        for (const AstNode& item : list)
        {
            if (!VerifyNode(item))
                return false;
        }
        return true;
    }

    bool Verifier::VerifyAttributes(const AstNode& node)
    {
        if (node.attributes.Size() > std::numeric_limits<uint16_t>::max())
        {
            m_context->AddError(node.location, "Number of applied attributes is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
            return false;
        }

        for (const AstAttribute& astAttr : node.attributes)
        {
            if (astAttr.name.kind != AstExpression::Kind::QualifiedName)
            {
                m_context->AddError(astAttr.location, "Applied attribute's name is not a qualified name. This is a parser bug.");
                return false;
            }

            const AstNode* attrNode = m_context->FindNode(astAttr.name, node);
            if (!attrNode)
            {
                m_context->AddError(astAttr.location, "Unknown attribute identifier, no declaration found for this name");
                return false;
            }

            if (attrNode->kind != AstNode::Kind::Attribute)
            {
                m_context->AddError(astAttr.location, "Expected attribute identifier, but got {}", attrNode->kind);
                return false;
            }

            if (!VerifyValue(attrNode->attribute.type, *attrNode->parent, astAttr.value, *node.parent))
                return false;

            switch (node.kind)
            {
                case AstNode::Kind::Alias:
                    m_context->AddError(node.location, "Aliases cannot have attributes.");
                    return false;
                    // if (!attrNode->attribute.targetsAlias)
                    // {
                    //     m_context->AddError(node.location, "Attribute {} cannot be used on Aliases", attrNode->name);
                    //     return false;
                    // }
                    // break;
                case AstNode::Kind::Attribute:
                    if (!attrNode->attribute.targetsAttribute)
                    {
                        m_context->AddError(node.location, "Attribute {} cannot be used on Attributes", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Constant:
                    if (!attrNode->attribute.targetsConstant)
                    {
                        m_context->AddError(node.location, "Attribute {} cannot be used on Constants", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Enum:
                    if (!attrNode->attribute.targetsEnum)
                    {
                        m_context->AddError(node.location, "Attribute {} cannot be used on Enums", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Enumerator:
                    if (!attrNode->attribute.targetsEnumerator)
                    {
                        m_context->AddError(node.location, "Attribute {} cannot be used on Enumerators", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Field:
                    if (!attrNode->attribute.targetsField)
                    {
                        m_context->AddError(node.location, "Attribute {} cannot be used on Fields", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::File:
                    if (!attrNode->attribute.targetsFile)
                    {
                        m_context->AddError(node.location, "Attribute {} cannot be used on Files", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Group:
                    m_context->AddError(node.location, "Groups cannot have attributes.");
                    return false;
                    // if (!attrNode->attribute.targetsGroup)
                    // {
                    //     m_context->AddError(node.location, "Attribute {} cannot be used on Groups", attrNode->name);
                    //     return false;
                    // }
                    // break;
                case AstNode::Kind::Interface:
                    if (!attrNode->attribute.targetsInterface)
                    {
                        m_context->AddError(node.location, "Attribute {} cannot be used on Interfaces", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Method:
                    if (!attrNode->attribute.targetsMethod)
                    {
                        m_context->AddError(node.location, "Attribute {} cannot be used on Methods", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Struct:
                    if (!attrNode->attribute.targetsStruct)
                    {
                        m_context->AddError(node.location, "Attribute {} cannot be used on Structs", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Union:
                    m_context->AddError(node.location, "Unions cannot have attributes.");
                    return false;
                    // if (!attrNode->attribute.targetsUnion)
                    // {
                    //     m_context->AddError(node.location, "Attribute {} cannot be used on Unions", attrNode->name);
                    //     return false;
                    // }
                    // break;
            }
        }

        return true;
    }

    bool Verifier::VerifyMembers(const AstNode& node, AstNode::Kind kind)
    {
        std::set<MemberOrdinal> ordinals;
        if (!VerifyMembersOf(node, kind, ordinals))
            return false;

        if (ordinals.size() > std::numeric_limits<uint16_t>::max())
        {
            m_context->AddError(node.location, "Declaration contains too many members");
            return false;
        }

        uint16_t expectedOrdinal = 0;
        for (const MemberOrdinal& ordinal : ordinals)
        {
            if (ordinal.value != expectedOrdinal)
            {
                m_context->AddError(ordinal.loc, "Encountered gap in ordinal values, expected {} but got {}", expectedOrdinal, ordinal.value);
                return false;
            }

            ++expectedOrdinal;
        }

        return true;
    }

    bool Verifier::VerifyMembersOf(const AstNode& node, AstNode::Kind kind, std::set<MemberOrdinal>& ordinals)
    {
        std::unordered_set<StringView> names;
        names.reserve(node.children.Size());

        for (const AstNode& child : node.children)
        {
            // Special-case handling for structs, groups, and unions which can contain groups or
            // unions that we need to recurse into to check all the members.
            if (node.kind == AstNode::Kind::Struct || node.kind == AstNode::Kind::Group || node.kind == AstNode::Kind::Union)
            {
                if (child.kind == AstNode::Kind::Group || child.kind == AstNode::Kind::Union)
                {
                    if (!VerifyMembersOf(child, kind, ordinals))
                        return false;
                }
            }

            if (child.kind != kind)
                continue;

            if (child.id > std::numeric_limits<uint16_t>::max())
            {
                m_context->AddError(child.location, "Ordinal value is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
                return false;
            }

            auto ordinalPair = ordinals.insert({ static_cast<uint16_t>(child.id), child.location });
            if (!ordinalPair.second)
            {
                m_context->AddError(child.location, "Encountered duplicate ordinal value for {} '{} @{}'", kind, child.name, child.id);
                return false;
            }

            auto namePair = names.insert(child.name);
            if (!namePair.second)
            {
                m_context->AddError(child.location, "Encountered duplicate name for {}", kind);
                return false;
            }
        }
        return true;
    }

    bool Verifier::VerifyMethodParams(const AstNode& node, const AstMethodParams& params)
    {
        switch (params.kind)
        {
            case AstMethodParams::Kind::Fields:
            {
                for (AstNode& child : node.children)
                {
                    if (node.kind != AstNode::Kind::Field)
                    {
                        m_context->AddError(child.location, "Method parameter lists may only contain fields");
                        return false;
                    }

                    if (!VerifyNode(child))
                        return false;
                }
                return true;
            }
            case AstMethodParams::Kind::Type:
            {
                if (params.type.kind == AstExpression::Kind::Unknown)
                    return true;

                if (params.type.kind != AstExpression::Kind::QualifiedName)
                {
                    m_context->AddError(params.type.location, "Expected structure name, but encountered {}", params.type.kind);
                    return false;
                }

                if (params.type.qualified.names.Size() == 1)
                {
                    const AstExpression* name = params.type.qualified.names.Front();
                    if (name->kind == AstExpression::Kind::Identifier && name->identifier == KW_Void)
                        return true;
                }

                if (!VerifyType(params.type, *node.parent, true))
                    return false;

                TypeKey key{ &params.type, node.parent };
                const TypeValue& value = m_context->GetType(key);
                if (!value.type || value.type->kind != AstNode::Kind::Struct)
                {
                    m_context->AddError(params.type.location, "Expected user-defined structure type, but got {}", value.tag);
                    return false;
                }

                return true;
            }
            default:
                break;
        }

        m_context->AddError(node.location, "Unknown method param type. This is a compiler bug.");
        return false;
    }

    bool Verifier::VerifyOrdinal(const AstNode& node)
    {
        if (node.id > std::numeric_limits<uint16_t>::max())
        {
            m_context->AddError(node.location, "Ordinal value is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
            return false;
        }

        return true;
    }

    bool Verifier::VerifyType(const AstExpression& ast, const AstNode& scope_, bool onlyPointers)
    {
        if (ast.kind == AstExpression::Kind::Array)
        {
            if (!ast.array.elementType)
            {
                m_context->AddError(ast.location, "Expected element type for array, but got nullptr. This is a parser bug.");
                return false;
            }

            if (ast.array.elementType->kind == AstExpression::Kind::Array && ast.array.elementType->array.size != nullptr)
            {
                m_context->AddError(ast.location, "Arrays cannot be nested in lists or other arrays are not supported.");
                return false;
            }

            Type::Data::Tag tag = Type::Data::Tag::List;

            if (ast.array.size)
            {
                if (onlyPointers)
                {
                    m_context->AddError(ast.location, "Only pointer types are allowed in this context, fixed-size arrays are not allowed.");
                    return false;
                }

                tag = Type::Data::Tag::Array;
                if (!VerifyTypeArraySize(*ast.array.size, scope_))
                    return false;
            }

            TypeKey key{ &ast, &scope_ };
            TypeValue value{ tag, nullptr, {} };
            m_context->TrackType(key, value);

            return VerifyType(*ast.array.elementType, scope_);
        }

        if (ast.kind != AstExpression::Kind::QualifiedName)
        {
            m_context->AddError(ast.location, "Expected type name, but encountered {}", ast.kind);
            return false;
        }

        const AstNode* scope = &scope_;
        for (const AstExpression& name : ast.qualified.names)
        {
            if (name.kind == AstExpression::Kind::Identifier)
            {
                // Check for type param
                const AstTypeParamRef ref = FindTypeParam(name.identifier, *scope);
                if (ref.scope)
                {
                    TypeKey key{ &ast, &scope_ };
                    TypeValue value{ Type::Data::Tag::AnyPointer, nullptr, ref };
                    m_context->TrackType(key, value);
                    continue;
                }

                // Check for builtin
                const auto it = m_builtinTypeMap.find(name.identifier);
                if (it != m_builtinTypeMap.end())
                {
                    if (onlyPointers && !IsPointer(it->second))
                    {
                        m_context->AddError(ast.location, "Only pointer types are allowed in this context, {} is not allowed.", name.identifier);
                        return false;
                    }

                    if (ast.qualified.names.Size() > 1)
                    {
                        m_context->AddError(ast.location, "Unexpected builtin type name in a qualified name.");
                        return false;
                    }

                    TypeKey key{ &ast, &scope_ };
                    TypeValue value{ it->second, nullptr, {} };
                    m_context->TrackType(key, value);
                    break;
                }

                // Check for user-defined type
                const AstNode* t = m_context->FindNode(name.identifier, *scope);
                if (!t)
                {
                    m_context->AddError(name.location, "Unknown identifier '{}'", name.identifier);
                    return false;
                }

                while (t->kind == AstNode::Kind::Alias)
                    t = m_context->FindNode(t->alias.target, *t->parent);

                if (t->kind != AstNode::Kind::Enum
                    && t->kind != AstNode::Kind::Interface
                    && t->kind != AstNode::Kind::Struct)
                {
                    m_context->AddError(name.location, "{} identifiers cannot be used as types", t->kind);
                    return false;
                }

                if (onlyPointers && t->kind == AstNode::Kind::Enum)
                {
                    m_context->AddError(ast.location, "Only pointer types are allowed in this context, Enums are not allowed.");
                    return false;
                }

                TypeKey key{ &ast, &scope_ };
                TypeValue value{ Type::Data::Tag::AnyPointer, t, {} };
                m_context->TrackType(key, value);

                scope = t;

                continue;
            }

            if (name.kind == AstExpression::Kind::Generic)
            {
                if (name.generic.params.IsEmpty())
                {
                    m_context->AddError(name.location, "Generic type parameters cannot be empty.");
                    return false;
                }

                // Check for user-defined type
                const AstNode* t = m_context->FindNode(name.generic.name, *scope);
                if (!t)
                {
                    m_context->AddError(name.location, "Unknown identifier '{}'", name.generic.name);
                    return false;
                }

                if (t->kind != AstNode::Kind::Interface && t->kind != AstNode::Kind::Struct)
                {
                    m_context->AddError(name.location, "{} identifiers cannot be used as generic types", t->kind);
                    return false;
                }

                if (t->typeParams.Size() != name.generic.params.Size())
                {
                    m_context->AddError(name.location, "Generic requires {} type parameters, but only {} were given.",
                        t->typeParams.Size(), name.generic.params.Size());
                    return false;
                }

                for (const AstExpression& param : name.generic.params)
                {
                    if (!VerifyType(param, *scope, true))
                        return false;
                }

                TypeKey key{ &ast, scope};
                TypeValue value{ Type::Data::Tag::AnyPointer, t, {} };
                m_context->TrackType(key, value);

                scope = t;

                continue;
            }

            m_context->AddError(name.location, "Expected identifier, but encountered {}", name.kind);
            return false;
        }

        return true;
    }

    bool Verifier::VerifyTypeArraySize(const AstExpression& ast, const AstNode& scope)
    {
        switch (ast.kind)
        {
            case AstExpression::Kind::UnsignedInt:
            {
                if (ast.unsignedInt > std::numeric_limits<uint16_t>::max())
                {
                    m_context->AddError(ast.location, "Array size is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
                    return false;
                }
                break;
            }
            case AstExpression::Kind::SignedInt:
            {
                if (ast.signedInt < 0)
                {
                    m_context->AddError(ast.location, "Array size is too small. Min is UINT16_MIN (0)");
                    return false;
                }
                if (ast.signedInt > std::numeric_limits<uint16_t>::max())
                {
                    m_context->AddError(ast.location, "Array size is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
                    return false;
                }
                break;
            }
            case AstExpression::Kind::QualifiedName:
            {
                const AstNode* sizeConst = m_context->FindNode(ast, scope);
                if (!sizeConst)
                {
                    m_context->AddError(ast.location, "Unknown identifier used as array size.");
                    return false;
                }

                if (sizeConst->kind != AstNode::Kind::Constant)
                {
                    m_context->AddError(ast.location, "Expected constant identifier, but encountered {}", sizeConst->kind);
                    return false;
                }

                if (sizeConst->constant.type.kind != AstExpression::Kind::QualifiedName
                    || sizeConst->constant.type.qualified.names.Size() > 1
                    || sizeConst->constant.type.qualified.names.Front()->kind != AstExpression::Kind::Identifier)
                {
                    m_context->AddError(ast.location, "Expected integer constant, but encountered constant of non-integral type.");
                    return false;
                }

                const auto typeIt = m_builtinTypeMap.find(sizeConst->constant.type.qualified.names.Front()->identifier);
                if (typeIt == m_builtinTypeMap.end())
                {
                    m_context->AddError(ast.location, "Expected integer constant, but encountered constant of non-integral type.");
                    return false;
                }

                switch (typeIt->second)
                {
                    case Type::Data::Tag::Int8:
                    case Type::Data::Tag::Int16:
                    case Type::Data::Tag::Int32:
                    case Type::Data::Tag::Int64:
                    case Type::Data::Tag::Uint8:
                    case Type::Data::Tag::Uint16:
                    case Type::Data::Tag::Uint32:
                    case Type::Data::Tag::Uint64:
                        break;

                    default:
                        m_context->AddError(ast.location, "Expected integer constant, but encountered constant of type {}", typeIt->second);
                        return false;
                }

                if (!VerifyTypeArraySize(sizeConst->constant.value, *sizeConst->parent))
                    return false;

                break;
            }
            default:
                m_context->AddError(ast.location, "Expected unsigned integer or integer constant for array size, but got {}", ast.kind);
                return false;
        }

        return true;
    }

    bool Verifier::VerifyTypeId(const AstNode& node)
    {
        if (!HasFlag(node.id, TypeIdFlag))
        {
            m_context->AddError(node.location, "Invalid unique ID. Did you accidentally use an ordinal as an ID?");
            return false;
        }

        bool result = m_context->TrackTypeId(node);
        if (!result)
        {
            const AstNode* otherNode = m_context->FindNode(node.id);
            HE_ASSERT(otherNode);
            m_context->AddError(node.location, "ID collision detected. Declaration '{}' has an ID that matches '{}' (L{}:{})",
                node.name, otherNode->name, otherNode->location.line, otherNode->location.column);
            return false;
        }

        return true;
    }

    bool Verifier::VerifyValue(const AstExpression& astType, const AstNode& scopeType, const AstExpression& astValue, const AstNode& scopeValue)
    {
        switch (astValue.kind)
        {
            case AstExpression::Kind::Blob:
                if (astType.kind == AstExpression::Kind::QualifiedName && astType.qualified.names.Size() == 1)
                {
                    const AstExpression* name = astType.qualified.names.Front();
                    return name->kind == AstExpression::Kind::Identifier && name->identifier == KW_Blob;
                }
                m_context->AddError(astValue.location, "A blob value expression is not valid for this type");
                return false;

            case AstExpression::Kind::Float:
                // TODO: Check size
                if (astType.kind == AstExpression::Kind::QualifiedName && astType.qualified.names.Size() == 1)
                {
                    const AstExpression* name = astType.qualified.names.Front();
                    return name->kind == AstExpression::Kind::Identifier && (name->identifier == KW_Float32 || name->identifier == KW_Float64);
                }
                m_context->AddError(astValue.location, "A float value expression is not valid for this type");
                return false;

            case AstExpression::Kind::String:
                if (astType.kind == AstExpression::Kind::QualifiedName && astType.qualified.names.Size() == 1)
                {
                    const AstExpression* name = astType.qualified.names.Front();
                    return name->kind == AstExpression::Kind::Identifier && name->identifier == KW_String;
                }
                m_context->AddError(astValue.location, "A string value expression is not valid for this type");
                return false;

            case AstExpression::Kind::SignedInt:
            case AstExpression::Kind::UnsignedInt:
                // TODO: Check size
                if (astType.kind == AstExpression::Kind::QualifiedName && astType.qualified.names.Size() == 1)
                {
                    const AstExpression* name = astType.qualified.names.Front();
                    if (name->kind == AstExpression::Kind::Identifier)
                    {
                        auto it = m_builtinTypeMap.find(name->identifier);
                        if (it != m_builtinTypeMap.end() && IsArithmetic(it->second))
                            return true;
                    }
                }
                m_context->AddError(astValue.location, "An integer value expression is not valid for this type");
                return false;

            case AstExpression::Kind::List:
                if (astType.kind != AstExpression::Kind::Array)
                {
                    m_context->AddError(astValue.location, "A list value expression is only valid for List and Array types");
                    return false;
                }
                return true;

            case AstExpression::Kind::QualifiedName:
            {
                const AstNode* valueNode = m_context->FindNode(astValue, scopeValue);
                if (!valueNode)
                {
                    m_context->AddError(astValue.location, "Unknown name for value.");
                    return false;
                }

                const AstNode* typeNode = m_context->FindNode(astType, scopeType);

                // We already validated the type, so if we don't find it then its a builtin.
                // Since there are no builtins that take a qualified name value, this is an error.
                // TODO: Bools take a value that is a qualified name (with one entry)
                if (!typeNode)
                {
                    m_context->AddError(astValue.location, "Invalid value expression for type");
                    return false;
                }

                if (astType.kind != AstExpression::Kind::QualifiedName)
                {
                    m_context->AddError(astValue.location, "A {} value expression is not valid for this type", valueNode->kind);
                    return false;
                }

                if (valueNode->kind == AstNode::Kind::Enumerator)
                {
                    if (typeNode->kind != AstNode::Kind::Enum)
                    {
                        m_context->AddError(astValue.location, "Value is an enumerator, but the type is not an Enum");
                        return false;
                    }

                    if (!valueNode->parent || valueNode->parent->kind != AstNode::Kind::Enum)
                    {
                        m_context->AddError(valueNode->location, "This enumerator does not live within an enum declaration. This is a parser bug.");
                        return false;
                    }

                    if (valueNode->parent != typeNode)
                    {
                        m_context->AddError(astValue.location, "Expected an enumerator from {}, but got an enumerator from {}",
                            typeNode->name, valueNode->parent->name);
                        return false;
                    }

                    return true;
                }

                if (valueNode->kind == AstNode::Kind::Constant)
                {
                    return VerifyValue(astType, scopeType, valueNode->constant.value, *valueNode->parent);
                }

                m_context->AddError(astValue.location, "Expected name to be an enumerator or constant, but found {}", valueNode->kind);
                return false;
            }
            case AstExpression::Kind::Tuple:
            {
                if (astType.kind != AstExpression::Kind::QualifiedName)
                {
                    m_context->AddError(astValue.location, "A {} value expression is not valid for this type", astValue.kind);
                    return false;
                }

                const AstNode* node = m_context->FindNode(astType, scopeType);
                HE_ASSERT(node); // we already validated the type, so we should always find it.
                if (node->kind != AstNode::Kind::Struct)
                {
                    m_context->AddError(astValue.location, "A Tuple value expression is only valid for Struct types");
                    return false;
                }

                for (const AstTupleParam& param : astValue.tuple)
                {
                    const AstNode* field = node->children.Find([&](const AstNode& child)
                    {
                        return child.kind == AstNode::Kind::Field && child.name == param.name;
                    });

                    if (!field)
                    {
                        m_context->AddError(param.location, "Field '{}' does not exist on type '{}'", param.name, node->name);
                        return false;
                    }

                    if (!VerifyValue(field->field.type, *field->parent, param.value, scopeValue))
                        return false;
                }

                return true;
            }
            case AstExpression::Kind::Unknown:
            {
                const TypeKey key{ &astType, &scopeType };
                const TypeValue& type = m_context->GetType(key);
                if (type.tag != Type::Data::Tag::Void)
                {
                    m_context->AddError(astValue.location, "A {} expression cannot be used as a value", astValue.kind);
                    return false;
                }
                return true;
            }

            case AstExpression::Kind::Array:
            case AstExpression::Kind::Generic:
            case AstExpression::Kind::Identifier:
            case AstExpression::Kind::Namespace:
                m_context->AddError(astValue.location, "A {} expression cannot be used as a value", astValue.kind);
                return false;
        }

        m_context->AddError(astValue.location, "Unknown expression type ({}) for value. This is a parser bug.", astValue.kind);
        return false;
    }

    AstTypeParamRef Verifier::FindTypeParam(StringView name, const AstNode& scope) const
    {
        bool found = false;
        AstTypeParamRef ref;
        uint16_t i = 0;

        for (const AstTypeParam& param : scope.typeParams)
        {
            if (param.name == name)
            {
                found = true;
                ref = { &scope, i++ };
                break;
            }
        }

        if (!found && scope.parent)
            return FindTypeParam(name, *scope.parent);

        return ref;
    }
}
