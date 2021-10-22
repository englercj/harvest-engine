// Copyright Chad Engler

#pragma once

#include "he/schema/schema.h"

namespace he::schema
{
    enum class CodegenLang
    {
        Cpp,
    };

    struct CodeGenOptions
    {
        /// File name of the input schema file.
        const char* fileName{ nullptr };

        /// Path to the output directory to write generated file into.
        const char* outDir{ nullptr };

        /// When true generates code to use services in GRPC.
        /// NOT YET IMPLEMENTED
        bool grpc{ false };

        /// When true generates code to serialize structures into JSON.
        bool json{ false };

        /// When true generates code to serialize structures into binary Buffers.
        bool buffer{ false };

        /// When true generates additional code for reflection and dynamic data access.
        bool reflection{ false };

        /// When true generates zero-copy buffer operations only. This means there is no native
        /// structure, only a buffer reader and builder.
        bool zeroCopy{ false };
    };

    bool GenerateCpp(const SchemaDef& def, const CodeGenOptions& options);
}
