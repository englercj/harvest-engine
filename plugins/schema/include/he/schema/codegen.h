// Copyright Chad Engler

#pragma once

#include "he/core/string.h"
#include "he/schema/code_writer.h"
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

    bool GenerateCpp(const SchemaDef& def, const CodeGenOptions& options, CodeWriter& writer);
}
