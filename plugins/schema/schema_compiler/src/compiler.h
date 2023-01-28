// Copyright Chad Engler

#pragma once

#include "parser.h"

#include "he/core/string.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/schema/schema.h"

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
        Value::Builder CreateValue(Type::Builder type, const AstExpression& ast, const AstNode& scope);
        List<Attribute>::Builder CreateAttributes(const AstList<AstAttribute>& ast, const AstNode& scope);
        List<String>::Builder CreateTypeParams(const AstList<AstTypeParam>& ast);

        bool DecodeBlob(const AstExpression& ast, he::Vector<uint8_t>& out);

        uint16_t GetArraySize(const AstExpression& ast, const AstNode& scope) const;

        template <typename T> requires(IsSame<T, uint64_t> || IsSame<T, int64_t>)
        void SetInt(const AstFileLocation& location, T value, Type::Data::Builder type, Value::Data::Builder data);

        ListBuilder CreateListValue(const Type::Builder elementType, const AstExpression& ast, const AstNode& scope);
        StructBuilder CreateStructValue(const Type::Data::Struct::Builder structType, const AstExpression& ast, const AstNode& scope);

        void FillValue(Value::Builder value, Type::Builder type, const AstExpression& ast, const AstNode& scope);
        void FillStructValue(StructBuilder dst, const Declaration::Data::Struct::Builder structDecl, const AstExpression& ast, const AstNode& scope);
        void FillStructField(StructBuilder dst, const Type::Data::Builder type, uint16_t index, uint32_t dataOffset, const AstExpression& ast, const AstNode& scope, bool markDataField);
        void FillUnionValue(StructBuilder dst, const Declaration::Data::Struct::Builder structDecl, const AstExpression& ast, const AstNode& scope);

        bool ReadBoolValue(const AstExpression& ast, const AstNode& scope) const;
        uint16_t ReadEnumValue(const AstExpression& ast, const AstNode& scope) const;

        template <typename OutType, typename InType>
        OutType ReadIntValue(const AstFileLocation& location, InType value);

        template <typename T> T ReadIntValue(const AstExpression& ast, const AstNode& scope);
        template <typename T> T ReadFloatValue(const AstExpression& ast, const AstNode& scope);

        void TrackDecl(const AstFileLocation& location, Declaration::Builder decl);

        const AstNode* TryResolveConstant(const AstExpression& ast, const AstNode& scope);

    private:
        struct PendingValue
        {
            const AstExpression* ast;
            const AstNode* scope;

            Type::Builder type;
            Value::Builder value;
        };

    private:
        CompileContext* m_context{ nullptr };
        Builder m_builder{};
        bool m_valid{ true };

        Vector<PendingValue> m_pendingValues{};
    };
}
