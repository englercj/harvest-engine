// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/schema/ast.h"
#include "he/schema/schema.h"

#include <unordered_map>

namespace he::schema
{
    struct AstTypeParamRef
    {
        const AstNode* scope{ nullptr };
        uint16_t index{ 0xffff };
    };

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
    using TypeIdMap = std::unordered_map<TypeId, const AstNode*>;
}
