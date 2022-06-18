// Copyright Chad Engler

#pragma once

#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he::schema
{
    // Declaration and scope identifiers
    inline constexpr StringView KW_Alias = "alias";
    inline constexpr StringView KW_Attribute = "attribute";
    inline constexpr StringView KW_Const = "const";
    inline constexpr StringView KW_Enum = "enum";
    inline constexpr StringView KW_Enumerator = "enumerator";
    inline constexpr StringView KW_Extends = "extends";
    inline constexpr StringView KW_Field = "field";
    inline constexpr StringView KW_File = "file";
    inline constexpr StringView KW_Group = "group";
    inline constexpr StringView KW_Import = "import";
    inline constexpr StringView KW_Interface = "interface";
    inline constexpr StringView KW_Method = "method";
    inline constexpr StringView KW_Namespace = "namespace";
    inline constexpr StringView KW_Parameter = "parameter";
    inline constexpr StringView KW_Stream = "stream";
    inline constexpr StringView KW_Struct = "struct";
    inline constexpr StringView KW_Union = "union";

    // Types
    inline constexpr StringView KW_AnyPointer = "AnyPointer";
    inline constexpr StringView KW_Blob = "Blob";
    inline constexpr StringView KW_Bool = "bool";
    inline constexpr StringView KW_Float32 = "float32";
    inline constexpr StringView KW_Float64 = "float64";
    inline constexpr StringView KW_Int8 = "int8";
    inline constexpr StringView KW_Int16 = "int16";
    inline constexpr StringView KW_Int32 = "int32";
    inline constexpr StringView KW_Int64 = "int64";
    inline constexpr StringView KW_Uint8 = "uint8";
    inline constexpr StringView KW_Uint16 = "uint16";
    inline constexpr StringView KW_Uint32 = "uint32";
    inline constexpr StringView KW_Uint64 = "uint64";
    inline constexpr StringView KW_String = "String";
    inline constexpr StringView KW_Void = "void";

    // Values
    inline constexpr StringView KW_False = "false";
    inline constexpr StringView KW_True = "true";
}
