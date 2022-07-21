// Copyright Chad Engler

#include "parser.h"

#include "compile_context.h"
#include "keywords.h"

#include "he/core/appender.h"
#include "he/core/assert.h"
#include "he/core/enum_fmt.h"
#include "he/core/hash.h"
#include "he/core/path.h"
#include "he/core/random.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view.h"
#include "he/core/string_view_fmt.h"
#include "he/core/utils.h"
#include "he/schema/schema.h"

#include "fmt/core.h"

#include <limits>

namespace he::schema
{
    Parser::DeclParser Parser::InterfaceMemberParsers[] =
    {
        { KW_Alias, &Parser::ConsumeAliasDecl },
        { KW_Const, &Parser::ConsumeConstantDecl },
        { KW_Enum, &Parser::ConsumeEnumDecl },
        { KW_Interface, &Parser::ConsumeInterfaceDecl },
        { KW_Struct, &Parser::ConsumeStructDecl },
    };

    Parser::DeclParser Parser::StructMemberParsers[] =
    {
        { KW_Alias, &Parser::ConsumeAliasDecl },
        { KW_Attribute, &Parser::ConsumeAttributeDecl },
        { KW_Const, &Parser::ConsumeConstantDecl },
        { KW_Enum, &Parser::ConsumeEnumDecl },
        { KW_Interface, &Parser::ConsumeInterfaceDecl },
        { KW_Struct, &Parser::ConsumeStructDecl },
    };

    Parser::DeclParser Parser::TopLevelParsers[] =
    {
        { KW_Alias, &Parser::ConsumeAliasDecl },
        { KW_Attribute, &Parser::ConsumeAttributeDecl },
        { KW_Const, &Parser::ConsumeConstantDecl },
        { KW_Enum, &Parser::ConsumeEnumDecl },
        { KW_Import, &Parser::ConsumeImportDecl },
        { KW_Interface, &Parser::ConsumeInterfaceDecl },
        { KW_Namespace, &Parser::ConsumeNamespaceDecl },
        { KW_Struct, &Parser::ConsumeStructDecl },
    };

    bool Parser::Parse(const char* src, CompileContext& ctx)
    {
        m_context = &ctx;
        m_lexer.Reset(src);

        if (!NextDecl())
            return false;

        m_ast.root.kind = AstNode::Kind::File;
        m_ast.root.file.nameSpace.kind = AstExpression::Kind::Unknown;
        m_ast.root.file.imports.Clear();

        // Consume the file ID
        if (!At(Lexer::TokenType::Ordinal))
        {
            TypeId id = 0;
            const bool idResult = GetSecureRandomBytes(reinterpret_cast<uint8_t*>(&id), sizeof(id));
            HE_ASSERT(idResult);
            HE_UNUSED(idResult);

            id |= TypeIdFlag;
            AddError("The first non-comment line of a schema file must be the file's unique ID. Add this line to the top of your file: @{:#018x};", id);
            return false;
        }

        if (!ConsumeId(m_ast.root.id))
            return false;

        if (!Consume(Lexer::TokenType::Semicolon))
            return false;

        bool valid = true;

        // Consume top level declarations
        while (!AtEnd())
        {
            while (TryConsume(Lexer::TokenType::Semicolon)) {}

            if (!ConsumeTopLevelDecl())
            {
                valid = false;

                if (At(Lexer::TokenType::Error))
                    break;

                SkipStatement();

                if (At(Lexer::TokenType::Semicolon))
                {
                    AddError("Unmatched curly bracket ('}}')");
                }
            }
        }

        return valid;
    }

    bool Parser::At(Lexer::TokenType expected) const
    {
        return m_token.type == expected;
    }

    bool Parser::AtEnd() const
    {
        return At(Lexer::TokenType::Eof);
    }

    bool Parser::AtIdentifier(StringView expected) const
    {
        return At(Lexer::TokenType::Identifier) && m_token.text == expected;
    }

    bool Parser::Expect(Lexer::TokenType expected)
    {
        if (!At(expected))
        {
            if (At(Lexer::TokenType::Error))
                AddLexerError();
            else
                AddError("Unexpected token {}, expected {}", m_token.type, expected);
            return false;
        }

        return true;
    }

    bool Parser::Next()
    {
        m_token = m_lexer.NextToken();

        if (m_token.type == Lexer::TokenType::Error)
        {
            AddLexerError();
            return false;
        }

        return true;
    }

    bool Parser::Next(Lexer::TokenType expected)
    {
        Next();
        return Expect(expected);
    }

    bool Parser::NextDecl()
    {
        do
        {
            if (!Next())
                return false;
        } while (At(Lexer::TokenType::Comment));

        return true;
    }

    bool Parser::NextDecl(Lexer::TokenType expected)
    {
        NextDecl();
        return Expect(expected);
    }

    void Parser::SkipStatement()
    {
        while (!AtEnd())
        {
            if (At(Lexer::TokenType::Semicolon))
            {
                NextDecl();
                return;
            }

            if (At(Lexer::TokenType::OpenCurlyBracket))
            {
                SkipBlock();
                return;
            }

            if (At(Lexer::TokenType::CloseCurlyBracket))
            {
                NextDecl();
                return;
            }

            Next();
        }
    }

    void Parser::SkipBlock()
    {
        while (!AtEnd() && !At(Lexer::TokenType::CloseCurlyBracket))
            Next();

        if (At(Lexer::TokenType::CloseCurlyBracket))
        {
            NextDecl();
        }
    }

    bool Parser::TryConsume(Lexer::TokenType expected)
    {
        if (!At(expected))
            return false;

        NextDecl();
        return true;
    }

    bool Parser::TryConsumeKeyword(StringView expected)
    {
        if (!At(Lexer::TokenType::Identifier))
            return false;

        if (m_token.text != expected)
            return false;

        NextDecl();
        return true;
    }

    bool Parser::Consume(Lexer::TokenType expected)
    {
        if (!Expect(expected))
            return false;

        NextDecl();
        return true;
    }

    bool Parser::ConsumeTopLevelDecl()
    {
        if (At(Lexer::TokenType::Dollar))
        {
            if (!ConsumeAttributes(m_ast.root.attributes))
                return false;

            if (!Consume(Lexer::TokenType::Semicolon))
                return false;

            return false;
        }

        return ConsumeDecl(m_ast.root, TopLevelParsers);
    }

    bool Parser::ConsumeAttributes(AstList<AstAttribute>& attributes)
    {
        while (TryConsume(Lexer::TokenType::Dollar))
        {
            AstAttribute* attr = AstCreate(attributes);
            if (!ConsumeQualifiedName(attr->name))
                return false;

            if (TryConsume(Lexer::TokenType::OpenParens))
            {
                if (!ConsumeValue(attr->value))
                    return false;

                if (!Consume(Lexer::TokenType::CloseParens))
                    return false;
            }
        }

        return true;
    }

    bool Parser::ConsumeId(TypeId& out)
    {
        if (!Expect(Lexer::TokenType::Ordinal))
            return false;

        const StringView str{ m_token.text.begin() + 1, m_token.text.end() };
        if (!DecodeInt(str, out))
            return false;

        if (!HasFlag(out, TypeIdFlag))
        {
            AddError("Invalid unique ID. Did you accidentally use an ordinal as an ID?");
            return false;
        }

        NextDecl();
        return true;
    }

    bool Parser::ConsumeType(AstExpression& type)
    {
        type.location.line = m_token.line;
        type.location.column = m_token.column;

        if (!ConsumeQualifiedName(type))
            return false;

        while (TryConsume(Lexer::TokenType::OpenSquareBracket))
        {
            HE_ASSERT(type.link.next == nullptr && type.link.prev == nullptr); // cannot be in a list
            AstExpression* elementType = AstCreate<AstExpression>();
            MemCopy(elementType, &type, sizeof(AstExpression));

            AstExpression* size = nullptr;

            if (!At(Lexer::TokenType::CloseSquareBracket))
            {
                size = AstCreate<AstExpression>();
                if (!ConsumeValue(*size))
                    return false;
            }

            if (!Consume(Lexer::TokenType::CloseSquareBracket))
                return false;

            if (size)
            {
                type.kind = AstExpression::Kind::Array;
                type.array.elementType = elementType;
                type.array.size = size;
            }
            else
            {
                type.kind = AstExpression::Kind::List;
                type.list.elementType = elementType;
            }
        }

        return true;
    }

    bool Parser::ConsumeValue(AstExpression& value)
    {
        value.location.line = m_token.line;
        value.location.column = m_token.column;

        switch (m_token.type)
        {
            case Lexer::TokenType::Blob:
                value.kind = AstExpression::Kind::Blob;
                value.blob = m_token.text;
                NextDecl();
                return true;

            case Lexer::TokenType::Float:
                value.kind = AstExpression::Kind::Float;
                value.floatingPoint = m_token.text.ToFloat<double>();
                NextDecl();
                return true;

            case Lexer::TokenType::OpenSquareBracket:
            {
                value.kind = AstExpression::Kind::Sequence;
                if (!Consume(Lexer::TokenType::OpenSquareBracket))
                    return false;

                do
                {
                    if (At(Lexer::TokenType::CloseSquareBracket))
                        break;

                    AstExpression* item = AstCreate(value.sequence);
                    if (!ConsumeValue(*item))
                        return false;
                } while (TryConsume(Lexer::TokenType::Comma));

                if (!Consume(Lexer::TokenType::CloseSquareBracket))
                    return false;

                return true;
            }

            case Lexer::TokenType::Dot:
            case Lexer::TokenType::Identifier:
                return ConsumeQualifiedName(value);

            case Lexer::TokenType::Integer:
            {
                if (m_token.text[0] == '-')
                {
                    value.kind = AstExpression::Kind::SignedInt;
                    if (!DecodeInt(m_token.text, value.signedInt))
                        return false;
                }
                else
                {
                    value.kind = AstExpression::Kind::UnsignedInt;
                    if (!DecodeInt(m_token.text, value.unsignedInt))
                        return false;
                }

                NextDecl();
                return true;
            }

            case Lexer::TokenType::String:
                value.kind = AstExpression::Kind::String;
                value.string = m_token.text;
                NextDecl();
                return true;

            case Lexer::TokenType::OpenCurlyBracket:
            {
                value.kind = AstExpression::Kind::Tuple;
                if (!Consume(Lexer::TokenType::OpenCurlyBracket))
                    return false;

                do
                {
                    if (At(Lexer::TokenType::CloseCurlyBracket))
                        break;

                    AstTupleParam* param = AstCreate(value.tuple);
                    if (!ConsumeIdentifier(param->name))
                        return false;

                    if (!Consume(Lexer::TokenType::Equals))
                        return false;

                    if (!ConsumeValue(param->value))
                        return false;
                } while (TryConsume(Lexer::TokenType::Comma));

                if (!Consume(Lexer::TokenType::CloseCurlyBracket))
                    return false;

                return true;
            }

            default:
                break;
        }

        AddError("Unexpected token {}, expected a value.", m_token.type);
        return false;
    }

    bool Parser::ConsumeTypeParams(AstNode& node)
    {
        if (TryConsume(Lexer::TokenType::OpenAngleBracket))
        {
            do
            {
                if (At(Lexer::TokenType::CloseAngleBracket))
                    break;

                AstTypeParam* param = AstCreate(node.typeParams);
                if (!ConsumeIdentifier(param->name))
                    return false;
            } while (TryConsume(Lexer::TokenType::Comma));

            if (!Consume(Lexer::TokenType::CloseAngleBracket))
                return false;

            if (node.typeParams.IsEmpty())
            {
                AddError("Generic type parameters cannot be empty.");
                return false;
            }
        }

        return true;
    }

    bool Parser::ConsumeIdentifier(he::StringView& out)
    {
        if (!Expect(Lexer::TokenType::Identifier))
            return false;

        out = m_token.text;

        NextDecl();
        return true;
    }

    bool Parser::ConsumeQualifiedName(AstExpression& name)
    {
        name.kind = AstExpression::Kind::QualifiedName;
        name.location.line = m_token.line;
        name.location.column = m_token.column;

        // leading dot inserts an empty expression into the qualified name so we know this is a "global" name
        if (TryConsume(Lexer::TokenType::Dot))
        {
            AstExpression* empty = AstCreate(name.qualified.names);
            empty->kind = AstExpression::Kind::Identifier;
            empty->identifier = {};
        }

        do
        {
            StringView identifier;
            if (!ConsumeIdentifier(identifier))
                return false;

            AstExpression* child = AstCreate(name.qualified.names);
            if (TryConsume(Lexer::TokenType::OpenAngleBracket))
            {
                child->kind = AstExpression::Kind::Generic;
                child->generic.name = identifier;

                do
                {
                    if (At(Lexer::TokenType::CloseAngleBracket))
                        break;

                    AstExpression* param = AstCreate(child->generic.params);
                    if (!ConsumeType(*param))
                        return false;
                } while (TryConsume(Lexer::TokenType::Comma));

                if (!Consume(Lexer::TokenType::CloseAngleBracket))
                    return false;
            }
            else
            {
                child->kind = AstExpression::Kind::Identifier;
                child->identifier = identifier;
            }
        } while (TryConsume(Lexer::TokenType::Dot));

        return true;
    }

    bool Parser::ConsumeOrdinal(uint16_t& out)
    {
        if (!Expect(Lexer::TokenType::Ordinal))
            return false;

        const StringView str{ m_token.text.begin() + 1, m_token.text.end() };
        if (!DecodeInt(str, out))
            return false;

        NextDecl();
        return true;
    }

    bool Parser::ConsumeDecl(AstNode& parent, Span<const DeclParser> parsers)
    {
        if (!Expect(Lexer::TokenType::Identifier))
            return false;

        for (uint32_t i = 0; i < parsers.Size(); ++i)
        {
            const DeclParser& parser = parsers[i];
            if (TryConsumeKeyword(parser.keyword))
            {
                return (this->*parser.handler)(parent);
            }
        }

        AddError("Declaration '{}' is not allowed in this scope ({}).", m_token.text, parent.kind);
        return false;
    }

    bool Parser::ConsumeDeclName(AstNode& node)
    {
        if (!ConsumeIdentifier(node.name))
            return false;

        if (At(Lexer::TokenType::Ordinal))
        {
            if (!ConsumeId(node.id))
                return false;
        }
        else
        {
            node.id = MakeTypeId(node.name, node.parent->id);
        }

        HE_ASSERT(node.id != 0);
        return true;
    }

    bool Parser::ConsumeAliasDecl(AstNode& parent)
    {
        AstNode* node = CreateNode(parent, AstNode::Kind::Alias);

        if (!ConsumeDeclName(*node))
            return false;

        if (!Consume(Lexer::TokenType::Equals))
            return false;

        if (!ConsumeType(node->alias.target))
            return false;

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ConsumeAttributeDecl(AstNode& parent)
    {
        AstNode* node = CreateNode(parent, AstNode::Kind::Attribute);

        if (!ConsumeDeclName(*node))
            return false;

        if (!Consume(Lexer::TokenType::OpenParens))
            return false;

        do
        {
            if (At(Lexer::TokenType::CloseParens))
                break;

            if (TryConsume(Lexer::TokenType::Asterisk))
            {
                node->attribute.targetsAttribute = true;
                node->attribute.targetsConstant = true;
                node->attribute.targetsEnum = true;
                node->attribute.targetsEnumerator = true;
                node->attribute.targetsField = true;
                node->attribute.targetsFile = true;
                node->attribute.targetsInterface = true;
                node->attribute.targetsMethod = true;
                node->attribute.targetsParameter = true;
                node->attribute.targetsStruct = true;
                break;
            }

            StringView kw;
            if (!ConsumeIdentifier(kw))
                return false;

            if (kw == KW_Attribute)
                node->attribute.targetsAttribute = true;
            else if (kw == KW_Const)
                node->attribute.targetsConstant= true;
            else if (kw == KW_Enum)
                node->attribute.targetsEnum = true;
            else if (kw == KW_Enumerator)
                node->attribute.targetsEnumerator = true;
            else if (kw == KW_Field)
                node->attribute.targetsField = true;
            else if (kw == KW_File)
                node->attribute.targetsFile = true;
            else if (kw == KW_Interface)
                node->attribute.targetsInterface = true;
            else if (kw == KW_Method)
                node->attribute.targetsMethod = true;
            else if (kw == KW_Parameter)
                node->attribute.targetsParameter = true;
            else if (kw == KW_Struct)
                node->attribute.targetsStruct = true;
            else
            {
                AddError("Expected a valid attribute target. Must be one of: attribute, const, enum, enumerator, field, file, interface, method, parameter, or struct");
                return false;
            }
        } while (TryConsume(Lexer::TokenType::Comma));

        if (!Consume(Lexer::TokenType::CloseParens))
            return false;

        if (TryConsume(Lexer::TokenType::Colon))
        {
            if (!ConsumeType(node->attribute.type))
                return false;
        }

        if (!ConsumeAttributes(node->attributes))
            return false;

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ConsumeConstantDecl(AstNode& parent)
    {
        AstNode* node = CreateNode(parent, AstNode::Kind::Constant);

        if (!ConsumeDeclName(*node))
            return false;

        if (!Consume(Lexer::TokenType::Colon))
            return false;

        if (!ConsumeType(node->constant.type))
            return false;

        if (!Consume(Lexer::TokenType::Equals))
            return false;

        if (!ConsumeValue(node->constant.value))
            return false;

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ConsumeEnumDecl(AstNode& parent)
    {
        AstNode* node = CreateNode(parent, AstNode::Kind::Enum);

        if (!ConsumeDeclName(*node))
            return false;

        if (!ConsumeAttributes(node->attributes))
            return false;

        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in enum definition (missing '}}')");
                return false;
            }

            AstNode* child = CreateNode(*node, AstNode::Kind::Enumerator);

            if (!ConsumeIdentifier(child->name))
                return false;

            uint16_t ordinal = 0;
            if (!ConsumeOrdinal(ordinal))
                return false;
            child->id = ordinal;

            if (!ConsumeAttributes(child->attributes))
                return false;

            if (!Consume(Lexer::TokenType::Semicolon))
                return false;
        }

        return true;
    }

    bool Parser::ConsumeImportDecl(AstNode& parent)
    {
        HE_ASSERT(parent.kind == AstNode::Kind::File);

        if (!Expect(Lexer::TokenType::String))
            return false;

        AstExpression* ex = AstCreate(parent.file.imports);
        ex->kind = AstExpression::Kind::String;
        ex->string = m_token.text;

        NextDecl();
        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ConsumeInterfaceDecl(AstNode& parent)
    {
        AstNode* node = CreateNode(parent, AstNode::Kind::Interface);

        if (!ConsumeDeclName(*node))
            return false;

        if (!ConsumeTypeParams(*node))
            return false;

        if (TryConsumeKeyword(KW_Extends))
        {
            if (!ConsumeType(node->interface.super))
                return false;
        }

        if (!ConsumeAttributes(node->attributes))
            return false;

        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in interface declaration (missing '}}')");
                return false;
            }

            // ignore empty statements
            if (TryConsume(Lexer::TokenType::Semicolon))
                return true;

            Lexer::Token nextToken = m_lexer.PeekNextToken();

            if (nextToken.type == Lexer::TokenType::Identifier)
            {
                if (!ConsumeDecl(*node, InterfaceMemberParsers))
                    return false;

                continue;
            }

            AstNode* method = CreateNode(*node, AstNode::Kind::Method);
            if (!ConsumeIdentifier(method->name))
                return false;

            if (!ConsumeTypeParams(*method))
                return false;

            uint16_t ordinal = 0;
            if (!ConsumeOrdinal(ordinal))
                return false;
            method->id = ordinal;

            if (!ConsumeMethodParams(*method, method->method.params))
                return false;

            if (!At(Lexer::TokenType::Semicolon))
            {
                if (!Consume(Lexer::TokenType::Arrow))
                    return false;

                if (!ConsumeMethodParams(*method, method->method.results))
                    return false;
            }

            if (!Consume(Lexer::TokenType::Semicolon))
                return false;
        }

        return true;
    }

    bool Parser::ConsumeMethodParams(AstNode& parent, AstMethodParams& params)
    {
        if (TryConsume(Lexer::TokenType::Colon))
        {
            params.kind = AstMethodParams::Kind::Type;
            params.stream = TryConsumeKeyword(KW_Stream);
            if (!ConsumeType(params.type))
                return false;

            return true;
        }

        params.kind = AstMethodParams::Kind::Fields;
        params.stream = TryConsumeKeyword(KW_Stream);

        if (!Consume(Lexer::TokenType::OpenParens))
            return false;

        if (!At(Lexer::TokenType::CloseParens))
        {
            do
            {
                if (At(Lexer::TokenType::CloseParens))
                    break;

                if (!ConsumeStructField(parent, params.fields, false))
                    return false;
            }
            while (TryConsume(Lexer::TokenType::Comma));
        }

        return Consume(Lexer::TokenType::CloseParens);
    }

    bool Parser::ConsumeNamespaceDecl(AstNode& parent)
    {
        HE_ASSERT(parent.kind == AstNode::Kind::File);
        AstExpression& nameSpace = parent.file.nameSpace;

        if (nameSpace.kind != AstExpression::Kind::Unknown)
        {
            AddError("A file can only contain a single namespace name.");
            return false;
        }

        if (!ConsumeQualifiedName(nameSpace))
            return false;

        if (nameSpace.qualified.names.IsEmpty())
        {
            AddError("Namespace cannot be specified as empty");
            return false;
        }

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ConsumeStructDecl(AstNode& parent)
    {
        AstNode* node = CreateNode(parent, AstNode::Kind::Struct);

        if (!ConsumeDeclName(*node))
            return false;

        if (!ConsumeTypeParams(*node))
            return false;

        if (!ConsumeAttributes(node->attributes))
            return false;

        return ConsumeStructBlock(*node);
    }

    bool Parser::ConsumeStructBlock(AstNode& node)
    {
        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in struct declaration (missing '}}')");
                return false;
            }

            // ignore empty statements
            if (TryConsume(Lexer::TokenType::Semicolon))
                continue;

            const Lexer::Token nextToken = m_lexer.PeekNextToken();

            switch (nextToken.type)
            {
                // Normal field
                case Lexer::TokenType::Ordinal:
                {
                    if (!ConsumeStructField(node, node.children, true))
                        return false;

                    if (!Consume(Lexer::TokenType::Semicolon))
                        return false;

                    break;
                }

                // Union or group field
                case Lexer::TokenType::Colon:
                {
                    StringView name;
                    if (!ConsumeIdentifier(name))
                        return false;

                    if (!Consume(Lexer::TokenType::Colon))
                        return false;

                    if (!Expect(Lexer::TokenType::Identifier))
                        return false;

                    const bool isGroup = m_token.text == KW_Group;
                    const bool isUnion = m_token.text == KW_Union;
                    if (!isGroup && !isUnion)
                    {
                        AddError("Unexpected identifier {}, expected group or union keyword.", name);
                        return false;
                    }
                    NextDecl();

                    AstNode* group = CreateNode(node, isUnion ? AstNode::Kind::Union : AstNode::Kind::Group);
                    group->name = name;

                    if (!ConsumeStructBlock(*group))
                        return false;

                    break;
                }

                // Nested declaration (const, enum, or struct)
                case Lexer::TokenType::Identifier:
                {
                    if (node.kind == AstNode::Kind::Group || node.kind == AstNode::Kind::Union)
                    {
                        AddError("Only fields, groups, and unions are allowed in group and union blocks");
                        return false;
                    }

                    if (!ConsumeDecl(node, StructMemberParsers))
                        return false;

                    break;
                }

                default:
                    if (At(Lexer::TokenType::Error))
                        AddLexerError();
                    else
                        AddError("Unexpected token {}, expected field ordinal, union, group, or nested declaration.", nextToken.type);
                    return false;
            }
        }

        return true;
    }

    bool Parser::ConsumeStructField(AstNode& parent, AstList<AstNode>& list, bool requireOrdinal)
    {
        AstNode* node = CreateNode(parent, AstNode::Kind::Field, list);

        if (!ConsumeIdentifier(node->name))
            return false;

        // Ordinals are optional for tuple struct syntax
        if (requireOrdinal || At(Lexer::TokenType::Ordinal))
        {
            uint16_t ordinal;
            if (!ConsumeOrdinal(ordinal))
                return false;
            node->id = ordinal;
        }

        if (!Consume(Lexer::TokenType::Colon))
            return false;

        if (!ConsumeType(node->field.type))
            return false;

        if (TryConsume(Lexer::TokenType::Equals))
        {
            if (!ConsumeValue(node->field.defaultValue))
                return false;
        }

        return ConsumeAttributes(node->attributes);
    }

    template <typename... Args>
    void Parser::AddError(fmt::format_string<Args...> fmt, Args&&... args)
    {
        m_context->AddError(m_token.line, m_token.column, fmt, Forward<Args>(args)...);
    }

    void Parser::AddLexerError()
    {
        HE_ASSERT(m_token.type == Lexer::TokenType::Error);
        m_context->AddError(m_token.line, m_token.column, fmt::runtime(m_token.error));
    }

    template <std::integral T>
    bool Parser::DecodeInt(StringView s, T& out)
    {
        const char* begin = s.begin();
        const char* end = s.end();
        const bool isSigned = *begin == '-';

        if (isSigned)
            ++begin;

        int32_t base = 10;
        if (*begin == '0' && s.Size() > 1)
        {
            ++begin;
            switch (*begin)
            {
                case 'x':
                    ++begin;
                    base = 16;
                    break;
                case 'o':
                    ++begin;
                    base = 8;
                    break;
                case 'b':
                    ++begin;
                    base = 2;
                    break;
            }
        }

        using LargestType = std::conditional_t<std::is_signed_v<T>, int64_t, uint64_t>;
        LargestType value = he::String::ToInteger<T>(begin, &end, base);

        if (isSigned)
        {
            if constexpr (std::is_signed_v<T>)
            {
                value *= -1;
            }
            else
            {
                // Disallow assignment of a signed value to an unsigned type, except for the
                // special case of allowing `-1` which is a common "set all bits" value.
                if (value == 1)
                {
                    value = static_cast<LargestType>(-1);
                }
                else
                {
                    AddError("Illegal negative value assigned to unsigned integer type");
                    return false;
                }
            }
        }

        if (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::min())
        {
            AddError("Integer value out of range");
            return false;
        }

        out = static_cast<T>(value);
        return true;
    }

    template <typename T>
    T* Parser::AstCreate()
    {
        T* item = m_ast.allocator.New<T>();
        item->location.line = m_token.line;
        item->location.column = m_token.column;
        return item;
    }

    template <typename T>
    T* Parser::AstCreate(AstList<T>& list)
    {
        T* item = AstCreate<T>();
        list.PushBack(item);
        return item;
    }

    AstNode* Parser::CreateNode(AstNode& parent, AstNode::Kind kind)
    {
        return CreateNode(parent, kind, parent.children);
    }

    AstNode* Parser::CreateNode(AstNode& parent, AstNode::Kind kind, AstList<AstNode>& list)
    {
        AstNode* node = AstCreate(list);
        node->kind = kind;
        node->parent = &parent;
        //node->docComment = ""; // TODO
        return node;
    }
}
