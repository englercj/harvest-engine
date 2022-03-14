// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/schema/ast.h"
#include "he/schema/compile_types.h"

#include <set>
#include <unordered_map>

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
            bool operator==(const MemberOrdinal& v) const { return value == v.value; }
            bool operator<(const MemberOrdinal& v) const { return value < v.value; }
        };

    private:
        bool VerifyNode(const AstNode& node);
        bool VerifyNodes(const AstList<AstNode>& list);

        bool VerifyAttributes(const AstNode& node);
        bool VerifyMembers(const AstNode& node, AstNode::Kind kind);
        bool VerifyMembersOf(const AstNode& node, AstNode::Kind kind, std::set<MemberOrdinal>& ordinals);
        bool VerifyMethodParams(const AstNode& node, const AstMethodParams& params);
        bool VerifyOrdinal(const AstNode& node);
        bool VerifyType(const AstExpression& ast, const AstNode& scope, bool isGenericParam = false);
        bool VerifyTypeArraySize(const AstExpression& ast, const AstNode& scope);
        bool VerifyTypeId(const AstNode& node);
        bool VerifyValue(const AstExpression& astType, const AstNode& scopeType, const AstExpression& astValue, const AstNode& scope);

        AstTypeParamRef FindTypeParam(StringView name, const AstNode& scope) const;

    private:
        class CompileContext* m_context{ nullptr };
        std::unordered_map<StringView, Type::Data::Tag> m_builtinTypeMap{};
    };
}
