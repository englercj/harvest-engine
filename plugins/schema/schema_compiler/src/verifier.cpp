// Copyright Chad Engler

#include "verifier.h"

#include "compile_context.h"
#include "keywords.h"

#include "he/core/ascii.h"
#include "he/core/enum_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/hash.h"
#include "he/core/hash_table.h"
#include "he/core/random.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view_fmt.h"
#include "he/schema/schema.h"

#include "fmt/format.h"

#include <limits>
#include <set>

namespace he::schema
{
    struct BuiltinType { const StringView name; const Type::Data::UnionTag kind; };
    constexpr BuiltinType BuiltinTypes[] =
    {
        { KW_Void, Type::Data::UnionTag::Void },
        { KW_Bool, Type::Data::UnionTag::Bool },
        { KW_Int8, Type::Data::UnionTag::Int8 },
        { KW_Int16, Type::Data::UnionTag::Int16 },
        { KW_Int32, Type::Data::UnionTag::Int32 },
        { KW_Int64, Type::Data::UnionTag::Int64 },
        { KW_Uint8, Type::Data::UnionTag::Uint8 },
        { KW_Uint16, Type::Data::UnionTag::Uint16 },
        { KW_Uint32, Type::Data::UnionTag::Uint32 },
        { KW_Uint64, Type::Data::UnionTag::Uint64 },
        { KW_Float32, Type::Data::UnionTag::Float32 },
        { KW_Float64, Type::Data::UnionTag::Float64 },
        { KW_Blob, Type::Data::UnionTag::Blob },
        { KW_String, Type::Data::UnionTag::String },
        { KW_AnyPointer, Type::Data::UnionTag::AnyPointer },
        { KW_AnyStruct, Type::Data::UnionTag::AnyStruct },
        { KW_AnyList, Type::Data::UnionTag::AnyList },
    };

    [[nodiscard]] constexpr uint64_t Verifier::MemberOrdinal::HashCode() const
    {
        const uint64_t locValue = (static_cast<uint64_t>(loc.column) << 32) | loc.line;
        return CombineHash64(Mix64(value), Mix64(locValue));
    }

    bool Verifier::Verify(const AstFile& ast, CompileContext& ctx)
    {
        if (m_builtinTypeMap.IsEmpty())
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
            m_context->AddError(node.location, "{:s} has too many members. Max is UINT16_MAX ({})", node.kind, std::numeric_limits<uint16_t>::max());
            return false;
        }

        switch (node.kind)
        {
            case AstNode::Kind::Alias:
            {
                return VerifyDeclName(node)
                    && VerifyTypeId(node)
                    && VerifyType(node.alias.target, *node.parent);
            }
            case AstNode::Kind::Attribute:
            {
                return VerifyDeclName(node)
                    && VerifyTypeId(node)
                    && VerifyType(node.attribute.type, *node.parent);
            }
            case AstNode::Kind::Constant:
            {
                return VerifyDeclName(node)
                    && VerifyTypeId(node)
                    && VerifyType(node.constant.type, *node.parent)
                    && VerifyValue(node.constant.type, *node.parent, node.constant.value, *node.parent);
            }
            case AstNode::Kind::Enum:
            {
                if (!VerifyDeclName(node))
                    return false;

                if (!VerifyTypeId(node))
                    return false;

                if (!VerifyMembers(node, AstNode::Kind::Enumerator))
                    return false;

                for (AstNode& child : node.children)
                {
                    if (child.kind != AstNode::Kind::Enumerator)
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
                return VerifyDeclName(node) && VerifyOrdinal(node);
            }
            case AstNode::Kind::Field:
            {
                return VerifyFieldName(node)
                    && VerifyOrdinal(node)
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
                        m_context->AddError(importExpr.location, "Expected string expression for import, but got {:s}", importExpr.kind);
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
                if (!VerifyFieldName(node))
                    return false;

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
                if (!VerifyDeclName(node))
                    return false;

                if (!VerifyTypeId(node))
                    return false;

                if (!VerifyMembers(node, AstNode::Kind::Method))
                    return false;

                return VerifyNodes(node.children);
            }
            case AstNode::Kind::Method:
            {
                if (!VerifyDeclName(node))
                    return false;

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
                if (!VerifyDeclName(node))
                    return false;

                if (!VerifyTypeId(node))
                    return false;

                if (!VerifyMembers(node, AstNode::Kind::Field))
                    return false;

                return VerifyNodes(node.children);
            }
            case AstNode::Kind::Union:
            {
                if (!VerifyFieldName(node))
                    return false;

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

        m_context->AddError(node.location, "Unknown node type ({:s}) in AST. This is a parser bug.", node.kind);
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

            const AstNode* attrNode = m_context->FindNodeByName(astAttr.name, node);
            if (!attrNode)
            {
                m_context->AddError(astAttr.location, "Unknown attribute identifier, no declaration found for this name");
                return false;
            }

            if (attrNode->kind != AstNode::Kind::Attribute)
            {
                m_context->AddError(astAttr.location, "Expected attribute identifier, but got {:s}", attrNode->kind);
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
        HashSet<MemberOrdinal> ordinals;
        if (!VerifyMembersOf(node, kind, ordinals))
            return false;

        if (ordinals.Size() > std::numeric_limits<uint16_t>::max())
        {
            m_context->AddError(node.location, "Declaration contains too many members");
            return false;
        }

        if (ordinals.IsEmpty())
            return true;

        Vector<MemberOrdinal> sortedOrdinals;
        sortedOrdinals.Insert(0, ordinals.Begin(), ordinals.End());
        std::sort(sortedOrdinals.Begin(), sortedOrdinals.End(), [](const auto& a, const auto& b) { return a.value < b.value; });

        uint16_t expectedOrdinal = 0;
        for (const MemberOrdinal& ordinal : sortedOrdinals)
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

    bool Verifier::VerifyMembersOf(const AstNode& node, AstNode::Kind kind, HashSet<MemberOrdinal>& ordinals)
    {
        HashSet<StringView> names;
        names.Reserve(node.children.Size());

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

            const auto ordinalResult = ordinals.Insert({ static_cast<uint16_t>(child.id), child.location });
            if (!ordinalResult.inserted)
            {
                m_context->AddError(child.location, "Encountered duplicate ordinal value for {} '{} @{}'", kind, child.name, child.id);
                return false;
            }

            const auto nameResult = names.Insert(child.name);
            if (!nameResult.inserted)
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
                    m_context->AddError(params.type.location, "Expected structure name, but encountered {:s}", params.type.kind);
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

    bool Verifier::VerifyDeclName(const AstNode& node)
    {
        if (IsUpper(node.name[0])) [[likely]]
            return true;

        const char* p = node.name.Begin();
        const char* end = node.name.End();

        while (p < end && *p == '_')
            ++p;

        if (p > node.name.Begin() && p < end)
        {
            return IsNumeric(*p) || IsUpper(*p);
        }

        m_context->AddError(node.location, "Declaration name must begin with an upper-case letter, or underscores followed by a number or upper-case letter.");
        return false;
    }

    bool Verifier::VerifyFieldName(const AstNode& node)
    {
        if (IsLower(node.name[0])) [[likely]]
            return true;

        const char* p = node.name.Begin();
        const char* end = node.name.End();

        while (p < end && *p == '_')
            ++p;

        if (p > node.name.Begin() && p < end)
        {
            return IsNumeric(*p) || IsLower(*p);
        }

        m_context->AddError(node.location, "Field name must begin with a lower-case letter, or underscores followed by a number or lower-case letter.");
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

            if (!ast.array.size)
            {
                m_context->AddError(ast.location, "Expected size expression for array, but got nullptr. This is a parser bug.");
                return false;
            }

            if (ast.array.elementType->kind == AstExpression::Kind::Array)
            {
                m_context->AddError(ast.location, "Arrays of Arrays are not supported.");
                return false;
            }

            if (onlyPointers)
            {
                m_context->AddError(ast.location, "Only pointer types are allowed in this context, fixed-size arrays are not allowed.");
                return false;
            }

            if (!VerifyTypeArraySize(*ast.array.size, scope_))
                return false;

            TypeKey key{ &ast, &scope_ };
            TypeValue value{ Type::Data::UnionTag::Array, nullptr, {} };
            m_context->TrackType(key, value);

            return VerifyType(*ast.array.elementType, scope_);
        }

        if (ast.kind == AstExpression::Kind::List)
        {
            if (!ast.list.elementType)
            {
                m_context->AddError(ast.location, "Expected element type for list, but got nullptr. This is a parser bug.");
                return false;
            }

            if (ast.list.elementType->kind == AstExpression::Kind::Array)
            {
                m_context->AddError(ast.location, "Lists of Arrays are not supported.");
                return false;
            }

            TypeKey key{ &ast, &scope_ };
            TypeValue value{ Type::Data::UnionTag::List, nullptr, {} };
            m_context->TrackType(key, value);

            return VerifyType(*ast.list.elementType, scope_);
        }

        if (ast.kind != AstExpression::Kind::QualifiedName)
        {
            m_context->AddError(ast.location, "Expected type name, but encountered {:s}", ast.kind);
            return false;
        }

        for (const AstExpression& name : ast.qualified.names)
        {
            if (name.kind != AstExpression::Kind::Identifier && name.kind != AstExpression::Kind::Generic)
            {
                m_context->AddError(name.location, "Only type names are allowed here.");
                return false;
            }
        }

        // Special case handling for type params and built-in types
        if (ast.qualified.names.Size() == 1)
        {
            const AstExpression& name = *ast.qualified.names.Front();
            if (name.kind == AstExpression::Kind::Identifier)
            {
                // Check for type param
                const AstTypeParamRef ref = FindTypeParam(name.identifier, scope_);
                if (ref.scope)
                {
                    TypeKey key{ &ast, &scope_ };
                    TypeValue value{ Type::Data::UnionTag::Parameter, nullptr, ref };
                    m_context->TrackType(key, value);
                    return true;
                }

                // Check for builtin
                const Type::Data::UnionTag* tag = m_builtinTypeMap.Find(name.identifier);
                if (tag)
                {
                    if (onlyPointers && !IsPointer(*tag))
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
                    TypeValue value{ *tag, nullptr, {} };
                    m_context->TrackType(key, value);
                    return true;
                }
            }
        }

        // Find the user-defined type and cache it
        {
            const AstNode* t = m_context->FindNodeByName(ast, scope_);
            if (!t)
            {
                m_context->AddError(ast.location, "Unknown type name");
                return false;
            }

            while (t->kind == AstNode::Kind::Alias)
                t = m_context->FindNodeByName(t->alias.target, *t->parent);

            if (t->kind != AstNode::Kind::Enum
                && t->kind != AstNode::Kind::Interface
                && t->kind != AstNode::Kind::Struct)
            {
                m_context->AddError(ast.location, "{} identifiers cannot be used as types", t->kind);
                return false;
            }

            if (onlyPointers && t->kind == AstNode::Kind::Enum)
            {
                m_context->AddError(ast.location, "Only pointer types are allowed in this context, Enums are not allowed.");
                return false;
            }

            TypeKey key{ &ast, &scope_ };
            TypeValue value{ Type::Data::UnionTag::AnyPointer, t, {} };
            m_context->TrackType(key, value);
        }

        // Check the generic names and their params
        const AstNode* scope = &scope_;
        AstListIterator<AstExpression> startIt = ast.qualified.names.begin();
        for (AstListIterator<AstExpression> it = ast.qualified.names.begin(); it != ast.qualified.names.end(); ++it)
        {
            if (it->kind == AstExpression::Kind::Generic)
            {
                if (it->generic.params.IsEmpty())
                {
                    m_context->AddError(it->location, "Generic type parameters cannot be empty.");
                    return false;
                }

                // Check for user-defined type
                const AstNode* t = m_context->FindNodeByName(startIt, (it + 1), *scope);
                if (!t)
                {
                    m_context->AddError(it->location, "Unknown type name '{}'", it->generic.name);
                    return false;
                }

                if (t->kind != AstNode::Kind::Interface && t->kind != AstNode::Kind::Struct)
                {
                    m_context->AddError(it->location, "{} identifiers cannot be used as generic types", t->kind);
                    return false;
                }

                if (t->typeParams.Size() != it->generic.params.Size())
                {
                    m_context->AddError(it->location, "Generic requires {} type parameters, but only {} were given.",
                        t->typeParams.Size(), it->generic.params.Size());
                    return false;
                }

                for (const AstExpression& param : it->generic.params)
                {
                    if (!VerifyType(param, scope_, true))
                        return false;
                }

                TypeKey key{ &ast, scope };
                TypeValue value{ Type::Data::UnionTag::AnyPointer, t, {} };
                m_context->TrackType(key, value);

                scope = t;
                startIt = it + 1;
            }
        }

        return true;
    }

    bool Verifier::VerifyTypeArraySize(const AstExpression& ast, const AstNode& scope)
    {
        switch (ast.kind)
        {
            case AstExpression::Kind::UnsignedInt:
            {
                if (ast.unsignedInt < 2)
                {
                    m_context->AddError(ast.location, "Array size is too small. Must have at least 2 elements.");
                    return false;
                }
                if (ast.unsignedInt > std::numeric_limits<uint16_t>::max())
                {
                    m_context->AddError(ast.location, "Array size is too large. Max is UINT16_MAX ({})", std::numeric_limits<uint16_t>::max());
                    return false;
                }
                break;
            }
            case AstExpression::Kind::SignedInt:
            {
                if (ast.signedInt < 2)
                {
                    m_context->AddError(ast.location, "Array size is too small. Must have at least 2 elements.");
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
                const AstNode* sizeConst = m_context->FindNodeByName(ast, scope);
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

                const Type::Data::UnionTag* tag = m_builtinTypeMap.Find(sizeConst->constant.type.qualified.names.Front()->identifier);
                if (!tag)
                {
                    m_context->AddError(ast.location, "Expected integer constant, but encountered constant of non-integral type.");
                    return false;
                }

                switch (*tag)
                {
                    case Type::Data::UnionTag::Int8:
                    case Type::Data::UnionTag::Int16:
                    case Type::Data::UnionTag::Int32:
                    case Type::Data::UnionTag::Int64:
                    case Type::Data::UnionTag::Uint8:
                    case Type::Data::UnionTag::Uint16:
                    case Type::Data::UnionTag::Uint32:
                    case Type::Data::UnionTag::Uint64:
                        break;

                    default:
                        m_context->AddError(ast.location, "Expected integer constant, but encountered constant of type {:s}", *tag);
                        return false;
                }

                if (!VerifyTypeArraySize(sizeConst->constant.value, *sizeConst->parent))
                    return false;

                break;
            }
            default:
                m_context->AddError(ast.location, "Expected unsigned integer or integer constant for array size, but got {:s}", ast.kind);
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
            const AstNode* otherNode = m_context->FindNodeById(node.id);
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
                if (astType.kind == AstExpression::Kind::QualifiedName && astType.qualified.names.Size() == 1)
                {
                    const AstExpression* name = astType.qualified.names.Front();
                    if (name->kind == AstExpression::Kind::Identifier)
                    {
                        const Type::Data::UnionTag* tag = m_builtinTypeMap.Find(name->identifier);
                        if (tag && IsArithmetic(*tag))
                            return true;
                    }
                }
                m_context->AddError(astValue.location, "An integer value expression is not valid for this type");
                return false;

            case AstExpression::Kind::Sequence:
                if (astType.kind != AstExpression::Kind::Array && astType.kind != AstExpression::Kind::List)
                {
                    m_context->AddError(astValue.location, "A sequence value expression is only valid for List and Array types");
                    return false;
                }
                if (astType.kind == AstExpression::Kind::Array)
                {
                    HE_ASSERT(astType.array.size);
                    const uint16_t size = GetArraySize(*astType.array.size, scopeType);
                    if (size != astValue.sequence.Size())
                    {
                        m_context->AddError(astValue.location, "Sequence value expression length does not match array size");
                        return false;
                    }
                }
                return true;

            case AstExpression::Kind::QualifiedName:
            {
                if (astType.kind != AstExpression::Kind::QualifiedName)
                {
                    m_context->AddError(astValue.location, "Invalid type name in expression. This is a verifier bug.");
                    return false;
                }

                const AstNode* valueNode = m_context->FindNodeByName(astValue, scopeValue);
                if (valueNode)
                {
                    if (valueNode->kind == AstNode::Kind::Constant)
                    {
                        return VerifyValue(astType, scopeType, valueNode->constant.value, *valueNode->parent);
                    }
                }

                // We already validated the type, so if we don't find it then its a builtin.
                const AstNode* typeNode = m_context->FindNodeByName(astType, scopeType);
                Type::Data::UnionTag builtinType = Type::Data::UnionTag::Void;

                if (!typeNode)
                {
                    if (astType.kind != AstExpression::Kind::QualifiedName
                        || astType.qualified.names.Size() != 1
                        || astType.qualified.names.Front()->kind != AstExpression::Kind::Identifier)
                    {
                        m_context->AddError(astValue.location, "Invalid type expression expected a named type or builtin. This is verifier bug.");
                        return false;
                    }

                    const Type::Data::UnionTag* tag = m_builtinTypeMap.Find(astType.qualified.names.Front()->identifier);
                    if (!tag)
                    {
                        m_context->AddError(astValue.location, "Invalid type expression expected a named type or builtin. This is verifier bug.");
                        return false;
                    }

                    builtinType = *tag;
                }

                // user-defined type
                if (typeNode)
                {
                    if (!valueNode)
                    {
                        m_context->AddError(astValue.location, "Invalid value expression for this type");
                        return false;
                    }

                    if (valueNode->kind == AstNode::Kind::Enumerator)
                    {
                        if (!typeNode || typeNode->kind != AstNode::Kind::Enum)
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

                    m_context->AddError(astValue.location, "Expected name to be an enumerator or constant, but found {}", valueNode->kind);
                    return false;
                }

                // If we got here this must be a boolean value, otherwise we don't know what it is
                if (astValue.qualified.names.Size() == 1
                    && astValue.qualified.names.Front()->kind == AstExpression::Kind::Identifier
                    && (astValue.qualified.names.Front()->identifier == KW_True || astValue.qualified.names.Front()->identifier == KW_False))
                {
                    if (builtinType != Type::Data::UnionTag::Bool)
                    {
                        m_context->AddError(astValue.location, "The values `true` and `false` are only valid for bool types.");
                        return false;
                    }

                    return true;
                }

                m_context->AddError(astValue.location, "Unknown qualified name in value expression. Is this value defined?");
                return false;
            }
            case AstExpression::Kind::Tuple:
            {
                if (astType.kind != AstExpression::Kind::QualifiedName)
                {
                    m_context->AddError(astValue.location, "A {:s} value expression is not valid for this type", astValue.kind);
                    return false;
                }

                const AstNode* node = m_context->FindNodeByName(astType, scopeType);
                return VerifyTupleValue(node, astValue, scopeValue);
            }
            case AstExpression::Kind::Unknown:
            {
                if (astType.kind != AstExpression::Kind::QualifiedName
                    || astType.qualified.names.Front()->kind != AstExpression::Kind::Identifier
                    || astType.qualified.names.Front()->identifier != KW_Void)
                {
                    const TypeKey key{ &astType, &scopeType };
                    const TypeValue& type = m_context->GetType(key);
                    if (type.tag != Type::Data::UnionTag::Void)
                    {
                        // TODO: Bug here! Location should be the node, not the value (since it doesn't exist)
                        // This information isn't passed through to here, so we need to do that for an accurate error.
                        m_context->AddError(astValue.location, "Missing value expression, expected a {:s}", type.tag);
                        return false;
                    }
                }
                return true;
            }

            case AstExpression::Kind::Array:
            case AstExpression::Kind::List:
            case AstExpression::Kind::Generic:
            case AstExpression::Kind::Identifier:
            case AstExpression::Kind::Namespace:
                m_context->AddError(astValue.location, "A {:s} expression cannot be used as a value", astValue.kind);
                return false;
        }

        m_context->AddError(astValue.location, "Unknown expression type ({:s}) for value. This is a parser bug.", astValue.kind);
        return false;
    }

    bool Verifier::VerifyTupleValue(const AstNode* node, const AstExpression& astValue, const AstNode& scopeValue)
    {
        HE_ASSERT(astValue.kind == AstExpression::Kind::Tuple);

        if (!node || (node->kind != AstNode::Kind::Struct && node->kind != AstNode::Kind::Group && node->kind != AstNode::Kind::Union))
        {
            m_context->AddError(astValue.location, "A Tuple value expression is only valid for user-defined struct types, unions, and groups.");
            return false;
        }

        if (node->kind == AstNode::Kind::Union && astValue.tuple.Size() > 1)
        {
            m_context->AddError(astValue.location, "A Tuple value expression for a union can only contain a single field to be set.");
            return false;
        }

        for (const AstTupleParam& param : astValue.tuple)
        {
            const AstNode* field = node->children.Find([&](const AstNode& child)
            {
                const bool isType = child.kind == AstNode::Kind::Field || child.kind == AstNode::Kind::Group || child.kind == AstNode::Kind::Union;
                return isType && child.name == param.name;
            });

            if (!field)
            {
                m_context->AddError(param.location, "Field '{}' does not exist on type '{}'", param.name, node->name);
                return false;
            }

            if (field->kind == AstNode::Kind::Field)
            {
                if (!VerifyValue(field->field.type, *field->parent, param.value, scopeValue))
                    return false;
            }
            else
            {
                if (param.value.kind != AstExpression::Kind::Tuple)
                {
                    m_context->AddError(param.location, "Union and group values must be set using a tuple expression.");
                    return false;
                }

                if (!VerifyTupleValue(field, param.value, scopeValue))
                    return false;
            }
        }

        return true;
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

    uint16_t Verifier::GetArraySize(const AstExpression& ast, const AstNode& scope) const
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
}
