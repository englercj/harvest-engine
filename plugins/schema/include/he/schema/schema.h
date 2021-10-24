// Copyright Chad Engler

#pragma once

#include "he/core/enum_ops.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/vector.h"
#include "he/schema/reflection.generated.h"

namespace he::schema
{
    constexpr bool IsIntegral(BaseType t)
    {
        switch (t)
        {
            case BaseType::Bool:
            case BaseType::Int8:
            case BaseType::Int16:
            case BaseType::Int32:
            case BaseType::Int64:
            case BaseType::Uint8:
            case BaseType::Uint16:
            case BaseType::Uint32:
            case BaseType::Uint64:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsUnsignedIntegral(BaseType t)
    {
        switch (t)
        {
            case BaseType::Bool:
            case BaseType::Uint8:
            case BaseType::Uint16:
            case BaseType::Uint32:
            case BaseType::Uint64:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsFloat(BaseType t)
    {
        switch (t)
        {
            case BaseType::Float32:
            case BaseType::Float64:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsArithmetic(BaseType t)
    {
        return IsIntegral(t) || IsFloat(t);
    }

    constexpr bool IsObject(BaseType t)
    {
        switch (t)
        {
            case BaseType::Array:
            case BaseType::List:
            case BaseType::Map:
            case BaseType::Pointer:
            case BaseType::Set:
            case BaseType::String:
            case BaseType::Vector:
            case BaseType::Interface:
            case BaseType::Struct:
            case BaseType::Union:
                return true;
            default:
                return false;
        }
    }

    inline void MarkTypeUsed(SchemaDef& def, BaseType t)
    {
        const uint64_t shift = static_cast<uint64_t>(t);
        def.usedTypesMask |= (1ull << shift);
    }

    inline bool IsTypeUsed(const SchemaDef& def, BaseType t)
    {
        const uint64_t shift = static_cast<uint64_t>(t);
        return (def.usedTypesMask & (1ull << shift)) != 0;
    }

    // Helper to linearly search for an attribute
    inline const Attribute* FindAttribute(StringView name, Span<const Attribute> attributes)
    {
        for (const Attribute& a : attributes)
        {
            if (a.name == name)
                return &a;
        }

        return nullptr;
    }

    inline bool HasAttribute(StringView name, Span<const Attribute> attributes)
    {
        return FindAttribute(name, attributes) != nullptr;
    }
}
