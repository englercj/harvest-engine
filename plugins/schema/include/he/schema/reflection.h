// Copyright Chad Engler

#pragma once

#include "he/schema/schema.h"

namespace he::schema::reflection
{
    enum class DeclKind
    {
        Unknown,
        Attribute,
        Field,
        Struct,
        Value,
    };

    struct ReflectionDecl
    {
        DeclKind kind{ DeclKind::Unknown };
    };

    struct ValueDecl : ReflectionDecl
    {
        ValueDecl() { kind = DeclKind::Value; }

        BasicValue
    };

    struct AttributeDecl : ReflectionDecl
    {
        AttributeDecl() { kind = DeclKind::Attribute; }

        const char* name;
    };

    struct FieldDecl : ReflectionDecl
    {
        FieldDecl() { kind = DeclKind::Field; }

        const char* name;
    };

    struct StructDecl : ReflectionDecl
    {
        StructDecl() { kind = DeclKind::Struct; }

        const char* name;
        StructDecl* extends;
        AttributeDecl* attributes;
    };
}
