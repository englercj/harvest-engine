// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/vector.h"
#include "he/schema/ast.h"
#include "he/schema/lexer.h"

#include "fmt/core.h"

#include <concepts>

namespace he::schema
{
    /// Transforms a Lexer's token stream into an AST.
    class Parser
    {
    public:
        struct ErrorInfo
        {
            he::String message;
            uint32_t line;
            uint32_t column;
        };

    public:
        Parser(Lexer& lexer);

        bool Parse();

        Span<const ErrorInfo> Errors() const { return m_errors; }

        AstFile& Ast() { return m_ast; }
        const AstFile& Ast() const { return m_ast; }

    private:
        using Pfn_DeclParser = bool (Parser::*)(AstNode&);
        struct DeclParser { const StringView keyword; Pfn_DeclParser handler; };

        bool At(Lexer::TokenType expected) const;
        bool AtEnd() const;
        bool AtIdentifier(StringView expected) const;

        bool Expect(Lexer::TokenType expected);

        bool Next();
        bool Next(Lexer::TokenType expected);

        bool NextDecl();
        bool NextDecl(Lexer::TokenType expected);

        void SkipStatement();
        void SkipBlock();

        bool TryConsume(Lexer::TokenType expected);
        bool TryConsumeKeyword(StringView expected);

        bool Consume(Lexer::TokenType expected);
        bool ConsumeTopLevelDecl();

        bool ConsumeAttributes(AstList<AstAttribute>& attributes);
        bool ConsumeId(TypeId& out);
        bool ConsumeType(AstExpression& type);
        bool ConsumeValue(AstExpression& value);
        bool ConsumeTypeParams(AstNode& node);
        bool ConsumeIdentifier(he::StringView& out);
        bool ConsumeQualifiedName(AstExpression& name);
        bool ConsumeOrdinal(uint16_t& out);

        bool ConsumeDecl(AstNode& parent, Span<const DeclParser> parsers);
        bool ConsumeDeclName(AstNode& node);

        bool ConsumeAliasDecl(AstNode& parent);
        bool ConsumeAttributeDecl(AstNode& parent);
        bool ConsumeConstantDecl(AstNode& parent);
        bool ConsumeEnumDecl(AstNode& parent);
        bool ConsumeImportDecl(AstNode& parent);
        bool ConsumeInterfaceDecl(AstNode& parent);
        bool ConsumeNamespaceDecl(AstNode& parent);
        bool ConsumeStructDecl(AstNode& parent);

        bool ConsumeMethodParams(AstNode& parent, AstMethodParams& params);
        bool ConsumeStructBlock(AstNode& parent);
        bool ConsumeStructField(AstNode& parent, AstList<AstNode>& list, bool requireOrdinal);

        template <typename... Args>
        void AddError(fmt::format_string<Args...> fmt, Args&&... args);
        void AddLexerError();

        template <std::integral T>
        bool DecodeInt(StringView s, T& out);

        template <typename T> T* AstCreate();
        template <typename T> T* AstCreate(AstList<T>& list);
        AstNode* CreateNode(AstNode& parent, AstNode::Kind kind);
        AstNode* CreateNode(AstNode& parent, AstNode::Kind kind, AstList<AstNode>& list);

    private:
        static DeclParser InterfaceMemberParsers[];
        static DeclParser StructMemberParsers[];
        static DeclParser TopLevelParsers[];

    private:
        Lexer& m_lexer;
        Lexer::Token m_token;
        AstFile m_ast;

        Vector<ErrorInfo> m_errors;
    };
}
