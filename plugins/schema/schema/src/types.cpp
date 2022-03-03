// Copyright Chad Engler

#include "he/schema/schema.h"

#include "he/core/enum_ops.h"

namespace he
{
    template <>
    const char* AsString(schema::PointerKind x)
    {
        switch (x)
        {
            case schema::PointerKind::Struct: return "Struct";
            case schema::PointerKind::List: return "List";
            case schema::PointerKind::_Count: return "_Count";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(schema::ElementSize x)
    {
        switch (x)
        {
            case schema::ElementSize::Void: return "Void";
            case schema::ElementSize::Bit: return "Bit";
            case schema::ElementSize::Byte: return "Byte";
            case schema::ElementSize::TwoBytes: return "TwoBytes";
            case schema::ElementSize::FourBytes: return "FourBytes";
            case schema::ElementSize::EightBytes: return "EightBytes";
            case schema::ElementSize::Pointer: return "Pointer";
            case schema::ElementSize::Composite: return "Composite";
            case schema::ElementSize::_Count: return "_Count";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(schema::DeclKind x)
    {
        switch (x)
        {
            case schema::DeclKind::Attribute: return "Attribute";
            case schema::DeclKind::Const: return "Const";
            case schema::DeclKind::Enum: return "Enum";
            case schema::DeclKind::File: return "File";
            case schema::DeclKind::Interface: return "Interface";
            case schema::DeclKind::Struct: return "Struct";
            case schema::DeclKind::_Count: return "_Count";
        }

        return "<unknown>";
    }

    // --------------------------------------------------------------------------------------------
    // TODO: MOVE THESE TO GENERATED CODE

    template <>
    const char* AsString(schema::Type::Data::Tag x)
    {
        switch (x)
        {
            case schema::Type::Data::Tag::Void: return "Void";
            case schema::Type::Data::Tag::Bool: return "Bool";
            case schema::Type::Data::Tag::Int8: return "Int8";
            case schema::Type::Data::Tag::Int16: return "Int16";
            case schema::Type::Data::Tag::Int32: return "Int32";
            case schema::Type::Data::Tag::Int64: return "Int64";
            case schema::Type::Data::Tag::Uint8: return "Uint8";
            case schema::Type::Data::Tag::Uint16: return "Uint16";
            case schema::Type::Data::Tag::Uint32: return "Uint32";
            case schema::Type::Data::Tag::Uint64: return "Uint64";
            case schema::Type::Data::Tag::Float32: return "Float32";
            case schema::Type::Data::Tag::Float64: return "Float64";
            case schema::Type::Data::Tag::Blob: return "Blob";
            case schema::Type::Data::Tag::String: return "String";
            case schema::Type::Data::Tag::Array: return "Array";
            case schema::Type::Data::Tag::List: return "List";
            case schema::Type::Data::Tag::Enum: return "Enum";
            case schema::Type::Data::Tag::Struct: return "Struct";
            case schema::Type::Data::Tag::Interface: return "Interface";
            case schema::Type::Data::Tag::AnyPointer: return "AnyPointer";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(schema::Declaration::Data::Tag x)
    {
        switch (x)
        {
            case schema::Declaration::Data::Tag::File: return "File";
            case schema::Declaration::Data::Tag::Attribute: return "Attribute";
            case schema::Declaration::Data::Tag::Constant: return "Const";
            case schema::Declaration::Data::Tag::Enum: return "Enum";
            case schema::Declaration::Data::Tag::Interface: return "Interface";
            case schema::Declaration::Data::Tag::Struct: return "Struct";
        }

        return "<unknown>";
    }
}
