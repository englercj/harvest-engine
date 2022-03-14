// Copyright Chad Engler

#pragma once

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

    const AstNode* CompileContext::FindNode(TypeId id) const
    {
        auto it = m_typeIdMap.find(id);
        return it == m_typeIdMap.end() ? nullptr : it->second;
    }

    const AstNode* CompileContext::FindNode(const AstExpression& name, const AstNode& scope_) const
    {
        HE_ASSERT(name.kind == AstExpression::Kind::QualifiedName);

        const AstNode* scope = &scope_;
        const AstExpression* expr = name.qualified.names.Front();
        if (expr->kind == AstExpression::Kind::Identifier && expr->identifier.IsEmpty())
        {
            while (scope->parent)
                scope = scope->parent;
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

    const AstNode* CompileContext::FindNode(StringView name, const AstNode& scope) const
    {
        const AstNode* node = scope.children.Find([&](const AstNode& node) { return node.name == name; });
        if (node)
            return node;

        if (scope.parent)
            return FindNode(name, *scope.parent);

        // If we walked up to top scope, and it isn't our root, then search our imports
        if (scope.parent == nullptr)
        {
            for (const CompileContext* importCtx : m_imports)
            {
                HE_ASSERT(importCtx->m_fullyParsed);
                node = FindNode(name, importCtx->m_parser.Ast().root);
                if (node)
                    return node;
            }
        }

        return nullptr;
    }

    bool CompileContext::TrackTypeId(const AstNode& node)
    {
        auto result = m_typeIdMap.emplace(node.id, &node);
        return result.second;
    }

    bool CompileContext::TrackType(const TypeKey& key, const TypeValue& value)
    {
        auto result = m_typeMap.insert_or_assign(key, value);
        return result.second;
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
