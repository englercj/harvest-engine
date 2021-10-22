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

namespace he::schema
{
    /// The parser takes a schema text blob as input and processes it into a schema object.
    class Parser
    {
    public:
        struct ErrorInfo
        {
            ErrorInfo(Allocator& allocator) : message(allocator), file(allocator) {}

            String message;
            String file;
            uint32_t line;
            uint32_t column;
        };

    public:
        Parser(Allocator& allocator = Allocator::GetDefault());

        bool ParseFile(const char* path, Span<const char*> includeDirs);
        bool Parse(const char* src, Span<const char*> includeDirs);

        bool HasErrors() const { return !m_errors.IsEmpty(); }
        Span<const ErrorInfo> GetErrors() const { return m_errors; }

        const SchemaDef& GetSchema() const { return m_schema; }

    private:
        bool ParseFileInternal(const char* path, Span<const char*> includeDirs);
        bool OpenFile(File& file, const char* path);
        bool LoadFile(String& dst, const char* path, Span<const char*> includeDirs);
        bool ReadFile(String& dst, File& file, const char* path);

        template <typename T, typename U>
        const T* FindDef(const U& schema, StringView name) const;

        template <typename T>
        const T* FindDef(StringView name) const;

        const AttributeDef* FindAttributeDef(StringView name) const;
        const AliasDef* FindAliasDef(StringView name) const;
        const EnumDef* FindEnumDef(StringView name) const;
        const InterfaceDef* FindInterfaceDef(StringView name) const;
        const StructDef* FindStructDef(StringView name) const;
        const UnionDef* FindUnionDef(StringView name) const;

        const Type& ResolveType(const Type& type) const;

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

        bool Consume(Lexer::TokenType expected);
        bool ConsumeAttribute(Attribute& attributes);
        bool ConsumeAttributes(Vector<Attribute>& attributes);
        bool ConsumeKeyword(StringView expected);
        bool ConsumeIdentifier(String& out);
        bool ConsumeDottedIdentifier(String& out);
        bool ConsumeBool(bool& out);
        template <std::integral T>
        bool ConsumeIntegerRaw(T& out);
        template <std::integral T>
        bool ConsumeInteger(T& out);
        bool ConsumeFloatRaw(double& out);
        template <std::floating_point T>
        bool ConsumeFloat(T& out);
        bool ConsumeString(String& out);
        bool ConsumeTypeRaw(Type& type);
        bool ConsumeTypeParams(Vector<String>& params);
        bool ConsumeType(Type& type);
        bool ConsumeValue(BaseType type, Value& value);

        bool ParseImports(Span<const char*> includeDirs);
        bool ParseNamespace();
        bool ParseTopLevelStatement();

        bool ParseStruct(StructDef& def, uint32_t parentId);
        bool ParseStructBlock(StructDef& def);
        bool ParseStructStatement(StructDef& def);

        bool ParseUnion(UnionDef& def, uint32_t parentId);
        bool ParseUnionBlock(UnionDef& def);
        bool ParseUnionStatement(UnionDef& def);

        bool ParseField(FieldDef& def);

        bool ParseAttribute(AttributeDef& def);
        bool ParseAttributeTarget(AttributeDef& def);

        bool ParseEnum(EnumDef& def);
        bool ParseEnumBlock(EnumDef& def, bool isFlags);
        bool ParseEnumStatement(const EnumDef& enumDef, bool isFlags, EnumValueDef& def, EnumValueDef* lastDef);

        bool ParseInterface(InterfaceDef& def);
        bool ParseInterfaceBlock(InterfaceDef& def);
        bool ParseInterfaceStatement(InterfaceDef& def);
        bool ParseInterfaceMethod(MethodDef& def);
        bool ParseInterfaceMethodParam(MethodParamDef& def);

        bool ParseAlias(AliasDef& def);
        bool ParseConst(ConstDef& def);

        bool At(Lexer::TokenType expected) const;
        bool AtEnd() const;
        bool AtIdentifier(StringView expected) const;

        template <typename... Args>
        void AddError(fmt::format_string<Args...> fmt, Args&&... args);

        bool DecodeString();

    private:
        struct Import
        {
            Import(Allocator& allocator)
                : importPath(allocator)
                , schema(allocator)
            {}

            bool directImport{ false };
            String importPath;
            SchemaDef schema;
        };

        using AttributeDefMap = std::unordered_map<StringView, AttributeDef>;
        using BaseTypeMap = std::unordered_map<StringView, BaseType>;
        using ImportMap = std::unordered_map<StringView, Vector<Import>>;

    private:
        Allocator& m_allocator;

        SchemaDef m_schema;
        ImportMap m_imports{};
        uint32_t m_importDepth{ 0 };

        Lexer m_lexer;
        Lexer::Token m_token{};
        StringView m_fileName{ "schemac" };

        Vector<ErrorInfo> m_errors;
        String m_decodedString;

        AttributeDefMap m_builtinAttributes{};
        BaseTypeMap m_builtinTypes{};
    };
}
