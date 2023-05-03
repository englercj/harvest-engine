// Copyright Chad Engler

#pragma once

#include "he/core/hash_table.h"
#include "he/core/rb_tree.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/schema/ast.h"
#include "he/schema/compile_types.h"

namespace he::schema
{
    class Verifier
    {
    public:
        bool Verify(const AstFile& ast, class CompileContext& ctx);

    private:
        struct MemberOrdinal
        {
            uint16_t value;
            AstFileLocation loc;
            [[nodiscard]] bool operator==(const MemberOrdinal& v) const { return value == v.value; }
            [[nodiscard]] bool operator<(const MemberOrdinal& v) const { return value < v.value; }
            [[nodiscard]] constexpr uint64_t HashCode() const;
        };

    private:
        bool VerifyNode(const AstNode& node);
        bool VerifyNodes(const AstList<AstNode>& list);

        bool VerifyAttributes(const AstNode& node);
        bool VerifyMembers(const AstNode& node, AstNode::Kind kind);
        bool VerifyMembersOf(const AstNode& node, AstNode::Kind kind, RBTreeSet<MemberOrdinal>& ordinals);
        bool VerifyMethodParams(const AstNode& node, const AstMethodParams& params);
        bool VerifyDeclName(const AstNode& node);
        bool VerifyFieldName(const AstNode& node);
        bool VerifyOrdinal(const AstNode& node);
        bool VerifyType(const AstExpression& ast, const AstNode& scope, bool isGenericParam = false);
        bool VerifyTypeArraySize(const AstExpression& ast, const AstNode& scope);
        bool VerifyTypeId(const AstNode& node);
        bool VerifyValue(const AstExpression& astType, const AstNode& scopeType, const AstExpression& astValue, const AstNode& scope);

        bool VerifyTupleValue(const AstNode* node, const AstExpression& astValue, const AstNode& scopeValue);

        AstTypeParamRef FindTypeParam(StringView name, const AstNode& scope) const;
        uint16_t GetArraySize(const AstExpression& ast, const AstNode& scope) const;

    private:
        class CompileContext* m_context{ nullptr };
        HashMap<StringView, Type::Data::UnionTag> m_builtinTypeMap{};
    };
}
