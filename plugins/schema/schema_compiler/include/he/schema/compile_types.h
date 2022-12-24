// Copyright Chad Engler

#pragma once

#include "he/core/hash_table.h"
#include "he/core/types.h"
#include "he/schema/ast.h"
#include "he/schema/schema.h"

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
        Type::Data::UnionTag tag;
        const AstNode* type;
        AstTypeParamRef typeParamRef;
    };

    struct TypeKeyHasher
    {
        [[nodiscard]] constexpr uint64_t operator()(const TypeKey& key) const noexcept
        {
            return CombineHash64(GetHashCode(key.expr), GetHashCode(key.scope));
        }
    };

    using TypeMap = HashMap<TypeKey, TypeValue, TypeKeyHasher>;
    using TypeIdMap = HashMap<TypeId, const AstNode*>;
    using DeclIdMap = HashMap<TypeId, Declaration::Builder, TypeIdHasher>;
}
