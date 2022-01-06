// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/file.h"
#include "he/core/hash.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/vector.h"
#include "he/schema/lexer.h"
#include "he/schema/schema.h"

#include "fmt/core.h"

#include <concepts>
#include <unordered_map>
#include <unordered_set>

namespace he::schema
{
    /// The parser takes a schema text blob as input and processes it into a schema object.
    class Parser
    {
    public:
        struct ErrorInfo
        {
            String message;
            String file;
            uint32_t line;
            uint32_t column;
        };

    public:
        Parser();

        bool ParseFile(const char* path, Span<const char*> includeDirs);
        bool Parse(const char* src, Span<const char*> includeDirs);

        bool HasErrors() const { return !m_errors.IsEmpty(); }
        Span<const ErrorInfo> GetErrors() const { return m_errors; }

        const SchemaFile& GetSchema() const { return m_schema; }

    private:
        bool ParseFileInternal(const char* path, Span<const char*> includeDirs);
        bool OpenFile(File& file, const char* path);
        bool LoadFile(String& dst, const char* path, Span<const char*> includeDirs);
        bool ReadFile(String& dst, File& file, const char* path);

        bool At(Lexer::TokenType expected) const;
        bool AtEnd() const;
        bool AtIdentifier(StringView expected) const;

        bool Expect(Lexer::TokenType expected);

        void Next();
        bool Next(Lexer::TokenType expected);

        void NextDecl();
        bool NextDecl(Lexer::TokenType expected);

        bool SkipWhitespace(Lexer::TokenType expected);
        void SkipStatement();
        void SkipBlock();

        bool TryConsume(Lexer::TokenType expected);
        bool TryConsumeKeyword(StringView expected);
        bool TryConsumeId(TypeId& out);

        bool Consume(Lexer::TokenType expected);
        bool ConsumeFileId();
        bool ConsumeImports(Span<const char*> includeDirs);
        bool ConsumeNamespace();
        bool ConsumeTopLevelDecl();

        bool ConsumeAttributes(Vector<Attribute>& attributes);
        bool ConsumeArraySize(Type& type);
        bool ConsumeType(Type& type, const Declaration* scope);
        bool ConsumeValue(const Type& type, Value& value);
        bool ConsumeTypeParams(Vector<String>& params);
        bool ConsumeIdentifier(String& out);
        bool ConsumeOrdinal(uint16_t& out);
        bool ConsumeBlob(Vector<uint8_t>& out);
        bool ConsumeString(String& out);
        bool ConsumeBool(bool& out);
        template <std::integral T> bool ConsumeInt(T& out);
        template <std::floating_point T> bool ConsumeFloat(T& out);

        bool ConsumeDeclName(Declaration& decl);

        bool ConsumeAttributeDecl(Declaration& parent);
        bool ConsumeConstDecl(Declaration& parent);
        bool ConsumeEnumDecl(Declaration& parent);
        bool ConsumeInterfaceDecl(Declaration& parent);
        bool ConsumeStructDecl(Declaration& parent);
        bool ConsumeStructBlock(Declaration& decl);

        bool ConsumeField(Field& field, const Declaration& scope, bool requireOrdinal);
        bool ConsumeTupleStruct(Declaration& decl);

        template <typename... Args>
        void AddError(fmt::format_string<Args...> fmt, Args&&... args);
        template <typename... Args>
        void AddDeclError(const Declaration& decl, fmt::format_string<Args...> fmt, Args&&... args);
        void AddLexerError();

        bool DecodeString();
        template <std::integral T> bool DecodeInt(StringView s, T& out);

        struct TypeParamRef
        {
            const Declaration* scope{ nullptr };
            uint32_t index{ 0 };
        };
        TypeParamRef FindTypeParam(StringView name, const Declaration& scope);

        const Declaration* FindDecl(StringView name) const;
        const Declaration* FindDecl(StringView name, const Declaration& scope) const;
        const Declaration* FindDecl(TypeId id, const SchemaFile* schema = nullptr) const;
        const Declaration* FindDecl(TypeId id, const Declaration& scope) const;

        const Declaration* FindForwardDecl(StringView name) const;
        const Declaration* FindForwardDecl(StringView name, const Declaration& scope) const;
        const Declaration* FindForwardDecl(TypeId id, const SchemaFile* schema = nullptr) const;
        const Declaration* FindForwardDecl(TypeId id, const Declaration& scope) const;

        bool StoreDeclId(Declaration& decl);

        bool ValidateAndLayoutStruct(Declaration& decl);
        bool ValidateStructFieldNames(const Declaration& decl);

    private:
        // schema ids are already hashes, just use it as-is
        struct SchemaIdHasher
        {
            size_t operator()(TypeId id) const { return static_cast<size_t>(id); }
        };

        using ImportNamespaceMap = std::unordered_map<StringView, Vector<SchemaFile*>>;
        using BuiltinTypeMap = std::unordered_map<StringView, TypeKind>;
        using NameMap = std::unordered_map<TypeId, String, SchemaIdHasher>;

    private:
        SchemaFile m_schema;
        ImportNamespaceMap m_importsByNamespace;
        uint32_t m_importDepth{ 0 };

        Lexer m_lexer;
        Lexer::Token m_token;
        StringView m_fileName{ "schemac" };

        Vector<ErrorInfo> m_errors;
        String m_scratchString;

        std::unordered_set<TypeId> m_forwardIds;

        NameMap m_nameMap;
        BuiltinTypeMap m_builtinTypeMap;
    };
}
