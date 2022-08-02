// Copyright Chad Engler

#include "he/schema/schema.h"

#include "he/core/enum_ops.h"
#include "he/core/memory_ops.h"

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
            case schema::DeclKind::Constant: return "Constant";
            case schema::DeclKind::Enum: return "Enum";
            case schema::DeclKind::File: return "File";
            case schema::DeclKind::Interface: return "Interface";
            case schema::DeclKind::Struct: return "Struct";
            case schema::DeclKind::_Count: return "_Count";
        }

        return "<unknown>";
    }
}
