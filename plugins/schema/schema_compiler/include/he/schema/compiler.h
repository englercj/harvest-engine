// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/schema/parser.h"
#include "he/schema/schema.h"

#include "fmt/core.h"

#include <concepts>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace he::schema
{
    /// Transforms a Parser's AST into a schema object.
    class Compiler
    {
    public:
        struct ErrorInfo
        {
            he::String message;
            uint32_t line;
            uint32_t column;
        };

    public:
        Compiler(AstFile& ast, const char* fileName, Span<const Compiler> includes);

        bool Compile();

        Span<const ErrorInfo> Errors() const { return m_errors; }

        SchemaFile::Reader Schema() const { return m_schema; }

    private:
        struct MemberOrdinal
        {
            uint16_t value;
            AstFileLocation loc;
            bool operator==(const MemberOrdinal& v) const { return value == v.value; }
            bool operator<(const MemberOrdinal& v) const { return value < v.value; }
        };

    private:
        bool VerifyNode(AstNode& node);
        bool VerifyNodes(AstList<AstNode>& list);

        bool VerifyAttributes(const AstNode& node);
        bool VerifyMembers(const AstNode& node, AstNode::Kind kind);
        bool VerifyMembersOf(const AstNode& node, AstNode::Kind kind, std::set<MemberOrdinal>& ordinals);
        bool VerifyMethodParams(AstNode& node, const AstMethodParams& params);
        bool VerifyOrdinal(const AstNode& node);
        bool VerifyType(const AstExpression& ast, const AstNode& scope, bool isGenericParam = false);
        bool VerifyTypeArraySize(const AstExpression& ast, const AstNode& scope);
        bool VerifyTypeId(AstNode& node);
        bool VerifyValue(const AstExpression& astType, const AstNode& scopeType, const AstExpression& astValue, const AstNode& scope);

        void CompileNode(const AstNode& node, Declaration::Builder decl);

        void CompileAttribute(const AstNode& node, Declaration::Builder decl);
        void CompileConstant(const AstNode& node, Declaration::Builder decl);
        void CompileEnum(const AstNode& node, Declaration::Builder decl);
        void CompileFile(const AstNode& node, Declaration::Builder decl);
        void CompileInterface(const AstNode& node, Declaration::Builder decl);
        void CompileStruct(const AstNode& node, Declaration::Builder decl);

        void CompileField(const AstNode& node, Field::Builder field, uint16_t index);
        TypeId CompileMethodParams(
            const AstNode& node,
            const AstNode& child,
            const AstMethodParams& params,
            List<Declaration>::Builder children,
            uint16_t& childIndex,
            StringView suffix);

        Type::Builder CreateType(const AstExpression& ast, const AstNode& scope);
        Brand::Builder CreateTypeBrand(const AstExpression& name, const AstNode& scope);
        Value::Builder CreateValue(Type::Reader type, const AstExpression& ast, const AstNode& scope);
        List<Attribute>::Builder CreateAttributes(const AstList<AstAttribute>& ast, const AstNode& scope);
        List<String>::Builder CreateTypeParams(const AstList<AstTypeParam>& ast);

        bool DecodeBlob(const AstExpression& ast, he::Vector<uint8_t>& out);
        bool DecodeString(const AstExpression& ast, he::String& out);

        const AstNode* FindNode(TypeId id) const;
        const AstNode* FindNode(const AstExpression& name, const AstNode& scope) const;
        const AstNode* FindNode(StringView name, const AstNode& scope) const;

        struct AstTypeParamRef { const AstNode* scope{ nullptr }; uint16_t index{ 0xffff }; };
        AstTypeParamRef FindTypeParam(StringView name, const AstNode& scope) const;

        uint16_t GetArraySize(const AstExpression& ast, const AstNode& scope) const;

        template <std::integral T>
        bool SetInt(const AstFileLocation& location, T value, Type::Data::Reader type, Value::Data::Builder data);

        template <typename... Args>
        void AddError(const AstFileLocation& loc, fmt::format_string<Args...> fmt, Args&&... args);

    private:
        struct TypeKey
        {
            const AstExpression* expr;
            const AstNode* scope;

            bool operator==(const TypeKey& x) const { return x.expr == expr && x.scope == scope; }
            bool operator!=(const TypeKey& x) const { return x.expr != expr || x.scope != scope; }
        };

        struct TypeValue
        {
            Type::Data::Tag tag;
            const AstNode* type;
            AstTypeParamRef typeParamRef;
        };

        struct TypeKeyHasher
        {
            size_t operator()(const TypeKey& key) const
            {
                const uintptr_t expr = reinterpret_cast<uintptr_t>(key.expr);
                const uintptr_t scope = reinterpret_cast<uintptr_t>(key.scope);

                return std::hash<uintptr_t>()(expr) ^ std::hash<uintptr_t>()(scope);
            }
        };

        using TypeMap = std::unordered_map<TypeKey, TypeValue, TypeKeyHasher>;

    private:
        AstFile& m_ast;
        const char* m_fileName;
        Span<const Compiler> m_includes;

        Builder m_builder;
        SchemaFile::Builder m_schema;

        std::unordered_map<StringView, Type::Data::Tag> m_builtinTypeMap;
        std::unordered_map<TypeId, AstNode*> m_typeIdMap;
        TypeMap m_typeMap;

        Vector<ErrorInfo> m_errors;
    };
}
