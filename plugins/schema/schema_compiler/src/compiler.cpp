// Copyright Chad Engler

#include "he/schema/compiler.h"

#include "struct_layout.h"
#include "utf8_helpers.h"

#include "he/core/ascii.h"
#include "he/core/enum_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/hash.h"
#include "he/core/random.h"
#include "he/core/string_view_fmt.h"

#include <concepts>
#include <set>
#include <unordered_set>

namespace he::schema
{
    constexpr StringView KW_Blob = "Blob";
    constexpr StringView KW_String = "String";
    constexpr StringView KW_AnyPointer = "AnyPointer";

    constexpr StringView KW_False = "false";
    constexpr StringView KW_True = "true";
    constexpr StringView KW_Void = "void";

    struct BuiltinType { const StringView name; const Type::Data::Tag kind; };
    constexpr BuiltinType BuiltinTypes[] =
    {
        { KW_Void, Type::Data::Tag::Void },
        { "bool", Type::Data::Tag::Bool },
        { "int8", Type::Data::Tag::Int8 },
        { "int16", Type::Data::Tag::Int16 },
        { "int32", Type::Data::Tag::Int32 },
        { "int64", Type::Data::Tag::Int64 },
        { "uint8", Type::Data::Tag::Uint8 },
        { "uint16", Type::Data::Tag::Uint16 },
        { "uint32", Type::Data::Tag::Uint32 },
        { "uint64", Type::Data::Tag::Uint64 },
        { "float32", Type::Data::Tag::Float32 },
        { "float64", Type::Data::Tag::Float64 },
        { KW_Blob, Type::Data::Tag::Blob },
        { KW_String, Type::Data::Tag::String },
        { KW_AnyPointer, Type::Data::Tag::AnyPointer },
    };

    static TypeId MakeTypeId(StringView name, TypeId parentId)
    {
        HE_ASSERT(HasFlag(parentId, TypeIdFlag));
        return FNV64::HashData(name.Data(), name.Size(), parentId) | TypeIdFlag;
    }

    Compiler::Compiler(AstFile& ast, const char* fileName, Span<const Compiler> includes)
        : m_ast(ast)
        , m_fileName(fileName)
        , m_includes(includes)
    {
        for (const BuiltinType& t : BuiltinTypes)
        {
            m_builtinTypeMap[t.name] = t.kind;
        }
    }

    bool Compiler::Compile()
    {
        if (!VerifyNode(m_ast.root))
            return false;

        m_schema = m_builder.AddStruct<SchemaFile>();
        CompileNode(m_ast.root, m_schema.InitRoot());
        return m_errors.IsEmpty();
    }

    bool Compiler::VerifyNode(AstNode& node)
    {
        if (node.kind != AstNode::Kind::File && node.parent == nullptr)
        {
            AddError(node.location, "AstNode is missing a parent reference. This is a bug in the parser.");
            return false;
        }

        if (!VerifyAttributes(node))
            return false;

        if (node.children.Size() > std::numeric_limits<uint16_t>::max())
        {
            AddError(node.location, "{} has too many members. Max is UINT16_MAX ({})", node.kind, std::numeric_limits<uint16_t>::max());
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
                        AddError(child.location, "Enums may only contain enumerators");
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

                    AddError(node.location, "Invalid file unique ID. Add this line to the top of your file: @{:#018x};", id);
                    return false;
                }

                if (node.file.nameSpace.kind != AstExpression::Kind::Unknown)
                {
                    if (node.file.nameSpace.kind != AstExpression::Kind::QualifiedName)
                    {
                        AddError(node.file.nameSpace.location, "Invalid namespace expression. Expected a series of identifiers separated by dots.");
                        return false;
                    }

                    for (const AstExpression& item : node.file.nameSpace.qualified.names)
                    {
                        if (item.kind != AstExpression::Kind::Identifier)
                        {
                            AddError(node.file.nameSpace.location, "Invalid namespace expression. Expected a series of identifiers separated by dots.");
                            return false;
                        }
                    }
                }

                for (const AstExpression& importExpr : node.file.imports)
                {
                    if (importExpr.kind != AstExpression::Kind::String)
                    {
                        AddError(importExpr.location, "Expected a string value for import, but got {}", importExpr.kind);
                        return false;
                    }

                    if (importExpr.string.IsEmpty())
                    {
                        AddError(importExpr.location, "Import values cannot be empty");
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
                    AddError(node.location, "A group must contain at least 1 field, group, or union.");
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
                    AddError(node.location, "A union must contain at least 2 fields, groups, or unions.");
                    return false;
                }
                return true;
            }
        }

        AddError(node.location, "Unknown node type ({}) in AST. This is a parser bug.", node.kind);
        return false;
    }

    bool Compiler::VerifyNodes(AstList<AstNode>& list)
    {
        for (AstNode& item : list)
        {
            if (!VerifyNode(item))
                return false;
        }
        return true;
    }

    bool Compiler::VerifyAttributes(const AstNode& node)
    {
        if (node.attributes.Size() > std::numeric_limits<uint16_t>::max())
        {
            AddError(node.location, "Enumerator ordinal is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
            return false;
        }

        for (const AstAttribute& astAttr : node.attributes)
        {
            if (astAttr.name.kind != AstExpression::Kind::QualifiedName)
            {
                AddError(astAttr.location, "Applied attribute's name is not a qualified name. This is a parser bug.");
                return false;
            }

            const AstNode* attrNode = FindNode(astAttr.name, node);
            if (!attrNode)
            {
                AddError(astAttr.location, "Unknown attribute identifier, no declaration found for this name");
                return false;
            }

            if (attrNode->kind != AstNode::Kind::Attribute)
            {
                AddError(astAttr.location, "Expected attribute identifier, but got {}", attrNode->kind);
                return false;
            }

            if (!VerifyValue(attrNode->attribute.type, *attrNode->parent, astAttr.value, *node.parent))
                return false;

            switch (node.kind)
            {
                case AstNode::Kind::Alias:
                    AddError(node.location, "Aliases cannot have attributes.");
                    return false;
                    // if (!attrNode->attribute.targetsAlias)
                    // {
                    //     AddError(node.location, "Attribute {} cannot be used on Aliases", attrNode->name);
                    //     return false;
                    // }
                    // break;
                case AstNode::Kind::Attribute:
                    if (!attrNode->attribute.targetsAttribute)
                    {
                        AddError(node.location, "Attribute {} cannot be used on Attributes", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Constant:
                    if (!attrNode->attribute.targetsConstant)
                    {
                        AddError(node.location, "Attribute {} cannot be used on Constants", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Enum:
                    if (!attrNode->attribute.targetsEnum)
                    {
                        AddError(node.location, "Attribute {} cannot be used on Enums", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Enumerator:
                    if (!attrNode->attribute.targetsEnumerator)
                    {
                        AddError(node.location, "Attribute {} cannot be used on Enumerators", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Field:
                    if (!attrNode->attribute.targetsField)
                    {
                        AddError(node.location, "Attribute {} cannot be used on Fields", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::File:
                    if (!attrNode->attribute.targetsFile)
                    {
                        AddError(node.location, "Attribute {} cannot be used on Files", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Group:
                    AddError(node.location, "Groups cannot have attributes.");
                    return false;
                    // if (!attrNode->attribute.targetsGroup)
                    // {
                    //     AddError(node.location, "Attribute {} cannot be used on Groups", attrNode->name);
                    //     return false;
                    // }
                    // break;
                case AstNode::Kind::Interface:
                    if (!attrNode->attribute.targetsInterface)
                    {
                        AddError(node.location, "Attribute {} cannot be used on Interfaces", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Method:
                    if (!attrNode->attribute.targetsMethod)
                    {
                        AddError(node.location, "Attribute {} cannot be used on Methods", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Struct:
                    if (!attrNode->attribute.targetsStruct)
                    {
                        AddError(node.location, "Attribute {} cannot be used on Structs", attrNode->name);
                        return false;
                    }
                    break;
                case AstNode::Kind::Union:
                    AddError(node.location, "Unions cannot have attributes.");
                    return false;
                    // if (!attrNode->attribute.targetsUnion)
                    // {
                    //     AddError(node.location, "Attribute {} cannot be used on Unions", attrNode->name);
                    //     return false;
                    // }
                    // break;
            }
        }

        return true;
    }

    bool Compiler::VerifyMembers(const AstNode& node, AstNode::Kind kind)
    {
        std::set<MemberOrdinal> ordinals;
        if (!VerifyMembersOf(node, kind, ordinals))
            return false;

        if (ordinals.size() > std::numeric_limits<uint16_t>::max())
        {
            AddError(node.location, "Declaration contains too many members");
            return false;
        }

        uint16_t expectedOrdinal = 0;
        for (const MemberOrdinal& ordinal : ordinals)
        {
            if (ordinal.value != expectedOrdinal)
            {
                AddError(ordinal.loc, "Encountered gap in ordinal values, expected {} but got {}", expectedOrdinal, ordinal.value);
                return false;
            }

            ++expectedOrdinal;
        }

        return true;
    }

    bool Compiler::VerifyMembersOf(const AstNode& node, AstNode::Kind kind, std::set<MemberOrdinal>& ordinals)
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
                AddError(child.location, "Ordinal value is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
                return false;
            }

            auto ordinalPair = ordinals.insert({ static_cast<uint16_t>(child.id), child.location });
            if (!ordinalPair.second)
            {
                AddError(child.location, "Encountered duplicate ordinal value for {} '{} @{}'", kind, child.name, child.id);
                return false;
            }

            auto namePair = names.insert(child.name);
            if (!namePair.second)
            {
                AddError(child.location, "Encountered duplicate name for {}", kind);
                return false;
            }
        }
        return true;
    }

    bool Compiler::VerifyMethodParams(AstNode& node, const AstMethodParams& params)
    {
        switch (params.kind)
        {
            case AstMethodParams::Kind::Type:
            {
                if (params.type.kind == AstExpression::Kind::Unknown)
                    return true;

                if (params.type.kind != AstExpression::Kind::QualifiedName)
                {
                    AddError(params.type.location, "Expected structure name, but encountered {}", params.type.kind);
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
                const TypeValue& value = m_typeMap.at(key);
                if (!value.type || value.type->kind != AstNode::Kind::Struct)
                {
                    AddError(params.type.location, "Expected user-defined structure type, but got {}", value.tag);
                    return false;
                }

                return true;
            }
            case AstMethodParams::Kind::Fields:
            {
                for (AstNode& child : node.children)
                {
                    if (node.kind != AstNode::Kind::Field)
                    {
                        AddError(child.location, "Method parameter lists may only contain fields");
                        return false;
                    }

                    if (!VerifyNode(child))
                        return false;
                }
                return true;
            }
            default:
                break;
        }

        AddError(node.location, "Unknown method param type. This is a compiler bug.");
        return false;
    }

    bool Compiler::VerifyOrdinal(const AstNode& node)
    {
        if (node.id > std::numeric_limits<uint16_t>::max())
        {
            AddError(node.location, "Ordinal value is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
            return false;
        }

        return true;
    }

    bool Compiler::VerifyType(const AstExpression& ast, const AstNode& scope_, bool onlyPointers)
    {
        if (ast.kind == AstExpression::Kind::Array)
        {
            if (!ast.array.elementType)
            {
                AddError(ast.location, "Expected element type for array, but got nullptr. This is a parser bug.");
                return false;
            }

            if (ast.array.elementType->kind == AstExpression::Kind::Array)
            {
                AddError(ast.location, "Nested Arrays and Lists are not supported.");
                return false;
            }

            Type::Data::Tag tag = Type::Data::Tag::List;

            if (ast.array.size)
            {
                if (onlyPointers)
                {
                    AddError(ast.location, "Only pointer types are allowed in this context, fixed-size arrays are not allowed.");
                    return false;
                }

                tag = Type::Data::Tag::Array;
                if (!VerifyTypeArraySize(*ast.array.size, scope_))
                    return false;
            }

            TypeKey key{ &ast, &scope_ };
            TypeValue value{ tag, nullptr, {} };
            m_typeMap.insert_or_assign(key, value);

            return VerifyType(*ast.array.elementType, scope_);
        }

        if (ast.kind != AstExpression::Kind::QualifiedName)
        {
            AddError(ast.location, "Expected type name, but encountered {}", ast.kind);
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
                    m_typeMap.insert_or_assign(key, value);
                    continue;
                }

                // Check for builtin
                const auto it = m_builtinTypeMap.find(name.identifier);
                if (it != m_builtinTypeMap.end())
                {
                    if (onlyPointers && !IsPointer(it->second))
                    {
                        AddError(ast.location, "Only pointer types are allowed in this context, {} is not allowed.", name.identifier);
                        return false;
                    }

                    if (ast.qualified.names.Size() > 1)
                    {
                        AddError(ast.location, "Unexpected builtin type name in a qualified name.");
                        return false;
                    }

                    TypeKey key{ &ast, &scope_ };
                    TypeValue value{ it->second, nullptr, {} };
                    m_typeMap.insert_or_assign(key, value);
                    break;
                }

                // Check for user-defined type
                const AstNode* t = FindNode(name.identifier, *scope);
                if (!t)
                {
                    AddError(name.location, "Unknown identifier '{}'", name.identifier);
                    return false;
                }

                while (t->kind == AstNode::Kind::Alias)
                    t = FindNode(t->alias.target, *t->parent);

                if (t->kind != AstNode::Kind::Enum
                    && t->kind != AstNode::Kind::Interface
                    && t->kind != AstNode::Kind::Struct)
                {
                    AddError(name.location, "{} identifiers cannot be used as types", t->kind);
                    return false;
                }

                if (onlyPointers && t->kind == AstNode::Kind::Enum)
                {
                    AddError(ast.location, "Only pointer types are allowed in this context, Enums are not allowed.");
                    return false;
                }

                TypeKey key{ &ast, &scope_ };
                TypeValue value{ Type::Data::Tag::AnyPointer, t, {} };
                m_typeMap.insert_or_assign(key, value);

                scope = t;

                continue;
            }

            if (name.kind == AstExpression::Kind::Generic)
            {
                if (name.generic.params.IsEmpty())
                {
                    AddError(name.location, "Generic type parameters cannot be empty.");
                    return false;
                }

                // Check for user-defined type
                const AstNode* t = FindNode(name.generic.name, *scope);
                if (!t)
                {
                    AddError(name.location, "Unknown identifier '{}'", name.generic.name);
                    return false;
                }

                if (t->kind != AstNode::Kind::Interface && t->kind != AstNode::Kind::Struct)
                {
                    AddError(name.location, "{} identifiers cannot be used as generic types", t->kind);
                    return false;
                }

                if (t->typeParams.Size() != name.generic.params.Size())
                {
                    AddError(name.location, "Generic requires {} type parameters, but only {} were given.",
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
                m_typeMap.insert_or_assign(key, value);

                scope = t;

                continue;
            }

            AddError(name.location, "Expected identifier, but encountered {}", name.kind);
            return false;
        }

        return true;
    }

    bool Compiler::VerifyTypeArraySize(const AstExpression& ast, const AstNode& scope)
    {
        switch (ast.kind)
        {
            case AstExpression::Kind::UnsignedInt:
            {
                if (ast.unsignedInt > std::numeric_limits<uint16_t>::max())
                {
                    AddError(ast.location, "Array size is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
                    return false;
                }
                break;
            }
            case AstExpression::Kind::SignedInt:
            {
                if (ast.signedInt < 0)
                {
                    AddError(ast.location, "Array size is too small. Min is UINT16_MIN (0)");
                    return false;
                }
                if (ast.signedInt > std::numeric_limits<uint16_t>::max())
                {
                    AddError(ast.location, "Array size is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
                    return false;
                }
                break;
            }
            case AstExpression::Kind::QualifiedName:
            {
                const AstNode* sizeConst = FindNode(ast, scope);
                if (!sizeConst)
                {
                    AddError(ast.location, "Unknown identifier used as array size.");
                    return false;
                }

                if (sizeConst->kind != AstNode::Kind::Constant)
                {
                    AddError(ast.location, "Expected constant identifier, but encountered {}", sizeConst->kind);
                    return false;
                }

                if (sizeConst->constant.type.kind != AstExpression::Kind::QualifiedName
                    || sizeConst->constant.type.qualified.names.Size() > 1
                    || sizeConst->constant.type.qualified.names.Front()->kind != AstExpression::Kind::Identifier)
                {
                    AddError(ast.location, "Expected integer constant, but encountered constant of non-integral type.");
                    return false;
                }

                const auto typeIt = m_builtinTypeMap.find(sizeConst->constant.type.qualified.names.Front()->identifier);
                if (typeIt == m_builtinTypeMap.end())
                {
                    AddError(ast.location, "Expected integer constant, but encountered constant of non-integral type.");
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
                        AddError(ast.location, "Expected integer constant, but encountered constant of type {}", typeIt->second);
                        return false;
                }

                if (!VerifyTypeArraySize(sizeConst->constant.value, *sizeConst->parent))
                    return false;

                break;
            }
            default:
                AddError(ast.location, "Expected unsigned integer or integer constant for array size, but got {}", ast.kind);
                return false;
        }

        return true;
    }

    bool Compiler::VerifyTypeId(AstNode& node)
    {
        if (node.id == 0)
        {
            node.id = MakeTypeId(node.name, node.parent->id);
        }

        if (!HasFlag(node.id, TypeIdFlag))
        {
            AddError(node.location, "Invalid unique ID. Did you accidentally use an ordinal as an ID?");
            return false;
        }

        auto result = m_typeIdMap.emplace(node.id, &node);
        if (!result.second)
        {
            const AstNode* otherNode = result.first->second;
            AddError(node.location, "ID collision detected. Declaration '{}' has an ID that matches '{}' (L{}:{})",
                node.name, otherNode->name, otherNode->location.line, otherNode->location.column);
            return false;
        }

        return true;
    }

    bool Compiler::VerifyValue(const AstExpression& astType, const AstNode& scopeType, const AstExpression& astValue, const AstNode& scopeValue)
    {
        switch (astValue.kind)
        {
            case AstExpression::Kind::Unknown:
                break;
            case AstExpression::Kind::Blob:
            case AstExpression::Kind::Float:
            case AstExpression::Kind::String:
                if (astType.kind != astValue.kind)
                {
                    AddError(astValue.location, "A {} value expression is not valid for this type", astValue.kind);
                    return false;
                }
                return true;
            case AstExpression::Kind::SignedInt:
            case AstExpression::Kind::UnsignedInt:
                if (astType.kind != AstExpression::Kind::SignedInt && astType.kind != AstExpression::Kind::UnsignedInt)
                {
                    AddError(astValue.location, "An Integer value expression is not valid for this type", astValue.kind);
                    return false;
                }
                return true;
            case AstExpression::Kind::List:
                if (astType.kind != AstExpression::Kind::List && astType.kind != AstExpression::Kind::Array)
                {
                    AddError(astValue.location, "A List value expression is only valid for List and Array types");
                    return false;
                }
                return true;
            case AstExpression::Kind::QualifiedName:
            {
                const AstNode* valueNode = FindNode(astValue, scopeValue);
                if (!valueNode)
                {
                    AddError(astValue.location, "Unknown name for value.");
                    return false;
                }

                const AstNode* typeNode = FindNode(astType, scopeType);
                HE_ASSERT(typeNode); // we already validated the type, so we should always find it.

                if (astType.kind != AstExpression::Kind::QualifiedName)
                {
                    AddError(astValue.location, "A {} value expression is not valid for this type", valueNode->kind);
                    return false;
                }

                if (valueNode->kind == AstNode::Kind::Enumerator)
                {
                    if (typeNode->kind != AstNode::Kind::Enum)
                    {
                        AddError(astValue.location, "Value is an enumerator, but the type is not an Enum");
                        return false;
                    }

                    if (!valueNode->parent || valueNode->parent->kind != AstNode::Kind::Enum)
                    {
                        AddError(valueNode->location, "This enumerator does not live within an enum declaration. This is a parser bug.");
                        return false;
                    }

                    if (valueNode->parent != typeNode)
                    {
                        AddError(astValue.location, "Expected an enumerator from {}, but got an enumerator from {}",
                            typeNode->name, valueNode->parent->name);
                        return false;
                    }

                    return true;
                }

                if (valueNode->kind == AstNode::Kind::Constant)
                {
                    return VerifyValue(astType, scopeType, valueNode->constant.value, *valueNode->parent);
                }

                AddError(astValue.location, "Expected name to be an enumerator or constant, but found {}", valueNode->kind);
                return false;
            }
            case AstExpression::Kind::Tuple:
            {
                if (astType.kind != AstExpression::Kind::QualifiedName)
                {
                    AddError(astValue.location, "A {} value expression is not valid for this type", astValue.kind);
                    return false;
                }

                const AstNode* node = FindNode(astType, scopeType);
                HE_ASSERT(node); // we already validated the type, so we should always find it.
                if (node->kind != AstNode::Kind::Struct)
                {
                    AddError(astValue.location, "A Tuple value expression is only valid for Struct types");
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
                        AddError(param.location, "Field '{}' does not exist on type '{}'", param.name, node->name);
                        return false;
                    }
                }

                return true;
            }
            case AstExpression::Kind::Array:
            case AstExpression::Kind::Generic:
            case AstExpression::Kind::Identifier:
            case AstExpression::Kind::Import:
            case AstExpression::Kind::Namespace:
                AddError(astValue.location, "A {} expression cannot be used as a value", astValue.kind);
                return false;
        }

        AddError(astValue.location, "Unknown expression type ({}) for value. This is a parser bug.", astValue.kind);
        return false;
    }

    void Compiler::CompileNode(const AstNode& node, Declaration::Builder decl)
    {
        decl.InitName(node.name);
        decl.SetId(node.id);
        if (node.parent)
            decl.SetParentId(node.parent->id);
        SourceInfo::Builder source = decl.InitSource();
        source.InitDocComment(node.docComment);
        source.InitFile(m_fileName);
        source.SetLine(node.location.line);
        source.SetColumn(node.location.column);
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
                AddError(node.location, "Expected declaration, but encountered {}", node.kind);
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
        decl.Data().SetFile();

        {
            he::String name(Allocator::GetTemp());
            for (const AstExpression& item : node.file.nameSpace.qualified.names)
            {
                HE_ASSERT(item.kind == AstExpression::Kind::Identifier);
                name += item.identifier;
                name += '.';
            }
            name.PopBack();
            decl.InitName(name);
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
                const AstNode* v = FindNode(params.type, node);
                HE_ASSERT(v && v->kind == AstNode::Kind::Struct);
                return v->id;
            }
        }

        AddError(child.location, "Unknown method param kind. This is a compiler bug.");
        return 0;
    }

    Type::Builder Compiler::CreateType(const AstExpression& ast, const AstNode& scope)
    {
        const auto it = m_typeMap.find({ &ast, &scope });
        HE_ASSERT(it != m_typeMap.end());
        const TypeValue& info = it->second;

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
                scope = FindNode(child.identifier, *scope);
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
                AddError(location, "Integer value out of range for type 'int8'");
                return false;
            }

            data.SetInt8(static_cast<int8_t>(value));
            return true;
        }

        if (type.IsInt16())
        {
            if (value > std::numeric_limits<int16_t>::max() || value < std::numeric_limits<int16_t>::min())
            {
                AddError(location, "Integer value out of range for type 'int16'");
                return false;
            }

            data.SetInt16(static_cast<int16_t>(value));
            return true;
        }

        if (type.IsInt32())
        {
            if (value > std::numeric_limits<int32_t>::max() || value < std::numeric_limits<int32_t>::min())
            {
                AddError(location, "Integer value out of range for type 'int32'");
                return false;
            }

            data.SetInt32(static_cast<int32_t>(value));
            return false;
        }

        if (type.IsInt64())
        {
            if (static_cast<uint64_t>(value) > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
            {
                AddError(location, "Integer value out of range for type 'int64'");
                return false;
            }

            data.SetInt64(static_cast<int64_t>(value));
            return true;
        }

        if (type.IsUint8())
        {
            if (value != -1 && (value > std::numeric_limits<uint8_t>::max() || value < std::numeric_limits<uint8_t>::min()))
            {
                AddError(location, "Integer value out of range for type 'uint8'");
                return false;
            }

            data.SetUint8(static_cast<int8_t>(value));
            return true;
        }

        if (type.IsUint16())
        {
            if (value != -1 && (value > std::numeric_limits<uint16_t>::max() || value < std::numeric_limits<uint16_t>::min()))
            {
                AddError(location, "Integer value out of range for type 'uint16'");
                return false;
            }

            data.SetUint16(static_cast<int16_t>(value));
            return true;
        }

        if (type.IsUint32())
        {
            if (value != -1 && (value > std::numeric_limits<uint32_t>::max() || value < std::numeric_limits<uint32_t>::min()))
            {
                AddError(location, "Integer value out of range for type 'uint32'");
                return false;
            }

            data.SetUint32(static_cast<uint32_t>(value));
            return false;
        }

        if (type.IsUint64())
        {
            if (value < 0 && static_cast<uint64_t>(value) != static_cast<uint64_t>(-1))
            {
                AddError(location, "Integer value out of range for type 'uint64'");
                return false;
            }

            data.SetUint64(static_cast<uint64_t>(value));
            return true;
        }

        AddError(location, "Expected {} value, but encountered integer expression", type.Tag());
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
                    AddError(ast.location, "Unexpected floating-point value");
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
                const AstNode* valueNode = FindNode(ast, scope);
                HE_ASSERT(valueNode);
                HE_ASSERT(valueNode->kind == AstNode::Kind::Enumerator || valueNode->kind == AstNode::Kind::Constant);

                if (valueNode->kind == AstNode::Kind::Constant)
                    return CreateValue(type, valueNode->constant.value, *valueNode->parent);

                data.SetEnum(static_cast<uint16_t>(valueNode->id));
                break;
            }
            case AstExpression::Kind::SignedInt:
            {
                if (SetInt(ast.location, ast.signedInt, typeData, data))
                    return {};
                break;
            }
            case AstExpression::Kind::String:
            {
                if (!typeData.IsString())
                {
                    AddError(ast.location, "Unexpected string value");
                    return {};
                }

                he::String str(Allocator::GetTemp());
                if (!DecodeString(ast, str))
                    return {};
                data.InitString(str);
                break;
            }
            case AstExpression::Kind::Tuple:
            {
                HE_ASSERT(typeData.IsStruct());
                const AstNode* structDeclNode = FindNode(typeData.Struct().Id());

                // Shouldn't be possible to not find this because if the ID is set on typeData,
                // then it must have come from a valid node.
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
                if (SetInt(ast.location, ast.unsignedInt, typeData, data))
                    return {};
                break;
            }

            // These are used in types, not in values. Any of this is a syntax error.
            case AstExpression::Kind::Array:
            case AstExpression::Kind::Generic:
            case AstExpression::Kind::Import:
            case AstExpression::Kind::Identifier:
            case AstExpression::Kind::Namespace:
            case AstExpression::Kind::Unknown:
                AddError(ast.location, "Expected a value expression, but got '{}' expression", ast.kind);
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
            const AstNode* attrDeclNode = FindNode(astAttr.name, scope);
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
            if (first == 0)
            {
                first = *s;
            }
            else
            {
                const uint8_t byte = HexPairToByte(first, *s);
                out.PushBack(byte);
                first = 0;
            }

            ++s;
        }

        if (first != 0)
        {
            AddError(ast.location, "Invalid blob byte string, there are trailing nibbles");
            return {};
        }

        return true;
    }

    bool Compiler::DecodeString(const AstExpression& ast, he::String& out)
    {
        HE_ASSERT(ast.kind == AstExpression::Kind::String);

        out.Clear();
        out.Reserve(ast.string.Size());

        bool decodedIsTrivial = true;
        int unicodeHighSurrogate = -1;

        const char* begin = ast.string.Begin();
        const char* end = ast.string.End();
        const char* s = begin;

        auto GetStrLoc = [&]()
        {
            AstFileLocation loc;
            loc.line = ast.location.line;
            loc.column = static_cast<uint32_t>(ast.location.column + (s - begin));
            return loc;
        };

        while (s < end)
        {
            char c = *s++;

            switch (c)
            {
                case '\"':
                    // Ignore surrounding double quotes.
                    break;
                case '\\':
                    decodedIsTrivial = false;
                    s++;

                    if (unicodeHighSurrogate != -1 && *s != 'u')
                    {
                        AddError(GetStrLoc(), "Illegal Unicode sequence (unpaired high surrogate)");
                        return false;
                    }

                    switch (*s)
                    {
                        case '\'': // single quote
                            out += '\'';
                            s++;
                            break;
                        case '\"': // double quote
                            out += '\"';
                            s++;
                            break;
                        case '\\': // backslash
                            out += '\\';
                            s++;
                            break;
                        case 'b': // backspace
                            out += '\b';
                            s++;
                            break;
                        case 'f': // form feed - new page
                            out += '\f';
                            s++;
                            break;
                        case 'n': // line feed - new line
                            out += '\n';
                            s++;
                            break;
                        case 'r': // carriage return
                            out += '\r';
                            s++;
                            break;
                        case 't': // horizontal tab
                            out += '\t';
                            s++;
                            break;
                        case 'v': // vertical tab
                            out += '\v';
                            s++;
                            break;
                        case 'x': // hex literal
                        case 'X':
                        {
                            s++;
                            char nibbles[]{ *s++, *s++ };
                            if (!IsHex(nibbles[0]) || !IsHex(nibbles[1]))
                            {
                                AddError(GetStrLoc(), "Escape code must be followed 2 hex digits in string literal");
                                return false;
                            }
                            uint8_t value = HexPairToByte(nibbles[0], nibbles[1]);
                            out += value;
                            break;
                        }
                        case 'u': // unicode sequence
                        case 'U':
                        {
                            s++;
                            char nibbles[]{ *s++, *s++, *s++, *s++ };
                            if (!IsHex(nibbles[0]) || !IsHex(nibbles[1]) || !IsHex(nibbles[2]) || !IsHex(nibbles[3]))
                            {
                                AddError(GetStrLoc(), "Escape code must be followed 4 hex digits in string literal");
                                return false;
                            }
                            uint16_t high = HexPairToByte(nibbles[0], nibbles[1]);
                            uint16_t low = HexPairToByte(nibbles[2], nibbles[3]);
                            uint16_t value = (high << 8) | low;

                            if (value >= 0xD800 && value <= 0xDBFF)
                            {
                                if (unicodeHighSurrogate != -1)
                                {
                                    AddError(GetStrLoc(), "Illegal Unicode sequence (multiple high surrogates) in string literal");
                                    return false;
                                }
                                else
                                {
                                    unicodeHighSurrogate = static_cast<int>(value);
                                }
                            }
                            else if (value >= 0xDC00 && value <= 0xDFFF)
                            {
                                if (unicodeHighSurrogate == -1)
                                {
                                    AddError(GetStrLoc(), "Illegal Unicode sequence (unpaired low surrogate) in string literal");
                                    return false;
                                }

                                uint32_t ucc = 0x10000 + ((unicodeHighSurrogate & 0x03FF) << 10) + (value & 0x03FF);
                                ToUTF8(out, ucc);
                                unicodeHighSurrogate = -1;
                            }
                            else
                            {
                                if (unicodeHighSurrogate == -1)
                                {
                                    AddError(GetStrLoc(), "Illegal Unicode sequence (unpaired high surrogate) in string literal");
                                    return false;
                                }
                                ToUTF8(out, value);
                            }
                        }
                        default:
                            AddError(GetStrLoc(), "Unknown escape sequence: \\{}", *s);
                            return false;
                    }
                    break;
                default:
                    if (unicodeHighSurrogate != -1)
                    {
                        AddError(GetStrLoc(), "Illegal Unicode sequence (unpaired high surrogate)");
                        return false;
                    }

                    decodedIsTrivial &= IsPrint(c);

                    out += c;
                    break;
            }
        }

        if (unicodeHighSurrogate != -1)
        {
            AddError(GetStrLoc(), "Illegal Unicode sequence (unpaired high surrogate)");
            return false;
        }

        if (!decodedIsTrivial && !ValidateUTF8(out.Data()))
        {
            AddError(GetStrLoc(), "Illegal UTF-8 sequence");
            return false;
        }

        return true;
    }

    const AstNode* Compiler::FindNode(TypeId id) const
    {
        auto it = m_typeIdMap.find(id);
        return it == m_typeIdMap.end() ? nullptr : it->second;
    }

    const AstNode* Compiler::FindNode(const AstExpression& name, const AstNode& scope_) const
    {
        HE_ASSERT(name.kind == AstExpression::Kind::QualifiedName);

        const AstNode* scope = &scope_;
        const AstExpression* expr = name.qualified.names.Front();
        if (expr->kind == AstExpression::Kind::Identifier && expr->identifier.IsEmpty())
        {
            scope = &m_ast.root;
            expr = name.qualified.names.Next(expr);
        }

        while (scope && expr)
        {
            switch (expr->kind)
            {
                case AstExpression::Kind::Identifier:
                    scope = FindNode(expr->identifier, *scope);
                    break;

                case AstExpression::Kind::Generic:
                    scope = FindNode(expr->generic.name, *scope);
                    break;

                default:
                    HE_ASSERT(false, "Invalid expression in qualified name, though should've been verified.");
                    break;
            }

            expr = name.qualified.names.Next(expr);
        }

        return scope;
    }

    const AstNode* Compiler::FindNode(StringView name, const AstNode& scope) const
    {
        const AstNode* node = scope.children.Find([&](const AstNode& node) { return node.name == name; });
        if (node)
            return node;

        if (scope.parent)
            return FindNode(name, *scope.parent);

        // If we walked up to top scope, and it isn't our root, then search our includes
        if (&scope == &m_ast.root)
        {
            for (const Compiler& compiler : m_includes)
            {
                node = FindNode(name, compiler.m_ast.root);
                if (node)
                    return node;
            }
        }

        return nullptr;
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
                const AstNode* constant = FindNode(ast, scope);
                HE_ASSERT(constant && constant->kind == AstNode::Kind::Constant);
                return GetArraySize(constant->constant.value, *constant->parent);
            }

            default:
                HE_ASSERT(false, "Encountered invalid array size value type. This should've been verified already.");
                return 0;
        }
    }

    Compiler::AstTypeParamRef Compiler::FindTypeParam(StringView name, const AstNode& scope) const
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

    template <typename... Args>
    void Compiler::AddError(const AstFileLocation& loc, fmt::format_string<Args...> fmt, Args&&... args)
    {
        ErrorInfo& entry = m_errors.EmplaceBack();
        entry.line = loc.line;
        entry.column = loc.column;
        entry.message.Clear();

        fmt::format_to(Appender(entry.message), fmt, Forward<Args>(args)...);
    }
}
