// Copyright Chad Engler

#pragma once

#include "parser.h"

#include "he/core/string.h"
#include "he/core/types.h"
#include "he/schema/schema.h"

#include "fmt/core.h"

#include <concepts>
#include <optional>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace he::schema
{
    class CompileContext;

    /// Transforms a Parser's AST into a schema object.
    class Compiler
    {
    public:
        bool Compile(const AstFile& ast, CompileContext& ctx);

        const Builder& Schema() const { return m_builder; }

    private:
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

        uint16_t GetArraySize(const AstExpression& ast, const AstNode& scope) const;

        template <std::integral T>
        bool SetInt(const AstFileLocation& location, T value, Type::Data::Reader type, Value::Data::Builder data);

        void SetStructValues(Declaration::Data::Struct::Reader typeStructDecl, const AstList<AstTupleParam>& params, StructBuilder st, const AstNode& scope);

        template <typename BuilderType>
        void SetValue(
            const AstExpression& ast,
            Type::Reader type,
            const AstNode& scope,
            BuilderType obj,
            uint16_t index,
            uint32_t dataOffset);

    private:
        CompileContext* m_context{ nullptr };
        Builder m_builder{};
        bool m_valid{ true };
    };
}
