// Copyright Chad Engler

#pragma once

#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he::schema
{
    // Declaration and scope identifiers
    constexpr StringView KW_Alias = "alias";
    constexpr StringView KW_Attribute = "attribute";
    constexpr StringView KW_Const = "const";
    constexpr StringView KW_Enum = "enum";
    constexpr StringView KW_Enumerator = "enumerator";
    constexpr StringView KW_Extends = "extends";
    constexpr StringView KW_Field = "field";
    constexpr StringView KW_File = "file";
    constexpr StringView KW_Group = "group";
    constexpr StringView KW_Import = "import";
    constexpr StringView KW_Interface = "interface";
    constexpr StringView KW_Method = "method";
    constexpr StringView KW_Namespace = "namespace";
    constexpr StringView KW_Parameter = "parameter";
    constexpr StringView KW_Stream = "stream";
    constexpr StringView KW_Struct = "struct";
    constexpr StringView KW_Union = "union";

    // Types
    constexpr StringView KW_AnyPointer = "AnyPointer";
    constexpr StringView KW_Blob = "Blob";
    constexpr StringView KW_Bool = "bool";
    constexpr StringView KW_Float32 = "float32";
    constexpr StringView KW_Float64 = "float64";
    constexpr StringView KW_Int8 = "int8";
    constexpr StringView KW_Int16 = "int16";
    constexpr StringView KW_Int32 = "int32";
    constexpr StringView KW_Int64 = "int64";
    constexpr StringView KW_Uint8 = "uint8";
    constexpr StringView KW_Uint16 = "uint16";
    constexpr StringView KW_Uint32 = "uint32";
    constexpr StringView KW_Uint64 = "uint64";
    constexpr StringView KW_String = "String";
    constexpr StringView KW_Void = "void";

    // Values
    constexpr StringView KW_False = "false";
    constexpr StringView KW_True = "true";
}
