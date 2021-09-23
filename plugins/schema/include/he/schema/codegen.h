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
        const char* fileName{ nullptr };
        const char* outDir{ nullptr };
    };

    bool GenerateCpp(const SchemaDef& def, const CodeGenOptions& options);
}
