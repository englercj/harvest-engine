// Copyright Chad Engler

#include "compile_context.h"

#include "utf8_helpers.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/log.h"
#include "he/core/file.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/types.h"

namespace he::schema
{
    static const AstNode* FindNodeByName(StringView name, const AstNode& scope)
    {
        const AstNode* node = scope.children.Find([&](const AstNode& node) { return node.name == name; });

        if (node)
            return node;

        if (scope.parent)
            return FindNodeByName(name, *scope.parent);

        HE_ASSERT(scope.kind == AstNode::Kind::File);
        return nullptr;
    }

    static const AstNode* FindNodeInScope(AstListIterator<AstExpression> begin, AstListIterator<AstExpression> end, const AstNode& scopeStart)
    {
        if (begin == end)
            return nullptr;

        const AstNode* scope = &scopeStart;

        // If our name expression starts with a dot ('.') it will leave an empty identifier at
        // the beginning. This means that we should unwind and search from the top scope.
        if (begin->kind == AstExpression::Kind::Identifier && begin->identifier.IsEmpty())
        {
            while (scope->parent)
                scope = scope->parent;
            ++begin;
        }

        // Try to match the qualified name against the namespace of the current top scope.
        const AstNode* topNode = scope;
        while (topNode->parent)
            topNode = topNode->parent;

        HE_ASSERT(topNode->kind == AstNode::Kind::File);
        if (topNode->file.nameSpace.kind == AstExpression::Kind::QualifiedName)
        {
            for (const AstExpression& name : topNode->file.nameSpace.qualified.names)
            {
                if (!begin || begin == end || begin->kind != AstExpression::Kind::Identifier)
                    break;

                if (name.kind != AstExpression::Kind::Identifier)
                    break;

                if (begin->identifier != name.identifier)
                    break;

                ++begin;
            }
        }

        // Try to match the qualified name against a name in the current scope, walking up the
        // scope for each try.
        while (begin && begin != end && scope)
        {
            switch (begin->kind)
            {
                case AstExpression::Kind::Identifier:
                    scope = FindNodeByName(begin->identifier, *scope);
                    break;
                case AstExpression::Kind::Generic:
                    scope = FindNodeByName(begin->generic.name, *scope);
                    break;
                default:
                    HE_ASSERT(false, HE_MSG("Invalid expression in qualified name. This is a verifier bug."));
                    break;
            }

            ++begin;
        }

        return scope;
    }

    bool CompileContext::LoadFile()
    {
        Result r = File::ReadAll(m_input, m_path.Data());
        if (!HE_VERIFY(r))
        {
            HE_LOG_ERROR(schema_compiler, HE_MSG("Failed to read file"), HE_KV(result, r), HE_KV(path, m_path));
            return false;
        }
        m_input.PushBack('\0');
        return true;
    }

    const AstNode* CompileContext::FindNodeById(TypeId id) const
    {
        const AstNode** node = m_typeIdMap.Find(id);
        return node ? *node : nullptr;
    }

    const AstNode* CompileContext::FindNodeByName(const AstExpression& name, const AstNode& scope) const
    {
        HE_ASSERT(name.kind == AstExpression::Kind::QualifiedName);
        return FindNodeByName(name.qualified.names.begin(), name.qualified.names.end(), scope);
    }

    const AstNode* CompileContext::FindNodeByName(AstListIterator<AstExpression> begin, AstListIterator<AstExpression> end, const AstNode& scope) const
    {
        if (begin == end)
            return nullptr;

        // Look for the node in our scope first.
        const AstNode* node = FindNodeInScope(begin, end, scope);
        if (node)
            return node;

        // If we didn't find it in our scope check if any of the imports have it.
        for (const CompileContext* importCtx : m_imports)
        {
            // TODO: Bug here! This finds names in other namespaces of includes without them
            // being fully specified. For example my file with a namespace of `he.test1` can
            // find `Type` defined in an import with a namespace of `he.test2` because we don't
            // check for namespace equivalence or require fqn here.

            HE_ASSERT(importCtx->m_fullyParsed);
            const AstNode& root = importCtx->m_parser.Ast().root;
            HE_ASSERT(root.kind == AstNode::Kind::File);

            node = FindNodeInScope(begin, end, root);
            if (node)
                return node;
        }

        return nullptr;
    }

    bool CompileContext::TrackDecl(Declaration::Builder decl)
    {
        HE_ASSERT(HasFlag(decl.GetId(), TypeIdFlag));
        const auto result = m_declIdMap.Emplace(decl.GetId(), decl);
        return result.inserted;
    }

    bool CompileContext::TrackTypeId(const AstNode& node)
    {
        const auto result = m_typeIdMap.Emplace(node.id, &node);
        return result.inserted;
    }

    bool CompileContext::TrackType(const TypeKey& key, const TypeValue& value)
    {
        auto result = m_typeMap.EmplaceOrAssign(key, value);
        return result.inserted;
    }


    bool CompileContext::DecodeString(const AstExpression& ast, he::String& out)
    {
        HE_ASSERT(ast.kind == AstExpression::Kind::String);

        out.Clear();
        out.Reserve(ast.string.Size());

        bool decodedIsTrivial = true;
        int unicodeHighSurrogate = -1;

        const char* begin = ast.string.Begin();
        const char* end = ast.string.End();
        const char* s = begin;

        auto GetStrLoc = [&]() -> AstFileLocation
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

                                unicodeHighSurrogate = static_cast<int>(value);
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
                            break;
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
}
