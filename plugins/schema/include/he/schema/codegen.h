// Copyright Chad Engler

#pragma once

#include "he/schema/schema.h"

#include <unordered_map>

namespace he::schema
{
    using DeclIdMap = std::unordered_map<TypeId, const Declaration*, IdHasher>;

    enum class CodegenLang
    {
        Cpp,
    };

    struct CodeGenRequest
    {
        /// Construct a new code generation request.
        CodeGenRequest(const SchemaFile& schema);

        /// Lookup a declaration by ID, assert that is is valid, and return a reference to it.
        const Declaration& GetDecl(uint64_t id) const;

        /// The schema to compile.
        /// This is set by the constructor.
        const SchemaFile& schema;

        /// Map of declarations in the schema by ID. Includes all declarations in imports as well.
        /// This is set by the constructor.
        const DeclIdMap declsById;

        /// File name of the input schema file.
        const char* fileName{ nullptr };

        /// Path to the output directory to write generated file into.
        const char* outDir{ nullptr };
    };

    bool GenerateEcho(const CodeGenRequest& request);
    bool GenerateCpp(const CodeGenRequest& request);
}
