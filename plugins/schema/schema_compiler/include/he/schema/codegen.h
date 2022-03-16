// Copyright Chad Engler

#pragma once

#include "he/core/enum_ops.h"
#include "he/schema/compile_types.h"
#include "he/schema/schema.h"

#include <unordered_map>

namespace he::schema
{
    class Builder;

    enum class CodegenTarget
    {
        None = 0,
        Echo = 1 << 0,
        Cpp = 1 << 1,
    };
    HE_ENUM_FLAGS(CodegenTarget);

    struct CodeGenRequest
    {
        /// Lookup a declaration by ID, assert that is is valid, and return a reference to it.
        Declaration::Reader GetDecl(uint64_t id) const { return declsById->at(id); }

        /// Raw data that was built by the compiler.
        Span<const Word> schemaData;

        /// The schema to compile.
        SchemaFile::Reader schemaFile;

        /// File name of the input schema file.
        const char* fileName{ nullptr };

        /// Path to the output directory to write generated file into.
        const char* outDir{ nullptr };

        /// Map of declarations in the schema by ID. Includes all declarations in imports as well.
        DeclIdMap* declsById{ nullptr };
    };

    bool GenerateEcho(const CodeGenRequest& request);
    bool GenerateCpp(const CodeGenRequest& request);
}
