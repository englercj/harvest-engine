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

        /// When true also generates code to serialize structures into JSON.
        bool json{ false };

        /// When true generates zero-copy buffer formats. This format can be more efficient
        /// for reading data, but is much more difficult to mutate.
        /// NOT YET IMPLEMENTED
        bool buffer{ false };
    };

    bool GenerateCpp(const SchemaDef& def, const CodeGenOptions& options);
}
