// Copyright Chad Engler

#include "he/schema/schema.h"

#include "he/core/enum_ops.h"

namespace he::schema
{
    Brand::Scope::~Scope()
    {
        for (Type* param : params)
            Allocator::GetDefault().Delete(param);
    }

    Type::~Type()
    {
        switch (kind)
        {
            case TypeKind::Array:
                HE_ASSERT(list_.elementType == nullptr);
                Allocator::GetDefault().Delete(array_.elementType);
                break;
            case TypeKind::List:
                HE_ASSERT(array_.elementType == nullptr);
                Allocator::GetDefault().Delete(list_.elementType);
                break;
        }
    }

    Type::Type(Type&& x)
    {
        *this = Move(x);
    }

    Type& Type::operator=(Type&& x)
    {
        if (this != &x)
        {
            kind = Exchange(x.kind, TypeKind::Void);
            any_.paramScopeId = Exchange(x.any_.paramScopeId, 0);
            any_.paramIndex = Exchange(x.any_.paramIndex, uint16_t(0));
            array_.size = Exchange(x.array_.size, uint16_t(0));
            array_.elementType = Exchange(x.array_.elementType, nullptr);
            list_.elementType = Exchange(x.list_.elementType, nullptr);
            enum_.id = Exchange(x.enum_.id, 0);
            enum_.brand = Move(x.enum_.brand);
            interface_.id = Exchange(x.interface_.id, 0);
            interface_.brand = Move(x.interface_.brand);
            struct_.id = Exchange(x.struct_.id, 0);
            struct_.brand = Move(x.struct_.brand);
        }
        return *this;
    }

    Value::~Value()
    {
        switch (kind)
        {
            case TypeKind::Array:
                for (Value* avalue : array)
                    Allocator::GetDefault().Delete(avalue);
            case TypeKind::List:
                for (Value* lvalue : list)
                    Allocator::GetDefault().Delete(lvalue);
                break;
            case TypeKind::Struct:
                for (StructValue& svalue : struct_)
                    Allocator::GetDefault().Delete(svalue.value);
                break;

            default: break;
        }
    }

    Import::~Import()
    {
        Allocator::GetDefault().Delete(schema);
    }
}

namespace he
{
    template <>
    const char* AsString(schema::DeclKind kind)
    {
        switch (kind)
        {
            case schema::DeclKind::None: return "None";
            case schema::DeclKind::Attribute: return "Attribute";
            case schema::DeclKind::Const: return "Const";
            case schema::DeclKind::Enum: return "Enum";
            case schema::DeclKind::File: return "File";
            case schema::DeclKind::Interface: return "Interface";
            case schema::DeclKind::Struct: return "Struct";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(schema::TypeKind kind)
    {
        switch (kind)
        {
            case schema::TypeKind::Void: return "Void";
            case schema::TypeKind::Bool: return "Bool";
            case schema::TypeKind::Int8: return "Int8";
            case schema::TypeKind::Int16: return "Int16";
            case schema::TypeKind::Int32: return "Int32";
            case schema::TypeKind::Int64: return "Int64";
            case schema::TypeKind::Uint8: return "Uint8";
            case schema::TypeKind::Uint16: return "Uint16";
            case schema::TypeKind::Uint32: return "Uint32";
            case schema::TypeKind::Uint64: return "Uint64";
            case schema::TypeKind::Float32: return "Float32";
            case schema::TypeKind::Float64: return "Float64";
            case schema::TypeKind::Array: return "Array";
            case schema::TypeKind::Blob: return "Blob";
            case schema::TypeKind::String: return "String";
            case schema::TypeKind::List: return "List";
            case schema::TypeKind::Enum: return "Enum";
            case schema::TypeKind::Struct: return "Struct";
            case schema::TypeKind::Interface: return "Interface";
            case schema::TypeKind::AnyPointer: return "AnyPointer";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(schema::PointerKind kind)
    {
        switch (kind)
        {
            case schema::PointerKind::Struct: return "Struct";
            case schema::PointerKind::List: return "List";
        }

        return "<unknown>";
    }
}
