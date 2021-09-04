// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/file.h"
#include "he/core/hash.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/vector.h"
#include "he/schema/lexer.h"
#include "he/schema/schema.h"

#include "fmt/core.h"

#include <unordered_map>

namespace he::schema
{
    /// The parser takes a schema text blob as input and processes it into a schema object.
    class Parser
    {
    public:
        struct ErrorInfo
        {
            ErrorInfo(Allocator& allocator) : message(allocator) {}

            uint32_t line;
            uint32_t column;
            String message;
        };

    public:
        Parser(Allocator& allocator);

        bool ParseFile(const char* path, Span<StringView> includeDirs);
        bool Parse(const char* src, Span<StringView> includeDirs);

        bool HasErrors() const { return !m_errors.IsEmpty(); }
        Span<const ErrorInfo> GetErrors() const { return m_errors; }

        const SchemaDef& GetSchema() const { return m_schema; }

    private:
        bool ParseFileInternal(const char* path, Span<StringView> includeDirs);
        bool OpenFile(File& file, const char* path);
        bool LoadFile(String& dst, const char* path, Span<StringView> includeDirs);
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
        template <typename T, HE_REQUIRES(std::is_integral_v<T>)>
        bool ConsumeIntegerRaw(T& out);
        template <typename T, HE_REQUIRES(std::is_integral_v<T>)>
        bool ConsumeInteger(T& out);
        bool ConsumeFloatRaw(double& out);
        template <typename T, HE_REQUIRES(std::is_floating_point_v<T>)>
        bool ConsumeFloat(T& out);
        bool ConsumeString(String& out);
        bool ConsumeTypeRaw(Type& type);
        bool ConsumeTypeParams(Vector<String>& params);
        bool ConsumeType(Type& type);
        bool ConsumeValue(BaseType type, Value& value);

        bool ParseImports(Span<StringView> includeDirs);
        bool ParseNamespace();
        bool ParseTopLevelStatement();

        bool ParseStruct(StructDef& def);
        bool ParseStructBlock(StructDef& def);
        bool ParseStructStatement(StructDef& def);
        bool ParseStructField(FieldDef& def);

        bool ParseAttribute(AttributeDef& def);
        bool ParseAttributeTarget(AttributeDef& def);

        bool ParseEnum(EnumDef& def);
        bool ParseEnumBlock(EnumDef& def);
        bool ParseEnumStatement(const EnumDef& enumDef, EnumValueDef& def, EnumValueDef* lastDef);

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
        struct StringViewHasher
        {
            size_t operator()(const StringView& s) const
            {
            #if HE_CPU_64_BIT
                return FNV64::HashData(s.Data(), s.Size());
            #else
                return FNV32::HashData(s.Data(), s.Size());
            #endif
            }
        };

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

        using BaseTypeMap = std::unordered_map<StringView, BaseType, StringViewHasher>;
        using ImportMap = std::unordered_map<StringView, Vector<Import>, StringViewHasher>;

    private:
        Allocator& m_allocator;

        SchemaDef m_schema;
        ImportMap m_imports;
        uint32_t m_importDepth{ 0 };

        Lexer m_lexer;
        Lexer::Token m_token{};

        Vector<ErrorInfo> m_errors;
        String m_decodedString;

        BaseTypeMap m_builtinTypes{};
    };
}
