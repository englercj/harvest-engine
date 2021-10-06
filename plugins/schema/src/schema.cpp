// Copyright Chad Engler

#include "he/schema/schema.h"

#include "he/core/enum_ops.h"

namespace he
{
    template <>
    const char* AsString(schema::BaseType x)
    {
        switch (x)
        {
            case schema::BaseType::Unknown: return "Unknown";
            case schema::BaseType::Bool: return "Bool";
            case schema::BaseType::Int8: return "Int8";
            case schema::BaseType::Int16: return "Int16";
            case schema::BaseType::Int32: return "Int32";
            case schema::BaseType::Int64: return "Int64";
            case schema::BaseType::Uint8: return "Uint8";
            case schema::BaseType::Uint16: return "Uint16";
            case schema::BaseType::Uint32: return "Uint32";
            case schema::BaseType::Uint64: return "Uint64";
            case schema::BaseType::Float32: return "Float32";
            case schema::BaseType::Float64: return "Float64";
            case schema::BaseType::Array: return "Array";
            case schema::BaseType::List: return "List";
            case schema::BaseType::Map: return "Map";
            case schema::BaseType::Pointer: return "Pointer";
            case schema::BaseType::Set: return "Set";
            case schema::BaseType::String: return "String";
            case schema::BaseType::Vector: return "Vector";
            case schema::BaseType::Alias: return "Alias";
            case schema::BaseType::Enum: return "Enum";
            case schema::BaseType::Interface: return "Interface";
            case schema::BaseType::Struct: return "Struct";
        }

        return "<unknown>";
    }
}
