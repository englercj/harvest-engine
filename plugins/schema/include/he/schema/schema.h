// Copyright Chad Engler

#pragma once

#include "he/core/enum_ops.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/vector.h"

namespace he::schema
{
    constexpr uint16_t InvalidSchemaIndex = 0xffff;

    enum class BaseType : uint8_t
    {
        Unknown,

        // Arithmetic types
        Bool,
        Int8,
        Int16,
        Int32,
        Int64,
        Uint8,
        Uint16,
        Uint32,
        Uint64,
        Float32,
        Float64,

        // Containers
        Array,
        List,
        Map,
        Pointer,
        Set,
        String,
        Vector,

        // User-defined types
        Alias,
        Enum,
        Interface,
        Struct,
        Union,
    };

    enum class AttributeTarget : uint32_t
    {
        None        = 0,
        Const       = 1 << 0,
        Enum        = 1 << 1,
        EnumValue   = 1 << 2,
        Field       = 1 << 3,
        File        = 1 << 4,
        Interface   = 1 << 5,
        Method      = 1 << 6,
        Parameter   = 1 << 7,
        Struct      = 1 << 8,
        Union       = 1 << 9,

        All         = 0xffffffff,
    };
    HE_ENUM_FLAGS(AttributeTarget);

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

    union BasicValue
    {
        bool b;
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        float f32;
        double f64;
    };

    struct ObjectDef
    {
        enum class Type
        {
            Alias,
            Attribute,
            Const,
            Enum,
            Interface,
            Struct,
            Union,
        };

        Type type;
        uint32_t index;
    };

    struct Value
    {
        Value(Allocator& allocator)
            : str(allocator)
        {}

        BasicValue basic{ .u64 = 0 };
        String str;
    };

    struct Attribute
    {
        Attribute(Allocator& allocator)
            : name(allocator)
            , parameters(allocator)
        {}

        // Name of the attribute
        String name;

        // Value of the attribute
        Vector<Value> parameters;
    };

    struct Type
    {
        Type(Allocator& allocator)
            : name(allocator)
            , typeParams(allocator)
        {}

        Type(const Type&) = delete;
        Type& operator=(const Type&) = delete;

        Type(Type&& x)
            : base(Exchange(x.base, BaseType::Unknown))
            , fixedSize(Exchange(x.fixedSize, uint16_t(0)))
            , name(Move(x.name))
            , element(Exchange(x.element, nullptr))
            , key(Exchange(x.key, nullptr))
            , typeParams(Move(x.typeParams))
        {}

        Type& operator=(Type&& x)
        {
            base = Exchange(x.base, BaseType::Unknown);
            fixedSize = Exchange(x.fixedSize, uint16_t(0));
            name = Move(x.name);
            element = Exchange(x.element, nullptr);
            key = Exchange(x.key, nullptr);
            typeParams = Move(x.typeParams);
            return *this;
        }

        ~Type()
        {
            Allocator& alloc = typeParams.GetAllocator();

            if (element)
                alloc.Delete(element);

            if (key)
                alloc.Delete(key);

            for (Type* t : typeParams)
            {
                alloc.Delete(t);
            }
        }

        Type& AddTypeParam()
        {
            Allocator& alloc = typeParams.GetAllocator();
            Type* t = alloc.New<Type>(alloc);
            typeParams.PushBack(t);
            return *t;
        }

        // The base type of this item.
        BaseType base{ BaseType::Unknown };

        // Size of a fixed array when `base` is `Array`.
        uint16_t fixedSize{ 0 };

        // Name of the AliasDef when `base` is `Alias`.
        // Name of the EnumDef when `base` is `Enum`.
        // Name of the InterfaceDef when `base` is `Interface`.
        // Name of the StructDef when `base` is `Struct`.
        // Name of the constant used as the fixed size when `base` is `Array`.
        String name;

        // Element type if `base` is `Array`, `List`, `Map`, `Pointer`, `Set`, or `Vector`.
        Type* element{ nullptr };

        // Key type, if `base` is `Map`.
        Type* key{ nullptr };

        // Parameters passed in angle brackets for generics
        Vector<Type*> typeParams;
    };

    struct AttributeDef
    {
        AttributeDef(Allocator& allocator)
            : type(allocator)
            , name(allocator)
        {}

        Type type;
        AttributeTarget targets{ AttributeTarget::None };
        String name;
    };

    struct FieldDef
    {
        FieldDef(Allocator& allocator)
            : type(allocator)
            , name(allocator)
            , attributes(allocator)
            , defaultValue(allocator)
        {}

        uint32_t id{ 0 };
        String name;
        Type type;
        Vector<Attribute> attributes;

        Value defaultValue;
    };

    struct EnumValueDef
    {
        EnumValueDef(Allocator& allocator)
            : name(allocator)
            , value(allocator)
            , attributes(allocator)
        {}

        String name;
        Value value;
        Vector<Attribute> attributes;
    };

    struct EnumDef
    {
        EnumDef(Allocator& allocator)
            : name(allocator)
            , attributes(allocator)
            , values(allocator)
        {}

        BaseType base{ BaseType::Unknown };
        String name;
        Vector<Attribute> attributes;
        Vector<EnumValueDef> values;
    };

    struct AliasDef
    {
        AliasDef(Allocator& allocator)
            : type(allocator)
            , name(allocator)
            , attributes(allocator)
            , typeParams(allocator)
        {}

        Type type;
        String name;
        Vector<Attribute> attributes;
        Vector<String> typeParams;
    };

    struct ConstDef
    {
        ConstDef(Allocator& allocator)
            : name(allocator)
            , attributes(allocator)
            , value(allocator)
        {}

        BaseType base{ BaseType::Unknown };
        String name;
        Vector<Attribute> attributes;

        Value value;
    };

    struct UnionDef
    {
        UnionDef(Allocator& allocator)
            : name(allocator)
        {}

        uint32_t id{ 0 };
        String name;

        Vector<Attribute> attributes;
        Vector<FieldDef> fields;
    };

    struct StructDef
    {
        StructDef(Allocator& allocator)
            : name(allocator)
            , extends(allocator)
            , attributes(allocator)
            , fields(allocator)
            , typeParams(allocator)
            , aliases(allocator)
            , consts(allocator)
            , enums(allocator)
            , structs(allocator)
            , unions(allocator)
        {}

        uint32_t id{ 0 };
        String name;
        Type extends;

        Vector<Attribute> attributes;
        Vector<FieldDef> fields;
        Vector<String> typeParams;

        Vector<ObjectDef> objects;

        Vector<AliasDef> aliases;
        Vector<ConstDef> consts;
        Vector<EnumDef> enums;
        Vector<StructDef> structs;
        Vector<UnionDef> unions;
    };

    struct MethodParamDef
    {
        MethodParamDef(Allocator& allocator)
            : type(allocator)
            , name(allocator)
            , attributes(allocator)
        {}

        Type type;
        String name;
        Vector<Attribute> attributes;
    };

    struct MethodDef
    {
        MethodDef(Allocator& allocator)
            : name(allocator)
            , attributes(allocator)
            , parameters(allocator)
            , returnType(allocator)
        {}

        uint32_t id{ 0 };
        String name;
        Vector<Attribute> attributes;
        Vector<MethodParamDef> parameters;
        Type returnType;
    };

    struct InterfaceDef
    {
        InterfaceDef(Allocator& allocator)
            : name(allocator)
            , implements(allocator)
            , attributes(allocator)
            , methods(allocator)
            , typeParams(allocator)
            , aliases(allocator)
            , consts(allocator)
            , enums(allocator)
            , structs(allocator)
        {}

        uint32_t id{ 0 };
        String name;
        Vector<Type> implements;

        Vector<Attribute> attributes;
        Vector<MethodDef> methods;
        Vector<String> typeParams;

        Vector<ObjectDef> objects;

        Vector<AliasDef> aliases;
        Vector<ConstDef> consts;
        Vector<EnumDef> enums;
        Vector<StructDef> structs;
        Vector<UnionDef> unions;
    };

    struct SchemaDef
    {
        SchemaDef(Allocator& allocator)
            : namespaceName(allocator)
            , imports(allocator)
            , aliases(allocator)
            , attributes(allocator)
            , consts(allocator)
            , enums(allocator)
            , interfaces(allocator)
            , structs(allocator)
        {}

        void MarkTypeUsed(BaseType t)
        {
            const uint64_t shift = static_cast<uint64_t>(t);
            usedTypes |= (1ull << shift);
        }

        bool IsTypeUsed(BaseType t) const
        {
            const uint64_t shift = static_cast<uint64_t>(t);
            return (usedTypes & (1ull << shift)) != 0;
        }

        uint32_t namespaceId{ 0x811c9dc5 };
        String namespaceName;

        Vector<String> imports;
        Vector<ObjectDef> objects;

        Vector<AliasDef> aliases;
        Vector<AttributeDef> attributes;
        Vector<ConstDef> consts;
        Vector<EnumDef> enums;
        Vector<InterfaceDef> interfaces;
        Vector<StructDef> structs;
        Vector<UnionDef> unions;

        uint64_t usedTypes{ 0 };
    };

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
